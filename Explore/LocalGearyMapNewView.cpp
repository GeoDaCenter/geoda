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

#include <limits>
#include <vector>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "ConditionalClusterMapView.h"
#include "LocalGearyCoordinator.h"
#include "LocalGearyMapNewView.h"
#include "../ShpFile.h"

IMPLEMENT_CLASS(LocalGearyMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(LocalGearyMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

using namespace std;

LocalGearyMapCanvas::LocalGearyMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project,
                             LocalGearyCoordinator* local_geary_coordinator,
                             CatClassification::CatClassifType theme_type_s,
                             bool isBivariate, bool isEBRate,
                             const wxPoint& pos, const wxSize& size)
:MapCanvas(parent, t_frame, project,
           vector<GdaVarTools::VarInfo>(0), vector<int>(0),
           CatClassification::no_theme,
           no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
local_geary_coord(local_geary_coordinator),
is_clust(theme_type_s==CatClassification::local_geary_categories),
is_bi(isBivariate),
is_rate(isEBRate),
is_diff(local_geary_coordinator->local_geary_type == LocalGearyCoordinator::differential)
{
	LOG_MSG("Entering LocalGearyMapCanvas::LocalGearyMapCanvas");

	cat_classif_def.cat_classif_type = theme_type_s;
	// must set var_info times from LocalGearyCoordinator initially
	var_info = local_geary_coordinator->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0, sz=var_info.size(); t<sz; ++t) {
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	CreateAndUpdateCategories();
	
	LOG_MSG("Exiting LocalGearyMapCanvas::LocalGearyMapCanvas");
}

LocalGearyMapCanvas::~LocalGearyMapCanvas()
{
	LOG_MSG("In LocalGearyMapCanvas::~LocalGearyMapCanvas");
}

void LocalGearyMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering LocalGearyMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((LocalGearyMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISAMAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting LocalGearyMapCanvas::DisplayRightClickMenu");
}

wxString LocalGearyMapCanvas::GetCanvasTitle()
{
	wxString local_geary_t;
	if (is_clust && !is_bi) local_geary_t = " LocalGeary Cluster Map";
	if (is_clust && is_bi) local_geary_t = " BiLocalGeary Cluster Map";
    if (is_clust && is_diff) local_geary_t = " Differential LocalGeary Cluster Map";
    
	if (!is_clust && !is_bi) local_geary_t = " LocalGeary Significance Map";
	if (!is_clust && is_bi) local_geary_t = " BiLocalGeary Significance Map";
    if (!is_clust && is_diff) local_geary_t = " Differential Significance Map";
	
	wxString field_t;
	if (is_bi) {
		field_t << GetNameWithTime(0) << " w/ " << GetNameWithTime(1);
    } else if (is_diff) {
        field_t << GetNameWithTime(0) << " - " << GetNameWithTime(1);
    } else if (local_geary_coord->local_geary_type == LocalGearyCoordinator::multivariate) {
        for (int i=0; i<local_geary_coord->num_vars; i++) {
            field_t << GetNameWithTime(i);
            if (i < local_geary_coord->num_vars -1 )
                field_t << "/";
        }
    
    } else {
		field_t << "C_" << GetNameWithTime(0);
	}
	if (is_rate) {
		field_t << "EB Rate: " << GetNameWithTime(0);
		field_t << " / " << GetNameWithTime(1);
	}
	
	wxString ret;
	ret << local_geary_t << ": " << local_geary_coord->weight_name << ", ";
	ret << field_t << " (" << local_geary_coord->permutations << " perm)";
	return ret;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by LocalGeary maps */
bool
LocalGearyMapCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme,
                             SmoothingType new_map_smoothing)
{
	LOG_MSG("In LocalGearyMapCanvas::ChangeMapType");
	return false;
}

void LocalGearyMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	MapCanvas::SetCheckMarks(menu);
	
	int sig_filter = local_geary_coord->GetSignificanceFilter();
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
								  sig_filter == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
								  sig_filter == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
								  sig_filter == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
								  sig_filter == 4);
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_SPECIFIED_SEED"),
								  local_geary_coord->IsReuseLastSeed());
}

void LocalGearyMapCanvas::TimeChange()
{
	LOG_MSG("Entering LocalGearyMapCanvas::TimeChange");
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
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting LocalGearyMapCanvas::TimeChange");
}

/** Update Categories based on info in LocalGearyCoordinator */
void LocalGearyMapCanvas::CreateAndUpdateCategories()
{
	SyncVarInfoFromCoordinator();
	cat_data.CreateEmptyCategories(num_time_vals, num_obs);
	
	for (int t=0; t<num_time_vals; t++) {
		if (!map_valid[t]) break;
		
		int undefined_cat = -1;
		int isolates_cat = -1;
		int num_cats = 0;
        double stop_sig = 0;
        
		if (local_geary_coord->GetHasIsolates(t))
            num_cats++;
		if (local_geary_coord->GetHasUndefined(t))
            num_cats++;
		if (is_clust) {
            if (local_geary_coord->local_geary_type == LocalGearyCoordinator::multivariate) {
                num_cats += 2;
            } else if (local_geary_coord->local_geary_type == LocalGearyCoordinator::bivariate) {
                num_cats += 3;
            } else {
                num_cats += 5;
            }
		} else {
            // significance map
			// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
			int s_f = local_geary_coord->GetSignificanceFilter();
            num_cats += 6 - s_f;
            
            // issue #474 only show significance levels that can be mapped for the given number of permutations, e.g., for 99 it would stop at 0.01, for 999 at 0.001, etc.
            double sig_cutoff = local_geary_coord->significance_cutoff;
            int set_perm = local_geary_coord->permutations;
            stop_sig = 1.0 / (1.0 + set_perm);
            
			if ( sig_cutoff >= 0.0001 && stop_sig > 0.0001) {
                num_cats -= 1;
            }
            if ( sig_cutoff >= 0.001 && stop_sig > 0.001 ) {
                num_cats -= 1;
            }
            if ( sig_cutoff >= 0.01 && stop_sig > 0.01 ) {
                num_cats -= 1;
            }
		}
		cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
		
        Shapefile::Header& hdr = project->main_data.header;
        
		if (is_clust) {
			cat_data.SetCategoryLabel(t, 0, "Not Significant");
            
            if (hdr.shape_type == Shapefile::POINT_TYP) {
                cat_data.SetCategoryColor(t, 0, wxColour(190, 190, 190));
            } else {
                cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
            }
            if (local_geary_coord->local_geary_type == LocalGearyCoordinator::multivariate) {
    			cat_data.SetCategoryLabel(t, 1, "Positive");
    			cat_data.SetCategoryColor(t, 1, wxColour(51,110,161));
                
                if (local_geary_coord->GetHasIsolates(t) &&
                    local_geary_coord->GetHasUndefined(t)) {
                    isolates_cat = 2;
                    undefined_cat = 3;
                } else if (local_geary_coord->GetHasUndefined(t)) {
                    undefined_cat = 2;
                } else if (local_geary_coord->GetHasIsolates(t)) {
                    isolates_cat = 2;
                }
                
            } else if (local_geary_coord->local_geary_type == LocalGearyCoordinator::bivariate) {
    			cat_data.SetCategoryLabel(t, 1, "Positive");
    			cat_data.SetCategoryColor(t, 1, wxColour(51,110,161));
    			cat_data.SetCategoryLabel(t, 2, "Negative");
    			cat_data.SetCategoryColor(t, 2, wxColour(113,250,142));
                
                if (local_geary_coord->GetHasIsolates(t) &&
                    local_geary_coord->GetHasUndefined(t)) {
                    isolates_cat = 3;
                    undefined_cat = 4;
                } else if (local_geary_coord->GetHasUndefined(t)) {
                    undefined_cat = 3;
                } else if (local_geary_coord->GetHasIsolates(t)) {
                    isolates_cat = 3;
                }
                
            } else {
    			cat_data.SetCategoryLabel(t, 1, "High-High");
    			cat_data.SetCategoryColor(t, 1, wxColour(178,24,43));
    			cat_data.SetCategoryLabel(t, 2, "Low-Low");
    			cat_data.SetCategoryColor(t, 2, wxColour(239,138,98));
    			cat_data.SetCategoryLabel(t, 3, "Other Pos");
    			cat_data.SetCategoryColor(t, 3, wxColour(253,219,199));
    			cat_data.SetCategoryLabel(t, 4, "Negative");
    			cat_data.SetCategoryColor(t, 4, wxColour(103,173,199));
                
                if (local_geary_coord->GetHasIsolates(t) &&
                    local_geary_coord->GetHasUndefined(t)) {
                    isolates_cat = 5;
                    undefined_cat = 6;
                } else if (local_geary_coord->GetHasUndefined(t)) {
                    undefined_cat = 5;
                } else if (local_geary_coord->GetHasIsolates(t)) {
                    isolates_cat = 5;
                }
            }
			
            
		} else {
			// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
			int s_f = local_geary_coord->GetSignificanceFilter();
			cat_data.SetCategoryLabel(t, 0, "Not Significant");

            if (hdr.shape_type == Shapefile::POINT_TYP) {
                cat_data.SetCategoryColor(t, 0, wxColour(190, 190, 190));
            } else {
                cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
            }
   
            int skip_cat = 0;
            if (s_f <=4 && stop_sig <= 0.0001) {
                cat_data.SetCategoryLabel(t, 5-s_f, "p = 0.0001");
                cat_data.SetCategoryColor(t, 5-s_f, wxColour(1, 70, 3));
            } else skip_cat++;
            
			if (s_f <= 3 && stop_sig <= 0.001) {
				cat_data.SetCategoryLabel(t, 4-s_f, "p = 0.001");
				cat_data.SetCategoryColor(t, 4-s_f, wxColour(3, 116, 6));
            } else skip_cat++;
            
			if (s_f <= 2 && stop_sig <= 0.01) {
				cat_data.SetCategoryLabel(t, 3-s_f, "p = 0.01");
				cat_data.SetCategoryColor(t, 3-s_f, wxColour(6, 196, 11));	
            } else skip_cat++;
            
			if (s_f <= 1) {
				cat_data.SetCategoryLabel(t, 2-s_f, "p = 0.05");
				cat_data.SetCategoryColor(t, 2-s_f, wxColour(75, 255, 80));
			}
			if (local_geary_coord->GetHasIsolates(t) &&
				local_geary_coord->GetHasUndefined(t)) {
				isolates_cat = 6 - s_f - skip_cat;
				undefined_cat = 7 - s_f - skip_cat;
                
			} else if (local_geary_coord->GetHasUndefined(t)) {
				undefined_cat = 6 -s_f - skip_cat;
                
			} else if (local_geary_coord->GetHasIsolates(t)) {
				isolates_cat = 6 - s_f -skip_cat;
                
			}
		}
		if (undefined_cat != -1) {
			cat_data.SetCategoryLabel(t, undefined_cat, "Undefined");
			cat_data.SetCategoryColor(t, undefined_cat, wxColour(70, 70, 70));
		}
		if (isolates_cat != -1) {
			cat_data.SetCategoryLabel(t, isolates_cat, "Neighborless");
			cat_data.SetCategoryColor(t, isolates_cat, wxColour(140, 140, 140));
		}
		
		double cuttoff = local_geary_coord->significance_cutoff;
		double* p = local_geary_coord->sig_local_geary_vecs[t];
		int* cluster = local_geary_coord->cluster_vecs[t];
		int* sigCat = local_geary_coord->sig_cat_vecs[t];
		
		if (is_clust) {
            if (local_geary_coord->local_geary_type == LocalGearyCoordinator::multivariate) {
                for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
                    if (p[i] > cuttoff && cluster[i] != 2 && cluster[i] != 3) {
                        cat_data.AppendIdToCategory(t, 0, i); // not significant
                    } else if (cluster[i] == 2) {
                        cat_data.AppendIdToCategory(t, isolates_cat, i);
                    } else if (cluster[i] == 3) {
                        cat_data.AppendIdToCategory(t, undefined_cat, i);
                    } else {
                        cat_data.AppendIdToCategory(t, cluster[i], i);
                    }
                }
            } else {
                for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
                    if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
                        cat_data.AppendIdToCategory(t, 0, i); // not significant
                    } else if (cluster[i] == 5) {
                        cat_data.AppendIdToCategory(t, isolates_cat, i);
                    } else if (cluster[i] == 6) {
                        cat_data.AppendIdToCategory(t, undefined_cat, i);
                    } else {
                        cat_data.AppendIdToCategory(t, cluster[i], i);
                    }
                }
            }
		} else {
			int s_f = local_geary_coord->GetSignificanceFilter();
            if (local_geary_coord->local_geary_type == LocalGearyCoordinator::multivariate) {
                for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
                    if (p[i] > cuttoff && cluster[i] != 2 && cluster[i] != 3) {
                        cat_data.AppendIdToCategory(t, 0, i); // not significant
                    } else if (cluster[i] == 2) {
                        cat_data.AppendIdToCategory(t, isolates_cat, i);
                    } else if (cluster[i] == 3) {
                        cat_data.AppendIdToCategory(t, undefined_cat, i);
                    } else {
                        cat_data.AppendIdToCategory(t, (sigCat[i]-s_f)+1, i);
                    }
                }
            } else {
                for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
                    if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
                        cat_data.AppendIdToCategory(t, 0, i); // not significant
                    } else if (cluster[i] == 5) {
                        cat_data.AppendIdToCategory(t, isolates_cat, i);
                    } else if (cluster[i] == 6) {
                        cat_data.AppendIdToCategory(t, undefined_cat, i);
                    } else {
                        cat_data.AppendIdToCategory(t, (sigCat[i]-s_f)+1, i);
                    }
                }
            }
		}
		for (int cat=0; cat<num_cats; cat++) {
			cat_data.SetCategoryCount(t, cat,
									  cat_data.GetNumObsInCategory(t, cat));
		}
	}
	
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	PopulateCanvas();
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void LocalGearyMapCanvas::SyncVarInfoFromCoordinator()
{
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = local_geary_coord->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0; t<var_info.size(); t++) {
		var_info[t].time = my_times[t];
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	is_any_time_variant = local_geary_coord->is_any_time_variant;
	is_any_sync_with_global_time = local_geary_coord->is_any_sync_with_global_time;
	ref_var_index = local_geary_coord->ref_var_index;
	num_time_vals = local_geary_coord->num_time_vals;
	map_valid = local_geary_coord->map_valid;
	map_error_message = local_geary_coord->map_error_message;
}

void LocalGearyMapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In LocalGearyMapCanvas::TimeSyncVariableToggle");
	local_geary_coord->var_info[var_index].sync_with_global_time =
		!local_geary_coord->var_info[var_index].sync_with_global_time;
	for (int i=0; i<var_info.size(); i++) {
		local_geary_coord->var_info[i].time = var_info[i].time;
	}
	local_geary_coord->VarInfoAttributeChange();
	local_geary_coord->InitFromVarInfo();
	local_geary_coord->notifyObservers();
}


IMPLEMENT_CLASS(LocalGearyMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(LocalGearyMapFrame, MapFrame)
	EVT_ACTIVATE(LocalGearyMapFrame::OnActivate)
END_EVENT_TABLE()

LocalGearyMapFrame::LocalGearyMapFrame(wxFrame *parent, Project* project,
                           LocalGearyCoordinator* local_geary_coordinator,
                           bool isClusterMap, bool isBivariate,
                           bool isEBRate,
                           const wxPoint& pos, const wxSize& size,
                           const long style)
: MapFrame(parent, project, pos, size, style),
local_geary_coord(local_geary_coordinator)
{
	LOG_MSG("Entering LocalGearyMapFrame::LocalGearyMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
    
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    CatClassification::CatClassifType theme_type_s = isClusterMap ? CatClassification::local_geary_categories : CatClassification::local_geary_significance;
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new LocalGearyMapCanvas(rpanel, this, project,
                                        local_geary_coordinator,
                                        theme_type_s,
                                        isBivariate,
                                        isEBRate,
                                        wxDefaultPosition,
                                        wxDefaultSize);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
	
	wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new MapNewLegend(lpanel, template_canvas,
                                       wxPoint(0,0), wxSize(0,0));
	wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);
    
	splitter_win->SplitVertically(lpanel, rpanel, GdaConst::map_default_legend_width);
    
    wxPanel* toolbar_panel = new wxPanel(this,-1, wxDefaultPosition);
	wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    wxToolBar* tb = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
	toolbar_sizer->Add(tb, 0, wxEXPAND|wxALL);
	toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL); 
	sizer->Add(splitter_win, 1, wxEXPAND|wxALL); 
    SetSizer(sizer);
    //splitter_win->SetSize(wxSize(width,height));
    SetAutoLayout(true);
    
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
    
    
	local_geary_coord->registerObserver(this);
	Show(true);
	LOG_MSG("Exiting LocalGearyMapFrame::LocalGearyMapFrame");
}

LocalGearyMapFrame::~LocalGearyMapFrame()
{
	LOG_MSG("In LocalGearyMapFrame::~LocalGearyMapFrame");
	if (local_geary_coord) {
		local_geary_coord->removeObserver(this);
		local_geary_coord = 0;
	}
}

void LocalGearyMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In LocalGearyMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("LocalGearyMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void LocalGearyMapFrame::MapMenus()
{
	LOG_MSG("In LocalGearyMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_LISAMAP_NEW_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void LocalGearyMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("LocalGearyMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((LocalGearyMapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void LocalGearyMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LocalGearyMapFrame::RanXPer(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	local_geary_coord->permutations = permutation;
	local_geary_coord->CalcPseudoP();
	local_geary_coord->notifyObservers();
}

void LocalGearyMapFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void LocalGearyMapFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void LocalGearyMapFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void LocalGearyMapFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void LocalGearyMapFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		RanXPer(num);
	}
}

void LocalGearyMapFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
	local_geary_coord->SetReuseLastSeed(!local_geary_coord->IsReuseLastSeed());
}

void LocalGearyMapFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
	uint64_t last_seed = local_geary_coord->GetLastUsedSeed();
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
		if (!local_geary_coord->IsReuseLastSeed()) local_geary_coord->SetLastUsedSeed(true);
		uint64_t new_seed_val = val;
		local_geary_coord->SetLastUsedSeed(new_seed_val);
	} else {
		wxString m;
		m << "\"" << dlg_val << "\" is not a valid seed. Seed unchanged.";
		wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void LocalGearyMapFrame::SetSigFilterX(int filter)
{
	if (filter == local_geary_coord->GetSignificanceFilter()) return;
	local_geary_coord->SetSignificanceFilter(filter);
	local_geary_coord->notifyObservers();
	UpdateOptionMenuItems();
}

void LocalGearyMapFrame::OnSigFilter05(wxCommandEvent& event)
{
	SetSigFilterX(1);
}

void LocalGearyMapFrame::OnSigFilter01(wxCommandEvent& event)
{
	SetSigFilterX(2);
}

void LocalGearyMapFrame::OnSigFilter001(wxCommandEvent& event)
{
	SetSigFilterX(3);
}

void LocalGearyMapFrame::OnSigFilter0001(wxCommandEvent& event)
{
	SetSigFilterX(4);
}

void LocalGearyMapFrame::OnSaveLocalGeary(wxCommandEvent& event)
{
    
	int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
    LocalGearyMapCanvas* lc = (LocalGearyMapCanvas*)template_canvas;
    
    std::vector<SaveToTableEntry> data;
    
    if (lc->is_diff) {
        data.resize(4);
    } else {
        data.resize(3);
    }
   
    std::vector<bool> undefs(local_geary_coord->num_obs, false);
    for (int i=0; i<local_geary_coord->undef_data[0][t].size(); i++){
        undefs[i] = undefs[i] || local_geary_coord->undef_data[0][t][i];
    }
    
	std::vector<double> tempLocalMoran(local_geary_coord->num_obs);
	for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
		tempLocalMoran[i] = local_geary_coord->local_geary_vecs[t][i];
	}
	data[0].d_val = &tempLocalMoran;
	data[0].label = "LocalGeary Indices";
	data[0].field_default = "LocalGeary_I";
	data[0].type = GdaConst::double_type;
    data[0].undefined = &undefs;
	
	double cuttoff = local_geary_coord->significance_cutoff;
	double* p = local_geary_coord->sig_local_geary_vecs[t];
	int* cluster = local_geary_coord->cluster_vecs[t];
	std::vector<wxInt64> clust(local_geary_coord->num_obs);
	for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
		if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
			clust[i] = 0; // not significant
		} else {
			clust[i] = cluster[i];
		}
	}
	data[1].l_val = &clust;
	data[1].label = "Clusters";
	data[1].field_default = "LocalGeary_CL";
	data[1].type = GdaConst::long64_type;
    data[1].undefined = &undefs;
	
	std::vector<double> sig(local_geary_coord->num_obs);
    std::vector<double> diff(local_geary_coord->num_obs);
    
	for (int i=0, iend=local_geary_coord->num_obs; i<iend; i++) {
		sig[i] = p[i];
        
        
        if (lc->is_diff ) {
            int t0 =  local_geary_coord->var_info[0].time;
            int t1 =  local_geary_coord->var_info[1].time;
            diff[i] = local_geary_coord->data[0][t0][i] - local_geary_coord->data[0][t1][i];
        }
	}
	
	data[2].d_val = &sig;
	data[2].label = "Significance";
	data[2].field_default = "LocalGeary_P";
	data[2].type = GdaConst::double_type;
    data[2].undefined = &undefs;
	
    if (lc->is_diff) {
        data[3].d_val = &diff;
        data[3].label = "Diff Values";
        data[3].field_default = "DIFF_VAL2";
        data[3].type = GdaConst::double_type;
        data[3].undefined = &undefs;
    }
    
	SaveToTableDlg dlg(project, this, data,
					   "Save Results: LocalGeary",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void LocalGearyMapFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
	HighlightState* highlight_state = project->GetHighlightState();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	for (int i=0; i<local_geary_coord->num_obs; i++) {
		if (!hs[i] && elem[i]) {
            hs[i] = true;
            selection_changed  = true;
		} else if (hs[i] && !elem[i]) {
            hs[i] = false;
            selection_changed  = true;
		}
	}
    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
}

void LocalGearyMapFrame::OnSelectCores(wxCommandEvent& event)
{
	LOG_MSG("Entering LocalGearyMapFrame::OnSelectCores");
	
	std::vector<bool> elem(local_geary_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = local_geary_coord->cluster_vecs[ts];
	int* sig_cat = local_geary_coord->sig_cat_vecs[ts];
	int sf = local_geary_coord->significance_filter;
	
	// add all cores to elem list.
	for (int i=0; i<local_geary_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = true;
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting LocalGearyMapFrame::OnSelectCores");
}

void LocalGearyMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	LOG_MSG("Entering LocalGearyMapFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(local_geary_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = local_geary_coord->cluster_vecs[ts];
	int* sig_cat = local_geary_coord->sig_cat_vecs[ts];
	int sf = local_geary_coord->significance_filter;
    const GalElement* W = local_geary_coord->Gal_vecs_orig[ts]->gal;
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<local_geary_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = true;
			const GalElement& e = W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e[j]] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<local_geary_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = false;
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting LocalGearyMapFrame::OnSelectNeighborsOfCores");
}

void LocalGearyMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	LOG_MSG("Entering LocalGearyMapFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(local_geary_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = local_geary_coord->cluster_vecs[ts];
	int* sig_cat = local_geary_coord->sig_cat_vecs[ts];
	int sf = local_geary_coord->significance_filter;
    const GalElement* W = local_geary_coord->Gal_vecs_orig[ts]->gal;
    
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<local_geary_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = true;
			const GalElement& e = W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e[j]] = true;
			}
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting LocalGearyMapFrame::OnSelectCoresAndNeighbors");
}

void LocalGearyMapFrame::OnAddNeighborToSelection(wxCommandEvent& event)
{
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
    GalWeight* gal_weights = local_geary_coord->Gal_vecs_orig[ts];
   
    HighlightState& hs = *project->GetHighlightState();
    std::vector<bool>& h = hs.GetHighlight();
    int nh_cnt = 0;
    std::vector<bool> add_elem(gal_weights->num_obs, false);
    
    std::vector<int> new_highlight_ids;
    
    for (int i=0; i<gal_weights->num_obs; i++) {
        if (h[i]) {
            GalElement& e = gal_weights->gal[i];
            for (int j=0, jend=e.Size(); j<jend; j++) {
                int obs = e[j];
                if (!h[obs] && !add_elem[obs]) {
                    add_elem[obs] = true;
                    new_highlight_ids.push_back(obs);
                }
            }
        }
    }
    
    for (int i=0; i<(int)new_highlight_ids.size(); i++) {
        h[ new_highlight_ids[i] ] = true;
        nh_cnt ++;
    }
    
    if (nh_cnt > 0) {
        hs.SetEventType(HLStateInt::delta);
        hs.notifyObservers();
    }
}

void LocalGearyMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
                            _("Conditional LocalGeary Map Variables"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"));
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    
	LocalGearyMapCanvas* lc = (LocalGearyMapCanvas*) template_canvas;
    wxString title = lc->GetCanvasTitle();
    ConditionalClusterMapFrame* subframe =
    new ConditionalClusterMapFrame(this, project,
                                   dlg.var_info, dlg.col_ids, local_geary_coord,
                                   title, wxDefaultPosition,
                                   GdaConst::cond_view_default_size);
}

/** Called by LocalGearyCoordinator to notify that state has changed.  State changes
 can include:
   - variable sync change and therefore all local_geary categories have changed
   - significance level has changed and therefore categories have changed
   - new randomization for p-vals and therefore categories have changed */
void LocalGearyMapFrame::update(LocalGearyCoordinator* o)
{
	LocalGearyMapCanvas* lc = (LocalGearyMapCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	lc->CreateAndUpdateCategories();
	if (template_legend) template_legend->Recreate();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
}

void LocalGearyMapFrame::closeObserver(LocalGearyCoordinator* o)
{
	LOG_MSG("In LocalGearyMapFrame::closeObserver(LocalGearyCoordinator*)");
	if (local_geary_coord) {
		local_geary_coord->removeObserver(this);
		local_geary_coord = 0;
	}
	Close(true);
}

void LocalGearyMapFrame::GetVizInfo(std::vector<int>& clusters)
{
	if (local_geary_coord) {
		if(local_geary_coord->sig_cat_vecs.size()>0) {
			for (int i=0; i<local_geary_coord->num_obs;i++) {
				clusters.push_back(local_geary_coord->sig_cat_vecs[0][i]);
			}
		}
	}
}
