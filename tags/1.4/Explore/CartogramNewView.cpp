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

#include <algorithm> // std::sort
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../DialogTools/MapQuantileDlg.h"
#include "../DialogTools/SelectWeightDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../FramesManager.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "CartogramNewView.h"

DorlingCartWorkerThread::DorlingCartWorkerThread(int iters_s,
								DorlingCartogram* cart_s,
								wxMutex* worker_list_mutex_s,
								wxCondition* worker_list_empty_cond_s,
								std::list<wxThread*> *worker_list_s,
								int thread_id_s)
: wxThread(),
iters(iters_s), cart(cart_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

DorlingCartWorkerThread::~DorlingCartWorkerThread()
{
}

wxThread::ExitCode DorlingCartWorkerThread::Entry()
{
	LOG_MSG(wxString::Format("DorlingCartWorkerThread %d started", thread_id));
	
	// improve by given number iterations
	cart->improve(iters);
	
	wxMutexLocker lock(*worker_list_mutex);
	// remove ourself from the list
	worker_list->remove(this);
	// if empty, signal on empty condition since only main thread
	// should be waiting on this condition
	LOG_MSG(wxString::Format("DorlingCartWorkerThread %d finished", thread_id));
	if (worker_list->empty()) {
		LOG_MSG("worker_list is empty, so signaling main thread");
		worker_list_empty_cond->Signal();
	}
	
	return NULL;
}


IMPLEMENT_CLASS(CartogramNewCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(CartogramNewCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int CartogramNewCanvas::RAD_VAR = 0; // circle size variable
const int CartogramNewCanvas::THM_VAR = 1; // circle color variable

CartogramNewCanvas::CartogramNewCanvas(wxWindow *parent,
									   TemplateFrame* t_frame,
									   Project* project_s,
									   const std::vector<GeoDaVarInfo>& v_info,
									   const std::vector<int>& col_ids,
									   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, true, true),
project(project_s), num_obs(project_s->GetNumRecords()),
num_time_vals(1),
num_cats(6),
highlight_state(project_s->highlight_state), custom_classif_state(0),
data(v_info.size()), var_info(v_info),
grid_base(project_s->GetGridBase()), gal_weight(0),
full_map_redraw_needed(true),
is_any_time_variant(false), is_any_sync_with_global_time(false),
improve_table(6), realtime_updates(false), all_init(false)
{
	using namespace Shapefile;
	LOG_MSG("Entering CartogramNewCanvas::CartogramNewCanvas");
	template_frame = t_frame;
	
	cat_classif_def.cat_classif_type = CatClassification::no_theme;
	cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
	CatClassification::ChangeNumCats(1, cat_classif_def);
	cat_classif_def.colors[0] = GeoDaConst::map_default_fill_colour;
	
	for (int i=0; i<var_info.size(); i++) {
		grid_base->GetColData(col_ids[i], data[i]);
	}
	
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) {
			is_any_time_variant = true;
			num_time_vals = 1 + var_info[i].time_max - var_info[i].time_min;
		}
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
		
	std::vector<double> orig_x(num_obs);
	std::vector<double> orig_y(num_obs);
	std::vector<double> orig_data(num_obs);
	project->GetCenters(orig_x, orig_y);
	
	cart_nbr_info = new CartNbrInfo(project->GetVoronoiRookNeighborGal(),
									num_obs);
	int num_cart_times = (grid_base->IsColTimeVariant(col_ids[RAD_VAR]) ?
						  grid_base->GetTimeSteps() : 1);
	carts.resize(num_cart_times);
	num_improvement_iters.resize(num_cart_times);
	for (int t=0; t<num_cart_times; t++) {
		grid_base->GetColData(col_ids[RAD_VAR], t, orig_data);
		carts[t] = new DorlingCartogram(cart_nbr_info, orig_x,
										orig_y, orig_data,
										var_info[RAD_VAR].min_over_time,
										var_info[RAD_VAR].max_over_time);
		num_improvement_iters[t] = 0;
	}
	// get timing for single iteration
	int cur_cart_ts = var_info[RAD_VAR].time;
	int iter_ms = carts[cur_cart_ts]->improve(1);
	if (iter_ms < 10) {
		carts[cur_cart_ts]->improve(100);
		num_improvement_iters[cur_cart_ts] += 100;
	}
	num_improvement_iters[cur_cart_ts]++;
	secs_per_iter = carts[cur_cart_ts]->secs_per_iter;
	LOG(secs_per_iter);
	num_cpus = wxThread::GetCPUCount();
	if (num_cpus < 1) num_cpus = 1;
	LOG(num_cpus);
	
	// only improve across all time periods if a single iteration of
	// the Cartogram takes less than 1 second.
	if (iter_ms < 1000) ImproveAll(2, 200);
	UpdateImproveLevelTable();
	
	// experiment with outlines that are just slightly brighter than
	// current color.  Perhaps could convert to HSV values, and then
	// bump up luminance slightly?
	
	// Enable realtime_updates for future calls to ImproveAll
	realtime_updates = true;
	
	double max_rad = 
		SampleStatistics::CalcMax(carts[cur_cart_ts]->output_radius);
	double min_out_x, max_out_x, min_out_y, max_out_y;
	SampleStatistics::CalcMinMax(carts[cur_cart_ts]->output_x,
								 min_out_x, max_out_x);
	SampleStatistics::CalcMinMax(carts[cur_cart_ts]->output_y,
								 min_out_y, max_out_y);
	min_out_x -= max_rad;
	min_out_y -= max_rad;
	max_out_x += max_rad;
	max_out_y += max_rad;
	
	selectable_fill_color = GeoDaConst::map_default_fill_colour;

	// Note: the shps_orig min/max will depend on the bubble sizes
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 25;
	virtual_screen_marg_left = 25;
	virtual_screen_marg_right = 25;	
	shps_orig_xmin = min_out_x;
	shps_orig_ymin = min_out_y;
	shps_orig_xmax = max_out_x;
	shps_orig_ymax = max_out_y;
	
	double scale_x, scale_y, trans_x, trans_y;
	MyScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
								   shps_orig_xmax, shps_orig_ymax,
								   virtual_screen_marg_top,
								   virtual_screen_marg_bottom,
								   virtual_screen_marg_left,
								   virtual_screen_marg_right,
								   GetVirtualSize().GetWidth(),
								   GetVirtualSize().GetHeight(),
								   fixed_aspect_ratio_mode, fit_to_window_mode,
								   &scale_x, &scale_y, &trans_x, &trans_y, 0, 0,
								   &current_shps_width, &current_shps_height);
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;

	selectable_shps_type = circles;
	highlight_color = GeoDaConst::map_default_highlight_colour;

	use_category_brushes = true;
	if (num_obs >= 6) {
		ChangeThemeType(CatClassification::hinge_15);
	} else {
		ChangeThemeType(CatClassification::unique_values);
	}
	
	all_init = true;
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting CartogramNewCanvas::CartogramNewCanvas");
}

CartogramNewCanvas::~CartogramNewCanvas()
{
	LOG_MSG("Entering CartogramNewCanvas::~CartogramNewCanvas");
	for (int i=0; i<carts.size(); i++) if (carts[i]) delete carts[i];
	if (cart_nbr_info) delete cart_nbr_info;
	highlight_state->removeObserver(this);
	if (custom_classif_state) custom_classif_state->removeObserver(this);
	LOG_MSG("Exiting CartogramNewCanvas::~CartogramNewCanvas");
}

void CartogramNewCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering CartogramNewCanvas::DisplayRightClickMenu");
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CARTOGRAM_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting CartogramNewCanvas::DisplayRightClickMenu");
}

void CartogramNewCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) {
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
				menu1->AppendCheckItem(GeoDaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}

	menu->Prepend(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}


void CartogramNewCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
								  GetCcType() == CatClassification::no_theme);	
	GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
								  GetCcType() == CatClassification::quantile);
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
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_EQUAL_INTERVALS"),
								  GetCcType() ==CatClassification::equal_intervals);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_NATURAL_BREAKS"),
								  GetCcType() == CatClassification::natural_breaks);
	
	std::vector<wxString> txt(6);
	for (int i=0; i<txt.size(); i++) {
		int seconds = (int) improve_table[i].first;
		txt[i] << improve_table[i].second << " more iterations, ~";
		if (seconds < 120) {
			txt[i] << seconds << " seconds";
		} else if (seconds < 7200) {
			txt[i] << (seconds / 60) << " minutes";
		} else {
			txt[i] << (seconds / 3600) << " hours";
		}
	}
	int txt_cnt = 0;
	GeneralWxUtils::SetMenuItemText(menu, XRCID("ID_CARTOGRAM_IMPROVE_1"),
									txt[txt_cnt++]);
	GeneralWxUtils::SetMenuItemText(menu, XRCID("ID_CARTOGRAM_IMPROVE_2"),
									txt[txt_cnt++]);
	GeneralWxUtils::SetMenuItemText(menu, XRCID("ID_CARTOGRAM_IMPROVE_3"),
									txt[txt_cnt++]);
	GeneralWxUtils::SetMenuItemText(menu, XRCID("ID_CARTOGRAM_IMPROVE_4"),
									txt[txt_cnt++]);
	GeneralWxUtils::SetMenuItemText(menu, XRCID("ID_CARTOGRAM_IMPROVE_5"),
									txt[txt_cnt++]);
	GeneralWxUtils::SetMenuItemText(menu, XRCID("ID_CARTOGRAM_IMPROVE_6"),
									txt[txt_cnt++]);
}

wxString CartogramNewCanvas::GetCategoriesTitle()
{
	wxString v;
	if (GetCcType() == CatClassification::no_theme) {
		v << "Themeless";
	} else if (GetCcType() == CatClassification::custom) {
		v << cat_classif_def.title << ": " << GetNameWithTime(THM_VAR);
	} else {
		v << CatClassification::CatClassifTypeToString(GetCcType());
		v << ": " << GetNameWithTime(THM_VAR);
	}
	return v;
}

wxString CartogramNewCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Cartogram - size: " << GetNameWithTime(RAD_VAR);
	if (GetCcType() != CatClassification::no_theme) {
		v << ", ";
		if (GetCcType() == CatClassification::custom) {
			v << cat_classif_def.title;
		} else {
			v << CatClassification::CatClassifTypeToString(GetCcType());
		}
		v << ": " << GetNameWithTime(THM_VAR);
	}
	return v;
}

wxString CartogramNewCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetGridBase()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

void CartogramNewCanvas::OnSaveCategories()
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

void CartogramNewCanvas::NewCustomCatClassif()
{
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		int tht = var_info[THM_VAR].time;
		CatClassification::ChangeNumCats(cat_classif_def.num_cats,
										 cat_classif_def);
		CatClassification::SetBreakPoints(cat_classif_def.breaks,
										  cat_var_sorted[tht],
										  cat_classif_def.cat_classif_type,
										  cat_classif_def.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def.num_cats; i++) {
			cat_classif_def.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def.names[i] = cat_data.GetCategoryLabel(time, i);
		}
	}
	
	CatClassifFrame* ccf = MyFrame::theFrame->GetCatClassifFrame();
	if (!ccf) return;
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def, "",
										  var_info[THM_VAR].name,
										  var_info[THM_VAR].time);
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

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification. */
void CartogramNewCanvas::ChangeThemeType(
						CatClassification::CatClassifType new_cat_theme,
						const wxString& custom_classif_title)
{
	// User has already chosen theme variable on startup, so no need
	// to ever ask for theme variable.
	if (new_cat_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm) return;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs) return;
		if (custom_classif_state == new_ccs) return;
		if (custom_classif_state) custom_classif_state->removeObserver(this);
		custom_classif_state = new_ccs;
		custom_classif_state->registerObserver(this);
		cat_classif_def = custom_classif_state->GetCatClassif();
	} else {
		if (custom_classif_state) custom_classif_state->removeObserver(this);
		custom_classif_state = 0;
	}
	cat_classif_def.cat_classif_type = new_cat_theme;
	VarInfoAttributeChange();	
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (all_init && template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}	
}

void CartogramNewCanvas::update(CatClassifState* o)
{
	cat_classif_def = o->GetCatClassif();
	VarInfoAttributeChange();
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
void CartogramNewCanvas::PopulateCanvas()
{
	LOG_MSG("Entering CartogramNewCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();

	int canvas_ts = cat_data.GetCurrentCanvasTmStep();
	if (!map_valid[canvas_ts]) full_map_redraw_needed = true;
	
	// Note: only need to delete selectable shapes if the cartogram
	// relative positions change.  Otherwise, just reuse.
	if (full_map_redraw_needed) {
		BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
		selectable_shps.clear();
	}
	
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();

	if (map_valid[canvas_ts]) {
		if (full_map_redraw_needed) {
			int cur_cart_ts = var_info[RAD_VAR].time;
			MyCircle* c;
			for (int i=0; i<num_obs; i++) {
				c = new MyCircle(wxRealPoint(carts[cur_cart_ts]->output_x[i],
											 carts[cur_cart_ts]->output_y[i]),
								 carts[cur_cart_ts]->output_radius[i], true);
				selectable_shps.push_back(c);
			}
			full_map_redraw_needed = false;
		}
	} else {
		wxRealPoint cntr_ref_pnt(shps_orig_xmin +
								 (shps_orig_xmax-shps_orig_xmin)/2.0,
								 shps_orig_ymin+ 
								 (shps_orig_ymax-shps_orig_ymin)/2.0);
		MyText* txt_shp = new MyText(map_error_message[canvas_ts],
									 *GeoDaConst::medium_font, cntr_ref_pnt);
		background_shps.push_back(txt_shp);
	}
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting CartogramNewCanvas::PopulateCanvas");
}

void CartogramNewCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering CartogramNewCanvas::TitleOrTimeChange");
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
	int cur_rad_time = var_info[RAD_VAR].time;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	// only force circle redraw if time has changed for circle variable
	if (var_info[RAD_VAR].time != cur_rad_time) full_map_redraw_needed = true;
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting CartogramNewCanvas::TitleOrTimeChange");
}

void CartogramNewCanvas::VarInfoAttributeChange()
{
	GeoDa::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	ref_var_index = -1;
	num_time_vals = 1;
	for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	//GeoDa::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_cats and ref_var_index.
 This method populates cat_var_sorted from data array. */
void CartogramNewCanvas::CreateAndUpdateCategories()
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
		cat_classif_def.colors[0] = GeoDaConst::map_default_fill_colour;
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t, 0,GeoDaConst::map_default_fill_colour);
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
	
	cat_var_sorted.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].resize(num_obs);
		int thm_t = (var_info[THM_VAR].sync_with_global_time ? 
					 t + var_info[THM_VAR].time_min : var_info[THM_VAR].time);
		for (int i=0; i<num_obs; i++) {
			cat_var_sorted[t][i].first = data[THM_VAR][thm_t][i];
			cat_var_sorted[t][i].second = i;
		}
	}
	
	// Sort each vector in ascending order
	std::sort(cat_var_sorted[0].begin(), cat_var_sorted[0].end(),
			  GeoDa::dbl_int_pair_cmp_less);
	if (var_info[THM_VAR].sync_with_global_time) {
		for (int t=1; t<num_time_vals; t++) {
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  GeoDa::dbl_int_pair_cmp_less);
		}
	} else {
		// just copy first sorted results
		for (int t=1; t<num_time_vals; t++) {
			cat_var_sorted[t] = cat_var_sorted[0];
		}
	}
	
	int num_cats = 1;
	if (GetCcType() == CatClassification::quantile ||
		GetCcType() == CatClassification::natural_breaks ||
		GetCcType() == CatClassification::equal_intervals) {
		// Need to ask user for number of categories
		
		wxString title;
		if (GetCcType() == CatClassification::quantile) {
			title = "Quantile Map";
		} else if (GetCcType() == CatClassification::natural_breaks) {
			title = "Natural Breaks";
		} else if (GetCcType() == CatClassification::equal_intervals) {
			title = "Equal Intervals";
		}
		
		MapQuantileDlg dlg(this, 1, CatClassification::max_num_classes,
						   4, title);
		dlg.SetTitle(title);
		if (dlg.ShowModal() != wxID_OK) {
			num_cats = 4;
		} else {
			num_cats = dlg.classes;
		}
	}
	
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(num_cats, cat_classif_def);
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

void CartogramNewCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In CartogramNewCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

CatClassification::CatClassifType CartogramNewCanvas::GetCcType()
{
	return cat_classif_def.cat_classif_type;
}

void CartogramNewCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "obs " << hover_obs[0]+1 << " = (";
			s << data[RAD_VAR][var_info[RAD_VAR].time][hover_obs[0]] << ", ";
			s << data[THM_VAR][var_info[THM_VAR].time][hover_obs[0]] << ")";
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = (";
			s << data[RAD_VAR][var_info[RAD_VAR].time][hover_obs[1]] << ", ";
			s << data[THM_VAR][var_info[THM_VAR].time][hover_obs[1]] << ")";
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = (";
			s << data[RAD_VAR][var_info[RAD_VAR].time][hover_obs[2]] << ", ";
			s << data[THM_VAR][var_info[THM_VAR].time][hover_obs[2]] << ")";
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	} else if (mousemode == select &&
			   (selectstate == dragging || selectstate == brushing)) {
		s << "#selected=" << highlight_state->GetTotalHighlighted();
	}
	sb->SetStatusText(s);
}

void CartogramNewCanvas::ImproveAll(double max_seconds, int max_iters)
{
	LOG_MSG("Entering CartogramNewCanvas::ImproveAll");
	if (max_iters == 0 || max_seconds <= 0) return;
	
	LOG_MSG(wxString::Format("%d threading cores detected", num_cpus));
	
	// must decide on work-batch units for available CPUs.
	// if num_time_periods <= nCPUs then each cpu gets one job
	// otherwise, we have to spawn multiple rounds of multiple threads
	
	// We must first pre-calculate time estimate to do max_iters
	// if time_max_iters > max_seconds, then linearly scale back
	// iters so that max_seconds is approximately respected.
	// report actual elapsed time.
	
	LOG(EstItersGivenTime(max_seconds));
	int update_rounds = 1; // do one round of batches by default
	int iters = GenUtils::min<int>(max_iters, EstItersGivenTime(max_seconds));
	int est_secs = EstSecondsGivenIters(iters);
	LOG(est_secs);
	LOG(iters);
	int num_batches = GetNumBatches();
	if (realtime_updates && est_secs > 2) {
		// break up iters into multiple rounds if total time estimate is
		// greater than 2 seconds
		if (secs_per_iter*num_batches >= 1) {
			update_rounds = iters;
			iters = 1;
		} else {
			update_rounds =
				((double) num_batches) * ((double) iters) * secs_per_iter;
			iters = 1/(secs_per_iter* ((double) num_batches));
		}
	}
	LOG(update_rounds);
	LOG(iters);
	for (int r=0; r<update_rounds; r++) {
		LOG_MSG(wxString::Format("update round %d of %d", r+1, update_rounds));
		int num_carts_rem = GetCurNumCartTms();
		int crt_min_tm = var_info[RAD_VAR].time_min;		
		for (int i=0; i<num_batches && num_carts_rem > 0; i++) {
			LOG_MSG(wxString::Format("batch %d of %d:", i+1, num_batches));
			int num_in_batch = GenUtils::min<int>(num_cpus, num_carts_rem);
			LOG_MSG(wxString::Format("  improving carts %d to %d",
									 crt_min_tm, crt_min_tm + (num_in_batch-1)));
		
			if (num_in_batch > 1) {
				// mutext protects access to the worker_list
				wxMutex worker_list_mutex;
				// signals that worker_list is empty
				wxCondition worker_list_empty_cond(worker_list_mutex);
				worker_list_mutex.Lock(); // mutex should be initially locked
			
				// List of all the threads currently alive.  As soon as the
				// thread terminates, it removes itself from the list.
				std::list<wxThread*> worker_list;
				int thread_id = 0;
				for (int t=crt_min_tm; t<crt_min_tm+num_in_batch; t++) {
					LOG_MSG(wxString::Format("    creating thread for cart %d",
											 t)); 
					DorlingCartWorkerThread* thread =
						new DorlingCartWorkerThread(iters, carts[t],
													&worker_list_mutex,
													&worker_list_empty_cond,
													&worker_list, thread_id++);
					if ( thread->Create() != wxTHREAD_NO_ERROR ) {
						LOG_MSG("Error: Can't create thread, switching to "
								"single thread mode!");
						delete thread;
						num_cpus = 1;
						ImproveAll(max_seconds, max_iters);
						return;
					} else {
						worker_list.push_front(thread);
					}
					num_improvement_iters[t] += iters;
				}
			
				LOG_MSG("Starting all worker threads");
				std::list<wxThread*>::iterator it;
				for (it = worker_list.begin(); it != worker_list.end(); it++) {
					(*it)->Run();
				}
			
				while (!worker_list.empty()) {
					// wait until thread_list might be empty
					worker_list_empty_cond.Wait();
					// We have been woken up. If this was not a false
					// alarm (spurious signal), the loop will exit.
					LOG_MSG("work_list_empty_cond signaled");
				}
				LOG_MSG("All worker threads exited");
			
			} else {
				carts[crt_min_tm]->improve(iters);
				num_improvement_iters[crt_min_tm] += iters;
			}
		
			num_carts_rem -= num_in_batch;
			crt_min_tm += num_in_batch;
		}
		if (update_rounds > 1 && realtime_updates) {
			full_map_redraw_needed = true;
			invalidateBms();
			PopulateCanvas();
			Refresh();
			Update();
		}
	}
	LOG_MSG(wxString::Format("Previous secs_per_iter: %f", secs_per_iter));
	secs_per_iter = carts[var_info[RAD_VAR].time]->secs_per_iter;
	LOG_MSG(wxString::Format("Updated secs_per_iter: %f", secs_per_iter));
	UpdateImproveLevelTable();
	full_map_redraw_needed = true;
	
	LOG_MSG("Exiting CartogramNewCanvas::ImproveAll");
}

int CartogramNewCanvas::GetCurNumCartTms()
{
	return 1 + var_info[RAD_VAR].time_max - var_info[RAD_VAR].time_min;
}

int CartogramNewCanvas::GetNumBatches()
{
	return (int) ceil(((double) GetCurNumCartTms())/((double) num_cpus));
}

int CartogramNewCanvas::EstItersGivenTime(double max_seconds)
{
	if (secs_per_iter <= 0) return 10000000;
	int num_batches = GetNumBatches();
	double max_secs_per_batch = max_seconds / ((double) num_batches);
	return (int) ceil(max_secs_per_batch / secs_per_iter);
}

double CartogramNewCanvas::EstSecondsGivenIters(int max_iters)
{
	double num_batches = (double) GetNumBatches();
	double iters_per_batch = (double) max_iters;
	return secs_per_iter * iters_per_batch * num_batches;
}

void CartogramNewCanvas::CartogramImproveLevel(int level)
{
	ImproveAll(improve_table[level].first, improve_table[level].second);
	PopulateCanvas();
}

void CartogramNewCanvas::UpdateImproveLevelTable()
{
	// as a standard, will have entries for 100, 500 and 1000 iterations
	// also 5, 30 and 60 seconds
	//GeoDa::dbl_int_pair_vec_type improve_table; // already size 6
	
	improve_table[0].first = 5.0; // seconds
	improve_table[0].second = EstItersGivenTime(improve_table[0].first);
	if (improve_table[0].second > 5000) {
		improve_table[0].second = 5000;
		improve_table[0].first = EstSecondsGivenIters(improve_table[0].second);
	} else if (improve_table[0].second <= 1) {
		improve_table[0].second = 1;
		improve_table[0].first = EstSecondsGivenIters(improve_table[0].second);
	}

	improve_table[1].first = 30.0; // seconds
	improve_table[1].second = EstItersGivenTime(improve_table[1].first);
	if (improve_table[1].second > 10000) {
		improve_table[1].second = 10000;
		improve_table[1].first = EstSecondsGivenIters(improve_table[1].second);
	} else if (improve_table[1].second <= improve_table[0].second) {
		improve_table[1].second = 2*improve_table[0].second;
		improve_table[1].first = EstSecondsGivenIters(improve_table[1].second);
	}
	
	improve_table[2].first = 60.0; // seconds
	improve_table[2].second = EstItersGivenTime(improve_table[2].first);
	if (improve_table[2].second > 20000) {
		improve_table[2].second = 20000;
		improve_table[2].first = EstSecondsGivenIters(improve_table[2].second);
	} else if (improve_table[2].second <= improve_table[1].second) {
		improve_table[2].second = 2*improve_table[1].second;
		improve_table[2].first = EstSecondsGivenIters(improve_table[2].second);
	}
	
	improve_table[3].second = 100; // iterations
	improve_table[3].first = EstSecondsGivenIters(improve_table[3].second);
	
	improve_table[4].second = 500; // iterations
	improve_table[4].first = EstSecondsGivenIters(improve_table[4].second);
	
	improve_table[5].second = 1000; // iterations
	improve_table[5].first = EstSecondsGivenIters(improve_table[5].second);

	std::sort(improve_table.begin(), improve_table.end(),
			  GeoDa::dbl_int_pair_cmp_second_less);	
}


CartogramNewLegend::CartogramNewLegend(wxWindow *parent,
									   TemplateCanvas* t_canvas,
									   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

CartogramNewLegend::~CartogramNewLegend()
{
    LOG_MSG("In CartogramNewLegend::~CartogramNewLegend");
}

IMPLEMENT_CLASS(CartogramNewFrame, TemplateFrame)
BEGIN_EVENT_TABLE(CartogramNewFrame, TemplateFrame)
	EVT_ACTIVATE(CartogramNewFrame::OnActivate)	
END_EVENT_TABLE()

CartogramNewFrame::CartogramNewFrame(wxFrame *parent, Project* project,
									 const std::vector<GeoDaVarInfo>& var_info,
									 const std::vector<int>& col_ids,
									 const wxString& title, const wxPoint& pos,
									 const wxSize& size, const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering CartogramNewFrame::CartogramNewFrame");

	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this);
	splitter_win->SetMinimumPaneSize(10);
	
	template_canvas = new CartogramNewCanvas(splitter_win, this, project,
											 var_info, col_ids,
											 wxDefaultPosition,
											 wxSize(width,height));
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	
	template_legend = new CartogramNewLegend(splitter_win, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	
	splitter_win->SplitVertically(template_legend, template_canvas,
								  GeoDaConst::map_default_legend_width);
	
	Show(true);
	LOG_MSG("Exiting CartogramNewFrame::CartogramNewFrame");
}

CartogramNewFrame::~CartogramNewFrame()
{
	LOG_MSG("In CartogramNewFrame::~CartogramNewFrame");
	DeregisterAsActive();
}

void CartogramNewFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In CartogramNewFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("CartogramNewFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void CartogramNewFrame::MapMenus()
{
	LOG_MSG("In CartogramNewFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CARTOGRAM_NEW_VIEW_MENU_OPTIONS");
	((CartogramNewCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((CartogramNewCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void CartogramNewFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("CartogramNewFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((CartogramNewCanvas*)
		 template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void CartogramNewFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
	
}

/** Implementation of FramesManagerObserver interface */
void  CartogramNewFrame::update(FramesManager* o)
{
	LOG_MSG("In CartogramNewFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

void CartogramNewFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
}

void CartogramNewFrame::OnNewCustomCatClassifA()
{
	((CartogramNewCanvas*) template_canvas)->NewCustomCatClassif();
}

void CartogramNewFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, cc_title);
}

void CartogramNewFrame::OnThemeless(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::no_theme);
}

void CartogramNewFrame::OnHinge15(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::hinge_15);
}

void CartogramNewFrame::OnHinge30(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::hinge_30);
}

void CartogramNewFrame::OnQuantile(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::quantile);
}

void CartogramNewFrame::OnPercentile(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::percentile);
}

void CartogramNewFrame::OnStdDevMap(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::stddev);
}

void CartogramNewFrame::OnUniqueValues(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::unique_values);
}

void CartogramNewFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::natural_breaks);
}

void CartogramNewFrame::OnEqualIntervals(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::equal_intervals);
}

void CartogramNewFrame::OnSaveCategories(wxCommandEvent& event)
{
	((CartogramNewCanvas*) template_canvas)->OnSaveCategories();
}

void CartogramNewFrame::ChangeThemeType(
						CatClassification::CatClassifType new_cat_theme,
						const wxString& custom_classif_title)
{
	((CartogramNewCanvas*) template_canvas)->
		ChangeThemeType(new_cat_theme, custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	template_legend->Refresh();
}

void CartogramNewFrame::CartogramImproveLevel(int level)
{
	LOG_MSG(wxString::Format("Improve Cartogram to Level %d", level));
	((CartogramNewCanvas*) template_canvas)->CartogramImproveLevel(level);
	UpdateOptionMenuItems();
}
