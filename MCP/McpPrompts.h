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

#ifndef __GEODA_CENTER_MCP_PROMPTS_H__
#define __GEODA_CENTER_MCP_PROMPTS_H__

#include <string>
#include <json_spirit/json_spirit.h>
#include <wx/string.h>

// MCP prompts served by the GeoDa MCP server. The main prompt,
// "spatial-analysis-workbook", is a workflow guide that follows Luc Anselin's
// "Exploring Spatial Data with GeoDa: A Workbook" (Center for Spatial Data
// Science, University of Chicago). It maps each part of the workbook onto the
// MCP tools so an LLM client can reproduce the workbook exercises against the
// open project. Stateless and read-only, so it is safe to call from any
// thread.
class McpPrompts
{
public:
    McpPrompts();
    ~McpPrompts();

    // Result for prompts/list: { "prompts": [ ... ] }
    json_spirit::Value GetPromptsList() const;

    // Result for prompts/get given the request params object. Throws McpError
    // for unknown prompt names or unknown section names.
    json_spirit::Value GetPrompt(const json_spirit::Object& params) const;

private:
    // Full workbook guide, as the body of a user message.
    static std::string GuideText();
    // One section of the guide (overview, maps, weights, global, local,
    // rates, multivariate, clustering). Returns an empty string for an
    // unknown section.
    static std::string SectionText(const wxString& section);
};

#endif
