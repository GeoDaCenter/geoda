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
	LOG_MSG("Entering GetisOrdMapCanvas::GetisOrdMapCanvas");
	
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
	
	LOG_MSG("Exiting GetisOrdMapCanvas::GetisOrdMapCanvas");
}

GetisOrdMapCanvas::~GetisOrdMapCanvas()
{
	LOG_MSG("In GetisOrdMapCanvas::~GetisOrdMapCanvas");
}

void GetisOrdMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering GetisOrdMapCanvas::DisplayRightClickMenu");
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
	LOG_MSG("Exiting MapCanvas::DisplayRightClickMenu");
}

wxString GetisOrdMapCanvas::GetCanvasTitle()
{
	wxString new_title;
	new_title << (is_gi ? "Gi " : "Gi* ");
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

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by GetisOrd maps */
bool GetisOrdMapCanvas::ChangeMapType(CatClassification::CatClassifType new_theme,
                                      SmoothingType new_smoothing)
{
	LOG_MSG("In GetisOrdMapCanvas::ChangeMapType");
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
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_SPECIFIED_SEED"),
								  gs_coord->IsReuseLastSeed());
}

void GetisOrdMapCanvas::TimeChange()
{
	LOG_MSG("Entering GetisOrdMapCanvas::TimeChange");
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
	LOG_MSG("Exiting GetisOrdMapCanvas::TimeChange");
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
        double stop_sig = 0;
        
        if (gs_coord->GetHasIsolates(t)) {
            num_cats++;
        }
        if (gs_coord->GetHasUndefined(t)) {
            num_cats++;
        }
        
		if (is_clust) {
			num_cats += 3;
		} else {
			num_cats += 6-gs_coord->GetSignificanceFilter();
            
            double sig_cutoff = gs_coord->significance_cutoff;
            int set_perm = gs_coord->permutations;
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
		
		if (is_clust) {
			cat_data.SetCategoryLabel(t, 0, "Not Significant");
			cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
			cat_data.SetCategoryLabel(t, 1, "High");
			cat_data.SetCategoryColor(t, 1, wxColour(255, 0, 0));
			cat_data.SetCategoryLabel(t, 2, "Low");
			cat_data.SetCategoryColor(t, 2, wxColour(0, 0, 255));
            
			if (gs_coord->GetHasIsolates(t) &&
				gs_coord->GetHasUndefined(t))
            {
				isolates_cat = 3;
				undefined_cat = 4;
			} else if (gs_coord->GetHasUndefined(t)) {
				undefined_cat = 3;
			} else if (gs_coord->GetHasIsolates(t)) {
				isolates_cat = 3;
			}
            
		} else {
			// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
			int s_f = gs_coord->GetSignificanceFilter();
			cat_data.SetCategoryLabel(t, 0, "Not Significant");
			cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
	
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
            
			if (gs_coord->GetHasIsolates(t) &&
				gs_coord->GetHasUndefined(t))
            {
				isolates_cat = 6-s_f - skip_cat;
				undefined_cat = 7-s_f - skip_cat;
			} else if (gs_coord->GetHasUndefined(t)) {
				undefined_cat = 6-s_f - skip_cat;
			} else if (gs_coord->GetHasIsolates(t)) {
				isolates_cat = 6-s_f - skip_cat;
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
		
		gs_coord->FillClusterCats(t, is_gi, is_perm, cluster);
		
		if (is_clust) {
			for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
				if (cluster[i] == 0) {
					cat_data.AppendIdToCategory(t, 0, i); // not significant
				} else if (cluster[i] == 3) {
					cat_data.AppendIdToCategory(t, isolates_cat, i);
				} else if (cluster[i] == 4) {
					cat_data.AppendIdToCategory(t, undefined_cat, i);
				} else {
					cat_data.AppendIdToCategory(t, cluster[i], i);
				}
			}
		} else {
			double* p_val = 0;
			if (is_gi && is_perm)
                p_val = gs_coord->pseudo_p_vecs[t];
			if (is_gi && !is_perm)
                p_val = gs_coord->p_vecs[t];
			if (!is_gi && is_perm)
                p_val = gs_coord->pseudo_p_star_vecs[t];
			if (!is_gi && !is_perm)
                p_val = gs_coord->p_star_vecs[t];
			int s_f = gs_coord->GetSignificanceFilter();
			for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
				if (cluster[i] == 0) {
					cat_data.AppendIdToCategory(t, 0, i); // not significant
				} else if (cluster[i] == 3) {
					cat_data.AppendIdToCategory(t, isolates_cat, i);
				} else if (cluster[i] == 4) {
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

void GetisOrdMapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In GetisOrdMapCanvas::TimeSyncVariableToggle");
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
	LOG_MSG("Entering GetisOrdMapFrame::GetisOrdMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
	
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
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
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	Show(true);
	LOG_MSG("Exiting GetisOrdMapFrame::GetisOrdMapFrame");
}

GetisOrdMapFrame::~GetisOrdMapFrame()
{
	LOG_MSG("In GetisOrdMapFrame::~GetisOrdMapFrame");
	if (gs_coord) {
		gs_coord->removeObserver(this);
		gs_coord = 0;
	}
}

void GetisOrdMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In GetisOrdMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("GetisOrdMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void GetisOrdMapFrame::MapMenus()
{
	LOG_MSG("In GetisOrdMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_GETIS_ORD_NEW_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void GetisOrdMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
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
		dlg.m_number->GetValue().ToLong(&num);
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
		wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void GetisOrdMapFrame::SetSigFilterX(int filter)
{
	if (filter == gs_coord->GetSignificanceFilter()) return;
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
	
	std::vector<SaveToTableEntry> data(is_perm ? 3 : 4);
    std::vector<bool> undefs(gs_coord->num_obs, false);
    
    for (size_t i=0; i<gs_coord->x_undefs.size(); i++) {
        for (size_t j=0; j<gs_coord->x_undefs[i].size(); j++) {
            undefs[j] = undefs[j] || gs_coord->x_undefs[i][j];
        }
    }
    
	int data_i = 0;
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
	if (!is_perm) {
		data[data_i].d_val = &z_val;
		data[data_i].label = "z-score";
		data[data_i].field_default = "Z_SCR";
		data[data_i].type = GdaConst::double_type;
        data[data_i].undefined = &undefs;
		data_i++;
	}
	data[data_i].d_val = &p_val;
	data[data_i].label = p_label;
	data[data_i].field_default = p_field_default;
	data[data_i].type = GdaConst::double_type;
    data[data_i].undefined = &undefs;
	data_i++;
	
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
	LOG_MSG("Entering GetisOrdMapFrame::OnSelectCores");
		
	std::vector<bool> elem(gs_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<wxInt64> c_val;
	gs_coord->FillClusterCats(ts, is_gi, is_perm, c_val);

	// add all cores to elem list.
	for (int i=0; i<gs_coord->num_obs; i++) {
		if (c_val[i] == 1 || c_val[i] == 2) elem[i] = true;
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting GetisOrdMapFrame::OnSelectCores");
}

void GetisOrdMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapFrame::OnSelectNeighborsOfCores");
	
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
	
	LOG_MSG("Exiting GetisOrdMapFrame::OnSelectNeighborsOfCores");
}

void GetisOrdMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	LOG_MSG("Entering GetisOrdMapFrame::OnSelectCoresAndNeighbors");
	
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
	
	LOG_MSG("Exiting GetisOrdMapFrame::OnSelectCoresAndNeighbors");
}

void GetisOrdMapFrame::OnAddNeighborToSelection(wxCommandEvent& event)
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

void GetisOrdMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
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
}

void GetisOrdMapFrame::closeObserver(GStatCoordinator* o)
{
	LOG_MSG("In GetisOrdMapFrame::closeObserver(GStatCoordinator*)");
	if (gs_coord) {
		gs_coord->removeObserver(this);
		gs_coord = 0;
	}
	Close(true);
}
