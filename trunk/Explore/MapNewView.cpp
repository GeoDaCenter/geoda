/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#include <algorithm> // std::sort
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../DialogTools/SelectWeightDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../FramesManager.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../ShapeOperations/WeightsManager.h"
#include "MapNewView.h"

IMPLEMENT_CLASS(MapNewCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(MapNewCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
	//EVT_KEY_DOWN(TemplateCanvas::OnKeyDown)
END_EVENT_TABLE()

MapNewCanvas::MapNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
						   Project* project_s,
						   const std::vector<GeoDaVarInfo>& var_info_s,
						   const std::vector<int>& col_ids_s,
						   CatClassification::CatClassifType theme_type,
						   SmoothingType smoothing_type_s,
						   int num_categories_s,
						   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, true, true),
project(project_s), num_obs(project_s->GetNumRecords()),
num_time_vals(1), highlight_state(project_s->GetHighlightState()),
custom_classif_state(0), data(0), var_info(0),
table_int(project_s->GetTableInt()),
smoothing_type(no_smoothing), gal_weight(0),
is_rate_smoother(false), full_map_redraw_needed(true),
display_mean_centers(false), display_centroids(false),
display_voronoi_diagram(false), voronoi_diagram_duplicates_exist(false),
num_categories(num_categories_s)
{
	using namespace Shapefile;
	LOG_MSG("Entering MapNewCanvas::MapNewCanvas");
	template_frame = t_frame;
	
	cat_classif_def.cat_classif_type = theme_type;
	if (theme_type == CatClassification::no_theme) {
		cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
		CatClassification::ChangeNumCats(1, cat_classif_def);
		cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
	}
	selectable_fill_color =
		GdaConst::map_default_fill_colour;
	
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 25;
	virtual_screen_marg_left = 25;
	virtual_screen_marg_right = 25;	
	shps_orig_xmin = project->main_data.header.bbox_x_min;
	shps_orig_ymin = project->main_data.header.bbox_y_min;
	shps_orig_xmax = project->main_data.header.bbox_x_max;
	shps_orig_ymax = project->main_data.header.bbox_y_max;
		
	double scale_x, scale_y, trans_x, trans_y;
	GdaScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
		shps_orig_xmax, shps_orig_ymax,
		virtual_screen_marg_top, virtual_screen_marg_bottom,
		virtual_screen_marg_left, virtual_screen_marg_right,
		GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(),
		fixed_aspect_ratio_mode, fit_to_window_mode,
		&scale_x, &scale_y, &trans_x, &trans_y, 0, 0,
		&current_shps_width, &current_shps_height);
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;

	if (project->main_data.header.shape_type == Shapefile::POINT) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GdaConst::map_default_highlight_colour;
	}
	
	use_category_brushes = true;
	if (!ChangeMapType(theme_type, smoothing_type_s, num_categories,
					   true, var_info_s, col_ids_s)) {
		// The user possibly clicked cancel.  Try again with
		// themeless map
		std::vector<GeoDaVarInfo> vi(0);
		std::vector<int> cids(0);
		ChangeMapType(CatClassification::no_theme, no_smoothing, 1,
					  true, vi, cids);
	}
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting MapNewCanvas::MapNewCanvas");
}

MapNewCanvas::~MapNewCanvas()
{
	LOG_MSG("Entering MapNewCanvas::~MapNewCanvas");
	highlight_state->removeObserver(this);
	if (custom_classif_state) custom_classif_state->removeObserver(this);
	LOG_MSG("Exiting MapNewCanvas::~MapNewCanvas");
}

void MapNewCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering MapNewCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((MapNewFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting MapNewCanvas::DisplayRightClickMenu");
}

void MapNewCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (size_t i=0, sz=GetNumVars(); i<sz; i++) {
		if (var_info[i].is_time_variant) {
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
				menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}
	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}


void MapNewCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
					GetCcType() == CatClassification::no_theme);

	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_1"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_2"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_3"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_4"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_5"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_6"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_7"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_8"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_9"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_10"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 10);

    GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
					GetCcType() == CatClassification::percentile);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_15"),
					GetCcType() == CatClassification::hinge_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_30"),
					GetCcType() == CatClassification::hinge_30);
    GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
					GetCcType() == CatClassification::stddev);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
					GetCcType() == CatClassification::unique_values);
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_1"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_2"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_3"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_4"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_5"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_6"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_7"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_8"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_9"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_10"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 10);
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_1"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_2"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_3"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_4"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_5"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_6"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_7"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_8"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_9"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_10"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 10);
	
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_RAWRATE"),
								  smoothing_type == raw_rate);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
								  smoothing_type == excess_risk);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
								  smoothing_type == empirical_bayes);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
								  smoothing_type == spatial_rate);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
								  smoothing_type == spatial_empirical_bayes);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_MEAN_CENTERS"),
								  selectable_shps_type != points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_CENTROIDS"),
								  selectable_shps_type != points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_VORONOI_DIAGRAM"),
								   selectable_shps_type == points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_EXPORT_VORONOI"),
								   selectable_shps_type == points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_SAVE_VORONOI_DUPS_TO_TABLE"),
								   voronoi_diagram_duplicates_exist);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_MEAN_CENTERS"),
								  display_mean_centers);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_CENTROIDS"),
								  display_centroids);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_VORONOI_DIAGRAM"),
								  display_voronoi_diagram);
}

wxString MapNewCanvas::GetCanvasTitle()
{
	wxString v;
	if (GetNumVars() == 1) v << GetNameWithTime(0);
	if (GetNumVars() == 2) {
		if (smoothing_type == raw_rate) {
			v << "Raw Rate ";
		} else if (smoothing_type == excess_risk) {
			// Excess Risk smoothing is a special case that comes with
			// its own theme.  See below.
			v << "";
		} else if (smoothing_type == empirical_bayes) {
			v << "EBS-Smoothed ";
		} else if (smoothing_type == spatial_rate) {
			v << "SRS-Smoothed ";
		} else if (smoothing_type == spatial_empirical_bayes) {
			v << "SEBS-Smoothed ";
		}
		v << GetNameWithTime(0) << " over " << GetNameWithTime(1);
	}
	
	wxString s;
	if (GetCcType() == CatClassification::excess_risk_theme) {
		// Excess Risk smoothing map is a special case in that it is a
		// type of smoothing, but is also its own theme.  Any theme associated
		// with Excess Risk is ignored
		s << "Excess Risk Map: " << v;
	}
	else if (GetCcType() == CatClassification::no_theme) {
		s << "Map - " << project->GetProjectTitle();
	} else if (GetCcType() == CatClassification::custom) {
		s << cat_classif_def.title << ": " << v;
	} else {
		s << CatClassification::CatClassifTypeToString(GetCcType());
		s << ": " << v;
	}
	
	return s;
}

wxString MapNewCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= GetNumVars()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;	
}

void MapNewCanvas::OnSaveCategories()
{
	wxString t_name;
	if (GetCcType() == CatClassification::custom) {
		t_name = cat_classif_def.title;
	} else {
		t_name = CatClassification::CatClassifTypeToString(GetCcType());
	}
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
	SaveCategories(title, label, "CATEGORIES");	
}

void MapNewCanvas::NewCustomCatClassif()
{
	// Begin by asking for a variable if none yet chosen
	if (var_info.size() == 0) {
		VariableSettingsDlg dlg(project, VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		var_info.resize(1);
		data.resize(1);
		var_info[0] = dlg.var_info[0];
		table_int->GetColData(dlg.col_ids[0], data[0]);
		VarInfoAttributeChange();
		cat_var_sorted.resize(num_time_vals);
		for (int t=0; t<num_time_vals; t++) {
			cat_var_sorted[t].resize(num_obs);
			for (int i=0; i<num_obs; i++) {
				cat_var_sorted[t][i].first = data[0][t+var_info[0].time_min][i];
				cat_var_sorted[t][i].second = i;
			}
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	}
	
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(cat_classif_def.num_cats,
										 cat_classif_def);
		std::vector<wxString> temp_cat_labels; // will be ignored
		CatClassification::SetBreakPoints(cat_classif_def.breaks,
										  temp_cat_labels,
										  cat_var_sorted[var_info[0].time],
										  cat_classif_def.cat_classif_type,
										  cat_classif_def.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def.num_cats; i++) {
			cat_classif_def.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def.names[i] = cat_data.GetCategoryLabel(time, i);
		}
		int col = table_int->FindColId(var_info[0].name);
		int tm = var_info[0].time;
		cat_classif_def.assoc_db_fld_name = table_int->GetColName(col, tm);
	}
	
	CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame();
	if (!ccf) return;
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def, "",
										  var_info[0].name, var_info[0].time);
	if (!ccs) return;
	if (custom_classif_state) custom_classif_state->removeObserver(this);
	cat_classif_def = ccs->GetCatClassif();
	custom_classif_state = ccs;
	custom_classif_state->registerObserver(this);
	//wxString s;
	//CatClassification::PrintCatClassifDef(cat_classif_def, s);
	//LOG_MSG(s);
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

/** ChangeMapType should always have var_info and col_ids passed in to it
 when needed.  */

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification including any needed data smoothing. */
bool MapNewCanvas::ChangeMapType(
							CatClassification::CatClassifType new_map_theme,
							SmoothingType new_map_smoothing,
							int num_categories_s,
							bool use_new_var_info_and_col_ids,
							const std::vector<GeoDaVarInfo>& new_var_info,
							const std::vector<int>& new_col_ids,
							const wxString& custom_classif_title)
{
	// We only ask for variables when changing from no_theme or
	// smoothed (with theme).
	num_categories = num_categories_s;
	
	if (new_map_theme == CatClassification::custom) {
		new_map_smoothing = no_smoothing;
	}
	
	if (smoothing_type != no_smoothing && new_map_smoothing == no_smoothing) {
		wxString msg;
		msg << "The new theme chosen will no longer include rates smoothing.";
		msg << " Please use the Rates submenu to choose a theme with rates";
		msg << " again.";
		wxMessageDialog dlg (this, msg, "Information",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	}
	
	if (new_map_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm) return false;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs) return false;
		if (custom_classif_state == new_ccs) return false;
		if (custom_classif_state) custom_classif_state->removeObserver(this);
		custom_classif_state = new_ccs;
		custom_classif_state->registerObserver(this);
		cat_classif_def = custom_classif_state->GetCatClassif();
	} else {
		if (custom_classif_state) custom_classif_state->removeObserver(this);
		custom_classif_state = 0;
	}
	
	if (new_map_smoothing == excess_risk) {
		new_map_theme = CatClassification::excess_risk_theme;
	}
	
	int new_num_vars = 1;
	if (new_map_smoothing != no_smoothing) {
		new_num_vars = 2;
	} else if (new_map_theme == CatClassification::no_theme) {
		new_num_vars = 0;
	}
	
	int num_vars = GetNumVars();
	
	if (new_num_vars == 0) {
		var_info.clear();
		template_frame->ClearAllGroupDependencies();
	} else if (new_num_vars == 1) {
		if (num_vars == 0) {
			if (!use_new_var_info_and_col_ids) return false;
			var_info.resize(1);
			data.resize(1);
			var_info[0] = new_var_info[0];
			template_frame->AddGroupDependancy(var_info[0].name);
			table_int->GetColData(new_col_ids[0], data[0]);
		} else if (num_vars == 1) {
			if (use_new_var_info_and_col_ids) {
				var_info[0] = new_var_info[0];
				template_frame->AddGroupDependancy(var_info[0].name);
				table_int->GetColData(new_col_ids[0], data[0]);
			} // else reuse current variable settings and values
		} else { // num_vars == 2
			if (!use_new_var_info_and_col_ids) return false;
			var_info.resize(1);
			template_frame->ClearAllGroupDependencies();
			data.resize(1);
			var_info[0] = new_var_info[0];
			template_frame->AddGroupDependancy(var_info[0].name);
			table_int->GetColData(new_col_ids[0], data[0]);	
		}
	} else if (new_num_vars == 2) {
		// For Rates, new var_info and col_id vectors should
		// always be passed in and num_cateogries, new_map_theme and
		// new_map_smoothing are assumed to be valid.
		if (!use_new_var_info_and_col_ids) return false;
		// user needs to choose new map themes
		
		if (new_map_smoothing == spatial_rate ||
			new_map_smoothing == spatial_empirical_bayes) {
			// check for weights file
			if (!project->GetWManager()->IsDefaultWeight()) {
				SelectWeightDlg dlg(project, this);
				if (dlg.ShowModal()!= wxID_OK) return false;
			}
			int w_ind = project->GetWManager()->GetCurrWeightInd();
			gal_weight = project->GetWManager()->GetGalWeight(w_ind);
		}
		
		var_info.clear();
		data.clear();
		var_info.resize(2);
		data.resize(2);
		template_frame->ClearAllGroupDependencies();
		for (int i=0; i<2; i++) {
			var_info[i] = new_var_info[i];
			template_frame->AddGroupDependancy(var_info[i].name);
			table_int->GetColData(new_col_ids[i], data[i]);
		}
		if (new_map_smoothing == excess_risk) {
			new_map_theme = CatClassification::excess_risk_theme;
		}
	}
	
	if (new_map_theme != CatClassification::custom && custom_classif_state) {
		custom_classif_state->removeObserver(this);
		custom_classif_state = 0;
	}
	cat_classif_def.cat_classif_type = new_map_theme;
	smoothing_type = new_map_smoothing;
	VarInfoAttributeChange();	
	CreateAndUpdateCategories();
	PopulateCanvas();
	return true;
}

void MapNewCanvas::update(CatClassifState* o)
{
	LOG_MSG("In MapNewCanvas::update(CatClassifState*)");
	cat_classif_def = o->GetCatClassif();
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

/** This method assumes that v1 is already set and valid.  It will
 recreate all canvas objects as needed and refresh the canvas.
 Assumes that CreateAndUpdateCategories has already been called.
 All data analysis will have been done in CreateAndUpdateCategories
 already. */
void MapNewCanvas::PopulateCanvas()
{
	LOG_MSG("Entering MapNewCanvas::PopulateCanvas");
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();

	int canvas_ts = cat_data.GetCurrentCanvasTmStep();
	if (!map_valid[canvas_ts]) full_map_redraw_needed = true;
	
	// Note: only need to delete selectable shapes if the map needs
	// to be resized.  Otherwise, just reuse.
	if (full_map_redraw_needed) {
		BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
		selectable_shps.clear();
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();

	if (map_valid[canvas_ts]) {		
		if (full_map_redraw_needed) {
			CreateSelShpsFromProj(selectable_shps, project);
			full_map_redraw_needed = false;
			
			if (selectable_shps_type == polygons &&
				(display_mean_centers || display_centroids)) {
				GdaPoint* p;
				wxPen cent_pen(wxColour(20, 20, 20));
				wxPen cntr_pen(wxColour(55, 55, 55));
				if (display_mean_centers) {
					const std::vector<GdaPoint*>& c = project->GetMeanCenters();
					for (int i=0; i<num_obs; i++) {
						p = new GdaPoint(*c[i]);
						p->setPen(cntr_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
				if (display_centroids) {
					const std::vector<GdaPoint*>& c = project->GetCentroids();
					for (int i=0; i<num_obs; i++) {
						p = new GdaPoint(*c[i]);
						p->setPen(cent_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
			}
			if (selectable_shps_type == points && display_voronoi_diagram) {
				GdaPolygon* p;
				const std::vector<GdaShape*>& polys =
					project->GetVoronoiPolygons();
				for (int i=0, num_polys=polys.size(); i<num_polys; i++) {
					p = new GdaPolygon(*(GdaPolygon*)polys[i]);
					background_shps.push_back(p);
				}
			}
		}
	} else {
		wxRealPoint cntr_ref_pnt(shps_orig_xmin +
								 (shps_orig_xmax-shps_orig_xmin)/2.0,
								 shps_orig_ymin+ 
								 (shps_orig_ymax-shps_orig_ymin)/2.0);
		GdaShapeText* txt_shp = new GdaShapeText(map_error_message[canvas_ts],
									 *GdaConst::medium_font, cntr_ref_pnt);
		background_shps.push_back(txt_shp);
	}
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting MapNewCanvas::PopulateCanvas");
}

void MapNewCanvas::TimeChange()
{
	LOG_MSG("Entering MapNewCanvas::TimeChange");
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
	for (size_t i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting MapNewCanvas::TimeChange");
}

void MapNewCanvas::VarInfoAttributeChange()
{
	Gda::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (size_t i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	template_frame->SetDependsOnNonSimpleGroups(is_any_time_variant);
	ref_var_index = -1;
	num_time_vals = 1;
	for (size_t i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	//Gda::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_categories and ref_var_index.
 This method populates cat_var_sorted from data array and performs any
 smoothing as needed, setting smoothing_valid vector as appropriate. */
void MapNewCanvas::CreateAndUpdateCategories()
{
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) map_valid[t] = true;
	map_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) map_error_message[t] = wxEmptyString;
	
	if (GetCcType() == CatClassification::no_theme) {
		 // 1 = #cats
		CatClassification::ChangeNumCats(1, cat_classif_def);
		cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
		cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t,0, GdaConst::map_default_fill_colour);
			cat_data.SetCategoryLabel(t, 0, "");
			cat_data.SetCategoryCount(t, 0, num_obs);
			for (int i=0; i<num_obs; i++) cat_data.AppendIdToCategory(t, 0, i);
		}
		
		if (ref_var_index != -1) {
			cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
											- var_info[ref_var_index].time_min);
		}
		return;
	}
	
	// Everything below assumes that GetCcType() != no_theme
	// We assume data has been initialized to correct data
	// for all time periods.
	
	double* P = 0;
	double* E = 0;
	double* smoothed_results = 0;
	std::vector<bool> undef_res(smoothing_type == no_smoothing ? 0 : num_obs);
	if (smoothing_type != no_smoothing) {
		P = new double[num_obs];
		E = new double[num_obs];
		smoothed_results = new double[num_obs];
	}
	
	cat_var_sorted.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].resize(num_obs);
		
		if (smoothing_type != no_smoothing) {
			if (var_info[0].sync_with_global_time) {
				for (int i=0; i<num_obs; i++) {
					E[i] = data[0][t+var_info[0].time_min][i];
				}
			} else {
				for (int i=0; i<num_obs; i++) {
					E[i] = data[0][var_info[0].time][i];
				}
			}
			
			if (var_info[1].sync_with_global_time) {
				for (int i=0; i<num_obs; i++) {
					P[i] = data[1][t+var_info[1].time_min][i];
				}
			} else {
				for (int i=0; i<num_obs; i++) {
					P[i] = data[1][var_info[1].time][i];
				}
			}
			
			for (int i=0; i<num_obs; i++) {
				if (P[i] <= 0) {
					map_valid[t] = false;
					map_error_message[t] = "Error: Base values contain"
						" non-positive numbers which will result in"
						" undefined values.";
					continue;
				}
			}
			
			if (!map_valid[t]) continue;
			
			if (smoothing_type == raw_rate) {
				GdaAlgs::RateSmoother_RawRate(num_obs, P, E,
												smoothed_results,
												undef_res);
			} else if (smoothing_type == excess_risk) {
				// Note: Excess Risk is a transformation, not a smoothing
				GdaAlgs::RateSmoother_ExcessRisk(num_obs, P, E,
												   smoothed_results,
												   undef_res);
			} else if (smoothing_type == empirical_bayes) {
				GdaAlgs::RateSmoother_EBS(num_obs, P, E,
											smoothed_results, undef_res);
			} else if (smoothing_type == spatial_rate) {
				GdaAlgs::RateSmoother_SRS(num_obs, gal_weight->gal, P, E,
											smoothed_results, undef_res);
			} else if (smoothing_type == spatial_empirical_bayes) {
				GdaAlgs::RateSmoother_SEBS(num_obs, gal_weight->gal, P, E,
											 smoothed_results, undef_res);
			}
		
			for (int i=0; i<num_obs; i++) {
				cat_var_sorted[t][i].first = smoothed_results[i];
				cat_var_sorted[t][i].second = i;
			}
		} else {
			for (int i=0; i<num_obs; i++) {
				cat_var_sorted[t][i].first =data[0][t+var_info[0].time_min][i];
				cat_var_sorted[t][i].second = i;
			}
		}
	}
	
	if (smoothing_type != no_smoothing) {
		if (P) delete [] P;
		if (E) delete [] E;
		if (smoothed_results) delete [] smoothed_results;
	}

	// Sort each vector in ascending order
	for (int t=0; t<num_time_vals; t++) {
		if (map_valid[t]) { // only sort data with valid smoothing
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	}
	
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def);
	}
	cat_classif_def.color_scheme =
		CatClassification::GetColSchmForType(cat_classif_def.cat_classif_type);
	CatClassification::PopulateCatClassifData(cat_classif_def,
											  cat_var_sorted,
											  cat_data, map_valid,
											  map_error_message);

	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int cnc = cat_data.GetNumCategories(cat_data.GetCurrentCanvasTmStep());
	CatClassification::ChangeNumCats(cnc, cat_classif_def);
}

void MapNewCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In MapNewCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	// Strictly speaking, should not have to repopulate map canvas
	// when time sync changes since scale of objects never changes. To keep
	// things simple, just redraw the whole canvas.
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void MapNewCanvas::DisplayMeanCenters()
{
	full_map_redraw_needed = true;
	display_mean_centers = !display_mean_centers;
	PopulateCanvas();
}

void MapNewCanvas::DisplayCentroids()
{
	full_map_redraw_needed = true;
	display_centroids = !display_centroids;
	PopulateCanvas();
}

void MapNewCanvas::DisplayVoronoiDiagram()
{
	full_map_redraw_needed = true;
	display_voronoi_diagram = !display_voronoi_diagram;
	PopulateCanvas();
}

int MapNewCanvas::GetNumVars()
{
	return var_info.size();
}

int MapNewCanvas::GetNumCats()
{
	return num_categories;
}

CatClassification::CatClassifType MapNewCanvas::GetCcType()
{
	return cat_classif_def.cat_classif_type;
}

/** Save Rates option should only be available when 
 smoothing_type != no_smoothing */
void MapNewCanvas::SaveRates()
{
	if (smoothing_type == no_smoothing) {
		wxString msg;
		msg << "No rates currently calculated to save.";
		wxMessageDialog dlg (this, msg, "Information",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	
	std::vector<SaveToTableEntry> data(1);
	
	std::vector<double> dt(num_obs);
	int t = cat_data.GetCurrentCanvasTmStep();
    for (int i=0; i<num_obs; i++) {
		dt[cat_var_sorted[t][i].second] = cat_var_sorted[t][i].first;
	}
	data[0].type = GdaConst::double_type;
	data[0].d_val = &dt;
	data[0].label = "Rate";
	
	if (smoothing_type == raw_rate) {
		data[0].field_default = "R_RAW_RT";
	} else if (smoothing_type == excess_risk) {
		data[0].field_default = "R_EXCESS";
	} else if (smoothing_type == empirical_bayes) {
		data[0].field_default = "R_EBS";
	} else if (smoothing_type == spatial_rate) {
		data[0].field_default = "R_SPAT_RT";
	} else if (smoothing_type == spatial_empirical_bayes) {
		data[0].field_default = "R_SPAT_EBS";
	} else {
		return;
	}

	wxString title = "Save Rates - ";
	title << GetNameWithTime(0) << " over " << GetNameWithTime(1);
	
    SaveToTableDlg dlg(project, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
}


void MapNewCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "obs " << hover_obs[0]+1;
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
	} else if (mousemode == select &&
			   (selectstate == dragging || selectstate == brushing)) {
		s << "#selected=" << highlight_state->GetTotalHighlighted();
		//if (brushtype == rectangle) {
			//wxRealPoint pt1 = MousePntToObsPnt(sel1);
			//wxRealPoint pt2 = MousePntToObsPnt(sel2);
			//wxString xmin = GenUtils::DblToStr(GenUtils::min<double>(pt1.x,
			//														 pt2.x));
			//wxString xmax = GenUtils::DblToStr(GenUtils::max<double>(pt1.x,
			//														 pt2.x));
			//wxString ymin = GenUtils::DblToStr(GenUtils::min<double>(pt1.y,
			//														 pt2.y));
			//wxString ymax = GenUtils::DblToStr(GenUtils::max<double>(pt1.y,
			//														 pt2.y));
			//s << ", select rect:";
			//s << " [" << xmin << "," << xmax << "] and";
			//s << " [" << ymin << "," << ymax << "]";
		//}
	}
	sb->SetStatusText(s);
}

MapNewLegend::MapNewLegend(wxWindow *parent, TemplateCanvas* t_canvas,
						   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

MapNewLegend::~MapNewLegend()
{
    LOG_MSG("In MapNewLegend::~MapNewLegend");
}

IMPLEMENT_CLASS(MapNewFrame, TemplateFrame)
BEGIN_EVENT_TABLE(MapNewFrame, TemplateFrame)
	EVT_ACTIVATE(MapNewFrame::OnActivate)	
END_EVENT_TABLE()

MapNewFrame::MapNewFrame(wxFrame *parent, Project* project,
						 const std::vector<GeoDaVarInfo>& var_info,
						 const std::vector<int>& col_ids,
						 CatClassification::CatClassifType theme_type,
						 MapNewCanvas::SmoothingType smoothing_type,
						 int num_categories,
						 const wxPoint& pos, const wxSize& size,
						 const long style)
: TemplateFrame(parent, project, "Map", pos, size, style)
{
	LOG_MSG("Entering MapNewFrame::MapNewFrame");

	int width, height;
	GetClientSize(&width, &height);

	wxSplitterWindow* splitter_win = 0;
	splitter_win = new wxSplitterWindow(this,-1,
										wxDefaultPosition, wxDefaultSize,
										wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
	wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new MapNewCanvas(rpanel, this, project,
									   var_info, col_ids,
									   theme_type, smoothing_type,
									   num_categories,
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
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);
    
	splitter_win->SplitVertically(lpanel, rpanel,
								  GdaConst::map_default_legend_width);
    
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(splitter_win, 1, wxEXPAND|wxALL);
    SetSizer(sizer);
    splitter_win->SetSize(wxSize(width,height));
    SetAutoLayout(true);
    DisplayStatusBar(true);
	Show(true);
	LOG_MSG("Exiting MapNewFrame::MapNewFrame");
}

MapNewFrame::MapNewFrame(wxFrame *parent, Project* project,
						 const wxPoint& pos, const wxSize& size,
						 const long style)
: TemplateFrame(parent, project, "Map", pos, size, style)
{
	LOG_MSG("Entering MapNewFrame::MapNewFrame");
	LOG_MSG("Exiting MapNewFrame::MapNewFrame");
}

MapNewFrame::~MapNewFrame()
{
	LOG_MSG("In MapNewFrame::~MapNewFrame");
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void MapNewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In MapNewFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("MapNewFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void MapNewFrame::MapMenus()
{
	LOG_MSG("In MapNewFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	((MapNewCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((MapNewCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void MapNewFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("MapNewFrame::UpdateOptionMenuItems: Options menu not found");
	} else {
		((MapNewCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void MapNewFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void  MapNewFrame::update(TimeState* o)
{
	LOG_MSG("In MapNewFrame::update(TimeState* o)");
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

void MapNewFrame::OnNewCustomCatClassifA()
{
	((MapNewCanvas*) template_canvas)->NewCustomCatClassif();
}

void MapNewFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::custom, MapNewCanvas::no_smoothing, 4,
					  true, dlg.var_info, dlg.col_ids, cc_title);
	} else {
		ChangeMapType(CatClassification::custom, MapNewCanvas::no_smoothing, 4,
					  false, std::vector<GeoDaVarInfo>(0), std::vector<int>(0),
					  cc_title);
	}
}

void MapNewFrame::OnThemelessMap()
{
	ChangeMapType(CatClassification::no_theme, MapNewCanvas::no_smoothing, 1,
				  false, std::vector<GeoDaVarInfo>(0), std::vector<int>(0));
}

void MapNewFrame::OnHinge15()
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::hinge_15, MapNewCanvas::no_smoothing,
					  6, true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::hinge_15, MapNewCanvas::no_smoothing,
					  6, false, std::vector<GeoDaVarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapNewFrame::OnHinge30()
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::hinge_30, MapNewCanvas::no_smoothing,
					  6, true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::hinge_30, MapNewCanvas::no_smoothing,
					  6, false, std::vector<GeoDaVarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapNewFrame::OnQuantile(int num_cats)
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::quantile, MapNewCanvas::no_smoothing,
					  num_cats, true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::quantile, MapNewCanvas::no_smoothing,
					  num_cats, false,
					  std::vector<GeoDaVarInfo>(0), std::vector<int>(0));
	}
}

void MapNewFrame::OnPercentile()
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::percentile, MapNewCanvas::no_smoothing,
					  6, true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::percentile, MapNewCanvas::no_smoothing,
					  6, false, std::vector<GeoDaVarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapNewFrame::OnStdDevMap()
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::stddev, MapNewCanvas::no_smoothing,
					  6, true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::stddev, MapNewCanvas::no_smoothing,
					  6, false, std::vector<GeoDaVarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapNewFrame::OnUniqueValues()
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::unique_values,
					  MapNewCanvas::no_smoothing,
					  6, true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::unique_values,
					  MapNewCanvas::no_smoothing,
					  6, false, std::vector<GeoDaVarInfo>(0),
					  std::vector<int>(0));
	}	
}

void MapNewFrame::OnNaturalBreaks(int num_cats)
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::natural_breaks,
					  MapNewCanvas::no_smoothing, num_cats,
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::natural_breaks,
					  MapNewCanvas::no_smoothing, num_cats,
					  false, std::vector<GeoDaVarInfo>(0), std::vector<int>(0));
	}
}

void MapNewFrame::OnEqualIntervals(int num_cats)
{
	if (((MapNewCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::equal_intervals,
					  MapNewCanvas::no_smoothing, num_cats,
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::equal_intervals,
					  MapNewCanvas::no_smoothing, num_cats,
					  false, std::vector<GeoDaVarInfo>(0), std::vector<int>(0));
	}
}

void MapNewFrame::OnSaveCategories()
{
	((MapNewCanvas*) template_canvas)->OnSaveCategories();
}

void MapNewFrame::OnRawrate()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed,
							"Raw Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapNewCanvas::raw_rate, dlg.GetNumCategories(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapNewFrame::OnExcessRisk()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
							"Excess Risk Map Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(CatClassification::excess_risk_theme,
				  MapNewCanvas::excess_risk, 6,
				  true, dlg.var_info, dlg.col_ids);
}

void MapNewFrame::OnEmpiricalBayes()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed,
							"Empirical Bayes Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapNewCanvas::empirical_bayes, dlg.GetNumCategories(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapNewFrame::OnSpatialRate()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed,
							"Spatial Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapNewCanvas::spatial_rate, dlg.GetNumCategories(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapNewFrame::OnSpatialEmpiricalBayes()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed,
							"Empirical Spatial Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapNewCanvas::spatial_empirical_bayes,
				  dlg.GetNumCategories(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapNewFrame::OnSaveRates()
{
	((MapNewCanvas*) template_canvas)->SaveRates();
}

bool MapNewFrame::ChangeMapType(CatClassification::CatClassifType new_map_theme,
								MapNewCanvas::SmoothingType new_map_smoothing,
								int num_categories,
								bool use_new_var_info_and_col_ids,
								const std::vector<GeoDaVarInfo>& new_var_info,
								const std::vector<int>& new_col_ids,
								const wxString& custom_classif_title)
{
	bool r=((MapNewCanvas*) template_canvas)->
		ChangeMapType(new_map_theme, new_map_smoothing, num_categories,
					  use_new_var_info_and_col_ids,
					  new_var_info, new_col_ids,
					  custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	if (template_legend) template_legend->Refresh();
	return r;
}

void MapNewFrame::OnDisplayMeanCenters()
{
	((MapNewCanvas*) template_canvas)->DisplayMeanCenters();
	UpdateOptionMenuItems();
}

void MapNewFrame::OnDisplayCentroids()
{
	((MapNewCanvas*) template_canvas)->DisplayCentroids();
	UpdateOptionMenuItems();
}

void MapNewFrame::OnDisplayVoronoiDiagram()
{
	((MapNewCanvas*) template_canvas)->DisplayVoronoiDiagram();
	((MapNewCanvas*) template_canvas)->voronoi_diagram_duplicates_exist =
		project->IsPointDuplicates();
	UpdateOptionMenuItems();
}

void MapNewFrame::OnExportVoronoi()
{
	project->ExportVoronoi();
	((MapNewCanvas*) template_canvas)->voronoi_diagram_duplicates_exist =
		project->IsPointDuplicates();
	UpdateOptionMenuItems();
}

void MapNewFrame::OnExportMeanCntrs()
{
	project->ExportCenters(true);
}

void MapNewFrame::OnExportCentroids()
{
	project->ExportCenters(false);
}

void MapNewFrame::OnSaveVoronoiDupsToTable()
{
	project->SaveVoronoiDupsToTable();
}

