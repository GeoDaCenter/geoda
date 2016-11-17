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
#include <wx/textdlg.h>
#include <wx/valnum.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../GdaConst.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/WebViewHelpWin.h"
#include "../rc/GeoDaIcon-16x16.xpm"
#include "VarsChooserDlg.h"

VarsChooserFrame::VarsChooserFrame(GdaVarTools::Manager& var_man,
																	 Project* project_,
																	 bool allow_duplicates_,
																	 bool specify_times_,
																	 const wxString& help_html_,
																	 const wxString& help_title_,
																	 const wxString& title,
																	 const wxPoint& pos,
																	 const wxSize& size)
: wxFrame(NULL, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE),
VarsChooserObservable(var_man), project(project_),
allow_duplicates(allow_duplicates_), specify_times(specify_times_),
help_html(help_html_), help_title(help_html_.IsEmpty() ? "Help" : help_title_),
vars_list(0), include_list(0)
{
	LOG_MSG("Entering VarsChooserFrame::VarsChooserFrame");
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
	SetBackgroundColour(*wxWHITE);
	wxPanel* panel = new wxPanel(this);
	
	wxStaticText* vars_list_text = new wxStaticText(panel, wxID_ANY, "Variables");
	wxStaticText* include_list_text = new wxStaticText(panel, wxID_ANY, "Include");
	
	vars_list = new wxListBox(panel, XRCID("ID_VARS_LIST"), wxDefaultPosition,
														wxSize(-1, 150), 0, 0, wxLB_SINGLE);
	
	include_list = new wxListBox(panel, XRCID("ID_INCLUDE_LIST"),
															 wxDefaultPosition, wxSize(-1, 150),
															 0, 0, wxLB_SINGLE);
	
	Connect(XRCID("ID_VARS_LIST"), wxEVT_LISTBOX_DCLICK,
					wxCommandEventHandler(VarsChooserFrame::OnVarsListDClick));
	Connect(XRCID("ID_INCLUDE_LIST"), wxEVT_LISTBOX_DCLICK,
					wxCommandEventHandler(VarsChooserFrame::OnIncludeListDClick));

	wxButton* include_btn = new wxButton(panel, XRCID("ID_INCLUDE_BTN"), ">",
																			 wxDefaultPosition, wxDefaultSize,
																			 wxBU_EXACTFIT);
	wxButton* remove_btn = new wxButton(panel, XRCID("ID_REMOVE_BTN"), "<",
																			wxDefaultPosition, wxDefaultSize,
																			wxBU_EXACTFIT);
	wxButton* up_btn = new wxButton(panel, XRCID("ID_UP_BTN"), "Up",
																	wxDefaultPosition, wxDefaultSize,
																	wxBU_EXACTFIT);
	wxButton* down_btn = new wxButton(panel, XRCID("ID_DOWN_BTN"), "Down",
																		wxDefaultPosition, wxDefaultSize,
																		wxBU_EXACTFIT);
	wxButton* help_btn = 0;
	if (!help_html.IsEmpty()) {
		help_btn = new wxButton(panel, XRCID("ID_HELP_BTN"), "Help",
														wxDefaultPosition, wxDefaultSize,
														wxBU_EXACTFIT);
	}
	//wxButton* close_btn = new wxButton(panel, XRCID("ID_CLOSE_BTN"), "Cancel",
	//																		wxDefaultPosition, wxDefaultSize,
	//																		wxBU_EXACTFIT);
	
	Connect(XRCID("ID_INCLUDE_BTN"), wxEVT_BUTTON,
					wxCommandEventHandler(VarsChooserFrame::OnIncludeBtn));
	Connect(XRCID("ID_REMOVE_BTN"), wxEVT_BUTTON,
					wxCommandEventHandler(VarsChooserFrame::OnRemoveBtn));
	Connect(XRCID("ID_UP_BTN"), wxEVT_BUTTON,
					wxCommandEventHandler(VarsChooserFrame::OnUpBtn));
	Connect(XRCID("ID_DOWN_BTN"), wxEVT_BUTTON,
					wxCommandEventHandler(VarsChooserFrame::OnDownBtn));
	if (!help_html.IsEmpty()) {
		Connect(XRCID("ID_HELP_BTN"), wxEVT_BUTTON,
						wxCommandEventHandler(VarsChooserFrame::OnHelpBtn));
	}
	//Connect(XRCID("ID_CLOSE_BTN"), wxEVT_BUTTON,
	//				wxCommandEventHandler(VarsChooserFrame::OnCloseBtn));

	wxBoxSizer* up_down_h_szr = new wxBoxSizer(wxHORIZONTAL);
	up_down_h_szr->Add(up_btn);
	up_down_h_szr->AddSpacer(10);
	up_down_h_szr->Add(down_btn);
	if (!help_html.IsEmpty()) {
		up_down_h_szr->AddSpacer(10);
		up_down_h_szr->Add(help_btn);
	}
	wxBoxSizer* inc_list_v_szr = new wxBoxSizer(wxVERTICAL);
	inc_list_v_szr->Add(include_list, 1, wxEXPAND);
	inc_list_v_szr->AddSpacer(4);
	inc_list_v_szr->Add(up_down_h_szr, 0, wxALIGN_CENTER_HORIZONTAL);
	
	wxBoxSizer* inc_rem_szr = new wxBoxSizer(wxVERTICAL);
	inc_rem_szr->Add(include_btn);
	inc_rem_szr->AddSpacer(8);
	inc_rem_szr->Add(remove_btn);
	
	// Arrange above widgets in panel using sizers.
	// Top level panel sizer will be panel_h_szr
	// Below that will be panel_v_szr
	// panel_v_szr will directly receive widgets
	
	// format: rows, cols, vgap, hgap
	wxFlexGridSizer* fg_sizer = new wxFlexGridSizer(2, 3, 3, 5);
	fg_sizer->SetFlexibleDirection(wxBOTH);
	fg_sizer->Add(vars_list_text, 0, wxALIGN_CENTER_HORIZONTAL);
	fg_sizer->AddSpacer(1);
	fg_sizer->Add(include_list_text, 0, wxALIGN_CENTER_HORIZONTAL);
	fg_sizer->Add(vars_list, 1, wxEXPAND);
	fg_sizer->Add(inc_rem_szr, 0, wxALIGN_CENTER_VERTICAL);
	fg_sizer->Add(inc_list_v_szr, 1, wxEXPAND);
	fg_sizer->AddGrowableCol(0, 1);
	fg_sizer->AddGrowableCol(2, 1);
	fg_sizer->AddGrowableRow(1, 1);
	
	//wxBoxSizer* btns_row1_h_szr = new wxBoxSizer(wxHORIZONTAL);
	//btns_row1_h_szr->Add(help_btn, 0, wxALIGN_CENTER_VERTICAL);
	//btns_row1_h_szr->AddSpacer(8);
	//btns_row1_h_szr->Add(close_btn, 0, wxALIGN_CENTER_VERTICAL);
			
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(fg_sizer, 1, wxALL|wxEXPAND, 5);
	//panel_v_szr->AddSpacer(2);
	//panel_v_szr->Add(btns_row1_h_szr, 0,
	//								 wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	panel->SetSizer(panel_h_szr);
	
	// Top Sizer for Frame
	wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);
	top_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	
	UpdateLists();
	
	SetSizerAndFit(top_sizer);
	
	Show(true);
	
	LOG_MSG("Exiting VarsChooserFrame::VarsChooserFrame");
}

VarsChooserFrame::~VarsChooserFrame()
{
	LOG_MSG("In VarsChooserFrame::~VarsChooserFrame");
	notifyObserversOfClosing();
}

void VarsChooserFrame::OnVarsListDClick(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnVarsListDClick");
	IncludeFromVarsListSel(ev.GetSelection());
}

void VarsChooserFrame::OnIncludeListDClick(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnIncludeListDClick");
	RemoveFromIncludeListSel(ev.GetSelection());
}

void VarsChooserFrame::OnIncludeBtn(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnIncludeBtn");
	if (!vars_list || !include_list || vars_list->GetCount() == 0) return;
	IncludeFromVarsListSel(vars_list->GetSelection());
}

void VarsChooserFrame::OnRemoveBtn(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnRemoveBtn");
	if (!vars_list || !include_list || include_list->GetCount() == 0) return;
	RemoveFromIncludeListSel(include_list->GetSelection());
}

void VarsChooserFrame::OnUpBtn(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnUpBtn");
	if (!include_list || include_list->GetCount() <= 1) return;
	int sel = include_list->GetSelection();
	int count = include_list->GetCount();
	if (sel == wxNOT_FOUND || sel == 0) return;
	var_man.MoveVarForwardOne(sel);
	wxString a = include_list->GetString(sel-1);
	wxString b = include_list->GetString(sel);
	include_list->SetString(sel-1, b);
	include_list->SetString(sel, a);
	include_list->SetSelection(sel-1);
	notifyObservers();
	Refresh();
}

void VarsChooserFrame::OnDownBtn(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnDownBtn");
	if (!include_list || include_list->GetCount() <= 1) return;
	int sel = include_list->GetSelection();
	int count = include_list->GetCount();
	if (sel == wxNOT_FOUND || sel >= count-1) return;
	var_man.MoveVarBackOne(sel);
	wxString a = include_list->GetString(sel);
	wxString b = include_list->GetString(sel+1);
	include_list->SetString(sel, b);
	include_list->SetString(sel+1, a);
	include_list->SetSelection(sel+1);
	notifyObservers();
	Refresh();
}

void VarsChooserFrame::OnHelpBtn(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnHelpBtn");
	WebViewHelpWin* win = new WebViewHelpWin(project, help_html, NULL, wxID_ANY,
																					 help_title);
}

void VarsChooserFrame::OnCloseBtn(wxCommandEvent& ev)
{
	LOG_MSG("In VarsChooserFrame::OnCloseBtn");
	Close(true);
}

void VarsChooserFrame::UpdateFromTable()
{
	LOG_MSG("Entering VarsChooserFrame::UpdateFromTable");
	TableInterface* table_int = project->GetTableInt();
	notifyObservers();
	LOG_MSG("Exiting VarsChooserFrame::UpdateFromTable");
}

void VarsChooserFrame::closeAndDeleteWhenEmpty()
{
	Close(true);
}

void VarsChooserFrame::UpdateLists()
{
	LOG_MSG("In VarsChooserFrame::UpdateLists");
	TableInterface* table_int = project->GetTableInt();
	if (!table_int || !vars_list || !include_list) return;
	
	std::set<wxString> vm_names;
	include_list->Clear();
	for (size_t i=0, sz=var_man.GetVarsCount(); i<sz; ++i) {
		include_list->Append(var_man.GetName(i));
		vm_names.insert(var_man.GetName(i));
	}
	
	vars_list->Clear();
	std::vector<wxString> names;
	table_int->FillNumericNameList(names);
	for (size_t i=0, sz=names.size(); i<sz; ++i) {
		if (vm_names.find(names[i]) == vm_names.end()) {
			vars_list->Append(names[i]);
		}
	}
	if (vars_list->GetCount() > 0) vars_list->SetFirstItem(0);
	
}

void VarsChooserFrame::IncludeFromVarsListSel(int sel)
{
	if (!vars_list || !include_list || vars_list->GetCount() == 0) return;
	if (sel == wxNOT_FOUND) return;
	wxString name = vars_list->GetString(sel);
	int time = project->GetTimeState()->GetCurrTime();
	TableInterface* table_int = project->GetTableInt();
	int col_id = table_int->FindColId(name);
	if (col_id < 0 ) return;
	std::vector<double> min_vals;
	std::vector<double> max_vals;
	table_int->GetMinMaxVals(col_id, min_vals, max_vals);
	var_man.AppendVar(name, min_vals, max_vals, time);
	include_list->Append(name);
	vars_list->Delete(sel);
	if (vars_list->GetCount() > 0) {
		if (sel < vars_list->GetCount()) {
			vars_list->SetSelection(sel);
		} else {
			vars_list->SetSelection(vars_list->GetCount()-1);
		}
	}
	notifyObservers();
	Refresh();
}

void VarsChooserFrame::RemoveFromIncludeListSel(int sel)
{
	if (!vars_list || !include_list || include_list->GetCount() == 0) return;
	if (sel == wxNOT_FOUND) return;
	var_man.RemoveVar(sel);
	include_list->Delete(sel);
	
	TableInterface* table_int = project->GetTableInt();
	if (table_int) {
		std::set<wxString> vm_names;
		for (size_t i=0, sz=var_man.GetVarsCount(); i<sz; ++i) {
			vm_names.insert(var_man.GetName(i));
		}
		
		vars_list->Clear();
		std::vector<wxString> names;
		table_int->FillNumericNameList(names);
		for (size_t i=0, sz=names.size(); i<sz; ++i) {
			if (vm_names.find(names[i]) == vm_names.end()) {
				vars_list->Append(names[i]);
			}
		}
	}
	
	notifyObservers();
	Refresh();
}

wxString VarsChooserFrame::PrintState()
{
	wxString s;
	s << "var_man contents: \n";
	for (int i=0; i<var_man.GetVarsCount(); i++) {
		s << "  var_man " << i << ": " << var_man.GetName(i);
		if (i+1 < var_man.GetVarsCount()) s << "\n";
	}
	return s;
}

