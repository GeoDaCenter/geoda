#!/usr/bin/env python3
"""Type-check GeoDa sources without building the application.

    tools/syntax_check.py Regression/SpregEngine.cpp DialogTools/SpregEngineDlg.cpp
    tools/syntax_check.py --build-dir ~/github/geoda/build <files...>

It lifts the compile flags CMake already used for a file in the same directory
out of the build's `compile_commands.json` and runs the compiler with
`-fsyntax-only`, so a new source file can be checked in a second instead of
rebuilding the app.  Useful for anything that only adds files, which is exactly
what the spreg work does.

Configure the build once with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` (the CMake
build in this repository already does).
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import shlex
import subprocess
import sys


def find_build_dir(explicit: str | None) -> pathlib.Path:
    candidates = []
    if explicit:
        candidates.append(pathlib.Path(explicit).expanduser())
    env = os.environ.get("GEODA_BUILD")
    if env:
        candidates.append(pathlib.Path(env).expanduser())
    here = pathlib.Path(__file__).resolve().parent.parent.parent  # repo root
    candidates += [here / "build", pathlib.Path.home() / "github/geoda/build"]
    for path in candidates:
        if (path / "compile_commands.json").is_file():
            return path
    raise SystemExit(
        "no compile_commands.json found; pass --build-dir or set GEODA_BUILD"
    )


def flags_for(build_dir: pathlib.Path, repo_root: pathlib.Path, source: pathlib.Path):
    """The flags used for some existing file in the same directory."""
    entries = json.loads((build_dir / "compile_commands.json").read_text())
    same_dir = [
        e for e in entries
        if pathlib.Path(e["file"]).parent == source.parent
        and pathlib.Path(e["file"]) != source.resolve()
    ]
    pool = same_dir or entries
    if not pool:
        raise SystemExit("compile_commands.json is empty")
    # prefer a plain translation unit over a generated one
    entry = min(pool, key=lambda e: len(e["file"]))
    args = shlex.split(entry["command"])
    out = []
    skip_next = False
    for arg in args:
        if skip_next:
            skip_next = False
            continue
        if arg in ("-o", "-c"):
            skip_next = True
            continue
        if arg.endswith(".cpp") or arg.endswith(".o"):
            continue
        if arg in ("-fsyntax-only",):
            continue
        out.append(arg)
    # a stale absolute path in the build dir should not matter, but make sure the
    # repository root the caller is working in comes first
    out = ["-I" + str(repo_root) if a == "-I" + str(entry["directory"]) else a for a in out]
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("files", nargs="+", help="sources to check")
    parser.add_argument("--build-dir", default=None)
    parser.add_argument("--quiet", action="store_true", help="only show failures")
    args = parser.parse_args()

    build_dir = find_build_dir(args.build_dir)
    repo_root = pathlib.Path(__file__).resolve().parent.parent.parent
    failures = 0

    for name in args.files:
        source = (repo_root / name).resolve()
        if not source.is_file():
            print("no such file: %s" % source)
            failures += 1
            continue
        flags = flags_for(build_dir, repo_root, source)
        cmd = [flags[0], "-fsyntax-only"] + flags[1:] + [str(source)]
        if not args.quiet:
            print("checking %s" % name, flush=True)
        result = subprocess.run(cmd, capture_output=True, text=True)
        lines = result.stderr.splitlines()
        # errors decide; warnings are shown when they are in the repository's own
        # code (wxWidgets and vendored headers are noisy under a new clang)
        errors = [line for line in lines if " error" in line]
        warnings = [line for line in lines
                    if "warning:" in line and str(repo_root) in line
                    and ".xpm" not in line]
        if errors:
            print("\n".join(errors))
            failures += 1
        else:
            if warnings and not args.quiet:
                print("\n".join(warnings))
            if not args.quiet:
                print("  ok")

    print("\n%d file(s) checked, %d with problems" % (len(args.files), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
