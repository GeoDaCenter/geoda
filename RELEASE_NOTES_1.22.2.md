## GeoDa 1.22.2 — built-in MCP server (preview)

GeoDa now hosts a local MCP server, so Claude Code or Codex can drive it: load a data set, build spatial weights, run Moran's I / LISA, clustering, regression, and map the results in the GeoDa window you have open. And it is not one-way — when a tool needs something your prompt left out, GeoDa **asks you** with a question card in the client.

You can even ask your AI harness to run spatial regression with the new models in [`spreg`](https://github.com/pysal/spreg) and bring the results back into GeoDa — export the table, fit the model, then reopen the fitted values or residuals as a layer and map them.

[Watch the 18-second trailer](https://github.com/GeoDaCenter/geoda/releases/download/v1.22.2/geoda-1222-trailer.mp4)

![Options → MCP → Start MCP Server…](https://github.com/GeoDaCenter/geoda/releases/download/v1.22.2/geoda-mcp-start-server.jpg)

![The agent asking which variable to use](https://github.com/GeoDaCenter/geoda/releases/download/v1.22.2/geoda-mcp-question-card.jpg)

**Try it**

Start GeoDa (this release), then **Options → MCP → Start MCP Server…** and click **Copy**. The dialog shows the port it bound — `http://127.0.0.1:8765` by default, loopback-only, the next free port if 8765 is taken.

Connect your client once (Optional):

    claude mcp add --transport http geoda http://127.0.0.1:8765/mcp

    codex mcp add geoda --url http://127.0.0.1:8765/mcp

Restart the client (Codex needs a full restart; in Claude Code `/mcp` reconnects), then paste and prompt it:

    Connect GeoDa MCP at http://127.0.0.1:8765, load ~/Downloads/natregimes/natregimes.shp, run LISA on HR60 with queen weights.

Preview: the tool set may still change, and the listener has no token — anything on the machine can drive GeoDa while it runs (`--no-mcp` turns it off).
