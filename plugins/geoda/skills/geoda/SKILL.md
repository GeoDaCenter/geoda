---
name: geoda
description: Set up and connect to GeoDa so this agent can run spatial analysis in it over MCP — maps and plots as live windows, spatial weights, Moran's I and LISA, rate smoothing, PCA, clustering and regionalization. Use when the user asks to explore or analyze spatial data "in GeoDa" / "with GeoDa", when a request needs GeoDa's spatial statistics, or when the geoda MCP tools are not yet available in this session.
---

# GeoDa

**GeoDa** is the desktop app for spatial data analysis from the Center for Spatial
Data Science (Luc Anselin, University of Chicago). It hosts a local MCP server.
Connected, this agent can open a data set, build choropleth maps and plots as
live GeoDa windows, create spatial weights matrices, and run Moran's I, LISA,
rate smoothing, PCA and clustering by calling the app's own commands.

## Step 0 — Are we already connected?

If this session exposes the `geoda` MCP tools (`list_command`, `execute_command`),
the connection is up: **go straight to "Step 1 — Load the workbook skill"**.

If not, the app is not running, or the client was not restarted after it was set
up. Both are quick to check.

## Step 1 — Load the workbook skill

**Do this first, before any other command.** GeoDa serves a workflow guide —
Luc Anselin's *Exploring Spatial Data with GeoDa: A Workbook*, mapped onto the
app's commands — and it says how to work through each analysis:

```
execute_command {name: "skill/get", arguments: {uri: "skill://spatial-analysis-workbook"}}
```

The same text also arrives in the `instructions` field of `initialize`, as the
resource `skill://spatial-analysis-workbook`, and as the prompt
`spatial-analysis-workbook`. Load it once per session and follow it.

## Step 2 — Start GeoDa, if it is not running

The MCP server only answers while the app is running. Start it the usual way:

- **macOS** — `open -a GeoDa`
- **Windows** — start GeoDa from the Start menu
- **Linux** — run the `geoda` AppImage

Then wait a few seconds. A tool call now either works, or answers with

> GeoDa is not reachable at http://127.0.0.1:8765/mcp - start GeoDa (it serves
> MCP while it runs) and retry.

which means the app is still not up — start it and call again. (GeoDa starts
its server on port 8765 when it launches, and falls back to a nearby port when
that one is taken; the launcher reads the port the app actually bound from
`~/.geoda/mcp.json`.)

If GeoDa *is* running and the tools still do not appear, the client has not
loaded the server: a client only sees the tools of the servers it started with.
Restart the client.

## Step 3 — If the tools are still missing, set the app up

GeoDa installs itself into the client: **Options → MCP → Install MCP Plugin…**
gives a button for Claude Code and one for Codex, and reports what it did step
by step. The same thing by hand — the marketplace is GeoDa's own repository, so
the `--sparse` paths keep the client from checking out the whole application:

```bash
claude plugin marketplace add GeoDaCenter/geoda --sparse .claude-plugin .agents plugins
claude plugin install -y geoda@geoda
```

```bash
codex plugin marketplace add GeoDaCenter/geoda --sparse .claude-plugin --sparse .agents --sparse plugins
codex plugin add geoda@geoda
```

Restart the client afterwards. (Without a plugin loader, GeoDa's MCP menu also
shows the endpoint URL — `http://127.0.0.1:8765/mcp` — for a client that takes a
URL, though a command is what lets this agent see the tools rather than a URL it
would have to call by hand.)

## Step 4 — Driving GeoDa

Two tools, always:

- `list_command` — the catalog: every command's id, its menu group, what it
  does, and its parameters. Call it when unsure rather than guessing an id.
- `execute_command {name: "<id>", arguments: {...}}` — run one command.

The shape of a session: `project/status` to see whether a data set is open, then
`file/open {path}` if not (an absolute path; `~` and `${VAR}` are expanded), then
`table/list_columns`, then the analysis. Maps and plots appear as live GeoDa
windows on the desktop, linked to one another, next to any the user already has
open. The workbook skill has the full univariate and multivariate pipelines,
including which spatial weights each statistic needs.

Some commands ask the user a question directly when a required variable is
missing (an MCP elicitation card listing the project's numeric columns), so you
do not have to ask first — pass the column if you already know it.

## Notes

- One app, one data set: the tools act on the project open in the running GeoDa.
  `file/open` replaces it; call `file/close` first when the user wants to switch.
- Long analyses (LISA with permutations, clustering) run for a while; the app
  stays responsive and reports when they are done.
