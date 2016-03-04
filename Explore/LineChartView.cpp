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
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/gauge.h>
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
                               const std::vector<GdaVarTools::VarInfo>& v_info,
                               const std::vector<int>& col_ids,
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
tms_subset1_tm_inv(1, false),
regReportDlg(0)
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
	panel->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this); // MMLCu
	message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition, wxSize(400,-1));
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
	//OnShowVarsChooser(ev);
    {
        // this block of code is used to force 1-variable selection
        // instead of VarsChooseDlg
    	std::vector<double> min_vals;
    	std::vector<double> max_vals;
        int col_id = col_ids[0];
    	project->GetTableInt()->GetMinMaxVals(col_id, min_vals, max_vals);
        wxString name = v_info[0].name;
        int time = v_info[0].time;
    	var_man.AppendVar(name, min_vals, max_vals, time);
     
        
    	UpdateDataMapFromVarMan();
    	SetupPanelForNumVariables(var_man.GetVarsCount());
        
        
    	Refresh();
	}
    
    Connect(XRCID("ID_DID_TEST"),
            wxEVT_MENU, 
            wxCommandEventHandler(LineChartFrame::OnDIDTest));
    Connect(XRCID("ID_SAVE_DUMMY"),
            wxEVT_MENU, 
            wxCommandEventHandler(LineChartFrame::OnSaveDummyTable));
    
    Connect(XRCID("ID_ADJUST_Y_AXIS"),
            wxEVT_MENU,
            wxCommandEventHandler(LineChartFrame::OnAdjustYAxis));
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
		LOG_MSG("LineChartFrame::UpdateOptionMenuItems: Options menu not found");
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
	
        if (var_man.IsAnyTimeVariant() == false) {
            menu->Enable(XRCID("ID_COMPARE_TIME_PERIODS"), false);
            menu->Enable(XRCID("ID_COMPARE_REG_AND_TM_PER"), false);
        }
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LineChartFrame::OnAdjustYAxis(wxCommandEvent& event)
{
    
    AdjustYAxisDlg dlg(def_y_min, def_y_max, this);
    if (dlg.ShowModal () != wxID_OK) return;
    
    wxString y_min = dlg.s_min_val;
    wxString y_max = dlg.s_max_val;
    
    for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        line_charts[i]->UpdateYAxis(y_min, y_max);
        line_charts[i]->UpdateAll();
    }
    
    Refresh();
}

void LineChartFrame::OnSaveDummyTable(wxCommandEvent& event)
{
    int nTests = var_man.GetVarsCount();
    TableInterface* table_int = project->GetTableInt();
    const std::vector<bool>& hs(highlight_state->GetHighlight());
    int n_obs = project->GetNumRecords();
  
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
   
    var_stack_array.resize(nTests);
    
    for (int i=0; i<nTests; i++) {
        
		wxString row_nm(var_man.GetName(i));
		const vec_vec_dbl_type& Y(data_map[row_nm]);
        
        n_ts = Y.size();
        
        if (compare_regimes) {
            
            int n= 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n+= n_obs;
                }
            }
            if (n== 0) {
                wxMessageBox("Please choose times on the time axis first.");
                return;
            }
   
            var_stack_array[i].resize(n);
            dummy_select_stack.resize(n);
            id_stack.resize(n);
            
            int idx = 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    for (int j=0; j<n_obs; j++) {
                        var_stack_array[i][idx] = Y[t][j];
                        dummy_select_stack[idx] = hs[j] == true ? 1 : 0;
                        id_stack[idx] = j;
                        newids.push_back(idx+1);
                        idx += 1;
                    }
                }
            }
           
        } else if (compare_time_periods) {
            
            int n1 = 0, n2 = 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n1 += n_obs;
                }
            }
            if (n1 == 0) {
                wxMessageBox("Please choose Period 1 on the horizontal axis first.");
                return;
            }
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset1[t]) {
                    n2 += n_obs;
                }
            }
            if (n2 == 0) {
                wxMessageBox("Please choose Period 2 on the horizontal axis first.");
                return;
            }
            
            int n = n1 + n2;
            
            var_stack_array[i].resize(n);
            dummy_time_stack.resize(n);
            id_stack.resize(n);
            
            int idx = 0;
            
            for (int t=0; t<n_ts; t++) {
                if (tms_subset0[t] || tms_subset1[t]) {
                    for (int j=0; j<n_obs; j++) {
                        var_stack_array[i][idx] = Y[t][j];
                        dummy_time_stack[idx] = tms_subset0[t] == true ? 0 : 1;
                        id_stack[idx] = j;
                        newids.push_back(idx+1);
                        idx += 1;
                    }
                }
            }
           
        } else if (compare_r_and_t) {
            
            int n1 = 0, n2 = 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n1 += n_obs;
                }
            }
            if (n1 == 0) {
                wxMessageBox("Please choose Period 1 on the horizontal axis first.");
                return;
            }
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset1[t]) {
                    n2 += n_obs;
                }
            }
            if (n2 == 0) {
                wxMessageBox("Please choose Period 2 on the horizontal axis first.");
                return;
            }
            
            int n = n1 + n2;
            
            var_stack_array[i].resize(n);
            dummy_time_stack.resize(n);
            dummy_select_stack.resize(n);
            interaction_stack.resize(n);
            id_stack.resize(n);
            
            int idx = 0;
            for (int t=0; t<n_ts; t++) {
                if (tms_subset0[t] || tms_subset1[t]) {
                    for (int j=0; j<n_obs; j++) {
                        var_stack_array[i][idx] = Y[t][j];
                        dummy_select_stack[idx] = hs[j] == true ? 1 : 0;
                        dummy_time_stack[idx] = tms_subset0[t] == true ? 0 : 1;
                        interaction_stack[idx] = dummy_select_stack[idx] * dummy_time_stack[idx];
                        id_stack[idx] = j;
                        newids.push_back(idx+1);
                        idx += 1;
                    }
                }
            }
        } // end if (compare_r_and_t)
    }
    
    // create in-memory table
    OGRTable* mem_table_int = NULL;
    
    if (!newids.empty()) {
        int n = newids.size();
        if (mem_table_int == NULL) mem_table_int = new OGRTable(n);
        OGRColumn* id_col = new OGRColumnInteger("STID", 18, 0, n);
        id_col->UpdateData(newids);
        mem_table_int->AddOGRColumn(id_col);
    }
    
    if (!id_stack.empty()) {
        int n = id_stack.size();
        if (mem_table_int == NULL) mem_table_int = new OGRTable(n);
        
        bool using_default_id = true;
        
        WeightsManInterface* wmi = NULL;
        if (project && project->GetWManInt()) {
            wmi = project->GetWManInt();
            boost::uuids::uuid default_wid = wmi->GetDefault();
            if (!default_wid.is_nil()) {
                GalWeight* gw = wmi->GetGal(default_wid);
                
                vector<wxString> id_vec;
                TableInterface* table_int = project->GetTableInt();
                int c_id = table_int->FindColId(gw->id_field);
                if (c_id > 0) {
                    table_int->GetColData(c_id, 1, id_vec);
                   
                    vector<wxString> new_id_vec;
                    for (int ii=0; ii<n; ii++) {
                        new_id_vec.push_back(id_vec[id_stack[ii]]);
                    }
                    OGRColumn* id_col = new OGRColumnString(gw->id_field, 50, 0, n);
                    id_col->UpdateData(new_id_vec);
                    mem_table_int->AddOGRColumn(id_col);
                    using_default_id = false;
                }
            }
        }
        /*
        if (using_default_id) {
            // if no weights/id_field, then use 0,1,2,...
            OGRColumn* id_col = new OGRColumnInteger("ORIG_ID", 18, 0, n);
            id_col->UpdateData(id_stack);
            mem_table_int->AddOGRColumn(id_col);
        }
         */
    }
    
    if (!dummy_time_stack.empty()) {
        int n = dummy_time_stack.size();
        if (mem_table_int == NULL) mem_table_int = new OGRTable(n);
        OGRColumn* time_col = new OGRColumnInteger("TIME", 18, 0, n);
        time_col->UpdateData(dummy_time_stack);
        mem_table_int->AddOGRColumn(time_col);
    }
    
    if (!dummy_select_stack.empty()) {
        int n = dummy_select_stack.size();
        if (mem_table_int == NULL) mem_table_int = new OGRTable(n);
        OGRColumn* select_col = new OGRColumnInteger("SPACE", 18, 0, n);
        select_col->UpdateData(dummy_select_stack);
        mem_table_int->AddOGRColumn(select_col);
    }

    if (!interaction_stack.empty()) {
        int n = interaction_stack.size();
        if (mem_table_int == NULL) mem_table_int = new OGRTable(n);
        OGRColumn* interact_col = new OGRColumnInteger("INTERACT", 18, 0, n);
        interact_col->UpdateData(interaction_stack);
        mem_table_int->AddOGRColumn(interact_col);
    }
    
    if (!var_stack_array.empty()) {
        for (size_t i=0; i<var_stack_array.size(); i++) {
            wxString col_name(var_man.GetName(i));
            int n = var_stack_array[i].size();
            if (mem_table_int == NULL) mem_table_int = new OGRTable(n);
            OGRColumn* var_col = new OGRColumnDouble(col_name, 18, -1, n);
            var_col->UpdateData(var_stack_array[i]);
            mem_table_int->AddOGRColumn(var_col);
        }
    }
    
    // export
    ExportDataDlg dlg(this, (TableInterface*)mem_table_int);
    if (dlg.ShowModal() == wxID_OK) {
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
    
    // clean memory
    delete mem_table_int;
}

void LineChartFrame::OnDIDTest(wxCommandEvent& event)
{
    int nTests = var_man.GetVarsCount();
    TableInterface* table_int = project->GetTableInt();
    const std::vector<bool>& hs(highlight_state->GetHighlight());
    int m_obs = project->GetNumRecords();
   

    
    // regression options
    bool m_constant_term = true;
    int RegressModel = 1; // for classic linear regression
    wxGauge* m_gauge = NULL;
    bool do_white_test = true;
	double *m_resid1, *m_yhat1;
   
    for (int i=0; i<nTests; i++) {
       
        wxString m_Yname = var_man.GetName(i);
        std::vector<wxString> m_Xnames;
        
        m_Xnames.push_back("CONSTANT");
        
        
        // Y and X data
        
		wxString row_nm(var_man.GetName(i));
		wxString row_title(row_nm);
		const vec_vec_dbl_type& Y(data_map[row_nm]);
        
        size_t n_ts = Y.size();

        // check selection
        if (compare_regimes || compare_r_and_t) {
            bool has_selection = false;
            for (int j=0; j<m_obs; j++) {
                if (hs[j] == true) {
                    has_selection = true;
                }
            }
            if (!has_selection) {
                wxMessageBox("Please first select observations in one of the other data or map views.");
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
                wxMessageBox("Please define time periods by selecting on the horizontal axis.");
                return;
            }
        }
        
        if (compare_regimes) {
            m_Xnames.push_back("SPACE_DUMMY");
            int nX = m_Xnames.size();
           
            int n = 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n += m_obs;
                }
            }
            if (n == 0) {
                wxMessageBox("Please choose time periods by selecting on the horizontal axis first.");
                return;
            }
            
            
            double *y = new double[n];
            double **x = new double* [2];
            for (int t=0; t<nX; t++)  x[t] = new double[n];
            
            int idx = 0;
            
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    for (int j=0; j<m_obs; j++) {
                        y[idx] = Y[t][j];
                        x[0][idx] = 1.0; //constant
                        x[1][idx] = hs[j] == true ? 1.0 : 0.0; // DUMMY_SELECT
                        idx += 1;
                    }
                }
            }
           
			DiagnosticReport m_DR(n, nX, m_constant_term, true, RegressModel);
        	for (int i = 0; i < nX; i++) {
        		m_DR.SetXVarNames(i, m_Xnames[i]);
        	}
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));
           
            
            classicalRegression(NULL, n, y, n, x, nX, &m_DR,
                                m_constant_term, true, m_gauge,
                                do_white_test);
            
			m_resid1= m_DR.GetResidual();
			printAndShowClassicalResults(row_nm, y, table_int->GetTableName(), wxEmptyString, &m_DR, m_obs, nX, do_white_test);
			m_yhat1 = m_DR.GetYHAT();
            
            delete[] y;
            for (int t=0; t<nX; t++) delete[] x[t];
			m_DR.release_Var();
            wxDateTime now = wxDateTime::Now();
            logReport = ">>" + now.FormatDate() + " " + now.FormatTime() + "\nREGRESSION (DIFF-IN-DIFF, COMPARE REGIMES) \n----------\n" + logReport;
            
        } else if (compare_time_periods) {
            m_Xnames.push_back("DUMMY_PERIOD");
            int nX = m_Xnames.size();
            
            int n1 = 0, n2 = 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n1 += m_obs;
                }
            }
            if (n1 == 0) {
                wxMessageBox("Please choose Period 1 on the horizontal axis first.");
                return;
            }
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset1[t]) {
                    n2 += m_obs;
                }
            }
            if (n2 == 0) {
                wxMessageBox("Please choose Period 2 on the horizontal axis first.");
                return;
            }
            
            int n = n1 + n2;
            
            double *y = new double[n];
            double **x = new double* [2];
            for (int t=0; t<nX; t++) {
                x[t] = new double[n];
            }
            
            int idx = 0;
            
            for (int t=0; t<n_ts; t++) {
                if (tms_subset0[t] || tms_subset1[t]) {
                    for (int j=0; j<m_obs; j++) {
                        y[idx] = Y[t][j];
                        x[0][idx] = 1.0; //constant
                        x[1][idx] = tms_subset0[t] == true ? 1 : 0; // DUMMY_PERIOD
                        idx += 1;
                    }
                }
            }
           
			DiagnosticReport m_DR(n, nX, m_constant_term, true, RegressModel);
        	for (int i = 0; i < nX; i++) {
        		m_DR.SetXVarNames(i, m_Xnames[i]);
        	}
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));
           
            
            classicalRegression(NULL, n, y, n, x, nX, &m_DR,
                                m_constant_term, true, m_gauge,
                                do_white_test);
            
			m_resid1= m_DR.GetResidual();
			printAndShowClassicalResults(row_nm, y, table_int->GetTableName(), wxEmptyString, &m_DR, m_obs, nX, do_white_test);
			m_yhat1 = m_DR.GetYHAT();
            
            delete[] y;
            for (int t=0; t<nX; t++) delete[] x[t];
			m_DR.release_Var();
            wxDateTime now = wxDateTime::Now();
            logReport = ">>" + now.FormatDate() + " " + now.FormatTime() + "\nREGRESSION (DIFF-IN-DIFF, COMPARE TIME PERIOD) \n----------\n" + logReport;
            
        } else if (compare_r_and_t) {
            m_Xnames.push_back("SPACE_DUMMY");
            m_Xnames.push_back("TIME_PERIOD");
            m_Xnames.push_back("INTERACTION");
            int nX = m_Xnames.size();
            
            int n1 = 0, n2 = 0;
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset0[t]) {
                    n1 += m_obs;
                }
            }
            if (n1 == 0) {
                wxMessageBox("Please choose Period 1 on the horizontal axis first.");
                return;
            }
    		for (size_t t=0; t<n_ts; ++t) {
                if (tms_subset1[t]) {
                    n2 += m_obs;
                }
            }
            if (n2 == 0) {
                wxMessageBox("Please choose Period 2 on the horizontal axis first.");
                return;
            }
            
            int n = n1 + n2;
            double *y = new double[n];
            double **x = new double* [nX];
            for (int t=0; t<nX; t++) {
                x[t] = new double[n];
            }
            
            int idx = 0;
            
            for (int t=0; t<n_ts; t++) {
                if (tms_subset0[t] || tms_subset1[t]) {
                    for (int j=0; j<m_obs; j++) {
                        y[idx] = Y[t][j];
                        x[0][idx] = 1.0; //constant
                        x[1][idx] = hs[j] == true ? 1.0 : 0.0; // DUMMY_SELECT
                        x[2][idx] = tms_subset0[t] == true ? 1 : 0; // DUMMY_PERIOD
                        x[3][idx] = x[1][idx] * x[2][idx];
                        idx += 1;
                    }
                }
            }
           
			DiagnosticReport m_DR(n, nX, m_constant_term, true, RegressModel);
        	for (int i = 0; i < nX; i++) {
        		m_DR.SetXVarNames(i, m_Xnames[i]);
        	}
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));
            
            classicalRegression(NULL, n, y, n, x, nX, &m_DR,
                                m_constant_term, true, m_gauge,
                                do_white_test);
            
            m_resid1= m_DR.GetResidual();
            printAndShowClassicalResults(row_nm, y, table_int->GetTableName(), wxEmptyString, &m_DR, m_obs, nX, do_white_test);
            m_yhat1 = m_DR.GetYHAT();
            
            delete[] y;
            for (int t=0; t<nX; t++) delete[] x[t];
            m_DR.release_Var();
 
            
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
        
    }
    

}

void LineChartFrame::OnReportClose(wxWindowDestroyEvent& event)
{
    regReportDlg = 0;
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
	LOG_MSG("In LineChartFrame::update(TableState*)");
	UpdateDataMapFromVarMan();
	if (vars_chooser_frame) vars_chooser_frame->UpdateFromTable();
	const std::vector<bool>& hs(highlight_state->GetHighlight());
	for (size_t i=0, sz=lc_stats.size(); i<sz; ++i) {
		lc_stats[i]->UpdateRegimesStats(hs);
		lc_stats[i]->UpdateOtherStats();
	}
	for (size_t i=0, sz=line_charts.size(); i<sz; ++i) {
        line_charts[i]->UpdateYAxis();
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
        line_charts[i]->UpdateYAxis();
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
		message_win->Unbind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
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
            stats_wins[i]->Unbind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
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
		message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1));
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
		title1_txt = new wxStaticText(panel, wxID_ANY, "New Title Text", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL);
		title1_h_szr->Add(title1_txt);
		panel_v_szr->Add(title1_h_szr, 0, wxALIGN_LEFT|wxBOTTOM, 5);

		if (compare_time_periods || compare_r_and_t) {
			ctrls_h_szr = new wxBoxSizer(wxHORIZONTAL);
			wxRadioButton* rb0 = new wxRadioButton(panel, XRCID("ID_RAD_BUT_0"), "Period 1", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL | wxRB_GROUP);
			rb0->SetValue(true);
			wxRadioButton* rb1 = new wxRadioButton(panel, XRCID("ID_RAD_BUT_1"), "Period 2", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL);
			rb1->SetValue(false);
			Connect(XRCID("ID_RAD_BUT_0"), wxEVT_RADIOBUTTON, wxCommandEventHandler(LineChartFrame::OnSelectPeriod0));
			Connect(XRCID("ID_RAD_BUT_1"), wxEVT_RADIOBUTTON, wxCommandEventHandler(LineChartFrame::OnSelectPeriod1));

            ctrls_h_szr->AddSpacer(15);
			ctrls_h_szr->Add(rb0);
			ctrls_h_szr->Add(rb1);
            ctrls_h_szr->AddSpacer(15);

			panel_v_szr->Add(ctrls_h_szr, 0, wxALIGN_LEFT|wxBOTTOM, 5);
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
				wv = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
				//wv = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr, wxDefaultPosition, wxDefaultSize);
                wv->Bind(wxEVT_RIGHT_UP, &LineChartFrame::OnMouseEvent, this);
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
		if (display_stats)
            col0_proportion = compare_r_and_t ? 2 : 3;
		
        int col1_proportion = 1;
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0))
            bag_szr->RemoveGrowableCol(0);
		bag_szr->AddGrowableCol(0, col0_proportion);
		
        if (display_stats) {
			if (bag_szr->IsColGrowable(1))
                bag_szr->RemoveGrowableCol(1);
			bag_szr->AddGrowableCol(1, col1_proportion);
		}
		for (int i=0; i<num_vars; ++i) {
			if (bag_szr->IsRowGrowable(i))
                bag_szr->RemoveGrowableRow(i);
			bag_szr->AddGrowableRow(i, 1);
		}
	}
    //panel_v_szr->AddSpacer(15);
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	UpdateTitleText();
	panel_h_szr->RecalcSizes();
    
    double y_min = lc_stats[0]->Y_avg_min;
    double y_max = lc_stats[0]->Y_avg_max;
    for (int i=0; i< lc_stats.size(); i++){
        if (y_min > lc_stats[i]->Y_avg_min)
            y_min = lc_stats[i]->Y_avg_min;
        if (y_max < lc_stats[i]->Y_avg_max)
            y_max = lc_stats[i]->Y_avg_max;
    }
    
    double y_pad = 0.1 * (y_max - y_min);
    def_y_min << floor(y_min - y_pad);
    def_y_max << y_max + y_pad;

    
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
			frame_title << " - Compare Regimes - " << var_man.GetName(0);
		} else if (compare_time_periods) {
			frame_title << " - Compare Time Periods - " << var_man.GetName(0);
		} else if (compare_r_and_t) {
			frame_title << " - Compare Regimes and Times - " << var_man.GetName(0);
		}
	}
	SetTitle(frame_title);
	
	if (!title1_txt)
        return;
	//if (compare_r_and_t && !title2_txt) return;
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
		if (compare_regimes) {

            ln1 << "Compare the means of two groups over time periods:";
            ln1 << tm0_st;
            ln1 << ":\n";
            ln1 << "    Group 1 (" << span_clr_sel << "selected" << span_end << ")";
            ln1 << " vs. ";
            ln1 << "Group 2 (" << span_clr_exl << "excluded" << span_end << ")";
            ln1 << "\n\n";
            ln1 << "    Select observations in a map or plot.\n";
            ln1 << "    Select time period(s) on the horizontal axis below.";
		}
		if (compare_time_periods) {
            ln1 << "Compare the means of the same variable in two time periods:\n";
            ln1 << span_clr_tm1;
            ln1 << "    Period 1 (" << (tm0_st.IsEmpty() ? "choose time" : tm0_st) << ")";
            ln1 << span_end;
            ln1 << " vs. ";
            ln1 << span_clr_tm2;
            ln1 << "Period 2 (" << (tm1_st.IsEmpty() ? "choose time" : tm1_st) << ")";
            ln1 << span_end;
            ln1 << "\n\n";
            ln1 << "    Select time period(s) on the horizontal axis below.";

		}
		if (compare_r_and_t) {
            
            ln1 << "Compare the means of two groups in two time periods:\n";
            ln1 << "    Group 1 (" << span_clr_sel << "selected" << span_end << ")";
            ln1 << " vs. ";
            ln1 << "Group 2 (" << span_clr_exl << "excluded" << span_end << ")";
            ln1 << "\n";
            ln1 << "    Period 1 (" << span_clr_tm1 << (tm0_st.IsEmpty() ? "choose time" : tm0_st) << span_end << ")";
            ln1 << " vs. ";
            ln1 << "Period 2 (" << span_clr_tm2 << (tm1_st.IsEmpty() ? "choose time" : tm1_st) << span_end << ")";
            ln1 << "\n\n";
            ln1 << "    Select observations in a map or plot.\n";
            ln1 << "    Select time periods on the horizontal axis below.";
		}
	} else {
		if (compare_regimes || compare_r_and_t) {
			ln1 << "Sample 1: " << selected_str << "    Sample 2: " << excluded_str;
		}
	}
    title1_txt->SetLabel("");
	title1_txt->SetLabelMarkup(ln1);
	//if (title2_txt && compare_r_and_t) title2_txt->SetLabelMarkup(ln2);
    
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
	s<< "<head></head>\n";
	s<< "<body>\n";
	s<< "<center>\n";
	s<< "<font face=\"verdana,arial,sans-serif\" color=\"black\" size=\"2\">";
	
	s<< "<table width=100% rules=\"rows\" >";
	s<< "<tr bgcolor=\"#CCCCCC\" >";
	s<< "<th width=120 align=\"center\">Group</th>";
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
            s<< "<td align=\"left\">1. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << ">Selected</font></td>";
        if (cmp_t)
            s<< "<td align=\"left\">1. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << ">Period 1</font></td>";
        if (cmp_r_t)
            s<< "<td align=\"left\">1. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << ">Selected</font> in <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << ">Period 1</font></td>";
        
		s<< "<td align=\"right\">" << lcs.s0.sz_i << "</td>";
		s<< td_s0_mean;
		s<< "<td align=\"right\">" << sd0 << "</td>";
		s<< "</tr>";
		s<< "<tr>";
        
        if (cmp_r)
            s<< "<td align=\"left\">2. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << ">Excluded</font></td>";
        if (cmp_t)
            s<< "<td align=\"left\">2. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << ">Period 2</font></td>";
        if (cmp_r_t)
            s<< "<td align=\"left\">2. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << ">Excluded</font> in <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm1_dark) << ">Period 1</font></td>";
        
		s<< "<td align=\"right\">" << lcs.s1.sz_i << "</td>";
		s<< td_s1_mean;
		s<< "<td align=\"right\">" << sd1 << "</td>";
		s<< "</tr>";
	}
    
	if (cmp_r_t && !single_sample) {
		s<< "<tr>";
		s<< "<td align=\"left\">3. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_sel_dark) << ">Selected</font> in <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << ">Period 2</font></td>";
		s<< "<td align=\"right\">" << lcs.s2.sz_i << "</td>";
		s<< td_s2_mean;
		s<< "<td align=\"right\">" << sd2 << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td align=\"left\">4. <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_exl_dark) << ">Excluded</font> in <font color=" << GdaColorUtils::ToHexColorStr(GdaConst::ln_cht_clr_tm2_dark) << ">Period 2</font></td>";
		s<< "<td align=\"right\">" << lcs.s3.sz_i << "</td>";
		s<< td_s3_mean;
		s<< "<td align=\"right\">" << sd3 << "</td>";
		s<< "</tr>";
	}
	s<< "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td></tr>";
	s<< "</table>\n";

    
    
	if (lcs.test_stat_valid && !cmp_r_t) {
        s<< "<br/>T-Test: Do Means Differ?<br/><br/>";
		s<< "<table>";
		s<< "<tr>";
		s<< "<td bgcolor=\"#CCCCCC\" align=\"center\">D.F.&nbsp;</td>";
        stringstream _s;
        _s << std::fixed << std::setprecision(2) << lcs.deg_free;
		s<< "<td align=\"center\">" << _s.str() << "</td>";
		s<< "</tr>";
		s<< "<tr>";
		s<< "<td bgcolor=\"#CCCCCC\" align=\"right\">T Stat&nbsp;</td>";
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
        s<< "<br/><br/>Right-click for diff-in-diff test.";
	}
	
	if (cmp_r_t) {
        s<< "<br/>T-Test: Do Means Differ?<br/><br/>";
		s<< "<table>";
		s<< "<tr bgcolor=\"#CCCCCC\">";
		s<< "<td align=\"center\">&nbsp;Compare&nbsp;</td>";
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
        s<< "<br/><br/>Right-click for diff-in-diff test.";
	}
	
	s<< "</center>\n";
	s<< "</font>";
	
	s<< "</body>\n";
	s<< "</html>\n";
	
	stats_win->SetPage(s);
}

void LineChartFrame::printAndShowClassicalResults(const wxString& yName, double* y,
                                                  const wxString& datasetname,
                                                 const wxString& wname,
                                                 DiagnosticReport *r,
                                                 int Obs, int nX,
                                                 bool do_white_test)
{
    LOG_MSG("Entering RegressionDlg::printAndShowClassicalResults");
    wxString f; // temporary formatting string
    wxString slog;
    
    logReport = wxEmptyString; // reset log report
    int cnt = 0;
    
    slog << "SUMMARY OF OUTPUT: ORDINARY LEAST SQUARES ESTIMATION\n"; cnt++;
    slog << "Data set            :  " << datasetname << "\n"; cnt++;
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
        slog << wxString::Format("  %12.7g   %12.7g   %12.7g   %9.5f\n",
                                 r->GetCoefficient(i), r->GetStdError(i),
                                 r->GetZValue(i), r->GetProbability(i)); cnt++;
    }
    slog << "----------------------------------------";
    slog << "-------------------------------------\n\n"; cnt++; cnt++;
   
    /*
    slog << "REGRESSION DIAGNOSTICS  \n"; cnt++;
    double *rr = r->GetBPtest();
    if (rr[1] > 1) {
        slog << wxString::Format("MULTICOLLINEARITY CONDITION NUMBER   %7f\n",
                                 r->GetConditionNumber()); cnt++;
    } else {
        slog << wxString::Format("MULTICOLLINEARITY CONDITION NUMBER   %7f\n",
                                 r->GetConditionNumber()); cnt++;
        slog << "                                ";
        slog << "      (Extreme Multicollinearity)\n"; cnt++;
    }
    slog << "TEST ON NORMALITY OF ERRORS\n"; cnt++;
    slog << "TEST                  DF           VALUE             PROB\n"; cnt++;
    rr = r->GetJBtest();
    f = "Jarque-Bera           %2.0f        %11.4f        %9.5f\n"; cnt++;
    slog << wxString::Format(f, rr[0], rr[1], rr[2]);
    
    slog << "\n"; cnt++;
    slog << "DIAGNOSTICS FOR HETEROSKEDASTICITY  \n"; cnt++;
    slog << "RANDOM COEFFICIENTS\n"; cnt++;
    slog << "TEST                  DF           VALUE             PROB\n"; cnt++;
    rr = r->GetBPtest();
    if (rr[1] > 0) {
        f = "Breusch-Pagan test    %2.0f        %11.4f        %9.5f\n"; cnt++;
        slog << wxString::Format(f, rr[0], rr[1], rr[2]);
    } else {
        f = "Breusch-Pagan test    %2.0f        %11.4f        N/A\n"; cnt++;
        slog << wxString::Format(f, rr[0], rr[1]);
    }
    rr = r->GetKBtest();
    if (rr[1]>0) {
        f = "Koenker-Bassett test  %2.0f        %11.4f        %9.5f\n"; cnt++;
        slog << wxString::Format(f, rr[0], rr[1], rr[2]);
    } else {
        f = "Koenker-Bassett test  %2.0f        %11.4f        N/A\n"; cnt++;
        slog << wxString::Format(f, rr[0], rr[1]);
    }
    if (do_white_test) {
        slog << "SPECIFICATION ROBUST TEST\n"; cnt++;
        rr = r->GetWhitetest();
        slog << "TEST                  DF           VALUE             PROB\n"; cnt++;
        if (rr[2] < 0.0) {
            f = "White                 %2.0f            N/A            N/A\n"; cnt++;
            slog << wxString::Format(f, rr[0]);
        } else {
            f = "White                 %2.0f        %11.4f        %9.5f\n"; cnt++;
            slog << wxString::Format(f, rr[0], rr[1], rr[2]);
        }
    }
    
    if (true) {
        slog << "\n"; cnt++;
        slog << "COEFFICIENTS VARIANCE MATRIX\n"; cnt++;
        int start = 0;
        while (start < nX) {
            wxString st = wxEmptyString;
            for (int j=start; j<nX && j<start+5; j++) {
                slog << " " << GenUtils::Pad(r->GetXVarName(j), 10) << " ";
            }
            slog << "\n"; cnt++;
            for (int i=0; i<nX; i++) {
                st = wxEmptyString;
                for (int j=start; j<nX && j<start+5; j++) {
                    slog << wxString::Format(" %10.6f ", r->GetCovariance(i,j));
                }
                slog << "\n"; cnt++;
            }
            slog << "\n"; cnt++;
            start += 5;
        }
    }

    
    if (true) {
        slog << "\n";
        cnt++;
        slog << "  OBS    " << GenUtils::Pad(yName, 12);
        slog << "        PREDICTED        RESIDUAL";
        for (int ii=1; ii<nX; ii++)
            slog << "        " << GenUtils::Pad(r->GetXVarName(ii), 12);
        slog << "\n";
        
        cnt++;
        double *res = r->GetResidual();
        double *yh = r->GetYHAT();
        for (int i=0; i<Obs; i++) {
            slog << wxString::Format("%5d     %12.5f    %12.5f    %12.5f",
                                     i+1, y[i], yh[i], res[i]);
            for (int ii=1; ii<nX; ii++)
                slog << wxString::Format("     %12.5f", X[ii-1][i]);
            slog << "\n";
            cnt++;
        }
        res = NULL;
        yh = NULL;
    }
    */
    
    slog << "============================== END OF REPORT";
    slog <<  " ================================\n\n"; cnt++; cnt++;
    
    slog << "\n\n"; cnt++; cnt++;
    logReport << slog;
    
    LOG_MSG(wxString::Format("%d lines written to logReport.", cnt));
    LOG_MSG("Exiting RegressionDlg::printAndShowClassicalResults");
}
