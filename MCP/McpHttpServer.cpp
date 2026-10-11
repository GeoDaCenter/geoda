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

#include "MCP/McpHttpServer.h"
#include "MCP/McpServer.h"
#include <json_spirit/json_spirit.h>

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/utils.h>

// Posted by a worker thread to hand a finished socket back to the main thread
// for closing. wxSocket on macOS must be closed on the thread that created it.
wxDEFINE_EVENT(wxEVT_MCP_SOCKET_CLOSE, wxCommandEvent);

namespace
{
    enum {
        MCP_SERVER_SOCKET_ID = wxID_HIGHEST + 1,
        MCP_CLIENT_SOCKET_ID = wxID_HIGHEST + 2
    };

    // Build a complete HTTP/1.1 response string.
    std::string BuildHttpResponse(const std::string& body, int status)
    {
        std::string status_text;
        switch (status) {
            case 200: status_text = "OK"; break;
            case 204: status_text = "No Content"; break;
            case 400: status_text = "Bad Request"; break;
            case 404: status_text = "Not Found"; break;
            case 500: status_text = "Internal Server Error"; break;
            case 503: status_text = "Service Unavailable"; break;
            default:  status_text = "OK"; break;
        }
        std::string resp;
        resp += "HTTP/1.1 " + std::to_string(status) + " " + status_text + "\r\n";
        resp += "Content-Type: application/json\r\n";
        resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        resp += "Connection: close\r\n";
        resp += "\r\n";
        resp += body;
        return resp;
    }

    // Begin an event stream. The response carries no Content-Length, so the
    // socket stays open and further messages -- a server-initiated request such
    // as elicitation/create, then the tool's result -- follow as SSE frames.
    void SendStreamHeaders(wxSocketBase* sock)
    {
        std::string resp;
        resp += "HTTP/1.1 200 OK\r\n";
        resp += "Content-Type: text/event-stream\r\n";
        resp += "Cache-Control: no-cache\r\n";
        resp += "Connection: close\r\n";
        resp += "\r\n";
        sock->Write(resp.c_str(), (wxUint32)resp.size());
    }

    // One SSE "message" event carrying a JSON-RPC message.
    void WriteSseFrame(wxSocketBase* sock, const std::string& json)
    {
        std::string frame = "event: message\r\ndata: " + json + "\r\n\r\n";
        sock->Write(frame.c_str(), (wxUint32)frame.size());
    }

    // 202 for a message that is not a request -- the client answering an
    // elicitation -- accepted with no body, as the transport requires.
    void SendAccepted(wxSocketBase* sock)
    {
        std::string resp;
        resp += "HTTP/1.1 202 Accepted\r\n";
        resp += "Content-Length: 0\r\n";
        resp += "Connection: close\r\n";
        resp += "\r\n";
        sock->Write(resp.c_str(), (wxUint32)resp.size());
    }

    // EmitFn for McpServerElicitChannel: writes one server->client message on
    // the stream of the request being handled. Runs on the worker thread that
    // owns the socket.
    void EmitToSocket(void* ctx, const std::string& json)
    {
        WriteSseFrame((wxSocketBase*) ctx, json);
    }

    // Parse the Content-Length header from a raw header block (before the
    // blank line). Returns -1 if absent or malformed.
    int ParseContentLength(const std::string& headers)
    {
        size_t pos = 0;
        while (pos < headers.size()) {
            size_t line_end = headers.find("\r\n", pos);
            if (line_end == std::string::npos) line_end = headers.size();
            std::string line = headers.substr(pos, line_end - pos);
            std::string lline;
            lline.reserve(line.size());
            for (size_t i = 0; i < line.size(); ++i)
                lline.push_back((char)tolower((unsigned char)line[i]));
            if (lline.compare(0, 15, "content-length:") == 0) {
                std::string val = lline.substr(15);
                size_t s = val.find_first_not_of(" \t");
                if (s != std::string::npos) val = val.substr(s);
                return atoi(val.c_str());
            }
            pos = line_end + 2;
        }
        return -1;
    }

    // Parse the request line ("POST /mcp HTTP/1.1") into method and path.
    void ParseRequestLine(const std::string& headers, std::string& method,
                          std::string& path)
    {
        size_t line_end = headers.find("\r\n");
        if (line_end == std::string::npos) line_end = headers.size();
        std::string line = headers.substr(0, line_end);
        size_t sp1 = line.find(' ');
        size_t sp2 = (sp1 == std::string::npos) ? std::string::npos
                                                : line.find(' ', sp1 + 1);
        if (sp1 != std::string::npos) {
            method = line.substr(0, sp1);
            if (sp2 != std::string::npos)
                path = line.substr(sp1 + 1, sp2 - sp1 - 1);
            else
                path = line.substr(sp1 + 1);
        }
    }

    // Discovery file: ~/.geoda/mcp.json = {server, url, port, pid, startedAt}.
    // The port is only known after binding (a taken port falls back to a
    // nearby one, then to an OS-assigned one), so external MCP clients read
    // this file instead of guessing. Written on start, removed on stop.
    wxString GetDiscoveryFilePath()
    {
        wxString dir = wxGetHomeDir() + wxFileName::GetPathSeparator() + ".geoda";
        if (!wxDirExists(dir)) wxFileName::Mkdir(dir);
        return dir + wxFileName::GetPathSeparator() + "mcp.json";
    }

    // Read back the port recorded in the discovery file, or 0 if unreadable.
    int ReadDiscoveryPort()
    {
        wxString path = GetDiscoveryFilePath();
        if (!wxFileExists(path)) return 0;
        wxFile f(path, wxFile::read);
        if (!f.IsOpened()) return 0;
        wxString content;
        f.ReadAll(&content);
        f.Close();
        int pos = content.Find("\"port\":");
        if (pos == wxNOT_FOUND) return 0;
        long port = 0;
        content.Mid(pos + 7).BeforeFirst(',').BeforeFirst('}').Trim(true).Trim(false)
            .ToLong(&port);
        return (int)port;
    }

    void WriteDiscoveryFile(int port)
    {
        wxString path = GetDiscoveryFilePath();
        wxString body = wxString::Format(
            "{\"server\":\"geoda\",\"url\":\"http://127.0.0.1:%d/mcp\","
            "\"port\":%d,\"pid\":%ld,\"startedAt\":\"%s\"}",
            port, port, (long)wxGetProcessId(),
            wxDateTime::UNow().FormatISOCombined());
        // Write to a temp file and rename, so a reader never sees a partial
        // file.
        wxString tmp = path + ".tmp";
        wxRemoveFile(tmp);
        wxFile f;
        if (f.Create(tmp, true) || f.Open(tmp, wxFile::write)) {
            f.Write(body);
            f.Close();
            wxRenameFile(tmp, path, true);
            // Owner-only: the file names a local endpoint.
            wxFileName(path).SetPermissions(wxS_IRUSR | wxS_IWUSR);
        }
    }

    // Only remove the file if it describes the port this instance bound --
    // several instances share the one path, and a stopping instance must not
    // erase a running one's entry.
    void DeleteDiscoveryFile(int port)
    {
        if (ReadDiscoveryPort() != port) return;
        wxRemoveFile(GetDiscoveryFilePath());
    }

    // True if something is already accepting connections on the loopback port,
    // i.e. the port cannot be used. Start() cannot rely on the bind failing:
    // wxSOCKET_REUSEADDR maps to SO_REUSEADDR, which on Windows lets a second
    // process bind a port another process is already listening on, so both
    // would think they own 8765. Connecting to the port answers the question on
    // every platform.
    bool IsPortInUse(int port)
    {
        if (port <= 0 || port > 65535) return false;
        wxIPV4address addr;
        addr.Hostname("127.0.0.1");
        addr.Service((unsigned short)port);
        wxSocketClient probe;
        // A free loopback port refuses the connection immediately; the timeout
        // only bounds the wait if the loopback itself stops answering.
        probe.SetTimeout(1);
        bool in_use = probe.Connect(addr);
        probe.Close();
        return in_use;
    }
}

// Worker thread for heavy tools (LISA with permutations, clustering). Owns the
// socket exclusively: computes the result and writes the response. The socket
// is then handed back to the main thread (via a queued event) for closing --
// wxSocket on macOS must be closed on the thread that created it.
class McpWorkerThread : public wxThread
{
public:
    McpWorkerThread(wxSocketBase* socket, const std::string& body,
                    McpServer* mcp, wxEvtHandler* handler,
                    std::atomic<int>* active_workers)
        : wxThread(wxTHREAD_DETACHED), m_socket(socket), m_body(body),
          m_mcp(mcp), m_handler(handler), m_active_workers(active_workers) {}

    virtual void* Entry()
    {
        try {
            // A tool that may ask the user a question needs an event stream
            // instead of a single JSON response: the question has to reach the
            // client while the tool is still running. Everything else keeps the
            // plain response it had before.
            json_spirit::Value request;
            bool streaming = json_spirit::read(m_body, request) &&
                             m_mcp->ClientSupportsElicitation() &&
                             m_mcp->IsElicitingTool(request);
            McpServerElicitChannel* channel = NULL;
            if (streaming) {
                // The frames must reach the client before the tool parks
                // waiting for the answer, so write synchronously from this
                // thread rather than letting the GUI event loop flush them.
                m_socket->SetFlags(wxSOCKET_BLOCK);
                m_socket->SetTimeout(30);
                SendStreamHeaders(m_socket);
                channel = new McpServerElicitChannel(*m_mcp, &EmitToSocket,
                                                     m_socket);
            }
            json_spirit::Value response = m_mcp->HandleRequest(m_body, channel);
            if (streaming) {
                WriteSseFrame(m_socket, json_spirit::write(response));
                delete channel;
            } else if (response.type() == json_spirit::null_type) {
                // A notification, which the server answers with a null value:
                // accepted with no body, as the transport requires. Sending the
                // null would put a frame with no id on the stream the client
                // reads replies from.
                SendAccepted(m_socket);
            } else {
                std::string http =
                    BuildHttpResponse(json_spirit::write(response), 200);
                m_socket->Write(http.c_str(), (wxUint32)http.size());
            }
        } catch (...) {
            // Never leak the worker slot: fall through and release it below.
        }
        wxCommandEvent evt(wxEVT_MCP_SOCKET_CLOSE);
        evt.SetClientData(m_socket);
        wxQueueEvent(m_handler, evt.Clone());
        if (m_active_workers) --(*m_active_workers);
        return NULL;
    }

private:
    wxSocketBase* m_socket;
    std::string m_body;
    McpServer* m_mcp;
    wxEvtHandler* m_handler;
    std::atomic<int>* m_active_workers;
};

BEGIN_EVENT_TABLE(McpHttpServer, wxEvtHandler)
    EVT_SOCKET(MCP_SERVER_SOCKET_ID, McpHttpServer::OnServerEvent)
    EVT_SOCKET(MCP_CLIENT_SOCKET_ID, McpHttpServer::OnClientEvent)
    EVT_COMMAND(wxID_ANY, wxEVT_MCP_SOCKET_CLOSE, McpHttpServer::OnSocketClose)
END_EVENT_TABLE()

McpHttpServer::McpHttpServer(int port)
    : m_server(NULL), m_port(port), m_mcp(new McpServer()),
      m_active_workers(0)
{
}

McpHttpServer::~McpHttpServer()
{
    Stop();
    delete m_mcp;
}

bool McpHttpServer::Start()
{
    if (m_server) return true;

    // Preferred port first, then the next few (so a second GeoDa instance
    // lands on a nearby port rather than a random one), then let the OS pick.
    int requested_port = m_port;
    int try_ports[12];
    int n = 0;
    if (m_port != 0) {
        for (int i = 0; i < 10 && m_port + i <= 65535; ++i) {
            try_ports[n++] = m_port + i;
        }
    }
    try_ports[n++] = 0;

    for (int i = 0; i < n; ++i) {
        // 0 asks the OS for any free port, so only a named port can be taken
        // and needs the check.
        if (try_ports[i] != 0 && IsPortInUse(try_ports[i])) continue;
        wxIPV4address addr;
        addr.Hostname("127.0.0.1");
        addr.Service((unsigned short)try_ports[i]);
        wxSocketServer* server = new wxSocketServer(addr, wxSOCKET_REUSEADDR);
        if (server->IsOk()) {
            m_server = server;
            wxIPV4address local;
            m_server->GetLocal(local);
            m_port = local.Service();
            m_server->SetEventHandler(*this, MCP_SERVER_SOCKET_ID);
            m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
            m_server->Notify(true);
            WriteDiscoveryFile(m_port);
            wxLogMessage("MCP server listening on %s", GetUrl());
            if (requested_port != 0 && m_port != requested_port) {
                wxLogMessage("MCP server: port %d is in use, using %d instead",
                             requested_port, m_port);
            }
            return true;
        }
        delete server;
    }
    return false;
}

void McpHttpServer::Stop()
{
    if (m_server) {
        m_server->Notify(false);
        m_server->Close();
        m_server->Destroy();
        m_server = NULL;
        DeleteDiscoveryFile(m_port);
    }
    for (std::map<wxSocketBase*, std::string>::iterator it = m_buffers.begin();
         it != m_buffers.end(); ++it) {
        it->first->Notify(false);
        it->first->Destroy();
    }
    m_buffers.clear();
}

wxString McpHttpServer::GetBaseUrl() const
{
    return wxString::Format("http://127.0.0.1:%d", m_port);
}

wxString McpHttpServer::GetUrl() const
{
    return GetBaseUrl() + "/mcp";
}

void McpHttpServer::OnServerEvent(wxSocketEvent& event)
{
    if (event.GetSocketEvent() != wxSOCKET_CONNECTION) return;
    wxSocketBase* client = m_server->Accept(false);
    if (!client) return;
    client->SetFlags(wxSOCKET_NOWAIT_READ);
    client->SetEventHandler(*this, MCP_CLIENT_SOCKET_ID);
    client->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    client->Notify(true);
}

void McpHttpServer::OnClientEvent(wxSocketEvent& event)
{
    wxSocketBase* sock = event.GetSocket();
    wxSocketNotify notify = event.GetSocketEvent();

    if (notify == wxSOCKET_LOST) {
        // Only touch sockets we are tracking. Sockets handed off to a worker
        // thread (or already destroyed) are ignored -- we only compare the
        // pointer value, never dereference it.
        std::map<wxSocketBase*, std::string>::iterator it = m_buffers.find(sock);
        if (it != m_buffers.end()) {
            m_buffers.erase(it);
            sock->Destroy();
        }
        return;
    }

    if (notify != wxSOCKET_INPUT) return;

    char buf[4096];
    wxUint32 n = sock->Read(buf, sizeof(buf)).LastReadCount();
    if (n == 0) return;

    std::string& buffer = m_buffers[sock];
    buffer.append(buf, n);

    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) return;

    std::string headers = buffer.substr(0, header_end);
    std::string method, path;
    ParseRequestLine(headers, method, path);

    // GET (health check) and OPTIONS (preflight) carry no body, so they can
    // be handled as soon as the header block has arrived.
    if (method == "GET" || method == "OPTIONS") {
        m_buffers.erase(sock);
        sock->Notify(false);
        HandleRequest(sock, method, path, "");
        return;
    }

    int content_length = ParseContentLength(headers);
    if (content_length < 0) {
        m_buffers.erase(sock);
        sock->Notify(false);
        sock->Close();
        sock->Destroy();
        return;
    }

    size_t body_start = header_end + 4;
    if (buffer.size() < body_start + (size_t)content_length) return;

    std::string body = buffer.substr(body_start, (size_t)content_length);
    m_buffers.erase(sock);
    sock->Notify(false);
    HandleRequest(sock, method, path, body);
}

// Close and destroy a socket handed back from a worker thread. Runs on the
// main thread, which is the thread that created the socket.
void McpHttpServer::OnSocketClose(wxCommandEvent& event)
{
    wxSocketBase* sock = (wxSocketBase*)event.GetClientData();
    if (sock) {
        sock->Close();
        sock->Destroy();
    }
}

void McpHttpServer::HandleRequest(wxSocketBase* socket,
                                  const std::string& method,
                                  const std::string& path,
                                  const std::string& body)
{
    if (method == "OPTIONS") {
        SendCorsPreflight(socket);
        socket->Close();
        socket->Destroy();
        return;
    }
    if (method == "GET" && path == "/") {
        SendHealth(socket);
        socket->Close();
        socket->Destroy();
        return;
    }
    if (method != "POST" || path != "/mcp") {
        SendResponse(socket, "{\"error\":\"not found\"}", 404);
        socket->Close();
        socket->Destroy();
        return;
    }

    json_spirit::Value request;
    if (!json_spirit::read(body, request)) {
        SendResponse(socket, json_spirit::write(m_mcp->MakeParseError()), 200);
        socket->Close();
        socket->Destroy();
        return;
    }

    // A response -- the client answering an elicitation -- is not a request.
    // Hand it to the server so the tool waiting on it wakes up, and
    // acknowledge with 202 and no body.
    if (m_mcp->IsResponseMessage(body)) {
        m_mcp->HandleRequest(body, NULL);
        SendAccepted(socket);
        socket->Close();
        socket->Destroy();
        return;
    }

    if (m_mcp->IsHeavyTool(request)) {
        // Bound the number of concurrent heavy-tool workers so a client
        // cannot exhaust threads/CPU. When at capacity, reject the request
        // with 503 instead of queueing unbounded work.
        if (m_active_workers >= kMaxWorkers) {
            SendResponse(socket, "{\"error\":\"server busy\"}", 503);
            socket->Close();
            socket->Destroy();
            return;
        }
        ++m_active_workers;
        // Hand off to a worker thread. Detach the socket from the main
        // thread's event loop first so the worker owns it exclusively; the
        // worker posts it back for closing when done.
        socket->SetNotify(0);
        socket->Notify(false);
        McpWorkerThread* thread =
            new McpWorkerThread(socket, body, m_mcp, this, &m_active_workers);
        if (thread->Create() == wxTHREAD_NO_ERROR) {
            thread->Run();
        } else {
            --m_active_workers;
            delete thread;
            SendResponse(socket, "{\"error\":\"internal\"}", 500);
            socket->Close();
            socket->Destroy();
        }
    } else {
        json_spirit::Value response = m_mcp->HandleRequest(body);
        if (response.type() == json_spirit::null_type) {
            // A notification: accepted with no body (see the worker thread
            // above). A client reading replies as frames must not be handed a
            // null one.
            SendAccepted(socket);
        } else {
            SendResponse(socket, json_spirit::write(response), 200);
        }
        socket->Close();
        socket->Destroy();
    }
}

void McpHttpServer::SendResponse(wxSocketBase* socket, const std::string& body,
                                 int status)
{
    std::string resp = BuildHttpResponse(body, status);
    socket->Write(resp.c_str(), (wxUint32)resp.size());
}

void McpHttpServer::SendCorsPreflight(wxSocketBase* socket)
{
    // No CORS headers: the server binds to 127.0.0.1 and is meant for
    // desktop MCP clients, not browsers. Omitting Access-Control-Allow-*
    // keeps any web page from issuing cross-origin requests to it.
    std::string resp;
    resp += "HTTP/1.1 204 No Content\r\n";
    resp += "Content-Length: 0\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    socket->Write(resp.c_str(), (wxUint32)resp.size());
}

void McpHttpServer::SendHealth(wxSocketBase* socket)
{
    std::string body = "GeoDa MCP server running";
    std::string resp;
    resp += "HTTP/1.1 200 OK\r\n";
    resp += "Content-Type: text/plain\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    socket->Write(resp.c_str(), (wxUint32)resp.size());
}
