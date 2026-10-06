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
}

// ---------------------------------------------------------------------------
// McpTools
// ---------------------------------------------------------------------------

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
    json_spirit::Array tools;
    for (size_t i = 0; i < m_tools.size(); ++i) {
        std::vector<json_spirit::Pair> t;
        t.push_back(P("name", json_spirit::Value(m_tools[i].name.ToStdString())));
        t.push_back(P("description",
                      json_spirit::Value(m_tools[i].description.ToStdString())));
        t.push_back(P("inputSchema", m_tools[i].input_schema));
        tools.push_back(Obj(t));
    }
    std::vector<json_spirit::Pair> result;
    result.push_back(P("tools", json_spirit::Value(tools)));
    return Obj(result);
}

void McpTools::RegisterTools()
{
    // All commands (menu actions and analysis tools) are registered in
    // MCP/McpCommands.cpp -- the single source of truth for the app's
    // command surface.
    RegisterCommands(*this);
}
