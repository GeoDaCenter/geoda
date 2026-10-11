# GeoDa — agent plugin

**GeoDa** is a desktop app for spatial data analysis. It hosts a local MCP
server, and this directory publishes that server as a **plugin**: the client
runs a local command that talks to the running app, so the agent sees GeoDa's
commands as tools instead of being handed an endpoint URL.

Installing the plugin gives the client three things at once:

- the **MCP server** — the app's own commands (`list_command`,
  `execute_command`: maps, plots, spatial weights, Moran's I, LISA, rate
  smoothing, PCA, clustering and regionalization);
- the **launcher** (`bin/geoda-mcp`) that connects to the app over stdio, and
  reads the port the app actually bound from `~/.geoda/mcp.json`;
- the **skill**, which starts GeoDa when it is not running and points at the
  workbook guide the app itself serves.

## Install

The marketplace lives in GeoDa's own repository, at its root (`.claude-plugin`
and `.agents`) with this plugin under `plugins/geoda`. The app is a large
source tree — a fresh checkout runs to a few hundred MB — and the plugin needs
three directories out of it, so both commands below ask for a sparse checkout
of exactly those.

Claude Code:

```bash
claude plugin marketplace add GeoDaCenter/geoda --sparse .claude-plugin .agents plugins
claude plugin install -y geoda@geoda
```

Codex (one `--sparse` per path):

```bash
codex plugin marketplace add GeoDaCenter/geoda --sparse .claude-plugin --sparse .agents --sparse plugins
codex plugin add geoda@geoda
```

Restart the client afterwards — a client only sees the tools of the servers it
started with. Then ask it to do something spatial, e.g. *"run a LISA analysis on
the homicide rate in the natregimes sample with GeoDa"*.

The same two clicks are in the app: **Options → MCP → Install MCP Plugin…**,
which reports what happened at each step (and passes the sparse paths itself),
falling back to registering the endpoint by URL when a plugin install is not
possible — Windows, whose clients have no `/bin/sh` for the launcher, or a
client without a plugin loader.

## Layout

```
.claude-plugin/marketplace.json                  Claude Code marketplace
.agents/plugins/marketplace.json                 Codex marketplace
plugins/geoda/.claude-plugin/plugin.json         Claude Code plugin manifest
plugins/geoda/.codex-plugin/plugin.json          Codex plugin manifest
plugins/geoda/.mcp.json                          MCP declaration (Claude: ${CLAUDE_PLUGIN_ROOT})
plugins/geoda/mcp.json                          MCP declaration (Codex: relative command + cwd)
plugins/geoda/bin/geoda-mcp                      the stdio launcher
plugins/geoda/skills/geoda/SKILL.md              the skill
```

The first two paths are fixed: a marketplace is looked for at the root of the
repository it names, so `.claude-plugin/marketplace.json` and
`.agents/plugins/marketplace.json` have to sit there even though everything else
about the plugin is under `plugins/geoda/`.

Note there is deliberately **no `plugin.json` at the plugin's root**: Codex
would read that as the manifest instead of `.codex-plugin/plugin.json`, find no
`mcpServers` in it, and install a plugin with no MCP server — reported as a
success. The two MCP declarations are also separate on purpose, because Codex
does not expand `${CLAUDE_PLUGIN_ROOT}`.

## Keeping this in sync with the app

`bin/geoda-mcp` is the same script the app writes when it installs the launcher
itself, which is committed at `MCP/bin/geoda-mcp`, and a third copy is embedded
in `MCP/McpClientSetup.cpp` as the raw string the direct route writes out.
`MCP/tests/launcher_test.py` fails when any of the three drifts from the others,
so change them together.

Bump `version` in both `plugins/geoda/.claude-plugin/plugin.json` and
`plugins/geoda/.codex-plugin/plugin.json` when the skill or the launcher
changes; then `claude plugin update geoda` picks it up.

## License

GPL-3.0, the same license as GeoDa.
