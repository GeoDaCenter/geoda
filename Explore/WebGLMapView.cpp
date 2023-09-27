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
#include <string>
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
#include <wx/event.h>
#include <wx/ffile.h>
#include <wx/frame.h>
#include <wx/fs_mem.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>

#include "../GenUtils.h"
#include "../GeomUtils.h"
#include "WebGLMapView.h"

IMPLEMENT_CLASS(WebGLMapFrame, TemplateFrame)
BEGIN_EVENT_TABLE(WebGLMapFrame, TemplateFrame)
END_EVENT_TABLE()

WebGLMapFrame::WebGLMapFrame(wxFrame* parent, Project* project, OGRLayer* layer, const wxString& title,
                             const wxPoint& pos, const wxSize& size, const int style)
    : TemplateFrame(parent, project, title, pos, size, style) {
  wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

  // Create a log window
  new wxLogWindow(NULL, _("Logging"), true, false);

  custom_scheme = "memory:";

#if wxUSE_WEBVIEW_EDGE
  custom_scheme = "http://memory.wxsite/";
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

  // Required for virtual file system archive and memory support
  wxFileSystem::AddHandler(new wxMemoryFSHandler);

  // Create the memory files
  CreateMemoryFiles(layer);

  // Create the webview
  m_browser = wxWebView::New();

  // With WKWebView handlers need to be registered before creation
  m_browser->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));

  m_browser->Create(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
  topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

  // Log backend information
  wxLogMessage("Backend: %s Version: %s", m_browser->GetClassInfo()->GetClassName(),
               wxWebView::GetBackendVersionInfo().ToString());
  wxLogMessage("User Agent: %s", m_browser->GetUserAgent());

  if (!m_browser->AddScriptMessageHandler("wx")) wxLogError("Could not add script message handler");

  SetSizer(topsizer);

  // Set a more sensible size for web browsing
  SetSize(FromDIP(wxSize(800, 600)));

  m_browser->LoadURL(custom_scheme + "index.html");
  m_browser->SetFocus();
  m_browser->EnableAccessToDevTools(true);
  // Connect the idle events
  Bind(wxEVT_IDLE, &WebGLMapFrame::OnIdle, this);
}

WebGLMapFrame::~WebGLMapFrame() {
  wxMemoryFSHandler::RemoveFile("data.arrow");
  wxMemoryFSHandler::RemoveFile("bundle.js");
  wxMemoryFSHandler::RemoveFile("index.html");
  DeregisterAsActive();
}

void WebGLMapFrame::CreateMemoryFiles(OGRLayer* layer) {
  wxStopWatch sw;
  // load web pages under web_plugins/
  wxString web_file_path = GenUtils::GetSamplesDir();

  wxPathList pathlist;
  pathlist.Add(web_file_path);

  // Create data.csv by passing in OGRLayer with only geometries and selected variables
  const std::string ogr_filename = "/vsimem/data.arrow";

  const std::string driver_name = "Arrow";
  char** options = nullptr;
  options = CSLSetNameValue(options, "COMPRESSION", "NONE");
  save_ogrlayer(layer, ogr_filename, driver_name, options);
  CSLDestroy(options);
  // save_ogrlayer(layer, ogr_filename, "GeoJSON");
  // read vsimem file in char* buffer
  VSILFILE* fp = VSIFOpenL(ogr_filename.c_str(), "rb");
  VSIFSeekL(fp, 0, SEEK_END);
  const size_t size = VSIFTellL(fp);
  VSIFSeekL(fp, 0, SEEK_SET);
  char* buffer = new char[size];
  VSIFReadL(buffer, 1, size, fp);
  VSIFCloseL(fp);
  wxMemoryFSHandler::AddFileWithMimeType("data.arrow", buffer, size, "");

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
  index_content.Replace("bundle.js", custom_scheme + "bundle.js");
  index_content.Replace("data.arrow", custom_scheme + "data.arrow");

  wxMemoryFSHandler::AddFile("index.html", index_content);

  std::cout << "Create Memory File:" << sw.Time() << std::endl;
}

void WebGLMapFrame::OnIdle(wxIdleEvent& WXUNUSED(evt)) {
  if (m_browser->IsBusy()) {
    wxSetCursor(wxCURSOR_ARROWWAIT);
  } else {
    wxSetCursor(wxNullCursor);
  }
}
