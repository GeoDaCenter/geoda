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

#include <wx/utils.h> 
#include <wx/xrc/xmlres.h>
#include "../logger.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../FramesManager.h"
#include "WebViewHelpWin.h"

WebViewHelpWin::WebViewHelpWin(Project* project_,
															 const wxString& page_source,
															 wxWindow* parent, wxWindowID id,
															 const wxString& title,
															 const wxPoint& pos, const wxSize& size)
: wxFrame(parent, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE),
project(project_), frames_manager(project->GetFramesManager()),
panel(0), web_view(0)
{
	LOG_MSG("Entering WebViewHelpWin::WebViewHelpWin");
	Init(page_source);
	frames_manager->registerObserver(this);
	Show(true);
	LOG_MSG("Exiting WebViewHelpWin::WebViewHelpWin");
}

WebViewHelpWin::~WebViewHelpWin()
{
	LOG_MSG("In ~WebViewHelpWin::WebViewHelpWin");
	frames_manager->removeObserver(this);
}

void WebViewHelpWin::Init(const wxString& page_source)
{
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	
	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler(WebViewHelpWin::OnClose) );
	
	web_view = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr,
														wxDefaultPosition,
														wxSize(600, 500));
	
	Connect(web_view->GetId(), wxEVT_WEBVIEW_LOADED,
					wxWebViewEventHandler(WebViewHelpWin::OnDocumentLoaded),
					NULL, this);
	Connect(web_view->GetId(), wxEVT_WEBVIEW_TITLE_CHANGED,
					wxWebViewEventHandler(WebViewHelpWin::OnTitleChanged),
					NULL, this);
	
	// Arrange above widgets in panel using sizers.
	// Top level panel sizer will be panel_h_szr
	// Below that will be panel_v_szr
	// panel_v_szr will directly receive widgets
		
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
	
	panel_v_szr->Add(web_view, 1, wxEXPAND);
		
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_h_szr);
	
	// Top Sizer
	wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 5);
	
	SetSizerAndFit(top_h_sizer);
	web_view->SetPage(page_source, "");
	web_view->EnableContextMenu(false);
	web_view->SetFocus();
}

void WebViewHelpWin::OnClose(wxCloseEvent& ev)
{
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

// { "action": "close" } // immediately closes the web view

void WebViewHelpWin::OnTitleChanged(wxWebViewEvent& ev)
{
	LOG_MSG("Entering WebViewHelpWin::OnTitleChanged");
	wxString s;
	s << "Title changed: " << ev.GetString();
	LOG_MSG(s);
	LOG_MSG("Exiting WebViewHelpWin::OnTitleChanged");
}

void WebViewHelpWin::OnDocumentLoaded(wxWebViewEvent& ev)
{
	LOG_MSG("Entering WebViewHelpWin::OnDocumentLoaded");
	LOG_MSG("Exiting WebViewHelpWin::OnDocumentLoaded");
}

void WebViewHelpWin::update(FramesManager* o)
{	
}

wxString WebViewHelpWin::get_blank()
{
	wxString s =
	"<!DOCTYPE html>\n"
	"<html lang=\"en\">\n"
	"  <head>\n"
	"    <meta charset=\"utf-8\"/>\n"
	"  </head>\n"
	"  <body>\n"
	"  </body>\n"
	"</html>\n";
	return s;
}

wxString WebViewHelpWin::get_hello_world()
{
	wxString s;
	s << "<!DOCTYPE html>\n";
	s << "<html lang=\"en\">\n";
	s << "<head>\n";
	s <<   "<meta charset=\"utf-8\">\n";
	s <<   "<style>\n";
	s <<     "body {\n";
	s <<       "font: 12px sans-serif;\n";
	s <<     "}\n";
	s <<   "</style>\n";
	s << "</head>\n";
	s << "<body>\n";
	s <<   "<p>Hello World.</p>\n";
	s << "</body>\n";
	s << "</html>\n";
	
	return s;
};

