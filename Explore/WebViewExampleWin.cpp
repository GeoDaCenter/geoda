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

#include <boost/uuid/uuid_io.hpp>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include <wx/utils.h> 
#include <wx/xrc/xmlres.h>
#include "../logger.h"
#include "../VarTools.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../FramesManager.h"
#include "../HighlightState.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../DataViewer/TimeState.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "WebViewExampleWin.h"

WebViewExampleDlg::WebViewExampleDlg(Project* project_,
									 const wxString& url,
									 wxWindow* parent,
									 wxWindowID id, const wxString& caption,
									 const wxPoint& pos, const wxSize& size,
									 long style)
: wxDialog(parent, id, caption, pos, size,
		   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
project(project_), highlight_state(project->GetHighlightState()),
frames_manager(project->GetFramesManager()),
table_state(project->GetTableState()), table_int(project->GetTableInt()),
time_state(project->GetTimeState()), w_man_state(project->GetWManState()),
web_view(0), load_url_called(false)
{
	LOG_MSG("Entering WebViewExampleDlg::WebViewExampleDlg");
	
	Init(get_blank(), url);
	
	frames_manager->registerObserver(this);
	highlight_state->registerObserver(this);
	table_state->registerObserver(this);
	time_state->registerObserver(this);
	w_man_state->registerObserver(this);
	
	LOG_MSG("Exiting WebViewExampleDlg::WebViewExampleDlg");
}

WebViewExampleDlg::WebViewExampleDlg(Project* project_,
									 const wxString& page_source,
									 std::vector<GdaVarTools::VarInfo> var_info_,
									 std::vector<int> col_ids_,
									 wxWindow* parent,
									 wxWindowID id, const wxString& caption,
									 const wxPoint& pos, const wxSize& size,
									 long style)
: wxDialog(parent, id, caption, pos, size,
		   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
project(project_), highlight_state(project->GetHighlightState()),
frames_manager(project->GetFramesManager()),
table_state(project->GetTableState()), table_int(project->GetTableInt()),
time_state(project->GetTimeState()), w_man_state(project->GetWManState()),
var_info(var_info_), col_ids(col_ids_),
web_view(0), load_url_called(false)
{
	LOG_MSG("Entering WebViewExampleDlg::WebViewExampleDlg");
	
	Init(page_source);
	//web_view->RunScript("AppendP(\"It Works!\");");
	//web_view->RunScript("document.body.style.background = 'orange';");
	
	frames_manager->registerObserver(this);
	highlight_state->registerObserver(this);
	table_state->registerObserver(this);
	time_state->registerObserver(this);
	w_man_state->registerObserver(this);
	
	LOG_MSG("Exiting WebViewExampleDlg::WebViewExampleDlg");
}

WebViewExampleDlg::~WebViewExampleDlg()
{
	LOG_MSG("In ~WebViewExampleDlg::WebViewExampleDlg");
	w_man_state->removeObserver(this);
	time_state->removeObserver(this);
	table_state->removeObserver(this);
	highlight_state->removeObserver(this);
	frames_manager->removeObserver(this);
}

void WebViewExampleDlg::Init(const wxString& page_source, const wxString& url)
{
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	
	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler(WebViewExampleDlg::OnClose) );
	
	web_view = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr,
							  wxDefaultPosition,
							  wxSize(600, 400));
	
	Connect(web_view->GetId(), wxEVT_WEBVIEW_LOADED,
            wxWebViewEventHandler(WebViewExampleDlg::OnDocumentLoaded),
			NULL, this);
	Connect(web_view->GetId(), wxEVT_WEBVIEW_TITLE_CHANGED,
            wxWebViewEventHandler(WebViewExampleDlg::OnTitleChanged),
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
	
	if (!url.IsEmpty()) {
		LOG_MSG("Loading URL: " + url);
		LoadURL(url);
	}
}

void WebViewExampleDlg::OnClose(wxCloseEvent& ev)
{
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

// { "action": "close" } // immediately closes the web view

void WebViewExampleDlg::OnTitleChanged(wxWebViewEvent& ev)
{
	LOG_MSG("Entering WebViewExampleDlg::OnTitleChanged");
	//wxString s;
	//s << "Title changed: " << ev.GetString();
	//LOG_MSG(s);
	//SetTitle(s);
	
	json_spirit::Value v;
	// try to parse as JSON
	try {
		if (!json_spirit::read( ev.GetString().ToStdString(), v)) {
			throw std::runtime_error("Could not parse title as JSON");
		}
		json_spirit::Value action_val;
		if (!GdaJson::findValue(v, action_val, "action")) {
			throw std::runtime_error("could not find action");
		}
		if (action_val.get_str() == "notify") {
			parseActionNotify(v);
		} else if (action_val.get_str() == "request") {
			parseActionRequest(v);
		} else if (action_val.get_str() == "close") {
			LOG_MSG("closing WebView");
			wxCloseEvent close_event;
			OnClose(close_event);
		}
	} catch (std::runtime_error e) {
		wxString msg;
		msg << "JSON parsing failed: ";
		msg << e.what();
		LOG_MSG(msg);
	}
	
	LOG_MSG("Exiting WebViewExampleDlg::OnTitleChanged");
}

void WebViewExampleDlg::parseActionNotify(const json_spirit::Value& v)
{
	LOG_MSG("In WebViewExampleDlg::parseActionNotify");
	json_spirit::Value observable_val;
	if (!GdaJson::findValue(v, observable_val, "observable")) {
		throw std::runtime_error("could not find observable");
	}
	if (observable_val.get_str() == "HighlightState") {
		parseHighlightStateNotify(v);
	} else if (observable_val.get_str() == "TimeState") {
		parseTimeStateNotify(v);
	} else {
		throw std::runtime_error("no handler defined for observable "
								 + observable_val.get_str());
	}
}

void WebViewExampleDlg::parseHighlightStateNotify(const json_spirit::Value& v)
{
	LOG_MSG("In WebViewExampleDlg::parseHighlightStateNotify");
	json_spirit::Value ev_type;
	if (!GdaJson::findValue(v, ev_type, "event_type")) {
		throw std::runtime_error("could not find event_type");
	}
	bool notify = false;
	LOG_MSG(ev_type.get_str());
	if (ev_type.get_str() == "delta") {
		json_spirit::Value new_hl;
		if (!GdaJson::findValue(v, new_hl, "newly_highlighted")) {
			throw std::runtime_error("newly_highlighted not found");
		}
		json_spirit::Value new_uhl;
		if (!GdaJson::findValue(v, new_uhl, "newly_unhighlighted")) {
			throw std::runtime_error("newly_unhighlighted not found");
		}
		std::vector<int> new_hl_vec;
		if (!GdaJson::arrayToVec(new_hl.get_array(), new_hl_vec)) {
			throw std::runtime_error("could not convert Array to vector<int>");
		}
		std::vector<int> new_uhl_vec;
		if (!GdaJson::arrayToVec(new_uhl.get_array(), new_uhl_vec)) {
			throw std::runtime_error("could not convert Array to vector<int>");
		}
		LOG(new_hl_vec.size());
		LOG(new_uhl_vec.size());
		if (new_hl_vec.size() > 0 || new_uhl_vec.size() > 0) {
			std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
			std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
			for (size_t i=0, sz=new_hl_vec.size(); i<sz; ++i) {
				nh[i] = new_hl_vec[i];
			}
			for (size_t i=0, sz=new_uhl_vec.size(); i<sz; ++i) {
				nuh[i] = new_uhl_vec[i];
			}
			highlight_state->SetEventType(HLStateInt::delta);
			highlight_state->SetTotalNewlyHighlighted(new_hl_vec.size());
			highlight_state->SetTotalNewlyUnhighlighted(new_uhl_vec.size());
			notify = true;
		}
	} else if (ev_type.get_str() == "unhighlight_all") {
		highlight_state->SetEventType(HLStateInt::unhighlight_all);
		notify = true;
	} else if (ev_type.get_str() == "invert") {
		highlight_state->SetEventType(HLStateInt::invert);
		notify = true;
	} else if (ev_type.get_str() == "empty") {
		LOG_MSG("Empty HighlightState event is empty.  Not notifying.");
	} else {
		wxString s("HighlightState event unrecognized: ");
		s << ev_type.get_str();
		LOG_MSG(s);
		throw std::runtime_error(s.ToStdString());
	}
	if (notify) {
		LOG_MSG("About to call HighlightState::notifyObservers, excluding "
				"WebViewExampleDlg::this");
		highlight_state->notifyObservers(this);
	}
}

void WebViewExampleDlg::parseTimeStateNotify(const json_spirit::Value& v)
{
	json_spirit::Value new_time;
	if (!GdaJson::findValue(v, new_time, "time")) {
		throw std::runtime_error("could not find time");
	}
	int tm = new_time.get_int();
	if (tm < 0 || tm >= time_state->GetTimeSteps()) {
		wxString s = "time value out of range: ";
		s << tm;
		throw std::runtime_error(s.ToStdString());
	}
	time_state->SetCurrTime(tm);
	LOG_MSG("About to call TimeState::notifyObservers, excluding "
			"WebViewExampleDlg::this");
	time_state->notifyObservers(this);
}

/**
 Requests come as an array of requests along with a callback_id.  Here's
 an example of two Table::GetName requests.
 
 {  "action": "request",
	"callback_id": "13032",
	"requests" : [
		{ "interface": "table",
			"operation": "getName",
			"params": { "col": 4, "time": 0 }
		},
		{ "interface": "table",
			"operation": "getName",
			"params": { "col": 2, "time": 0 }
		}
	]
 }
 
 Valid requests are given a response.  An example response for the
 above request could be:
 
 {
	"callback_id": 13032,
	"responses" : [ "HOVAL", "AREA" ]
 }

 JS function gda.response() is then called with this JSON object as
 the parameter.
 
 Other examples:
 
 { "action": "request",
	"callback_id": "43",
	"requests" : [
		{ "interface": "table",
			"operation": "GetColData",
			"col": 3
		},
		{ "interface": "table",
			"operation": "GetColData",
			"col": 6
			}
		]
 }
 
 response:
 
 { "callback_id": "43",
	"responses": [
		[ [1, 2, 3, 4, 5, 6], [2.1, 2.2, 2.3, 2.4, 2.5, 2.6] ],
		[ [13, 22, 43, 24, 54, 16], [23.1, 22.2, 42.3, 22.4, 221.5, 21.6] ]
	]
 }

 
 */
void WebViewExampleDlg::parseActionRequest(const json_spirit::Value& v)
{
	using namespace json_spirit;
	using namespace GdaJson;
	
	Value cb_v;
	if (!findValue(v, cb_v, "callback_id")) {
		throw std::runtime_error("could not find callback_id");
	}
	std::string cb_id = cb_v.get_str();
	LOG_MSG("Entering WebViewExampleDlg::parseActionRequest");

	Value requests_v;
	if (!findValue(v, requests_v, "requests")) {
		throw std::runtime_error("could not find requests array");
	}
	Array& req_a = requests_v.get_array();
	
	Object ret_obj;
	ret_obj.push_back(Pair("callback_id", cb_id));
	
	Array resp_a;
	for (size_t i=0, sz=req_a.size(); i<sz; ++i) {
		Value resp_val;
		if (req_a[i].type() != json_spirit::obj_type) {
			throw std::runtime_error("found a request not of JSON object type.");
		}
		if (getStrValFromObj(req_a[i], "interface") == "project" &&
			getStrValFromObj(req_a[i], "operation") == "promptVarSettings") {
			promptVarSettings(req_a[i].get_obj(), resp_val);
		}
		resp_a.push_back(resp_val);
	}
	ret_obj.push_back(Pair("responses", resp_a));
	
	std::string json_str = write(ret_obj);
	wxString js_call;
	js_call << "gda.response(" << json_str << ");";
	LOG_MSG(js_call);
	web_view->RunScript(js_call);
}

/**
 
 { "action": "request",
	"callback_id": "53",
	"requests" : [ {
		"interface": "project",
		"operation": "promptVarSettings",
		"arity": "bivariate", // also (uni/bi/tri/quad)variate
		"show_weights": false, // default
		"title": "Variable Settings" // optional
		"var1_title": "First Variable (X)" // optional
		"var2_title": "Second Variable (Y)", // optional
		"var3_title": "Third Variable (Z)", // optional
		"var4_title": "Fourth Variable" // optional
	}]
 }
 
 response:
 
 { "callback_id": "43",
	"responses": [
		{ "time_info": [ "1960", "1970", "1980", "1990" ],
		"current_time": 0,
		"var1": {
			"col": 4,
			"name": "POP",
			"time": 0,
			"time_variant": true,
			"displayed_decimals": 4, // or -1 for default
			"type": "real"
			"data": [ [1, 2, 3, 4, 5, 6], [2.1, 2.2, 2.3, 2.4, 2.5, 2.6] ] ],
		},
		"var2": {},
		"weights": { // full weights details and actual weights
		  },
		"selected": [1, 2, 3, 32]
		}
	]
 }
 
 or response:
 
 { "callback_id": "43",
	"responses": [
		{ "user_cancel" : null } ]
 }

 */
void WebViewExampleDlg::promptVarSettings(const json_spirit::Object& req,
										  json_spirit::Value& response_val)
{
	using namespace json_spirit;
	using namespace GdaJson;
	using namespace std;
	
	VariableSettingsDlg::VarType arity = VariableSettingsDlg::univariate;
	wxString arity_str = getStrValFromObj(req, "arity");
	if (arity_str == "univariate") {
		arity = VariableSettingsDlg::univariate;
	} else if (arity_str == "bivariate") {
		arity = VariableSettingsDlg::bivariate;
	} else if (arity_str == "trivariate") {
		arity = VariableSettingsDlg::trivariate;
	} else if (arity_str == "quadvariate") {
		arity = VariableSettingsDlg::quadvariate;
	} else {
		throw std::runtime_error("arity missing or invalid");
	}
	wxString title = "Variable Settings";
	wxString var1_title = "First Variable (X)";
	wxString var2_title = "Second Variable (Y)";
	wxString var3_title = "Third Variable (Z)";
	wxString var4_title = "Fourth Variable";	
	if (hasName(req, "var1_title")) {
		var1_title = getStrValFromObj(req, "var1_title");
	}
	if (hasName(req, "var2_title")) {
		var2_title = getStrValFromObj(req, "var2_title");
	}
	if (hasName(req, "var3_title")) {
		var3_title = getStrValFromObj(req, "var3_title");
	}
	if (hasName(req, "var4_title")) {
		var4_title = getStrValFromObj(req, "var4_title");
	}
	bool show_weights = getBoolValFromObj(req, "show_weights");
	
	VariableSettingsDlg VS(project, arity, show_weights, false, title,
						   var1_title, var2_title, var3_title, var4_title);
	if (VS.ShowModal() != wxID_OK) {
		wxString s;
		s << "{ \"user_cancel\": null}";
		response_val = Value(s.ToStdString());
		return;
	}
	
	Object response_obj;
	
	vector<string> var_nms;
	var_nms.push_back("var1");
	var_nms.push_back("var2");
	var_nms.push_back("var3");
	var_nms.push_back("var4");
	
	vector<wxString> tm_strs;
	table_int->GetTimeStrings(tm_strs);
	Value tm_strs_val;
	toValue(tm_strs_val, tm_strs);
	response_obj.push_back(Pair("time_info", tm_strs_val));
	response_obj.push_back(toPair("current_time", time_state->GetCurrTime()));
	
	for (size_t i=0; i<VS.col_ids.size(); ++i) {
		Object o;
		int col = VS.col_ids[i];
		o.push_back(toPair("col", col));
		o.push_back(toPair("name", VS.var_info[i].name));
		o.push_back(toPair("time", VS.var_info[i].time));
		o.push_back(toPair("time_variant", VS.var_info[i].is_time_variant));
		o.push_back(toPair("displayed_decimals",
						   table_int->GetColDispDecimals(col)));
		GdaConst::FieldType ft = table_int->GetColType(col);
		o.push_back(toPair("type", GdaConst::FieldTypeToStr(ft)));
		Value data;
		b_array_type undefined;
		table_int->GetColUndefined(col, undefined);
		vector<GdaConst::FieldType> f_types = table_int->GetColTypes(col);
		if (ft == GdaConst::double_type) {
			d_array_type d;
			table_int->GetColData(col, d);
			toValue(data, d);
		} else if (ft == GdaConst::long64_type) {
			l_array_type d;
			table_int->GetColData(col, d);
			toValue(data, d);
		} else { // assume GdaConst::string
			s_array_type d;
			table_int->GetColData(col, d);
			toValue(data, d);
		}
		o.push_back(Pair("data", data));
		response_obj.push_back(Pair(var_nms[i], Value(o)));
	}
	
	// get current selection
	std::vector<int> sel;
	project->GetSelectedRows(sel);
	Value sel_val;
	toValue(sel_val, sel);
	response_obj.push_back(Pair("selected", sel_val));
	
	response_val = Value(response_obj);
	
	std::string json_str = write(response_obj);
	//LOG_MSG(json_str);
}

void WebViewExampleDlg::OnDocumentLoaded(wxWebViewEvent& ev)
{
	LOG_MSG("Entering WebViewExampleDlg::OnDocumentLoaded");
    //Only notify if the document is the main frame, not a subframe
    if (ev.GetURL() == web_view->GetCurrentURL()) {
		wxString s;
		s << "url: " << ev.GetURL();
		LOG_MSG(s);
		
		if (load_url_called) {
			web_view->RunScript("gda.readyToInit();");
			
			load_url_called = false;
		}
		
		//web_view->RunScript("document.body.style.background = 'orange';");
		//web_view->RunScript("AppendP(\"It Works!\");");
		
		/*
		if (load_url_called) {
			// call various JS functions to initialize.
			// initFromDataset expects the following init_obj JSON message:
			 // {
			 // var1_title: "title",
			 // var2_title: "title2",
			 // var1_data: [ 1, 2, 3, 4 ],
			 // var2_data: [ 1.1, 2.2, 3.3, 4.4 ],
			 // selected: [ 3, 12, 23, 33 ]
			 // }
			 
			using namespace json_spirit;
			Object o;
			
			o.push_back(Pair("var1_title", var_info[0].name.ToStdString()));
			o.push_back(Pair("var2_title", var_info[1].name.ToStdString()));
			
			std::vector<double> v1;
			std::vector<double> v2;
			table_int->GetColData(col_ids[0], var_info[0].time, v1);
			table_int->GetColData(col_ids[1], var_info[1].time, v2);
			Array v1_array;
			Array v2_array;
			for (size_t i=0, sz=v1.size(); i<sz; ++i) {
				v1_array.push_back(Value(v1[i]));
				v2_array.push_back(Value(v2[i]));
			}
			o.push_back(Pair("var1_data", v1_array));
			o.push_back(Pair("var2_data", v2_array));
			
			Array init_sel;
			std::vector<bool>& hs = highlight_state->GetHighlight();
			for (size_t i=0, sz=hs.size(); i<sz; ++i) {
				if (hs[i]) init_sel.push_back((int) i);
			}
			o.push_back(Pair("selected", init_sel));
			
			std::string json_str = write(o);
			wxString js_call;
			js_call << "gda.initFromDataset(" << json_str << ");";
			//LOG_MSG(js_call);
			web_view->RunScript(js_call);
			
			load_url_called = false;
		}
	*/
    }
	LOG_MSG("Exiting WebViewExampleDlg::OnDocumentLoaded");
}

void WebViewExampleDlg::LoadURL(const wxString& url)
{
	load_url_called = true;
	web_view->LoadURL(url);
}

void WebViewExampleDlg::update(HLStateInt* o)
{
	LOG_MSG("Entering WebViewExampleDlg::update(HLStateInt*)");
	if (o->GetEventType() == HLStateInt::empty) return;
	
	wxString js_method = "gda.update";
	wxString s;
	s << js_method << "({";
	s << "\"observable\": \"HighlightState\",\n";
	s << "\"event\": \"" << o->GetEventTypeStr() << "\"";
	if (o->GetEventType() == HLStateInt::delta) {
		s << ",\n";
		s << "\"newly_highlighted\": [";
		std::vector<int>& nh = o->GetNewlyHighlighted();
		for (size_t i=0, sz=o->GetTotalNewlyHighlighted(); i<sz; ++i) {
			s << nh[i];
			if (i+1 < sz) s << ",";
		}
		s << "],\n";
		s << "\"newly_unhighlighted\": [";
		std::vector<int>& nuh = o->GetNewlyUnhighlighted();
		for (size_t i=0, sz=o->GetTotalNewlyUnhighlighted(); i<sz; ++i) {
			s << nuh[i];
			if (i+1 < sz) s << ",";
		}
		s << "]";
	}
	s << "});";
	
	
	/*
	 gda.update({"update": "HighlightState",
	 "event": "delta",
	 "newly_highlighted": [3,2,32,4],
	 "newly_unhighlighted": [23, 6, 7]
	 };
	 */
	
	//LOG_MSG(s);
	web_view->RunScript(s);
	
	LOG_MSG("Exiting WebViewExampleDlg::update(HLStateInt*)");
}

void WebViewExampleDlg::update(FramesManager* o)
{	
}

void WebViewExampleDlg::update(TableState* o)
{
	LOG_MSG("In WebViewExampleDlg::update(TableState*)");
	TableState::EventType ev_type = o->GetEventType();
}

void WebViewExampleDlg::update(TimeState* o)
{
	LOG_MSG("Entering WebViewExampleDlg::update(TimeState*)");
	LOG(o->GetCurrTime());
	LOG(o->GetCurrTimeString());
	wxString js_method = "gda.update";
	wxString s;
	s << js_method << "({";
	s << "\"observable\": \"TimeState\",\n";
	s << "\"curr_time\": " << o->GetCurrTime() << ",\n";
	s << "\"curr_time_str\": \"" << o->GetCurrTimeString() << "\"";
	s << "});";
	
	LOG_MSG(s);
	web_view->RunScript(s);
	LOG_MSG("Exiting WebViewExampleDlg::update(TimeState*)");
}

void WebViewExampleDlg::update(WeightsManState* o)
{
	LOG_MSG("Entering WebViewExampleDlg::update(WeightsManState*)");
	LOG(o->GetEventTypeStr());
	wxString js_method = "gda.update";
	
	using namespace json_spirit;
	Object js_obj;
	
	js_obj.push_back(Pair("observable", "WeightsManState"));
	js_obj.push_back(Pair("event", o->GetEventTypeStr().ToStdString()));
	js_obj.push_back(Pair("weights_uuid",
						  boost::uuids::to_string(o->GetWeightsId())));
	if (o->GetEventType() == WeightsManState::name_change_evt) {
		js_obj.push_back(Pair("new_title",
							  project->GetWManInt()->
								GetTitle(o->GetWeightsId()).ToStdString()));
	}
	
	std::string json_str = write(js_obj);
	wxString js_call;
	js_call << js_method << "(" << json_str << ");";
	LOG_MSG(js_call);
	web_view->RunScript(js_call);
	
	
	LOG_MSG("Exiting WebViewExampleDlg::update(WeightsManState*)");
}




// wxWidgets Ticket: http://trac.wxwidgets.org/ticket/15548

wxString WebViewExampleDlg::get_blank()
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

wxString WebViewExampleDlg::get_hover_example()
{
	wxString s = "<html><body><div "
	"onmouseover=\"this.style.background='red';\" "
	"onmouseout=\"this.style.background='blue';\"> "
	"onmouseover and onmouseout test<br/><br/> "
	"Move your mouse over here<br/> "
	"and background color should change.<br/><br/> "
	"It works on OSX when you click (=wrong) "
	"</div></body></html>";

	return s;
}

wxString WebViewExampleDlg::get_d3_chord_diagram()
{
	wxString s;
	s << "<!DOCTYPE html>\n";
	s << "<meta charset=\"utf-8\">\n";
	s << "<style>\n";
	
	s << "body {\n";
	s << "font: 10px sans-serif;\n";
	s << "}\n";
	
	s << ".chord path {\n";
	s << "	fill-opacity: .67;\n";
	s << "stroke: #000;\n";
	s << "	stroke-width: .5px;\n";
	s << "}\n";
	
	s << "</style>\n";
	s << "<body>\n";
	s << "<script src=\"http://d3js.org/d3.v3.min.js\"></script>\n";
	s << "<script>\n";
	
	s << "// From http://mkweb.bcgsc.ca/circos/guide/tables/\n";
	s << "var matrix = [\n";
	s << "			  [11975,  5871, 8916, 2868],\n";
	s << "			  [ 1951, 10048, 2060, 6171],\n";
	s << "			  [ 8010, 16145, 8090, 8045],\n";
	s << "			  [ 1013,   990,  940, 6907]\n";
	s << "			  ];\n";
	
	s << "var chord = d3.layout.chord()\n";
    s << ".padding(.05)\n";
    s << ".sortSubgroups(d3.descending)\n";
    s << ".matrix(matrix);\n";
	
	s << "var width = 960,\n";
    s << "height = 500,\n";
    s << "innerRadius = Math.min(width, height) * .41,\n";
    s << "outerRadius = innerRadius * 1.1;\n";
	
	s << "var fill = d3.scale.ordinal()\n";
    s << ".domain(d3.range(4))\n";
    s << ".range([\"#000000\", \"#FFDD89\", \"#957244\", \"#F26223\"]);\n";
	
	s << "var svg = d3.select(\"body\").append(\"svg\")\n";
    s << ".attr(\"width\", width)\n";
    s << ".attr(\"height\", height)\n";
	s << ".append(\"g\")\n";
    s << ".attr(\"transform\", \"translate(\" + width / 2 + \",\" + height / 2 + \")\");\n";
	
	s << "svg.append(\"g\").selectAll(\"path\")\n";
    s << ".data(chord.groups)\n";
	s << ".enter().append(\"path\")\n";
    s << ".style(\"fill\", function(d) { return fill(d.index); })\n";
    s << ".style(\"stroke\", function(d) { return fill(d.index); })\n";
    s << ".attr(\"d\", d3.svg.arc().innerRadius(innerRadius).outerRadius(outerRadius))\n";
    s << ".on(\"mouseover\", fade(.1))\n";
    s << ".on(\"mouseout\", fade(1));\n";
	
	s << "var ticks = svg.append(\"g\").selectAll(\"g\")\n";
    s << ".data(chord.groups)\n";
	s << ".enter().append(\"g\").selectAll(\"g\")\n";
    s << ".data(groupTicks)\n";
	s << ".enter().append(\"g\")\n";
    s << ".attr(\"transform\", function(d) {\n";
	s << "	return \"rotate(\" + (d.angle * 180 / Math.PI - 90) + \")\"\n";
	s << "	+ \"translate(\" + outerRadius + \",0)\";\n";
    s << "});\n";
	
	s << "ticks.append(\"line\")\n";
    s << ".attr(\"x1\", 1)\n";
    s << ".attr(\"y1\", 0)\n";
    s << ".attr(\"x2\", 5)\n";
    s << ".attr(\"y2\", 0)\n";
    s << ".style(\"stroke\", \"#000\");\n";
	
	s << "ticks.append(\"text\")\n";
    s << ".attr(\"x\", 8)\n";
    s << ".attr(\"dy\", \".35em\")\n";
    s << ".attr(\"transform\", function(d) { return d.angle > Math.PI ? \"rotate(180)translate(-16)\" : null; })\n";
    s << ".style(\"text-anchor\", function(d) { return d.angle > Math.PI ? \"end\" : null; })\n";
    s << ".text(function(d) { return d.label; });\n";
	
	s << "svg.append(\"g\")\n";
    s << ".attr(\"class\", \"chord\")\n";
	s << ".selectAll(\"path\")\n";
    s << ".data(chord.chords)\n";
	s << ".enter().append(\"path\")\n";
    s << ".attr(\"d\", d3.svg.chord().radius(innerRadius))\n";
    s << ".style(\"fill\", function(d) { return fill(d.target.index); })\n";
    s << ".style(\"opacity\", 1);\n";
	
	s << "// Returns an array of tick angles and labels, given a group.\n";
	s << "function groupTicks(d) {\n";
	s << "	var k = (d.endAngle - d.startAngle) / d.value;\n";
	s << "	return d3.range(0, d.value, 1000).map(function(v, i) {\n";
	s << "		return {\n";
	s << "		angle: v * k + d.startAngle,\n";
	s << "		label: i % 5 ? null : v / 1000 + \"k\"\n";
	s << "		};\n";
	s << "	});\n";
	s << "}\n";
	
	s << "// Returns an event handler for fading a given chord group.\n";
	s << "function fade(opacity) {\n";
	s << "	return function(g, i) {\n";
	s << "	  svg.selectAll(\".chord path\")\n";
	s << "		.filter(function(d) { return d.source.index != i && d.target.index != i; })\n";
    s << "  .transition()\n";
    s << "    .style(\"opacity\", opacity);\n";
	s << "};\n";
	s << "}\n";
	
	s << "</script>\n";
	
	return s;
}

wxString WebViewExampleDlg::get_zoomable_map_tiles()
{
	wxString s;
	
	s << "<!DOCTYPE html>";
	s << "<meta charset=\"utf-8\">";
	s << "<style>";
	
	s << "body {";
	s << "margin: 0;";
	s << "}";
	
	s << ".map {";
	s << "position: relative;";
	s << "overflow: hidden;";
	s << "}";
	
	s << ".layer {";
	s << "position: absolute;";
	s << "}";
	
	s << ".tile {";
	s << "	pointer-events: none;";
	s << "position: absolute;";
	s << "width: 256px;";
	s << "height: 256px;";
	s << "}";
	
	s << ".info {";
	s << "position: absolute;";
	s << "bottom: 10px;";
	s << "left: 10px;";
	s << "}";
	
	s << "</style>";
	s << "<body>";
	s << "<script src=\"http://d3js.org/d3.v3.min.js\"></script>";
	s << "<script src=\"http://d3js.org/d3.geo.tile.v0.min.js\"></script>";
	s << "<script>";
	
	s << "var width = Math.max(960, window.innerWidth),";
    s << "height = Math.max(500, window.innerHeight),";
    s << "prefix = prefixMatch([\"webkit\", \"ms\", \"Moz\", \"O\"]);";
	
	s << "var tile = d3.geo.tile()";
    s << ".size([width, height]);";
	
	s << "var projection = d3.geo.mercator();";
	
	s << "var zoom = d3.behavior.zoom()";
    s << ".scale(1 << 12)";
    s << ".scaleExtent([1 << 9, 1 << 23])";
    s << ".translate([width / 2, height / 2])";
    s << ".on(\"zoom\", zoomed);";
	
	s << "var map = d3.select(\"body\").append(\"div\")";
    s << ".attr(\"class\", \"map\")";
    s << ".style(\"width\", width + \"px\")";
    s << ".style(\"height\", height + \"px\")";
    s << ".call(zoom)";
    s << ".on(\"mousemove\", mousemoved);";
	
    s << "var layer = map.append(\"div\")";
    s << ".attr(\"class\", \"layer\");";
	
    s << "var info = map.append(\"div\")";
    s << ".attr(\"class\", \"info\");";
	
    s << "zoomed();";
	
    s << "function zoomed() {";
    s << "	var tiles = tile";
	s << "	.scale(zoom.scale())";
	s << "	.translate(zoom.translate())";
	s << "	();";
	
	s << "	projection";
	s << "	.scale(zoom.scale() / 2 / Math.PI)";
	s << "	.translate(zoom.translate());";
	
	s << "	var image = layer";
	s << "	.style(prefix + \"transform\", matrix3d(tiles.scale, tiles.translate))";
	s << "	.selectAll(\".tile\")";
	s << "	.data(tiles, function(d) { return d; });";
	
	s << "	image.exit()";
	s << "	.remove();";
	
	s << "	image.enter().append(\"img\")";
	s << "	.attr(\"class\", \"tile\")";
	s << "	.attr(\"src\", function(d) { return \"http://\" + [\"a\", \"b\", \"c\", \"d\"][Math.random() * 4 | 0] + \".tiles.mapbox.com/v3/examples.map-i86nkdio/\" + d[2] + \"/\" + d[0] + \"/\" + d[1] + \".png\"; })";
	s << "	.style(\"left\", function(d) { return (d[0] << 8) + \"px\"; })";
	s << "	.style(\"top\", function(d) { return (d[1] << 8) + \"px\"; });";
	s << "}";
	
	s << "function mousemoved() {";
	s << "	info.text(formatLocation(projection.invert(d3.mouse(this)), zoom.scale()));";
	s << "}";
	
	s << "function matrix3d(scale, translate) {";
	s << "	var k = scale / 256, r = scale % 1 ? Number : Math.round;";
	s << "	return \"matrix3d(\" + [k, 0, 0, 0, 0, k, 0, 0, 0, 0, k, 0, r(translate[0] * scale), r(translate[1] * scale), 0, 1 ] + \")\";";
	s << "}";
	
	s << "function prefixMatch(p) {";
	s << "	var i = -1, n = p.length, s = document.body.style;";
	s << "	while (++i < n) if (p[i] + \"Transform\" in s) return \"-\" + p[i].toLowerCase() + \"-\";";
	s << "	return \"\";";
	s << "}";
	
	s << "function formatLocation(p, k) {";
	s << "	var format = d3.format(\".\" + Math.floor(Math.log(k) / 2 - 2) + \"f\");";
	s << "	return (p[1] < 0 ? format(-p[1]) + \"°S\" : format(p[1]) + \"°N\") + \" \"";
    s << "   + (p[0] < 0 ? format(-p[0]) + \"°W\" : format(p[0]) + \"°E\");";
	s << "}";
	
    s << "</script>";
	
	return s;
}

wxString WebViewExampleDlg::get_json_spirit_ex1()
{
	LOG_MSG("Entering WebViewExampleDlg::get_json_spirit_ex1");
	json_spirit::Value jv1;
	json_spirit::Object addr_obj;
	addr_obj.push_back( json_spirit::Pair( "house_number", 42 ) );
	addr_obj.push_back( json_spirit::Pair( "road", "East Street" ) );
	addr_obj.push_back( json_spirit::Pair( "town", "Newtown" ) );
	std::string s;
	s = json_spirit::write(addr_obj, json_spirit::pretty_print);
	wxString ws(s);
	LOG_MSG(ws);
	LOG_MSG("Exiting WebViewExampleDlg::get_json_spirit_ex1");
	return s;
}

wxString WebViewExampleDlg::get_d3_circles_ex1()
{
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html lang=\"en\">";
	s << "<head>";
    s << "<meta charset=\"utf-8\" />";
    s << "<title>D3 Test</title>";
	s << "<script src=\"http://d3js.org/d3.v3.min.js\"></script>";
    //s << "<script type=\"text/javascript\" src=\"http://d3js.org/d3.v3.min.js\"></script>";
    s << "<style type=\"text/css\">";
    s << "</style>";
    s << "<script type=\"text/javascript\">";
	s << "function setDataset()";
	s << "{";
	s << "	window.dataset = [];";
	s << "	var i;";
	s << "	for (i = 0; i < arguments.length; i++) {";
	s << "		window.dataset[i] = arguments[i];";
	s << "	}";
	s << "}";
    s << "</script>";
	s << "</head>";
	s << "<body>";
    s << "<script type=\"text/javascript\">";
	s << "var w = 500; // width";
	s << "var h = 50; // height";
	s << "setDataset(5, 10, 15, 20, 25);";
	s << "var svg = d3.select(\"body\").append(\"svg\");";
	s << "svg.selectAll(\"circle\")";
	s << ".data(dataset)";
	s << ".enter()";
	s << ".append(\"circle\")";
	s << ".attr(\"cx\", function(d, i) { return (i*50)+25; })";
	s << ".attr(\"cy\", h/2)";
	s << ".attr(\"r\", function(d) { return d; })";
	s << ".attr(\"fill\", \"orange\");";
    s << "</script>";
	s << "</body>";
	s << "</html>";
	return s;
}

wxString WebViewExampleDlg::get_hello_world()
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

wxString WebViewExampleDlg::get_bars_example()
{
	wxString s;
	s << "<!DOCTYPE html>\n";
	s << "<html lang=\"en\">\n";
	s << "<head>\n";
    s << "<meta charset=\"utf-8\">\n";
    s << "<title>D3 Demo: Making a bar chart with SVG</title>\n";
    s << "<script type=\"text/javascript\" src=\"http://d3js.org/d3.v3.min.js\"></script>\n";
    s << "<style type=\"text/css\">\n";
    s << "</style>\n";
	s << "</head>\n";
	s << "<body>\n";
    s << "<script type=\"text/javascript\">\n";
	s << "var w = 500;\n";
	s << "var h = 100;\n";
	s << "var barPadding = 1;\n";
	s << "var dataset = [ 5, 10, 13, 19, 21, 25, 22, 18, 15, 13,\n";
	s << "			   11, 12, 15, 20, 18, 17, 16, 18, 23, 25 ];\n";
	s << "//Create SVG element\n";
	s << "var svg = d3.select(\"body\")\n";
	s << ".append(\"svg\")\n";
	s << ".attr(\"width\", w)\n";
	s << ".attr(\"height\", h);\n";
	s << "svg.selectAll(\"rect\")\n";
	s << ".data(dataset)\n";
	s << ".enter()\n";
	s << ".append(\"rect\")\n";
	s << ".attr(\"x\", function(d, i) {\n";
	s << "	return i * (w / dataset.length);\n";
	s << "})\n";
	s << ".attr(\"y\", function(d) {\n";
	s << "	return h - (d * 4);\n";
	s << "})\n";
	s << ".attr(\"width\", w / dataset.length - barPadding)\n";
	s << ".attr(\"height\", function(d) {\n";
	s << "	return d * 4;\n";
	s << "})\n";
	s << ".attr(\"fill\", function(d) {\n";
	s << "	return \"rgb(0, 0, \" + (d * 10) + \")\";\n";
	s << "});\n";
    s << "</script>\n";
	s << "</body>\n";
	s << "</html>\n";
	return s;
}


