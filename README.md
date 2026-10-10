[![Mac OSX builds](https://github.com/geodacenter/geoda/actions/workflows/osx_build.yml/badge.svg)](https://github.com/geodacenter/geoda/actions/workflows/osx_build.yml)
[![Ubuntu builds](https://github.com/geodacenter/geoda/actions/workflows/ubuntu_build.yml/badge.svg)](https://github.com/geodacenter/geoda/actions/workflows/ubuntu_build.yml)
[![Windows builds](https://github.com/geodacenter/geoda/actions/workflows/windows_build.yml/badge.svg)](https://github.com/geodacenter/geoda/actions/workflows/windows_build.yml)

# Acknowledgements #

GeoDa TM is built upon several open source libraries and source-code files.

GeoDa is the flagship program of the GeoDa Center, following a long line of software tools developed by Dr. Luc Anselin. It is designed to implement techniques for exploratory spatial data analysis (ESDA) on lattice data (points and polygons). The free program provides a user friendly and graphical interface to methods of descriptive spatial data analysis, such as spatial autocorrelation statistics, as well as basic spatial regression functionality. The latest version contains several new features such as full space-time data support in all views, a new cartogram, a refined map movie, parallel coordinate plot, 3D visualization, conditional plots (and maps) and spatial regression.

Since its initial release in February 2003, GeoDa's user numbers have increased exponentially, as the chart and map of global users above shows. This includes lab users at universities such as Harvard, MIT, and Cornell. The user community and press embraced the program enthusiastically, calling it a "hugely important analytic tool," a "very fine piece of software," an "exciting development" and more.

# Build GeoDa #

Please read the detail instructions under directory BuildTools/

[Windows](BuildTools/windows/readme.md)

[Mac OSX](BuildTools/macosx/readme.md)

[Linux/Ubuntu](BuildTools/ubuntu/readme.md)

Note:  contributions of build scripts under other platforms are welcomed, please follow the structure of building script under BuildTools/.

# Editing the MCP skill #

The built-in MCP server ships a skill, the spatial data analysis workbook, and serves it to MCP clients itself: an agent that connects to GeoDa can read it with nothing installed. It exposes **two MCP tools**: `list_command` returns the catalog (each command's id, group and parameters), and `execute_command {name: "<command id>", arguments: {...}}` runs one. The same text reaches a client four ways:

* `execute_command {name: "skill/get", arguments: {uri: "skill://spatial-analysis-workbook"}}` - the `skill/get` command (listed by `skill/list`), so a client that does not expose MCP resources can still read the skill.
* The resource `skill://spatial-analysis-workbook`, via `resources/read`.
* The prompt `spatial-analysis-workbook`, via `prompts/get`. Its `section` argument loads one part: `workflow`, `overview`, `maps`, `weights`, `global`, `local`, `rates`, `multivariate`, `clustering`.
* The `instructions` that `initialize` returns, which name the skill and how to read it.

The text lives in three files, and all three have to be edited together:

* `MCP/skills/spatial-analysis-workbook.md` - the authoring source, and the copy a human reads.
* `MCP/McpResources.cpp`, `WorkbookSkillText()` - the whole document as the C string that `skill/get` and the resource serve.
* `MCP/McpPrompts.cpp`, `GuideText()` and `SectionText()` - the same guide split into the prompt's sections.

Worth keeping in mind when editing:

* The "FIRST: Always load this skill" section is the contract that makes an agent read the workbook before it calls anything. Keep it, and keep it first.
* Name commands and parameters exactly as they are registered in `MCP/McpCommands.cpp`; this document is their documentation. Commands reach a client only through `execute_command` (`MCP/McpTools.cpp` builds the two tool schemas, and `list_command` reports the same registry), so the skill is where a command's workflow is described.
* Adding a section means a new `kSectionKeys`/`kSectionLabels` entry, a `SectionText()` branch, and a matching heading in the `.md`.
* If how the skill is delivered changes, update the `instructions` in `MCP/McpServer.cpp` too.

To see what a client sees, with GeoDa running (default port 8765):

        curl -s -X POST -H 'content-type: application/json' \
          -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"execute_command","arguments":{"name":"skill/get","arguments":{"uri":"skill://spatial-analysis-workbook"}}}}' \
          http://127.0.0.1:8765/mcp

`MCP/tests/skill_serving_test.py` checks the whole arrangement: that `tools/list` returns only `list_command` and `execute_command`, that the instructions point at the skill, that `skill/get` and `resources/read` return identical text, that the document still opens with the load-me-first section, and that an unknown URI is an error.

        GEODA_BIN=/path/to/GeoDa python3 MCP/tests/skill_serving_test.py

# The agent plugin #

GeoDa's MCP server can also reach a client as a **plugin**, which the client installs and starts itself, so the agent sees GeoDa's commands as tools rather than being handed the endpoint URL. That plugin is published from this repository: `.claude-plugin/marketplace.json` (Claude Code) and `.agents/plugins/marketplace.json` (Codex) at the root, with the plugin itself under `plugins/geoda/`. `plugins/geoda/README.md` has the layout, and why there is deliberately no `plugin.json` at the plugin's root.

A marketplace is looked for at the root of the repository it names, and this one is a large source tree, so the install asks for a sparse checkout of the three directories the plugin needs:

        claude plugin marketplace add GeoDaCenter/geoda --sparse .claude-plugin .agents plugins
        claude plugin install -y geoda@geoda

GeoDa does the same thing from its own window: **Options -> MCP -> Install MCP Plugin...** gives a button for Claude Code and one for Codex, reports each step, and falls back to registering the HTTP endpoint by URL where a plugin install is not possible.

The launcher exists in three copies - `plugins/geoda/bin/geoda-mcp`, `MCP/bin/geoda-mcp`, and the raw string `MCP/McpClientSetup.cpp` writes out for the direct route - and `MCP/tests/launcher_test.py` fails when they drift apart.

        GEODA_BIN=/path/to/GeoDa python3 MCP/tests/launcher_test.py

# Internationalization #

GeoDa Internationalization (I18n) and Localization(L10n) project aims to provide an online tool that GeoDa users could help to translate the GeoDa UI to different languages.

For crowdsourcing, we use Google Spreadsheet with the public address [here](https://docs.google.com/spreadsheets/d/1iZa4wCIyTDlIRYoW7229YoZWKZ0lmIiOFsCJG3ZVw-s/edit?usp=sharing). Anyone can access this spreadsheet, and edit each translation.

# Contributors: #

* @corochasco
* Gulrukh Rakhmatullaeva

Thanks for your contributions!

# Dependencies #

Below is a list of some of these that we'd like to acknowledge.

* GDAL Libraries, version 1.10

        License: X/MIT style Open Source license
        Authors: many
        Links: http://www.gdal.org/
    
* Boost Libraries, version 1.53

        License: Boost Software License - Version 1.0
        Authors: many
        Links: http://www.boost.org/
              http://www.boost.org/LICENSE_1_0.txt

* Boost.Polygon Voronoi Library, Boost version 1.53

        License: Boost Software License - Version 1.0
        Author: Andrii Sydorchuk
        Links: http://www.boost.org/
              http://www.boost.org/LICENSE_1_0.txt

* wxWidgets Cross-Platform GUI Library, version 2.9.4

        License: The wxWindows Library Licence
        Authors: Julian Smart, Robert Roebling, and others
        Links: http://www.wxwidgets.org/
                http://www.opensource.org/licenses/wxwindows.php

* CLAPACK Linear Algebra Libraries, version 3.2.1

        Authors: many
        License: Custom by University of Tennessee
        Links: http://www.netlib.org/clapack/
                http://www.netlib.org/lapack/lapack-3.2/LICENSE

* Approximate Nearest Neighbor Library, version 0.1

        Note: Full source of 0.1 release included in kNN directory
        Authors: Sunil Arya and David Mount
        License: See kNN/AHH.h in included source files
        Links: http://www.cs.umd.edu/~mount/ANN/

* FastArea.c++ source code

        Note: We have based the source for functions findArea and
        ComputeArea2D in our file GenGeomAlgs.h from FastArea.c++
        in Journal of Graphics Tools, 7(2):9-13, 2002
        Author: Daniel Sunday
        License: unknown
        Links: http://www.tandfonline.com/doi/abs/10.1080/10867651.2002.10487556
