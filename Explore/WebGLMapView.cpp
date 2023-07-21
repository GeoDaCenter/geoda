/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#if !wxUSE_WEBVIEW_WEBKIT && !wxUSE_WEBVIEW_WEBKIT2 && !wxUSE_WEBVIEW_IE && !wxUSE_WEBVIEW_EDGE
#error "A wxWebView backend is required by this sample"
#endif

#include <wx/webview.h>
#if wxUSE_WEBVIEW_IE
#include <wx/msw/webview_ie.h>
#endif
#if wxUSE_WEBVIEW_EDGE
#include <wx/msw/webview_edge.h>
#endif
#include <wx/ffile.h>
#include <wx/fs_mem.h>
#include <wx/stdpaths.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>

#include "./WebGLMapView.h"

IMPLEMENT_CLASS(WebGLMapFrame, wxFrame)
BEGIN_EVENT_TABLE(WebGLMapFrame, wxFrame)
END_EVENT_TABLE()

WebGLMapFrame::WebGLMapFrame(const wxString& title) : wxFrame(NULL, wxID_ANY, title) {
  wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

  // Create a log window
  new wxLogWindow(this, _("Logging"), true, false);

#if wxUSE_WEBVIEW_EDGE
  // Check if a fixed version of edge is present in
  // $executable_path/edge_fixed and use it
  wxFileName edgeFixedDir(wxStandardPaths::Get().GetExecutablePath());
  edgeFixedDir.SetFullName("");
  edgeFixedDir.AppendDir("edge_fixed");
  if (edgeFixedDir.DirExists()) {
    wxWebViewEdge::MSWSetBrowserExecutableDir(edgeFixedDir.GetFullPath());
    wxLogMessage("Using fixed edge version");
  }
#endif

  // Create the webview
  m_browser = wxWebView::New();

#ifdef __WXMAC__
  // With WKWebView handlers need to be registered before creation
  m_browser->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
#endif

  m_browser->Create(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
  topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

  // Log backend information
  wxLogMessage("Backend: %s Version: %s", m_browser->GetClassInfo()->GetClassName(),
               wxWebView::GetBackendVersionInfo().ToString());
  wxLogMessage("User Agent: %s", m_browser->GetUserAgent());

#ifndef __WXMAC__
  // register the memory: file system
  m_browser->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
#endif

  if (!m_browser->AddScriptMessageHandler("wx")) wxLogError("Could not add script message handler");

  SetSizer(topsizer);

  // Set a more sensible size for web browsing
  SetSize(FromDIP(wxSize(800, 600)));

  // Required for virtual file system archive and memory support
  wxFileSystem::AddHandler(new wxMemoryFSHandler);

  // Create the memory files
  CreateMemoryFiles();

  m_browser->LoadURL("memory:index.html");
  m_browser->SetFocus();

  // Connect the idle events
  Bind(wxEVT_IDLE, &WebGLMapFrame::OnIdle, this);
}

WebGLMapFrame::~WebGLMapFrame() {}

void WebGLMapFrame::CreateMemoryFiles() {
  wxString exe_path = wxStandardPaths::Get().GetExecutablePath();
  wxFileName exe_file(exe_path);
  wxString exe_dir = exe_file.GetPathWithSep();

  wxPathList pathlist;
  pathlist.Add(exe_dir + "../Resources");

  // Create data.json TODO: can be replaced by passing in OGRLayer with only geometries
  // and selected variables
  wxString data_path = wxFileName(pathlist.FindValidPath("data.json")).GetAbsolutePath();
  wxFFile data_file(data_path);
  wxString data_content;
  data_file.ReadAll(&data_content);

  wxMemoryFSHandler::AddFile("data.json", data_content);

  // Create bundle.js
  wxString bundle_path = wxFileName(pathlist.FindValidPath("bundle.js")).GetAbsolutePath();
  wxFFile bundle_file(bundle_path);
  wxString bundle_content;
  bundle_file.ReadAll(&bundle_content);

  wxMemoryFSHandler::AddFile("bundle.js", bundle_content);

  // Create index.html
  wxString index_path = wxFileName(pathlist.FindValidPath("index.html")).GetAbsolutePath();
  wxFFile index_file(index_path);
  wxString index_content;
  index_file.ReadAll(&index_content);

  // replace relative urls in index.html with "memory:bundle.js"
  index_content.Replace("bundle.js", "memory:bundle.js");
  index_content.Replace("data.json", "memory:data.json");

  wxMemoryFSHandler::AddFile("index.html", index_content);
}

void WebGLMapFrame::OnIdle(wxIdleEvent& WXUNUSED(evt)) {
  if (m_browser->IsBusy()) {
    wxSetCursor(wxCURSOR_ARROWWAIT);
  } else {
    wxSetCursor(wxNullCursor);
  }
}
