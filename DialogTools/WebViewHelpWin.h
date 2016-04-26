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

#ifndef __GEODA_CENTER_WEB_VIEW_HELP_WIN_H__
#define __GEODA_CENTER_WEB_VIEW_HELP_WIN_H__

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/webview.h>
#include "../FramesManagerObserver.h"

class FramesManager;
class Project;

class WebViewHelpWin: public wxFrame, public FramesManagerObserver
{
public:
	WebViewHelpWin(Project* project,
					  const wxString& page_source,
					  wxWindow* parent, wxWindowID id = wxID_ANY,
					  const wxString& title = "GeoDa Help",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize);
	virtual ~WebViewHelpWin();
	
	// example HTML source pages
	static wxString get_blank();
	static wxString get_hello_world();
	
	void OnClose(wxCloseEvent& ev);
	void OnTitleChanged(wxWebViewEvent& ev);
	void OnDocumentLoaded(wxWebViewEvent& ev);
	void LoadURL(const wxString& url);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);

private:
	void Init(const wxString& page_source);
	
	wxPanel* panel;
	wxWebView* web_view;
	
	Project* project;
	FramesManager* frames_manager;
};

#endif
