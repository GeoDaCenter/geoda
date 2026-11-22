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

#include "MCP/McpServer.h"
#include "GdaJson.h"
#include "GeoDa.h"

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

    // Extract the "id" member of a JSON-RPC request (number, string, or null).
    json_spirit::Value GetRequestId(const json_spirit::Object& obj)
    {
        json_spirit::Value id;
        if (GdaJson::findValue(obj, id, "id")) return id;
        return json_spirit::Value();
    }
}

McpServer::McpServer() : m_client_elicits(false), m_next_ask_id(1)
{
}

McpServer::~McpServer()
{
    // Wake any worker still waiting on an answer so it does not sit until the
    // timeout. The wait objects are deliberately not freed: a detached worker
    // may still be inside Ask() holding a pointer to one of them, and this only
    // runs as the server is being torn down.
    wxMutexLocker lock(m_asks_mutex);
    for (std::map<std::string, McpAskWait*>::iterator it = m_asks.begin();
         it != m_asks.end(); ++it) {
        wxMutexLocker entry_lock(it->second->mutex);
        it->second->done = true;
        it->second->accepted = false;
        it->second->cond.Signal();
    }
}

McpAskWait* McpServer::RegisterAsk(std::string& id)
{
    wxMutexLocker lock(m_asks_mutex);
    McpAskWait* wait = new McpAskWait();
    id = wxString::Format("geoda-ask-%lld", (long long) m_next_ask_id++).ToStdString();
    m_asks[id] = wait;
    return wait;
}

void McpServer::ResolveAsk(const std::string& id, bool accepted,
                           const std::string& value)
{
    McpAskWait* wait = NULL;
    {
        wxMutexLocker lock(m_asks_mutex);
        std::map<std::string, McpAskWait*>::iterator it = m_asks.find(id);
        if (it == m_asks.end()) return;
        wait = it->second;
    }
    wxMutexLocker entry_lock(wait->mutex);
    wait->accepted = accepted;
    wait->value = value;
    wait->done = true;
    wait->cond.Signal();
}

void McpServer::UnregisterAsk(const std::string& id)
{
    wxMutexLocker lock(m_asks_mutex);
    std::map<std::string, McpAskWait*>::iterator it = m_asks.find(id);
    if (it == m_asks.end()) return;
    delete it->second;
    m_asks.erase(it);
}

json_spirit::Value McpServer::MakeParseError() const
{
    std::vector<json_spirit::Pair> err;
    err.push_back(P("code", json_spirit::Value(-32700)));
    err.push_back(P("message", json_spirit::Value("Parse error")));
    std::vector<json_spirit::Pair> resp;
    resp.push_back(P("jsonrpc", json_spirit::Value("2.0")));
    resp.push_back(P("id", json_spirit::Value()));
    resp.push_back(P("error", Obj(err)));
    return Obj(resp);
}

// The client sees two tools and names the real command inside
// execute_command's "arguments", so both the thread choice and the
// elicitation opt-in have to resolve the inner id rather than the request's
// own tool name.
const McpTool* McpServer::ResolveRequestTool(const json_spirit::Value& request) const
{
    if (request.type() != json_spirit::obj_type) return NULL;
    const json_spirit::Object& obj = request.get_obj();
    wxString method = GdaJson::getStrValFromObj(obj, "method");
    if (method != "tools/call") return NULL;
    json_spirit::Value params;
    if (!GdaJson::findValue(request, params, "params")) return NULL;
    if (params.type() != json_spirit::obj_type) return NULL;
    const json_spirit::Object& p = params.get_obj();
    wxString name = GdaJson::getStrValFromObj(p, "name");
    if (name == kMcpExecuteToolName) {
        json_spirit::Value tool_args;
        if (!GdaJson::findValue(p, tool_args, "arguments")) return NULL;
        if (tool_args.type() != json_spirit::obj_type) return NULL;
        name = GdaJson::getStrValFromObj(tool_args.get_obj(), "name");
    }
    return m_tools.FindTool(name);
}

bool McpServer::IsHeavyTool(const json_spirit::Value& request) const
{
    const McpTool* tool = ResolveRequestTool(request);
    return tool && tool->run_on_worker;
}

json_spirit::Value McpServer::HandleRequest(const std::string& body,
                                            McpElicitChannel* elicit)
{
    json_spirit::Value request;
    if (!json_spirit::read(body, request)) {
        return MakeParseError();
    }
    return Dispatch(request, elicit);
}

bool McpServer::IsResponseMessage(const std::string& body) const
{
    json_spirit::Value request;
    if (!json_spirit::read(body, request)) return false;
    if (request.type() != json_spirit::obj_type) return false;
    const json_spirit::Object& obj = request.get_obj();
    // An "id" and no "method" is how the client delivers an elicitation answer.
    return GdaJson::hasName(obj, "id") && !GdaJson::hasName(obj, "method");
}

bool McpServer::IsElicitingTool(const json_spirit::Value& request) const
{
    const McpTool* tool = ResolveRequestTool(request);
    return tool && tool->may_elicit;
}

json_spirit::Value McpServer::Dispatch(const json_spirit::Value& request,
                                       McpElicitChannel* elicit)
{
    if (request.type() != json_spirit::obj_type) {
        std::vector<json_spirit::Pair> err;
        err.push_back(P("code", json_spirit::Value(-32600)));
        err.push_back(P("message",
                        json_spirit::Value("Invalid Request: expected an object")));
        std::vector<json_spirit::Pair> resp;
        resp.push_back(P("jsonrpc", json_spirit::Value("2.0")));
        resp.push_back(P("id", json_spirit::Value()));
        resp.push_back(P("error", Obj(err)));
        return Obj(resp);
    }

    const json_spirit::Object& obj = request.get_obj();
    bool is_notification = !GdaJson::hasName(obj, "id");
    json_spirit::Value id = GetRequestId(obj);
    wxString method = GdaJson::getStrValFromObj(obj, "method");

    // An "id" with no "method" is a response: the client answering an
    // elicitation we sent. Wake the tool that asked and send no reply of our
    // own (the HTTP layer answers 202).
    if (!is_notification && method.IsEmpty()) {
        ApplyAskResponse(obj);
        return json_spirit::Value();
    }

    json_spirit::Value params;
    GdaJson::findValue(request, params, "params");
    json_spirit::Object params_obj;
    if (params.type() == json_spirit::obj_type) params_obj = params.get_obj();

    json_spirit::Value inner;
    bool is_error = false;
    int error_code = 0;
    wxString error_message;

    if (method == "initialize") {
        inner = HandleInitialize(params_obj);
    } else if (method == "notifications/initialized") {
        inner = Obj(std::vector<json_spirit::Pair>());
    } else if (method == "ping") {
        inner = Obj(std::vector<json_spirit::Pair>());
    } else if (method == "tools/list") {
        inner = HandleToolsList();
    } else if (method == "tools/call" || method == "prompts/get" ||
               method == "resources/read") {
        try {
            if (method == "tools/call") {
                inner = HandleToolsCall(params_obj, elicit);
            } else if (method == "prompts/get") {
                inner = HandlePromptsGet(params_obj);
            } else {
                inner = HandleResourcesRead(params_obj);
            }
        } catch (const McpError& e) {
            is_error = true;
            error_code = e.code();
            error_message = e.message();
        } catch (const std::exception& e) {
            is_error = true;
            error_code = -32603;
            error_message = e.what();
        }
    } else if (method == "prompts/list") {
        inner = HandlePromptsList();
    } else if (method == "resources/list") {
        inner = HandleResourcesList();
    } else {
        is_error = true;
        error_code = -32601;
        error_message = "Method not found: " + method;
    }

    // Notifications get no response.
    if (is_notification) return json_spirit::Value();

    std::vector<json_spirit::Pair> resp;
    resp.push_back(P("jsonrpc", json_spirit::Value("2.0")));
    resp.push_back(P("id", id));
    if (is_error) {
        std::vector<json_spirit::Pair> err;
        err.push_back(P("code", json_spirit::Value(error_code)));
        err.push_back(P("message", json_spirit::Value(error_message.ToStdString())));
        resp.push_back(P("error", Obj(err)));
    } else {
        resp.push_back(P("result", inner));
    }
    return Obj(resp);
}

json_spirit::Value McpServer::HandleInitialize(const json_spirit::Object& params)
{
    // Negotiate the protocol revision and remember whether the client can be
    // asked questions. The revision that introduced elicitation is only used
    // when the client asked for it or newer -- answering with a revision the
    // client predates makes it disconnect -- and elicitation is enabled only
    // when the client also advertised the capability.
    const wxString kElicitRevision("2025-06-18");
    wxString requested = GdaJson::getStrValFromObj(params, "protocolVersion");
    m_client_elicits = false;
    wxString negotiated("2025-03-26");
    if (!requested.IsEmpty() && requested >= kElicitRevision) {
        negotiated = kElicitRevision;
        json_spirit::Value caps;
        if (GdaJson::findValue(params, caps, "capabilities") &&
            caps.type() == json_spirit::obj_type) {
            json_spirit::Value elicit_cap;
            if (GdaJson::findValue(caps, elicit_cap, "elicitation")) {
                m_client_elicits = true;
            }
        }
    }

    std::vector<json_spirit::Pair> tools_cap;
    tools_cap.push_back(P("listChanged", json_spirit::Value(false)));
    std::vector<json_spirit::Pair> prompts_cap;
    std::vector<json_spirit::Pair> resources_cap;
    resources_cap.push_back(P("listChanged", json_spirit::Value(false)));
    std::vector<json_spirit::Pair> capabilities;
    capabilities.push_back(P("tools", Obj(tools_cap)));
    capabilities.push_back(P("prompts", Obj(prompts_cap)));
    capabilities.push_back(P("resources", Obj(resources_cap)));

    std::vector<json_spirit::Pair> server_info;
    server_info.push_back(P("name", json_spirit::Value("geoda-mcp")));
    server_info.push_back(P("version", json_spirit::Value("1.0.0")));

    std::vector<json_spirit::Pair> result;
    result.push_back(P("protocolVersion",
                       json_spirit::Value(negotiated.ToStdString())));
    result.push_back(P("capabilities", Obj(capabilities)));
    result.push_back(P("serverInfo", Obj(server_info)));
    // The initialize `instructions` are the one thing a client is handed
    // before it calls anything, so the workbook skill is announced here: an
    // agent that connects to GeoDa learns to load the skill from the server
    // itself, with no external skill repo and nothing to install. skill/get
    // rather than the resource URI alone, because Claude Code exposes MCP
    // resources only as user @-mentions.
    result.push_back(P("instructions", json_spirit::Value(
        "GeoDa: desktop spatial data analysis -- spatial weights, global and "
        "local autocorrelation (LISA), clustering, regression, maps. Two "
        "tools: list_command lists every command (id, group, parameters), and "
        "execute_command {name: \"<command id>\", arguments: {...}} runs one. "
        "Read the workbook skill first: execute_command {name: \"skill/get\", "
        "arguments: {uri: \"skill://spatial-analysis-workbook\"}} (or "
        "resources/read {uri: skill://spatial-analysis-workbook}). It maps "
        "each task onto the commands -- do not run another command before you "
        "have read it. If project/status reports no data set open, open one "
        "with execute_command {name: \"file/open\", arguments: {path: ...}}; "
        "execute_command {name: \"file/close\"} closes it.")));
    return Obj(result);
}

json_spirit::Value McpServer::HandleToolsList()
{
    return m_tools.GetToolsList();
}

json_spirit::Value McpServer::HandlePromptsList()
{
    return m_prompts.GetPromptsList();
}

json_spirit::Value McpServer::HandlePromptsGet(const json_spirit::Object& params)
{
    return m_prompts.GetPrompt(params);
}

json_spirit::Value McpServer::HandleResourcesList()
{
    return m_resources.GetResourcesList();
}

json_spirit::Value McpServer::HandleResourcesRead(const json_spirit::Object& params)
{
    return m_resources.GetResource(params);
}

json_spirit::Value McpServer::HandleToolsCall(const json_spirit::Object& params,
                                              McpElicitChannel* elicit)
{
    // Two exposed tools: list_command returns the catalog, and
    // execute_command {name, arguments} runs one command. Any other tool name
    // is an error -- there is no per-command tool surface.
    wxString tool_name = GdaJson::getStrValFromObj(params, "name");

    json_spirit::Value result;
    if (tool_name == kMcpListToolName) {
        json_spirit::Object list_args;
        json_spirit::Value list_val;
        if (GdaJson::findValue(params, list_val, "arguments") &&
            list_val.type() == json_spirit::obj_type) {
            list_args = list_val.get_obj();
        }
        result = m_tools.GetCommandsList(list_args);
    } else if (tool_name == kMcpExecuteToolName) {
        json_spirit::Value tool_args_val;
        if (!GdaJson::findValue(params, tool_args_val, "arguments") ||
            tool_args_val.type() != json_spirit::obj_type) {
            throw McpError(-32602,
                "Missing arguments: call execute_command {name: "
                "\"<command id>\", arguments: {...}}.");
        }
        const json_spirit::Object& tool_args = tool_args_val.get_obj();

        wxString name = GdaJson::getStrValFromObj(tool_args, "name");
        if (name.IsEmpty()) {
            throw McpError(-32602,
                "Missing command name: call execute_command {name: "
                "\"<command id>\", arguments: {...}}.");
        }
        const McpTool* tool = m_tools.FindTool(name);
        if (!tool) {
            throw McpError(-32602, "Unknown command: " + name.ToStdString());
        }

        McpToolContext ctx;
        ctx.project = GdaFrame::GetProject();
        // Only commands that opted in may ask the user, and only when the
        // client advertised the capability; every other command keeps the
        // parameter error it raised before.
        ctx.elicit = (tool->may_elicit && m_client_elicits) ? elicit : NULL;

        json_spirit::Object args;
        json_spirit::Value args_val;
        if (GdaJson::findValue(tool_args, args_val, "arguments") &&
            args_val.type() == json_spirit::obj_type) {
            args = args_val.get_obj();
        }
        result = tool->handler(ctx, args);
    } else {
        throw McpError(-32602,
            "Unknown tool: " + tool_name.ToStdString() +
            " (this server exposes list_command and execute_command)");
    }

    // If the handler returned a MCP "content" array directly (e.g. an image
    // snapshot from return_image), pass it through verbatim. Otherwise wrap
    // the JSON result in a single text block (default behavior).
    json_spirit::Array content;
    bool has_content = false;
    if (result.type() == json_spirit::obj_type) {
        const json_spirit::Object& o = result.get_obj();
        for (json_spirit::Object::const_iterator it = o.begin();
             it != o.end(); ++it) {
            if (it->name_ == "content" &&
                it->value_.type() == json_spirit::array_type) {
                content = it->value_.get_array();
                has_content = true;
                break;
            }
        }
    }
    if (!has_content) {
        std::vector<json_spirit::Pair> content_item;
        content_item.push_back(P("type", json_spirit::Value("text")));
        content_item.push_back(P("text",
            json_spirit::Value(wxString::FromUTF8(json_spirit::write(result).c_str())
                               .ToStdString())));
        content.push_back(Obj(content_item));
    }
    std::vector<json_spirit::Pair> call_result;
    call_result.push_back(P("content", json_spirit::Value(content)));
    return Obj(call_result);
}

// The client answered one of our elicitations: wake the tool that asked.
void McpServer::ApplyAskResponse(const json_spirit::Object& response)
{
    // Ids are strings so the client echoes back exactly what we sent.
    wxString id = GdaJson::getStrValFromObj(response, "id");
    if (id.IsEmpty()) return;

    json_spirit::Value result;
    if (!GdaJson::findValue(response, result, "result")) return;
    if (result.type() != json_spirit::obj_type) return;
    const json_spirit::Object& r = result.get_obj();

    wxString action = GdaJson::getStrValFromObj(r, "action");
    bool accepted = (action == "accept");

    // A form answers one value per requested property; we ask a single
    // question, so take the only value there is.
    std::string value;
    json_spirit::Value content;
    if (GdaJson::findValue(r, content, "content") &&
        content.type() == json_spirit::obj_type) {
        const json_spirit::Object& c = content.get_obj();
        for (json_spirit::Object::const_iterator it = c.begin();
             it != c.end(); ++it) {
            if (it->value_.type() == json_spirit::str_type) {
                value = it->value_.get_str();
                break;
            }
        }
    }

    ResolveAsk(id.ToStdString(), accepted, value);
}

namespace
{
    // How long a tool waits for the user to answer before giving up. The card
    // stays up for this long, so it has to be time enough to read and decide.
    const unsigned long kAskTimeoutMs = 180000;
}

McpServerElicitChannel::McpServerElicitChannel(McpServer& server, EmitFn emit,
                                               void* emit_ctx)
    : m_server(server), m_emit(emit), m_emit_ctx(emit_ctx)
{
}

bool McpServerElicitChannel::Available() const
{
    return m_server.ClientSupportsElicitation() && m_emit != NULL;
}

bool McpServerElicitChannel::Ask(const std::string& message,
                                 const std::string& key,
                                 const std::string& title,
                                 const std::vector<std::string>& options,
                                 const std::string& default_value,
                                 std::string& answer)
{
    if (!Available() || options.empty()) return false;

    std::string id;
    McpAskWait* wait = m_server.RegisterAsk(id);

    // Form mode, one property with an enum of the values we accept:
    //   {"jsonrpc":"2.0","id":"geoda-ask-1","method":"elicitation/create",
    //    "params":{"message":"...","requestedSchema":{"type":"object", ...}}}
    std::vector<json_spirit::Value> enum_vals;
    for (size_t i = 0; i < options.size(); ++i) {
        enum_vals.push_back(json_spirit::Value(options[i]));
    }
    std::vector<json_spirit::Pair> prop;
    prop.push_back(P("type", json_spirit::Value("string")));
    prop.push_back(P("title", json_spirit::Value(title)));
    prop.push_back(P("enum", json_spirit::Value(json_spirit::Array(enum_vals))));
    if (!default_value.empty()) {
        prop.push_back(P("default", json_spirit::Value(default_value)));
    }
    std::vector<json_spirit::Pair> properties;
    properties.push_back(json_spirit::Pair(key, Obj(prop)));
    std::vector<json_spirit::Value> required;
    required.push_back(json_spirit::Value(key));

    std::vector<json_spirit::Pair> schema;
    schema.push_back(P("type", json_spirit::Value("object")));
    schema.push_back(P("properties", Obj(properties)));
    schema.push_back(P("required",
                       json_spirit::Value(json_spirit::Array(required))));

    std::vector<json_spirit::Pair> ask_params;
    ask_params.push_back(P("message", json_spirit::Value(message)));
    ask_params.push_back(P("requestedSchema", Obj(schema)));

    std::vector<json_spirit::Pair> req;
    req.push_back(P("jsonrpc", json_spirit::Value("2.0")));
    req.push_back(P("id", json_spirit::Value(id)));
    req.push_back(P("method", json_spirit::Value("elicitation/create")));
    req.push_back(P("params", Obj(ask_params)));

    m_emit(m_emit_ctx, json_spirit::write(Obj(req)));

    // Wait for the client to deliver the answer (McpServer::ResolveAsk) or for
    // the timeout to fire. The tool thread is parked here, which is why only
    // worker threads are given a channel.
    bool ok = false;
    {
        wxMutexLocker lock(wait->mutex);
        while (!wait->done) {
            if (wait->cond.WaitTimeout(kAskTimeoutMs) == wxCOND_TIMEOUT) break;
        }
        ok = wait->done && wait->accepted;
        if (ok) answer = wait->value;
    }
    m_server.UnregisterAsk(id);
    return ok;
}
