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

#ifndef __GEODA_CENTER_WEB_VIEW_EXAMPLE_WIN_H__
#define __GEODA_CENTER_WEB_VIEW_EXAMPLE_WIN_H__

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/webview.h>
#include "../GdaJson.h"
#include "../GenUtils.h"
#include "../HighlightStateObserver.h"
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableStateObserver.h"
#include "../DataViewer/TimeStateObserver.h"
#include "../ShapeOperations/WeightsManStateObserver.h"

class HighlightState;
class FramesManager;
class TableState;
class TableInterface;
class TimeState;
class WeightsManState;
class Project;

class WebViewExampleDlg: public wxDialog, public HighlightStateObserver,
public FramesManagerObserver, public TableStateObserver,
public TimeStateObserver, public WeightsManStateObserver
{    
public:
	WebViewExampleDlg(Project* project,
					  const wxString& page_source,
					  wxWindow* parent, wxWindowID id = wxID_ANY,
					  const wxString& caption = "wxWebView Example",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
	WebViewExampleDlg(Project* project,
					  const wxString& page_source,
					  std::vector<GdaVarTools::VarInfo> var_info,
					  std::vector<int> col_ids,
					  wxWindow* parent, wxWindowID id = wxID_ANY,
					  const wxString& caption = "wxWebView Example",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
	virtual ~WebViewExampleDlg();
	
	// D3 embeded in wxWebView examples
	static wxString get_blank();
	static wxString get_hover_example();
	static wxString get_d3_chord_diagram();
	static wxString get_zoomable_map_tiles();
	static wxString get_json_spirit_ex1();
	static wxString get_d3_circles_ex1();
	static wxString get_hello_world();
	static wxString get_bars_example();
	
	void OnClose(wxCloseEvent& ev);
	void OnTitleChanged(wxWebViewEvent& ev);
	void OnDocumentLoaded(wxWebViewEvent& ev);
	void LoadURL(const wxString& url);
	
	/** Implementation of the HighlightStateObserver interface. */
	virtual void update(HLStateInt* o);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return 0; }
	virtual void closeObserver(boost::uuids::uuid id) {};
	
protected:
	virtual void parseActionNotify(const json_spirit::Value& v);
	virtual void parseHighlightStateNotify(const json_spirit::Value& v);
	virtual void parseTimeStateNotify(const json_spirit::Value& v);
	virtual void parseActionRequest(const json_spirit::Value& v);
	
	virtual void promptVarSettings(const json_spirit::Object&,
								   json_spirit::Value& response_val);
	
private:
	void Init(const wxString& page_source, const wxString& url=wxEmptyString);
	
	wxPanel* panel;
	wxWebView* web_view;
	
	Project* project;
	HighlightState* highlight_state;
	FramesManager* frames_manager;
	TableState* table_state;
	TableInterface* table_int;
	TimeState* time_state;
	WeightsManState* w_man_state;
	
	/** Before any JS initialization routines are called, must be sure
	 that document has finished loading.  When LoadURL is called, this
	 is set to true.  When the OnDocumentLoaded callback is called, it
	 will call gda.Init and other JS functions to initialize the window. */
	bool load_url_called;
	
	std::vector<int> col_ids;
	std::vector<GdaVarTools::VarInfo> var_info;
};




#endif
