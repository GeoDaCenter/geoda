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

#ifndef __GEODA_CENTER_MCP_RESOURCES_H__
#define __GEODA_CENTER_MCP_RESOURCES_H__

#include <string>
#include <json_spirit/json_spirit.h>
#include <wx/string.h>

// MCP resources served by the GeoDa MCP server. Skills are served as
// resources with skill:// URIs and text/markdown mime type, following the
// fsq-spatial-desktop pattern of exposing SKILL.md files to MCP clients.
// Content is embedded as C++ strings (same approach as McpPrompts.cpp) so it
// works from any install location; the files in MCP/skills/ are the
// authoring source. Stateless and read-only, so it is safe to call from any
// thread.
class McpResources
{
public:
    McpResources();
    ~McpResources();

    // Result for resources/list: { "resources": [ ... ] }
    json_spirit::Value GetResourcesList() const;

    // Result for resources/read given the request params object. Throws
    // McpError for unknown resource URIs.
    json_spirit::Value GetResource(const json_spirit::Object& params) const;

private:
    // The workbook skill, as markdown text.
    static std::string WorkbookSkillText();
};

#endif
