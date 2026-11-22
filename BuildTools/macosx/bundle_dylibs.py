#!/usr/bin/env python3
"""
Bundle the runtime dylib dependencies of GeoDa.app into Contents/Frameworks so
the app is self-contained and portable.

The CMake build links GeoDa against homebrew (or other) dylibs by absolute
path. Those paths exist only on the build machine, so the .app would fail to
launch anywhere else. This script mirrors the legacy install_name.py step of
the old BuildTools toolchain: copy every non-system dylib (recursively, so
transitive deps like GDAL's libarchive/openssl/libtiff chain are included) into
Contents/Frameworks and rewrite every load command to @executable_path/../Frameworks/.

Usage:
    bundle_dylibs.py <GeoDa.app> [codesign-identity]

The optional second argument is a codesign identity (e.g. "Developer ID
Application: ... (TEAMID)"). When given, each bundled dylib is re-signed with
it; when omitted, dylibs are ad-hoc signed (codesign -s -) so the app launches
on the build machine. install_name_tool and codesign must be on PATH.
"""

import os
import re
import shutil
import subprocess
import sys

SYSTEM_PREFIXES = ("/usr/lib/", "/System/", "/usr/local/lib/libSystem")

NEW_REF = "@executable_path/../Frameworks/"


def log(msg):
    print(f"[bundle_dylibs] {msg}")


def run(cmd):
    subprocess.run(cmd, check=True)


def otool_deps(path):
    """Return the load-command dylib references of `path` (excluding its own id)."""
    out = subprocess.check_output(["otool", "-L", path], text=True)
    deps = []
    for line in out.splitlines()[1:]:
        m = re.match(r"\t(.+?) \(", line)
        if m:
            deps.append(m.group(1))
    return deps


def get_rpaths(path):
    """Return the LC_RPATH entries of `path`."""
    out = subprocess.check_output(["otool", "-l", path], text=True)
    rpaths = []
    for line in out.splitlines():
        m = re.search(r"^\s*path (.+?) \(", line)
        if m:
            rpaths.append(m.group(1).strip())
    return rpaths


def resolve(dep, current_dir, executable_dir, rpaths):
    """Resolve a load-command reference to a filesystem path, or None."""
    if dep.startswith("@loader_path/"):
        return os.path.join(current_dir, dep[len("@loader_path/"):])
    if dep.startswith("@executable_path/"):
        return os.path.join(executable_dir, dep[len("@executable_path/"):])
    if dep.startswith("@rpath/"):
        rel = dep[len("@rpath/"):]
        for rp in rpaths:
            cand = os.path.join(expand_path_var(rp, current_dir, executable_dir), rel)
            if os.path.exists(cand):
                return cand
        return None
    # Absolute path (the common case for homebrew dylibs).
    return dep


def expand_path_var(path, current_dir, executable_dir):
    """Expand @loader_path/@executable_path inside an LC_RPATH entry.

    Homebrew dylibs carry LC_RPATHs like `@loader_path/../lib` (e.g. geos' C
    API references its C++ sibling libgeos via @rpath) or the bare
    `@loader_path` (abseil dylibs reference each other that way); these resolve
    against the dylib's own directory, not the app's.
    """
    if path == "@loader_path":
        return current_dir
    if path.startswith("@loader_path/"):
        return os.path.join(current_dir, path[len("@loader_path/"):])
    if path == "@executable_path":
        return executable_dir
    if path.startswith("@executable_path/"):
        return os.path.join(executable_dir, path[len("@executable_path/"):])
    return path


def main():
    app = sys.argv[1] if len(sys.argv) > 1 else None
    codesign_id = sys.argv[2] if len(sys.argv) > 2 else None
    if not app:
        log("usage: bundle_dylibs.py <GeoDa.app> [codesign-identity]")
        sys.exit(2)

    contents = os.path.join(app, "Contents")
    binary = os.path.join(contents, "MacOS", "GeoDa")
    frameworks = os.path.join(contents, "Frameworks")
    if not os.path.isfile(binary):
        log(f"error: {binary} not found")
        sys.exit(1)
    os.makedirs(frameworks, exist_ok=True)

    executable_dir = os.path.dirname(binary)
    processed = set()
    bundled = []  # dylibs we copied, to codesign afterwards

    def process(path, source=None):
        """Copy deps of `path` into Frameworks and rewrite its load commands.

        `path` is the file being rewritten (the main binary, or a dylib already
        copied into Frameworks). `source` is the original filesystem location
        that `path` was copied from (or None for the binary); @loader_path /
        @rpath references must be resolved against it, since LC_RPATH entries
        like `@loader_path/../lib` point at the source directory (e.g. the
        homebrew lib dir), not the bundle.
        """
        path = os.path.abspath(path)
        if path in processed:
            return
        processed.add(path)
        current_dir = os.path.dirname(source if source else path)
        rpaths = get_rpaths(path)
        for dep in otool_deps(path):
            if dep.startswith(SYSTEM_PREFIXES):
                continue
            if dep.startswith("@executable_path/../Frameworks/"):
                continue  # already bundled in a previous run
            resolved = resolve(dep, current_dir, executable_dir, rpaths)
            if not resolved or not os.path.exists(resolved):
                log(f"warning: cannot resolve {dep} (referenced by {os.path.basename(path)}); skipping")
                continue
            # Canonicalize so symlink-vs-versioned names of the same dylib
            # (homebrew wxWidgets ships both libwx_osx_cocoau_core-3.2.dylib
            # and ...-3.2.0.2.2.dylib) deduplicate to one bundled copy; two
            # copies would register the same ObjC classes twice and crash.
            resolved = os.path.realpath(resolved)
            name = os.path.basename(resolved)
            dest = os.path.join(frameworks, name)
            if not os.path.exists(dest):
                log(f"copy {resolved} -> Frameworks/{name}")
                shutil.copy2(resolved, dest, follow_symlinks=True)
                bundled.append(dest)
            new_ref = NEW_REF + name
            if dep != new_ref:
                log(f"  -change {dep} -> {new_ref}  ({os.path.basename(path)})")
                run(["install_name_tool", "-change", dep, new_ref, path])
            process(dest, source=resolved)

    log(f"bundling dylibs for {app}")
    process(binary)

    # Set each bundled dylib's own install name so cross-references and any
    # not-yet-rewritten absolute refs resolve inside the bundle.
    for dylib in bundled:
        name = os.path.basename(dylib)
        target_id = NEW_REF + name
        run(["install_name_tool", "-id", target_id, dylib])
        # Re-process in case -id changed nothing about deps; then sign.
        process(dylib)
        sign_dylib(dylib, codesign_id)

    # The main binary was rewritten in place; sign it too so the app launches.
    sign_dylib(binary, codesign_id)

    log(f"done. {len(bundled)} dylibs bundled into {frameworks}")


def sign_dylib(path, codesign_id):
    # No identity -> ad-hoc sign so the app launches on the build machine.
    # CI passes the Developer ID identity and re-signs each dylib properly.
    # NOTE: `-o runtime` (hardened runtime) is only applied with a real
    # identity. Hardened runtime enables library validation, which rejects
    # ad-hoc signed dylibs (they have no Team ID to match the executable's),
    # so an ad-hoc signed bundle with -o runtime would not launch.
    ident = codesign_id or "-"
    cmd = ["codesign", "--force", "--timestamp", "-o", "runtime", "-s", ident, path]
    if not codesign_id:
        cmd = ["codesign", "--force", "--timestamp", "-s", ident, path]
    run(cmd)


if __name__ == "__main__":
    main()
