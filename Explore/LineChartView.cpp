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

#include <utility> // std::pair
#include <stdlib.h>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/uuid/uuid.hpp>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/gauge.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include "../HighlightState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "LineChartCanvas.h"
#include "LineChartView.h"
#include "../DataViewer/OGRTable.h"
#include "../DialogTools/RegressionReportDlg.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../Regression/DiagnosticReport.h"
#include "../DialogTools/AdjustYAxisDlg.h"

#include "../Regression/Lite2.h"
#include "../GenUtils.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/GalWeight.h"

bool classicalRegression(GalElement *g, int num_obs, double * Y,
						 int dim, double ** X, 
						 int expl, DiagnosticReport *dr, bool InclConstant,
						 bool m_moranz, wxGauge* gauge,
						 bool do_white_test);

BEGIN_EVENT_TABLE(LineChartFrame, TemplateFrame)
	EVT_ACTIVATE(LineChartFrame::OnActivate)
END_EVENT_TABLE()

LineChartFrame::LineChartFrame(wxFrame *parent, Project* project,
							   const wxString& title,
							   const wxPoint& pos,
							   const wxSize& size)
: TemplateFrame(parent, project, title, pos, size, wxDEFAULT_FRAME_STYLE),
highlight_state(project->GetHighlightState()), 
panel(0),
panel_v_szr(0), 
panel_h_szr(0), 
ctrls_h_szr(0), 
title1_h_szr(0),
title2_h_szr(0), 
bag_szr(0), 
title1_txt(0), 
title2_txt(0),
display_stats(true),
compare_regimes(true),
compare_time_periods(false), 
compare_r_and_t(false),
selection_period(0),
tms_subset0(project->GetTableInt()->GetTimeSteps(), true),
tms_subset1(project->GetTableInt()->GetTimeSteps(), false),
tms_subset0_tm_inv(1, true), 
tms_subset1_tm_inv(1, false),
regReportDlg(0),
def_y_precision(1),
use_def_y_range(false),
has_selection(1),
has_excluded(1)
{
	wxLogMessage("Open LineChartFrame(Average Charts).");
    
    // Init variables
	supports_timeline_changes = true;
    int n_cols = project->GetTableInt()->GetNumberCols();
    for (int i=0; i<n_cols; i++) {
        if (project->GetTableInt()->IsColNumeric(i)) {
            wxString col_name = project->GetTableInt()->GetColName(i);
            variable_names.push_back(col_name);
        }
    }
    
    // UI
    /*
      -----------------------------------
     |       |                           |
     |lpanel |       rpanel              |
     |       |                           |
     |       |                           |
      -----------------------------------
     */
    SetBackgroundColour(*wxWHITE);
    wxSplitterWindow* splitter_win = 0;
    splitter_win = new wxSplitterWindow(this,-1, wxDefaultPosition,
                                        wxDefaultSize,
                                        wxSP_THIN_SASH |wxSP_NOBORDER | wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
    splitter_win->SetMinimumPaneSize(10);
    
    // Left Panel
    wxPanel* lpanel = new wxPanel(splitter_win);
    lpanel->SetBackgroundColour(*wxWHITE);
    
    // 0 vgap, 0 hgap
    wxFlexGridSizer* variable_sizer = new wxFlexGridSizer(2,2, 10, 5);
    variable_sizer->SetFlexibleDirection(wxBOTH);
    variable_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);
    
    wxStaticText* lbl_variable =new wxStaticText(lpanel, wxID_ANY, _("Variable:"));
    choice_variable = new wxChoice(lpanel, wxID_ANY, wxDefaultPosition,
                                   wxSize(230, -1));
    wxStaticText* lbl_groups =new wxStaticText(lpanel, wxID_ANY, _("Groups:"));
    choice_groups = new wxChoice(lpanel, wxID_ANY, wxDefaultPosition,
                                 wxSize(230, -1));
    variable_sizer->Add(lbl_variable, 1, wxEXPAND);
    variable_sizer->Add(choice_variable, 1, wxEXPAND);
    variable_sizer->Add(lbl_groups, 1, wxEXPAND);
    variable_sizer->Add(choice_groups, 1, wxEXPAND);
    
    wxStaticText* lbl_tests =new wxStaticText(lpanel, wxID_ANY,
                                              _("Difference-in-Means Test:"));
    
    wxFlexGridSizer* tests_sizer = new wxFlexGridSizer(2,4, 10, 5);
    tests_sizer->SetFlexibleDirection(wxBOTH);
    tests_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);
    
    wxStaticText* lbl_group1 =new wxStaticText(lpanel, wxID_ANY, _("Group 1:"));
    choice_group1 = new wxChoice(lpanel, wxID_ANY, wxDefaultPosition,
                                 wxSize(90, -1));
    wxStaticText* lbl_time1 =new wxStaticText(lpanel, wxID_ANY, _("Period 1:"));
    choice_time1 = new wxChoice(lpanel, wxID_ANY, wxDefaultPosition,
                                wxSize(80, -1));
    wxStaticText* lbl_group2 =new wxStaticText(lpanel, wxID_ANY, _("Group 2:"));
    choice_group2 = new wxChoice(lpanel, wxID_ANY, wxDefaultPosition,
                                 wxSize(90, -1));
    wxStaticText* lbl_time2 =new wxStaticText(lpanel, wxID_ANY, _("Period 2:"));
    choice_time2 = new wxChoice(lpanel,wxID_ANY, wxDefaultPosition,
                                wxSize(80, -1));
    
    tests_sizer->Add(lbl_group1, 1, wxEXPAND);
    tests_sizer->Add(choice_group1, 1, wxEXPAND);
    tests_sizer->Add(lbl_time1, 1, wxEXPAND);
    tests_sizer->Add(choice_time1, 1, wxEXPAND);
    tests_sizer->Add(lbl_group2, 1, wxEXPAND);
    tests_sizer->Add(choice_group2, 1, wxEXPAND);
    tests_sizer->Add(lbl_time2, 1, wxEXPAND);
    tests_sizer->Add(choice_time2, 1, wxEXPAND);
    
    //chk_run_test = new wxCheckBox(lpanel, wxID_ANY, "Run Diff-in-Diff Test");
    
    wxButton* btn_save_dummy = new wxButton(lpanel, wxID_ANY, _("Save Dummy"));
    wxButton* btn_apply = new wxButton(lpanel, wxID_ANY,_("Run Diff-in-Diff Test"));
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_apply, 1, wxALIGN_CENTER | wxALL, 10);
    btn_box->Add(btn_save_dummy, 1, wxALIGN_CENTER | wxALL, 10);

    chk_save_did = new wxCheckBox(lpanel, wxID_ANY, _("Save Test Results"));
    wxBoxSizer* chk_box = new wxBoxSizer(wxHORIZONTAL);
    chk_box->Add(chk_save_did, 1, wxALIGN_LEFT |wxLEFT, 10);
    
    
    wxHtmlWindow* wv = 0;
    wv = new wxHtmlWindow(lpanel, wxID_ANY, wxDefaultPosition, wxSize(230, 100));
    stats_wins.push_back(wv);
    wxBoxSizer* stats_box = new wxBoxSizer(wxHORIZONTAL);
    stats_box->Add(wv, 1, wxEXPAND | wxALL, 10);
    
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(variable_sizer, 0, wxALIGN_TOP | wxALL, 20);
    rbox->Add(lbl_tests, 0, wxALIGN_TOP| wxLEFT | wxRIGHT, 20);
    rbox->Add(tests_sizer, 0, wxALIGN_TOP | wxALL, 20);
    //rbox->Add(chk_run_test, 0, wxALIGN_TOP | wxLEFT | wxRIGHT | wxBOTTOM, 20);
    rbox->Add(btn_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    rbox->Add(chk_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT , 10);
    rbox->Add(stats_box, 1, wxALIGN_TOP | wxEXPAND | wxALL, 10);
    lpanel->SetSizerAndFit(rbox);
    
    // Right Panel
	panel = new wxPanel(splitter_win);
	panel->SetBackgroundColour(*wxWHITE);
	
	panel->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
	message_win = new wxHtmlWindow(panel, wxID_ANY,
                                   wxDefaultPosition,
                                   wxSize(380,-1));
	message_win->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
	
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap
	bag_szr->Add(message_win, wxGBPosition(0,0), wxGBSpan(1,1), wxEXPAND);
	bag_szr->SetFlexibleDirection(wxBOTH);
	bag_szr->AddGrowableCol(0, 1);
	bag_szr->AddGrowableRow(0, 1);

	panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	
	panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND | wxALL, 8);
	
	panel->SetSizer(panel_h_szr);
		
    splitter_win->SplitVertically(lpanel, panel, 380);
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(splitter_win, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
   
    // Init controls
    // -- variable control
    InitVariableChoiceCtrl();
    InitGroupsChoiceCtrl();
    InitGroup12ChoiceCtrl();
    InitTimeChoiceCtrl();
    
	DisplayStatusBar(true);
	notifyNewHoverMsg("");
	
	highlight_state->registerObserver(this);
	Show(true);

    // Init Canvas
    wxCommandEvent ev;
	OnVariableChoice(ev);
    
    btn_apply->Connect(wxEVT_BUTTON,
                       wxCommandEventHandler(LineChartFrame::OnApplyButton),
                       NULL, this);
    btn_save_dummy->Connect(wxEVT_BUTTON,
                            wxCommandEventHandler(LineChartFrame::OnSaveDummyTable),
                            NULL, this);
    
    Connect(XRCID("ID_USE_ADJUST_Y_AXIS"),
            wxEVT_MENU,
            wxCommandEventHandler(LineChartFrame::OnUseAdjustYAxis));
    Connect(XRCID("ID_ADJUST_Y_AXIS"),
            wxEVT_MENU,
            wxCommandEventHandler(LineChartFrame::OnAdjustYAxis));
    Connect(XRCID("ID_ADJUST_Y_AXIS_PRECISION"),
            wxEVT_MENU,
            wxCommandEventHandler(LineChartFrame::OnAdjustYAxisPrecision));
}

LineChartFrame::~LineChartFrame()
{
	highlight_state->removeObserver(this);
	if (HasCapture())
        ReleaseMouse();
	DeregisterAsActive();
}

void LineChartFrame::InitVariableChoiceCtrl()
{
    TableInterface* table_int = project->GetTableInt();
    if (table_int == NULL) {
        wxLogMessage("ERROR: Table interface NULL.");
        return;
    }
  
    int n_times = table_int->GetTimeSteps();

    wxString time_range_str("");
    if (n_times == 1) {
        time_range_str = wxString::Format("(%s)", table_int->GetTimeString(0));
    } else {
        time_range_str = wxString::Format("(%s-%s)",
                                          table_int->GetTimeString(0),
                                          table_int->GetTimeString(n_times-1));
    }
    
    for (size_t i=0, sz=variable_names.size(); i<sz; ++i) {
        wxString col_name = variable_names[i];
        int col = table_int->FindColId(col_name);
        if (table_int->IsColTimeVariant(col)) {
            col_name = col_name + " " + time_range_str;
        }
        choice_variable->Append(col_name);
    }
	choice_variable->SetSelection(0);
	choice_variable->Connect(wxEVT_CHOICE,
                             wxCommandEventHandler(LineChartFrame::OnVariableChoice),
                             NULL, this);
}

void LineChartFrame::InitGroupsChoiceCtrl()
{
    choice_groups->Append(_("Selected vs. Unselected"));
    choice_groups->Append(_("All"));
    choice_groups->SetSelection(0);
    
	choice_groups->Connect(wxEVT_CHOICE,
                           wxCommandEventHandler(LineChartFrame::OnGroupsChoice),
                           NULL, this);
    choice_group1->Connect(wxEVT_CHOICE,
                           wxCommandEventHandler(LineChartFrame::OnGroup1Choice),
                           NULL, this);
    choice_group2->Connect(wxEVT_CHOICE,
                           wxCommandEventHandler(LineChartFrame::OnGroup2Choice),
                           NULL, this);
}

void LineChartFrame::InitTimeChoiceCtrl()
{
    std::vector<wxString> tm_strs;
    project->GetTableInt()->GetTimeStrings(tm_strs);

    choice_time1->Clear();
    choice_time2->Clear();
    
   
    if (tm_strs.size() > 1) {
        
        for (size_t i=0; i<tm_strs.size(); i++ ) {
            wxString t_str = tm_strs[i];
            choice_time1->Append(t_str);
            choice_time2->Append(t_str);
        }
        
        int group_selection = choice_groups->GetSelection();
        if (group_selection == 0 &&
            choice_group1->GetSelection() != choice_group2->GetSelection())
        {
            choice_time1->SetSelection(0);
            choice_time2->SetSelection(0);
        } else {
            choice_time1->SetSelection(0);
            choice_time2->SetSelection(1);
        }
        
        choice_time1->Connect(wxEVT_CHOICE,
                              wxCommandEventHandler(LineChartFrame::OnTime1Choice),
                              NULL, this);
        choice_time2->Connect(wxEVT_CHOICE,
                              wxCommandEventHandler(LineChartFrame::OnTime2Choice),
                              NULL, this);
    }
}

void LineChartFrame::OnSelectionChange()
{
    int var_selection = choice_variable->GetSelection();
    int group_type = choice_groups->GetSelection();
    int group1 = choice_group1->GetSelection();
    int group2 = choice_group2->GetSelection();
    int time1 = choice_time1->GetSelection();
    int time2 = choice_time2->GetSelection();
   
    UpdateTitleText();
    
    // process variable name selection change
    TableInterface* table_int = project->GetTableInt();
    wxString col_name = variable_names[var_selection];
    int col = table_int->FindColId(col_name);
    
    wxLogMessage(wxString::Format("var: %s, time1:%d, time2:%d, group1:%d, group2:%d", col_name, time1, time2, group1, group2));
    
    std::vector<double> min_vals;
    std::vector<double> max_vals;
    table_int->GetMinMaxVals(col, min_vals, max_vals);
    
    std::vector<wxString> tm_strs;
    table_int->GetTimeStrings(tm_strs);
    var_man.ClearAndInit(tm_strs);
    
    int time = 0;
    var_man.AppendVar(col_name, min_vals, max_vals, time);
   
    // process group selection
    if (group_type == 0 ) {
        if (group1 != group2) {
            compare_time_periods = false;
            if (time1 > -1 && time2 > -1 && (time1 != time2) ) {
                compare_regimes = false;
                compare_r_and_t = true;
                if (choice_group1->GetSelection() == 0) {
                    // first choice is "selected"
                    has_selection = 1;
                    has_excluded = 2;
                } else {
                    has_selection = 2;
                    has_excluded = 1;
                }
            } else {
                compare_regimes = true;
                compare_r_and_t = false;
                has_selection = 1;
                has_excluded = 1;
            }
        } else {
            compare_time_periods = false;
            compare_regimes = false;
            compare_r_and_t = true;
            if (group1 == 0) {
                has_selection = 1;
                has_excluded = 0;
            } else {
                has_selection = 0;
                has_excluded = 1;
            }
        }
        
    } else {
        compare_time_periods = true;
        compare_regimes = false;
        compare_r_and_t = false;
        has_selection = -1;
        has_excluded = -1;
    }
    
    // process Time Selection
    for (size_t i=0; i< tms_subset0.size(); i++) {
        tms_subset0[i] = false;
        tms_subset1[i] = false;
    }
    if (time1 >= 0) {
        for (size_t i=0; i< tms_subset0.size(); i++) {
            tms_subset0[i] = (i == time1) ? true : false;
        }
    }
    if (time2 >= 0) {
        for (size_t i=0; i< tms_subset1.size(); i++) {
            tms_subset1[i] = (i == time2) ? true : false;
        }
    }
   
    // Update draw
    UpdateDataMapFromVarMan();
    SetupPanelForNumVariables(1);
   
    Refresh();
}

void LineChartFrame::OnApplyButton(wxCommandEvent &event)
{
    wxLogMessage("In LineChartFrame::OnApplyButton()");
    RunDIDTest();
}

void LineChartFrame::OnVariableChoice(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnVariableChoice()");
    
    int variable_selection = choice_variable->GetSelection();
    if (variable_selection < 0 )
        return;
    
    wxString col_name = variable_names[variable_selection];

    TableInterface* table_int = project->GetTableInt();
    int col = table_int->FindColId(col_name);
    
    if (!table_int->IsColTimeVariant(col_name) ||
        table_int->GetTimeSteps() <= 1) {
        choice_groups->SetSelection(0);
    }
    
    InitGroup12ChoiceCtrl();
    InitTimeChoiceCtrl();
    
    if (table_int->IsColTimeVariant(col_name)) {
        choice_time1->Enable(true);
        choice_time2->Enable(true);
        choice_time1->SetSelection(0);
        choice_time2->SetSelection(0);
    } else {
        choice_groups->SetSelection(0);
        choice_group1->Enable(true);
        choice_group2->Enable(true);
        choice_time1->SetSelection(-1);
        choice_time2->SetSelection(-1);
        choice_time1->Enable(false);
        choice_time2->Enable(false);
    }

    OnSelectionChange();
}

void LineChartFrame::OnTime1Choice(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnTime1Choice()");
    
    int time1_selection = choice_time1->GetSelection();
    int time2_selection = choice_time2->GetSelection();
    int group_selection = choice_groups->GetSelection();
   
    int time_count = choice_time1->GetCount();
    
    if (group_selection == 0 ) {
        if (choice_group1->GetSelection() != choice_group2->GetSelection()) {
            // sel vs excl
            choice_time2->SetSelection(time1_selection);
        } else {
            // sel vs sel or excl vs excl
            if (time2_selection == time1_selection ||
                time1_selection > time2_selection) {
                if (time1_selection +1 < time_count) {
                    choice_time2->SetSelection(time1_selection+1);
                } else {
                    wxMessageBox("Please select Period 1 < Period 2.");
                    choice_time1->SetSelection(time_count-2);
                    choice_time2->SetSelection(time_count-1);
                }
            }
        }
    } else {
        if (time2_selection == time1_selection||
            time1_selection > time2_selection) {
            if (time1_selection +1 < time_count) {
                choice_time2->SetSelection(time1_selection+1);
            } else {
                wxMessageBox("Please select Period 1 < Period 2.");
                choice_time1->SetSelection(time_count-2);
                choice_time2->SetSelection(time_count-1);
            }
        }
    }
    
    OnSelectionChange();
}

void LineChartFrame::OnTime2Choice(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnTime2Choice()");
    
    int time1_selection = choice_time1->GetSelection();
    int time2_selection = choice_time2->GetSelection();
    int group_selection = choice_groups->GetSelection();
    int time_count = choice_time1->GetCount();
   
    if (group_selection == 0 ) {
        if (time1_selection > time2_selection) {
            if (time2_selection - 1 >=0 ) {
                choice_time1->SetSelection(time2_selection-1);
            } else {
                wxMessageBox(_("Please select Period 2 > Period 1."));
                choice_time1->SetSelection(0);
                choice_time2->SetSelection(1);
            }
        }
        
    } else {
        if (time2_selection == time1_selection||
            time1_selection > time2_selection) {
            if (time2_selection - 1 >=0 ) {
                choice_time1->SetSelection(time2_selection-1);
            } else {
                wxMessageBox(("Please select Period 2 > Period 1."));
                choice_time1->SetSelection(0);
                choice_time2->SetSelection(1);
            }
        }
    }
    
    OnSelectionChange();
}

void LineChartFrame::OnGroupsChoice(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnGroupsChoice()");
    
    int variable_selection = choice_variable->GetSelection();
    if (variable_selection < 0)
        return;
    
    wxString col_name = variable_names[variable_selection];
    wxLogMessage(_("var name:") + col_name);
    
    TableInterface* table_int = project->GetTableInt();
    if (!table_int->IsColTimeVariant(col_name) ||table_int->GetTimeSteps() <= 1) {
        if (choice_groups->GetSelection() == 1) {
            wxMessageBox(_("Please select a time variable first, and make sure more than one time steps have been defined."));
            choice_groups->SetSelection(0);
            return;
        }
    }
    
    InitGroup12ChoiceCtrl();
    InitTimeChoiceCtrl();
    
    OnSelectionChange();
}

void LineChartFrame::OnGroup1Choice(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnGroupsChoice()");
    int variable_selection = choice_variable->GetSelection();
    if (variable_selection < 0)
        return;
    
    wxString col_name = variable_names[variable_selection];
    
    TableInterface* table_int = project->GetTableInt();
    
    int group_selection = choice_groups->GetSelection();
    int time1_selection = choice_time1->GetSelection();
    int time2_selection = choice_time2->GetSelection();
    
    if (group_selection == 0 ) {
        if (choice_group1->GetSelection() == choice_group2->GetSelection())
        {
            if (!table_int->IsColTimeVariant(col_name)) {
                choice_group2->SetSelection(1-choice_group1->GetSelection());
            } else {
                if (time2_selection == time1_selection) {
                    if (time1_selection -1 >=0)
                        choice_time1->SetSelection(time1_selection-1);
                    else if (time1_selection + 1 < (int)choice_time1->GetCount()) {
                        choice_time2->SetSelection(time1_selection+1);
                    } else {
                        choice_time1->SetSelection(-1);
                    }
                }

            }
            
        } else {
            choice_time1->SetSelection(choice_time2->GetSelection());
        }
    }
    OnSelectionChange();
}

void LineChartFrame::OnGroup2Choice(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnGroup2Choice()");
    int variable_selection = choice_variable->GetSelection();
    if (variable_selection < 0)
        return;
    
    wxString col_name = variable_names[variable_selection];
    
    TableInterface* table_int = project->GetTableInt();
    
    int group_selection = choice_groups->GetSelection();
    int time1_selection = choice_time1->GetSelection();
    int time2_selection = choice_time2->GetSelection();
    
    if (group_selection == 0 ) {
        if (choice_group1->GetSelection() == choice_group2->GetSelection())
        {
            if (!table_int->IsColTimeVariant(col_name)) {
                choice_group1->SetSelection(1- choice_group2->GetSelection());
            } else {
                if (time2_selection == time1_selection) {
                    if (time2_selection -1 >=0)
                        choice_time1->SetSelection(time2_selection-1);
                    else if (time2_selection + 1 < (int)choice_time2->GetCount()) {
                        choice_time2->SetSelection(time2_selection+1);
                    } else {
                        choice_time2->SetSelection(-1);
                    }
                }
            }
            
            
            //
        } else {
            choice_time2->SetSelection(choice_time1->GetSelection());
        }
    }
    OnSelectionChange();
}

void LineChartFrame::InitGroup12ChoiceCtrl()
{
    int group_selection = choice_groups->GetSelection();
    if (group_selection == 0 ) {
        choice_group1->Clear();
        choice_group1->Append("Selected");
        choice_group1->Append("Unselected");
        choice_group1->SetSelection(0);
        choice_group1->Enable(true);
        
        choice_group2->Clear();
        choice_group2->Append("Selected");
        choice_group2->Append("Unselected");
        choice_group2->SetSelection(1);
        choice_group2->Enable(true);
    } else {
        choice_group1->Clear();
        choice_group1->Append("All");
        choice_group1->Enable(false);
        
        choice_group2->Clear();
        choice_group2->Append("All");
        choice_group2->Enable(false);
    }
}

void LineChartFrame::OnMouseEvent(wxMouseEvent& event)
{
    if (event.RightUp()) {
        wxPoint pos = event.GetPosition();
        
        wxObject* t = event.GetEventObject();
        if (wxHtmlWindow* f = dynamic_cast<wxHtmlWindow*>(t)) {
            wxPoint f_pos = f->GetPosition();
            pos.x += f_pos.x;
            pos.y += f_pos.y;
        }
        
    	// Workaround for right-click not changing window focus in OSX / wxW 3.0
    	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
    	OnActivate(ae);
    	
    	wxMenu* optMenu;
    	optMenu = wxXmlResource::Get()->LoadMenu("ID_LINE_CHART_MENU_OPTIONS");
    	if (!optMenu) return;
    	
    	UpdateContextMenuItems(optMenu);
    	PopupMenu(optMenu, pos);
    	UpdateOptionMenuItems();
       
    }
}

void LineChartFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In LineChartFrame::OnActivate()");
		RegisterAsActive("LineChartFrame", GetTitle());
	}
}

void LineChartFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->LoadMenu("ID_LINE_CHART_MENU_OPTIONS");
	LineChartFrame::UpdateContextMenuItems(optMenu);
	
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void LineChartFrame::UpdateOptionMenuItems()
{
	//TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
	if (menu == wxNOT_FOUND) {
	} else {
		LineChartFrame::UpdateContextMenuItems(mb->GetMenu(menu));
	}
}

void LineChartFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_COMPARE_REGIMES"),
								  compare_regimes);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_COMPARE_TIME_PERIODS"),
								  compare_time_periods);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_COMPARE_REG_AND_TM_PER"),
								  compare_r_and_t);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  display_stats);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_ADJUST_Y_AXIS"),
                                  use_def_y_range);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_ADJUST_Y_AXIS"), use_def_y_range);
    
    if (var_man.IsAnyTimeVariant() == false) {
        GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_COMPARE_TIME_PERIODS"), false);
        GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_COMPARE_REG_AND_TM_PER"), false);
    }
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LineChartFrame::OnUseAdjustYAxis(wxCommandEvent& event)
{
	wxLogMessage("In LineChartFrame:OnUseAdjustYAxis()");
    if (use_def_y_range == false) {
        use_def_y_range = true;
        OnAdjustYAxis(event);
        
    } else {
        use_def_y_range = false;
        for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
            line_charts[i]->UpdateYAxis();
            line_charts[i]->UpdateAll();
        }
    }
}

void LineChartFrame::OnAdjustYAxis(wxCommandEvent& event)
{
	wxLogMessage("In LineChartFrame:OnAdjustYAxis()");
    double y_axis_min = 0;
    double y_axis_max = 0;
    
    for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        y_axis_min = line_charts[i]->GetYAxisMinVal();
        y_axis_max = line_charts[i]->GetYAxisMaxVal();
    }
    
    AdjustYAxisDlg dlg(y_axis_min, y_axis_max, this);
    if (dlg.ShowModal () != wxID_OK)
        return;
    
    def_y_min = dlg.s_min_val;
    def_y_max = dlg.s_max_val;
    
    for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        line_charts[i]->UpdateYAxis(def_y_min, def_y_max);
        line_charts[i]->UpdateAll();
    }
    
    Refresh();
}

void LineChartFrame::OnAdjustYAxisPrecision(wxCommandEvent& event)
{
	wxLogMessage("In LineChartFrame:OnAdjustYAxisPrecision()");
    AxisLabelPrecisionDlg dlg(def_y_precision, this);
    if (dlg.ShowModal () != wxID_OK) return;
    
    def_y_precision = dlg.precision;
    
    for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        line_charts[i]->UpdateYAxisPrecision(def_y_precision);
        line_charts[i]->UpdateAll();
    }
    
    Refresh();
}

void LineChartFrame::SaveDataAndResults(bool save_weights, bool save_did,
                                        double* m_yhat1, double* m_resid1)
{
    int variable_selection = choice_variable->GetSelection();
    if (variable_selection < 0 )
        return;
    
    int nTests = var_man.GetVarsCount();
    //nTests = 1; // only handle one variable at a time
    
    TableInterface* table_int = project->GetTableInt();
    const std::vector<bool>& hs(highlight_state->GetHighlight());
    int n_obs = project->GetNumRecords();
    
    std::vector<wxString> tm_strs;
    table_int->GetTimeStrings(tm_strs);
    
    //double** var_stack_array = new double*[nTests];
    //double *dummy_select_stack = NULL;
    //double *dummy_time_stack = NULL;
    
    size_t n_ts = 1;
    
    std::vector<std::vector<double> > var_stack_array;
    std::vector<wxInt64> dummy_select_stack;
    std::vector<wxInt64> dummy_time_stack;
    std::vector<wxInt64> interaction_stack;
    std::vector<wxInt64> id_stack;
    std::vector<wxInt64> newids;
    std::vector<wxString> period_stack;
    
    var_stack_array.resize(nTests);
    
    int idx_var = 0;
    wxString row_nm(var_man.GetName(idx_var));
    const vec_vec_dbl_type& Y(data_map[row_nm]);
    std::vector<bool>& undefs(data_map_undef[row_nm]);
    
    int valid_n_obs = 0;
    for (std::vector<bool>::iterator it = undefs.begin();
         it != undefs.end(); ++it)
    {
        if (*it == false)
            valid_n_obs += 1;
    }
    
    n_ts = Y.size();
    
    if (compare_regimes) {
        int n= 0;
        wxString col_name = variable_names[variable_selection];
        int col = table_int->FindColId(col_name);
        
        if (!table_int->IsColTimeVariant(col_name)) {
            n = valid_n_obs;
            var_stack_array[idx_var].resize(n);
            dummy_select_stack.resize(n);
            id_stack.resize(n);
            
            int idx = 0;
            for (int j=0; j<n_obs; j++) {
                if (undefs[j])
                    continue;
                var_stack_array[idx_var][idx] = Y[0][j];
                dummy_select_stack[idx] = hs[j] == true ? 1 : 0;
                id_stack[idx] = j;
                newids.push_back(idx+1);
                idx += 1;
            }
            
        } else {
            for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n+= valid_n_obs;
                }
            }
            if (n== 0) {
                wxMessageBox(_("Please choose Periods first."));
                return;
            }
            
            var_stack_array[idx_var].resize(n);
            dummy_select_stack.resize(n);
            period_stack.resize(n);
            id_stack.resize(n);
            
            int idx = 0;
            for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    for (int j=0; j<n_obs; j++) {
                        if (undefs[j])
                            continue;
                        var_stack_array[idx_var][idx] = Y[t][j];
                        dummy_select_stack[idx] = hs[j] == true ? 1 : 0;
                        id_stack[idx] = j;
                        period_stack[idx] = tm_strs[t];
                        newids.push_back(idx+1);
                        idx += 1;
                    }
                }
            }
        }
        
    } else if (compare_time_periods) {
        
        int n1 = 0, n2 = 0;
        for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset0[t]) {
                n1 += valid_n_obs;
            }
        }
        if (n1 == 0) {
            wxMessageBox(_("Please choose Period 1."));
            return;
        }
        for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset1[t]) {
                n2 += valid_n_obs;
            }
        }
        if (n2 == 0) {
            wxMessageBox(_("Please choose Period 2."));
            return;
        }
        
        int n = n1 + n2;
        
        var_stack_array[idx_var].resize(n);
        dummy_time_stack.resize(n);
        period_stack.resize(n);
        id_stack.resize(n);
        
        int idx = 0;
        
        for (int t=0; t<n_ts; t++) {
            if (tms_subset0[t] || tms_subset1[t]) {
                for (int j=0; j<n_obs; j++) {
                    if (undefs[j])
                        continue;
                    var_stack_array[idx_var][idx] = Y[t][j];
                    dummy_time_stack[idx] = tms_subset0[t] == true ? 0 : 1;
                    id_stack[idx] = j;
                    period_stack[idx] = tm_strs[t];
                    newids.push_back(idx+1);
                    idx += 1;
                }
            }
        }
        
    } else if (compare_r_and_t) {
        
        int n1 = 0, n2 = 0;
        for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset0[t]) {
                n1 += valid_n_obs;
            }
        }
        if (n1 == 0) {
            wxMessageBox(_("Please choose Period 1."));
            return;
        }
        for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset1[t]) {
                n2 += valid_n_obs;
            }
        }
        if (n2 == 0) {
            wxMessageBox(_("Please choose Period 2."));
            return;
        }
        
        
        int idx = 0;
        for (int t=0; t<n_ts; t++) {
            if (tms_subset0[t] || tms_subset1[t]) {
                bool filter_flag = false;
                if (tms_subset0[t]) {
                    filter_flag = choice_group1->GetSelection() == 0 ? true :false;
                    
                } else if (tms_subset1[t]) {
                    filter_flag = choice_group2->GetSelection() == 0 ? true :false;
                }
                
                for (int j=0; j<n_obs; j++) {
                    if (undefs[j] || hs[j] != filter_flag )
                        continue;
                    var_stack_array[idx_var].push_back(Y[t][j]);
                    dummy_select_stack.push_back(hs[j] == true ? 1 : 0);
                    dummy_time_stack.push_back(tms_subset0[t] == true ? 0 : 1);
                    interaction_stack.push_back(dummy_select_stack[idx] * dummy_time_stack[idx]);
                    period_stack.push_back(tm_strs[t]);
                    id_stack.push_back(j);
                    newids.push_back(idx+1);
                    idx += 1;
                }
            }
        }
    } // end if (compare_r_and_t)
    
    // create in-memory table
    OGRTable* mem_table_int = NULL;
    int n = 0;
    
    if (!newids.empty()) {
        // add ID column
        n = newids.size();
        if (mem_table_int == NULL) {
            mem_table_int = new OGRTable(n);
        }
        OGRColumn* id_col = new OGRColumnInteger("STID", 18, 0, n);
        id_col->UpdateData(newids);
        mem_table_int->AddOGRColumn(id_col);
    }
    
    if (!id_stack.empty()) {
        // add original ID
        n = id_stack.size();
        if (mem_table_int == NULL) {
            mem_table_int = new OGRTable(n);
        }
        
        bool using_default_id = true;
        
        WeightsManInterface* wmi = NULL;
        if (project && project->GetWManInt()) {
            wmi = project->GetWManInt();
            boost::uuids::uuid default_wid = wmi->GetDefault();
            if (!default_wid.is_nil()) {
                GalWeight* gw = wmi->GetGal(default_wid);
                wxString id_field = gw->id_field;
                
                vector<wxString> id_vec;
                int c_id = table_int->FindColId(id_field);
                if (c_id > 0) {
                    table_int->GetColData(c_id, 1, id_vec); // 1 time step
                    
                    
                    vector<wxString> new_id_vec;
                    for (int ii=0; ii<n; ii++) {
                        if (undefs[ii % n_obs])
                            continue;
                        new_id_vec.push_back(id_vec[id_stack[ii]]);
                    }
                    OGRColumn* id_col = new OGRColumnString(id_field, 50, 0, n);
                    id_col->UpdateData(new_id_vec);
                    mem_table_int->AddOGRColumn(id_col);
                    using_default_id = false;
                }
            }
        }
    }
    
    if (!period_stack.empty()) {
        n = period_stack.size();
        if (mem_table_int == NULL) {
            mem_table_int = new OGRTable(n);
        }
        OGRColumn* period_col = new OGRColumnString("PERIOD", 18, 0, n);
        period_col->UpdateData(period_stack);
        mem_table_int->AddOGRColumn(period_col);
    }
    
    if (!var_stack_array.empty()) {
        for (size_t i=0; i<var_stack_array.size(); i++) {
            wxString col_name(var_man.GetName(i));
            int n = var_stack_array[i].size();
            if (mem_table_int == NULL) {
                mem_table_int = new OGRTable(n);
            }
            int col_idx = table_int->FindColId(col_name);
            GdaConst::FieldType f_type = table_int->GetColType(col_idx, 0);
            OGRColumn* var_col;
            if (f_type == GdaConst::long64_type) {
                var_col = new OGRColumnInteger(col_name, 18, 0, n);
            } else {
                var_col = new OGRColumnDouble(col_name, 18, 9, n);
            }
            var_col->UpdateData(var_stack_array[i]);
            mem_table_int->AddOGRColumn(var_col);
        }
    }
    
    if (!dummy_time_stack.empty()) {
        n = dummy_time_stack.size();
        if (mem_table_int == NULL) {
            mem_table_int = new OGRTable(n);
        }
        OGRColumn* time_col = new OGRColumnInteger("TIME", 18, 0, n);
        time_col->UpdateData(dummy_time_stack);
        mem_table_int->AddOGRColumn(time_col);
    }
    
    if (!dummy_select_stack.empty()) {
        n = dummy_select_stack.size();
        if (mem_table_int == NULL) {
            mem_table_int = new OGRTable(n);
        }
        OGRColumn* select_col = new OGRColumnInteger("SPACE", 18, 0, n);
        select_col->UpdateData(dummy_select_stack);
        mem_table_int->AddOGRColumn(select_col);
    }
    
    if (!interaction_stack.empty()) {
        n = interaction_stack.size();
        if (mem_table_int == NULL) {
            mem_table_int = new OGRTable(n);
        }
        OGRColumn* interact_col = new OGRColumnInteger("INTERACT", 18, 0, n);
        interact_col->UpdateData(interaction_stack);
        mem_table_int->AddOGRColumn(interact_col);
    }

    
    if (save_did) {
        if (m_yhat1 != 0 && m_resid1 != 0) {
            std::vector<double> yhat;
            std::vector<double> resid;
            for (int m=0; m<n; m++) {
                yhat.push_back(m_yhat1[m]);
                resid.push_back(m_resid1[m]);
            }
            if (mem_table_int == NULL) {
                mem_table_int = new OGRTable(n);
            }
            OGRColumn* pred_col = new OGRColumnDouble("OLS_PREDIC", 18, 9, n);
            pred_col->UpdateData(yhat);
            mem_table_int->AddOGRColumn(pred_col);
            OGRColumn* resi_col = new OGRColumnDouble("OLS_RESIDU", 18, 9, n);
            resi_col->UpdateData(resid);
            mem_table_int->AddOGRColumn(resi_col);
            
        }
    }
    
    // export
    ExportDataDlg dlg(this, (TableInterface*)mem_table_int);
    if (dlg.ShowModal() == wxID_OK) {
        if (save_weights) {
            wxString ds_name = dlg.GetDatasourceName();
            wxFileName wx_fn(ds_name);
            
            // save weights
            // Get default GalWeight*
            // change to space-time weights
            WeightsManInterface* wmi = NULL;
            if (project && project->GetWManInt()) {
                wmi = project->GetWManInt();
                boost::uuids::uuid default_wid = wmi->GetDefault();
                if (!default_wid.is_nil()) {
                    GeoDaWeight* w = wmi->GetWeights(default_wid);
                    if (w->weight_type == GeoDaWeight::gal_type) {
                        wx_fn.SetExt("gal");
                    } else if (w->weight_type == GeoDaWeight::gwt_type) {
                        wx_fn.SetExt("gwt");
                    }
                    wxString ofn(wx_fn.GetFullPath());
                    w->SaveDIDWeights(project, n_obs, newids, id_stack, ofn);
                }
            }
        }
    }
    
    // clean memory
    delete mem_table_int;

}
void LineChartFrame::OnSaveDummyTable(wxCommandEvent& event)
{
    wxLogMessage("Start LineChartFrame::OnSaveDummyTable");
    bool save_w = true;
    SaveDataAndResults(save_w);
}

void LineChartFrame::RunDIDTest()
{
    wxLogMessage("Run LineChartFrame::RunDIDTest");
    
    int var_cnt = var_man.GetVarsCount(); // should be 1
    int var_idx = 0;
    
    TableInterface* table_int = project->GetTableInt();
    const std::vector<bool>& hs(highlight_state->GetHighlight());
    
    // regression options
    bool m_constant_term = true;
    int RegressModel = 1; // for classic linear regression
    wxGauge* m_gauge = NULL;
    bool do_white_test = true;
	double *m_resid1, *m_yhat1;
   
    wxString m_Yname = var_man.GetName(var_idx);
    std::vector<wxString> m_Xnames;
    
    std::vector<bool>& undefs(data_map_undef[m_Yname]);
    
    int valid_n_obs = 0;
    for (std::vector<bool>::iterator it = undefs.begin();
         it != undefs.end(); ++it)
    {
        if (*it == false)
            valid_n_obs += 1;
    }
    
    m_Xnames.push_back("CONSTANT");
    
    
    // Y and X data
	wxString row_nm(var_man.GetName(var_idx));
	wxString row_title(row_nm);
	const vec_vec_dbl_type& Y(data_map[row_nm]);
    
    size_t n_ts = Y.size();

    // check selection
    if (compare_regimes || compare_r_and_t) {
        bool has_selection = false;
        for (int j=0; j<undefs.size(); j++) {
            if (undefs[j])
                continue;
            if (hs[j] == true)
                has_selection = true;
        }
        if (!has_selection) {
            wxMessageBox(_("Please first select observations in one of the other data or map views."));
            return;
        }
        
    } else if (compare_time_periods) {
        bool has_time0_def = false;
        bool has_time1_def = false;
        for (int t=0; t<n_ts; t++) {
            if (tms_subset0[t]) {
                has_time0_def = true;
            }
            if (tms_subset1[t]) {
                has_time1_def = true;
            }
        }
        if (!has_time0_def || !has_time1_def) {
            wxMessageBox(_("Please choose Periods first."));
            return;
        }
    }
    
    // start regression
    int nX = 0;
    double* y;
    double **x;
    DiagnosticReport* m_DR;
    
    if (compare_regimes) {
        m_Xnames.push_back("SPACE");
        nX = m_Xnames.size();
       
        int n = valid_n_obs;
        y = new double[n];
        x = new double* [2];
        
        for (int t=0; t<nX; t++)
            x[t] = new double[n];
        
        int idx = 0;
        
        int col = table_int->FindColId(m_Yname);
        
        if (!table_int->IsColTimeVariant(col)) {
            for (int j=0; j<undefs.size(); j++) {
                if (undefs[j])
                    continue;
                y[idx] = Y[0][j];
                x[0][idx] = 1.0; //constant
                x[1][idx] = hs[j] == true ? 1.0 : 0.0; // DUMMY_SELECT
                idx += 1;
            }
            
        } else {
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    for (int j=0; j<undefs.size(); j++) {
                        if (undefs[j])
                            continue;
                        y[idx] = Y[t][j];
                        x[0][idx] = 1.0; //constant
                        x[1][idx] = hs[j] == true ? 1.0 : 0.0; // DUMMY_SELECT
                        idx += 1;
                    }
                }
            }
        }
       
		m_DR = new DiagnosticReport(n, nX, m_constant_term, true, RegressModel);
    	for (int i = 0; i < nX; i++) {
    		m_DR->SetXVarNames(i, m_Xnames[i]);
    	}
		m_DR->SetMeanY(ComputeMean(y, n));
		m_DR->SetSDevY(ComputeSdev(y, n));
       
        
        classicalRegression(NULL, n, y, n, x, nX, m_DR,
                            m_constant_term, true, m_gauge,
                            do_white_test);
        
		m_resid1= m_DR->GetResidual();
		printAndShowClassicalResults(row_nm, y, table_int->GetTableName(), wxEmptyString, m_DR, n, nX, do_white_test);
		m_yhat1 = m_DR->GetYHAT();
        
        wxDateTime now = wxDateTime::Now();
        logReport = ">>" + now.FormatDate() + " " + now.FormatTime() + "\nREGRESSION (DIFF-IN-DIFF, COMPARE REGIMES) \n----------\n" + logReport;
        
    } else if (compare_time_periods) {
        wxString time_var = "T" + choice_time1->GetString(choice_time1->GetSelection()) + "_" + choice_time2->GetString(choice_time2->GetSelection());
        m_Xnames.push_back(time_var);
        nX = m_Xnames.size();
        
        int n1 = 0, n2 = 0;
		for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset0[t]) {
                n1 += valid_n_obs;
            }
        }
        if (n1 == 0) {
            wxMessageBox(_("Please choose Period 1 first."));
            return;
        }
		for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset1[t]) {
                n2 += valid_n_obs;
            }
        }
        if (n2 == 0) {
            wxMessageBox(_("Please choose Period 2 first."));
            return;
        }
        
        int n = n1 + n2;
        
        y = new double[n];
        x = new double* [2];
        for (int t=0; t<nX; t++) {
            x[t] = new double[n];
        }
        
        int idx = 0;
        for (int t=0; t<n_ts; t++) {
            if (tms_subset0[t] || tms_subset1[t]) {
                for (int j=0; j<undefs.size(); j++) {
                    if (undefs[j])
                        continue;
                    y[idx] = Y[t][j];
                    x[0][idx] = 1.0; //constant
                    x[1][idx] = tms_subset0[t] == true ? 0 : 1; // DUMMY_PERIOD
                    idx += 1;
                }
            }
        }
       
		m_DR = new DiagnosticReport(n, nX, m_constant_term, true, RegressModel);
    	for (int i = 0; i < nX; i++) {
    		m_DR->SetXVarNames(i, m_Xnames[i]);
    	}
		m_DR->SetMeanY(ComputeMean(y, n));
		m_DR->SetSDevY(ComputeSdev(y, n));
       
        
        classicalRegression(NULL, n, y, n, x, nX, m_DR,
                            m_constant_term, true, m_gauge,
                            do_white_test);
        
		m_resid1= m_DR->GetResidual();
		printAndShowClassicalResults(row_nm, y, table_int->GetTableName(), wxEmptyString, m_DR, n, nX, do_white_test);
		m_yhat1 = m_DR->GetYHAT();
        
        wxDateTime now = wxDateTime::Now();
        logReport = ">>" + now.FormatDate() + " " + now.FormatTime() + "\nREGRESSION (DIFF-IN-DIFF, COMPARE TIME PERIOD) \n----------\n" + logReport;
        
    } else if (compare_r_and_t) {
        m_Xnames.push_back("SPACE");
        wxString time_var = "T" + choice_time1->GetString(choice_time1->GetSelection()) + "_" + choice_time2->GetString(choice_time2->GetSelection());
        m_Xnames.push_back(time_var);
        m_Xnames.push_back("INTERACT");
        nX = m_Xnames.size();
        
        int n1 = 0, n2 = 0;
		for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset0[t]) {
                n1 += valid_n_obs;
            }
        }
        if (n1 == 0) {
            wxMessageBox(_("Please choose Period 1 first."));
            return;
        }
		for (size_t t=0; t<n_ts; ++t) {
            if (tms_subset1[t]) {
                n2 += valid_n_obs;
            }
        }
        if (n2 == 0) {
            wxMessageBox(_("Please choose Period 2 first."));
            return;
        }
        
        int n = n1 + n2;
        y = new double[n];
        x = new double* [nX];
        for (int t=0; t<nX; t++) {
            x[t] = new double[n];
        }
        
        int idx = 0;
        
        for (int t=0; t<n_ts; t++) {
            if (tms_subset0[t] || tms_subset1[t]) {
                for (int j=0; j<undefs.size(); j++) {
                    if (undefs[j])
                        continue;
                    y[idx] = Y[t][j];
                    x[0][idx] = 1.0; //constant
                    x[1][idx] = hs[j] == true ? 1.0 : 0.0; // DUMMY_SELECT
                    x[2][idx] = tms_subset0[t] == true ? 0 : 1; // DUMMY_PERIOD
                    x[3][idx] = x[1][idx] * x[2][idx];
                    idx += 1;
                }
            }
        }
       
		m_DR = new DiagnosticReport(n, nX, m_constant_term, true, RegressModel);
    	for (int i = 0; i < nX; i++) {
    		m_DR->SetXVarNames(i, m_Xnames[i]);
    	}
		m_DR->SetMeanY(ComputeMean(y, n));
		m_DR->SetSDevY(ComputeSdev(y, n));
        
        classicalRegression(NULL, n, y, n, x, nX, m_DR,
                            m_constant_term, true, m_gauge,
                            do_white_test);
        
        m_resid1= m_DR->GetResidual();
        printAndShowClassicalResults(row_nm, y, table_int->GetTableName(), wxEmptyString, m_DR, n, nX, do_white_test);
        m_yhat1 = m_DR->GetYHAT();
        

        
        wxDateTime now = wxDateTime::Now();
        logReport = ">>" + now.FormatDate() + " " + now.FormatTime() + "\nREGRESSION (DIFF-IN-DIFF, COMPARE REGIMES AND TIME PERIOD) \n----------\n" + logReport;
    }
    
   
    
    // display regression in dialog
    if (regReportDlg == 0) {
        regReportDlg = new RegressionReportDlg(this, logReport, wxID_ANY, "Diff-in-Diff Regression Report");
        regReportDlg->Connect(wxEVT_DESTROY, wxWindowDestroyEventHandler(LineChartFrame::OnReportClose), NULL, this);
        
        
    } else {
        regReportDlg->AddNewReport(logReport);
    }
    regReportDlg->Show(true);
    regReportDlg->m_textbox->SetSelection(0, 0);
    
    if (chk_save_did && chk_save_did->IsChecked()) {
        wxMessageDialog saveDlg(this, _("Do you want to save the results of Diff-in-Diff test?\n\nNote: the results can only be saved into an external data file, due to the change of cross-sectional observations in a space-time context."), _("Save Diff-in-Diff Test Results"), wxYES_NO | wxICON_QUESTION);
        if (saveDlg.ShowModal() == wxID_YES) {
            bool save_w = false;
            bool save_did = true;
            SaveDataAndResults(save_w, save_did, m_yhat1, m_resid1);
        }
    }
    
    delete[] y;
    for (int t=0; t<nX; t++) delete[] x[t];
    
    m_DR->release_Var();
    delete m_DR;
}

void LineChartFrame::OnReportClose(wxWindowDestroyEvent& event)
{
    regReportDlg = 0;
}

void LineChartFrame::OnCompareRegimes(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnCompareRegimes()");
	if (compare_regimes) return;
	compare_regimes = true;
	compare_time_periods = false;
	compare_r_and_t = false;
	selection_period = 0;
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
	UpdateOptionMenuItems();
}

void LineChartFrame::OnCompareTimePeriods(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnCompareTimePeriods()");
	if (compare_time_periods) return;
	compare_regimes = false;
	compare_time_periods = true;
	compare_r_and_t = false;
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
	UpdateOptionMenuItems();
}

void LineChartFrame::OnCompareRegAndTmPer(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnCompareRegAndTmPer()");
	if (compare_r_and_t) return;
	compare_regimes = false;
	compare_time_periods = false;
	compare_r_and_t = true;
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
	UpdateOptionMenuItems();
}

void LineChartFrame::OnSelectPeriod0(wxCommandEvent& event)
{
	selection_period = 0;
}

void LineChartFrame::OnSelectPeriod1(wxCommandEvent& event)
{
	selection_period = 1;
}

void LineChartFrame::OnDisplayStatistics(wxCommandEvent& event)
{
    wxLogMessage("In LineChartFrame::OnDisplayStatistics()");
	display_stats = !display_stats;
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
	UpdateOptionMenuItems();
}

void LineChartFrame::update(HLStateInt* o)
{
	if (!compare_regimes && !compare_r_and_t) return;
	const std::vector<bool>& hs(highlight_state->GetHighlight());
	for (size_t i=0, sz=lc_stats.size(); i<sz; ++i) {
		lc_stats[i]->UpdateRegimesStats(hs, has_selection, has_excluded);
		lc_stats[i]->UpdateOtherStats();
	}
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        if (use_def_y_range)
            line_charts[i]->UpdateYAxis(def_y_min, def_y_max);
        else
            line_charts[i]->UpdateYAxis();
		line_charts[i]->UpdateAll();
	}
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		UpdateStatsWinContent(i);
	}
}

/** Implementation of TableStateObserver interface */
void LineChartFrame::update(TableState* o)
{
	UpdateDataMapFromVarMan();
	const std::vector<bool>& hs(highlight_state->GetHighlight());
	for (size_t i=0, sz=lc_stats.size(); i<sz; ++i) {
		lc_stats[i]->UpdateRegimesStats(hs, has_selection, has_excluded);
		lc_stats[i]->UpdateOtherStats();
	}
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        if (use_def_y_range)
            line_charts[i]->UpdateYAxis(def_y_min, def_y_max);
        else
            line_charts[i]->UpdateYAxis();
		line_charts[i]->UpdateAll();
	}
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		UpdateStatsWinContent(i);
	}	
}

void LineChartFrame::update(VarsChooserObservable* o)
{
	UpdateDataMapFromVarMan();
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
}

void LineChartFrame::notifyOfClosing(VarsChooserObservable* o)
{
    UpdateMessageWin();
}

// need to know if a shift selection or if a click selection
// shift: don't unselect anything for current
// click: only works if just a single new selection.
// tms_sel is to indicate newly selected items, not existing.
void LineChartFrame::notifyNewSelection(const std::vector<bool>& tms_sel,
										bool shiftdown, bool pointsel)
{
	wxString s;
	size_t tms = tms_sel.size();
	s << "  new selection:";
	for (size_t t=0; t<tms; ++t) {
		if (tms_sel[t]) s << " " << t;
	} 
	
	if (compare_regimes) {
		// In compare regimes mode, we make minimal effort to
		// preserve tm_subset1.  It is assumed that selection_period == 0
		if (pointsel) {
			for (size_t t=0; t<tms; ++t) {
				if (tms_sel[t]) {
					tms_subset0[t] = !tms_subset0[t];
				} else if (!shiftdown) {
					tms_subset0[t] = false;
				}
			}
		} else {
			// normal rectangle selection
			for (size_t t=0; t<tms; ++t) {
				if (tms_sel[t]) {
					tms_subset0[t] = true;
				} else if (!shiftdown) {
					tms_subset0[t] = false;
				}
			}
		}
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset0[t]) tms_subset1[t] = false;
		}
	} else { // compare_r_and_t || compare_time_periods == true
		// similar to above, but restirct deselection ops to
		// selection points already selected
		if (pointsel) {
			for (size_t t=0; t<tms; ++t) {
				if (tms_sel[t]) {
					if (selection_period == 0) {
						tms_subset0[t] = !tms_subset0[t];
						tms_subset1[t] = false;
					} else {
						tms_subset1[t] = !tms_subset1[t];
						tms_subset0[t] = false;
					}
				} else if (!shiftdown && !
						   ((selection_period == 0 && tms_subset1[t]) ||
							(selection_period == 1 && tms_subset0[t])))
				{
					if (selection_period == 0) {
						tms_subset0[t] = false;
					} else { // selection_period == 1
						tms_subset1[t] = false;
					}
				}
			}
		} else {
			// normal rectangle selection
			for (size_t t=0; t<tms; ++t) {
				if (tms_sel[t]) {
					if (selection_period == 0) {
						tms_subset0[t] = true;
						tms_subset1[t] = false;
					} else {
						tms_subset1[t] = true;
						tms_subset0[t] = false;
					}
				} else if (!shiftdown && 
						   !((selection_period == 0 && tms_subset1[t]) ||
							 (selection_period == 1 && tms_subset0[t])))
				{
					if (selection_period == 0) {
						tms_subset0[t] = false;
					} else { // selection_period == 1
						tms_subset1[t] = false;
					}
				}
			}
		}
		
		if (selection_period == 0) {
			for (size_t t=0; t<tms; ++t) {
				if (tms_subset0[t]) tms_subset1[t] = false;
			}
		} else { // selection_period == 1
			for (size_t t=0; t<tms; ++t) {
				if (tms_subset1[t]) tms_subset0[t] = false;
			}
		}
	}
	
	for (size_t i=0, sz=lc_stats.size(); i<sz; ++i) {
		lc_stats[i]->UpdateOtherStats();
	}
	
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        if (use_def_y_range)
            line_charts[i]->UpdateYAxis(def_y_min, def_y_max);
        else
            line_charts[i]->UpdateYAxis();
		line_charts[i]->UpdateAll();
	}
	
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		UpdateStatsWinContent(i);
	}
	panel_h_szr->RecalcSizes();
}

void LineChartFrame::notifyNewHoverMsg(const wxString& msg)
{
	wxStatusBar* sb = GetStatusBar();
	if (!sb) return;
	sb->SetStatusText(msg);
}

void LineChartFrame::SetupPanelForNumVariables(int num_vars)
{
	if (!panel || !bag_szr) return;
	if (message_win) {
		message_win->Unbind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
		bool detatch_success = bag_szr->Detach(0);
		message_win->Destroy();
		message_win = 0;
	}
	if (ctrls_h_szr)
        ctrls_h_szr->Clear(true);
	if (title1_txt) {
		title1_txt->Destroy();
		title1_txt = 0;
	}
	if (title1_h_szr) {
		title1_h_szr->Clear(true);
		panel_v_szr->Remove(title1_h_szr);
		title1_h_szr = 0;
	}
	if (title2_txt) {
		title2_txt->Destroy();
		title2_txt = 0;
	}
	if (title2_h_szr) {
		title2_h_szr->Clear(true);
		panel_v_szr->Remove(title2_h_szr);
		title2_h_szr = 0;
	}
	bag_szr->Clear();
	panel_v_szr->Remove(bag_szr); // bag_szr is deleted automatically
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
		if (line_charts[i]) {
			line_charts[i]->Destroy();
		}
	}
	line_charts.clear();
	lc_stats.clear();
	size_t tms = project->GetTableInt()->GetTimeSteps();
	if (tms_subset0.size() != tms) {
		tms_subset0.resize(tms);
		tms_subset1.resize(tms);
		for (size_t t=0; t<tms; ++t) {
			tms_subset0[t] = true;
			tms_subset1[t] = false;
		}
	}
	
	if (num_vars < 1) {
		message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1));
		message_win->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
		bag_szr->Add(message_win, wxGBPosition(0,0), wxGBSpan(1,1), wxEXPAND);
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0)) bag_szr->RemoveGrowableCol(0);
		bag_szr->AddGrowableCol(0, 1);
		if (bag_szr->IsRowGrowable(0)) bag_szr->RemoveGrowableRow(0);
		bag_szr->AddGrowableRow(0, 1);
		
	} else {
		for (int row=0; row<num_vars; ++row) {
			wxString row_nm(var_man.GetName(row));
			wxString row_title(row_nm);
			const vec_vec_dbl_type& X(data_map[row_nm]);
            const std::vector<bool>& X_undef(data_map_undef[row_nm]);
            const std::vector<bool>& hl(highlight_state->GetHighlight());
           
			LineChartStats* lcs_p = 0;
			if (X.size() > 1) {
                lcs_p = new LineChartStats(X, X_undef, row_title,
                                           tms_subset0,
                                           tms_subset1,
                                           compare_regimes,
                                           compare_time_periods,
                                           compare_r_and_t);
			} else {
                lcs_p = new LineChartStats(X, X_undef, row_title,
                                           tms_subset0_tm_inv,
                                           tms_subset1_tm_inv,
                                           compare_regimes,
                                           compare_time_periods,
                                           compare_r_and_t);
			}
			lcs_p->UpdateNonRegimesNonTmsStats();
			lcs_p->UpdateRegimesStats(hl,
                                      has_selection,
                                      has_excluded);
			lcs_p->UpdateOtherStats();
            if (lcs_p->compare_r_and_t) {
                int group1 = choice_group1->GetSelection();
                int group2 = choice_group2->GetSelection();
                int time1 = choice_time1->GetSelection();
                int time2 = choice_time2->GetSelection();
                lcs_p->Y_sel_tm0_avg_valid = false;
                lcs_p->Y_excl_tm0_avg_valid = false;
                lcs_p->Y_sel_tm1_avg_valid = false;
                lcs_p->Y_excl_tm1_avg_valid = false;
                
                lcs_p->Y_sel_tm0_avg_valid = group1 == 0;
                lcs_p->Y_excl_tm0_avg_valid = group1 == 1;
                
                lcs_p->Y_sel_tm1_avg_valid = group2 == 0;
                lcs_p->Y_excl_tm1_avg_valid = group2 == 1;
            }
			lc_stats.push_back(lcs_p);
			
			LineChartCanvas* canvas = 0;
			canvas = new LineChartCanvas(panel, this, project, *lcs_p, this);
            if (use_def_y_range) {
                canvas->UpdateYAxis(def_y_min, def_y_max);
                canvas->UpdateAll();
            }
            if (def_y_precision !=1) {
                canvas->UpdateYAxisPrecision(def_y_precision);
                canvas->UpdateAll();
            }
			bag_szr->Add(canvas, wxGBPosition(row, 0), wxGBSpan(1,1), wxEXPAND);
			line_charts.push_back(canvas);
		}
		int col0_proportion = 1;
		
        int col1_proportion = 1;
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0))
            bag_szr->RemoveGrowableCol(0);
		bag_szr->AddGrowableCol(0, col0_proportion);
		
		for (int i=0; i<num_vars; ++i) {
			if (bag_szr->IsRowGrowable(i))
                bag_szr->RemoveGrowableRow(i);
			bag_szr->AddGrowableRow(i, 1);
		}
	}
    //panel_v_szr->AddSpacer(5);
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	panel_h_szr->RecalcSizes();
   
    UpdateStatsWinContent(0);
	Refresh();
}

void LineChartFrame::UpdateMessageWin()
{
	if (!message_win) return;
	wxString s;
	message_win->SetPage(s);
}

void LineChartFrame::UpdateTitleText()
{
	wxString frame_title("Averages Chart");
    
    int sel = choice_variable->GetSelection();
    wxString col_name = variable_names[sel];
    int col = project->GetTableInt()->FindColId(col_name);
    
    if (sel >=0 ) {
        frame_title << " - " << choice_variable->GetString(sel);
        
        int group1 = choice_group1->GetSelection();
        int group2 = choice_group2->GetSelection();
        int time1 = choice_time1->GetSelection();
        int time2 = choice_time2->GetSelection();
        
        if (time1 == time2) {
            
            if (project->GetTableInt()->IsColTimeVariant(col) &&
                time1 >= 0) {
                
                frame_title << " - " << choice_group1->GetString(group1) << " vs " << choice_group2->GetString(group2) << " " << choice_time1->GetString(time1);
            } else {
                frame_title << " - " << choice_group1->GetString(group1) << " vs " << choice_group2->GetString(group2);
            }
            
        } else {
            if (project->GetTableInt()->IsColTimeVariant(col) && time1 >= 0 && time2 >=0) {
                frame_title << " - " << choice_group1->GetString(group1) << " " << choice_time1->GetString(time1) << " vs " << choice_time2->GetString(time2);
            } else {
                frame_title << " - " << choice_group1->GetString(group1);
            }
        }
    }
    
	SetTitle(frame_title);
    
	Refresh();
}

void LineChartFrame::UpdateTitleWin()
{
}

/** Adds/removes variables from data_map according to variables present
 in var_man. */
void LineChartFrame::UpdateDataMapFromVarMan()
{
	using namespace std;
	// get set of var_man names
	set<wxString> vm_nms;
	for (int i=0; i<var_man.GetVarsCount(); ++i) {
		vm_nms.insert(var_man.GetName(i));
	}
	
	// remove items from data_map not in vm_nms
	set<wxString> to_remove;
	for (data_map_type::iterator i=data_map.begin(); i!=data_map.end(); ++i) {
		wxString nm(i->first);
		if (vm_nms.find(nm) != vm_nms.end())
            continue;
		to_remove.insert(nm);
	}
	
	for (set<wxString>::iterator i=to_remove.begin(); i!=to_remove.end(); ++i) {
		data_map.erase(*i);
        data_map_undef.erase(*i);
	}
	
	// add items to data_map that are in vm_nms, but not currently in data_map
	set<wxString> to_add;
	for (set<wxString>::iterator i=vm_nms.begin(); i!=vm_nms.end(); ++i) {
		wxString nm(*i);
        if (data_map.find(nm) != data_map.end()) {
            continue;
        }
		to_add.insert(nm);
	}
	
	TableInterface* table_int = project->GetTableInt();
    int num_obs = table_int->GetNumberRows();
    
	for (set<wxString>::iterator i=to_add.begin(); i!=to_add.end(); ++i) {
		wxString nm = (*i);
		int c_id = table_int->FindColId(nm);
		if (c_id < 0) {
			continue;
		}
		int tms = table_int->GetColTimeSteps(c_id);
		pair<wxString, vec_vec_dbl_type> p(nm, vec_vec_dbl_type(tms));
		data_map.insert(p);
		data_map_type::iterator e = data_map.find(nm);
		if (e == data_map.end()) {
			continue;
		}
        std::vector<bool> undef_all(num_obs, false);
		for (int t=0; t<tms; ++t) {
            std::vector<bool> undefs(num_obs);
			table_int->GetColData(c_id, t, e->second[t]);
            table_int->GetColUndefined(c_id, t, undefs);
            for (int ii=0; ii<num_obs; ii++) {
                undef_all[ii] = undef_all[ii] || undefs[ii];
            }
		}
        data_map_undef.insert(std::make_pair(nm, undef_all));
	}
}

wxString LineChartFrame::GetHelpHtml()
{
	return "";
}

void LineChartFrame::UpdateStatsWinContent(int var)
{
	if (var < 0 || var >= stats_wins.size() || !stats_wins[var]) return;
	if (var >= lc_stats.size() || !lc_stats[var]) return;
	wxHtmlWindow* stats_win = stats_wins[var];
	const LineChartStats& lcs(*lc_stats[var]);
	
	bool cmp_r = compare_regimes;
	bool cmp_t = compare_time_periods;
	bool cmp_r_t = compare_r_and_t;
	
	bool single_time = lcs.tms_subset0.size() == 1;
	bool single_sample = compare_time_periods && single_time;
	
	if (cmp_r_t && single_time) {
		cmp_r = true;
		cmp_r_t = false;
	}
	
	//stats_win->EnableContextMenu(false);
	
	wxString sel_clr;
	{
		wxColour c;
		c = GdaColorUtils::ChangeBrightness(GdaConst::ln_cht_clr_sel_dark, 100);
		sel_clr << "\"" << GdaColorUtils::ToHexColorStr(c) << "\"";
	}
	wxString exl_clr;
	{
		wxColour c;
		c = GdaColorUtils::ChangeBrightness(GdaConst::ln_cht_clr_exl_dark, 100);
		exl_clr << "\"" << GdaColorUtils::ToHexColorStr(c) << "\"";
	}
	wxString tm1_clr;
	{
		wxColour c;
		c = GdaColorUtils::ChangeBrightness(GdaConst::ln_cht_clr_tm1_light, 135);
		tm1_clr << "\"" << GdaColorUtils::ToHexColorStr(c) << "\"";
	}
	wxString tm2_clr;
	{
		wxColour c;
		c = GdaColorUtils::ChangeBrightness(GdaConst::ln_cht_clr_tm2_light, 135);
		tm2_clr << "\"" << GdaColorUtils::ToHexColorStr(c) << "\"";
	}
	
    stringstream _s;
    _s << std::fixed << std::setprecision(2);
	wxString td_s0_mean;
    
	if (lcs.s0.mean_v) {
        _s << lcs.s0.mean;
		if (single_sample) {
			td_s0_mean << "<td align=\"center\">" << _s.str() << "</td>";
		} else {
			td_s0_mean << "<td";
			if (cmp_r_t || cmp_t) td_s0_mean << " bgcolor="<<tm1_clr;
			td_s0_mean << " align=\"center\">";
			if (cmp_r || cmp_r_t) td_s0_mean << "<font color="<<sel_clr<<">";
            td_s0_mean << _s.str();
			if (cmp_r || cmp_r_t) td_s0_mean << "</font>";
			td_s0_mean << "</td>";
		}
	} else {
		td_s0_mean << "<td></td>";
	}
	
	wxString td_s1_mean;
	if (!single_sample) {
        _s.str("");
        _s << lcs.s1.mean;
		if (!lcs.s1.mean_v) {
			td_s1_mean << "<td></td>";
		} else {
			td_s1_mean << "<td";
			if (cmp_r_t) td_s1_mean<< " bgcolor="<<tm1_clr;
			if (cmp_t) td_s1_mean<< " bgcolor="<<tm2_clr;
			td_s1_mean << " align=\"center\">";
			if (cmp_r || cmp_r_t) td_s1_mean << "<font color="<<exl_clr<<">";
            td_s1_mean << _s.str();
			if (cmp_r || cmp_r_t) td_s1_mean << "</font>";
			td_s1_mean << "</td>";
		}
	}
	
	wxString td_s2_mean;
	if (!single_sample && cmp_r_t) {
        _s.str("");
        _s << lcs.s2.mean;
		if (!lcs.s2.mean_v) {
			td_s2_mean << "<td></td>";
		} else {
			td_s2_mean << "<td bgcolor="<<tm2_clr<<" align=\"center\">";
			td_s2_mean << "<font color="<<sel_clr<<">";
            td_s2_mean << _s.str();
			td_s2_mean << "</font></td>";
		}
	}
	
	wxString td_s3_mean;
	if (!single_sample && cmp_r_t) {
        _s.str("");
        _s << lcs.s3.mean;
		if (!lcs.s3.mean_v) {
			td_s3_mean << "<td></td>";
		} else {
			td_s3_mean << "<td bgcolor="<<tm2_clr<<" align=\"center\">";
			td_s3_mean << "<font color="<<exl_clr<<">";
            td_s3_mean << _s.str();
			td_s3_mean << "</font></td>";
		}
	}

	wxString sd0;
    _s.str("");
    if (lcs.s0.var_v) {
        _s << lcs.s0.sd;
        sd0 << _s.str();
    }
	wxString sd1;
    _s.str("");
    if (lcs.s1.var_v) {
        _s << lcs.s1.sd;
        sd1 << _s.str();
    }
	wxString sd2;
    _s.str("");
    if (lcs.s2.var_v) {
        _s << lcs.s2.sd;
        sd2 << _s.str();
    }
	wxString sd3;
    _s.str("");
    if (lcs.s3.var_v) {
        _s << lcs.s3.sd;
        sd3 << _s.str();
    }
	
	wxString s;
	s<< "<!DOCTYPE html>\n";
	s<< "<html lang=\"en\">\n";
	s<< "<head></head>\n";
	s<< "<body>\n";
	s<< "<center>\n";
	s<< "<font face=\"verdana,arial,sans-serif\" color=\"black\" size=\"2\">";
	
	s<< "<table width=100% rules=\"rows\" >";
	s<< "<tr bgcolor=\"#CCCCCC\" >";
	s<< "<th width=130 align=\"center\">Group</th>";
	s<< "<th align=\"center\">&nbsp;Obs.&nbsp;</th>";
	s<< "<th align=\"center\">&nbsp;Mean&nbsp;</th>";
	s<< "<th align=\"center\">&nbsp;S.D.&nbsp;</th>";
	s<< "</tr>";
   
    if (single_sample) {
    	s<< "<tr>";
    	s<< "<td align=\"left\"></td>";
    	s<< "<td align=\"right\">" << lcs.s0.sz_i << "</td>";
    	s<< td_s0_mean;
    	s<< "<td align=\"right\">" << sd0 << "</td>";
    	s<< "</tr>";
        
    } else {
		s<< "<tr>";
        if (cmp_r)
            s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << ">Selected</font></td>";
        if (cmp_t)
            s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << ">Period 1</font></td>";
        
        if (cmp_r || cmp_t) {
            if (cmp_r)
                s<< "<td align=\"right\">" << lcs.sel_sz_i << "</td>";
            if (cmp_t)
                s<< "<td align=\"right\">" << lcs.obs_sz_i << "</td>";
    		s<< td_s0_mean;
    		s<< "<td align=\"right\">" << sd0 << "</td>";
    		s<< "</tr>";
    		s<< "<tr>";
        }
        
        if (cmp_r_t) {
            if (choice_group1->GetSelection() == 0) {
                s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << ">Selected</font>/<font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << ">Period 1</font></td>";
        		s<< "<td align=\"right\">" << lcs.sel_sz_i << "</td>";
        		s<< td_s0_mean;
        		s<< "<td align=\"right\">" << sd0 << "</td>";
        		s<< "</tr>";
        		s<< "<tr>";
            } else {
                s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << ">Unselected</font>/<font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << ">Period 1</font></td>";
            
        		s<< "<td align=\"right\">" << lcs.excl_sz_i << "</td>";
        		s<< td_s1_mean;
        		s<< "<td align=\"right\">" << sd1 << "</td>";
        		s<< "</tr>";
        		s<< "<tr>";
            }
        }
        
        
        if (cmp_r)
            s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << ">Unselected</font></td>";
        if (cmp_t)
            s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << ">Period 2</font></td>";
        
        if (cmp_r || cmp_t) {
            if (cmp_r)
                s<< "<td align=\"right\">" << lcs.excl_sz_i << "</td>";
            if (cmp_t)
                s<< "<td align=\"right\">" << lcs.obs_sz_i << "</td>";
    		s<< td_s1_mean;
    		s<< "<td align=\"right\">" << sd1 << "</td>";
    		s<< "</tr>";
        }
        
        if (cmp_r_t) {
            if (choice_group2->GetSelection() == 0) {
                s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << ">Selected</font>/<font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << ">Period 2</font></td>";
        		s<< "<td align=\"right\">" << lcs.sel_sz_i  << "</td>";
        		s<< td_s2_mean;
        		s<< "<td align=\"right\">" << sd2 << "</td>";
        		s<< "</tr>";
                
            } else {
        		s<< "<td align=\"left\"><font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << ">Unselected</font>/<font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << ">Period 2</font></td>";
        		s<< "<td align=\"right\">" << lcs.excl_sz_i << "</td>";
        		s<< td_s3_mean;
        		s<< "<td align=\"right\">" << sd3 << "</td>";
        		s<< "</tr>";
            }
        }
       
	}
   
	s<< "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td></tr>";
	s<< "</table>\n";
    
	if (lcs.test_stat_valid && !cmp_r_t) {
        s<< "<br/>Do Means Differ? (ANOVA)<br/><br/>";
		s<< "<table>";
		s<< "<tr>";
		s<< "<td bgcolor=\"#CCCCCC\" align=\"center\">D.F.&nbsp;</td>";
        stringstream _s;
        if (choice_groups->GetSelection() == 0)
            _s << (int)lcs.deg_free -1;
        else
            _s << (int)(lcs.deg_free * 2);
            
        _s << std::fixed << std::setprecision(2);
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td bgcolor=\"#CCCCCC\" align=\"right\">F-val&nbsp;</td>";
        _s.str("");
        _s << lcs.test_stat;
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td bgcolor=\"#CCCCCC\" align=\"right\">p-val&nbsp;</td>";
		double pval = lcs.p_val;
        _s.str("");
        _s << std::setprecision(3) << pval;
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "</table>\n";
	}
	
	if (cmp_r_t) {
        s<< "<br/>Do Means Differ? (ANOVA)<br/><br/>";
        s<< "<table>";
        s<< "<tr>";
        s<< "<td bgcolor=\"#CCCCCC\" align=\"center\">D.F.&nbsp;</td>";
        stringstream _s;
        if (choice_group1->GetSelection() == 0) {
            _s << (int)lcs.deg_free_c[1] * 2;
        } else {
            _s << (int)lcs.deg_free_c[4] * 2;
        }
        _s << std::fixed << std::setprecision(2);
        s<< "<td align=\"center\">" << _s.str() << "</td>";
        s<< "</tr>";
        s<< "<tr>";
        s<< "<td bgcolor=\"#CCCCCC\" align=\"right\">F-val&nbsp;</td>";
        _s.str("");
        if (choice_group1->GetSelection() == 0) {
            _s << lcs.test_stat_c[1];
        } else {
            _s << lcs.test_stat_c[4];
        }
        s<< "<td align=\"center\">" << _s.str() << "</td>";
        s<< "</tr>";
        s<< "<tr>";
        s<< "<td bgcolor=\"#CCCCCC\" align=\"right\">p-val&nbsp;</td>";
        double pval = 0;
        if (choice_group1->GetSelection() == 0) {
            _s << lcs.p_val_c[1];
        } else {
            _s << lcs.p_val_c[4];
        }
        _s.str("");
        _s << std::setprecision(3) << pval;
        s<< "<td align=\"center\">" << _s.str() << "</td>";
        s<< "</tr>";
        s<< "</table>\n";
	}
	
	s<< "</center>\n";
	s<< "</font>";
	
	s<< "</body>\n";
	s<< "</html>\n";
	
	stats_win->SetPage(s);
}

void LineChartFrame::printAndShowClassicalResults(const wxString& _yName, double* y,
                                                  const wxString& datasetname,
                                                 const wxString& wname,
                                                 DiagnosticReport *r,
                                                 int Obs, int nX,
                                                 bool do_white_test)
{
    wxString yName(_yName);
    wxString time1, time2;
    for(size_t i=0; i < tms_subset0.size(); i++) {
        if (tms_subset0[i] == true) {
            time1 = project->GetTableInt()->GetTimeString(i);
        }
        if (tms_subset1[i] == true) {
            time2 = project->GetTableInt()->GetTimeString(i);
        }
    }
    if (!time1.IsEmpty() && !time2.IsEmpty() && time1 != time2) {
        yName = wxString::Format("%s (%s,%s)", yName, time1, time2);
    } else if (!time1.IsEmpty()) {
        yName = wxString::Format("%s (%s)", yName, time1);
    } else if (!time2.IsEmpty()) {
        yName = wxString::Format("%s (%s)", yName, time2);
    }
    
    
    wxString f; // temporary formatting string
    wxString slog;
    
    logReport = wxEmptyString; // reset log report
    int cnt = 0;
    
    slog << "SUMMARY OF OUTPUT: ORDINARY LEAST SQUARES ESTIMATION\n"; cnt++;
    slog << "Data Set            :  " << datasetname << "\n"; cnt++;
    slog << "Dependent Variable  :";
    
    if (yName.length() > 12 )
        slog << "  " << GenUtils::Pad(yName, 12) << "\n";
    else
        slog << GenUtils::Pad(yName, 12) <<  "  ";
    
    slog << "Number of Observations:" << wxString::Format("%5d\n",Obs); cnt++;
    f = "Mean dependent var  :%12.6g  Number of Variables   :%5d\n";
    slog << wxString::Format(f, r->GetMeanY(), nX); cnt++;
    f = "S.D. dependent var  :%12.6g  Degrees of Freedom    :%5d \n";
    slog << wxString::Format(f, r->GetSDevY(), Obs-nX); cnt++;
    slog << "\n"; cnt++;
    
    f = "R-squared           :%12.6f  F-statistic           :%12.6g\n"; cnt++;
    slog << wxString::Format(f, r->GetR2(), r->GetFtest());
    f = "Adjusted R-squared  :%12.6f  Prob(F-statistic)     :%12.6g\n"; cnt++;
    slog << wxString::Format(f, r->GetR2_adjust(), r->GetFtestProb());
    f = "Sum squared residual:%12.6g  Log likelihood        :%12.6g\n"; cnt++;
    slog << wxString::Format(f, r->GetRSS() ,r->GetLIK());
    f = "Sigma-square        :%12.6g  Akaike info criterion :%12.6g\n"; cnt++;
    slog << wxString::Format(f, r->GetSIQ_SQ(), r->GetAIC());
    f = "S.E. of regression  :%12.6g  Schwarz criterion     :%12.6g\n"; cnt++;
    slog << wxString::Format(f, sqrt(r->GetSIQ_SQ()), r->GetOLS_SC());
    f = "Sigma-square ML     :%12.6g\n"; cnt++;
    slog << wxString::Format(f, r->GetSIQ_SQLM());
    f = "S.E of regression ML:%12.6g\n\n"; cnt++; cnt++;
    slog << wxString::Format(f, sqrt(r->GetSIQ_SQLM()));
    
    slog << "--------------------------------------";
    slog << "---------------------------------------\n"; cnt++;
    slog << "       Variable      Coefficient      ";
    slog << "Std.Error    t-Statistic   Probability\n"; cnt++;
    slog << "--------------------------------------";
    slog << "---------------------------------------\n"; cnt++;
    
    for (int i=0; i<nX; i++) {
        slog << GenUtils::PadTrim(r->GetXVarName(i), 18);
        slog << wxString::Format("  %12.6g   %12.6g   %12.6g   %9.5f\n",
                                 r->GetCoefficient(i), r->GetStdError(i),
                                 r->GetZValue(i), r->GetProbability(i)); cnt++;
    }
    slog << "----------------------------------------";
    slog << "-------------------------------------\n\n"; cnt++; cnt++;
    
    slog << "============================== END OF REPORT";
    slog <<  " ================================\n\n"; cnt++; cnt++;
    
    slog << "\n\n"; cnt++; cnt++;
    logReport << slog;
}
