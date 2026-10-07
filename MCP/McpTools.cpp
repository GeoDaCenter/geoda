/**
 * GeoDa TM, Copyright (C) 2011-2025 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MCP/McpTools.h"
#include "MCP/McpCommands.h"
#include "GdaJson.h"

#include <set>

namespace
{
    json_spirit::Pair P(const wxString& name, const json_spirit::Value& v)
    {
        return json_spirit::Pair(name.ToStdString(), v);
    }

    json_spirit::Value Obj(const std::vector<json_spirit::Pair>& pairs)
    {
        return json_spirit::Value(json_spirit::Object(pairs));
    }

    // A command's input_schema is a JSON Schema object; flatten it to the
    // [{name, type, required, description}] that list_command returns.
    json_spirit::Array ParametersOf(const json_spirit::Value& schema)
    {
        json_spirit::Array out;
        json_spirit::Value props;
        if (schema.type() != json_spirit::obj_type ||
            !GdaJson::findValue(schema, props, "properties") ||
            props.type() != json_spirit::obj_type) {
            return out;
        }

        std::set<std::string> required;
        json_spirit::Value req;
        if (GdaJson::findValue(schema, req, "required") &&
            req.type() == json_spirit::array_type) {
            const json_spirit::Array& ra = req.get_array();
            for (size_t i = 0; i < ra.size(); ++i) {
                if (ra[i].type() == json_spirit::str_type) {
                    required.insert(ra[i].get_str());
                }
            }
        }

        const json_spirit::Object& p = props.get_obj();
        for (json_spirit::Object::const_iterator it = p.begin();
             it != p.end(); ++it) {
            std::vector<json_spirit::Pair> item;
            item.push_back(P("name", json_spirit::Value(it->name_)));
            if (it->value_.type() == json_spirit::obj_type) {
                const json_spirit::Object& spec = it->value_.get_obj();
                wxString type = GdaJson::getStrValFromObj(spec, "type");
                wxString desc = GdaJson::getStrValFromObj(spec, "description");
                if (!type.IsEmpty()) {
                    item.push_back(P("type",
                                     json_spirit::Value(type.ToStdString())));
                }
                if (!desc.IsEmpty()) {
                    item.push_back(P("description",
                                     json_spirit::Value(desc.ToStdString())));
                }
            }
            item.push_back(P("required", json_spirit::Value(
                required.count(it->name_) > 0)));
            out.push_back(Obj(item));
        }
        return out;
    }
}

// ---------------------------------------------------------------------------
// McpTools
// ---------------------------------------------------------------------------

const char* const kMcpListToolName = "list_command";
const char* const kMcpExecuteToolName = "execute_command";

McpTools::McpTools()
{
    RegisterTools();
}

McpTools::~McpTools()
{
}

void McpTools::AddTool(const wxString& command_id, const wxString& label,
                       const wxString& menu_path, const wxString& description,
                       const json_spirit::Value& input_schema,
                       bool run_on_worker, McpToolHandler handler,
                       bool may_elicit)
{
    McpTool t;
    t.name = command_id;
    t.label = label;
    t.menu_path = menu_path;
    t.description = description;
    t.input_schema = input_schema;
    t.run_on_worker = run_on_worker;
    t.may_elicit = may_elicit;
    t.handler = handler;
    m_tools.push_back(t);
}

const McpTool* McpTools::FindTool(const wxString& name) const
{
    for (size_t i = 0; i < m_tools.size(); ++i) {
        if (m_tools[i].name == name) return &m_tools[i];
    }
    return NULL;
}

json_spirit::Value McpTools::GetToolsList() const
{
    // Two tools instead of one per command. The ~90 commands stay in the
    // registry (McpCommands.cpp) as the executor; a client discovers them
    // through list_command and runs them through execute_command, so it loads
    // two schemas rather than ninety.
    std::vector<json_spirit::Pair> list_props;
    {
        std::vector<json_spirit::Pair> g;
        g.push_back(P("type", json_spirit::Value("string")));
        g.push_back(P("description", json_spirit::Value(
            "Optional menu group to keep, case-insensitive -- e.g. \"File\", "
            "\"Table\", \"Map\", \"Space\", \"Cluster\".")));
        std::vector<json_spirit::Pair> n;
        n.push_back(P("type", json_spirit::Value("string")));
        n.push_back(P("description", json_spirit::Value(
            "Optional command id to return on its own, e.g. "
            "\"space/lisa_univariate\".")));
        list_props.push_back(P("group", Obj(g)));
        list_props.push_back(P("name", Obj(n)));
    }
    std::vector<json_spirit::Pair> list_schema;
    list_schema.push_back(P("type", json_spirit::Value("object")));
    list_schema.push_back(P("properties", Obj(list_props)));

    std::vector<json_spirit::Pair> list_tool;
    list_tool.push_back(P("name", json_spirit::Value(kMcpListToolName)));
    list_tool.push_back(P("description", json_spirit::Value(
        "List the GeoDa commands: for each one its id, menu group, "
        "description and parameters. Pass \"group\" to keep one menu group, "
        "or \"name\" for a single command. Run a command with "
        "execute_command.")));
    list_tool.push_back(P("inputSchema", Obj(list_schema)));

    std::vector<json_spirit::Pair> exec_props;
    {
        std::vector<json_spirit::Pair> n;
        n.push_back(P("type", json_spirit::Value("string")));
        n.push_back(P("description", json_spirit::Value(
            "Command id, one of the ids list_command returns -- e.g. "
            "\"file/open\", \"weights/create\", \"space/lisa_univariate\".")));
        std::vector<json_spirit::Pair> a;
        a.push_back(P("type", json_spirit::Value("object")));
        a.push_back(P("description", json_spirit::Value(
            "Parameters for that command, as list_command documents them. "
            "Omit or pass {} for commands that take none.")));
        exec_props.push_back(P("name", Obj(n)));
        exec_props.push_back(P("arguments", Obj(a)));
    }
    json_spirit::Array exec_required;
    exec_required.push_back(json_spirit::Value("name"));
    std::vector<json_spirit::Pair> exec_schema;
    exec_schema.push_back(P("type", json_spirit::Value("object")));
    exec_schema.push_back(P("properties", Obj(exec_props)));
    exec_schema.push_back(P("required", json_spirit::Value(exec_required)));

    std::vector<json_spirit::Pair> exec_tool;
    exec_tool.push_back(P("name", json_spirit::Value(kMcpExecuteToolName)));
    exec_tool.push_back(P("description", json_spirit::Value(
        "Run a GeoDa command by id: pass the command id as \"name\" and its "
        "parameters as \"arguments\". Every GeoDa action -- opening a data "
        "set, maps, spatial weights, global and local autocorrelation, rates, "
        "clustering -- is one of these commands; list_command lists them. The "
        "workbook skill "
        "(execute_command {name: \"skill/get\", arguments: {uri: "
        "\"skill://spatial-analysis-workbook\"}}) describes the analysis "
        "workflows that combine them.")));
    exec_tool.push_back(P("inputSchema", Obj(exec_schema)));

    json_spirit::Array tools;
    tools.push_back(Obj(list_tool));
    tools.push_back(Obj(exec_tool));

    std::vector<json_spirit::Pair> result;
    result.push_back(P("tools", json_spirit::Value(tools)));
    return Obj(result);
}

json_spirit::Value McpTools::GetCommandsList(const json_spirit::Object& params) const
{
    wxString group = GdaJson::getStrValFromObj(params, "group");
    wxString name = GdaJson::getStrValFromObj(params, "name");

    json_spirit::Array commands;
    for (size_t i = 0; i < m_tools.size(); ++i) {
        const McpTool& c = m_tools[i];
        if (!name.IsEmpty() && c.name != name) continue;
        if (!group.IsEmpty() && !c.menu_path.IsSameAs(group, false)) continue;
        std::vector<json_spirit::Pair> item;
        item.push_back(P("name", json_spirit::Value(c.name.ToStdString())));
        item.push_back(P("group", json_spirit::Value(c.menu_path.ToStdString())));
        item.push_back(P("label", json_spirit::Value(c.label.ToStdString())));
        item.push_back(P("description",
                         json_spirit::Value(c.description.ToStdString())));
        item.push_back(P("parameters",
                         json_spirit::Value(ParametersOf(c.input_schema))));
        commands.push_back(Obj(item));
    }

    if (!name.IsEmpty() && commands.empty()) {
        throw McpError(-32602, "Unknown command: " + name.ToStdString());
    }

    std::vector<json_spirit::Pair> result;
    result.push_back(P("commands", json_spirit::Value(commands)));
    return Obj(result);
}

void McpTools::RegisterTools()
{
    // All commands (menu actions and analysis tools) are registered in
    // MCP/McpCommands.cpp -- the single source of truth for the app's
    // command surface.
    RegisterCommands(*this);
}
