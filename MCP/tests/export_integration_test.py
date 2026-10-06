#!/usr/bin/env python3
"""Integration test for the GeoDa MCP export tools.

Launches the GeoDa binary with ``--mcp-port`` and a small GeoJSON data set
(on macOS the app window briefly appears), then exercises the export tools
over the MCP HTTP endpoint:

  1. file/export   -> GeoJSON   (whole table + geometry, EPSG:4326)
  2. table/export  -> CSV       (subset of columns, no geometry)
  3. table/export  -> GeoJSON   (subset of columns + geometry)
  4. table/export  -> GeoJSON   (subset of columns, geometry off)
  5. table/export  -> unknown column must be rejected with -32602

Each export is validated by reopening the written file (JSON or CSV) and
checking feature counts, properties and coordinate range.

Usage:
    GEODA_BIN=/path/to/debug/GeoDa python3 export_integration_test.py

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
import urllib.request
import urllib.error

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GEODA_BIN = os.environ.get(
    "GEODA_BIN",
    os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "BuildTools",
                                  "macosx", "debug", "GeoDa")),
)
PORT = int(os.environ.get("MCP_TEST_PORT", "8765"))
BASE = "http://127.0.0.1:%d/mcp" % PORT

# A tiny polygon data set: 3 squares near lon -100 / lat 40 with an int,
# a string and a real column.
FEATURES = []
for i, (lon, lat, val, name) in enumerate([
    (-100, 40, 10.5, "a"),
    (-99, 41, 20.25, "b"),
    (-101, 39, 5.0, "c"),
], start=1):
    FEATURES.append({
        "type": "Feature",
        "properties": {"id": i, "name": name, "val": val},
        "geometry": {
            "type": "Polygon",
            "coordinates": [[
                [lon, lat], [lon, lat + 1], [lon + 1, lat + 1],
                [lon + 1, lat], [lon, lat],
            ]],
        },
    })

SAMPLE = {"type": "FeatureCollection", "features": FEATURES}


def provision_proj_resources(geoda_bin):
    """GeoDa.cpp hardcodes OSRSetPROJSearchPaths to <exe>/../Resources/proj
    and the debug build does not ship that directory, so proj.db is never
    found and reprojection fails. Provision the directory as a symlink to the
    system proj data when it is missing. Returns the dir or None."""
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


def proj_lib_dir():
    """Find the PROJ data directory (proj.db). GeoDa run from a terminal
    cannot locate its bundled PROJ data, so the test points PROJ_DATA /
    PROJ_LIB at the system proj.db when it can find one."""
    if os.environ.get("PROJ_DATA") and os.path.isfile(
            os.path.join(os.environ["PROJ_DATA"], "proj.db")):
        return os.environ["PROJ_DATA"]
    if os.environ.get("PROJ_LIB") and os.path.isfile(
            os.path.join(os.environ["PROJ_LIB"], "proj.db")):
        return os.environ["PROJ_LIB"]
    for cand in ("/opt/homebrew/share/proj", "/usr/local/share/proj",
                 "/usr/share/proj"):
        if os.path.isfile(os.path.join(cand, "proj.db")):
            return cand
    return None


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
        raise McpError("HTTP %d: %s" % (e.code, e.read().decode("utf-8", "replace")))
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
    text = blocks[0].get("text", "")
    return json.loads(text)


def wait_for_server(proc, timeout=90):
    """Poll until the MCP endpoint answers, or the app exits first."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise McpError(
                "GeoDa exited early with code %d" % proc.returncode)
        try:
            rpc("initialize", {
                "protocolVersion": "2024-11-05",
                "capabilities": {},
                "clientInfo": {"name": "export-integration-test",
                               "version": "1.0"},
            })
            return
        except Exception:
            time.sleep(0.5)
    raise McpError("MCP server did not answer within %ds" % timeout)


def read_geojson(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def read_csv(path):
    with open(path, "r", encoding="utf-8") as f:
        lines = [ln.rstrip("\r\n") for ln in f if ln.strip()]
    return [ln.split(",") for ln in lines]


def check(cond, msg):
    if not cond:
        raise AssertionError("FAIL: " + msg)
    print("  ok: " + msg)


def main():
    if not os.path.isfile(GEODA_BIN):
        sys.exit("GeoDa binary not found: %s\nSet GEODA_BIN to point at it."
                 % GEODA_BIN)

    tmp = tempfile.mkdtemp(prefix="geoda-mcp-export-")
    proc = None
    try:
        data_path = os.path.join(tmp, "sample.geojson")
        with open(data_path, "w", encoding="utf-8") as f:
            json.dump(SAMPLE, f)

        print("launching: %s --mcp-port %d %s" % (GEODA_BIN, PORT, data_path))
        env = dict(os.environ)
        proj = provision_proj_resources(GEODA_BIN)
        if proj:
            env["PROJ_DATA"] = proj
            env["PROJ_LIB"] = proj
            print("  proj data: %s" % proj)
        proc = subprocess.Popen([GEODA_BIN, "--mcp-port", str(PORT),
                                 data_path], env=env)
        wait_for_server(proc)

        # 0. Sanity: the data set is loaded.
        cols = call_tool("table/list_columns", {})
        names = [c["name"] for c in cols.get("columns", [])]
        check(sorted(names) == ["id", "name", "val"],
              "data set loaded with columns id/name/val")

        # 1. file/export -> GeoJSON (whole table, EPSG:4326).
        out1 = os.path.join(tmp, "all.geojson")
        res = call_tool("file/export", {"path": out1})
        check(res.get("success") is True, "file/export returns success")
        check(res.get("num_features") == 3, "file/export writes 3 features")
        gj = read_geojson(out1)
        feats = gj["features"]
        check(len(feats) == 3, "file/export GeoJSON has 3 features")
        props = feats[0]["properties"]
        check(sorted(props.keys()) == ["id", "name", "val"],
              "file/export GeoJSON keeps all columns")
        xs = [p["geometry"]["coordinates"][0][0][0] for p in feats]
        ys = [p["geometry"]["coordinates"][0][0][1] for p in feats]
        check(min(xs) >= -102 and max(xs) <= -98,
              "file/export coordinates in lon range (EPSG:4326)")
        check(min(ys) >= 38 and max(ys) <= 42,
              "file/export coordinates in lat range (EPSG:4326)")

        # 2. table/export -> CSV (subset of columns, no geometry).
        out2 = os.path.join(tmp, "subset.csv")
        res = call_tool("table/export",
                        {"path": out2, "columns": ["id", "val"]})
        check(res.get("success") is True, "table/export CSV returns success")
        check(res.get("num_columns") == 2, "table/export CSV exports 2 columns")
        rows = read_csv(out2)
        check(rows[0] == ["id", "val"], "table/export CSV header is id,val")
        check(len(rows) == 4, "table/export CSV has header + 3 rows")
        check("name" not in rows[0], "table/export CSV excludes unselected columns")

        # 3. table/export -> GeoJSON (subset of columns + geometry).
        out3 = os.path.join(tmp, "subset.geojson")
        res = call_tool("table/export",
                        {"path": out3, "columns": ["id", "name"]})
        check(res.get("success") is True, "table/export GeoJSON returns success")
        gj = read_geojson(out3)
        check(len(gj["features"]) == 3, "table/export GeoJSON has 3 features")
        props0 = gj["features"][0]["properties"]
        check(sorted(props0.keys()) == ["id", "name"],
              "table/export GeoJSON writes only selected columns")
        check("geometry" in gj["features"][0],
              "table/export GeoJSON keeps geometry by default")

        # 4. table/export -> GeoJSON, geometry disabled.
        out4 = os.path.join(tmp, "nogeom.geojson")
        res = call_tool("table/export",
                        {"path": out4, "columns": ["id"],
                         "include_geometry": False})
        check(res.get("success") is True, "table/export geometry-off succeeds")
        gj = read_geojson(out4)
        check(len(gj["features"]) == 3, "geometry-off still writes 3 features")
        check("geometry" not in gj["features"][0] or
              gj["features"][0].get("geometry") is None,
              "geometry-off writes features without geometry")

        # 5. Unknown column is rejected, not silently dropped.
        try:
            call_tool("table/export",
                      {"path": os.path.join(tmp, "bad.geojson"),
                       "columns": ["id", "nope"]})
            raise AssertionError("FAIL: unknown column was not rejected")
        except McpError as e:
            check("nope" in str(e), "unknown column rejected: %s" % e)

        print("\nALL EXPORT INTEGRATION TESTS PASSED")
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
