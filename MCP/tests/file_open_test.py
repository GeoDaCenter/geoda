#!/usr/bin/env python3
"""Integration test for the MCP file/open and file/close tools.

Starts GeoDa with **no data set** -- the way a user starts the app and then
asks an MCP client to load a file -- and drives the file tools over the MCP
HTTP endpoint:

  1. file/open  a GeoJSON from disk, and the project it opens is the one the
                analysis tools see afterwards (project/status, table/*)
  2. file/open  is refused while a project is open, instead of replacing it
                behind the caller's back
  3. file/close closes it, after which the next data set can be opened
  4. file/open  reports a missing path and a relative path as errors, and
                file/close reports that there is nothing to close

macOS only in practice: started without a data set, GeoDa brings up its data
source dialog on Linux and Windows (only macOS 10.14+ skips it), which this
test does not cover.

Usage:
    GEODA_BIN=/path/to/GeoDa python3 file_open_test.py

Optional env vars:
    MCP_TEST_PORT   MCP server port to use (default 8765)
    GEODA_BIN       path to the GeoDa executable
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GEODA_BIN = os.environ.get(
    "GEODA_BIN",
    os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "BuildTools",
                                  "macosx", "debug", "GeoDa")),
)
PORT = int(os.environ.get("MCP_TEST_PORT", "8765"))
BASE = "http://127.0.0.1:%d/mcp" % PORT


def sample(num_features, val_name):
    """A small polygon data set: squares near lon -100 / lat 40."""
    features = []
    for i in range(num_features):
        lon, lat = -100 + i, 40 + i
        features.append({
            "type": "Feature",
            "properties": {"id": i + 1, "name": chr(ord("a") + i),
                           val_name: 5.0 * (i + 1)},
            "geometry": {
                "type": "Polygon",
                "coordinates": [[
                    [lon, lat], [lon, lat + 1], [lon + 1, lat + 1],
                    [lon + 1, lat], [lon, lat],
                ]],
            },
        })
    return {"type": "FeatureCollection", "features": features}


class McpError(Exception):
    pass


def rpc(method, params=None, rid=1):
    """Send one JSON-RPC request, return the 'result' object."""
    payload = {"jsonrpc": "2.0", "id": rid, "method": method}
    if params is not None:
        payload["params"] = params
    body = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        BASE, data=body, headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            out = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        raise McpError("HTTP %d: %s" % (e.code,
                                        e.read().decode("utf-8", "replace")))
    if out.get("error"):
        raise McpError("JSON-RPC %s: %s" % (out["error"].get("code"),
                                            out["error"].get("message")))
    return out.get("result")


def call_tool(name, arguments, rid=1):
    """Call an MCP tool and parse the JSON text block of the result."""
    result = rpc("tools/call", {"name": name, "arguments": arguments}, rid=rid)
    blocks = result.get("content", [])
    if not blocks:
        raise McpError("tool %s returned empty content" % name)
    return json.loads(blocks[0].get("text", ""))


def wait_for_server(proc, timeout=90):
    """Poll until the MCP endpoint answers, or the app exits first."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise McpError("GeoDa exited early with code %d" % proc.returncode)
        try:
            rpc("initialize", {
                "protocolVersion": "2024-11-05",
                "capabilities": {},
                "clientInfo": {"name": "file-open-test", "version": "1.0"},
            })
            return
        except Exception:
            time.sleep(0.5)
    raise McpError("MCP server did not answer within %ds" % timeout)


def proj_lib_dir():
    """Find the PROJ data directory (proj.db)."""
    for cand in (os.environ.get("PROJ_DATA"), os.environ.get("PROJ_LIB"),
                 "/opt/homebrew/share/proj", "/usr/local/share/proj",
                 "/usr/share/proj"):
        if cand and os.path.isfile(os.path.join(cand, "proj.db")):
            return cand
    return None


def provision_proj_resources(geoda_bin):
    """GeoDa.cpp hardcodes OSRSetPROJSearchPaths to <exe>/../Resources/proj
    and the debug build does not ship that directory, so reprojection fails
    unless it is provisioned. Returns the dir or None."""
    parent = os.path.dirname(os.path.dirname(os.path.abspath(geoda_bin)))
    target = os.path.join(parent, "Resources", "proj")
    if os.path.isfile(os.path.join(target, "proj.db")):
        return target
    proj = proj_lib_dir()
    if not proj:
        return None
    os.makedirs(os.path.dirname(target), exist_ok=True)
    if not os.path.lexists(target):
        os.symlink(proj, target)
    return target


def check(cond, msg):
    if not cond:
        raise AssertionError("FAIL: " + msg)
    print("  ok: " + msg)


def expect_error(name, arguments, needle, msg, rid=1):
    """The tool must fail, and say something that mentions `needle`."""
    try:
        call_tool(name, arguments, rid=rid)
    except McpError as e:
        check(needle in str(e), "%s: %s" % (msg, e))
        return
    raise AssertionError("FAIL: %s (no error was raised)" % msg)


def main():
    if not os.path.isfile(GEODA_BIN):
        sys.exit("GeoDa binary not found: %s\nSet GEODA_BIN to point at it."
                 % GEODA_BIN)

    tmp = tempfile.mkdtemp(prefix="geoda-mcp-open-")
    proc = None
    try:
        first = os.path.join(tmp, "first.geojson")
        second = os.path.join(tmp, "second.geojson")
        with open(first, "w", encoding="utf-8") as f:
            json.dump(sample(3, "val"), f)
        with open(second, "w", encoding="utf-8") as f:
            json.dump(sample(2, "other"), f)

        # No data set on the command line: the client opens the file itself.
        print("launching: %s --mcp-port %d" % (GEODA_BIN, PORT))
        env = dict(os.environ)
        proj = provision_proj_resources(GEODA_BIN)
        if proj:
            env["PROJ_DATA"] = proj
            env["PROJ_LIB"] = proj
        proc = subprocess.Popen([GEODA_BIN, "--mcp-port", str(PORT)], env=env)
        wait_for_server(proc)

        # 0. The tools are advertised with the schema the test needs.
        listed = rpc("tools/list", {})["tools"]
        tools = {t["name"]: t for t in listed}
        check("file/open" in tools, "file/open is listed")
        check("path" in tools["file/open"]["inputSchema"].get("properties", {}),
              "file/open takes a path parameter")
        check("file/close" in tools, "file/close is listed")

        # 1. Nothing is open before the first open.
        status = call_tool("project/status", {})
        check(status.get("open") is False,
              "no project is open before file/open")

        # 2. Open the first data set from disk.
        res = call_tool("file/open", {"path": first}, rid=2)
        check(res.get("open") is True, "file/open reports the project as open")
        check(res.get("title") == "first", "file/open reports the title")
        check(res.get("num_records") == 3, "file/open reports 3 records")
        check(res.get("num_columns") == 3, "file/open reports 3 columns")

        # 3. The opened project is the one the other tools work on.
        status = call_tool("project/status", {})
        check(status.get("open") is True, "project/status sees the new project")
        # project/status reports the .gda file path, which a data source
        # project does not have; its title comes from the file name.
        check(status.get("title") == "first",
              "project/status reports the opened data set: %s"
              % status.get("title"))
        cols = call_tool("table/list_columns", {})
        names = sorted(c["name"] for c in cols.get("columns", []))
        check(names == ["id", "name", "val"],
              "the analysis tools see the opened table: %s" % names)

        # 4. A second open is refused while a project is open -- the caller has
        #    to close it, so nothing is replaced silently.
        expect_error("file/open", {"path": second}, "already open",
                     "second file/open is refused while a project is open",
                     rid=3)
        status = call_tool("project/status", {})
        check(status.get("title") == "first",
              "the refused open left the first project in place")

        # 5. Close, then the second data set opens.
        res = call_tool("file/close", {}, rid=4)
        check(res.get("open") is False, "file/close reports the project closed")
        status = call_tool("project/status", {})
        check(status.get("open") is False, "project/status sees it closed")
        res = call_tool("file/open", {"path": second}, rid=5)
        check(res.get("num_records") == 2,
              "the second data set opens after file/close")
        cols = call_tool("table/list_columns", {})
        names = sorted(c["name"] for c in cols.get("columns", []))
        check(names == ["id", "name", "other"],
              "the second table replaced the first: %s" % names)

        # 6. Errors are reported, not turned into dialogs.
        expect_error("file/open", {"path": os.path.join(tmp, "nope.geojson")},
                     "not found", "a missing file is reported", rid=6)
        expect_error("file/open", {"path": "relative.geojson"},
                     "must be absolute", "a relative path is reported", rid=7)
        status = call_tool("project/status", {})
        check(status.get("title") == "second",
              "a failed open left the open project alone")
        call_tool("file/close", {}, rid=8)
        expect_error("file/close", {}, "No project is open",
                     "file/close with nothing open is reported", rid=9)

        print("\nALL FILE OPEN TESTS PASSED")
    finally:
        if proc is not None:
            if proc.poll() is not None:
                print("note: GeoDa exited during the test (code %d)"
                      % proc.returncode)
            else:
                proc.terminate()
                try:
                    proc.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
