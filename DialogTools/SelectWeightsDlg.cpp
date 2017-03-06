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
#include <wx/textdlg.h>
#include <wx/xrc/xmlres.h>
#include "../FramesManager.h"
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "SelectWeightsDlg.h"


SelectWeightsDlg::SelectWeightsDlg(Project* project, wxWindow* parent,
                                const wxString& caption, wxWindowID id,
								   const wxPoint& pos, const wxSize& size,
								   long style)
: wxDialog(parent, id, caption, pos, size, style),
w_man_int(project->GetWManInt()), w_list(0), ok_btn(0), cancel_btn(0)
{
	LOG_MSG("Entering SelectWeightsDlg::SelectWeightsDlg");
	
	w_man_int->GetIds(ids);
    no_weights = ids.size() == 0 ? true : false;
	if (no_weights) {
		InitNoWeights();
	} else {
		InitNormal();
	}
	
	LOG_MSG("Exiting SelectWeightsDlg::SelectWeightsDlg");
}

void SelectWeightsDlg::InitNoWeights()
{
	panel = new wxPanel(this);
	ok_btn = new wxButton(panel, XRCID("wxID_OK"), "OK",
						  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	Connect(XRCID("wxID_OK"), wxEVT_BUTTON,
			wxCommandEventHandler(SelectWeightsDlg::OnOkClick));
	
	wxStaticText* ln1 = new wxStaticText(panel, wxID_ANY,
										 "No Weights Found.");
	wxStaticText* ln2 = new wxStaticText(panel, wxID_ANY,
                                         "This view requires weights, but none defined.\n Please use Tools > Weights > Weights Manager\n to define weights.");
 
	wxBoxSizer* v_szr = new wxBoxSizer(wxVERTICAL);
	v_szr->AddSpacer(35);
	v_szr->Add(ln1, 0, wxALIGN_CENTER_HORIZONTAL);
	v_szr->AddSpacer(4);
	v_szr->Add(ln2, 10, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT);
	v_szr->AddSpacer(25);
	v_szr->Add(ok_btn, 5, wxALIGN_CENTER_HORIZONTAL|wxALL);
	v_szr->AddSpacer(3);
	panel->SetSizer(v_szr);
	
	// Top Sizer
	wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 10);
	SetSizerAndFit(top_h_sizer);
	ok_btn->SetFocus();
}

void SelectWeightsDlg::InitNormal()
{
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	
	ok_btn = new wxButton(panel, XRCID("wxID_OK"), "OK",
						  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	cancel_btn = new wxButton(panel, XRCID("wxID_CANCEL"), "Cancel",
							  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	
	Connect(XRCID("wxID_OK"), wxEVT_BUTTON,
			wxCommandEventHandler(SelectWeightsDlg::OnOkClick));
	Connect(XRCID("wxID_CANCEL"), wxEVT_BUTTON,
			wxCommandEventHandler(SelectWeightsDlg::OnCancelClick));
	
	w_list = new wxListCtrl(panel, XRCID("ID_W_LIST"), wxDefaultPosition,
							wxSize(-1, 100), wxLC_REPORT);
	// Note: search for "ungrouped_list" for examples of wxListCtrl usage.
	w_list->AppendColumn("Weights Name");
	w_list->SetColumnWidth(TITLE_COL, 300);
	InitWeightsList();
	
	Connect(XRCID("ID_W_LIST"), wxEVT_LIST_ITEM_ACTIVATED,
			wxListEventHandler(SelectWeightsDlg::OnWListItemDblClick));
	Connect(XRCID("ID_W_LIST"), wxEVT_LIST_ITEM_SELECTED,
			wxListEventHandler(SelectWeightsDlg::OnWListItemSelect));
	Connect(XRCID("ID_W_LIST"), wxEVT_LIST_ITEM_DESELECTED,
			wxListEventHandler(SelectWeightsDlg::OnWListItemDeselect));
	
	details_win = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr,
								 wxDefaultPosition,
								 wxSize(-1, 200));
	
	// Arrange above widgets in panel using sizers.
	// Top level panel sizer will be panel_h_szr
	// Below that will be panel_v_szr
	// panel_v_szr will directly receive widgets
	
	wxBoxSizer* btns_row1_h_szr = new wxBoxSizer(wxHORIZONTAL);
	btns_row1_h_szr->Add(ok_btn, 0, wxALIGN_CENTER_VERTICAL);
	btns_row1_h_szr->AddSpacer(10);
	btns_row1_h_szr->Add(cancel_btn, 0, wxALIGN_CENTER_VERTICAL);
	
	wxBoxSizer* wghts_list_h_szr = new wxBoxSizer(wxHORIZONTAL);
	wghts_list_h_szr->Add(w_list);
	
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(wghts_list_h_szr, 0,
					 wxALIGN_CENTER_HORIZONTAL);
	
	panel_v_szr->Add(details_win, 1, wxEXPAND);
	
	panel_v_szr->Add(btns_row1_h_szr, 0,
					 wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_h_szr);
	
	boost::uuids::uuid default_id = w_man_int->GetDefault();
	SelectId(default_id);
	UpdateButtons();
	
	// Top Sizer
	wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	
	SetSizerAndFit(top_h_sizer);
	w_list->SetFocus();
}

// List item Enter key or double click
void SelectWeightsDlg::OnWListItemDblClick(wxListEvent& ev)
{
	LOG_MSG("In SelectWeightsDlg::OnWListItemDblClick");
	long item = ev.GetIndex();

	if (item < 0 || item >= ids.size()) return;

	HighlightId(ids[item]);	
	EndDialog(wxID_OK);
}

void SelectWeightsDlg::OnWListItemSelect(wxListEvent& ev)
{
	LOG_MSG("In SelectWeightsDlg::OnWListItemSelect");
	long item = ev.GetIndex();

	SelectId(ids[item]);
	UpdateButtons();
	Refresh();
}

void SelectWeightsDlg::OnWListItemDeselect(wxListEvent& ev)
{
	LOG_MSG("In SelectWeightsDlg::OnWListItemDeselect");
	long item = ev.GetIndex();

	SelectId(boost::uuids::nil_uuid());
	UpdateButtons();
	Refresh();
}

void SelectWeightsDlg::OnOkClick(wxCommandEvent& ev)
{
	LOG_MSG("In SelectWeightsDlg::OnOkClick");
	if (no_weights) {
		GdaFrame::GetGdaFrame()->OnToolsWeightsManager(ev);
		EndDialog(wxID_CANCEL);
	} else {
		EndDialog(wxID_OK);
	}
}

void SelectWeightsDlg::OnCancelClick(wxCommandEvent& ev)
{
	LOG_MSG("In SelectWeightsDlg::OnCancelClick");
	EndDialog(wxID_CANCEL);
}

/** During creation of frame, load weights from weights manager.
 This should only be called once.  After initial call, list will
 be kept synchronized through WeightsStateObserver::update. */
void SelectWeightsDlg::InitWeightsList()
{
	w_list->DeleteAllItems();
	ids.clear();
	w_man_int->GetIds(ids);
	for (size_t i=0; i<ids.size(); ++i) {
		w_list->InsertItem(i, "");
		w_list->SetItem(i, TITLE_COL, w_man_int->GetTitle(ids[i]));
		if (ids[i] == w_man_int->GetDefault()) {
			w_list->SetItemState(i, wxLIST_STATE_SELECTED,
								 wxLIST_STATE_SELECTED);
		}
	}
}

void SelectWeightsDlg::SetDetailsForId(boost::uuids::uuid id)
{
	LOG_MSG("In SelectWeightsDlg::SetDetailsForItem");
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

void SelectWeightsDlg::SetDetailsWin(const std::vector<wxString>& row_title,
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

void SelectWeightsDlg::SelectId(boost::uuids::uuid id)
{
	SetDetailsForId(id);
}

void SelectWeightsDlg::HighlightId(boost::uuids::uuid id)
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
}

boost::uuids::uuid SelectWeightsDlg::GetSelWeightsId()
{
	if (no_weights || !w_list) return boost::uuids::nil_uuid();
	for (size_t i=0, sz=w_list->GetItemCount(); i<sz; ++i) {
		if (w_list->GetItemState(i, wxLIST_STATE_SELECTED) != 0) {
			return ids[i];
		}
	}
	return boost::uuids::nil_uuid();
}

void SelectWeightsDlg::UpdateButtons()
{
	bool any_sel = !GetSelWeightsId().is_nil();
	if (ok_btn) ok_btn->Enable(any_sel);
	if (cancel_btn) cancel_btn->Enable(true);
}
