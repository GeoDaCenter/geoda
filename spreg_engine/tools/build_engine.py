#!/usr/bin/env python3
"""Build the downloadable spreg engine archive.

    tools/build_engine.py --outdir dist                 # for this machine
    tools/build_engine.py --outdir dist --manifest-entry dist/entry.json

What it does, per platform:

  1. fetch a standalone, relocatable CPython (python-build-standalone, via `uv`);
  2. install the hash-pinned wheel set from solver/requirements.lock into it;
  3. prune caches and test suites, then byte-compile with hash-based
     invalidation so the archive stays byte-for-byte reproducible;
  4. write engine.json (versions, protocol, executable list) and collect the
     bundled packages' licences;
  5. zip it with fixed timestamps and print the sha256 to put in the manifest.

The archive is what GeoDa downloads and unpacks into
    <user data>/GeoDa/engines/spreg-<spreg>-py<major><minor>/
so the payload sits at the archive root:

    python/            standalone interpreter + site-packages
    engine.json        versions and protocol, read by GeoDa after unpacking
    licenses/          licences of the bundled packages

Requires `uv` on PATH (https://docs.astral.sh/uv) - it is what knows how to
fetch and lay out python-build-standalone builds.  Run one instance per
platform in CI; each produces one archive.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import stat
import subprocess
import sys
import sysconfig
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SOLVER = ROOT / "solver"
FIXED_DATE = (1980, 1, 1, 0, 0, 0)

# directories that are dead weight in a shipped engine
PRUNE_DIRS = ["__pycache__", "tests", "test", "testing", "doc", "docs", "benchmarks",
              "examples", "scripts"]
PRUNE_FILES = ["*.pyc", "*.pyo", "*.a", "*.lib", "*.pdb", "*.c", "*.h", "*.pxd", "*.pyx"]


def platform_key() -> str:
    system = sys.platform
    machine = platform.machine().lower()
    if system == "darwin":
        return "macos-arm64" if machine in ("arm64", "aarch64") else "macos-x86_64"
    if system.startswith("win"):
        return "windows-x86_64" if machine in ("amd64", "x86_64") else "windows-" + machine
    if system.startswith("linux"):
        arch = {"x86_64": "x86_64", "amd64": "x86_64", "aarch64": "aarch64"}.get(machine, machine)
        return "linux-" + arch
    return "%s-%s" % (system, machine)


def run(cmd, **kw) -> subprocess.CompletedProcess:
    print("+ %s" % " ".join(str(c) for c in cmd), flush=True)
    return subprocess.run([str(c) for c in cmd], check=True, **kw)


def fetch_python(python_version: str, workdir: Path) -> Path:
    """Standalone, relocatable CPython, copied out of uv's managed store."""
    install_dir = workdir / "python-store"
    run(["uv", "python", "install", python_version, "--install-dir", str(install_dir)])
    # uv also leaves a convenience symlink next to the real build; resolve it
    builds = sorted({p.resolve() for p in install_dir.iterdir() if p.is_dir()},
                    key=lambda p: p.name)
    if not builds:
        raise SystemExit("uv did not install a python into %s" % install_dir)
    return builds[-1]


def prune(staging: Path) -> None:
    site_packages = next(staging.glob("python/lib/python*/site-packages"), None)
    if site_packages is None:
        raise SystemExit("no site-packages under %s" % staging)

    for path in list(site_packages.rglob("*")):
        if path.is_dir() and path.name in PRUNE_DIRS:
            # numpy/testing, pandas/tests, ... but never a package's own __init__
            if (path / "__init__.py").exists() and path.name != "__pycache__":
                continue
            shutil.rmtree(path, ignore_errors=True)
    for pattern in PRUNE_FILES:
        for path in site_packages.rglob(pattern):
            try:
                path.unlink()
            except OSError:
                pass

    # Console scripts (pip, f2py, ...) are wrappers whose shebang is the build
    # directory: useless once the engine lives somewhere else, and a source of
    # build-to-build differences.  `python3 -m pip` still works without them.
    for sub, keep in (("bin", ("python",)), ("Scripts", ())):
        directory = staging / "python" / sub
        if not directory.is_dir():
            continue
        for path in sorted(directory.iterdir()):
            if path.is_file() and not path.name.startswith(keep):
                path.unlink()


def collect_licenses(site_packages: Path, target: Path) -> None:
    target.mkdir(parents=True, exist_ok=True)
    count = 0
    for dist in sorted(site_packages.glob("*.dist-info")):
        name = re.sub(r"-\d.*$", "", dist.name)
        copied = False
        for pattern in ("licenses/*", "LICENSE*", "COPYING*", "NOTICE*"):
            for src in sorted(dist.glob(pattern)):
                if src.is_file():
                    dest = target / ("%s-%s" % (name, src.name))
                    shutil.copyfile(src, dest)
                    copied = count = count + 1
            if copied:
                break
    print("   collected %d licence file(s)" % count)


def compile_bytecode(python: Path, site_packages: Path) -> None:
    """Hash-invalidated .pyc: deterministic bytes, fast cold start.

    PYTHONHASHSEED matters: a handful of modules marshal a frozenset, whose
    iteration order otherwise follows the randomized string hash, which would
    make those .pyc files - and so the archive - differ from build to build.
    """
    run([python, "-m", "compileall", "-q", "-j", str(os.cpu_count() or 2),
         "--invalidation-mode", "unchecked-hash", str(site_packages)],
        env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1", "PYTHONHASHSEED": "0"})


def write_archive(staging: Path, outfile: Path) -> tuple[int, int, str]:
    """Deterministic zip: sorted entries, fixed timestamps, unix modes kept."""
    entries = sorted(p for p in staging.rglob("*"))
    with zipfile.ZipFile(outfile, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path in entries:
            rel = path.relative_to(staging).as_posix()
            if path.is_dir():
                continue
            info = zipfile.ZipInfo(rel, date_time=FIXED_DATE)
            mode = 0o755 if path.stat().st_mode & stat.S_IXUSR else 0o644
            info.external_attr = (stat.S_IFREG | mode) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with open(path, "rb") as fh:
                zf.writestr(info, fh.read())

    digest = hashlib.sha256()
    size = 0
    with open(outfile, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            digest.update(chunk)
            size += len(chunk)
    unpacked = sum(p.stat().st_size for p in staging.rglob("*") if p.is_file())
    return size, unpacked, digest.hexdigest()


def executables_list(staging: Path) -> list[str]:
    """Paths GeoDa must chmod +x after unpacking (wx does not restore modes)."""
    names = []
    for sub in ("bin", "Scripts"):                    # posix / windows layouts
        directory = staging / "python" / sub
        if directory.is_dir():
            for path in sorted(directory.iterdir()):
                if path.name.startswith("python"):
                    names.append("python/%s/%s" % (sub, path.name))
    for name in ("python.exe", "pythonw.exe"):
        if (staging / "python" / name).exists():
            names.append("python/" + name)
    return names


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--outdir", default="dist", help="where to write the archive")
    parser.add_argument("--python", default="3.13", help="standalone CPython series to fetch")
    parser.add_argument("--platform", default=None, help="override the platform key")
    parser.add_argument("--requirements", default=str(SOLVER / "requirements.lock"))
    parser.add_argument("--manifest-entry", default=None,
                        help="also write the manifest entry for this platform here")
    parser.add_argument("--no-bytecode", action="store_true",
                        help="skip precompilation: ~40%% smaller download, slower first start")
    parser.add_argument("--build-dir", default=None,
                        help="fixed staging directory (default <outdir>/.build).  It must be "
                             "fixed: the absolute path is baked into _sysconfigdata and into "
                             "every .pyc, so a changing build directory changes the archive")
    parser.add_argument("--keep-staging", action="store_true")
    args = parser.parse_args()

    key = args.platform or platform_key()
    outdir = Path(args.outdir).resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    requirements = Path(args.requirements)
    locked = {}
    for line in requirements.read_text().splitlines():
        m = re.match(r"^([A-Za-z0-9_.\-]+)==([^\s\\]+)", line)
        if m:
            locked[m.group(1).lower()] = m.group(2)
    spreg_version = locked.get("spreg")
    if not spreg_version:
        raise SystemExit("solver/requirements.lock does not pin spreg")

    workdir = Path(args.build_dir) if args.build_dir else outdir / ".build"
    shutil.rmtree(workdir, ignore_errors=True)
    staging = workdir / "engine"
    staging.mkdir(parents=True)
    print("== building the spreg engine for %s (spreg %s)" % (key, spreg_version))

    try:
        build = fetch_python(args.python, workdir)
        python_src = build / ("python" if (build / "python").is_dir() else ".")
        shutil.copytree(python_src, staging / "python", symlinks=False)
        python = staging / "python" / "bin" / ("python.exe" if os.name == "nt" else "python3")
        if not python.exists():
            raise SystemExit("no interpreter at %s" % python)

        # uv marks interpreters it manages; a shipped engine must not carry that
        for marker in staging.rglob("EXTERNALLY-MANAGED"):
            marker.unlink()

        # --no-compile: we compile in the next step, with a fixed hash seed
        run(["uv", "pip", "install", "--python", str(python), "--require-hashes",
             "--only-binary", ":all:", "--no-compile", "-r", str(requirements)])
        site_packages = Path(run([python, "-c",
                                   "import sysconfig;print(sysconfig.get_paths()['purelib'])"
                                   ], capture_output=True, text=True).stdout.strip())
        print("   site-packages: %s" % site_packages)

        prune(staging)
        if not args.no_bytecode:
            compile_bytecode(python, site_packages)
        collect_licenses(site_packages, staging / "licenses")

        match = re.search(r'SOLVER_VERSION\s*=\s*"([^"]+)"', (SOLVER / "solve.py").read_text())
        engine = {
            "engine": 1,
            "protocol": 1,
            "solver": match.group(1) if match else "unknown",
            "spreg": spreg_version,
            "python": platform.python_version(),
            "platform": key,
            "executables": executables_list(staging),
        }
        if os.environ.get("SOURCE_DATE_EPOCH"):          # keep the archive reproducible
            engine["built"] = os.environ["SOURCE_DATE_EPOCH"]
        (staging / "engine.json").write_text(json.dumps(engine, indent=2) + "\n")

        py_tag = "%d%d" % (sys.version_info.major, sys.version_info.minor)
        archive = outdir / ("geoda-spreg-%s-py%s-%s.zip" % (spreg_version, py_tag, key))
        size, unpacked, sha = write_archive(staging, archive)

        entry = {
            "file": archive.name,
            "url": "https://github.com/GeoDaCenter/software/releases/download/"
                   "spreg-engine-v1/" + archive.name,
            "sha256": sha,
            "size_bytes": size,
            "unpacked_bytes": unpacked,
            "format": "zip",
            "python": platform.python_version(),
            "spreg": spreg_version,
            "executables": engine["executables"],
        }
        if args.manifest_entry:
            Path(args.manifest_entry).write_text(json.dumps({key: entry}, indent=2) + "\n")

        print()
        print("archive : %s" % archive)
        print("size    : %.1f MB compressed, %.1f MB unpacked" % (size / 1e6, unpacked / 1e6))
        print("sha256  : %s" % sha)
        print("entry   : %s" % json.dumps({key: entry}, indent=2))
        return 0
    finally:
        if args.keep_staging:
            print("staging kept at %s" % staging)
        else:
            shutil.rmtree(workdir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
