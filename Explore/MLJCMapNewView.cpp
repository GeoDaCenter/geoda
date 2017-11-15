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

#include "MLJCMapNewView.h"

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

#include "ConditionalClusterMapView.h"
#include "MLJCCoordinator.h"
#include "MLJCMapNewView.h"

IMPLEMENT_CLASS(MLJCMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(MLJCMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()


MLJCMapCanvas::MLJCMapCanvas(wxWindow *parent, TemplateFrame* t_frame, bool is_clust_p, Project* project, JCCoordinator* gs_coordinator, const wxPoint& pos, const wxSize& size)
: MapCanvas(parent, t_frame, project, std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0), CatClassification::no_theme, no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
gs_coord(gs_coordinator), is_clust(is_clust_p)
{
	LOG_MSG("Entering MLJCMapCanvas::MLJCMapCanvas");

    str_sig = _("Not Significant");
    str_low = _("No Colocation");
    str_med = _("Has Colocation");
    str_high = _("Colocation Cluster");
    str_undefined = _("Undefined");
    str_neighborless = _("Neighborless");
    str_p005 = _("p = 0.05");
    str_p001 = _("p = 0.01");
    str_p0001 = _("p = 0.001");
    str_p00001 = _("p = 0.00001");
    
    SetPredefinedColor(str_sig, wxColour(240, 240, 240));
    SetPredefinedColor(str_high, wxColour(255, 0, 0));
    SetPredefinedColor(str_med, wxColour(0, 255, 0));
    SetPredefinedColor(str_low, wxColour(0, 0, 255));
    SetPredefinedColor(str_undefined, wxColour(70, 70, 70));
    SetPredefinedColor(str_neighborless, wxColour(140, 140, 140));
    SetPredefinedColor(str_p005, wxColour(75, 255, 80));
    SetPredefinedColor(str_p001, wxColour(6, 196, 11));
    SetPredefinedColor(str_p0001, wxColour(3, 116, 6));
    SetPredefinedColor(str_p00001, wxColour(1, 70, 3));

	if (is_clust) {
		cat_classif_def.cat_classif_type = CatClassification::local_join_count_categories;
	} else {
		cat_classif_def.cat_classif_type = CatClassification::local_join_count_significance;
	}
	
	// must set var_info times from JCCoordinator initially
	var_info = gs_coord->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0, sz=var_info.size(); t<sz; ++t) {
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	CreateAndUpdateCategories();
	UpdateStatusBar();
	LOG_MSG("Exiting MLJCMapCanvas::MLJCMapCanvas");
}

MLJCMapCanvas::~MLJCMapCanvas()
{
	LOG_MSG("In MLJCMapCanvas::~MLJCMapCanvas");
}

void MLJCMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering MLJCMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((MLJCMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting MLJCMapCanvas::DisplayRightClickMenu");
}

wxString MLJCMapCanvas::GetCanvasTitle()
{
	wxString new_title = _("Bivariate Local Join Count ");
    
	new_title << (is_clust ? "Cluster" : "Significance") << " Map ";
	new_title << "(" << gs_coord->weight_name << "): ";
	new_title << GetNameWithTime(0);
	new_title << wxString::Format(", pseudo p (%d perm)", gs_coord->permutations);
    
	return new_title;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by MLJC maps */
bool MLJCMapCanvas::ChangeMapType(CatClassification::CatClassifType new_theme, SmoothingType new_smoothing)
{
	LOG_MSG("In MLJCMapCanvas::ChangeMapType");
	return false;
}

void MLJCMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	MapCanvas::SetCheckMarks(menu);
	
	int sig_filter = ((MLJCMapFrame*) template_frame)->GetJCCoordinator()->GetSignificanceFilter();
	
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

void MLJCMapCanvas::TimeChange()
{
	LOG_MSG("Entering MLJCMapCanvas::TimeChange");
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
	LOG_MSG("Exiting MLJCMapCanvas::TimeChange");
}

/** Update Categories based on info in JCCoordinator */
void MLJCMapCanvas::CreateAndUpdateCategories()
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
        double stop_sig = 0;
        
        if (gs_coord->GetHasIsolates(t)) {
            num_cats++;
        }
        if (gs_coord->GetHasUndefined(t)) {
            num_cats++;
        }
        
		if (is_clust) {
			num_cats += 4; // 0 not sig 1 no loc 2 has loc 3 loc cluster
            
		} else {
            int set_perm = gs_coord->permutations;
            stop_sig = 1.0 / (1.0 + set_perm);
            double sig_cutoff = gs_coord->significance_cutoff;
            
            if (gs_coord->GetSignificanceFilter() < 0) {
                // user specified cutoff
                num_cats += 2;
            } else {
                num_cats += 6 - gs_coord->GetSignificanceFilter();
                
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
		}
		cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
	
		if (is_clust) {
			cat_data.SetCategoryLabel(t, 0, str_sig);
			cat_data.SetCategoryColor(t, 0, lbl_color_dict[str_sig]);
			cat_data.SetCategoryLabel(t, 1, str_low);
			cat_data.SetCategoryColor(t, 1, lbl_color_dict[str_low]);
			cat_data.SetCategoryLabel(t, 2, str_med);
			cat_data.SetCategoryColor(t, 2, lbl_color_dict[str_med]);
			cat_data.SetCategoryLabel(t, 3, str_high);
			cat_data.SetCategoryColor(t, 3, lbl_color_dict[str_high]);
            
			if (gs_coord->GetHasIsolates(t) && gs_coord->GetHasUndefined(t)) {
                isolates_cat = 4;
                undefined_cat = 5;
			} else if (gs_coord->GetHasUndefined(t)) {
                undefined_cat = 4;
			} else if (gs_coord->GetHasIsolates(t)) {
                isolates_cat = 4;
			}
            
		} else {
			cat_data.SetCategoryLabel(t, 0, str_sig);
			cat_data.SetCategoryColor(t, 0, lbl_color_dict[str_sig]);
	
            if (gs_coord->GetSignificanceFilter() < 0) {
                // user specified cutoff
                wxString lbl = wxString::Format("p = %g", gs_coord->significance_cutoff);
                cat_data.SetCategoryLabel(t, 1, lbl);
                cat_data.SetCategoryColor(t, 1, wxColour(3, 116, 6));
                
                if (gs_coord->GetHasIsolates(t) &&
                    gs_coord->GetHasUndefined(t))
                {
                    isolates_cat = 2;
                    undefined_cat = 3;
                } else if (gs_coord->GetHasUndefined(t)) {
                    undefined_cat = 2;
                } else if (gs_coord->GetHasIsolates(t)) {
                    isolates_cat = 2;
                }
                
            } else {
                int s_f = gs_coord->GetSignificanceFilter();
                int set_perm = gs_coord->permutations;
                stop_sig = 1.0 / (1.0 + set_perm);
                
                wxString def_cats[4] = {str_p005, str_p001, str_p0001, str_p00001};
                double def_cutoffs[4] = {0.05, 0.01, 0.001, 0.0001};
                
                int cat_idx = 1;
                for (int j=s_f-1; j < 4; j++) {
                    if (def_cutoffs[j] >= stop_sig) {
                        cat_data.SetCategoryLabel(t, cat_idx, def_cats[j]);
                        cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[def_cats[j]]);
                    }
                }
                if (gs_coord->GetHasIsolates(t) &&
                    gs_coord->GetHasUndefined(t)) {
                    isolates_cat = cat_idx++;
                    undefined_cat = cat_idx++;
                    
                } else if (gs_coord->GetHasUndefined(t)) {
                    undefined_cat = cat_idx++;
                    
                } else if (gs_coord->GetHasIsolates(t)) {
                    isolates_cat = cat_idx++;
                }
            }
		}
        
		if (undefined_cat != -1) {
			cat_data.SetCategoryLabel(t, undefined_cat, str_undefined);
            cat_data.SetCategoryColor(t, undefined_cat, lbl_color_dict[str_undefined]);
		}
		if (isolates_cat != -1) {
			cat_data.SetCategoryLabel(t, isolates_cat, str_neighborless);
			cat_data.SetCategoryColor(t, isolates_cat, lbl_color_dict[str_neighborless]);
		}
		
		gs_coord->FillClusterCats(t, cluster);
		
		if (is_clust) {
			for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
				if (cluster[i] == 0) {
					cat_data.AppendIdToCategory(t, 0, i); // not significant
				} else if (cluster[i] == 4) {
					cat_data.AppendIdToCategory(t, isolates_cat, i);
				} else if (cluster[i] == 5) {
					cat_data.AppendIdToCategory(t, undefined_cat, i);
				} else {
					cat_data.AppendIdToCategory(t, cluster[i], i);
				}
			}
		} else {
            double* p_val = gs_coord->pseudo_p_vecs[t];
            
            if (gs_coord->GetSignificanceFilter() < 0) {
                // user specified cutoff
                int s_f = 1;
                double sig_cutoff = gs_coord->significance_cutoff;
                for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
                    if (cluster[i] == 4) {
                        cat_data.AppendIdToCategory(t, isolates_cat, i);
                    } else if (cluster[i] == 5) {
                        cat_data.AppendIdToCategory(t, undefined_cat, i);
                    } else if (cluster[i] == 0) {
                        cat_data.AppendIdToCategory(t, 0, i); // not significant
                    } else {
                        if (p_val[i] <= sig_cutoff) {
                            cat_data.AppendIdToCategory(t, 1, i);
                        } else {
                            cat_data.AppendIdToCategory(t, 0, i); // not significant
                        }
                    }

                }
            } else {
                int s_f = gs_coord->GetSignificanceFilter();
                for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
                    if (cluster[i] == 0) {
                        cat_data.AppendIdToCategory(t, 0, i); // not significant
                    } else if (cluster[i] == 4) {
                        cat_data.AppendIdToCategory(t, isolates_cat, i);
                    } else if (cluster[i] == 5) {
                        cat_data.AppendIdToCategory(t, undefined_cat, i);
                    } else if (p_val[i] <= 0.0001) {
                        cat_data.AppendIdToCategory(t, 5-s_f, i);
                    } else if (p_val[i] <= 0.001) {
                        cat_data.AppendIdToCategory(t, 4-s_f, i);
                    } else if (p_val[i] <= 0.01) {
                        cat_data.AppendIdToCategory(t, 3-s_f, i);
                    } else if (p_val[i] <= 0.05) {
                        cat_data.AppendIdToCategory(t, 2-s_f, i);
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

void MLJCMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = 0;
    if (template_frame) {
        sb = template_frame->GetStatusBar();
    }
    if (!sb)
        return;
    wxString s;
    s << "#obs=" << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
        s << "#selected=" << highlight_state->GetTotalHighlighted()<< "  ";
    }
    if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << "hover obs " << hover_obs[0]+1;
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << "obs " << hover_obs[1]+1;
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << "obs " << hover_obs[2]+1;
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

void MLJCMapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In MLJCMapCanvas::TimeSyncVariableToggle");
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
void MLJCMapCanvas::SyncVarInfoFromCoordinator()
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

IMPLEMENT_CLASS(MLJCMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(MLJCMapFrame, MapFrame)
	EVT_ACTIVATE(MLJCMapFrame::OnActivate)
END_EVENT_TABLE()

MLJCMapFrame::MLJCMapFrame(wxFrame *parent, Project* project, JCCoordinator* gs_coordinator, bool isClusterMap, const wxPoint& pos, const wxSize& size, const long style)
: MapFrame(parent, project, pos, size, style),
gs_coord(gs_coordinator),
is_clust(isClusterMap)
{
	wxLogMessage("Entering MLJCMapFrame::MLJCMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);

	DisplayStatusBar(true);
    
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new MLJCMapCanvas(rpanel, this, is_clust, project, gs_coordinator);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
    wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new MapNewLegend(lpanel, template_canvas, wxPoint(0,0), wxSize(0,0));
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
    SetAutoLayout(true);
   
	gs_coord->registerObserver(this);
	
	SetTitle(template_canvas->GetCanvasTitle());
	Show(true);
	wxLogMessage("Exiting MLJCMapFrame::MLJCMapFrame");
}

MLJCMapFrame::~MLJCMapFrame()
{
	wxLogMessage("In MLJCMapFrame::~MLJCMapFrame");
	if (gs_coord) {
		gs_coord->removeObserver(this);
		gs_coord = 0;
	}
}

void MLJCMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In MLJCMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("MLJCMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void MLJCMapFrame::MapMenus()
{
	LOG_MSG("In MLJCMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void MLJCMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("MLJCMapFrame::UpdateOptionMenuItems: Options menu not found");
	} else {
		((MLJCMapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void MLJCMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void MLJCMapFrame::RanXPer(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	gs_coord->permutations = permutation;
	gs_coord->CalcPseudoP();
	gs_coord->notifyObservers();
}

void MLJCMapFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void MLJCMapFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void MLJCMapFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void MLJCMapFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void MLJCMapFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
        wxString input = dlg.m_number->GetValue();
        
        wxLogMessage(input);
        
        input.ToLong(&num);
		RanXPer(num);
	}
}

void MLJCMapFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
	gs_coord->SetReuseLastSeed(!gs_coord->IsReuseLastSeed());
}

void MLJCMapFrame::OnSpecifySeedDlg(wxCommandEvent& event)
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
    
    wxLogMessage(dlg_val);
    
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
		wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void MLJCMapFrame::SetSigFilterX(int filter)
{
	if (filter == gs_coord->GetSignificanceFilter())
        return;
	gs_coord->SetSignificanceFilter(filter);
	gs_coord->notifyObservers();
	UpdateOptionMenuItems();
}

void MLJCMapFrame::OnSigFilter05(wxCommandEvent& event)
{
	SetSigFilterX(1);
}

void MLJCMapFrame::OnSigFilter01(wxCommandEvent& event)
{
	SetSigFilterX(2);
}

void MLJCMapFrame::OnSigFilter001(wxCommandEvent& event)
{
	SetSigFilterX(3);
}

void MLJCMapFrame::OnSigFilter0001(wxCommandEvent& event)
{
	SetSigFilterX(4);
}

void MLJCMapFrame::OnSigFilterSetup(wxCommandEvent& event)
{
    MLJCMapCanvas* lc = (MLJCMapCanvas*)template_canvas;
    int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
    double* p_val_t = gs_coord->pseudo_p_vecs[t];
    int n = gs_coord->num_obs;
    
    wxString ttl = _("Inference Settings");
    ttl << "  (" << gs_coord->permutations << " perm)";
    
    double user_sig = gs_coord->significance_cutoff;
    if (gs_coord->GetSignificanceFilter()<0) user_sig = gs_coord->user_sig_cutoff;
  
    int new_n = 0;
    for (int i=0; i<gs_coord->num_obs; i++) {
        if (gs_coord->x_vecs[t][i] == 1) {
            new_n += 1;
        }
    }
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



void MLJCMapFrame::OnSaveMLJC(wxCommandEvent& event)
{
    int t = 0;//template_canvas->cat_data.GetCurrentCanvasTmStep();
	wxString title = _("Save Results: Bivariate Local Join Count stats, ");
    title += wxString::Format("pseudo p (%d perm), ", gs_coord->permutations);

    int num_obs = gs_coord->num_obs;
    std::vector<bool> p_undefs(num_obs, false);
    
    double* g_val_t = gs_coord->G_vecs[t];
	std::vector<double> g_val(num_obs);
    for (int i=0; i<num_obs; i++) g_val[i] = g_val_t[i];
    
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(t, c_val);
    for (int i=0; i<num_obs; i++) {
        if (c_val[i] < 1 || c_val[i] > 3)
            p_undefs[i] = true;
    }
	wxString c_label = "Cluster Category";
	wxString c_field_default = "C_ID";
	
	double* jc_t = gs_coord->G_vecs[t];
	std::vector<wxInt64> jc_val(num_obs);
    for (int i=0; i<num_obs; i++) jc_val[i] = (wxInt64)jc_t[i];
	wxString jc_label = "Local Joint Count";
	wxString jc_field_default =  "JC";
    
	double* pp_val_t = gs_coord->pseudo_p_vecs[t];
	std::vector<double> pp_val(num_obs);
    for (int i=0; i<num_obs; i++) {
        pp_val[i] = pp_val_t[i];
    }
	wxString pp_label = "Pseudo p-value";
	wxString pp_field_default =  "P_VAL";
	
	double* p_val_t = gs_coord->p_vecs[t];
	std::vector<double> p_val(num_obs);
	for (int i=0; i<num_obs; i++) p_val[i] = p_val_t[i];
	wxString p_label = "Exact Inference";
	wxString p_field_default =  "EP_VAL";
    
    int num_data = 8;
    
	std::vector<SaveToTableEntry> data(num_data);
    std::vector<bool> undefs = gs_coord->x_undefs[t];
   
    int data_i = 0;
    data[data_i].l_val = &c_val;
    data[data_i].label = c_label;
    data[data_i].field_default = c_field_default;
    data[data_i].type = GdaConst::long64_type;
    data[data_i].undefined = &undefs;
    data_i++;
    
    data[data_i].l_val = &jc_val;
    data[data_i].label = jc_label;
    data[data_i].field_default = jc_field_default;
    data[data_i].type = GdaConst::long64_type;
    data[data_i].undefined = &undefs;
    data_i++;
    
    data[data_i].l_val = &gs_coord->num_neighbors[t];
    data[data_i].label = "Number of Neighbors";
    data[data_i].field_default = "NN";
    data[data_i].type = GdaConst::long64_type;
    data[data_i].undefined = &undefs;
    data_i++;
    
    data[data_i].l_val = &gs_coord->num_neighbors_x1[t];
    data[data_i].label = "Number of Neighbors with Value 1 (X)";
    data[data_i].field_default = "NNX_1";
    data[data_i].type = GdaConst::long64_type;
    data[data_i].undefined = &undefs;
    data_i++;
    
    data[data_i].l_val = &gs_coord->num_neighbors_y1[t];
    data[data_i].label = "Number of Neighbors with Value 1 (Z)";
    data[data_i].field_default = "NNZ_1";
    data[data_i].type = GdaConst::long64_type;
    data[data_i].undefined = &undefs;
    data_i++;
    
    data[data_i].l_val = &gs_coord->num_neighbors_xy1[t];
    data[data_i].label = "Number of Neighbors both with Value 1";
    data[data_i].field_default = "NN_1";
    data[data_i].type = GdaConst::long64_type;
    data[data_i].undefined = &undefs;
    data_i++;
    
    data[data_i].d_val = &pp_val;
    data[data_i].label = pp_label;
    data[data_i].field_default = pp_field_default;
    data[data_i].type = GdaConst::double_type;
    data[data_i].undefined = &p_undefs;
    data_i++;
    
    data[data_i].d_val = &p_val;
    data[data_i].label = p_label;
    data[data_i].field_default = p_field_default;
    data[data_i].type = GdaConst::double_type;
    data[data_i].undefined = &p_undefs;
    data_i++;
	
	SaveToTableDlg dlg(project, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void MLJCMapFrame::CoreSelectHelper(const std::vector<bool>& elem)
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

void MLJCMapFrame::OnSelectCores(wxCommandEvent& event)
{
	wxLogMessage("Entering MLJCMapFrame::OnSelectCores");
		
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, c_val);

	// add all cores to elem list.
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) elem[i] = true;
	}
	CoreSelectHelper(elem);
	
	wxLogMessage("Exiting MLJCMapFrame::OnSelectCores");
}

void MLJCMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	wxLogMessage("Entering MLJCMapFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, c_val);
	
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
	
	wxLogMessage("Exiting MLJCMapFrame::OnSelectNeighborsOfCores");
}

void MLJCMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	wxLogMessage("Entering MLJCMapFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, c_val);
	
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
	
	wxLogMessage("Exiting MLJCMapFrame::OnSelectCoresAndNeighbors");
}

void MLJCMapFrame::OnAddNeighborToSelection(wxCommandEvent& event)
{
    int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
    GalWeight* gal_weights = gs_coord->Gal_vecs_orig[ts];
    
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

void MLJCMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
                            _("Conditional G Cluster Map Variables"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"));
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    
	MLJCMapCanvas* lc = (MLJCMapCanvas*) template_canvas;
    wxString title = lc->GetCanvasTitle();
    //ConditionalClusterMapFrame* subframe = new ConditionalClusterMapFrame(this, project, dlg.var_info, dlg.col_ids, gs_coord);
}

/** Called by JCCoordinator to notify that state has changed.  State changes
 can include:
 - variable sync change and therefore all Gi categories have changed
 - significance level has changed and therefore categories have changed
 - new randomization for p-vals and therefore categories have changed */
void MLJCMapFrame::update(JCCoordinator* o)
{
	MLJCMapCanvas* lc = (MLJCMapCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	lc->CreateAndUpdateCategories();
	if (template_legend) template_legend->Recreate();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
    lc->UpdateStatusBar();
}

void MLJCMapFrame::closeObserver(JCCoordinator* o)
{
	wxLogMessage("In MLJCMapFrame::closeObserver(JCCoordinator*)");
	if (gs_coord) {
		gs_coord->removeObserver(this);
		gs_coord = 0;
	}
	Close(true);
}
