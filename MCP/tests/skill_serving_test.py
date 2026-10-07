#!/usr/bin/env python3
"""Integration test for the skill the GeoDa MCP server serves about itself.

Clients are handed the skill two ways and neither needs anything installed
outside the app:

  * `initialize` returns it in the `instructions` field, which is the only
    channel that reaches a client before it calls anything;
  * `skill/list` + `skill/get` return it as tools, which is what an agent
    needs because Claude Code exposes MCP resources only as user @-mentions;
  * `resources/list` + `resources/read` serve the same text as the
    `skill://spatial-analysis-workbook` resource, and `prompts/list` +
    `prompts/get` as the workbook prompt.

The last check is that the served skill actually tells the reader how to get a
data set open (`file/open`), since the tools act on the project in the app.

Usage:
    GEODA_BIN=/path/to/GeoDa python3 skill_serving_test.py

Optional env vars: MCP_TEST_PORT (default 8765), GEODA_BIN.
"""

import json
import os
import subprocess
import sys
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
SKILL_URI = "skill://spatial-analysis-workbook"


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
                "clientInfo": {"name": "skill-serving-test", "version": "1.0"},
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


def main():
    if not os.path.isfile(GEODA_BIN):
        sys.exit("GeoDa binary not found: %s\nSet GEODA_BIN to point at it."
                 % GEODA_BIN)

    proc = None
    try:
        # No data set: the skill is what the client needs before it has one.
        print("launching: %s --mcp-port %d" % (GEODA_BIN, PORT))
        env = dict(os.environ)
        proj = provision_proj_resources(GEODA_BIN)
        if proj:
            env["PROJ_DATA"] = proj
            env["PROJ_LIB"] = proj
        proc = subprocess.Popen([GEODA_BIN, "--mcp-port", str(PORT)], env=env)
        wait_for_server(proc)

        # initialize hands the client the instructions, before any other call.
        init = rpc("initialize", {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "skill-serving-test", "version": "1.0"},
        })
        instructions = init.get("instructions", "")
        check(SKILL_URI in instructions,
              "initialize instructions name the skill URI")
        check("skill/get" in instructions,
              "initialize instructions say how to read the skill")
        check("file/open" in instructions,
              "initialize instructions say how to open a data set")

        # The skill is a tool, so an agent can read it without @-mentioning a
        # resource.
        tools = {t["name"]: t for t in rpc("tools/list", {})["tools"]}
        check("skill/list" in tools, "skill/list is listed")
        check("skill/get" in tools, "skill/get is listed")
        check("uri" in tools["skill/get"]["inputSchema"].get("properties", {}),
              "skill/get takes a uri parameter")

        listed = call_tool("skill/list", {})
        skills = {
            s.get("uri"): s for s in listed.get("resources", [])
        }
        check(SKILL_URI in skills, "skill/list returns the workbook skill")
        check(skills.get(SKILL_URI, {}).get("mimeType") == "text/markdown",
              "the skill is markdown")

        got = call_tool("skill/get", {"uri": SKILL_URI})
        contents = got.get("contents", [])
        check(len(contents) == 1, "skill/get returns one content block")
        text = contents[0].get("text", "") if contents else ""
        check(len(text) > 2000, "the skill text is the whole document")
        check(text.startswith("# Spatial Data Analysis"),
              "the skill text is the workbook")
        check("FIRST: Always load this skill" in text,
              "the skill still opens with the load-me-first section")
        check("file/open" in text,
              "the skill says how to open a data set")
        check("file/close" in text, "the skill says how to switch data sets")

        # The resource and prompt routes serve the same text.
        resources = {
            r.get("uri"): r for r in rpc("resources/list", {})["resources"]
        }
        check(SKILL_URI in resources,
              "the same skill is advertised as an MCP resource")
        read = rpc("resources/read", {"uri": SKILL_URI})
        res_text = read["contents"][0].get("text", "")
        check(res_text == text,
              "resources/read and skill/get serve identical text")

        prompts = [p["name"] for p in rpc("prompts/list", {})["prompts"]]
        check("spatial-analysis-workbook" in prompts,
              "the workbook prompt is still served")
        prompt = rpc("prompts/get", {"name": "spatial-analysis-workbook"})
        prompt_text = prompt["messages"][0]["content"][0]["text"]
        check("skill/get" in prompt_text,
              "the prompt points at the tool route too")
        check("file/open" in prompt_text,
              "the prompt says how to open a data set")

        # An unknown URI is an error, not an empty skill.
        try:
            call_tool("skill/get", {"uri": "skill://nope"}, rid=9)
            raise AssertionError("FAIL: unknown skill URI was not rejected")
        except McpError as e:
            check("Unknown skill" in str(e), "unknown skill URI rejected: %s" % e)
        try:
            call_tool("skill/get", {}, rid=10)
            raise AssertionError("FAIL: missing uri was not rejected")
        except McpError as e:
            check("Missing required parameter" in str(e),
                  "missing uri rejected: %s" % e)

        print("\nALL SKILL SERVING TESTS PASSED")
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


if __name__ == "__main__":
    main()
