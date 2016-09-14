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

#include <math.h>
#include <iostream>
#include <iomanip>
#include <utility> // std::pair
#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include "../HighlightState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "SimpleAxisCanvas.h"
#include "CorrelogramView.h"

#ifdef __WIN32__
#ifndef isnan
#define isnan(x) _isnan(x)
#endif
#endif


BEGIN_EVENT_TABLE(CorrelogramFrame, TemplateFrame)
	EVT_MOUSE_EVENTS(CorrelogramFrame::OnMouseEvent)
	EVT_ACTIVATE(CorrelogramFrame::OnActivate)
END_EVENT_TABLE()

CorrelogramFrame::CorrelogramFrame(wxFrame *parent, Project* project,
                                   const wxString& title,
                                   const wxPoint& pos,
                                   const wxSize& size)
: TemplateFrame(parent, project, title, pos, size, wxDEFAULT_FRAME_STYLE),
correl_params_frame(0), panel(0),
panel_v_szr(0), bag_szr(0), top_h_sizer(0),
hist_plot(0), local_hl_state(0), message_win(0), project(project)
{
	LOG_MSG("Entering CorrelogramFrame::CorrelogramFrame");
	local_hl_state = new HighlightState();
	supports_timeline_changes = true;
	{
		std::vector<wxString> tm_strs;
		project->GetTableInt()->GetTimeStrings(tm_strs);
		var_man.ClearAndInit(tm_strs);
	}
	
	int width, height;
	GetClientSize(&width, &height);
	
	panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	panel->Bind(wxEVT_MOTION, &CorrelogramFrame::OnMouseEvent, this);
		
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap
	
	panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_h_szr);
	
	// Top Sizer for Frame
	top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	
	SetSizer(top_h_sizer);
	DisplayStatusBar(true);
	
	local_hl_state->SetSize(par.bins);
    
	UpdateDataMapFromVarMan();
	UpdateCorrelogramData();
    
	SetupPanelForNumVariables(var_man.GetVarsCount());
	
    
	Show(true);
	
	wxCommandEvent ev;
	OnShowCorrelParams(ev);
	
	LOG_MSG("Exiting CorrelogramFrame::CorrelogramFrame");
}

CorrelogramFrame::~CorrelogramFrame()
{
	LOG_MSG("In CorrelogramFrame::~CorrelogramFrame");
	if (correl_params_frame) {
		correl_params_frame->removeObserver(this);
		correl_params_frame->closeAndDeleteWhenEmpty();
	}
	if (local_hl_state) local_hl_state->closeAndDeleteWhenEmpty();
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void CorrelogramFrame::OnMouseEvent(wxMouseEvent& event)
{
    if (event.RightUp()) {
        const wxPoint& pos = event.GetPosition();
    	// Workaround for right-click not changing window focus in OSX / wxW 3.0
    	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
    	OnActivate(ae);
    	
    	wxMenu* optMenu;
    	optMenu = wxXmlResource::Get()->LoadMenu("ID_CORRELOGRAM_MENU_OPTIONS");
    	if (!optMenu) return;
    	
    	UpdateContextMenuItems(optMenu);
    	PopupMenu(optMenu, pos);
    	UpdateOptionMenuItems();
    }
}

void CorrelogramFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In CorrelogramFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("CorrelogramFrame", GetTitle());
	}
	//if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void CorrelogramFrame::MapMenus()
{
	LOG_MSG("In CorrelogramFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->LoadMenu("ID_CORRELOGRAM_MENU_OPTIONS");
	CorrelogramFrame::UpdateContextMenuItems(optMenu);

	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void CorrelogramFrame::UpdateOptionMenuItems()
{
	//TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
	if (menu == wxNOT_FOUND) {
		LOG_MSG("CorrelogramFrame::UpdateOptionMenuItems: Options "
						"menu not found");
	} else {
		CorrelogramFrame::UpdateContextMenuItems(mb->GetMenu(menu));
	}
}

void CorrelogramFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void CorrelogramFrame::OnShowCorrelParams(wxCommandEvent& event)
{
	LOG_MSG("In CorrelogramFrame::OnShowCorrelParams");
	if (correl_params_frame) {
		correl_params_frame->Iconize(false);
		correl_params_frame->Raise();
		correl_params_frame->SetFocus();
	} else {
		CorrelParams cp;
		cp.dist_metric = project->GetDefaultDistMetric();
		cp.dist_units = project->GetDefaultDistUnits();
		correl_params_frame = new CorrelParamsFrame(cp, var_man, project);
		correl_params_frame->registerObserver(this);
	}
}

void CorrelogramFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	LOG_MSG("In CorrelogramFrame::OnDisplayStatistics");
	UpdateOptionMenuItems();
}

/** Implementation of TableStateObserver interface */
void CorrelogramFrame::update(TableState* o)
{
	LOG_MSG("In CorrelogramFrame::update(TableState*)");
	if (correl_params_frame) correl_params_frame->UpdateFromTable();
}

/** Implementation of TimeStateObserver interface */
void CorrelogramFrame::update(TimeState* o)
{
	LOG_MSG("In CorrelogramFrame::update(TimeState* o)");
	
    
    
    bool has_time_var = false;
    for (data_map_type::iterator i=data_map.begin(); i!=data_map.end(); ++i) {
        if (i->second.size() > 1) {
            has_time_var = true;
            break;
        }
    }
    
    if (has_time_var) {
        var_man.UpdateGlobalTime(o->GetCurrTime());
        UpdateDataMapFromVarMan();
        UpdateCorrelogramData();
        SetupPanelForNumVariables(var_man.GetVarsCount());
        Refresh();
    }
}

void CorrelogramFrame::ReDraw()
{
	LOG_MSG("In CorrelogramFrame::update(CorrelParamsObservable*)");
	UpdateDataMapFromVarMan();
	UpdateCorrelogramData();
	SetupPanelForNumVariables(var_man.GetVarsCount());
	Refresh();
}


/** Implementation of CorrelParams interface */
void CorrelogramFrame::update(CorrelParamsObservable* o)
{
    LOG_MSG("In CorrelogramFrame::update(CorrelParamsObservable*)");
    par = o->GetCorrelParams();
    UpdateDataMapFromVarMan();
    UpdateCorrelogramData();
    SetupPanelForNumVariables(var_man.GetVarsCount());
    Refresh();
}

void CorrelogramFrame::notifyOfClosing(CorrelParamsObservable* o)
{
	correl_params_frame = 0;
    UpdateMessageWin();
}

void CorrelogramFrame::notifyNewHover(const std::vector<int>& hover_obs,
                                      int total_hover_obs)
{
	wxStatusBar* sb = GetStatusBar();
	if (!sb) return;
	
	wxString s;
	if (total_hover_obs > 0) {
		int id = hover_obs[0];
		s << "autocorrelation is " << cbins[id].corr_avg;
		s << " for obs within distance band ";
		s << cbins[id].dist_min << " to " << cbins[id].dist_max;
	}
	sb->SetStatusText(s);	
}

/** Implementation of SimpleScatterPlotCanvasCbInt interface */	
void CorrelogramFrame::notifyNewHistHover(const std::vector<int>& hover_obs,
                                          int total_hover_obs)
{
	wxStatusBar* sb = GetStatusBar();
	if (!sb) return;
	if (total_hover_obs == 0) {
		sb->SetStatusText("");
		return;
	}
	wxString s;
	int id = hover_obs[0];
	s << cbins[id].num_pairs << " pairs in distance band ";
	s << cbins[id].dist_min << " to " << cbins[id].dist_max;
	sb->SetStatusText(s);	
}

/** At this time only one variable is supported, but this could easily
 be changed in the future, so will leave the num_vars parameter. */
void CorrelogramFrame::SetupPanelForNumVariables(int num_vars)
{
	LOG_MSG("Entering CorrelogramFrame::SetupPanelForNumVariables");
	if (!panel || !bag_szr) return;
	LOG(num_vars);
	int num_top_rows = GenUtils::max<int>(1, num_vars);
	LOG(num_top_rows);
	int num_rows_total = num_top_rows + 2;
	if (message_win) {
		message_win->Unbind(wxEVT_MOTION, &CorrelogramFrame::OnMouseEvent, this);
		//if (bag_szr->GetItemCount() > 0) {
		//	bool detatch_success = bag_szr->Detach(0);
		//	LOG(detatch_success);
		//}
		message_win->Destroy();
		message_win = 0;
	}
	bag_szr->Clear();
	panel_v_szr->Remove(bag_szr); // bag_szr is deleted automatically
	bag_szr = new wxGridBagSizer(0, 0); // 0 vgap, 0 hgap
	for (size_t i=0, sz=scatt_plots.size(); i<sz; ++i) {
		if (scatt_plots[i]) {
			scatt_plots[i]->Unbind(wxEVT_MOTION, &CorrelogramFrame::OnMouseEvent,
														 this);
			scatt_plots[i]->Destroy();
		}
	}
	scatt_plots.clear();
	if (hist_plot) {
		hist_plot->Unbind(wxEVT_MOTION, &CorrelogramFrame::OnMouseEvent, this);
		hist_plot->Destroy();
	}
	hist_plot = 0;
	for (size_t i=0, sz=vert_labels.size(); i<sz; ++i) {
		if (vert_labels[i])
            vert_labels[i]->Destroy();
	}
	vert_labels.clear();
	for (size_t i=0, sz=horiz_labels.size(); i<sz; ++i) {
		if (horiz_labels[i])
            horiz_labels[i]->Destroy();
	}
	horiz_labels.clear();
	
	wxString type_str;
	if (par.method == CorrelParams::ALL_PAIRS ||
			par.method == CorrelParams::ALL_PAIRS_THRESH) {
		type_str = " - all pairs";
	} else if (par.method == CorrelParams::RAND_SAMP ||
						 par.method == CorrelParams::RAND_SAMP_THRESH) {
		type_str = " - random";
	}
	if (par.method == CorrelParams::ALL_PAIRS_THRESH ||
			par.method == CorrelParams::RAND_SAMP_THRESH) {
		type_str << ", cutoff: " << GenUtils::DblToStr(par.threshold, 4); 
	}
    
    bool valid_sampling = true;
	
	double freq_min = 0;
	double freq_max = 0;
	double dist_min = 0;
	double dist_max = 1;
	if (cbins.size() > 0) {
		dist_min = cbins[0].dist_min;
		dist_max = cbins[cbins.size()-1].dist_max;
	}
	std::vector<double> dist_vals(cbins.size());
	std::vector<double> freq_vals(cbins.size());
	for (size_t i=0; i<cbins.size(); ++i) {
		freq_vals[i] = (double) cbins[i].num_pairs;
		if (i == 0) {
			dist_vals[i] = cbins[i].dist_min;
		} else {
			dist_vals[i] = (double) cbins[i].dist_max;
		}
		if (freq_vals[i] > freq_max) freq_max = freq_vals[i];
	}
	if (num_vars < 1) {
		// add blank cell upper left-hand corner
		bag_szr->Add(50, 50, wxGBPosition(0, 0), wxGBSpan(1,1));
		message_win = new wxHtmlWindow(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1));
		message_win->Bind(wxEVT_RIGHT_UP, &CorrelogramFrame::OnMouseEvent, this);
        UpdateMessageWin();
		bag_szr->Add(message_win, wxGBPosition(0,1), wxGBSpan(1,1), wxEXPAND);
		SetTitle("Correlogram" + type_str);
	} else {
		for (int row=0; row<num_vars; ++row) {
			wxString nm=var_man.GetName(row);
			int tm=var_man.GetTime(row);
			double y_min = -1;
			double y_max = 1;
			if (row == 0) {
				SetTitle("Correlogram - " + var_man.GetNameWithTime(row) + type_str);
			}
            
            
            
			wxString title("Autocorr. of " + var_man.GetNameWithTime(row));
			std::vector<double> Y(cbins.size());
			for (size_t i=0; i<cbins.size(); ++i) {
				Y[i] = cbins[i].corr_avg;
                if (isnan(Y[i])) {
                    valid_sampling = false;
                }
			}
			
            
            
			AxisScale v_axs;
			v_axs.ticks = 5;
			v_axs.data_min = -1;
			v_axs.data_max = 1;
			v_axs.scale_min = v_axs.data_min;
			v_axs.scale_max = v_axs.data_max;
			v_axs.scale_range = v_axs.data_max-v_axs.data_min;
			v_axs.tic_inc = v_axs.scale_range / (double) (v_axs.ticks-1);
			v_axs.p = 1;
			v_axs.tics.resize(v_axs.ticks);
			v_axs.tics_str.resize(v_axs.ticks);
			v_axs.tics_str_show.resize(v_axs.ticks);
			for (int i=0; i<v_axs.ticks; ++i) {
				double d = i;
				v_axs.tics[i] = v_axs.data_min + d*v_axs.tic_inc;
				std::stringstream s;
				s << v_axs.tics[i];
				v_axs.tics_str[i] = s.str();
				v_axs.tics_str_show[i] = true;
			}

			SimpleAxisCanvas* sa_can = 0;
			{
				sa_can = new SimpleAxisCanvas(panel, this, project, 
											  local_hl_state,
											  Y, title, y_min, y_max,
											  false, // is horizontal ?
											  true, // show axes
											  false, // hide negative labels
											  false, // add auto padding min
											  false, // add auto padding max
											  -1, // number ticks
											  true, // force tick at min
											  true, // force tick at max
											  &v_axs,
                                              false, // non-standardized
											  wxDefaultPosition, 
											  wxSize(50, -1));
				bag_szr->Add(sa_can, wxGBPosition(row, 0), wxGBSpan(1,1), wxEXPAND);
				vert_labels.push_back(sa_can);
			}
			
			std::vector<double> X(cbins.size());
			for (size_t i=0; i<cbins.size(); ++i) {
				X[i] = cbins[i].dist_min + (cbins[i].dist_max - cbins[i].dist_min)/2.0;
			}
            
			SimpleScatterPlotCanvas* sp_can = 0;
			sp_can = new SimpleScatterPlotCanvas(panel, this, project,
												 local_hl_state, this,
												 X, Y, title, title,
												 dist_min, dist_max, 
												 y_min, y_max,
												 false, false, true,
												 "ID_CORRELOGRAM_MENU_OPTIONS",
												 false, // show axes 
												 true, // show horiz axis thru orig
												 false, // show vert axis thru orig
												 false, false, 
												 valid_sampling, // show LOWESS fit
												 false);
			sp_can->ChangeLoessParams(0.2,5,0.02);
			bag_szr->Add(sp_can, wxGBPosition(row, 1), wxGBSpan(1,1), wxEXPAND);
			scatt_plots.push_back(sp_can);
		}
	}

	{
		{
			AxisScale v_axs;
			v_axs.ticks = 4;
			v_axs.data_min = freq_min;
			v_axs.data_max = freq_max;
			v_axs.scale_min = v_axs.data_min;
			v_axs.scale_max = v_axs.data_max;
			v_axs.scale_range = v_axs.data_max-v_axs.data_min;
			v_axs.tic_inc = v_axs.scale_range / (double) (v_axs.ticks-1);
			v_axs.p = 1;
			v_axs.tics.resize(v_axs.ticks);
			v_axs.tics_str.resize(v_axs.ticks);
			v_axs.tics_str_show.resize(v_axs.ticks);
			for (int i=0; i<v_axs.ticks; ++i) {
				double d = i;
				v_axs.tics[i] = v_axs.data_min + d*v_axs.tic_inc;
				std::stringstream s;
				// round to nearest whole number
				s << setprecision(0) << fixed << v_axs.tics[i];
				v_axs.tics_str[i] = s.str();
				v_axs.tics_str_show[i] = true;
			}
			SimpleAxisCanvas* sa_can = 0;
			{
				sa_can = new SimpleAxisCanvas(panel, this, project, 
											  local_hl_state,
											  freq_vals, "Frequency",
											  freq_min, freq_max,
											  false, // is horizontal ?
											  true, // show axes
											  false, // hide negative labels
											  false, // add auto padding min
											  false, // add auto padding max
											  -1, // number ticks
											  true, // force tick at min
											  true, // force tick at max
											  &v_axs,
                                              false, // non-standardized
											  wxDefaultPosition, 
											  wxSize(50, -1));
				bag_szr->Add(sa_can, wxGBPosition(num_top_rows, 0),
										 wxGBSpan(1,1), wxEXPAND);
				vert_labels.push_back(sa_can);
			}
			
			SimpleBinsHistCanvas* sh_can = 0;
			std::vector<SimpleBin> hist_bins(cbins.size());
			for (size_t i=0; i<cbins.size(); ++i) {
				hist_bins[i].min = cbins[i].dist_min;
				hist_bins[i].max = cbins[i].dist_max;
				hist_bins[i].count = cbins[i].num_pairs;
			}
			sh_can = new SimpleBinsHistCanvas(panel, this, project, 
											  local_hl_state,
											  this,
											  hist_bins, "", dist_min, dist_max,
											  "ID_CORRELOGRAM_MENU_OPTIONS",
											  false);
			bag_szr->Add(sh_can, wxGBPosition(num_top_rows, 1),
									 wxGBSpan(1,1), wxEXPAND);
			hist_plot = sh_can;
		}
		
		// add blank cell lower left-hand corner
		bag_szr->Add(50, 50, wxGBPosition(num_top_rows+1, 0), wxGBSpan(1,1));
		
		wxString title;
		if (par.dist_metric == WeightsMetaInfo::DM_arc) {
			title << "Arc Distance, ";
			if (par.dist_units == WeightsMetaInfo::DU_mile) {
				title << " mi";
			} else {
				title << " km";
			}
		} else {
			title << "Euclidean Distance";
		}
		
		AxisScale h_axs;
		h_axs.ticks = cbins.size()+1;
		h_axs.data_min = cbins[0].dist_min;
		h_axs.data_max = cbins[cbins.size()-1].dist_max;
		h_axs.scale_min = h_axs.data_min;
		h_axs.scale_max = h_axs.data_max;
		h_axs.scale_range = h_axs.data_max-h_axs.data_min;
		h_axs.tic_inc = h_axs.scale_range / (double) (h_axs.ticks-1);
		h_axs.p = 1;
		h_axs.tics.resize(h_axs.ticks);
		h_axs.tics_str.resize(h_axs.ticks);
		h_axs.tics_str_show.resize(h_axs.ticks);
		for (int i=0; i<h_axs.ticks; ++i) {
			double d = i;
			h_axs.tics[i] = h_axs.data_min + d*h_axs.tic_inc;
			stringstream ss;
			if (h_axs.tics[i] < 10000000) {
				ss << std::fixed << std::setprecision(1) << h_axs.tics[i];
				h_axs.tics_str[i] = ss.str();
			} else {
				ss << std::setprecision(1) << h_axs.tics[i];
				h_axs.tics_str[i] = ss.str();
			}
			h_axs.tics_str_show[i] = true;
		}
		
		SimpleAxisCanvas* sa_can = 0;
		sa_can = new SimpleAxisCanvas(panel, this, project, local_hl_state,
									dist_vals, title, dist_min, dist_max,
									true, // is horizontal ?
									true, // show axes
									false, // hide negative labels
									false, // add auto padding min
									false, // add auto padding max
									cbins.size(), // number ticks,
									true, // force tick at min
									true, // force tick at max
									&h_axs, // custom axis scale pointer
                                    false, // non-standardized
									wxDefaultPosition, wxSize(-1, 50));

		bag_szr->Add(sa_can, wxGBPosition(num_top_rows+1, 1),
								 wxGBSpan(1,1), wxEXPAND);
		horiz_labels.push_back(sa_can);
			
	}
    
	
	bag_szr->SetFlexibleDirection(wxBOTH);
	// first column
	if (bag_szr->IsColGrowable(0)) bag_szr->RemoveGrowableCol(0);
	// final row
	if (bag_szr->IsRowGrowable(num_rows_total)) {
		bag_szr->RemoveGrowableRow(num_rows_total);
	}
	
	// second column
	if (bag_szr->IsColGrowable(1)) bag_szr->RemoveGrowableCol(1);
	bag_szr->AddGrowableCol(1, 1);

	// all rows exluding last two
	for (int i=0; i<num_top_rows; ++i) {
		if (bag_szr->IsRowGrowable(i)) bag_szr->RemoveGrowableRow(i);
		bag_szr->AddGrowableRow(i, 2);
	}

	// second-to-last row
	if (bag_szr->IsRowGrowable(num_top_rows)) {
		bag_szr->RemoveGrowableRow(num_top_rows);
	}
	bag_szr->AddGrowableRow(num_top_rows, 1);
	
	panel_v_szr->Add(bag_szr, 1, wxEXPAND);
	LOG(bag_szr->GetItemCount());
	top_h_sizer->RecalcSizes();
    
    if (valid_sampling == false ) {
        wxString msg = "The sample size for random sampling is too small.\nPlease increase the number of iterations.";
        wxString title = "Insufficient Random Sampling";
        wxMessageDialog dlg (this, msg, title, wxOK | wxICON_WARNING);
        dlg.ShowModal();
    }
	LOG_MSG("Exiting CorrelogramFrame::SetupPanelForNumVariables");
}

void CorrelogramFrame::UpdateMessageWin()
{
	if (!message_win) return;
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "  <style type=\"text/css\">";	
	s << "  body {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    font-size: small;";
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
	s << "<font face=\"verdana,arial,sans-serif\" color=\"black\" size=\"4\">";
	
	int count = var_man.GetVarsCount();
	if (count == 0) {
		s << "Please right-click or use<br />";
		s << "<font color=\"blue\">Options > Change Parameters<br /></font>";
		s << "to specify variable and distance parameters.";
	} else if (count > 0) {
		s << "Variables specified: <br />";
		for (int i=0; i<count; ++i) {
			s << var_man.GetName(i);
			if (i+1 < count) s << "<br />";
		}
	}
	
	s << "  </font></p></center>";
	s << "</body>";
	s << "</html>";
	message_win->SetPage(s );
}

/** Adds/removes variables from data_map according to variables present
 in var_man. */
void CorrelogramFrame::UpdateDataMapFromVarMan()
{
	LOG_MSG("Entering CorrelogramFrame::UpdateDataMapFromVarMan");
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
	
	LOG_MSG("Exiting CorrelogramFrame::UpdateDataMapFromVarMan");
}

/** Update histogram and update correlogram if data_map has at least
 one variable */
bool CorrelogramFrame::UpdateCorrelogramData()
{
	using namespace GenGeomAlgs;
	using namespace CorrelogramAlgs;
	bool success = false;
	std::vector<double> Z;
	if (var_man.GetVarsCount() > 0) {
		wxString nm = var_man.GetName(0);
		int tm = var_man.GetTime(0);
		wxString title(var_man.GetNameWithTime(0));
		const std::vector<double>& data(data_map[nm][tm]);
		Z.resize(data.size());
		for (size_t i=0, sz=data.size(); i<sz; ++i) Z[i] = data[i];
	}
	bool is_arc = par.dist_metric == WeightsMetaInfo::DM_arc;
	bool is_mi = par.dist_units == WeightsMetaInfo::DU_mile;
	double th_rad = par.threshold;
	if (is_arc) {
		th_rad = is_mi ? EarthMiToRad(par.threshold) : EarthKmToRad(par.threshold);
	}

	std::vector<wxRealPoint> pts;
	project->GetCentroids(pts);

	if (par.method == CorrelParams::ALL_PAIRS) {	
		success = MakeCorrAllPairs(pts, Z, is_arc, par.bins, cbins);
	
	} else if (par.method == CorrelParams::ALL_PAIRS_THRESH) {
		if (is_arc) {
			success = MakeCorrThresh(project->GetUnitSphereRtree(), Z, th_rad, par.bins, cbins);
		} else {
			success = MakeCorrThresh(project->GetEucPlaneRtree(), Z,  par.threshold, par.bins, cbins);
		}
	} else if (par.method == CorrelParams::RAND_SAMP) {
		success = MakeCorrRandSamp(pts, Z, is_arc, -1, par.bins,  par.max_iterations, cbins);
	}	else if (par.method == CorrelParams::RAND_SAMP_THRESH) {
		success = MakeCorrRandSamp(pts, Z, is_arc, (is_arc ? th_rad : par.threshold), par.bins,  par.max_iterations, cbins);
	}
    
    if (success == false) {
        wxString msg = "Please select another variable with values more suitable for computing a correlogram.";
        wxString title = "Variable Value Error";
        wxMessageDialog dlg (this, msg, title, wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return success;
    }
    
	if (success && par.bins != local_hl_state->GetHighlightSize()) {
		local_hl_state->SetSize(par.bins);
	}
	
	if (success && is_arc) {
		for (size_t i=0; i<cbins.size(); ++i) {
			if (is_mi) {
				cbins[i].dist_min = EarthRadToMi(cbins[i].dist_min);
				cbins[i].dist_max = EarthRadToMi(cbins[i].dist_max);
			} else {
				cbins[i].dist_min = EarthRadToKm(cbins[i].dist_min);
				cbins[i].dist_max = EarthRadToKm(cbins[i].dist_max);
			}
		}
	}
	
	return success;
}

