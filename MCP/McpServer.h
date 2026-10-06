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

#ifndef __GEODA_CENTER_MCP_SERVER_H__
#define __GEODA_CENTER_MCP_SERVER_H__

#include <map>
#include <string>
#include <json_spirit/json_spirit.h>
#include <wx/string.h>
#include <wx/thread.h>
#include "MCP/McpTools.h"
#include "MCP/McpPrompts.h"
#include "MCP/McpResources.h"

// One outstanding elicitation. The worker thread running the tool waits on
// `cond` until the client's answer arrives (McpServer::ResolveAsk) or the wait
// times out; `value` holds the chosen option.
struct McpAskWait
{
    McpAskWait() : cond(mutex), done(false), accepted(false) {}
    wxMutex mutex;
    wxCondition cond;
    bool done;
    bool accepted;
    std::string value;
};

// JSON-RPC 2.0 dispatch for the MCP protocol. Stateless apart from the
// elicitation plumbing: each request is handled independently. HandleRequest
// may be called from the main thread (light and window tools) or from a worker
// thread (heavy tools); the tool registry is read-only after construction.
class McpServer
{
public:
    McpServer();
    ~McpServer();

    // Parse and handle a JSON-RPC request body. Returns the JSON-RPC response
    // as a Value. For notifications (no "id") and for responses to requests we
    // sent (the client's answer to an elicitation), returns an empty Value and
    // no JSON-RPC reply should be sent.
    //
    // `elicit` is the channel tools may use to ask the user; pass null when the
    // caller cannot wait for an answer (the main thread, which must not block).
    json_spirit::Value HandleRequest(const std::string& body,
                                     McpElicitChannel* elicit = NULL);

    // True if the request is a tools/call for a heavy tool that should run on
    // a worker thread.
    bool IsHeavyTool(const json_spirit::Value& request) const;

    // True if the request is a tools/call for a tool that asks the user for a
    // missing parameter (see McpTool::may_elicit).
    bool IsElicitingTool(const json_spirit::Value& request) const;

    // True if the connected client advertised the elicitation capability in
    // initialize. Only then may a tool ask the user anything.
    bool ClientSupportsElicitation() const { return m_client_elicits; }

    // True if the body is a JSON-RPC response: it carries an "id" but no
    // "method", which is how a client delivers an elicitation answer. Such a
    // message is not a request and gets no JSON-RPC reply (HTTP 202).
    bool IsResponseMessage(const std::string& body) const;

    // JSON-RPC parse error response (used by the HTTP layer when the body is
    // not valid JSON).
    json_spirit::Value MakeParseError() const;

    // Elicitation plumbing. The asking worker thread registers, waits on the
    // returned wait object, then unregisters; the thread that receives the
    // client's answer resolves it. Ids are strings ("geoda-ask-1") so the
    // client echoes back exactly what we sent, whatever id type it uses
    // internally. Exposed for the transport and McpServerElicitChannel below.
    McpAskWait* RegisterAsk(std::string& id);
    void ResolveAsk(const std::string& id, bool accepted,
                    const std::string& value);
    void UnregisterAsk(const std::string& id);

private:
    json_spirit::Value Dispatch(const json_spirit::Value& request,
                                McpElicitChannel* elicit);
    // The client answered one of our elicitation requests: wake the tool that
    // asked.
    void ApplyAskResponse(const json_spirit::Object& response);
    json_spirit::Value HandleInitialize(const json_spirit::Object& params);
    json_spirit::Value HandleToolsList();
    json_spirit::Value HandleToolsCall(const json_spirit::Object& params,
                                       McpElicitChannel* elicit);
    json_spirit::Value HandlePromptsList();
    json_spirit::Value HandlePromptsGet(const json_spirit::Object& params);
    json_spirit::Value HandleResourcesList();
    json_spirit::Value HandleResourcesRead(const json_spirit::Object& params);

    // Set from the client's initialize request.
    bool m_client_elicits;

    // Elicitations we sent and are waiting on, keyed by the request id we
    // chose. m_asks_mutex guards the map itself; each entry has its own mutex.
    std::map<std::string, McpAskWait*> m_asks;
    wxMutex m_asks_mutex;
    long long m_next_ask_id;

    McpTools m_tools;
    McpPrompts m_prompts;
    McpResources m_resources;
};

// The channel a worker thread is given for one tools/call. `emit` writes a
// single server->client message on that call's stream; the transport supplies
// it because it owns the socket. Ask() sends the elicitation request and blocks
// until the client answers, the user cancels, or the wait times out.
class McpServerElicitChannel : public McpElicitChannel
{
public:
    typedef void (*EmitFn)(void* ctx, const std::string& json);

    McpServerElicitChannel(McpServer& server, EmitFn emit, void* emit_ctx);

    virtual bool Available() const;
    virtual bool Ask(const std::string& message, const std::string& key,
                     const std::string& title,
                     const std::vector<std::string>& options,
                     const std::string& default_value,
                     std::string& answer);

private:
    McpServer& m_server;
    EmitFn m_emit;
    void* m_emit_ctx;
};

#endif
