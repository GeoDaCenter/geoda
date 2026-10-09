#!/usr/bin/env python3
"""Integration test for geoda-mcp, the stdio launcher GeoDa ships.

A client registers the launcher as a **command** rather than the app's HTTP
URL: a URL in the model's context invites hand-written JSON-RPC, while a
command lets the client own the transport and the model see only tools. The
launcher is what makes that work, so it has three properties worth testing:

  1. it is byte-for-byte the script the app writes when it installs the
     launcher itself -- MCP/McpClientSetup.cpp embeds a copy of
     MCP/bin/geoda-mcp for the direct route, and the two drifting apart is
     silent otherwise (the client would run an older script);
  2. with GeoDa running, one newline-delimited request in gives one JSON reply
     out and nothing else on stdout, because stdout is the frame stream;
  3. with GeoDa not running, it answers with a JSON-RPC error that names that,
     instead of hanging or dying silently -- which is what a client shows the
     user when the app is not up.

Usage:
    GEODA_BIN=/path/to/GeoDa python3 launcher_test.py

Optional env vars:
    MCP_TEST_PORT   MCP server port to use (default 8791)
    GEODA_BIN       path to the GeoDa executable
"""

import json
import os
import re
import subprocess
import sys
import time
import urllib.request

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GEODA_BIN = os.environ.get(
    "GEODA_BIN",
    os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "BuildTools",
                                  "macosx", "debug", "GeoDa")),
)
PORT = int(os.environ.get("MCP_TEST_PORT", "8791"))
LAUNCHER = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "bin", "geoda-mcp"))
SETUP_CPP = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "McpClientSetup.cpp"))

# The raw-string delimiter the embedded copy is wrapped in. Must stay within
# 16 characters: that is the C++ limit for a raw string delimiter.
DELIM = "GEODALAUNCH"


def check(cond, msg):
    if not cond:
        raise AssertionError("FAIL: " + msg)
    print("  ok: " + msg)


def embedded_launcher():
    """The copy of the launcher compiled into the app, as text."""
    with open(SETUP_CPP, encoding="utf-8") as f:
        src = f.read()
    m = re.search(r'R"%s\((.*?)\)%s"' % (DELIM, DELIM), src, re.S)
    if not m:
        raise AssertionError(
            "FAIL: no R\"%s(...)%s\" block in %s" % (DELIM, DELIM, SETUP_CPP))
    return m.group(1)


def run_launcher(frames, env_extra=None, timeout=180):
    """Feed newline-delimited frames in, return the lines that came back."""
    env = dict(os.environ)
    env.update(env_extra or {})
    proc = subprocess.run(
        [LAUNCHER],
        input="".join(json.dumps(f) + "\n" for f in frames),
        capture_output=True, text=True, env=env, timeout=timeout)
    if proc.returncode != 0:
        raise AssertionError("FAIL: launcher exited %d: %s"
                             % (proc.returncode, proc.stderr.strip()))
    return [line for line in proc.stdout.splitlines() if line.strip()]


def wait_for_server(proc, timeout=90):
    """Wait until the app is serving, and return the endpoint it bound.

    The port comes from the discovery file the app writes when it binds, not
    from the one that was asked for: that port is taken whenever something else
    already holds it, and the app then takes the next free one. Reading it here
    also checks that the file names *this* process.
    """
    path = os.path.expanduser("~/.geoda/mcp.json")
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise AssertionError("FAIL: GeoDa exited early (code %d)"
                                 % proc.returncode)
        try:
            with open(path, encoding="utf-8") as f:
                discovery = json.load(f)
        except Exception:
            time.sleep(0.5)
            continue
        if discovery.get("pid") != proc.pid:
            # Another GeoDa instance's file; wait for this one to publish its own.
            time.sleep(0.5)
            continue
        health = discovery["url"].rsplit("/", 1)[0] + "/"
        try:
            with urllib.request.urlopen(health, timeout=5) as resp:
                if b"GeoDa MCP server running" in resp.read():
                    return discovery["url"]
        except Exception:
            pass
        time.sleep(0.5)
    raise AssertionError("FAIL: the MCP server did not answer within %ds"
                         % timeout)


def main():
    print("1. the embedded launcher and MCP/bin/geoda-mcp are the same script")
    with open(LAUNCHER, encoding="utf-8") as f:
        on_disk = f.read()
    embedded = embedded_launcher()
    check(embedded == on_disk,
          "MCP/McpClientSetup.cpp embeds MCP/bin/geoda-mcp byte for byte")
    check(os.access(LAUNCHER, os.X_OK), "the launcher is executable")

    print("2. GeoDa not running: a named error, not a hang")
    # A port nothing is listening on, named explicitly so the answer does not
    # depend on whether another GeoDa happens to be running on the default one.
    dead = {"GEODA_MCP_URL": "http://127.0.0.1:%d/mcp" % (PORT + 200),
            "GEODA_MCP_DISCOVERY_FILE": os.path.join(
                SCRIPT_DIR, "no-such-discovery-file.json")}
    started = time.time()
    lines = run_launcher(
        [{"jsonrpc": "2.0", "id": 1, "method": "initialize",
          "params": {"protocolVersion": "2025-06-18", "capabilities": {},
                     "clientInfo": {"name": "launcher-test", "version": "1"}}}],
        env_extra=dead, timeout=60)
    elapsed = time.time() - started
    check(len(lines) == 1, "exactly one reply came back, got %d" % len(lines))
    reply = json.loads(lines[0])
    check(reply.get("id") == 1, "the reply carries the request id")
    err = reply.get("error", {})
    check("GeoDa is not reachable" in err.get("message", ""),
          "the error names the app: %s" % err.get("message", "")[:80])
    check(elapsed < 30, "it answered rather than hung (%.1fs)" % elapsed)

    print("3. GeoDa running: a frame in, a JSON reply out, nothing else")
    if not os.path.isfile(GEODA_BIN):
        sys.exit("GeoDa binary not found: %s\nSet GEODA_BIN to point at it."
                 % GEODA_BIN)
    env = dict(os.environ)
    proc = subprocess.Popen([GEODA_BIN, "--mcp-port", str(PORT)], env=env)
    try:
        endpoint = wait_for_server(proc)
        check(endpoint.startswith("http://127.0.0.1:"),
              "the app publishes the endpoint it bound: %s" % endpoint)
        lines = run_launcher([
            {"jsonrpc": "2.0", "id": 1, "method": "initialize",
             "params": {"protocolVersion": "2025-06-18", "capabilities": {},
                        "clientInfo": {"name": "launcher-test",
                                       "version": "1"}}},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/list", "params": {}},
            # A notification: answered 202 with no body, so nothing comes back
            # for it either -- a reply to a notification is a protocol error.
            {"jsonrpc": "2.0", "method": "notifications/initialized"},
        ])
        check(len(lines) == 2,
              "two requests, two replies (the notification is silent): %d"
              % len(lines))
        for line in lines:
            json.loads(line)  # every line on stdout has to be a frame
        check([json.loads(l)["id"] for l in lines] == [1, 2],
              "the ids come back in order")
        init = json.loads(lines[0])["result"]
        check(init.get("serverInfo", {}).get("name") == "geoda-mcp",
              "initialize is answered by the app")
        tools = [t["name"] for t in json.loads(lines[1])["result"]["tools"]]
        check(tools == ["list_command", "execute_command"],
              "tools/list comes through the launcher: %s" % tools)

        # The frames are newline-delimited JSON, so a missing trailing newline
        # on the last reply would merge it with the next one.
        check(lines[-1].endswith("}") and "\n" not in lines[-1],
              "each reply is one line")
    finally:
        proc.terminate()
        proc.wait(timeout=30)

    print("\nall launcher checks passed")


if __name__ == "__main__":
    main()
