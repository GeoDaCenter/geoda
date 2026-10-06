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

#ifndef __GEODA_CENTER_MCP_HTTP_SERVER_H__
#define __GEODA_CENTER_MCP_HTTP_SERVER_H__

#include <atomic>
#include <map>
#include <string>
#include <wx/event.h>
#include <wx/socket.h>

class McpServer;

// Minimal HTTP/1.1 server on wxSocketServer bound to 127.0.0.1. Serves the
// MCP protocol (JSON-RPC 2.0) on POST /mcp, a no-op response on OPTIONS /mcp,
// and a health check on GET /. No CORS headers are sent: the server is meant
// for desktop MCP clients, and browsers must not be able to reach it.
//
// Threading: light tools (project/status, table/*, weights/*) and window
// tools (window/create_map, window/create_plot -- wx window creation is
// main-thread-only) are handled synchronously on the main thread. Heavy tools
// (LISA with permutations, clustering) spawn a detached wxThread that computes
// the result and writes the response to the handed-off socket; the main thread
// never touches the socket after handoff. At most kMaxWorkers heavy tools run
// concurrently; further heavy requests are rejected with 503.
class McpHttpServer : public wxEvtHandler
{
public:
    // port 0 = let the OS auto-assign a free port.
    McpHttpServer(int port = 0);
    ~McpHttpServer();

    bool Start();
    void Stop();
    bool IsRunning() const { return m_server != NULL; }
    int GetPort() const { return m_port; }
    wxString GetUrl() const;

private:
    // Maximum number of concurrent heavy-tool worker threads.
    static const int kMaxWorkers = 4;

    void OnServerEvent(wxSocketEvent& event);
    void OnClientEvent(wxSocketEvent& event);
    // Closes and destroys a socket handed back from a worker thread. wxSocket
    // on macOS must be closed on the thread that created it (the main thread).
    void OnSocketClose(wxCommandEvent& event);
    void HandleRequest(wxSocketBase* socket, const std::string& method,
                       const std::string& path, const std::string& body);
    void SendResponse(wxSocketBase* socket, const std::string& body,
                      int status);
    void SendCorsPreflight(wxSocketBase* socket);
    void SendHealth(wxSocketBase* socket);

    wxSocketServer* m_server;
    int m_port;
    McpServer* m_mcp;
    // Sockets with partially-received requests -> accumulated raw bytes.
    std::map<wxSocketBase*, std::string> m_buffers;
    // Number of heavy-tool workers currently running (bounded by kMaxWorkers).
    std::atomic<int> m_active_workers;

    wxDECLARE_NO_COPY_CLASS(McpHttpServer);
    wxDECLARE_EVENT_TABLE();
};

#endif
