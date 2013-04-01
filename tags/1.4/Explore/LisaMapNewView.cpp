/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include <vector>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "LisaCoordinator.h"
#include "LisaMapNewView.h"

IMPLEMENT_CLASS(LisaMapNewCanvas, MapNewCanvas)
BEGIN_EVENT_TABLE(LisaMapNewCanvas, MapNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

LisaMapNewCanvas::LisaMapNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
								   Project* project,
								   LisaCoordinator* lisa_coordinator,
								   CatClassification::CatClassifType theme_type_s,
								   bool isBivariate, bool isEBRate,
								   const wxPoint& pos, const wxSize& size)
: MapNewCanvas(parent, t_frame, project, CatClassification::no_theme,
			   no_smoothing, pos, size),
lisa_coord(lisa_coordinator),
is_clust(theme_type_s==CatClassification::lisa_categories),
is_bi(isBivariate), is_rate(isEBRate)
{
	LOG_MSG("Entering LisaMapNewCanvas::LisaMapNewCanvas");

	cat_classif_def.cat_classif_type = theme_type_s;
	// must set var_info times from LisaCoordinator initially
	var_info = lisa_coordinator->var_info;
	CreateAndUpdateCategories();
	
	LOG_MSG("Exiting LisaMapNewCanvas::LisaMapNewCanvas");
}

LisaMapNewCanvas::~LisaMapNewCanvas()
{
	LOG_MSG("In LisaMapNewCanvas::~LisaMapNewCanvas");
}

void LisaMapNewCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering LisaMapNewCanvas::DisplayRightClickMenu");
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISAMAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting LisaMapNewCanvas::DisplayRightClickMenu");
}

wxString LisaMapNewCanvas::GetCanvasTitle()
{
	wxString lisa_t;
	if (is_clust && !is_bi) lisa_t = " LISA Cluster Map";
	if (is_clust && is_bi) lisa_t = " BiLISA Cluster Map";
	if (!is_clust && !is_bi) lisa_t = " LISA Significance Map";
	if (!is_clust && is_bi) lisa_t = " BiLISA Significance Map";
	
	wxString field_t;
	if (is_bi) {
		field_t << GetNameWithTime(0) << " w/ " << GetNameWithTime(1);
	} else {
		field_t << "I_" << GetNameWithTime(0);
	}
	if (is_rate) {
		field_t << "EB Rate: " << GetNameWithTime(0);
		field_t << " / " << GetNameWithTime(1);
	}
	
	wxString ret;
	ret << lisa_t << ": " << lisa_coord->weight_name << ", ";
	ret << field_t << " (" << lisa_coord->permutations << " perm)";
	return ret;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by LISA maps */
bool LisaMapNewCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme,
									 SmoothingType new_map_smoothing)
{
	LOG_MSG("In LisaMapNewCanvas::ChangeMapType");
	return false;
}

void LisaMapNewCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	MapNewCanvas::SetCheckMarks(menu);
	
	int sig_filter = lisa_coord->GetSignificanceFilter();
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
								  sig_filter == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
								  sig_filter == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
								  sig_filter == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
								  sig_filter == 4);
}

void LisaMapNewCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering LisaMapNewCanvas::TitleOrTimeChange");
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetGridBase()->curr_time_step;
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
	LOG_MSG("Exiting LisaMapNewCanvas::TitleOrTimeChange");
}

/** Update Categories based on info in LisaCoordinator */
void LisaMapNewCanvas::CreateAndUpdateCategories()
{
	SyncVarInfoFromCoordinator();
	cat_data.CreateEmptyCategories(num_time_vals, num_obs);
	
	for (int t=0; t<num_time_vals; t++) {
		if (!map_valid[t]) break;
		
		int undefined_cat = -1;
		int isolates_cat = -1;
		int num_cats = 0;
		if (lisa_coord->GetHasIsolates(t)) num_cats++;
		if (lisa_coord->GetHasUndefined(t)) num_cats++;
		if (is_clust) {
			num_cats += 5;
		} else {
			num_cats += 6-lisa_coord->GetSignificanceFilter();
		}
		cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
		
		if (is_clust) {
			cat_data.SetCategoryLabel(t, 0, "Not Significant");
			cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
			cat_data.SetCategoryLabel(t, 1, "High-High");
			cat_data.SetCategoryColor(t, 1, wxColour(255, 0, 0));
			cat_data.SetCategoryLabel(t, 2, "Low-Low");
			cat_data.SetCategoryColor(t, 2, wxColour(0, 0, 255));
			cat_data.SetCategoryLabel(t, 3, "Low-High");
			cat_data.SetCategoryColor(t, 3, wxColour(150, 150, 255));
			cat_data.SetCategoryLabel(t, 4, "High-Low");
			cat_data.SetCategoryColor(t, 4, wxColour(255, 150, 150));
			if (lisa_coord->GetHasIsolates(t) &&
				lisa_coord->GetHasUndefined(t)) {
				isolates_cat = 5;
				undefined_cat = 6;
			} else if (lisa_coord->GetHasUndefined(t)) {
				undefined_cat = 5;
			} else if (lisa_coord->GetHasIsolates(t)) {
				isolates_cat = 5;
			}
		} else {
			// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
			int s_f = lisa_coord->GetSignificanceFilter();
			cat_data.SetCategoryLabel(t, 0, "Not Significant");
			cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));

			cat_data.SetCategoryLabel(t, 5-s_f, "p = 0.0001");
			cat_data.SetCategoryColor(t, 5-s_f, wxColour(1, 70, 3));
			if (s_f <= 3) {
				cat_data.SetCategoryLabel(t, 4-s_f, "p = 0.001");
				cat_data.SetCategoryColor(t, 4-s_f, wxColour(3, 116, 6));	
			}
			if (s_f <= 2) {
				cat_data.SetCategoryLabel(t, 3-s_f, "p = 0.01");
				cat_data.SetCategoryColor(t, 3-s_f, wxColour(6, 196, 11));	
			}
			if (s_f <= 1) {
				cat_data.SetCategoryLabel(t, 2-s_f, "p = 0.05");
				cat_data.SetCategoryColor(t, 2-s_f, wxColour(75, 255, 80));
			}
			if (lisa_coord->GetHasIsolates(t) &&
				lisa_coord->GetHasUndefined(t)) {
				isolates_cat = 6-s_f;
				undefined_cat = 7-s_f;
			} else if (lisa_coord->GetHasUndefined(t)) {
				undefined_cat = 6-s_f;
			} else if (lisa_coord->GetHasIsolates(t)) {
				isolates_cat = 6-s_f;
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
		
		double cuttoff = lisa_coord->significance_cutoff;
		double* p = lisa_coord->sig_local_moran_vecs[t];
		int* cluster = lisa_coord->cluster_vecs[t];
		int* sigCat = lisa_coord->sig_cat_vecs[t];
		
		if (is_clust) {
			for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
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
		} else {
			int s_f = lisa_coord->GetSignificanceFilter();
			for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
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
void LisaMapNewCanvas::SyncVarInfoFromCoordinator()
{
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = lisa_coord->var_info;
	for (int t=0; t<var_info.size(); t++) var_info[t].time = my_times[t];
	is_any_time_variant = lisa_coord->is_any_time_variant;
	is_any_sync_with_global_time = lisa_coord->is_any_sync_with_global_time;
	ref_var_index = lisa_coord->ref_var_index;
	num_time_vals = lisa_coord->num_time_vals;
	map_valid = lisa_coord->map_valid;
	map_error_message = lisa_coord->map_error_message;
}

void LisaMapNewCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In LisaMapNewCanvas::TimeSyncVariableToggle");
	lisa_coord->var_info[var_index].sync_with_global_time =
		!lisa_coord->var_info[var_index].sync_with_global_time;
	for (int i=0; i<var_info.size(); i++) {
		lisa_coord->var_info[i].time = var_info[i].time;
	}
	lisa_coord->VarInfoAttributeChange();
	lisa_coord->InitFromVarInfo();
	lisa_coord->notifyObservers();
}


IMPLEMENT_CLASS(LisaMapNewFrame, MapNewFrame)
	BEGIN_EVENT_TABLE(LisaMapNewFrame, MapNewFrame)
	EVT_ACTIVATE(LisaMapNewFrame::OnActivate)
END_EVENT_TABLE()

LisaMapNewFrame::LisaMapNewFrame(wxFrame *parent, Project* project,
								 LisaCoordinator* lisa_coordinator,
								 bool isClusterMap, bool isBivariate,
								 bool isEBRate,
								 const wxPoint& pos, const wxSize& size,
								 const long style)
: MapNewFrame(parent, project, pos, size, style),
lisa_coord(lisa_coordinator)
{
	LOG_MSG("Entering LisaMapNewFrame::LisaMapNewFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
	
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this);
	splitter_win->SetMinimumPaneSize(10);
	
	template_canvas = new LisaMapNewCanvas(splitter_win, this, project,
										   lisa_coordinator,
										   (isClusterMap ?
											CatClassification::lisa_categories :
											CatClassification::lisa_significance),
										   isBivariate, isEBRate,
										   wxDefaultPosition,
										   wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	
	template_legend = new MapNewLegend(splitter_win, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	
	splitter_win->SplitVertically(template_legend, template_canvas,
								  GeoDaConst::map_default_legend_width);
	
	lisa_coord->registerObserver(this);
	
	Show(true);
	LOG_MSG("Exiting LisaMapNewFrame::LisaMapNewFrame");
}

LisaMapNewFrame::~LisaMapNewFrame()
{
	LOG_MSG("In LisaMapNewFrame::~LisaMapNewFrame");
	lisa_coord->removeObserver(this);
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void LisaMapNewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In LisaMapNewFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("LisaMapNewFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void LisaMapNewFrame::MapMenus()
{
	LOG_MSG("In LisaMapNewFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_LISAMAP_NEW_VIEW_MENU_OPTIONS");
	((MapNewCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapNewCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void LisaMapNewFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("LisaMapNewFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((LisaMapNewCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void LisaMapNewFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LisaMapNewFrame::RanXPer(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	lisa_coord->permutations = permutation;
	lisa_coord->CalcPseudoP();
	lisa_coord->notifyObservers();
}

void LisaMapNewFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void LisaMapNewFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void LisaMapNewFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void LisaMapNewFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void LisaMapNewFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		dlg.m_number->GetValue().ToLong(&num);
		RanXPer(num);
	}
}

void LisaMapNewFrame::SetSigFilterX(int filter)
{
	if (filter == lisa_coord->GetSignificanceFilter()) return;
	lisa_coord->SetSignificanceFilter(filter);
	lisa_coord->notifyObservers();
	UpdateOptionMenuItems();
}

void LisaMapNewFrame::OnSigFilter05(wxCommandEvent& event)
{
	SetSigFilterX(1);
}

void LisaMapNewFrame::OnSigFilter01(wxCommandEvent& event)
{
	SetSigFilterX(2);
}

void LisaMapNewFrame::OnSigFilter001(wxCommandEvent& event)
{
	SetSigFilterX(3);
}

void LisaMapNewFrame::OnSigFilter0001(wxCommandEvent& event)
{
	SetSigFilterX(4);
}

void LisaMapNewFrame::OnSaveLisa(wxCommandEvent& event)
{
	int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
	std::vector<SaveToTableEntry> data(3);
	std::vector<double> tempLocalMoran(lisa_coord->num_obs);
	for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
		tempLocalMoran[i] = lisa_coord->local_moran_vecs[t][i];
	}
	data[0].d_val = &tempLocalMoran;
	data[0].label = "Lisa Indices";
	data[0].field_default = "LISA_I";
	data[0].type = GeoDaConst::double_type;
	
	double cuttoff = lisa_coord->significance_cutoff;
	double* p = lisa_coord->sig_local_moran_vecs[t];
	int* cluster = lisa_coord->cluster_vecs[t];
	std::vector<wxInt64> clust(lisa_coord->num_obs);
	for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
		if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
			clust[i] = 0; // not significant
		} else {
			clust[i] = cluster[i];
		}
	}
	data[1].l_val = &clust;
	data[1].label = "Clusters";
	data[1].field_default = "LISA_CL";
	data[1].type = GeoDaConst::long64_type;
	
	std::vector<double> sig(lisa_coord->num_obs);
	for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
		sig[i] = p[i];
	}
	
	data[2].d_val = &sig;
	data[2].label = "Significances";
	data[2].field_default = "LISA_P";
	data[2].type = GeoDaConst::double_type;	
	
	SaveToTableDlg dlg(project->GetGridBase(), this, data,
					   "Save Results: LISA",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void LisaMapNewFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
	HighlightState* highlight_state = project->highlight_state;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	for (int i=0; i<lisa_coord->num_obs; i++) {
		if (!hs[i] && elem[i]) {
			nh[total_newly_selected++] = i;
		} else if (hs[i] && !elem[i]) {
			nuh[total_newly_unselected++] = i;
		}
	}
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetEventType(HighlightState::delta);
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		highlight_state->notifyObservers();
	}
}

void LisaMapNewFrame::OnSelectCores(wxCommandEvent& event)
{
	LOG_MSG("Entering LisaMapNewFrame::OnSelectCores");
	
	std::vector<bool> elem(lisa_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = lisa_coord->cluster_vecs[ts];
	int* sig_cat = lisa_coord->sig_cat_vecs[ts];
	int sf = lisa_coord->significance_filter;
	
	// add all cores to elem list.
	for (int i=0; i<lisa_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = true;
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting LisaMapNewFrame::OnSelectCores");
}

void LisaMapNewFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	LOG_MSG("Entering LisaMapNewFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(lisa_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = lisa_coord->cluster_vecs[ts];
	int* sig_cat = lisa_coord->sig_cat_vecs[ts];
	int sf = lisa_coord->significance_filter;
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<lisa_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = true;
			const GalElement& e = lisa_coord->W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<lisa_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = false;
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting LisaMapNewFrame::OnSelectNeighborsOfCores");
}

void LisaMapNewFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	LOG_MSG("Entering LisaMapNewFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(lisa_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = lisa_coord->cluster_vecs[ts];
	int* sig_cat = lisa_coord->sig_cat_vecs[ts];
	int sf = lisa_coord->significance_filter;
	
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<lisa_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 && sig_cat[i] >= sf) {
			elem[i] = true;
			const GalElement& e = lisa_coord->W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e.elt(j)] = true;
			}
		}
	}
	CoreSelectHelper(elem);
	
	LOG_MSG("Exiting LisaMapNewFrame::OnSelectCoresAndNeighbors");
}

/** Called by LisaCoordinator to notify that state has changed.  State changes
 can include:
   - variable sync change and therefore all lisa categories have changed
   - significance level has changed and therefore categories have changed
   - new randomization for p-vals and therefore categories have changed */
void LisaMapNewFrame::update(LisaCoordinator* o)
{
	LisaMapNewCanvas* lc = (LisaMapNewCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	lc->CreateAndUpdateCategories();
	if (template_legend) template_legend->Refresh();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
}
