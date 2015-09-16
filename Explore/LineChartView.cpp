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
#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include "../HighlightState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "LineChartCanvas.h"
#include "LineChartView.h"

BEGIN_EVENT_TABLE(LineChartFrame, TemplateFrame)
	EVT_ACTIVATE(LineChartFrame::OnActivate)
END_EVENT_TABLE()

LineChartFrame::LineChartFrame(wxFrame *parent, Project* project,
							   const wxString& title,
							   const wxPoint& pos,
							   const wxSize& size)
: TemplateFrame(parent, project, title, pos, size, wxDEFAULT_FRAME_STYLE),
highlight_state(project->GetHighlightState()), 
vars_chooser_frame(0), 
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
tms_subset1_tm_inv(1, false)
{
	LOG_MSG("Entering LineChartFrame::LineChartFrame");
	supports_timeline_changes = true;
	{
		std::vector<wxString> tm_strs;
		project->GetTableInt()->GetTimeStrings(tm_strs);
		var_man.ClearAndInit(tm_strs);
	}
	
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	//panel->Bind(wxEVT_MOTION, &LineChartFrame::OnMouseEvent, this); // MMLCu
	
	message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition,
								   wxSize(200,-1));
	
	message_win->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
	
	//title_win = new wxHtmlWindow(panel);
	//title_win = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr,
	//													 wxDefaultPosition, wxSize(500, 30));
	//title_win->SetMinSize(wxSize(500, 30));
	
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap	
	bag_szr->Add(message_win, wxGBPosition(0,0), wxGBSpan(1,1), wxEXPAND);
	bag_szr->SetFlexibleDirection(wxBOTH);
	bag_szr->AddGrowableCol(0, 1);
	bag_szr->AddGrowableRow(0, 1);

	// new flex grid sizer with 1 row, and 1 column
	//wxFlexGridSizer* title_fg_szr = new wxFlexGridSizer(1, 1, 0, 0);
	//title_fg_szr->SetFlexibleDirection(wxHORIZONTAL);
	//title_fg_szr->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);
	//title_fg_szr->Add(title_win, 1, wxEXPAND);
	
	panel_v_szr = new wxBoxSizer(wxVERTICAL);
	//panel_v_szr->Add(title_fg_szr, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL);
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	
	panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND | wxALL, 8);
	
	panel->SetSizer(panel_h_szr);
		
	//UpdateMessageWin();
	UpdateTitleWin();
	
	DisplayStatusBar(true);
	notifyNewHoverMsg("");
	
	highlight_state->registerObserver(this);
	Show(true);
	
	wxCommandEvent ev;
	OnShowVarsChooser(ev);
    
    Connect(XRCID("ID_SAVE_DUMMIES"), 
            wxEVT_MENU, 
            wxCommandEventHandler(LineChartFrame::OnSaveDummyVars));
	LOG_MSG("Exiting LineChartFrame::LineChartFrame");
}

LineChartFrame::~LineChartFrame()
{
	LOG_MSG("In LineChartFrame::~LineChartFrame");
	highlight_state->removeObserver(this);
	if (vars_chooser_frame) {
		vars_chooser_frame->removeObserver(this);
		vars_chooser_frame->closeAndDeleteWhenEmpty();
	}
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void LineChartFrame::OnMouseEvent(wxMouseEvent& event)
{
    if (event.RightUp()) {
        const wxPoint& pos = event.GetPosition();
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
	LOG_MSG("In LineChartFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("LineChartFrame", GetTitle());
	}
}

void LineChartFrame::MapMenus()
{
	LOG_MSG("In LineChartFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
	LoadMenu("ID_LINE_CHART_MENU_OPTIONS");	
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
		LOG_MSG("LineChartFrame::UpdateOptionMenuItems: Options "
                "menu not found");
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
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}


void LineChartFrame::OnSaveDummyVars(wxCommandEvent& event)
{
    wxString dummy_name_sel("DUM_SEL");
    wxString dummy_name_unsel("DUM_UNSEL");
    wxString dummy_name_t1("DUM_T1");
    wxString dummy_name_t2("DUM_T2");
    wxString dummy_name_sel_t1("DUM_SEL1");
    wxString dummy_name_sel_t2("DUM_SEL2");
    wxString dummy_name_unsel_t1("DUM_UNSEL1");
    wxString dummy_name_unsel_t2("DUM_UNSEL2");
    
    const std::vector<bool>& hs(highlight_state->GetHighlight());
    size_t tms = tms_subset0.size();

    if (compare_r_and_t) {
        // 4 dummy vars: 
        // reg1     reg2
        // t1   t2  t1   t2
    } else if (compare_time_periods) {
        // 2 dummy vars
        // t1   t2
        
    } 
    // compare_regimes
    {
        // 2 dummy vars
        // reg1      reg2
        bool hasSel = false;
        size_t num_obs = hs.size();
        std::vector<wxInt64> reg_dummy_sel(num_obs, 0);
        std::vector<wxInt64> reg_dummy_unsel(num_obs, 0);
        
        for (size_t i=0; i<num_obs; ++i) {
            if (hs[i] == true) {
                reg_dummy_sel[i] = 1;
                hasSel = true;
            } else 
                reg_dummy_sel[i] = 0;
            
            reg_dummy_unsel[i] = hs[i] ? 0 : 1;
        }
        
        if (hasSel) {
            TableInterface* table_int = project->GetTableInt();
            int col_pos; 
            if (!table_int->ColNameExists(dummy_name_unsel)) {
                col_pos = table_int->InsertCol(GdaConst::long64_type, 
                                               dummy_name_unsel,
                                               0);
            } else {
                col_pos = table_int->FindColId(dummy_name_unsel);
            }
            table_int->SetColData(col_pos, 0/*time*/, reg_dummy_unsel);
            
            if (!table_int->ColNameExists(dummy_name_sel)) {
                col_pos = table_int->InsertCol(GdaConst::long64_type, 
                                               dummy_name_sel,
                                               0);
            } else {
                col_pos = table_int->FindColId(dummy_name_sel);
            }
            table_int->SetColData(col_pos, 0/*time*/, reg_dummy_sel);
            //const vec_vec_dbl_type& X(data_map[row_nm]);
        }
    }
}

void LineChartFrame::OnShowVarsChooser(wxCommandEvent& event)
{
	LOG_MSG("In LineChartFrame::OnShowVarsChooser");
	if (vars_chooser_frame) {
		vars_chooser_frame->Iconize(false);
		vars_chooser_frame->Raise();
		vars_chooser_frame->SetFocus();
	} else {
		wxString title("Averages Chart Variables Add/Remove");
		vars_chooser_frame = new VarsChooserFrame(var_man, project, false, false,
												  GetHelpHtml(),
												  "Averages Chart Help",
												  title);
		vars_chooser_frame->registerObserver(this);
		vars_chooser_frame->SetSize(-1, -1, -1, 400);
	}
}

void LineChartFrame::OnCompareRegimes(wxCommandEvent& event)
{
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
	LOG_MSG("In LineChartFrame::DisplayStatistics");
	display_stats = !display_stats;
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
	UpdateOptionMenuItems();
}

void LineChartFrame::update(HLStateInt* o)
{
	LOG_MSG("In LineChartFrame::update");
	if (!compare_regimes && !compare_r_and_t) return;
	const std::vector<bool>& hs(highlight_state->GetHighlight());
	for (size_t i=0, sz=lc_stats.size(); i<sz; ++i) {
		lc_stats[i]->UpdateRegimesStats(hs);
		lc_stats[i]->UpdateOtherStats();
	}
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
		line_charts[i]->UpdateAll();
	}
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		UpdateStatsWinContent(i);
	}
}

/** Implementation of TableStateObserver interface */
void LineChartFrame::update(TableState* o)
{
	LOG_MSG("In LineChartFrame::update(TableState*)");
	UpdateDataMapFromVarMan();
	if (vars_chooser_frame) vars_chooser_frame->UpdateFromTable();
	const std::vector<bool>& hs(highlight_state->GetHighlight());
	for (size_t i=0, sz=lc_stats.size(); i<sz; ++i) {
		lc_stats[i]->UpdateRegimesStats(hs);
		lc_stats[i]->UpdateOtherStats();
	}
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
		line_charts[i]->UpdateAll();
	}
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		UpdateStatsWinContent(i);
	}	
}

void LineChartFrame::update(VarsChooserObservable* o)
{
	LOG_MSG("In LineChartFrame::update(VarsChooserObservable*)");
	UpdateDataMapFromVarMan();
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
}

void LineChartFrame::notifyOfClosing(VarsChooserObservable* o)
{
	vars_chooser_frame = 0;
    UpdateMessageWin();
}

// need to know if a shift selection or if a click selection
// shift: don't unselect anything for current
// click: only works if just a single new selection.
// tms_sel is to indicate newly selected items, not existing.
void LineChartFrame::notifyNewSelection(const std::vector<bool>& tms_sel,
										bool shiftdown, bool pointsel)
{
	LOG_MSG("LineChartFrame::notifyNewSelection");
	wxString s;
	size_t tms = tms_sel.size();
	s << "  new selection:";
	for (size_t t=0; t<tms; ++t) {
		if (tms_sel[t]) s << " " << t;
	}
	LOG(shiftdown);
	LOG(pointsel);
	LOG_MSG(s);
	
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
		line_charts[i]->UpdateAll();
	}
	
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		UpdateStatsWinContent(i);
	}
	UpdateTitleText();
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
	LOG_MSG("Entering LineChartFrame::SetupPanelForNumVariables");
	if (!panel || !bag_szr) return;
	LOG(num_vars);
	if (message_win) {
		message_win->Unbind(wxEVT_MOTION, &LineChartFrame::OnMouseEvent, this);
		bool detatch_success = bag_szr->Detach(0);
		LOG(detatch_success);
		message_win->Destroy();
		message_win = 0;
	}
	if (ctrls_h_szr) ctrls_h_szr->Clear(true);
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
			//line_charts[i]->Unbind(wxEVT_MOTION, &LineChartFrame::OnMouseEvent, this); //MMLCu
			line_charts[i]->Destroy();
		}
	}
	line_charts.clear();
	lc_stats.clear();
	for (size_t i=0, sz=stats_wins.size(); i<sz; ++i) {
		if (stats_wins[i]) {
			stats_wins[i]->Destroy();
		}
	}	
	stats_wins.clear();
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
		message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition,
									   wxSize(200,-1));
		message_win->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
        if (vars_chooser_frame) {
            UpdateMessageWin();
        }
		bag_szr->Add(message_win, wxGBPosition(0,0), wxGBSpan(1,1), wxEXPAND);
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0)) bag_szr->RemoveGrowableCol(0);
		bag_szr->AddGrowableCol(0, 1);
		if (bag_szr->IsRowGrowable(0)) bag_szr->RemoveGrowableRow(0);
		bag_szr->AddGrowableRow(0, 1);
		
	} else {
		title1_h_szr = new wxBoxSizer(wxHORIZONTAL);
		title1_txt = new wxStaticText(panel, wxID_ANY, "New Title Text",
									  wxDefaultPosition, wxDefaultSize,
									  wxALIGN_CENTER_VERTICAL);
		title1_h_szr->Add(title1_txt);
		panel_v_szr->Add(title1_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5);
		if (compare_r_and_t) {
			title2_h_szr = new wxBoxSizer(wxHORIZONTAL);
			title2_txt = new wxStaticText(panel, wxID_ANY, "New Title Text",
										  wxDefaultPosition, wxDefaultSize,
										  wxALIGN_CENTER_VERTICAL);
			title2_h_szr->Add(title2_txt);
			panel_v_szr->Add(title2_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5);
		}
		if (compare_time_periods || compare_r_and_t) {
			ctrls_h_szr = new wxBoxSizer(wxHORIZONTAL);
			wxRadioButton* rb0 = new wxRadioButton(panel, XRCID("ID_RAD_BUT_0"),
											 "Time Period 1",
											 wxDefaultPosition, wxDefaultSize,
											 wxALIGN_CENTER_VERTICAL |
											 wxRB_GROUP);
			rb0->SetValue(true);
			wxRadioButton* rb1 = new wxRadioButton(panel, XRCID("ID_RAD_BUT_1"),
											 "Time Period 2",
											 wxDefaultPosition, wxDefaultSize,
											 wxALIGN_CENTER_VERTICAL);
			rb1->SetValue(false);
			Connect(XRCID("ID_RAD_BUT_0"), wxEVT_RADIOBUTTON,
					wxCommandEventHandler(LineChartFrame::OnSelectPeriod0));
			Connect(XRCID("ID_RAD_BUT_1"), wxEVT_RADIOBUTTON,
					wxCommandEventHandler(LineChartFrame::OnSelectPeriod1));
	
			ctrls_h_szr->Add(rb0);
			ctrls_h_szr->AddSpacer(15);	
			ctrls_h_szr->Add(rb1);
			panel_v_szr->Add(ctrls_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5);
		}
		
		for (int row=0; row<num_vars; ++row) {
			wxString row_nm(var_man.GetName(row));
			wxString row_title(row_nm);
			const vec_vec_dbl_type& X(data_map[row_nm]);

			LineChartStats* lcs_p = 0;
			if (X.size() > 1) {
				lcs_p = new LineChartStats(X, row_title,
									 tms_subset0, tms_subset1, compare_regimes,
									 compare_time_periods, compare_r_and_t);
			} else {
				lcs_p = new LineChartStats(X, row_title,
										 tms_subset0_tm_inv, tms_subset1_tm_inv,
										 compare_regimes, compare_time_periods,
										 compare_r_and_t);
			}
			lcs_p->UpdateNonRegimesNonTmsStats();
			lcs_p->UpdateRegimesStats(highlight_state->GetHighlight());
			lcs_p->UpdateOtherStats();
			lc_stats.push_back(lcs_p);
			
			//wxWebView* wv = 0;
			wxHtmlWindow* wv = 0;
			if (display_stats) {
				wv = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition,
															wxSize(80, -1));
				//wv = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr,
				//										wxDefaultPosition, wxDefaultSize);
				stats_wins.push_back(wv);
				UpdateStatsWinContent(row);
			}
			LineChartCanvas* canvas = 0;
			canvas = new LineChartCanvas(panel, this, project, *lcs_p, this);
			bag_szr->Add(canvas, wxGBPosition(row, 0), wxGBSpan(1,1), wxEXPAND);
			line_charts.push_back(canvas);
			if (display_stats && wv) {
				bag_szr->Add(wv, wxGBPosition(row, 1), wxGBSpan(1,1), wxEXPAND);
			}
		}
		int col0_proportion = 1;
		if (display_stats) col0_proportion = compare_r_and_t ? 2 : 3;
		int col1_proportion = 1;
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0)) bag_szr->RemoveGrowableCol(0);
		bag_szr->AddGrowableCol(0, col0_proportion);
		if (display_stats) {
			if (bag_szr->IsColGrowable(1)) bag_szr->RemoveGrowableCol(1);
			bag_szr->AddGrowableCol(1, col1_proportion);
		}
		for (int i=0; i<num_vars; ++i) {
			if (bag_szr->IsRowGrowable(i)) bag_szr->RemoveGrowableRow(i);
			bag_szr->AddGrowableRow(i, 1);
		}
	}
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	UpdateTitleText();
	panel_h_szr->RecalcSizes();
	Refresh();
	LOG_MSG("Exiting LineChartFrame::SetupPanelForNumVariables");
}

void LineChartFrame::UpdateMessageWin()
{
	if (!message_win) return;
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "</head>";
	s << "<body>";
	s << "<br /><br /><br />";
	s << "<center><p>";
	s << "<font face=\"verdana,arial,sans-serif\" color=\"black\" size=\"5\">";
	
	int count = var_man.GetVarsCount();
	if (count == 0) {
		s << "Please right-click or use<br />";
		s << "<font color=\"blue\">Options > Add/Remove Variables<br /></font>";
		s << "to specify one or more variables.";
	} if (count > 1) {
		s << "Variables specified: <br />";
		for (int i=0; i<count; ++i) {
			s << "<font color=\"blue\">" << var_man.GetName(i) << "</font>";
			if (i+1 < count) s << "<br />";
		}
	}
	
	s << "  </font></p></center>";
	s << "</body>";
	s << "</html>";
	message_win->SetPage(s );
}

void LineChartFrame::UpdateTitleText()
{
	wxString frame_title("Averages Chart");
	if (var_man.GetVarsCount() > 0) {
		if (compare_regimes) {
			frame_title << " - Compare Regimes";
		} else if (compare_time_periods) {
			frame_title << " - Compare Time Periods";
		} else if (compare_r_and_t) {
			frame_title << " - Compare Regimes and Times";
		}
	}
	SetTitle(frame_title);
	
	if (!title1_txt) return;
	if (compare_r_and_t && !title2_txt) return;
	TableInterface* table_int = project->GetTableInt();
	
	wxString span_end("</span>");
	wxString span_clr_sel("<span foreground='");
	span_clr_sel
		<< GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << "'>";
	wxString span_clr_exl("<span foreground='");
	span_clr_exl
		<< GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << "'>";
	wxString span_clr_tm1("<span foreground='");
	span_clr_tm1 
		<< GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << "'>";
	wxString span_clr_tm2("<span foreground='");
	span_clr_tm2 
		<< GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << "'>";
	wxString selected_str;
	wxString excluded_str;
	if (compare_regimes || compare_r_and_t) {
		selected_str << span_clr_sel << "selected" << span_end;
		excluded_str << span_clr_exl << "excluded" << span_end;
	}
	wxString ln1;
	wxString ln2;
	size_t tms = tms_subset0.size();
	if (!var_man.IsAnyTimeVariant()) 
        tms = 1;
	
	wxString tm0_st;
	if (tms_subset0.size() > 1) {
		bool any_found=false;
		bool lfl=false; // looking for last
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset0[t] && !lfl) {
				if (any_found) 
                    tm0_st << ", ";
				any_found = true;
				tm0_st << table_int->GetTimeString(t);
				if (t+1 < tms && tms_subset0[t+1]) 
                    lfl = true;
			} else if (tms_subset0[t] && lfl) {
				if (t+1 == tms || !tms_subset0[t+1]) {
					tm0_st << "-" << table_int->GetTimeString(t);
					lfl = false;
				}
			}
		}
	}
	wxString tm1_st;
	if (tms_subset1.size() > 1) {
		bool any_found=false;
		bool lfl=false; // looking for last
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset1[t] && !lfl) {
				if (any_found) tm1_st << ", ";
				any_found = true;
				tm1_st << table_int->GetTimeString(t);
				if (t+1 < tms && tms_subset1[t+1]) lfl = true;
			} else if (tms_subset1[t] && lfl) {
				if (t+1 == tms || !tms_subset1[t+1]) {
					tm1_st << "-" << table_int->GetTimeString(t);
					lfl = false;
				}
			}
		}
	}
	
	if (lc_stats.size() > 0 && tms > 1) {
		if (compare_regimes || compare_r_and_t) {
			ln1 << "Sample 1: " << selected_str << " ";
			if (compare_r_and_t) ln1 << span_clr_tm1;
			ln1 << (tm0_st.IsEmpty() ? "(choose times)" : tm0_st);
			if (compare_r_and_t) ln1 << span_end;
			ln1 << "    Sample 2: " << excluded_str << " ";
			if (compare_r_and_t) ln1 << span_clr_tm1;
			ln1 << (tm0_st.IsEmpty() ? "(choose times)" : tm0_st);
			if (compare_r_and_t) ln1 << span_end;
		}
		if (compare_time_periods) {
			ln1 << "Sample 1: ";
			ln1 << span_clr_tm1;
			ln1 << (tm0_st.IsEmpty() ? "(choose times)" : tm0_st) << span_end;
			ln1 << "    Sample 2: ";
			ln1 << span_clr_tm2;
			ln1 << (tm1_st.IsEmpty() ? "(choose times)" : tm1_st) << span_end;
		}
		if (compare_r_and_t) {
			ln2 << "Sample 3: " << selected_str << " ";
			ln2 << span_clr_tm2 << (tm1_st.IsEmpty() ? "(choose times)" : tm1_st);
			ln2 << span_end;
			ln2 << "    Sample 4: " << excluded_str << " ";
			ln2 << span_clr_tm2 << (tm1_st.IsEmpty() ? "(choose times)" : tm1_st);
			ln2 << span_end;
		}
	} else {
		if (compare_regimes || compare_r_and_t) {
			ln1 << "Sample 1: " << selected_str << "    Sample 2: " << excluded_str;
		}
	}
	title1_txt->SetLabelMarkup(ln1);
	if (title2_txt && compare_r_and_t) title2_txt->SetLabelMarkup(ln2);
	Refresh();
}

void LineChartFrame::UpdateTitleWin()
{
	/*
	if (!title_win) return;
	wxString s;
	s<< "<!DOCTYPE html>";
	s<< "<html>";
	s<< "<head>";
	s<<   "<meta charset=\"utf-8\">\n";
	//s<<   "<style>\n";
  //border: 0; outline: 0;"
	//"-webkit-margin-before: 0em;"
	//"-webkit-margin-after: 0em; }";
	//s<<     "body {\n";
	//s<<       "overflow: hidden;";
	//s<<       "background-color:#E6E6FA;";
	//s<<       "font: 12px verdana,arial,sans-serif;";
	//s<<     "}\n";
	//s<<     "html, body, h1, h2, h3, p, table { margin: 0; padding: 0;}";
	//s<<     "html, body, h1 {text-align: center;}";
	//s<<   "</style>\n";
	s<< "</head>";
	s<< "<body>";
	s<< "<h3>";
	s<< "Title window is so very very long";
	s<< "</h3>";
	s<< "</body>";
	s<< "</html>";
	title_win->SetPage(s);
*/
}

/** Adds/removes variables from data_map according to variables present
 in var_man. */
void LineChartFrame::UpdateDataMapFromVarMan()
{
	LOG_MSG("Entering LineChartFrame::UpdateDataMapFromVarMan");
	using namespace std;
	// get set of var_man names
	set<wxString> vm_nms;
	for (int i=0; i<var_man.GetVarsCount(); ++i) {
		vm_nms.insert(var_man.GetName(i));
	}
	
	// remove items from data_map not in vm_nms
	set<wxString> to_remove;
	LOG_MSG("to_remove from data_map:");
	for (data_map_type::iterator i=data_map.begin(); i!=data_map.end(); ++i) {
		wxString nm(i->first);
		if (vm_nms.find(nm) != vm_nms.end()) continue;
		to_remove.insert(nm);
		LOG_MSG("  " + nm);
	}
	
	for (set<wxString>::iterator i=to_remove.begin(); i!=to_remove.end(); ++i) {
		LOG_MSG("Being removed from data_map: " + (*i));
		data_map.erase(*i);
	}
	
	// add items to data_map that are in vm_nms, but not currently in data_map
	set<wxString> to_add;
	for (set<wxString>::iterator i=vm_nms.begin(); i!=vm_nms.end(); ++i) {
		wxString nm(*i);
		if (data_map.find(nm) != data_map.end()) continue;
		to_add.insert(nm);
		LOG_MSG("Must add to data_map: " + nm);
	}
	
	TableInterface* table_int = project->GetTableInt();
	for (set<wxString>::iterator i=to_add.begin(); i!=to_add.end(); ++i) {
		wxString nm = (*i);
		LOG_MSG(nm);
		int c_id = table_int->FindColId(nm);
		if (c_id < 0) {
			LOG_MSG("Error, variable not found in table: " + nm);
			continue;
		}
		int tms = table_int->GetColTimeSteps(c_id);
		LOG(tms);
		pair<wxString, vec_vec_dbl_type> p(nm, vec_vec_dbl_type(tms));
		data_map.insert(p);
		data_map_type::iterator e = data_map.find(nm);
		if (e == data_map.end()) {
			LOG_MSG("Could not find element just inserted! " + nm);
			continue;
		}
		for (int t=0; t<tms; ++t) {
			table_int->GetColData(c_id, t, e->second[t]);
		}
	}
	
	LOG_MSG("Exiting LineChartFrame::UpdateDataMapFromVarMan");
}

wxString LineChartFrame::GetHelpHtml()
{
	return "";
}

/*
 s<< "<tr>";
 s<< "<td>" << << "</td>";
 s<< "</tr>"
 */


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
	LOG(td_s0_mean);
	
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
	LOG(td_s1_mean);
	
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
	s<< "<head>\n";
	//s<<   "<meta charset=\"utf-8\">\n";
	//s<<   "<style>\n";
	//s<<     "html, body, h1, h2, h3, p, table { margin: 0; padding: 0;}";
	//s<<     "body {\n";
	//s<<       "font: 10px sans-serif;\n";
	//s<<     "}\n";
	//s<<   "</style>\n";
	s<< "</head>\n";
    s<< "<style>td, th { border-bottom: thin short }table { border: none }</style>";
	s<< "<body>\n";
	s<< "<center>\n";
	s<< "<font face=\"verdana,arial,sans-serif\" color=\"black\" size=\"2\">";
	
	s<< "<table align=\"center\" cellspacing=\"0\" cellpadding=\"1\" style=\"border:none\">";
	s<< "<tr>";
	s<< "<td align=\"center\">Smpl.</td>";
	s<< "<td align=\"center\">&nbsp;Obs.&nbsp;</td>";
	s<< "<td align=\"center\">&nbsp;Mean&nbsp;</td>";
	s<< "<td align=\"center\">&nbsp;S.D.&nbsp;</td>";
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
            s<< "<td align=\"left\">1. sel. over time</td>";
        if (cmp_t)
            s<< "<td align=\"left\">1. 1st period</td>";
        if (cmp_r_t)
            s<< "<td align=\"left\">1. sel. 1st period</td>";
		s<< "<td align=\"right\">" << lcs.s0.sz_i << "</td>";
		s<< td_s0_mean;
		s<< "<td align=\"right\">" << sd0 << "</td>";
		s<< "</tr>";
		s<< "<tr>";
        if (cmp_r)
            s<< "<td align=\"left\">2. excl. over time</td>";
        if (cmp_t)
            s<< "<td align=\"left\">2. 2nd period</td>";
        if (cmp_r_t)
            s<< "<td align=\"left\">2. excl. 1st period</td>";
		s<< "<td align=\"right\">" << lcs.s1.sz_i << "</td>";
		s<< td_s1_mean;
		s<< "<td align=\"right\">" << sd1 << "</td>";
		s<< "</tr>";
	}
	if (cmp_r_t && !single_sample) {
		s<< "<tr>";
		s<< "<td align=\"left\">3. sel. 2nd period</td>";
		s<< "<td align=\"right\">" << lcs.s2.sz_i << "</td>";
		s<< td_s2_mean;
		s<< "<td align=\"right\">" << sd2 << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td align=\"left\">4.excl. 2nd period</td>";
		s<< "<td align=\"right\">" << lcs.s3.sz_i << "</td>";
		s<< td_s3_mean;
		s<< "<td align=\"right\">" << sd3 << "</td>";
		s<< "</tr>";
	}
	s<< "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td></tr>";
	s<< "</table>\n";

	if (lcs.test_stat_valid && !cmp_r_t) {
		s<< "<table align=\"center\" cellspacing=\"0\" cellpadding=\"1\">";
		s<< "<tr>";
		s<< "<td align=\"center\">D.F.&nbsp;</td>";
        stringstream _s;
        _s << std::fixed << std::setprecision(2) << lcs.deg_free;
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td align=\"right\">T Stat&nbsp;</td>";
        _s.str("");
        _s << lcs.test_stat;
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td align=\"right\">p-val&nbsp;</td>";
		double pval = lcs.p_val;
        _s.str("");
        _s << std::setprecision(3) << pval;
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "</table>\n";
	}
	
	if (cmp_r_t) {
		s<< "<table align=\"center\" cellspacing=\"0\" cellpadding=\"1\">";
		s<< "<tr>";
		s<< "<td align=\"center\">Smpl.&nbsp;&nbsp;<br>Comp.&nbsp;&nbsp;</td>";
		s<< "<td align=\"center\">&nbsp;D.F.&nbsp;</td>";
		s<< "<td align=\"center\">&nbsp;T Stat&nbsp;</td>";
		s<< "<td align=\"center\">&nbsp;p-val</td>";
		s<< "</tr>";
		size_t c=0;
		for (size_t i=0; i<lcs.ss_ptrs.size(); ++i) {
			for (size_t j=i+1; j<lcs.ss_ptrs.size(); ++j) {
                if ((i+1 == 1 && j+1 == 4 ) || (i+1 == 2 && j+1 == 3)) {
                    // escape unnecessary comparison result, only pre- and
                    // post- comparisons are needed. issue 168
                } else {
    				s<< "<tr>";
    				s<< "<td align=\"center\">" << i+1 <<"&nbsp;vs&nbsp;"<< j+1 << "</td>";
    				if (lcs.test_stat_valid_c[c]) {
                        stringstream _s;
                        _s << std::fixed << std::setprecision(2);
                        _s << lcs.deg_free_c[c];
                        
    					s<< "<td align=\"right\">" << _s.str() << "&nbsp;</td>";
                        
                        _s.str("");
                        _s << lcs.test_stat_c[c];
    					s<< "<td align=\"right\">" << _s.str() << "&nbsp;</td>";
                        
    					double pval = lcs.p_val_c[c];
                        _s.str("");
                        _s << std::fixed << std::setprecision(3) << pval;
    					s<< "<td align=\"right\">" << _s.str() << "</td>";
    				} else {
    					s<< "<td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td>";
    				}
    				s<< "</tr>";
                }
				++c;
			}
		}
		s<< "</table>\n";
	}
	
	s<< "</center>\n";
	s<< "</font>";
	
	s<< "</body>\n";
	s<< "</html>\n";
	
	stats_win->SetPage(s);
}

