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

#ifndef __GEODA_CENTER_MCP_TOOLS_H__
#define __GEODA_CENTER_MCP_TOOLS_H__

#include <exception>
#include <string>
#include <vector>
#include <json_spirit/json_spirit.h>
#include <wx/string.h>

class Project;

// Server-initiated elicitation: a tool asks the client to prompt the user and
// waits for the answer ("elicitation/create", MCP spec 2025-06-18). Claude Code
// and Codex both render it as a question card. A channel is handed only to
// tools that run on a worker thread -- asking from the main thread would block
// the GUI -- and is null when the connected client did not advertise the
// capability, or when the request was answered with plain JSON instead of a
// stream. Tools must fall back to their parameter error when it is null or
// Available() is false.
class McpElicitChannel
{
public:
    virtual ~McpElicitChannel() {}

    // True when the client can actually be asked.
    virtual bool Available() const = 0;

    // Ask a single multiple-choice question. `options` are both the accepted
    // values and the labels shown to the user. Returns true and sets `answer`
    // when the user accepted; false when the client cannot be asked, the user
    // declined or cancelled, or the wait timed out.
    virtual bool Ask(const std::string& message, const std::string& key,
                     const std::string& title,
                     const std::vector<std::string>& options,
                     const std::string& default_value,
                     std::string& answer) = 0;
};

// Context passed to every tool handler. project is null when no project is
// open; elicit is null when the client cannot be asked (see above).
struct McpToolContext
{
    Project* project;
    McpElicitChannel* elicit;
};

// Thrown by tool handlers for expected errors (no project open, unknown
// column, unknown weights id, ...). McpServer converts these to JSON-RPC
// errors.
class McpError : public std::exception
{
public:
    McpError(int code, const std::string& message)
        : m_code(code), m_message(message) {}
    virtual ~McpError() throw() {}
    virtual const char* what() const throw() { return m_message.c_str(); }
    int code() const { return m_code; }
    const std::string& message() const { return m_message; }

private:
    int m_code;
    std::string m_message;
};

// A tool handler takes the tool context and the JSON-RPC "arguments" object
// and returns the tool's result as a json_spirit::Value. Expected errors are
// signaled by throwing McpError.
typedef json_spirit::Value (*McpToolHandler)(const McpToolContext& ctx,
                                             const json_spirit::Object& params);

struct McpTool
{
    wxString name;                    // command id (MCP tool name)
    wxString label;                   // human-readable label
    wxString menu_path;               // menu grouping, e.g. "Space"
    wxString description;
    json_spirit::Value input_schema;  // JSON Schema object
    bool run_on_worker;               // heavy tools run on a worker thread
    bool may_elicit;                  // asks the user for parameters when they
                                      // are missing and the client supports it
    McpToolHandler handler;
};

// Registry of MCP tools. Populated once in the constructor; read-only
// afterwards so it is safe to query from worker threads.
class McpTools
{
public:
    McpTools();
    ~McpTools();

    const McpTool* FindTool(const wxString& name) const;
    const std::vector<McpTool>& GetTools() const { return m_tools; }
    json_spirit::Value GetToolsList() const;

    // Registration API, used by RegisterCommands (MCP/McpCommands.cpp).
    // may_elicit marks tools that ask the user for a missing parameter; it
    // defaults to false so only the tools that do need to opt in.
    void AddTool(const wxString& command_id, const wxString& label,
                 const wxString& menu_path, const wxString& description,
                 const json_spirit::Value& input_schema, bool run_on_worker,
                 McpToolHandler handler, bool may_elicit = false);

private:
    void RegisterTools();

    std::vector<McpTool> m_tools;
};

#endif
