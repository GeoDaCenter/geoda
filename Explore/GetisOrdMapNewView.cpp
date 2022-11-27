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

#include "GetisOrdMapNewView.h"

#include <sstream>
#include <vector>
#include <limits>
#include <vector>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../DialogTools/RandomizationDlg.h"
#include "../VarCalc/WeightsManInterface.h"
#include "ConditionalClusterMapView.h"
#include "GStatCoordinator.h"
#include "GetisOrdMapNewView.h"

IMPLEMENT_CLASS(GetisOrdMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(GetisOrdMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()


GetisOrdMapCanvas::GetisOrdMapCanvas(wxWindow *parent,
                                     TemplateFrame* t_frame,
                                     Project* project,
                                     GStatCoordinator* gs_coordinator,
                                     bool is_gi_s, bool is_clust_s,
                                     bool is_perm_s,
                                     bool row_standardize_s,
                                     const wxPoint& pos,
                                     const wxSize& size)
: MapCanvas(parent, t_frame, project,
            std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0),
            CatClassification::no_theme,
            no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
gs_coord(gs_coordinator),
is_gi(is_gi_s), is_clust(is_clust_s), is_perm(is_perm_s),
row_standardize(row_standardize_s)
{
	wxLogMessage("Entering GetisOrdMapCanvas::GetisOrdMapCanvas");

    str_not_sig = _("Not Significant");
    str_high = _("High");
    str_low = _("Low");
    str_undefined = _("Undefined");
    str_neighborless = _("Neighborless");
    str_p005 = "p = 0.05";
    str_p001 = "p = 0.01";
    str_p0001 = "p = 0.001";
    str_p00001 = "p = 0.0001";
    str_p000001 = "p = 0.00001";
    
    SetPredefinedColor(str_not_sig, wxColour(240, 240, 240));
    SetPredefinedColor(str_high, wxColour(255, 0, 0));
    SetPredefinedColor(str_low, wxColour(0, 0, 255));
    SetPredefinedColor(str_undefined, wxColour(70, 70, 70));
    SetPredefinedColor(str_neighborless, wxColour(140, 140, 140));
    SetPredefinedColor(str_p005, wxColour(75, 255, 80));
    SetPredefinedColor(str_p001, wxColour(6, 196, 11));
    SetPredefinedColor(str_p0001, wxColour(3, 116, 6));
    SetPredefinedColor(str_p00001, wxColour(1, 70, 3));
    SetPredefinedColor(str_p000001, wxColour(0, 50, 2));

	if (is_clust) {
		cat_classif_def.cat_classif_type
			= CatClassification::getis_ord_categories;
	} else {
		cat_classif_def.cat_classif_type
			= CatClassification::getis_ord_significance;
	}
	
	// must set var_info times from GStatCoordinator initially
	var_info = gs_coord->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0, sz=var_info.size(); t<sz; ++t) {
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	CreateAndUpdateCategories();
	UpdateStatusBar();
	wxLogMessage("Exiting GetisOrdMapCanvas::GetisOrdMapCanvas");
}

GetisOrdMapCanvas::~GetisOrdMapCanvas()
{
	wxLogMessage("In GetisOrdMapCanvas::~GetisOrdMapCanvas");
}

void GetisOrdMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	wxLogMessage("Entering GetisOrdMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((GetisOrdMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	wxLogMessage("Exiting MapCanvas::DisplayRightClickMenu");
}

wxString GetisOrdMapCanvas::GetCanvasTitle()
{
	wxString new_title;
	
    if (gs_coord->is_local_join_count) new_title = "Local Join Count ";
    else new_title = (is_gi ? "Gi " : "Gi* ");
    
	new_title << (is_clust ? "Cluster" : "Significance") << " Map ";
	new_title << "(" << gs_coord->weight_name << "): ";
	new_title << GetNameWithTime(0);
	if (is_perm) new_title << wxString::Format(", pseudo p (%d perm)",
											   gs_coord->permutations);
	if (!is_perm) new_title << ", normal p";
	new_title << (row_standardize ? ", row-standardized W" :
				  ", binary W");
	return new_title;
}

wxString GetisOrdMapCanvas::GetVariableNames()
{
    wxString new_title;
    new_title << GetNameWithTime(0);
    return new_title;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by GetisOrd maps */
bool GetisOrdMapCanvas::ChangeMapType(CatClassification::CatClassifType new_theme,
                                      SmoothingType new_smoothing)
{
	wxLogMessage("In GetisOrdMapCanvas::ChangeMapType");
	return false;
}

void GetisOrdMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	MapCanvas::SetCheckMarks(menu);
	
	int sig_filter = ((GetisOrdMapFrame*) template_frame)->
		GetGStatCoordinator()->GetSignificanceFilter();
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
								  sig_filter == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
								  sig_filter == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
								  sig_filter == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
								  sig_filter == 4);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_SETUP"),
                                  sig_filter == -1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_SPECIFIED_SEED"),
								  gs_coord->IsReuseLastSeed());
}

void GetisOrdMapCanvas::TimeChange()
{
	wxLogMessage("Entering GetisOrdMapCanvas::TimeChange");
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetTimeState()->GetCurrTime();
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
        (cts < ref_time_min && ref_time == ref_time_min))
    {
        return;
    }
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
	wxLogMessage("Exiting GetisOrdMapCanvas::TimeChange");
}

/** Update Categories based on info in GStatCoordinator */
void GetisOrdMapCanvas::CreateAndUpdateCategories()
{
	SyncVarInfoFromCoordinator();
	cat_data.CreateEmptyCategories(num_time_vals, num_obs);
	
	std::vector<wxInt64> cluster;
	for (int t=0; t<num_time_vals; t++) {
		if (!map_valid[t])
            break;
		
		int undefined_cat = -1;
		int isolates_cat = -1;
		int num_cats = 0;
        Shapefile::Header& hdr = project->main_data.header;
        
        int set_perm = gs_coord->permutations;
        double stop_sig = 1.0 / (1.0 + set_perm);
        double sig_cutoff = gs_coord->significance_cutoff;
        wxString def_cats[NUM_SIG_CATS] = {str_p005, str_p001, str_p0001, str_p00001, str_p000001};
        double def_cutoffs[NUM_SIG_CATS] = {0.05, 0.01, 0.001, 0.0001, 0.00001};
        bool is_cust_cutoff = gs_coord->GetSignificanceFilter() < 0;
        bool has_isolates = gs_coord->GetHasIsolates(t);
        bool has_undefined = gs_coord->GetHasUndefined(t);
        int cat_idx = 0;
        
        if (has_isolates) {
            num_cats++;
        }
        if (has_undefined) {
            num_cats++;
        }
        
		if (is_clust) {
            // Not Sig, HH, LL
			num_cats += 3;
            // in Local Join Count, don't display Low category
            if (gs_coord->is_local_join_count) {
                num_cats -= 1;
            }
            
            cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
            
            cat_data.SetCategoryLabel(t, cat_idx, str_not_sig);
            cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_not_sig]);
            cat_data.SetCategoryLabel(t, cat_idx, str_high);
            cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_high]);
            
            if (!gs_coord->is_local_join_count) {
                cat_data.SetCategoryLabel(t, cat_idx, str_low);
                cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_low]);
            }
            
            if (has_undefined) {
                undefined_cat = cat_idx;
                cat_data.SetCategoryLabel(t, cat_idx, str_undefined);
                cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_undefined]);
            }
            if (has_isolates) {
                isolates_cat = cat_idx;
                cat_data.SetCategoryLabel(t, cat_idx, str_neighborless);
                cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_neighborless]);
            }
            
            gs_coord->FillClusterCats(t, is_gi, is_perm, cluster);
            
            for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
                if (cluster[i] == 0) {
                    cat_data.AppendIdToCategory(t, 0, i); // not significant
                } else if (cluster[i] == GStatCoordinator::NEIGHBORLESS_CLUSTER) {
                    cat_data.AppendIdToCategory(t, isolates_cat, i);
                } else if (cluster[i] == GStatCoordinator::UNDEFINED_CLUSTER) {
                    cat_data.AppendIdToCategory(t, undefined_cat, i);
                } else {
                    cat_data.AppendIdToCategory(t, cluster[i], i);
                }
            }
            
		} else {
            // significance map
            // 0: >0.05 (Not sig) 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001, 5: 0.00001
            num_cats += 1; // not sig category
            
            if (is_cust_cutoff) {
                num_cats += 1;
            } else {
                for (int j=0; j < NUM_SIG_CATS; j++) {
                    if (sig_cutoff >= def_cutoffs[j] && stop_sig <= def_cutoffs[j]) {
                        num_cats += 1;
                    }
                }
            }
            cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
            
            int cat_idx = 0;
            cat_data.SetCategoryLabel(t, cat_idx, str_not_sig);
            cat_data.SetCategoryColor(t, cat_idx++, hdr.shape_type == Shapefile::POINT_TYP ? wxColour(190, 190, 190) : wxColour(240, 240, 240));

            std::map<int, int> level_cat_dict;
            
            if (is_cust_cutoff) {
                std::ostringstream ss_sig_cutoff;
                ss_sig_cutoff << std::fixed << sig_cutoff;
                wxString lbl =  wxString::Format("p = %s", ss_sig_cutoff.str());
                cat_data.SetCategoryColor(t, cat_idx, lbl_color_dict[def_cats[0]]);
                cat_data.SetCategoryLabel(t, cat_idx++, lbl);
            } else {
                for (int j=0; j < NUM_SIG_CATS; j++) {
                    if (stop_sig <= def_cutoffs[j]) {
                        level_cat_dict[j] = cat_idx;
                        cat_data.SetCategoryColor(t, cat_idx, lbl_color_dict[def_cats[j]]);
                        cat_data.SetCategoryLabel(t, cat_idx++, def_cats[j]);
                    }
                }
            }
            
            if (has_isolates) {
                isolates_cat = cat_idx;
                cat_data.SetCategoryLabel(t, cat_idx, str_neighborless);
                cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_neighborless]);
            }
            if (has_undefined) {
                undefined_cat = cat_idx;
                cat_data.SetCategoryLabel(t, cat_idx, str_undefined);
                cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[str_undefined]);
            }
            
            gs_coord->FillClusterCats(t, is_gi, is_perm, cluster);
            
            double* p = 0;
            if (is_gi && is_perm)
                p = gs_coord->pseudo_p_vecs[t];
            if (is_gi && !is_perm)
                p = gs_coord->p_vecs[t];
            if (!is_gi && is_perm)
                p = gs_coord->pseudo_p_star_vecs[t];
            if (!is_gi && !is_perm)
                p = gs_coord->p_star_vecs[t];
            
            for (int i=0; i<gs_coord->num_obs; i++) {
                if (cluster[i] == 0) {
                    cat_data.AppendIdToCategory(t, 0, i); // not significant
                } else if (cluster[i] == GStatCoordinator::NEIGHBORLESS_CLUSTER) {
                    cat_data.AppendIdToCategory(t, isolates_cat, i);
                } else if (cluster[i] == GStatCoordinator::UNDEFINED_CLUSTER) {
                    cat_data.AppendIdToCategory(t, undefined_cat, i);
                } else {
                    if (is_cust_cutoff) {
                        if (p[i] <= sig_cutoff) {
                            cat_data.AppendIdToCategory(t, 1, i);
                        }
                    } else {
                        for (int c = NUM_SIG_CATS - 1; c >= 0; c--) {
                            if (p[i] <= def_cutoffs[c]) {
                                cat_data.AppendIdToCategory(t, level_cat_dict[c], i);
                                break;
                            }
                        }
                    }
                }
            }
		}
		
		for (int cat=0; cat<num_cats; cat++) {
			cat_data.SetCategoryCount(t, cat, cat_data.GetNumObsInCategory(t, cat));
		}
	}
	
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	PopulateCanvas();
}

void GetisOrdMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = 0;
    if (template_frame) {
        sb = template_frame->GetStatusBar();
    }
    if (!sb)
        return;
    wxString s;
    s << _("#obs=") << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
        s << _("#selected=") << highlight_state->GetTotalHighlighted()<< "  ";
    }
    if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << _("#hover obs ") << hover_obs[0]+1;
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << _("obs ") << hover_obs[1]+1;
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << _("obs ") << hover_obs[2]+1;
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    }
    if (is_clust && gs_coord) {
        double p_val = gs_coord->significance_cutoff;
        wxString inf_str = wxString::Format(" p <= %g", p_val);
        s << inf_str;
    }
    sb->SetStatusText(s);
}

void GetisOrdMapCanvas::TimeSyncVariableToggle(int var_index)
{
	wxLogMessage("In GetisOrdMapCanvas::TimeSyncVariableToggle");
	gs_coord->var_info[var_index].sync_with_global_time =
	!gs_coord->var_info[var_index].sync_with_global_time;
	for (int i=0; i<var_info.size(); i++) {
		gs_coord->var_info[i].time = var_info[i].time;
	}
	gs_coord->VarInfoAttributeChange();
	gs_coord->InitFromVarInfo();
	gs_coord->notifyObservers();
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void GetisOrdMapCanvas::SyncVarInfoFromCoordinator()
{
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = gs_coord->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0; t<var_info.size(); t++) {
		var_info[t].time = my_times[t];
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	is_any_time_variant = gs_coord->is_any_time_variant;
	is_any_sync_with_global_time = gs_coord->is_any_sync_with_global_time;
	ref_var_index = gs_coord->ref_var_index;
	num_time_vals = gs_coord->num_time_vals;
	map_valid = gs_coord->map_valid;
	map_error_message = gs_coord->map_error_message;
}

IMPLEMENT_CLASS(GetisOrdMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(GetisOrdMapFrame, MapFrame)
	EVT_ACTIVATE(GetisOrdMapFrame::OnActivate)
END_EVENT_TABLE()

GetisOrdMapFrame::GetisOrdMapFrame(wxFrame *parent, Project* project,
                                   GStatCoordinator* gs_coordinator,
                                   GMapType map_type_s,
                                   bool row_standardize_s,
                                   const wxPoint& pos, const wxSize& size,
                                   const long style)
: MapFrame(parent, project, pos, size, style),
gs_coord(gs_coordinator), map_type(map_type_s),
row_standardize(row_standardize_s)
{
	wxLogMessage("Entering GetisOrdMapFrame::GetisOrdMapFrame");
	
    no_update_weights = true;
    
	int width, height;
	GetClientSize(&width, &height);

	DisplayStatusBar(true);
    
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
	is_gi = (map_type == Gi_clus_perm || map_type == Gi_clus_norm ||
			 map_type == Gi_sig_perm || map_type == Gi_sig_norm);
	is_clust = (map_type == Gi_clus_perm || map_type == Gi_clus_norm ||
				map_type == GiStar_clus_perm || map_type == GiStar_clus_norm);
	is_perm = (map_type == Gi_clus_perm || map_type == Gi_sig_perm ||
			   map_type == GiStar_clus_perm || map_type == GiStar_sig_perm);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new GetisOrdMapCanvas(rpanel, this, project,
                                            gs_coordinator,
                                            is_gi, is_clust, is_perm,
                                            row_standardize,
                                            wxDefaultPosition,
                                            wxDefaultSize);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
    WeightsManInterface* w_man_int = project->GetWManInt();
    ((MapCanvas*) template_canvas)->SetWeightsId(w_man_int->GetDefault());
    
    wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new MapNewLegend(lpanel, template_canvas, wxPoint(0,0), wxSize(0,0));
	wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);
    
	splitter_win->SplitVertically(lpanel, rpanel, GdaConst::map_default_legend_width);
    
	
    wxPanel* toolbar_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition);
	wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    toolbar = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
	toolbar_sizer->Add(toolbar, 0, wxEXPAND|wxALL);
	toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL); 
	sizer->Add(splitter_win, 1, wxEXPAND|wxALL); 
    SetSizer(sizer);
    SetAutoLayout(true);
   
	gs_coord->registerObserver(this);
	
	SetTitle(template_canvas->GetCanvasTitle());
	Show(true);
	wxLogMessage("Exiting GetisOrdMapFrame::GetisOrdMapFrame");
}

GetisOrdMapFrame::~GetisOrdMapFrame()
{
	wxLogMessage("In GetisOrdMapFrame::~GetisOrdMapFrame");
	if (gs_coord) {
		gs_coord->removeObserver(this);
		gs_coord = 0;
	}
}

void GetisOrdMapFrame::OnActivate(wxActivateEvent& event)
{
	wxLogMessage("In GetisOrdMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("GetisOrdMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void GetisOrdMapFrame::MapMenus()
{
	wxLogMessage("In GetisOrdMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);	
	UpdateOptionMenuItems();
}

void GetisOrdMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu(_("Options"));
    if (menu == wxNOT_FOUND) {
        LOG_MSG("GetisOrdMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((GetisOrdMapCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void GetisOrdMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void GetisOrdMapFrame::RanXPer(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	gs_coord->permutations = permutation;
	gs_coord->CalcPseudoP();
	gs_coord->notifyObservers();
}

void GetisOrdMapFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void GetisOrdMapFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void GetisOrdMapFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void GetisOrdMapFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void GetisOrdMapFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
        wxString input = dlg.m_number->GetValue();

        input.ToLong(&num);
		RanXPer(num);
	}
}

void GetisOrdMapFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
	gs_coord->SetReuseLastSeed(!gs_coord->IsReuseLastSeed());
}

void GetisOrdMapFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
	uint64_t last_seed = gs_coord->GetLastUsedSeed();
	wxString m;
	m << "The last seed used by the pseudo random\nnumber ";
	m << "generator was " << last_seed << ".\n";
	m << "Enter a seed value to use between\n0 and ";
	m << std::numeric_limits<uint64_t>::max() << ".";
	long long unsigned int val;
	wxString dlg_val;
	wxString cur_val;
	cur_val << last_seed;
	
	wxTextEntryDialog dlg(NULL, m, "\nEnter a seed value", cur_val);
	if (dlg.ShowModal() != wxID_OK) return;
	dlg_val = dlg.GetValue();
        
	dlg_val.Trim(true);
	dlg_val.Trim(false);
	if (dlg_val.IsEmpty()) return;
	if (dlg_val.ToULongLong(&val)) {
		if (!gs_coord->IsReuseLastSeed()) gs_coord->SetLastUsedSeed(true);
		uint64_t new_seed_val = val;
		gs_coord->SetLastUsedSeed(new_seed_val);
	} else {
		wxString m;
		m << "\"" << dlg_val << "\" is not a valid seed. Seed unchanged.";
		wxMessageDialog dlg(NULL, m, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void GetisOrdMapFrame::SetSigFilterX(int filter)
{
	if (filter == gs_coord->GetSignificanceFilter())
        return;
	gs_coord->SetSignificanceFilter(filter);
	gs_coord->notifyObservers();
	UpdateOptionMenuItems();
}

void GetisOrdMapFrame::OnSigFilter05(wxCommandEvent& event)
{
	SetSigFilterX(1);
}

void GetisOrdMapFrame::OnSigFilter01(wxCommandEvent& event)
{
	SetSigFilterX(2);
}

void GetisOrdMapFrame::OnSigFilter001(wxCommandEvent& event)
{
	SetSigFilterX(3);
}

void GetisOrdMapFrame::OnSigFilter0001(wxCommandEvent& event)
{
	SetSigFilterX(4);
}

void GetisOrdMapFrame::OnSigFilterSetup(wxCommandEvent& event)
{
    GetisOrdMapCanvas* lc = (GetisOrdMapCanvas*)template_canvas;
    int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
    double* p_val_t;
    if (map_type == Gi_clus_perm || map_type == Gi_sig_perm) {
        p_val_t = gs_coord->pseudo_p_vecs[t];
    } else if (map_type == Gi_clus_norm || map_type == Gi_sig_norm) {
        p_val_t = gs_coord->p_vecs[t];
    } else if (map_type == GiStar_clus_perm || map_type == GiStar_sig_perm) {
        p_val_t = gs_coord->pseudo_p_star_vecs[t];
    } else { // (map_type == GiStar_clus_norm || map_type == GiStar_sig_norm)
        p_val_t = gs_coord->p_star_vecs[t];
    }
    int n = gs_coord->num_obs;
    
    wxString ttl = _("Inference Settings");
    ttl << "  (" << gs_coord->permutations << " perm)";
    
    double user_sig = gs_coord->significance_cutoff;
    if (gs_coord->GetSignificanceFilter()<0) user_sig = gs_coord->user_sig_cutoff;
  
    if (gs_coord->is_local_join_count) {
        int new_n = 0;
        for (int i=0; i<gs_coord->num_obs; i++) {
            if (gs_coord->x_vecs[t][i] == 1) {
                new_n += 1;
            }
        }
        if (new_n > 0) {
            int j= 0;
            double* p_val = new double[new_n];
            for (int i=0; i<gs_coord->num_obs; i++) {
                if (gs_coord->x_vecs[t][i] == 1) {
                    p_val[j++] = p_val_t[i];
                }
            }
            InferenceSettingsDlg dlg(this, user_sig, p_val, new_n, ttl);
            if (dlg.ShowModal() == wxID_OK) {
                gs_coord->SetSignificanceFilter(-1);
                gs_coord->significance_cutoff = dlg.GetAlphaLevel();
                gs_coord->user_sig_cutoff = dlg.GetUserInput();
                gs_coord->notifyObservers();
                gs_coord->bo = dlg.GetBO();
                gs_coord->fdr = dlg.GetFDR();
                UpdateOptionMenuItems();
            }
            delete[] p_val;
        }
    } else {
        InferenceSettingsDlg dlg(this, user_sig, p_val_t, n, ttl);
        if (dlg.ShowModal() == wxID_OK) {
            gs_coord->SetSignificanceFilter(-1);
            gs_coord->significance_cutoff = dlg.GetAlphaLevel();
            gs_coord->user_sig_cutoff = dlg.GetUserInput();
            gs_coord->notifyObservers();
            gs_coord->bo = dlg.GetBO();
            gs_coord->fdr = dlg.GetFDR();
            UpdateOptionMenuItems();
        }
    }
}



void GetisOrdMapFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
	wxString title = "Save Results: ";
	title += is_gi ? "Gi" : "Gi*";
	title += "-stats, ";
    
	if (is_perm)
        title += wxString::Format("pseudo p (%d perm), ",
                                  gs_coord->permutations);
	if (!is_perm)
        title += "normal p, ";
    
	title += row_standardize ? "row-standarized W" : "binary W";
	
	double* g_val_t = is_gi ? gs_coord->G_vecs[t] : gs_coord->G_star_vecs[t];
	std::vector<double> g_val(gs_coord->num_obs);
    
    for (int i=0; i<gs_coord->num_obs; i++) {
        g_val[i] = g_val_t[i];
    }
    
	wxString g_label = is_gi ? "G" : "G*";
	wxString g_field_default = is_gi ? "G" : "G_STR";
    
    if (gs_coord->is_local_join_count) {
        title = "Save Results: Local Join Count-stats";
        g_label = "JC";
        g_field_default = "JC";
    }
	
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(t, is_gi, is_perm, c_val);
	wxString c_label = "cluster category";
	wxString c_field_default = "C_ID";
	
	double* p_val_t = 0;
	std::vector<double> p_val(gs_coord->num_obs);
	double* z_val_t = is_gi ? gs_coord->z_vecs[t] : gs_coord->z_star_vecs[t];
	std::vector<double> z_val(gs_coord->num_obs);
	for (int i=0; i<gs_coord->num_obs; i++)
        z_val[i] = z_val_t[i];
    
	wxString p_label = is_perm ? "pseudo p-value" : "p-value";
	wxString p_field_default = is_perm ? "PP_VAL" : "P_VAL";
	
	if (map_type == Gi_clus_perm || map_type == Gi_sig_perm) {
		p_val_t = gs_coord->pseudo_p_vecs[t];
	} else if (map_type == Gi_clus_norm || map_type == Gi_sig_norm) {
		p_val_t = gs_coord->p_vecs[t];
	} else if (map_type == GiStar_clus_perm || map_type == GiStar_sig_perm) {
		p_val_t = gs_coord->pseudo_p_star_vecs[t];
	} else { // (map_type == GiStar_clus_norm || map_type == GiStar_sig_norm)
		p_val_t = gs_coord->p_star_vecs[t];
	}
	for (int i=0; i<gs_coord->num_obs; i++) p_val[i] = p_val_t[i];
	
    int num_data = is_perm ? 3: 4;
     // drop C_ID for local JC, add NN and NN_1
	std::vector<SaveToTableEntry> data(num_data);
    std::vector<bool> undefs(gs_coord->num_obs, false);
    std::vector<bool> c_undefs(gs_coord->num_obs, true);
    
    for (size_t i=0; i<gs_coord->x_undefs.size(); i++) {
        for (size_t j=0; j<gs_coord->x_undefs[i].size(); j++) {
            undefs[j] = undefs[j] || gs_coord->x_undefs[i][j];
        }
    }
    int data_i = 0;
    std::vector<wxInt64> nn_1_val;
    
    if (gs_coord->is_local_join_count == false) {
    	data[data_i].d_val = &g_val;
    	data[data_i].label = g_label;
    	data[data_i].field_default = g_field_default;
    	data[data_i].type = GdaConst::double_type;
        data[data_i].undefined = &undefs;
    	data_i++;
    	data[data_i].l_val = &c_val;
    	data[data_i].label = c_label;
    	data[data_i].field_default = c_field_default;
    	data[data_i].type = GdaConst::long64_type;
        data[data_i].undefined = &undefs;
    	data_i++;
    }
	if (!is_perm) {
		data[data_i].d_val = &z_val;
		data[data_i].label = "z-score";
		data[data_i].field_default = "Z_SCR";
		data[data_i].type = GdaConst::double_type;
        data[data_i].undefined = &undefs;
		data_i++;
	}
    if (gs_coord->is_local_join_count == false) {
    	data[data_i].d_val = &p_val;
    	data[data_i].label = p_label;
    	data[data_i].field_default = p_field_default;
    	data[data_i].type = GdaConst::double_type;
        data[data_i].undefined = &undefs;
    	data_i++;
    } else {
        
        
        for (int i=0; i<gs_coord->num_obs; i++) {
            nn_1_val.push_back( gs_coord->num_neighbors_1[t][i]);
        }
        
        data[data_i].l_val = &nn_1_val;
        data[data_i].label = g_label;
        data[data_i].field_default = g_field_default;
        data[data_i].type = GdaConst::long64_type;
        data[data_i].undefined = &undefs;
        data_i++;
        
        data[data_i].l_val = &gs_coord->num_neighbors;
        data[data_i].label = "Number of Neighbors";
        data[data_i].field_default = "NN";
        data[data_i].type = GdaConst::long64_type;
        data[data_i].undefined = &undefs;
        data_i++;
        
        for (size_t i=0; i<gs_coord->num_obs; i++) {
            if (gs_coord->num_neighbors_1[t][i] > 0 &&
                gs_coord->x_vecs[t][i] == 1)
                c_undefs[i] = false;
        }
        data[data_i].d_val = &p_val;
    	data[data_i].label = p_label;
    	data[data_i].field_default = p_field_default;
    	data[data_i].type = GdaConst::double_type;
        data[data_i].undefined = &c_undefs;
    	data_i++;
    }
	
	SaveToTableDlg dlg(project, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void GetisOrdMapFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
	HighlightState* highlight_state = project->GetHighlightState();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (!hs[i] && elem[i]) {
            hs[i] = true;
            selection_changed  = true;
		} else if (hs[i] && !elem[i]) {
            hs[i] = false;
            selection_changed  = true;
		}
	}
    if (selection_changed ) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
}

void GetisOrdMapFrame::OnSelectCores(wxCommandEvent& event)
{
	wxLogMessage("Entering GetisOrdMapFrame::OnSelectCores");
		
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);

	// add all cores to elem list.
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) elem[i] = true;
	}
	CoreSelectHelper(elem);
	
	wxLogMessage("Exiting GetisOrdMapFrame::OnSelectCores");
}

void GetisOrdMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	wxLogMessage("Entering GetisOrdMapFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) {
			elem[i] = true;
			const GalElement& e = gs_coord->Gal_vecs_orig[ts]->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e[j]] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) {
			elem[i] = false;
		}
	}
	CoreSelectHelper(elem);	
	
	wxLogMessage("Exiting GetisOrdMapFrame::OnSelectNeighborsOfCores");
}

void GetisOrdMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	wxLogMessage("Entering GetisOrdMapFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) {
			elem[i] = true;
			const GalElement& e = gs_coord->Gal_vecs_orig[ts]->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e[j]] = true;
			}
		}
	}
	CoreSelectHelper(elem);
	
	wxLogMessage("Exiting GetisOrdMapFrame::OnSelectCoresAndNeighbors");
}

void GetisOrdMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    int style = VariableSettingsDlg::ALLOW_STRING_IN_FIRST | VariableSettingsDlg::ALLOW_STRING_IN_SECOND |
        VariableSettingsDlg::ALLOW_EMPTY_IN_FIRST |
        VariableSettingsDlg::ALLOW_EMPTY_IN_SECOND;
    

    VariableSettingsDlg dlg(project,
                            VariableSettingsDlg::bivariate,
                            style,
                            _("Conditional G Cluster Map Variables"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"));
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    
	GetisOrdMapCanvas* lc = (GetisOrdMapCanvas*) template_canvas;
    wxString title = lc->GetCanvasTitle();
    ConditionalClusterMapFrame* subframe =
    new ConditionalClusterMapFrame(this, project,
                                   dlg.var_info, dlg.col_ids, gs_coord, is_gi, is_perm,
                                   title, wxDefaultPosition,
                                   GdaConst::cond_view_default_size);
}

/** Called by GStatCoordinator to notify that state has changed.  State changes
 can include:
 - variable sync change and therefore all Gi categories have changed
 - significance level has changed and therefore categories have changed
 - new randomization for p-vals and therefore categories have changed */
void GetisOrdMapFrame::update(GStatCoordinator* o)
{
	GetisOrdMapCanvas* lc = (GetisOrdMapCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	lc->CreateAndUpdateCategories();
	if (template_legend) template_legend->Recreate();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
    lc->UpdateStatusBar();
}

void GetisOrdMapFrame::closeObserver(GStatCoordinator* o)
{
	wxLogMessage("In GetisOrdMapFrame::closeObserver(GStatCoordinator*)");
	if (gs_coord) {
		gs_coord->removeObserver(this);
		gs_coord = 0;
	}
	Close(true);
}
