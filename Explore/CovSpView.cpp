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
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include "../DialogTools/VariableSettingsDlg.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "SimpleAxisCanvas.h"
#include "SimpleScatterPlotCanvas.h"
#include "SimpleHistCanvas.h"
#include "CovSpHLStateProxy.h"
#include "CovSpView.h"

BEGIN_EVENT_TABLE(CovSpFrame, TemplateFrame)
EVT_MOUSE_EVENTS(CovSpFrame::OnMouseEvent)
EVT_ACTIVATE(CovSpFrame::OnActivate)
END_EVENT_TABLE()

using namespace std;

CovSpFrame::CovSpFrame(wxFrame *parent, Project* project,
                       const GdaVarTools::Manager& var_man_,
                       WeightsMetaInfo::DistanceMetricEnum dist_metric_,
                       WeightsMetaInfo::DistanceUnitsEnum dist_units_,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size)
: TemplateFrame(parent, project, title, pos, size, wxDEFAULT_FRAME_STYLE),
var_man(var_man_),
dist_metric(dist_metric_), dist_units(dist_units_),
lowess_param_frame(0), panel(0),
panel_v_szr(0), bag_szr(0), top_h_sizer(0),
show_regimes(false), show_outside_titles(true), show_linear_smoother(false),
show_lowess_smoother(true), show_slope_values(false),
scatt_plot(0), vert_label(0), horiz_label(0),
too_many_obs(project->GetNumRecords() > 1000)
{
    wxLogMessage("Open CovSpFrame (Non-parametric Spatial Autocorrelation.");
	if (!too_many_obs) {
		pairs_hl_state = project->GetPairsHLState();
		project->FillDistances(D, dist_metric, dist_units);
		D_min = D[0];
		D_max = D[0];
		for (size_t i=0, sz=D.size(); i<sz; ++i) {
			if (D[i] < D_min) {
				D_min = D[i];
			} else if (D[i] > D_max) {
				D_max = D[i];
			}
		}
		UpdateDataFromVarMan();
		SetGetStatusBarStringFromFrame(true);
	}
	
	supports_timeline_changes = true;
	int width, height;
	GetClientSize(&width, &height);
	
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	panel->Bind(wxEVT_MOTION, &CovSpFrame::OnMouseEvent, this);
	
	message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition,
																 wxSize(200,-1));
	
	message_win->Bind(wxEVT_MOTION, &CovSpFrame::OnMouseEvent, this);
	
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap
	
	bag_szr->Add(message_win, wxGBPosition(0,0), wxGBSpan(1,1), wxEXPAND);
	bag_szr->SetFlexibleDirection(wxBOTH);
	bag_szr->AddGrowableCol(0, 1);
	bag_szr->AddGrowableRow(0, 1);	
	
	panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_h_szr);
	
	// Top Sizer for Frame
	top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	
	SetSizer(top_h_sizer);
	DisplayStatusBar(!too_many_obs);

	UpdatePanel();
	
	Show(true);
}

CovSpFrame::~CovSpFrame()
{
	if (lowess_param_frame) {
		lowess_param_frame->removeObserver(this);
		lowess_param_frame->closeAndDeleteWhenEmpty();
	}
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void CovSpFrame::OnMouseEvent(wxMouseEvent& event)
{
	if (event.RightDown()) {
	}
}

void CovSpFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In CovSpFrame::OnActivate()");
		RegisterAsActive("CovSpFrame", GetTitle());
	}
	//if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void CovSpFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
	LoadMenu("ID_COV_SCATTER_PLOT_MENU_OPTIONS");	
	CovSpFrame::UpdateContextMenuItems(optMenu);
	
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void CovSpFrame::UpdateOptionMenuItems()
{
	//TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
	if (menu == wxNOT_FOUND) {
	} else {
		CovSpFrame::UpdateContextMenuItems(mb->GetMenu(menu));
	}
}

void CovSpFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_LINEAR_SMOOTHER"),
                                  show_linear_smoother);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_LOWESS_SMOOTHER"),
                                  show_lowess_smoother);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_REGIMES_REGRESSION"),
                                  show_regimes);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_SLOPE_VALUES"),
                                  show_slope_values);
    
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void CovSpFrame::UpdateTitle()
{
	wxString s = _("Nonparametric Spatial Autocorrelation");
	if (var_man.GetVarsCount() > 0) s << " - " << var_man.GetNameWithTime(0);
	SetTitle(s);
}

wxString CovSpFrame::GetUpdateStatusBarString(const vector<int>& hover_obs,
                                              int total_hover_obs)
{
	wxString s;
	const pairs_bimap_type& bimap = project->GetSharedPairsBimap();
	int last = GenUtils::min<int>(total_hover_obs, hover_obs.size(), 2);
	size_t t = var_man.GetTime(0);
	typedef pairs_bimap_type::left_map::const_iterator left_const_iterator;
	for (int h=0; h<last; ++h) {
		left_const_iterator p = bimap.left.find(hover_obs[h]);
		int i = p->second.i;
		int j = p->second.j;
		//s << "sz(Z)=" << Z[t].size() << ", sz(D)=" << D.size(); 
		//s << ", hover_obs[" << h << "]=" << hover_obs[h];
		s << "dist(" << i+1 << "," << j+1 << ")=" << D[hover_obs[h]];
		s << ", corr(" << i+1 << "," << j+1 << ")=" << Zprod[t][hover_obs[h]];
		//s << ", Z["<<i+1<<"]="<<Z[t][i];
		//s << ", Z["<<j+1<<"]="<<Z[t][j];
		//s << ", MeanZ="<<MeanZ[t];
		//s << ", VarZ="<<VarZ[t];
		if (h+1<last) s << ";  ";
	}
	if (total_hover_obs > 2) {
		s << "; ...";
	}
	return s;
}

void CovSpFrame::OnViewLinearSmoother(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnViewLinearSmoother");
	if (too_many_obs) return;
	show_linear_smoother = !show_linear_smoother;
	scatt_plot->ShowLinearSmoother(show_linear_smoother);
	UpdateOptionMenuItems();
}

void CovSpFrame::OnViewLowessSmoother(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnViewLowessSmoother");
	if (too_many_obs) return;
	show_lowess_smoother = !show_lowess_smoother;
	scatt_plot->ShowLowessSmoother(show_lowess_smoother);
	UpdateOptionMenuItems();
}

void CovSpFrame::OnEditLowessParams(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnEditLowessParams");
	if (too_many_obs) return;
	if (lowess_param_frame) {
		lowess_param_frame->Iconize(false);
		lowess_param_frame->Raise();
		lowess_param_frame->SetFocus();
	} else {
		Lowess l; // = t->GetLowess();  // should be shared by all cells
		lowess_param_frame = new LowessParamFrame(l.GetF(), l.GetIter(),
																							l.GetDeltaFactor(),
																							project);
		lowess_param_frame->registerObserver(this);
	}
}

void CovSpFrame::OnShowVarsChooser(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnShowVarsChooser");
	if (too_many_obs) return;
	VariableSettingsDlg VS(project, VariableSettingsDlg::univariate,
												 false, true, "Variable Choice", "Variable");
	if (VS.ShowModal() != wxID_OK) return;
	GdaVarTools::VarInfo& v = VS.var_info[0];
	vector<wxString> tm_strs;
	project->GetTableInt()->GetTimeStrings(tm_strs);
	GdaVarTools::Manager t_var_man(tm_strs);
	t_var_man.AppendVar(v.name, v.min, v.max, v.time,
										v.sync_with_global_time, v.fixed_scale);
	var_man = t_var_man;
	
	// If distance metric or units changed, then reinit distance as well
	bool change = false;
	if (dist_metric != VS.GetDistanceMetric()) {
		change = true;
		dist_metric = VS.GetDistanceMetric();
	} else if (dist_metric == WeightsMetaInfo::DM_arc) {
		if (dist_units != VS.GetDistanceUnits()) {
			change = true;
			dist_units = VS.GetDistanceUnits();
		}
	}
	if (change) {
		project->FillDistances(D, dist_metric, dist_units);
		D_min = D[0];
		D_max = D[0];
		for (size_t i=0, sz=D.size(); i<sz; ++i) {
			if (D[i] < D_min) {
				D_min = D[i];
			} else if (D[i] > D_max) {
				D_max = D[i];
			}
		}
	}
	UpdateDataFromVarMan();
	UpdatePanel();
}

void CovSpFrame::OnViewRegimesRegression(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnViewRegimesRegression");
	if (too_many_obs) return;
	show_regimes = !show_regimes;
	scatt_plot->ShowRegimes(show_regimes);
	UpdateOptionMenuItems();
}

void CovSpFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnDisplayStatistics");
	if (too_many_obs) return;
	// should be managed here or by shared manager
	//CovSpCanvas* t = (CovSpCanvas*) template_canvas;
	//t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void CovSpFrame::OnDisplaySlopeValues(wxCommandEvent& event)
{
	wxLogMessage("In CovSpFrame::OnDisplaySlopeValues");
	if (too_many_obs) return;
	show_slope_values = !show_slope_values;
	scatt_plot->ShowSlopeValues(show_slope_values);
	UpdateOptionMenuItems();
}

/** Implementation of TableStateObserver interface */
void CovSpFrame::update(TableState* o)
{
}

/** Implementation of TimeStateObserver interface */
void CovSpFrame::update(TimeState* o)
{
	var_man.UpdateGlobalTime(o->GetCurrTime());
	if (var_man.GetVarsCount() >= 1)
        UpdatePanel();
	UpdateTitle();
	Refresh();
}

void CovSpFrame::update(LowessParamObservable* o)
{
	scatt_plot->ChangeLoessParams(o->GetF(), o->GetIter(), o->GetDeltaFactor());
}

void CovSpFrame::notifyOfClosing(LowessParamObservable* o)
{
	lowess_param_frame = 0;
}

void CovSpFrame::UpdatePanel()
{
	if (!panel || !bag_szr) return;
	template_canvas = 0;
	int num_vars = var_man.GetVarsCount();
	if (message_win) {
		message_win->Unbind(wxEVT_MOTION, &CovSpFrame::OnMouseEvent, this);
		bool detatch_success = bag_szr->Detach(0);
		LOG(detatch_success);
		message_win->Destroy();
		message_win = 0;
	}
	bag_szr->Clear();
	panel_v_szr->Remove(bag_szr); // bag_szr is deleted automatically
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap
	if (scatt_plot) {
		scatt_plot->Unbind(wxEVT_MOTION, &CovSpFrame::OnMouseEvent, this);
		scatt_plot->Destroy();
	}
	scatt_plot = 0;
	if (vert_label) vert_label->Destroy();
    
	vert_label = 0;
	if (horiz_label) horiz_label->Destroy();
    
	horiz_label = 0;
	wxString z_err_msg;
    
	if (!too_many_obs) {
		if (var_man.GetVarsCount() > 0) z_err_msg = Z_error_msg[var_man.GetTime(0)];
	}
    
	bool z_var_good = false;
	if (!too_many_obs) {
		z_var_good = (var_man.GetVarsCount() > 0 && z_err_msg.IsEmpty());
	}
    
	if (too_many_obs || var_man.GetVarsCount() <= 0 || !z_var_good) {
		message_win = new wxHtmlWindow(panel, wxID_ANY,
                                       wxDefaultPosition,wxSize(200,-1));
		message_win->Bind(wxEVT_MOTION, &CovSpFrame::OnMouseEvent, this);
		UpdateMessageWin();
		bag_szr->Add(message_win, wxGBPosition(0,0), wxGBSpan(1,1), wxEXPAND);
		
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0)) bag_szr->RemoveGrowableCol(0);
		bag_szr->AddGrowableCol(0, 1);
		if (bag_szr->IsRowGrowable(0)) bag_szr->RemoveGrowableRow(0);
		bag_szr->AddGrowableRow(0, 1);
		
	} else {
		TableInterface* table_int = project->GetTableInt();
		{
			int row = 0;
			wxString z_nm(var_man.GetName(row));
			int z_tm = var_man.GetTime(row);
			wxString z_title = _("Sample Autocorrelation");
			//var_man.GetNameWithTime(row);
			wxString z_tm_str = table_int->GetTimeString(z_tm);
			SimpleAxisCanvas* sa_can = 0;
			{
                sa_can = new SimpleAxisCanvas(panel, this, project, pairs_hl_state,
                                              Zprod[z_tm],
                                              Zprod_undef[z_tm],
                                              z_title,
                                              Zprod_min[z_tm], Zprod_max[z_tm], false,
                                              show_outside_titles, false,
                                              true, true, -1, false, false, 0, false,
                                              wxDefaultPosition, wxSize(50, -1));
				bag_szr->Add(sa_can, wxGBPosition(row, 0), wxGBSpan(1,1), wxEXPAND);
				vert_label = sa_can;
			}
			{
				wxString y_title = "Distance";
				if (dist_metric == WeightsMetaInfo::DM_arc) {
					if (dist_units == WeightsMetaInfo::DU_mile) {
						y_title << " (Arc in miles)";
					} else {
						y_title << " (Arc in kms)";
					}
				}
                sa_can = new SimpleAxisCanvas(panel, this, project, pairs_hl_state,
                                              D, Zprod_undef[z_tm], "Distance",
                                              D_min, D_max, true,
                                              show_outside_titles, true,
                                              true, true, -1, false, false, 0, false,
                                              wxDefaultPosition, wxSize(-1, 50));
				bag_szr->Add(sa_can, wxGBPosition(num_vars, row+1), wxGBSpan(1,1),
										 wxEXPAND);
				horiz_label = sa_can;
			}
			{
				int col = 0;
				SimpleScatterPlotCanvas* sp_can = 0;
				sp_can = new SimpleScatterPlotCanvas(panel, this, project,
													 pairs_hl_state, 0,
													 D,
                                                     Zprod[z_tm],
                                                     Zprod_undef[z_tm],
                                                     Zprod_undef[z_tm],
													 "Distance",
                                                     z_title,
													 D_min, D_max,
													 Zprod_min[z_tm],
                                                     Zprod_max[z_tm],
													 true, true, false,
													 "ID_COV_SCATTER_PLOT_MENU_OPTIONS",
													 !show_outside_titles,
													 true, false, //show axes thru org
													 show_regimes,
													 false, //show_linear_smoother,
													 show_lowess_smoother,
													 false); // show_slope_values
				bag_szr->Add(sp_can, wxGBPosition(row, col+1), wxGBSpan(1,1), wxEXPAND);
				scatt_plot = sp_can;
				template_canvas = sp_can;
			}
		}
		bag_szr->Add(50, 50, wxGBPosition(num_vars, 0), wxGBSpan(1,1));
		
		bag_szr->SetFlexibleDirection(wxBOTH);
		if (bag_szr->IsColGrowable(0)) bag_szr->RemoveGrowableCol(0);
		if (bag_szr->IsRowGrowable(num_vars)) {
			bag_szr->RemoveGrowableRow(num_vars);
		}
		for (int i=0; i<num_vars; ++i) {
			if (bag_szr->IsColGrowable(i+1)) bag_szr->RemoveGrowableCol(i+1);
			bag_szr->AddGrowableCol(i+1, 1);
		}
		for (int i=0; i<num_vars; ++i) {
			if (bag_szr->IsRowGrowable(i)) bag_szr->RemoveGrowableRow(i);
			bag_szr->AddGrowableRow(i, 1);
		}
	}
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	LOG(bag_szr->GetItemCount());
	top_h_sizer->RecalcSizes();
	UpdateTitle();
	Refresh();
}

void CovSpFrame::UpdateMessageWin()
{
	if (!message_win) return;
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "  <style type=\"text/css\">";	
	s << "  body {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    font-size: large;";
	s << "    color: blue;";
	s << "  }";
	
	s << "  h1 {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    color: blue;";
	s << "  }";
	s <<   "</style>";
	
	s << "</head>";
	s << "<body>";
	s << "<br /><br /><br />";
	//s << "  <h1 align=\"center\">Message</h1>";
	s << "<center><p>";
	s << "<font face=\"verdana,arial,sans-serif\" color=\"black\" size=\"5\">";
	
	if (too_many_obs) {
		long n_obs = project->GetNumRecords();
		long n_pairs = (n_obs*(n_obs-1))/2;
		s << "This view currently supports data with at most 1000 observations. ";
		s << "The Nonparametric Spatial Autocorrelation Scatterplot plots ";
		s << "distances between all pairs of observations. ";
		s << "The current data set has " << n_obs << " observations and ";
		s << n_pairs << " unordered pairs of observations.";
	} else {
		int count = var_man.GetVarsCount();
		if (count == 0) {
			s << "Please use<br />";
			s << "<font color=\"blue\">Options > Change Variable<br /></font>";
			s << "to specify a variable.";
		} if (Z_error_msg[var_man.GetTime(0)].IsEmpty()) {
			s << "Variable <font color=\"blue\">" << var_man.GetName(0);
			s << "</font> is specified. ";
		} else {
			s << "Error: " << Z_error_msg[var_man.GetTime(0)];
		}
	}
	s << "  </font></p></center>";
	s << "</body>";
	s << "</html>";
	message_win->SetPage(s );
}

/** Updates Z according to variable present in var_man. */
void CovSpFrame::UpdateDataFromVarMan()
{
	TableInterface* table_int = project->GetTableInt();
	const pairs_bimap_type& bimap = project->GetSharedPairsBimap();
	
    if (var_man.GetVarsCount() == 0) {
        return;
    }
    
	wxString z_name = var_man.GetName(0);
	int c_id = table_int->FindColId(z_name);
    
    if (c_id < 0) {
        return;
    }
    
	bool tm_variant = var_man.IsTimeVariant(0);
	int tms = table_int->GetColTimeSteps(c_id);
    
	if (Z.size() != tms) {
		Z.resize(tms);
		Z_undef.resize(tms);
		Zprod.resize(tms);
		Zprod_min.resize(tms);
		Zprod_max.resize(tms);
		MeanZ.resize(tms);
		VarZ.resize(tms);
		Z_error_msg.resize(tms);
	}
    
	size_t num_obs = table_int->GetNumberRows();
    
	for (size_t t=0; t<tms; ++t) {
		if (Z[t].size() != num_obs) {
			Z[t].resize(num_obs);
			Z_undef[t].resize(num_obs);
			Zprod[t].resize(bimap.size());
			Zprod_undef[t].resize(bimap.size());
		}
        
        // get data from table
		table_int->GetColData(c_id, t, Z[t]);
        table_int->GetColUndefined(c_id, t, Z_undef[t]);
        
        // init Zprod[t]
		if (GdaConst::placeholder_type == table_int->GetColType(c_id, t)) {
			for (pairs_bimap_type::const_iterator e=bimap.begin();
                 e!=bimap.end(); ++e)
            {
                int pair_idx = e->left;
				Zprod[t][pair_idx] = 0;
                
                int obs_i = e->right.i;
                int obs_j = e->right.j;
                Zprod[t][pair_idx]  = Z_undef[t][obs_i] || Z_undef[t][obs_j];
			}
            wxString str_template;
            
            str_template = _T("Variable %s is a placeholer");
            if (tm_variant) {
                str_template = _T("Variable %s at time %d is a placeholer");
            }
            
            wxString s = wxString::Format(str_template, z_name,
                                          table_int->GetTimeString(t));
			Z_error_msg[t] = s;
			continue;
		}
        
        // do calculation
		size_t Z_sz = Z[t].size();
		double N = (double) Z_sz;
		double sum = 0.0;
        
        for (size_t i=0; i<Z_sz; ++i){
            if (Z_undef[t][i])
                continue;
            sum += Z[t][i];
        }
        
		double smpl_mn = sum/N;
        
		MeanZ[t] = smpl_mn;
        
		double ssd = 0.0;
		double diff = 0;
		for (size_t i=0; i<Z_sz; ++i) {
            if (Z_undef[t][i])
                continue;
			diff = Z[t][i] - smpl_mn;
			ssd += diff*diff;
		}
        
		double smpl_var = ssd/N;
		VarZ[t] = smpl_var;
        
		bool success = smpl_var > 0; //GenUtils::StandardizeData(Z[t]);
		if (!success) {
            wxString str_template;
            
            str_template = _T("Variable %s is a placeholer");
            if (tm_variant) {
                str_template = _T("Variable %s at time %d could not be standardized.");
            }
            
            wxString s = wxString::Format(str_template, z_name,
                                          table_int->GetTimeString(t));
			Z_error_msg[t] = s;
		} else {
            
			Z_error_msg[t] = "";
			Zprod_min[t] = numeric_limits<double>::max();
			Zprod_max[t] = numeric_limits<double>::min();
            
			for (pairs_bimap_type::const_iterator e=bimap.begin();
                 e!=bimap.end(); ++e)
            {
                int idx_i = e->right.i;
                int idx_j = e->right.j;
                
                if (Z_undef[t][idx_i] || Z_undef[t][idx_j])
                    continue;
                
				//double p = Z[t][e->right.i]*Z[t][e->right.j];
				double p = (Z[t][idx_i] - smpl_mn) * (Z[t][idx_j] - smpl_mn);
				p = p / smpl_var;
                
				Zprod[t][e->left] = p;
                
				if (p < Zprod_min[t]) Zprod_min[t] = p;
				if (p > Zprod_max[t]) Zprod_max[t] = p;
			}
		}
	}
}

wxString CovSpFrame::GetHelpHtml()
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
	s << "  h1 h2 {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    color: blue;";
	s << "  }";
	s <<   "</style>";
	s << "</head>";
	s << "<body>";
	
	s << "<h2>Nonparametric Spatial Covariance Function Estimation Help</h2>";
	s << "<p>";
	s << "A scatter plot of pairwise spatial correlation versus distance is shown.";
	s << "</p>";
	s << "</body>";
	s << "</html>";
	return s;	
}

