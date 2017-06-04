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

#include <boost/foreach.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/valnum.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../Project.h"
#include "../SaveButtonManager.h"
#include "../DataViewer/TableInterface.h"
#include "../Explore/ConnectivityHistView.h"
#include "../Explore/ConnectivityMapView.h"
#include "../HighlightState.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "WeightsManDlg.h"

BEGIN_EVENT_TABLE(WeightsManFrame, TemplateFrame)
	EVT_ACTIVATE(WeightsManFrame::OnActivate)
END_EVENT_TABLE()

WeightsManFrame::WeightsManFrame(wxFrame *parent, Project* project,
								 const wxString& title, const wxPoint& pos,
								 const wxSize& size, const long style)
: TemplateFrame(parent, project, title, pos, size, style),
conn_hist_canvas(0),
conn_map_canvas(0),
project_p(project),
w_man_int(project->GetWManInt()), w_man_state(project->GetWManState()),
table_int(project->GetTableInt()), suspend_w_man_state_updates(false),
create_btn(0), load_btn(0), remove_btn(0), w_list(0)
{
	wxLogMessage("Entering WeightsManFrame::WeightsManFrame");
	
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	
	create_btn = new wxButton(panel, XRCID("ID_CREATE_BTN"), "Create",  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
	load_btn = new wxButton(panel, XRCID("ID_LOAD_BTN"), "Load", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
	remove_btn = new wxButton(panel, XRCID("ID_REMOVE_BTN"), "Remove", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    histogram_btn = new wxButton(panel, XRCID("ID_HISTOGRAM_BTN"), "Histogram", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    connectivity_map_btn = new wxButton(panel, XRCID("ID_CONNECT_MAP_BTN"), "Connectivity Map", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	
	Connect(XRCID("ID_CREATE_BTN"), wxEVT_BUTTON, wxCommandEventHandler(WeightsManFrame::OnCreateBtn));
	Connect(XRCID("ID_LOAD_BTN"), wxEVT_BUTTON, wxCommandEventHandler(WeightsManFrame::OnLoadBtn));
	Connect(XRCID("ID_REMOVE_BTN"), wxEVT_BUTTON, wxCommandEventHandler(WeightsManFrame::OnRemoveBtn));
    Connect(XRCID("ID_HISTOGRAM_BTN"), wxEVT_BUTTON, wxCommandEventHandler(WeightsManFrame::OnHistogramBtn));
    Connect(XRCID("ID_CONNECT_MAP_BTN"), wxEVT_BUTTON, wxCommandEventHandler(WeightsManFrame::OnConnectMapBtn));

	w_list = new wxListCtrl(panel, XRCID("ID_W_LIST"), wxDefaultPosition, wxSize(-1, 100), wxLC_REPORT);
    
	// Note: search for "ungrouped_list" for examples of wxListCtrl usage.
	w_list->AppendColumn("Weights Name");
	w_list->SetColumnWidth(TITLE_COL, 300);
	InitWeightsList();
	
	Connect(XRCID("ID_W_LIST"), wxEVT_LIST_ITEM_SELECTED, wxListEventHandler(WeightsManFrame::OnWListItemSelect));
	Connect(XRCID("ID_W_LIST"), wxEVT_LIST_ITEM_DESELECTED, wxListEventHandler(WeightsManFrame::OnWListItemDeselect));
	
	details_win = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr, wxDefaultPosition, wxSize(-1, 200));

	// Arrange above widgets in panel using sizers.
	// Top level panel sizer will be panel_h_szr
	// Below that will be panel_v_szr
	// panel_v_szr will directly receive widgets
	
	wxBoxSizer* btns_row1_h_szr = new wxBoxSizer(wxHORIZONTAL);
	btns_row1_h_szr->Add(create_btn, 0, wxALIGN_CENTER_VERTICAL);
	btns_row1_h_szr->AddSpacer(5);
	btns_row1_h_szr->Add(load_btn, 0, wxALIGN_CENTER_VERTICAL);
	btns_row1_h_szr->AddSpacer(5);
	btns_row1_h_szr->Add(remove_btn, 0, wxALIGN_CENTER_VERTICAL);
	
    wxBoxSizer* btns_row2_h_szr = new wxBoxSizer(wxHORIZONTAL);
    btns_row2_h_szr->Add(histogram_btn, 0, wxALIGN_CENTER_VERTICAL);
    btns_row2_h_szr->AddSpacer(5);
    btns_row2_h_szr->Add(connectivity_map_btn, 0, wxALIGN_CENTER_VERTICAL);
    btns_row2_h_szr->AddSpacer(5);
 
    
	wxBoxSizer* wghts_list_h_szr = new wxBoxSizer(wxHORIZONTAL);
	wghts_list_h_szr->Add(w_list);
	
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(btns_row1_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    panel_v_szr->AddSpacer(15);
	panel_v_szr->Add(wghts_list_h_szr, 0, wxALIGN_CENTER_HORIZONTAL);
	panel_v_szr->Add(details_win, 1, wxEXPAND);
	panel_v_szr->Add(btns_row2_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_h_szr);
	
	
	//wxBoxSizer* right_v_szr = new wxBoxSizer(wxVERTICAL);
	//conn_hist_canvas = new ConnectivityHistCanvas(this, this, project, boost::uuids::nil_uuid());
	//right_v_szr->Add(conn_hist_canvas, 1, wxEXPAND);
	
	// We have decided not to display the ConnectivityMapCanvas.  Uncomment
	// the following 4 lines to re-enable for shape-enabled projects.
	//if (!project->IsTableOnlyProject()) {
	//	conn_map_canvas = new ConnectivityMapCanvas(this, this, project,
	//												boost::uuids::nil_uuid());
	//	right_v_szr->Add(conn_map_canvas, 1, wxEXPAND);
	//}
	
	boost::uuids::uuid default_id = w_man_int->GetDefault();
	SelectId(default_id);
	UpdateButtons();
	
	// Top Sizer for Frame
	wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	//top_h_sizer->Add(right_v_szr, 1, wxEXPAND);
	
	wxColour panel_color = panel->GetBackgroundColour();
	SetBackgroundColour(panel_color);
	//hist_canvas->SetCanvasBackgroundColor(panel_color);
	
	SetSizerAndFit(top_h_sizer);
	DisplayStatusBar(false);

	w_man_state->registerObserver(this);
	Show(true);
	wxLogMessage("Exiting WeightsManFrame::WeightsManFrame");
}

WeightsManFrame::~WeightsManFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
	w_man_state->removeObserver(this);
}

void WeightsManFrame::OnHistogramBtn(wxCommandEvent& ev)
{
    wxLogMessage("WeightsManFrame::OnHistogramBtn()");
    boost::uuids::uuid id = GetHighlightId();
    if (id.is_nil()) return;
    ConnectivityHistFrame* f = new ConnectivityHistFrame(this, project_p, id);
}

void WeightsManFrame::OnConnectMapBtn(wxCommandEvent& ev)
{
    wxLogMessage("WeightsManFrame::OnConnectMapBtn()");
    boost::uuids::uuid id = GetHighlightId();
    if (id.is_nil()) return;
    ConnectivityMapFrame* f = new ConnectivityMapFrame(this, project_p, id, wxDefaultPosition, GdaConst::conn_map_default_size);
}

void WeightsManFrame::OnActivate(wxActivateEvent& event)
{
	wxLogMessage("In WeightsManFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("WeightsManFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void WeightsManFrame::OnWListItemSelect(wxListEvent& ev)
{
	wxLogMessage("In WeightsManFrame::OnWListItemSelect");
	long item = ev.GetIndex();
	SelectId(ids[item]);
	UpdateButtons();
	Refresh();
}

void WeightsManFrame::OnWListItemDeselect(wxListEvent& ev)
{
	//LOG_MSG("In WeightsManFrame::OnWListItemDeselect");
	//long item = ev.GetIndex();
	//LOG(item);
	//SelectId(boost::uuids::nil_uuid());
	//UpdateButtons();
	//Refresh();
}

void WeightsManFrame::OnCreateBtn(wxCommandEvent& ev)
{
	wxLogMessage("In WeightsManFrame::OnCreateBtn");
	GdaFrame::GetGdaFrame()->OnToolsWeightsCreate(ev);
}

void WeightsManFrame::OnLoadBtn(wxCommandEvent& ev)
{
	wxLogMessage("In WeightsManFrame::OnLoadBtn");
    wxFileName default_dir = project_p->GetWorkingDir();
    wxString default_path = default_dir.GetPath();
	wxFileDialog dlg( this, "Choose Weights File", default_path, "",
					 "Weights Files (*.gal, *.gwt)|*.gal;*.gwt");
	
    if (dlg.ShowModal() != wxID_OK) return;
	wxString path  = dlg.GetPath();
	wxString ext = GenUtils::GetFileExt(path).Lower();
	
	if (ext != "gal" && ext != "gwt") {
		wxString msg("Only 'gal' and 'gwt' weights files supported.");
		wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	WeightsMetaInfo wmi;
	wxString id_field = WeightUtils::ReadIdField(path);
	wmi.SetToCustom(id_field);
	wmi.filename = path;
	
	suspend_w_man_state_updates = true;
	
	// Check if weights already loaded and simply select and set as
	// new default if already loaded.
	boost::uuids::uuid id = w_man_int->FindIdByFilename(path);
	if (id.is_nil()) {
		//id = w_man_int->FindIdByMetaInfo(wmi);
	}
	if (!id.is_nil()) {
		HighlightId(id);
		SelectId(id);
		Refresh();
		suspend_w_man_state_updates = false;
		return;
	}
	
	GalElement* tempGal = 0;
	if (ext == "gal") {
		tempGal = WeightUtils::ReadGal(path, table_int);
	} else {
		tempGal = WeightUtils::ReadGwtAsGal(path, table_int);
	}
	if (tempGal == NULL) {
		// WeightsUtils read functions already reported any issues
		// to user when NULL returned.
		suspend_w_man_state_updates = false;
		return;
	}
    
    id = w_man_int->RequestWeights(wmi);
    if (id.is_nil()) {
        wxString msg("There was a problem requesting the weights file.");
        wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
        dlg.ShowModal();
        suspend_w_man_state_updates = false;
        return;
    }
	
	GalWeight* gw = new GalWeight();
	gw->num_obs = table_int->GetNumberRows();
	gw->wflnm = wmi.filename;
    gw->id_field = id_field;
	gw->gal = tempGal;

	if (!((WeightsNewManager*) w_man_int)->AssociateGal(id, gw)) {
		wxString msg("There was a problem associating the weights file.");
		wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
		dlg.ShowModal();
		delete gw;
		suspend_w_man_state_updates = false;
		return;
	}
	ids.push_back(id);
	long last = ids.size()-1;
	w_list->InsertItem(last, wxEmptyString);
	w_list->SetItem(last, TITLE_COL, w_man_int->GetTitle(id));
	w_man_int->MakeDefault(id);
	HighlightId(id);
	SelectId(id);
	Refresh();
	suspend_w_man_state_updates = false;
}

void WeightsManFrame::OnRemoveBtn(wxCommandEvent& ev)
{
	wxLogMessage("Entering WeightsManFrame::OnRemoveBtn");
	boost::uuids::uuid id = GetHighlightId();
	if (id.is_nil()) return;
	int nb = w_man_state->NumBlockingRemoveId(id);
	if (nb > 0) {
		wxString msg;
		msg << "There " << (nb==1 ? "is" : "are") << " ";
		if (nb==1) { msg << "one"; } else { msg << nb; }
		msg << " other " << (nb==1 ? "view" : "views");
		msg << " open that depend" << (nb==1 ? "s" : "");
		msg << " on this matrix. ";
		msg << "OK to close " << (nb==1 ? "this view" : "these views");
		msg << " and remove?";
		wxMessageDialog dlg(this, msg, "Notice", 
							wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
		if (dlg.ShowModal() == wxID_YES) {
			w_man_state->closeObservers(id, this);
			int nb = w_man_state->NumBlockingRemoveId(id);
			if (nb > 0) {
				// there was a problem closing some views
				wxString s;
				s << nb << " " << (nb==1 ? "view" : "views");
				s << " could not be closed. ";
				s << "Please manually close and try again.";
				wxMessageDialog dlg(this, s, "Error", wxICON_ERROR | wxOK);
				dlg.ShowModal();
			} else {
				w_man_int->Remove(id);
			}
		}
	} else {
		w_man_int->Remove(id);
	}
	LOG_MSG("Exiting WeightsManFrame::OnRemoveBtn");
}

/** Implementation of WeightsManStateObserver interface */
void WeightsManFrame::update(WeightsManState* o)
{
	wxLogMessage("In WeightsManFrame::update(WeightsManState* o)");
	if (suspend_w_man_state_updates) {
		return;
	}
	boost::uuids::uuid id = o->GetWeightsId();
	if (o->GetEventType() == WeightsManState::add_evt) {
		ids.push_back(id);
		if (!ids.size()-1 == w_list->GetItemCount()) {
		}
		long x = w_list->InsertItem(ids.size(), wxEmptyString);
		if (x == -1) {
		} else {
			w_list->SetItem(x, TITLE_COL, w_man_int->GetTitle(id));
		}
		HighlightId(id);
		Refresh();
	} else if (o->GetEventType() == WeightsManState::remove_evt) {
		std::vector<boost::uuids::uuid> new_ids;
		for (size_t i=0; i<ids.size(); ++i) {
			if (ids[i] == id) {
				w_list->DeleteItem(i);
			} else {
				new_ids.push_back(ids[i]);
			}
		}
		ids = new_ids;
		if (ids.size() > 0) HighlightId(ids[0]);
		SelectId(GetHighlightId());
	} else if (o->GetEventType() == WeightsManState::name_change_evt) {
		for (size_t i=0; i<ids.size(); ++i) {
			if (ids[i] == id) {
				// no need to change default
				w_list->SetItem(i, TITLE_COL, w_man_int->GetTitle(ids[i]));
			}
		}
		Refresh();
	}
	UpdateButtons();
}

void WeightsManFrame::OnShowAxes(wxCommandEvent& event)
{
	wxLogMessage("In WeightsManFrame::OnShowAxes");
	if (conn_hist_canvas) {
		conn_hist_canvas->ShowAxes(!conn_hist_canvas->IsShowAxes());
		UpdateOptionMenuItems();
	}
}

void WeightsManFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	wxLogMessage("In WeightsManFrame::OnDisplayStatistics");
	if (conn_hist_canvas) {
		conn_hist_canvas->DisplayStatistics(
				!conn_hist_canvas->IsDisplayStats());
		UpdateOptionMenuItems();
	}
}

void WeightsManFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	wxLogMessage("In WeightsManFrame::OnDisplayStatistics");
	if (conn_hist_canvas) {
		conn_hist_canvas->HistogramIntervals();
	}
}

void WeightsManFrame::OnSaveConnectivityToTable(wxCommandEvent& event)
{
	wxLogMessage("In WeightsManFrame::OnSaveConnectivityToTable");
	if (conn_hist_canvas) {
		conn_hist_canvas->SaveConnectivityToTable();
	}
}

void WeightsManFrame::OnSelectIsolates(wxCommandEvent& event)
{
	wxLogMessage("In WeightsManFrame::OnSelectIsolates");
	if (conn_hist_canvas) {
		conn_hist_canvas->SelectIsolates();
	}
}


/** During creation of frame, load weights from weights manager.
 This should only be called once.  After initial call, list will
 be kept synchronized through WeightsStateObserver::update. */
void WeightsManFrame::InitWeightsList()
{
	w_list->DeleteAllItems();
	ids.clear();
	w_man_int->GetIds(ids);
	boost::uuids::uuid def_id = w_man_int->GetDefault();
	for (size_t i=0; i<ids.size(); ++i) {
		w_list->InsertItem(i, wxEmptyString);
		w_list->SetItem(i, TITLE_COL, w_man_int->GetTitle(ids[i]));
		if (ids[i] == def_id) {
			w_list->SetItemState(i, wxLIST_STATE_SELECTED,
								 wxLIST_STATE_SELECTED);
		}
	}
}

void WeightsManFrame::SetDetailsForId(boost::uuids::uuid id)
{
	wxLogMessage("In WeightsManFrame::SetDetailsForItem");
	if (id.is_nil()) {
		SetDetailsWin(std::vector<wxString>(0), std::vector<wxString>(0));
		return;
	}
	std::vector<wxString> row_title;
	std::vector<wxString> row_content;
	
	WeightsMetaInfo wmi = w_man_int->GetMetaInfo(id);
	
	row_title.push_back("type");
	row_content.push_back(wmi.TypeToStr());
	
	row_title.push_back("symmetry");
	row_content.push_back(wmi.SymToStr());
	
	row_title.push_back("file");
	if (wmi.filename.IsEmpty()) {
		row_content.push_back("not saved");
	} else {
        wxFileName fm(wmi.filename);
		row_content.push_back(fm.GetFullName());
	}
	
	row_title.push_back("id variable");
	row_content.push_back(wmi.id_var);
	
	if (wmi.weights_type == WeightsMetaInfo::WT_rook ||
		wmi.weights_type == WeightsMetaInfo::WT_queen) {
		row_title.push_back("order");
		wxString rs;
		rs << wmi.order;
		row_content.push_back(rs);
		if (wmi.order > 1) {
			row_title.push_back("include lower orders");
			if (wmi.inc_lower_orders) {
				row_content.push_back("true");
			} else {
				row_content.push_back("false");
			}
		}
	} else if (wmi.weights_type == WeightsMetaInfo::WT_knn ||
			   wmi.weights_type == WeightsMetaInfo::WT_threshold) {
		row_title.push_back("distance metric");
		row_content.push_back(wmi.DistMetricToStr());
		
		row_title.push_back("distance vars");
		row_content.push_back(wmi.DistValsToStr());
		
		if (wmi.weights_type == WeightsMetaInfo::WT_threshold) {
			row_title.push_back("distance unit");
			row_content.push_back(wmi.DistUnitsToStr());
		}
		
		if (wmi.weights_type == WeightsMetaInfo::WT_knn) {
			row_title.push_back("neighbors");
			wxString rs;
			rs << wmi.num_neighbors;
			row_content.push_back(rs);
		} else {
			row_title.push_back("threshold value");
			wxString rs;
			rs << wmi.threshold_val;
			row_content.push_back(rs);
		}
	}
	SetDetailsWin(row_title, row_content);
}

void WeightsManFrame::SetDetailsWin(const std::vector<wxString>& row_title,
									const std::vector<wxString>& row_content)
{
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "  <style type=\"text/css\">";
	
	s << "  body {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    font-size: small;";
	s << "  }";
	
	s << "  h1 {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    color: blue;";
	s << "  }";
	
	s << "  #my_table {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    width: 100%;";
    s << "    border-collapse: collapse;";
	s << "  }";
	
	s << "  #my_table td, #my_table th {";
	s << "    font-size: 1em;";
	s << "    border: 1px solid #98bf21;";
	s << "    padding: 3px 7px 2px 7px;";
	s << "  }";
	
	s << "  #my_table th {";
	s << "    font-size: 1.1em;";
	s << "    text-align: left;";
	s << "    padding-top: 5px;";
	s << "    padding-bottom: 4px;";
	s << "    background-color: #A7C942;";
	s << "    color: #ffffff;";
	s << "  }";
	
	s << "  #my_table tr.alt td {";
	s << "    color: #000000;";
	s << "    background-color: #EAF2D3;";
	s << "  }";
	
	s <<   "</style>";
	s << "</head>";
	s << "<body>";
	s << "  <table id=\"my_table\">";
	s << "    <tr>";
	s << "      <th>Property</th>";
	s << "      <th>Value</th>";
	s << "    </tr>";
	for (size_t i=0, last=row_title.size()-1; i<last+1; ++i) {
		s << (i%2 == 0 ? "<tr>" : "<tr class=\"alt\">");
		
		s <<   "<td style=\"text-align:right; word-wrap: break-word\">";
		s <<      row_title[i] << "</td>";
		s <<   "<td>";
		if (row_title[i].CmpNoCase("file") == 0) {
			std::vector<wxString> parts;
			wxString html_formatted;
			int max_chars_per_part = 30;
			GenUtils::SplitLongPath(row_content[i], parts,
									html_formatted, max_chars_per_part);
			s << html_formatted;
		} else {
			s << row_content[i];
		}
		s <<   "</td>";
		
		s << "</tr>";
	}
	s << "  </table>";
	s << "</body>";
	s << "</html>";
	details_win->SetPage(s,"");
}

void WeightsManFrame::SelectId(boost::uuids::uuid id)
{
	w_man_int->MakeDefault(id);
	SetDetailsForId(id);
	if (conn_hist_canvas) conn_hist_canvas->ChangeWeights(id);
	if (conn_map_canvas) conn_map_canvas->ChangeWeights(id);
}

void WeightsManFrame::HighlightId(boost::uuids::uuid id)
{
	for (size_t i=0; i<ids.size(); ++i) {
		if (ids[i] == id) {
			w_list->SetItemState(i, wxLIST_STATE_SELECTED,
								 wxLIST_STATE_SELECTED);
			if (w_man_int->GetDefault() != id) {
				w_man_int->MakeDefault(id);
			}
		} else {
			// unselect all other items
			w_list->SetItemState(i, 0, wxLIST_STATE_SELECTED);
		}
	}
	w_man_int->MakeDefault(id);
}

boost::uuids::uuid WeightsManFrame::GetHighlightId()
{
	if (!w_list) return boost::uuids::nil_uuid();
	for (size_t i=0, sz=w_list->GetItemCount(); i<sz; ++i) {
		if (w_list->GetItemState(i, wxLIST_STATE_SELECTED) != 0) {
			return ids[i];
		}
	}
	return boost::uuids::nil_uuid();
}

void WeightsManFrame::UpdateButtons()
{
	bool any_sel = !GetHighlightId().is_nil();
	if (remove_btn) remove_btn->Enable(any_sel);
	if (histogram_btn) histogram_btn->Enable(any_sel);
	if (connectivity_map_btn) connectivity_map_btn->Enable(any_sel);
    if (project_p->isTableOnly) {
        connectivity_map_btn->Disable();
    }
}

