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
#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../DialogTools/MapQuantileDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../FramesManager.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ConditionalMapView.h"


IMPLEMENT_CLASS(ConditionalMapCanvas, ConditionalNewCanvas)
BEGIN_EVENT_TABLE(ConditionalMapCanvas, ConditionalNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalMapCanvas::CAT_VAR = 2; // main theme variable

ConditionalMapCanvas::ConditionalMapCanvas(wxWindow *parent,
									   TemplateFrame* t_frame,
									   Project* project_s,
									   const std::vector<GeoDaVarInfo>& v_info,
									   const std::vector<int>& col_ids,
									   const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   true, true, pos, size),
num_cats(1),bin_bm(0),
bin_bg_map_pen(wxColor(200,200,200)), bin_bg_map_brush(wxColor(200,200,200)),
cc_state_map(0),
full_map_redraw_needed(true)
{
	using namespace Shapefile;
	LOG_MSG("Entering ConditionalMapCanvas::ConditionalMapCanvas");
	SetCatType(CatClassification::no_theme);
	
	selectable_fill_color = GeoDaConst::map_default_fill_colour;

	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 50;
	virtual_screen_marg_left = 50;
	virtual_screen_marg_right = 25;
	shps_orig_xmin = project->main_data.header.bbox_x_min;
	shps_orig_ymin = project->main_data.header.bbox_y_min;
	shps_orig_xmax = project->main_data.header.bbox_x_max;
	shps_orig_ymax = project->main_data.header.bbox_y_max;
	
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

	if (project->main_data.header.shape_type == Shapefile::POINT) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GeoDaConst::map_default_highlight_colour;
	}

	use_category_brushes = true;
	if (num_obs >= 6) {
		ChangeCatThemeType(CatClassification::hinge_15, false);
	} else {
		ChangeCatThemeType(CatClassification::unique_values, false);
	}
	
	all_init = true;
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting ConditionalMapCanvas::ConditionalMapCanvas");
}

ConditionalMapCanvas::~ConditionalMapCanvas()
{
	LOG_MSG("Entering ConditionalMapCanvas::~ConditionalMapCanvas");
	if (cc_state_map) cc_state_map->removeObserver(this);
	LOG_MSG("Exiting ConditionalMapCanvas::~ConditionalMapCanvas");
}

void ConditionalMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering ConditionalMapCanvas::DisplayRightClickMenu");
		wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting ConditionalMapCanvas::DisplayRightClickMenu");
}

void ConditionalMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	ConditionalNewCanvas::SetCheckMarks(menu);
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
					GetCatType() == CatClassification::no_theme);
	GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
					GetCatType() == CatClassification::quantile);
    GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
					GetCatType() == CatClassification::percentile);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_15"),
					GetCatType() == CatClassification::hinge_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_30"),
					GetCatType() == CatClassification::hinge_30);
    GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
					GetCatType() == CatClassification::stddev);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
					GetCatType() == CatClassification::unique_values);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_EQUAL_INTERVALS"),
					GetCatType() ==CatClassification::equal_intervals);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_NATURAL_BREAKS"),
					GetCatType() == CatClassification::natural_breaks);	
}

wxString ConditionalMapCanvas::GetCategoriesTitle()
{
	wxString v;
	if (GetCatType() == CatClassification::custom) {
		v << cat_classif_def_map.title;
		v << ": " << GetNameWithTime(CAT_VAR);
	} else if (GetCatType() == CatClassification::no_theme) {
		v << "Themeless";
	} else {
		v << CatClassification::CatClassifTypeToString(GetCatType());
		v << ": " << GetNameWithTime(CAT_VAR);
	}
	return v;	
}

wxString ConditionalMapCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Conditional Map - ";
	v << "x: " << GetNameWithTime(HOR_VAR);
	v << ", y: " << GetNameWithTime(VERT_VAR);
	if (GetCatType() == CatClassification::custom) {
		v << ", " << cat_classif_def_map.title;
		v << ": " << GetNameWithTime(CAT_VAR);
	} else if (GetCatType() != CatClassification::no_theme) {
		v << ", " << CatClassification::CatClassifTypeToString(GetCatType());
		v << ": " << GetNameWithTime(CAT_VAR);
	}
	return v;
}

void ConditionalMapCanvas::OnSaveCategories()
{
	wxString t_name = CatClassification::CatClassifTypeToString(GetCatType());
	if (GetCatType() == CatClassification::custom) {
		t_name = cat_classif_def_map.title;
	}
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
	SaveCategories(title, label, "CATEGORIES");	
}

void ConditionalMapCanvas::NewCustomCatClassifMap()
{
	// we know that all three var_info variables are defined, so need
	// need to prompt user as with MapNewCanvas
	
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def_map.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(cat_classif_def_map.num_cats,
										 cat_classif_def_map);
		CatClassification::SetBreakPoints(cat_classif_def_map.breaks,
										  cat_var_sorted[var_info[CAT_VAR].time],
										  cat_classif_def_map.cat_classif_type,
										  cat_classif_def_map.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def_map.num_cats; i++) {
			cat_classif_def_map.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def_map.names[i] = cat_data.GetCategoryLabel(time, i);
		}
	}
	
	CatClassifFrame* ccf = MyFrame::theFrame->GetCatClassifFrame();
	if (!ccf) return;
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def_map, "",
										  var_info[CAT_VAR].name,
										  var_info[CAT_VAR].time);
	if (!ccs) return;
	if (cc_state_map) cc_state_vert->removeObserver(this);
	cat_classif_def_map = ccs->GetCatClassif();
	cc_state_map = ccs;
	cc_state_map->registerObserver(this);
	
	CreateAndUpdateCategories(false);
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
void ConditionalMapCanvas::ChangeCatThemeType(
							CatClassification::CatClassifType new_cat_theme,
							bool prompt_num_cats,
							const wxString& custom_classif_title)
{
	// User has already chosen theme variable on startup, so no need
	// to ever ask for theme variable.
	
	if (new_cat_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm) return;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs) return;
		if (cc_state_map == new_ccs) return;
		if (cc_state_map) cc_state_map->removeObserver(this);
		cc_state_map = new_ccs;
		cc_state_map->registerObserver(this);
		cat_classif_def_map = cc_state_map->GetCatClassif();
	} else {
		if (cc_state_map) cc_state_map->removeObserver(this);
		cc_state_map = 0;
	}
	SetCatType(new_cat_theme);
	VarInfoAttributeChange();
	CreateAndUpdateCategories(prompt_num_cats);
	UserChangedCellCategories();
	PopulateCanvas();
	if (all_init && template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

void ConditionalMapCanvas::update(CatClassifState* o)
{
	LOG_MSG("In ConditionalMapCanvas::update(CatClassifState*)");
	if (cc_state_map && cc_state_map == o) {
		CreateAndUpdateCategories(false);
		UserChangedCellCategories();
		PopulateCanvas();
		if (template_frame) {
			template_frame->UpdateTitle();
			if (template_frame->GetTemplateLegend()) {
				template_frame->GetTemplateLegend()->Refresh();
			}
		}
	} else {
		ConditionalNewCanvas::update(o);
	}
}

void ConditionalMapCanvas::ResizeSelectableShps(int virtual_scrn_w,
												int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
	LOG_MSG("Entering ConditionalMapCanvas::ResizeSelectableShps");
	int vs_w=virtual_scrn_w, vs_h=virtual_scrn_h;
	if (vs_w <= 0 && vs_h <= 0) GetVirtualSize(&vs_w, &vs_h);
	
	double image_width, image_height;
	bool ftwm = GetFitToWindowMode();
	
	// last_scale_trans is only used in calls made to ApplyLastResizeToShp
	// which are made in ScaterNewPlotView
	MyScaleTrans **st;
	st = new MyScaleTrans*[vert_num_cats];
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new MyScaleTrans[horiz_num_cats];
	}
	
	// Total width height:  vs_w   vs_h
	// Working area margins: virtual_screen_marg_top,
	//  virtual_screen_marg_bottom,
	//  virtual_screen_marg_left,
	//  virtual_screen_marg_right
	// We need to increase these as needed for each tile area
	
	double scn_w = vs_w;
	double scn_h = vs_h;
	
	// pixels between columns/rows
	double fac = 0.02;
	if (vert_num_cats >= 4 || horiz_num_cats >=4) fac = 0.015;
	double pad_w = scn_w * fac;
	double pad_h = scn_h * fac;
	if (pad_w < 1) pad_w = 1;
	if (pad_h < 1) pad_h = 1;
	double pad = GenUtils::min<double>(pad_w, pad_h);
	
	double marg_top = virtual_screen_marg_top;
	double marg_bottom = virtual_screen_marg_bottom;
	double marg_left = virtual_screen_marg_left;
	double marg_right = virtual_screen_marg_right;
	
	double d_rows = vert_num_cats;
	double d_cols = horiz_num_cats;
	
	double tot_width = scn_w - ((d_cols-1)*pad + marg_left + marg_right);
	double tot_height = scn_h - ((d_rows-1)*pad + marg_top + marg_bottom);
	double del_width = tot_width / d_cols;
	double del_height = tot_height / d_rows;
	
	bin_extents.resize(boost::extents[vert_num_cats][horiz_num_cats]);
	for (int row=0; row<vert_num_cats; row++) {
		double drow = row;
		for (int col=0; col<horiz_num_cats; col++) {
			double dcol = col;
			double ml = marg_left + col*(pad+del_width);
			double mr = marg_right + ((d_cols-1)-col)*(pad+del_width);
			double mt = marg_top + row*(pad+del_height);
			double mb = marg_bottom + ((d_rows-1)-row)*(pad+del_height);
			
			double s_x, s_y, t_x, t_y;
			MyScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
										   shps_orig_xmax, shps_orig_ymax,
										   mt, mb, ml, mr,
										   vs_w, vs_h, fixed_aspect_ratio_mode,
										   ftwm,
										   &s_x, &s_y, &t_x, &t_y,
										   ftwm ? 0 : current_shps_width,
										   ftwm ? 0 : current_shps_height,
										   &image_width, &image_height);
			st[(vert_num_cats-1)-row][col].scale_x = s_x;
			st[(vert_num_cats-1)-row][col].scale_y = s_y;
			st[(vert_num_cats-1)-row][col].trans_x = t_x;
			st[(vert_num_cats-1)-row][col].trans_y = t_y;
			st[(vert_num_cats-1)-row][col].max_scale =
				GenUtils::max<double>(s_x, s_y);
			
			wxRealPoint ll(shps_orig_xmin, shps_orig_ymin);
			wxRealPoint ur(shps_orig_xmax, shps_orig_ymax);
			bin_extents[(vert_num_cats-1)-row][col] = MyRectangle(ll, ur);
			bin_extents[(vert_num_cats-1)-row][col].applyScaleTrans(
											st[(vert_num_cats-1)-row][col]);
		}
	}
	
	bin_w = bin_extents[0][0].upper_right.x-bin_extents[0][0].lower_left.x;
	if (bin_w < 0) bin_w = -bin_w;
	bin_h = bin_extents[0][0].upper_right.y-bin_extents[0][0].lower_left.y;
	if (bin_h < 0) bin_h = -bin_h;
	
	bool bin_bm_redraw_needed = false;
	if (bin_bm &&
		(bin_bm->GetWidth() != bin_w || bin_bm->GetHeight() != bin_h)) {
		delete bin_bm; bin_bm = 0;
		bin_bm_redraw_needed = true;
	} else if (!bin_bm) {
		bin_bm_redraw_needed = true;
	}
	
	if (bin_bm_redraw_needed) {
		bin_bm = new wxBitmap(bin_w, bin_h);
		wxMemoryDC dc(*bin_bm);
		dc.SetPen(*wxWHITE_PEN);
		dc.SetBrush(*wxWHITE_BRUSH);
		dc.DrawRectangle(0, 0, bin_w, bin_h);

		double s_x, s_y, t_x, t_y;
		MyScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
									   shps_orig_xmax, shps_orig_ymax,
									   0, 0, 0, 0,
									   bin_w, bin_h, fixed_aspect_ratio_mode,
									   ftwm,
									   &s_x, &s_y, &t_x, &t_y,
									   ftwm ? 0 : current_shps_width,
									   ftwm ? 0 : current_shps_height,
									   &image_width, &image_height);
		MyScaleTrans bin_st(s_x, s_y, t_x, t_y);
		for (int i=0; i<num_obs; i++) {
			selectable_shps[i]->applyScaleTrans(bin_st);
		}
		BOOST_FOREACH( MyShape* shp, selectable_shps ) {
			shp->paintSelf(dc);
		}
		bin_bm_redraw_needed = false;
	}
	
	int row_c;
	int col_c;
	for (int i=0; i<num_obs; i++) {
		row_c = vert_cat_data.categories[var_info[VERT_VAR].time].id_to_cat[i];
		col_c = horiz_cat_data.categories[var_info[HOR_VAR].time].id_to_cat[i];
		selectable_shps[i]->applyScaleTrans(st[row_c][col_c]);
	}

	
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();	
	
	double bg_xmin = marg_left;
	double bg_xmax = scn_w-marg_right;
	double bg_ymin = marg_bottom;
	double bg_ymax = scn_h-marg_top;
		
	std::vector<wxRealPoint> v_brk_ref(vert_num_cats-1);
	std::vector<wxRealPoint> h_brk_ref(horiz_num_cats-1);
	
	for (int row=0; row<vert_num_cats-1; row++) {
		double y = (bin_extents[row][0].lower_left.y +
					bin_extents[row+1][0].upper_right.y)/2.0;
		v_brk_ref[row].x = bg_xmin;
		v_brk_ref[row].y = scn_h-y;
	}

	for (int col=0; col<horiz_num_cats-1; col++) {
		double x = (bin_extents[0][col].upper_right.x +
					bin_extents[0][col+1].lower_left.x)/2.0;
		h_brk_ref[col].x = x;
		h_brk_ref[col].y = bg_ymin;
	}
	
	MyShape* s;
	int vt = var_info[VERT_VAR].time;
	for (int row=0; row<vert_num_cats-1; row++) {
		double b;
		if (cat_classif_def_vert.cat_classif_type != CatClassification::custom){
			if (!vert_cat_data.HasBreakVal(vt, row)) continue;
			b = vert_cat_data.GetBreakVal(vt, row);
		} else {
			b = cat_classif_def_vert.breaks[row];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new MyText(t, *GeoDaConst::small_font, v_brk_ref[row], 90,
					   MyText::h_center, MyText::bottom, -7, 0);
		background_shps.push_back(s);
	}
	if (ConditionalNewCanvas::GetCatType(VERT_VAR)
		!= CatClassification::no_theme) {
		s = new MyText(ConditionalNewCanvas::GetCategoriesTitle(VERT_VAR),
					   *GeoDaConst::small_font,
					   wxRealPoint(bg_xmin, bg_ymin+(bg_ymax-bg_ymin)/2.0), 90,
					   MyText::h_center, MyText::bottom, -(7+18), 0);
		background_shps.push_back(s);
	}
	
	int rt = var_info[HOR_VAR].time;
	for (int col=0; col<horiz_num_cats-1; col++) {
		double b;
		if (cat_classif_def_horiz.cat_classif_type!= CatClassification::custom){
			if (!horiz_cat_data.HasBreakVal(vt, col)) continue;
			b = horiz_cat_data.GetBreakVal(vt, col);
		} else {
			b = cat_classif_def_horiz.breaks[col];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new MyText(t, *GeoDaConst::small_font, h_brk_ref[col], 0,
					   MyText::h_center, MyText::top, 0, 7);
		background_shps.push_back(s);
	}
	if (ConditionalNewCanvas::GetCatType(HOR_VAR)
		!= CatClassification::no_theme) {
		s = new MyText(ConditionalNewCanvas::GetCategoriesTitle(HOR_VAR),
					   *GeoDaConst::small_font,
					   wxRealPoint(bg_xmin+(bg_xmax-bg_xmin)/2.0, bg_ymin), 0,
					   MyText::h_center, MyText::top, 0, (7+18));
		background_shps.push_back(s);
	}
	
	MyScaleTrans::calcAffineParams(marg_left, marg_bottom,
								   scn_w-marg_right,
								   scn_h-marg_top,
								   marg_top, marg_bottom,
								   marg_left, marg_right,
								   vs_w, vs_h, false,
								   ftwm,
								   &last_scale_trans.scale_x,
								   &last_scale_trans.scale_y,
								   &last_scale_trans.trans_x,
								   &last_scale_trans.trans_y,
								   0, 0, &image_width, &image_height);
	last_scale_trans.max_scale =
	GenUtils::max<double>(last_scale_trans.scale_x,
						  last_scale_trans.scale_y);
	BOOST_FOREACH( MyShape* ms, background_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}
	BOOST_FOREACH( MyShape* ms, foreground_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}
	
	layer0_valid = false;
	Refresh();
	
	for (int i=0; i<vert_num_cats; i++) delete [] st[i];
	delete [] st;
	
	LOG_MSG("Exiting ConditionalMapCanvas::ResizeSelectableShps");
}

// Draw all solid background, background decorations and unhighlighted
// shapes.
void ConditionalMapCanvas::DrawLayer0()
{
	LOG_MSG("In ConditionalMapCanvas::DrawLayer0");
	wxSize sz = GetVirtualSize();
	if (!layer0_bm) resizeLayerBms(sz.GetWidth(), sz.GetHeight());
	wxMemoryDC dc(*layer0_bm);
	dc.SetPen(canvas_background_color);
	dc.SetBrush(canvas_background_color);
	dc.DrawRectangle(wxPoint(0,0), sz);
	
	// using bin_extents, tile bin_bm at every cell position
	if (bin_bm) {
		for (int i=0; i<bin_extents.shape()[0]; i++) {
			for (int j=0; j<bin_extents.shape()[1]; j++) {
				int x = bin_extents[i][j].lower_left.x;
				int y = bin_extents[i][j].upper_right.y;
				dc.DrawBitmap(*bin_bm, x, y);
				
				// Draw a red rectangle for debugging purposes
				//int w = (bin_extents[i][j].upper_right.x -
				//		 bin_extents[i][j].lower_left.x);
				//if (w < 0) w = -w;
				//int h = (bin_extents[i][j].upper_right.y -
				//		 bin_extents[i][j].lower_left.y);
				//if (h < 0) h = -h;
				//dc.SetBrush(*wxRED_BRUSH);
				//dc.DrawRectangle(wxPoint(x,y), wxSize(w,h));
			}
		}
	}
		
	BOOST_FOREACH( MyShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
	if (draw_sel_shps_by_z_val) {
		DrawSelectableShapesByZVal(dc);
	} else {
		DrawSelectableShapes(dc);
	}
	
	layer0_valid = true;
	layer1_valid = false;
	layer2_valid = false;
}



/** This method assumes that v1 is already set and valid.  It will
 recreate all canvas objects as needed and refresh the canvas.
 Assumes that CreateAndUpdateCategories has already been called.
 All data analysis will have been done in CreateAndUpdateCategories
 already. */
void ConditionalMapCanvas::PopulateCanvas()
{
	LOG_MSG("Entering ConditionalMapCanvas::PopulateCanvas");
	
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
			CreateSelShpsFromProj(selectable_shps, project);
			BOOST_FOREACH( MyShape* shp, selectable_shps ) {
				shp->setPen(bin_bg_map_pen);
				shp->setBrush(bin_bg_map_brush);
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
	
	LOG_MSG("Exiting ConditionalMapCanvas::PopulateCanvas");
}

void ConditionalMapCanvas::TitleOrTimeChange()
{
	LOG_MSG("Entering ConditionalMapCanvas::TitleOrTimeChange");
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
	
	// Note: although horiz_cat_data and vert_cat_data have canvas time steps,
	// these are not the same as map canvas time steps which can change
	// according to the time periods chosen for each variable in the
	// map.  The number of time steps for the horizontal and vertical
	// variables remains the same, but only a subset of time periods might
	// be accessed at a certain time.
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);

	UpdateNumVertHorizCats();
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting ConditionalMapCanvas::TitleOrTimeChange");
}

/** Update Categories based on num_time_vals, num_cats and ref_var_index.
 This method populates cat_var_sorted from data array. */
void ConditionalMapCanvas::CreateAndUpdateCategories(bool prompt_num_cats)
{
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) map_valid[t] = true;
	map_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) map_error_message[t] = wxEmptyString;
	
	//NOTE: cat_var_sorted is sized to current num_time_vals, but
	// cat_var_sorted_vert and horiz is sized to all available number time
	// vals.  Perhaps this should be moved into the constructor since
	// we do not allow smoothing with multiple time variables.
	cat_var_sorted.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].resize(num_obs);
		int thm_t = (var_info[CAT_VAR].sync_with_global_time ? 
					 t + var_info[CAT_VAR].time_min : var_info[CAT_VAR].time);
		for (int i=0; i<num_obs; i++) {
			cat_var_sorted[t][i].first = data[CAT_VAR][thm_t][i];
			cat_var_sorted[t][i].second = i;
		}
	}
	
	// Sort each vector in ascending order
	std::sort(cat_var_sorted[0].begin(), cat_var_sorted[0].end(),
			  GeoDa::dbl_int_pair_cmp_less);
	if (var_info[CAT_VAR].sync_with_global_time) {
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
	
	if (prompt_num_cats &&
		(GetCatType() == CatClassification::quantile ||
		 GetCatType() == CatClassification::natural_breaks ||
		 GetCatType() == CatClassification::equal_intervals))
	{
		num_cats = CatClassification::PromptNumCats(GetCatType());
	}
	if (cat_classif_def_map.cat_classif_type !=
		CatClassification::custom) {
		CatClassification::ChangeNumCats(num_cats, cat_classif_def_map);
	}
	cat_classif_def_map.color_scheme =
		CatClassification::GetColSchmForType(
							 cat_classif_def_map.cat_classif_type);
	CatClassification::PopulateCatClassifData(cat_classif_def_map,
											  cat_var_sorted,
											  cat_data, map_valid,
											  map_error_message);
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int mt = cat_data.GetCurrentCanvasTmStep();
	num_cats = cat_data.categories[mt].cat_vec.size();
	CatClassification::ChangeNumCats(num_cats, cat_classif_def_map);
}

void ConditionalMapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In ConditionalMapCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

CatClassification::CatClassifType ConditionalMapCanvas::GetCatType()
{
	return cat_classif_def_map.cat_classif_type;
}

void ConditionalMapCanvas::SetCatType(CatClassification::CatClassifType t)
{
	cat_classif_def_map.cat_classif_type = t;
}

void ConditionalMapCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "obs " << hover_obs[0]+1 << " = ";
			s << data[CAT_VAR][var_info[CAT_VAR].time][hover_obs[0]];
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = ";
			s << data[CAT_VAR][var_info[CAT_VAR].time][hover_obs[1]];
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = ";
			s << data[CAT_VAR][var_info[CAT_VAR].time][hover_obs[2]];
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


ConditionalMapLegend::ConditionalMapLegend(wxWindow *parent,
									   TemplateCanvas* t_canvas,
									   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

ConditionalMapLegend::~ConditionalMapLegend()
{
    LOG_MSG("In ConditionalMapLegend::~ConditionalMapLegend");
}


IMPLEMENT_CLASS(ConditionalMapFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalMapFrame, ConditionalNewFrame)
	EVT_ACTIVATE(ConditionalMapFrame::OnActivate)	
END_EVENT_TABLE()

ConditionalMapFrame::ConditionalMapFrame(wxFrame *parent, Project* project,
									 const std::vector<GeoDaVarInfo>& var_info,
									 const std::vector<int>& col_ids,
									 const wxString& title, const wxPoint& pos,
									 const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,
					  size, style)
{
	LOG_MSG("Entering ConditionalMapFrame::ConditionalMapFrame");

	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this);
	splitter_win->SetMinimumPaneSize(10);
	
	template_canvas = new ConditionalMapCanvas(splitter_win, this, project,
											 var_info, col_ids,
											 wxDefaultPosition,
											 wxSize(width,height));
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	
	template_legend = new ConditionalMapLegend(splitter_win, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	
	splitter_win->SplitVertically(template_legend, template_canvas,
								  GeoDaConst::map_default_legend_width);
	
	Show(true);
	LOG_MSG("Exiting ConditionalMapFrame::ConditionalMapFrame");
}

ConditionalMapFrame::~ConditionalMapFrame()
{
	LOG_MSG("In ConditionalMapFrame::~ConditionalMapFrame");
	DeregisterAsActive();
}

void ConditionalMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In ConditionalMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("ConditionalMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalMapFrame::MapMenus()
{
	LOG_MSG("In ConditionalMapFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");
	((ConditionalMapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((ConditionalMapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConditionalMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("ConditionalMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((ConditionalMapCanvas*)
		 template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConditionalMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of FramesManagerObserver interface */
void  ConditionalMapFrame::update(FramesManager* o)
{
	LOG_MSG("In ConditionalMapFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

void ConditionalMapFrame::UpdateTitle()
{
	SetTitle(template_canvas->GetCanvasTitle());
}

void ConditionalMapFrame::OnNewCustomCatClassifA()
{
	((ConditionalMapCanvas*) template_canvas)->NewCustomCatClassifMap();
}

void ConditionalMapFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, cc_title);
}

void ConditionalMapFrame::OnThemeless(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::no_theme);
}

void ConditionalMapFrame::OnHinge15(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::hinge_15);
}

void ConditionalMapFrame::OnHinge30(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::hinge_30);
}

void ConditionalMapFrame::OnQuantile(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::quantile);
}

void ConditionalMapFrame::OnPercentile(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::percentile);
}

void ConditionalMapFrame::OnStdDevMap(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::stddev);
}

void ConditionalMapFrame::OnUniqueValues(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::unique_values);
}

void ConditionalMapFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::natural_breaks);
}

void ConditionalMapFrame::OnEqualIntervals(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::equal_intervals);
}

void ConditionalMapFrame::OnSaveCategories(wxCommandEvent& event)
{
	((ConditionalMapCanvas*) template_canvas)->OnSaveCategories();
}

void ConditionalMapFrame::ChangeThemeType(
								CatClassification::CatClassifType new_theme,
								const wxString& custom_classif_title)
{
	ConditionalMapCanvas* cc = (ConditionalMapCanvas*) template_canvas;
	cc->ChangeCatThemeType(new_theme, false, custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	template_legend->Refresh();
}
