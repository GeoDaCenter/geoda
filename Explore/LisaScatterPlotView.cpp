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

#include <algorithm>
#include <cmath>
#include <vector>
#include <wx/splitter.h>
#include <wx/textdlg.h>
#include <wx/xrc/xmlres.h>
#include <boost/foreach.hpp>
#include <boost/multi_array.hpp>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../VarTools.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/RandomizationDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "LisaCoordinator.h"
#include "LisaScatterPlotView.h"

const int ID_RANDDLG = wxID_ANY;

IMPLEMENT_CLASS(LisaScatterPlotCanvas, ScatterNewPlotCanvas)
BEGIN_EVENT_TABLE(LisaScatterPlotCanvas, ScatterNewPlotCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

LisaScatterPlotCanvas::LisaScatterPlotCanvas(wxWindow *parent,
											 TemplateFrame* t_frame,
											 Project* project,
											 LisaCoordinator* lisa_coordinator,
											 const wxPoint& pos,
											 const wxSize& size)
: ScatterNewPlotCanvas(parent, t_frame, project, pos, size),
lisa_coord(lisa_coordinator),
is_bi(lisa_coordinator->lisa_type == LisaCoordinator::bivariate),
is_rate(lisa_coordinator->lisa_type == LisaCoordinator::eb_rate_standardized),
is_diff(lisa_coordinator->lisa_type == LisaCoordinator::differential),
rand_dlg(0)
{
	LOG_MSG("Entering LisaScatterPlotCanvas::LisaMapCanvas");
	
	fixed_aspect_ratio_mode = true;
	show_reg_selected = false;
	show_reg_excluded = false;
	
	// must set var_info from LisaCoordinator initially in order to get
	// intial times for each variable.
	sp_var_info.resize(2);
	var_info = lisa_coord->var_info;
	var_info_orig = var_info;
    
	SyncVarInfoFromCoordinator();

    wxColour default_cat_color = GdaConst::scatterplot_regression_excluded_color;
	cat_data.CreateCategoriesAllCanvasTms(1, 1, num_obs);
	cat_data.SetCategoryColor(0, 0, default_cat_color);
    for (int i=0; i<num_obs; i++) {
        cat_data.AppendIdToCategory(0, 0, i);
    }
    
	// For LisaScatterPlot, all time steps have the exact same trivial categorization.
	cat_data.SetCurrentCanvasTmStep(0);
	
	PopulateCanvas();
	
	UpdateDisplayLinesAndMargins();
	ResizeSelectableShps();	

	LOG_MSG("Exiting LisaScatterPlotCanvas::LisaMapCanvas");
}

LisaScatterPlotCanvas::~LisaScatterPlotCanvas()
{
	LOG_MSG("Entering LisaScatterPlotCanvas::~LisaScatterPlotCanvas");
    if (rand_dlg) {
        rand_dlg->Destroy();
    }
	LOG_MSG("Exiting LisaScatterPlotCanvas::~LisaScatterPlotCanvas");
}

void LisaScatterPlotCanvas::OnRandDlgClose( wxWindowDestroyEvent& event)
{
    rand_dlg = 0;
}
            
void LisaScatterPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering LisaScatterPlotCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((LisaScatterPlotFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISA_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting LisaScatterPlotCanvas::DisplayRightClickMenu");
}

void LisaScatterPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant && (i==0 || is_bi || is_rate)) {
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
			menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}

    menu->AppendSeparator();
    menu->Append(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}

wxString LisaScatterPlotCanvas::GetCanvasTitle()
{
	wxString s;
	wxString v0(var_info_orig[0].name);
	if (var_info_orig[0].is_time_variant) {
		v0 << " (" << project->GetTableInt()->
			GetTimeString(var_info_orig[0].time);
		v0 << ")";
	}
	wxString v1;
	if (is_bi || is_rate || is_diff) {
		v1 << var_info_orig[1].name;
		if (var_info_orig[1].is_time_variant) {
			v1 << " (" << project->GetTableInt()->
				GetTimeString(var_info_orig[1].time);
			v1 << ")";
		}
	}	
	wxString w(lisa_coord->weight_name);
	if (is_bi) {
		s << "Bivariate Moran's I (" << w << "): ";
		s << v0 << " and lagged " << v1;
	} else if (is_rate) {
		s << "Emp Bayes Rate Std Moran's I (" << w << "): ";
		s << v0 << " / " << v1;
    } else if (is_diff) {
        s << "Differential Moran's I (" << w << "): " << v0 << " - " << v1;
    } else {
		s << "Moran's I (" << w << "): " << v0;
	}
	return s;
}


/** This virtual function will be called by the Scatter Plot base class
 to determine x and y axis labels. */
wxString LisaScatterPlotCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	// Different behaviour depending on whether univariate, bivariate or
	// rate-adjusted.
		
	wxString v0(var_info_orig[0].name);
	if (var_info_orig[0].is_time_variant) {
		v0 << " (" << project->GetTableInt()->
			GetTimeString(var_info_orig[0].time);
		v0 << ")";
	}
	wxString v1;
	if (is_bi || is_rate || is_diff) {
		v1 << var_info_orig[1].name;
		if (var_info_orig[1].is_time_variant) {
			v1 << " (" << project->GetTableInt()->
				GetTimeString(var_info_orig[1].time);
			v1 << ")";
		}
	}
	wxString s0;
	wxString s1;
	if (is_diff) {
        s0 << v0 << " - " << v1;
	} else if (is_rate) {
		s0 << v0 << " / " << v1;
    } else {
        s0 << v0;
    }
	if (is_bi) {
		s1 << v1;
	} else if (is_rate) {
		s1 << v0 << " / " << v1;
    } else if (is_diff) {
    	s1 << v0 << " - " << v1;
	} else {
		s1 << v0;
	}
	
	if (var == 1) return "lagged " + s1;
	return s0;
}

void LisaScatterPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	ScatterNewPlotCanvas::SetCheckMarks(menu);
    
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_SPECIFIED_SEED"),
								  lisa_coord->IsReuseLastSeed());
}

void LisaScatterPlotCanvas::TimeChange()
{
	LOG_MSG("Entering LisaScatterPlotCanvas::TimeChange");
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetTimeState()->GetCurrTime();
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
		(cts < ref_time_min && ref_time == ref_time_min)) return;
	if (cts > ref_time_max) {
		ref_time = ref_time_max;
	} else if (cts < ref_time_min) {
		ref_time = ref_time_min;
	} else {
		ref_time = cts;
	}
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	var_info_orig = var_info;
	sp_var_info[0].time = var_info[ref_var_index].time;
	sp_var_info[1].time = var_info[ref_var_index].time;
	//SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting LisaScatterPlotCanvas::TimeChange");
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void LisaScatterPlotCanvas::SyncVarInfoFromCoordinator()
{
	using namespace boost;
    
	std::vector<int>my_times(var_info.size());
    
    for (int t=0; t<var_info.size(); t++) {
        my_times[t] = var_info[t].time;
    }
    
	var_info = lisa_coord->var_info;
	template_frame->ClearAllGroupDependencies();
    
	for (int t=0; t<var_info.size(); t++) {
		var_info[t].time = my_times[t];
		template_frame->AddGroupDependancy(var_info[t].name);
	}
    
	is_any_time_variant = lisa_coord->is_any_time_variant;
	is_any_sync_with_global_time = lisa_coord->is_any_sync_with_global_time;
	ref_var_index = lisa_coord->ref_var_index;
	num_time_vals = lisa_coord->num_time_vals;
	
	//Now, we need to also update sp_var_info appropriately.
	// both sp_var_info objects will have the same range of time values.
	// we should base them off of the reference time value, or else
	// just variable 1.
	int t_ind = (ref_var_index == -1) ? 0 : ref_var_index;
	sp_var_info[0] = var_info[t_ind];
	sp_var_info[1] = var_info[t_ind];

    int num_time_vals = lisa_coord->num_time_vals;
    int num_obs = lisa_coord->num_obs;
    
	x_data.resize(extents[num_time_vals][num_obs]);
	y_data.resize(extents[num_time_vals][num_obs]);
	x_undef_data.resize(extents[num_time_vals][num_obs]);
	y_undef_data.resize(extents[num_time_vals][num_obs]);
    
	for (int t=0; t<lisa_coord->num_time_vals; t++) {
        double x_min, x_max, y_min, y_max;
        
        bool has_init = false;
        
		for (int i=0; i<lisa_coord->num_obs; i++) {
            
			x_data[t][i] = lisa_coord->data1_vecs[t][i];
			y_data[t][i] = lisa_coord->lags_vecs[t][i];
            x_undef_data[t][i] = lisa_coord->undef_data[0][t][i];
            y_undef_data[t][i] = lisa_coord->undef_data[0][t][i];
            
			if (x_undef_data[t][i] || y_undef_data[t][i])
                continue;
            
            if (!has_init) {
                x_min = x_data[t][i];
                x_max = x_data[t][i];
                y_min = y_data[t][i];
                y_max = y_data[t][i];
                has_init = true;
            } else {
                if (x_data[t][i] < x_min) {
                    x_min = x_data[t][i];
                }
                if (x_data[t][i] > x_max) {
                    x_max = x_data[t][i];
                }
                if (y_data[t][i] < y_min) {
                    y_min = y_data[t][i];
                }
                if (y_data[t][i] > y_max) {
                    y_max = y_data[t][i];
                }
            }
		}
        
        // if no valid data, should raise an exception
        
		double mag = std::max(std::max(fabs(x_min), fabs(x_max)),
							  std::max(fabs(y_min), fabs(y_max)));
       
		sp_var_info[0].min[sp_var_info[0].time_min+t] = -mag;
		sp_var_info[0].max[sp_var_info[0].time_min+t] = mag;
		sp_var_info[1].min[sp_var_info[1].time_min+t] = -mag;
		sp_var_info[1].max[sp_var_info[1].time_min+t] = mag;
	}
    
	for (int i=0; i<sp_var_info.size(); i++) {
		sp_var_info[i].min_over_time =
			sp_var_info[i].min[sp_var_info[i].time_min];
        
		sp_var_info[i].max_over_time =
			sp_var_info[i].max[sp_var_info[i].time_min];
        
		for (int t=sp_var_info[i].time_min; t<=sp_var_info[i].time_max; t++) {
			if (sp_var_info[i].min[t] < sp_var_info[i].min_over_time) {
				sp_var_info[i].min_over_time = sp_var_info[i].min[t];
			}
			if (sp_var_info[i].max[t] > sp_var_info[i].max_over_time) {
				sp_var_info[i].max_over_time = sp_var_info[i].max[t];
			}
		}
	}
}

void LisaScatterPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In LisaScatterPlotCanvas::TimeSyncVariableToggle");
	lisa_coord->var_info[var_index].sync_with_global_time =
		!lisa_coord->var_info[var_index].sync_with_global_time;
	lisa_coord->var_info[0].time = var_info[0].time;
	if (is_bi || is_rate) {
		lisa_coord->var_info[1].time = var_info[1].time;
	}
	lisa_coord->VarInfoAttributeChange();
	lisa_coord->InitFromVarInfo();
	lisa_coord->notifyObservers();
}

void LisaScatterPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In LisaScatterPlotCanvas::FixedScaleVariableToggle");
	// Not implemented for now.
	//lisa_coord->var_info[var_index].fixed_scale =
	//	!lisa_coord->var_info[var_index].fixed_scale;
	//lisa_coord->var_info[0].time = var_info[0].time;
	//if (is_bi || is_rate) {
	//	lisa_coord->var_info[1].time = var_info[1].time;
	//}
	//lisa_coord->VarInfoAttributeChange();
	//lisa_coord->notifyObservers();
}

void LisaScatterPlotCanvas::PopulateCanvas()
{
	LOG_MSG("Entering LisaScatterPlotCanvas::PopulateCanvas");
	
	// need to modify var_info temporarily for PopulateCanvas since
	var_info_orig = var_info;
	var_info = sp_var_info;
	ScatterNewPlotCanvas::PopulateCanvas();
	var_info = var_info_orig;
	
	LOG_MSG("Exiting LisaScatterPlotCanvas::PopulateCanvas");
}

void LisaScatterPlotCanvas::PopCanvPreResizeShpsHook()
{
	wxString s("Moran's I: ");
	s << regressionXY.beta;
    GdaShapeText* morans_i_text = new GdaShapeText(s, *GdaConst::small_font,
                                                   wxRealPoint(50, 100), 0,
                                                   GdaShapeText::h_center,
                                                   GdaShapeText::v_center,
                                                   0, -15);

    morans_i_text->setPen(*GdaConst::scatterplot_reg_pen);
	foreground_shps.push_back(morans_i_text);
}

void LisaScatterPlotCanvas::ShowRandomizationDialog(int permutation)
{
    if (permutation < 9) {
        permutation = 9;
    } else if (permutation > 99999) {
        permutation = 99999;
    }
   
	int cts = project->GetTimeState()->GetCurrTime();
    
	std::vector<double> raw_data1(num_obs);
    
	int xt = var_info_orig[0].time-var_info_orig[0].time_min;
	for (int i=0; i<num_obs; i++) {
		raw_data1[i] = lisa_coord->data1_vecs[xt][i];
	}
   
    const GalElement* W = lisa_coord->Gal_vecs[cts]->gal;
    bool reuse_last_seed = lisa_coord->IsReuseLastSeed();
    uint64_t last_used_seed = lisa_coord->GetLastUsedSeed();
    
	if (is_bi) {
		std::vector<double> raw_data2(num_obs);
		int yt = var_info_orig[1].time-var_info_orig[1].time_min;
		for (int i=0; i<num_obs; i++) {
			raw_data2[i] = lisa_coord->data2_vecs[yt][i];
		}

        if (rand_dlg != 0) {
            rand_dlg->Destroy();
            rand_dlg = 0;
        }
        // here W handles undefined
        rand_dlg = new RandomizationDlg(raw_data1, raw_data2,
                                        W, permutation,
                                        reuse_last_seed,
                                        last_used_seed, this);
		
        rand_dlg->Connect(wxEVT_DESTROY,
                          wxWindowDestroyEventHandler(LisaScatterPlotCanvas::OnRandDlgClose),
                          NULL, this);
        rand_dlg->Show(true);
        
	} else {
        if (rand_dlg != 0) {
            rand_dlg->Destroy();
            rand_dlg = 0;
        }
        rand_dlg = new RandomizationDlg(raw_data1, W, permutation,
                                        reuse_last_seed,
                                        last_used_seed, this);
        rand_dlg->Connect(wxEVT_DESTROY,
                          wxWindowDestroyEventHandler(LisaScatterPlotCanvas::OnRandDlgClose),
                          NULL, this);
		rand_dlg->Show(true);
	}
}

void LisaScatterPlotCanvas::SaveMoranI()
{
	wxString title = "Save Results: Moran's I";
	std::vector<double> std_data(num_obs);
	std::vector<double> lag(num_obs);
    std::vector<double> diff(num_obs);

    
	int xt = sp_var_info[0].time-sp_var_info[0].time_min;
	int yt = sp_var_info[1].time-sp_var_info[1].time_min;


	for (int i=0; i<num_obs; i++) {
		std_data[i] = x_data[xt][i];
		lag[i] = y_data[yt][i];
   
        if (is_diff ) {
            int t0 =  lisa_coord->var_info[0].time;
            int t1 =  lisa_coord->var_info[1].time;
            diff[i] = lisa_coord->data[0][t0][i] - lisa_coord->data[0][t1][i];
        }
	}
    
    std::vector<SaveToTableEntry> data;
    
    if (is_diff) {
        data.resize(3);
    } else {
        data.resize(2);
    }
    
	data[0].d_val = &std_data;
	data[0].label = "Standardized Data";
	data[0].field_default = "MORAN_STD";
	data[0].type = GdaConst::double_type;
    data[0].undefined = &XYZ_undef;
    
	data[1].d_val = &lag;
	data[1].label = "Spatial Lag";
	data[1].field_default = "MORAN_LAG";
	data[1].type = GdaConst::double_type;
    data[1].undefined = &XYZ_undef;
    
    if (is_diff) {
        data[2].d_val = &diff;
        data[2].label = "Diff Values";
        data[2].field_default = "DIFF_VAL";
        data[2].type = GdaConst::double_type;
        data[2].undefined = &XYZ_undef;
    }
	
	SaveToTableDlg dlg(project, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

IMPLEMENT_CLASS(LisaScatterPlotFrame, ScatterNewPlotFrame)
BEGIN_EVENT_TABLE(LisaScatterPlotFrame, ScatterNewPlotFrame)
	EVT_ACTIVATE(LisaScatterPlotFrame::OnActivate)
END_EVENT_TABLE()

LisaScatterPlotFrame::LisaScatterPlotFrame(wxFrame *parent, Project* project,
										   LisaCoordinator* lisa_coordinator,
										   const wxPoint& pos,
										   const wxSize& size,
                                           const long style)
: ScatterNewPlotFrame(parent, project, pos, size, style),
lisa_coord(lisa_coordinator)
{
	LOG_MSG("Entering LisaScatterPlotFrame::LisaScatterPlotFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	template_canvas = new LisaScatterPlotCanvas(this, this, project,
												lisa_coord,
												wxDefaultPosition,
												wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	lisa_coord->registerObserver(this);
	Show(true);
	
	LOG_MSG("Exiting LisaScatterPlotFrame::LisaScatterPlotFrame");
}

LisaScatterPlotFrame::~LisaScatterPlotFrame()
{
	LOG_MSG("In LisaScatterPlotFrame::~LisaScatterPlotFrame");
	if (lisa_coord) {
		lisa_coord->removeObserver(this);
		lisa_coord = 0;
	}
}

void LisaScatterPlotFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
	lisa_coord->SetReuseLastSeed(!lisa_coord->IsReuseLastSeed());
}

void LisaScatterPlotFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
	uint64_t last_seed = lisa_coord->GetLastUsedSeed();
	wxString m;
	m << "The last seed used by the pseudo random\nnumber ";
	m << "generator was " << last_seed << ".\n";
	m << "Enter a seed value to use between\n0 and ";
	m << std::numeric_limits<uint64_t>::max() << ".";
	long long unsigned int val;
	wxString dlg_val;
	wxString cur_val;
	cur_val << last_seed;
	
	wxTextEntryDialog dlg(NULL, m, "Enter a seed value", cur_val);
	if (dlg.ShowModal() != wxID_OK) return;
	dlg_val = dlg.GetValue();
	dlg_val.Trim(true);
	dlg_val.Trim(false);
	if (dlg_val.IsEmpty()) return;
	if (dlg_val.ToULongLong(&val)) {
		if (!lisa_coord->IsReuseLastSeed()) lisa_coord->SetLastUsedSeed(true);
		uint64_t new_seed_val = val;
		lisa_coord->SetLastUsedSeed(new_seed_val);
	} else {
		wxString m;
		m << "\"" << dlg_val << "\" is not a valid seed. Seed unchanged.";
		wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void LisaScatterPlotFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In LisaScatterPlotFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("LisaScatterPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void LisaScatterPlotFrame::MapMenus()
{
	LOG_MSG("In LisaScatterPlotFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISA_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	((ScatterNewPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((ScatterNewPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
		GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void LisaScatterPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("LisaScatterPlotFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((LisaScatterPlotCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void LisaScatterPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LisaScatterPlotFrame::RanXPer(int perm)
{
	((LisaScatterPlotCanvas*) template_canvas)->ShowRandomizationDialog(perm);
}

void LisaScatterPlotFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void LisaScatterPlotFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void LisaScatterPlotFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void LisaScatterPlotFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void LisaScatterPlotFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		RanXPer(num);
	}
}

void LisaScatterPlotFrame::OnSaveMoranI(wxCommandEvent& event)
{
	LisaScatterPlotCanvas* lc = (LisaScatterPlotCanvas*) template_canvas;
	lc->SaveMoranI();
}

/** Called by LisaCoordinator to notify that state has changed.  State changes
 can include:
 - variable sync change and therefore all lisa categories have changed
 - significance level has changed and therefore categories have changed
 - new randomization for p-vals and therefore categories have changed */
void LisaScatterPlotFrame::update(LisaCoordinator* o)
{
	LisaScatterPlotCanvas* lc = (LisaScatterPlotCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
}

void LisaScatterPlotFrame::closeObserver(LisaCoordinator* o)
{
	LOG_MSG("In LisaScatterPlotFrame::closeObserver(LisaCoordinator*)");
	if (lisa_coord) {
		lisa_coord->removeObserver(this);
		lisa_coord = 0;
	}
	Close(true);
}


