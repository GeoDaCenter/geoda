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

#include <algorithm> // sort
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>
#include <boost/foreach.hpp>
#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ConditionalMapView.h"

using namespace std;

IMPLEMENT_CLASS(ConditionalMapCanvas, ConditionalNewCanvas)
BEGIN_EVENT_TABLE(ConditionalMapCanvas, ConditionalNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalMapCanvas::CAT_VAR = 2; // main theme variable

ConditionalMapCanvas::
ConditionalMapCanvas(wxWindow *parent,
                     TemplateFrame* t_frame,
                     Project* project_s,
                     const vector<GdaVarTools::VarInfo>& v_info,
                     const vector<int>& col_ids,
                     const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   true, true, pos, size),
num_categories(1),bin_bm(0),
bin_bg_map_pen(wxColor(200,200,200)),
bin_bg_map_brush(wxColor(200,200,200)),
cc_state_map(0),
full_map_redraw_needed(true)
{
	using namespace Shapefile;
	SetCatType(CatClassification::no_theme);
	
	selectable_fill_color = GdaConst::map_default_fill_colour;

    last_scale_trans.SetMargin(25,50,50,25);
    last_scale_trans.SetFixedAspectRatio(false);
    last_scale_trans.SetData(project->main_data.header.bbox_x_min,
                             project->main_data.header.bbox_y_min,
                             project->main_data.header.bbox_x_max,
                             project->main_data.header.bbox_y_max);
    last_scale_trans.SetView(size.GetWidth(), size.GetHeight());
    
    
	if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GdaConst::map_default_highlight_colour;
	}

	use_category_brushes = true;
	if (num_obs >= 6) {
		ChangeCatThemeType(CatClassification::hinge_15, 6);
	} else {
		ChangeCatThemeType(CatClassification::unique_values, 4);
	}
	
	all_init = true;
}

ConditionalMapCanvas::~ConditionalMapCanvas()
{
	if (cc_state_map) cc_state_map->removeObserver(this);
}

void ConditionalMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ConditionalMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");
	
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

/**
 * Overwrite TemplaceCanvas Scroll
 */
void ConditionalMapCanvas::OnScrollChanged(wxScrollWinEvent& event)
{
	event.Skip();
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
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_1"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_2"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_3"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_4"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_5"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_6"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_7"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_8"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_9"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_10"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 10);
	
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
	
    // since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_1"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_2"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_3"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_4"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_5"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_6"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_7"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_8"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_9"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_10"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 10);
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_1"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_2"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_3"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_4"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_5"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_6"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_7"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_8"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_9"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_10"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 10);
	
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
    
    vector<bool> undefs(num_obs, false);
    
    for (size_t i=0; i<cat_var_undef.size(); i++) {
        for (size_t j=0; j<cat_var_undef[i].size(); j++) {
            undefs[j] = undefs[j] || cat_var_undef[i][j];
        }
    }
    
	SaveCategories(title, label, "CATEGORIES", undefs);
}

void ConditionalMapCanvas::NewCustomCatClassifMap()
{
	// we know that all three var_info variables are defined, so need
	// need to prompt user as with MapCanvas
	
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def_map.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(cat_classif_def_map.num_cats,
										 cat_classif_def_map);
		vector<wxString> temp_cat_labels; // will be ignored
		CatClassification::SetBreakPoints(cat_classif_def_map.breaks,
										  temp_cat_labels,
										  cat_var_sorted[var_info[CAT_VAR].time],
                                          cat_var_undef[var_info[CAT_VAR].time],
										  cat_classif_def_map.cat_classif_type,
										  cat_classif_def_map.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def_map.num_cats; i++) {
			cat_classif_def_map.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def_map.names[i] = cat_data.GetCategoryLabel(time, i);
		}
	}
	
	CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame(this->useScientificNotation);
    
	if (!ccf)
        return;
    
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def_map, "",
										  var_info[CAT_VAR].name,
										  var_info[CAT_VAR].time);
	if (!ccs)
        return;
    
	if (cc_state_map)
        cc_state_map->removeObserver(this);
    
	cat_classif_def_map = ccs->GetCatClassif();
	cc_state_map = ccs;
	cc_state_map->registerObserver(this);
	
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
void ConditionalMapCanvas::ChangeCatThemeType(
							CatClassification::CatClassifType new_cat_theme,
							int num_categories_s,
							const wxString& custom_classif_title)
{
	num_categories = num_categories_s;
	
	if (new_cat_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm)
            return;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs)
            return;
		if (cc_state_map == new_ccs)
            return;
        if (cc_state_map) {
            cc_state_map->removeObserver(this);
        }
		cc_state_map = new_ccs;
		cc_state_map->registerObserver(this);
		cat_classif_def_map = cc_state_map->GetCatClassif();
	} else {
        if (cc_state_map) {
            cc_state_map->removeObserver(this);
        }
		cc_state_map = 0;
	}
	SetCatType(new_cat_theme);
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
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
	if (cc_state_map == o) {
		cat_classif_def_map = o->GetCatClassif();
		CreateAndUpdateCategories();
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

void ConditionalMapCanvas::ZoomShapes(bool is_zoomin)
{
	if (sel2.x == 0 && sel2.y==0) return;
}

void ConditionalMapCanvas::ResizeSelectableShps(int virtual_scrn_w,
												int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
    int vs_w=virtual_scrn_w;
    int vs_h=virtual_scrn_h;
	if (vs_w <= 0 && vs_h <= 0)
        GetVirtualSize(&vs_w, &vs_h);
	
	// last_scale_trans is only used in calls made to ApplyLastResizeToShp
	// which are made in ScaterNewPlotView
	GdaScaleTrans **st;
	st = new GdaScaleTrans*[vert_num_cats];
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new GdaScaleTrans[horiz_num_cats];
	}
    
	double scn_w = vs_w;
	double scn_h = vs_h;
	
	// pixels between columns/rows
	double fac = 0.02;
	if (vert_num_cats >= 4 || horiz_num_cats >=4)
        fac = 0.015;
    
	double pad_w = scn_w * fac;
	double pad_h = scn_h * fac;
    
	if (pad_w < 1)
        pad_w = 1;
	if (pad_h < 1)
        pad_h = 1;
    
	double pad = GenUtils::min<double>(pad_w, pad_h);
	
	double marg_top = last_scale_trans.top_margin;
	double marg_bottom = last_scale_trans.bottom_margin;
	double marg_left = last_scale_trans.left_margin;
	double marg_right = last_scale_trans.right_margin;
	
    double shps_orig_xmin = last_scale_trans.data_x_min;
    double shps_orig_ymin = last_scale_trans.data_y_min;
    double shps_orig_xmax = last_scale_trans.data_x_max;
    double shps_orig_ymax = last_scale_trans.data_y_max;
    
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
		
            GdaScaleTrans& sub_scale_trans = st[(vert_num_cats-1)-row][col];
            //sub_scale_trans.SetFixedAspectRatio(false);
            sub_scale_trans.SetData(shps_orig_xmin, shps_orig_ymin,
                                    shps_orig_xmax, shps_orig_ymax);
            sub_scale_trans.SetMargin(mt, mb, ml, mr);
            sub_scale_trans.SetView(scn_w, scn_h);
            
			wxRealPoint ll(shps_orig_xmin, shps_orig_ymin);
			wxRealPoint ur(shps_orig_xmax, shps_orig_ymax);
			bin_extents[(vert_num_cats-1)-row][col] = GdaRectangle(ll, ur);
			bin_extents[(vert_num_cats-1)-row][col].applyScaleTrans(sub_scale_trans);
		}
	}
	
	bin_w = bin_extents[0][0].upper_right.x - bin_extents[0][0].lower_left.x;
    if (bin_w < 0) {
        bin_w = -bin_w;
    }
	bin_h = bin_extents[0][0].upper_right.y - bin_extents[0][0].lower_left.y;
    if (bin_h < 0) {
        bin_h = -bin_h;
    }
	
	bool bin_bm_redraw_needed = false;
	if (bin_bm &&
		(bin_bm->GetWidth() != bin_w || bin_bm->GetHeight() != bin_h)) {
		delete bin_bm; bin_bm = 0;
		bin_bm_redraw_needed = true;
	} else if (!bin_bm) {
		bin_bm_redraw_needed = true;
	}
	
	if (bin_bm_redraw_needed) {
        if (bin_w <= 0) bin_w = 1;
        if (bin_h <= 0) bin_h = 1;
		bin_bm = new wxBitmap(bin_w, bin_h);
		wxMemoryDC dc(*bin_bm);
		dc.SetPen(*wxWHITE_PEN);
		dc.SetBrush(*wxWHITE_BRUSH);
		dc.DrawRectangle(0, 0, bin_w, bin_h);

        GdaScaleTrans bin_st;
        bin_st.SetFixedAspectRatio(false);
        bin_st.SetMargin(0, 0, 0, 0);
        bin_st.SetData(shps_orig_xmin, shps_orig_ymin,
                       shps_orig_xmax, shps_orig_ymax);
        bin_st.SetView(bin_w, bin_h);
        
		for (int i=0; i<num_obs; i++) {
			selectable_shps[i]->applyScaleTrans(bin_st);
		}
		BOOST_FOREACH( GdaShape* shp, selectable_shps ) {
			shp->paintSelf(dc);
		}
		bin_bm_redraw_needed = false;
	}
	
	int row_c;
	int col_c;
	for (int i=0; i<num_obs; i++) {
        int v_time = var_info[VERT_VAR].time;
        int h_time = var_info[HOR_VAR].time;
		row_c = vert_cat_data.categories[v_time].id_to_cat[i];
		col_c = horiz_cat_data.categories[h_time].id_to_cat[i];
		selectable_shps[i]->applyScaleTrans(st[row_c][col_c]);
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();	
	
	double bg_xmin = marg_left;
	double bg_xmax = scn_w-marg_right;
	double bg_ymin = marg_bottom;
	double bg_ymax = scn_h-marg_top;
		
	vector<wxRealPoint> v_brk_ref(vert_num_cats-1);
	vector<wxRealPoint> h_brk_ref(horiz_num_cats-1);
	
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
	
	GdaShape* s;
	int vt = var_info[VERT_VAR].time;

	int vnn = vert_num_cats - 1;
	if (cat_classif_def_vert.cat_classif_type == CatClassification::unique_values) {
		vnn = vert_num_cats;
	}

	for (int row=0; row<vnn; row++) {
		double hh = bin_extents[row][0].lower_left.y - bin_extents[row][0].upper_right.y;
		wxRealPoint pt; 
		wxString t;

		if (cat_classif_def_vert.cat_classif_type == CatClassification::unique_values) {
			if (row <= vert_num_cats - 2)
				pt = v_brk_ref[row];
			else if (row <= vert_num_cats - 1) {
				pt = v_brk_ref[row-1];
				pt.y = pt.y + 2 * hh;
			}
			t = vert_cat_data.GetCategoryLabel(vt, row);
			pt.y = pt.y - hh;
		}
		else {
			pt = v_brk_ref[row];
			double b;
			if (cat_classif_def_vert.cat_classif_type != CatClassification::custom) {
				if (!vert_cat_data.HasBreakVal(vt, row))
					continue;
				b = vert_cat_data.GetBreakVal(vt, row);
			}
			else {
				b = cat_classif_def_vert.breaks[row];
			}
			t = GenUtils::DblToStr(b);
		}

		s = new GdaShapeText(t, *GdaConst::small_font, pt, 90,
					   GdaShapeText::h_center, GdaShapeText::bottom, -7, 0);
		foreground_shps.push_back(s);
	}
    
	if (ConditionalNewCanvas::GetCatType(VERT_VAR)
		!= CatClassification::no_theme) {
        wxString ttl = ConditionalNewCanvas::GetCategoriesTitle(VERT_VAR);
        wxRealPoint pos(bg_xmin, bg_ymin+(bg_ymax-bg_ymin)/2.0);
        s = new GdaShapeText(ttl, *GdaConst::small_font, pos, 90,
                             GdaShapeText::h_center,
                             GdaShapeText::bottom, -(7+18), 0);
		foreground_shps.push_back(s);
	}
	
	int ht = var_info[HOR_VAR].time;

	int hnn = horiz_num_cats - 1;
	if (cat_classif_def_horiz.cat_classif_type == CatClassification::unique_values) {
		hnn = horiz_num_cats;
	}

	for (int col = 0; col < hnn; col++) {
		wxString t;
		wxRealPoint pt;

		if (cat_classif_def_horiz.cat_classif_type == CatClassification::unique_values) {
			double ww = bin_extents[col][0].upper_right.x - bin_extents[col][0].lower_left.x;
			t = horiz_cat_data.GetCategoryLabel(vt, col);

			if (col <= horiz_num_cats - 2)
				pt = h_brk_ref[col];
			else if (col <= horiz_num_cats - 1) {
				pt = h_brk_ref[col-1];
				pt.x = pt.x + ww;
			}

			pt.x = pt.x - ww/2.0;
		}
		else {
			double b;
			if (cat_classif_def_horiz.cat_classif_type != CatClassification::custom) {
				if (!horiz_cat_data.HasBreakVal(ht, col))
					continue;
				b = horiz_cat_data.GetBreakVal(ht, col);
			}
			else {
				b = cat_classif_def_horiz.breaks[col];
			}
			t  = GenUtils::DblToStr(b);
			pt = h_brk_ref[col];
		}

		s = new GdaShapeText(t, *GdaConst::small_font, pt, 0,
					   GdaShapeText::h_center, GdaShapeText::top, 0, 7);
		foreground_shps.push_back(s);
	}

	if (ConditionalNewCanvas::GetCatType(HOR_VAR)
		!= CatClassification::no_theme) {
        wxString ttl = ConditionalNewCanvas::GetCategoriesTitle(HOR_VAR);
        wxRealPoint pos(bg_xmin+(bg_xmax-bg_xmin)/2.0, bg_ymin);
        
		s = new GdaShapeText(ttl, *GdaConst::small_font, pos, 0,
					   GdaShapeText::h_center, GdaShapeText::top, 0, (7+18));
		foreground_shps.push_back(s);
	}

    GdaScaleTrans background_st;
    background_st.SetFixedAspectRatio(false);
    background_st.SetData(marg_left, marg_bottom, scn_w-marg_right,
                          scn_h-marg_top);
    background_st.SetMargin(marg_top, marg_bottom, marg_left, marg_right);
    background_st.SetView(vs_w, vs_h);
    
	BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
		ms->applyScaleTrans(background_st);
	}
	BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
		ms->applyScaleTrans(background_st);
	}
	
	layer0_valid = false;
	Refresh();
	
	for (int i=0; i<vert_num_cats; i++) delete [] st[i];
	delete [] st;
	
}

// Draw all solid background, background decorations and unhighlighted
// shapes.
void ConditionalMapCanvas::DrawLayer0()
{
	wxSize sz = GetVirtualSize();
	if (!layer0_bm)
        resizeLayerBms(sz.GetWidth(), sz.GetHeight());
    
	wxMemoryDC dc(*layer0_bm);
	dc.SetPen(canvas_background_color);
	dc.SetBrush(canvas_background_color);
	dc.DrawRectangle(wxPoint(0,0), sz);
	
	// using bin_extents, tile bin_bm at every cell position
	if (bin_bm) {
		for (size_t i=0; i<bin_extents.shape()[0]; i++) {
			for (size_t j=0; j<bin_extents.shape()[1]; j++) {
				int x = bin_extents[i][j].lower_left.x;
				int y = bin_extents[i][j].upper_right.y;
                dc.DrawBitmap(*bin_bm, x, y);
			}
		}
	}
		
	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
    
    DrawSelectableShapes(dc);
	
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
	
	int canvas_ts = cat_data.GetCurrentCanvasTmStep();
	if (!map_valid[canvas_ts]) full_map_redraw_needed = true;
	
	// Note: only need to delete selectable shapes if the cartogram
	// relative positions change.  Otherwise, just reuse.
	if (full_map_redraw_needed) {
		BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
		selectable_shps.clear();
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();

	if (map_valid[canvas_ts]) {
		if (full_map_redraw_needed) {
			CreateSelShpsFromProj(selectable_shps, project);
			BOOST_FOREACH( GdaShape* shp, selectable_shps ) {
				shp->setPen(bin_bg_map_pen);
				shp->setBrush(bin_bg_map_brush);
			}
			full_map_redraw_needed = false;
		}
	} else {
		wxRealPoint cntr_ref_pnt = last_scale_trans.GetDataCenter();
		GdaShapeText* txt_shp = new GdaShapeText(map_error_message[canvas_ts],
									 *GdaConst::medium_font, cntr_ref_pnt);
		foreground_shps.push_back(txt_shp);
	}
	
	ResizeSelectableShps();
	
}

void ConditionalMapCanvas::TimeChange()
{
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
}

/** Update Categories based on num_time_vals, num_categories and ref_var_index.
 This method populates cat_var_sorted from data array. */
void ConditionalMapCanvas::CreateAndUpdateCategories()
{
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++)
        map_valid[t] = true;
    
	map_error_message.resize(num_time_vals);
    
	for (int t=0; t<num_time_vals; t++)
        map_error_message[t] = wxEmptyString;
	
	//NOTE: cat_var_sorted is sized to current num_time_vals, but
	// cat_var_sorted_vert and horiz is sized to all available number time
	// vals.  Perhaps this should be moved into the constructor since
	// we do not allow smoothing with multiple time variables.
	cat_var_sorted.resize(num_time_vals);
    cat_var_undef.resize(num_time_vals);
    
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].resize(num_obs);
        cat_var_undef[t].resize(num_obs);
        
		int thm_t = (var_info[CAT_VAR].sync_with_global_time ? 
					 t + var_info[CAT_VAR].time_min : var_info[CAT_VAR].time);
        
		for (int i=0; i<num_obs; i++) {
			cat_var_sorted[t][i].first = data[CAT_VAR][thm_t][i];
			cat_var_sorted[t][i].second = i;
            
            cat_var_undef[t][i] = data_undef[CAT_VAR][thm_t][i];
		}
	}
	
	// Sort each vector in ascending order
	sort(cat_var_sorted[0].begin(), cat_var_sorted[0].end(),
			  Gda::dbl_int_pair_cmp_less);
    
	if (var_info[CAT_VAR].sync_with_global_time) {
		for (int t=1; t<num_time_vals; t++) {
			sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	} else {
		// just copy first sorted results
		for (int t=1; t<num_time_vals; t++) {
			cat_var_sorted[t] = cat_var_sorted[0];
		}
	}
	
	if (cat_classif_def_map.cat_classif_type !=
		CatClassification::custom) {
		CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
	}
	cat_classif_def_map.color_scheme =
		CatClassification::GetColSchmForType(
							 cat_classif_def_map.cat_classif_type);
	CatClassification::PopulateCatClassifData(cat_classif_def_map,
											  cat_var_sorted,
                                              cat_var_undef,
											  cat_data,
                                              map_valid,
											  map_error_message,
                                              this->useScientificNotation);
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int mt = cat_data.GetCurrentCanvasTmStep();
	num_categories = cat_data.categories[mt].cat_vec.size();
	CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
}

void ConditionalMapCanvas::TimeSyncVariableToggle(int var_index)
{
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
    
    int t = var_info[CAT_VAR].time;
    
    const vector<bool>& hl = highlight_state->GetHighlight();
    wxString s;
    if (highlight_state->GetTotalHighlighted()> 0) {
        int n_total_hl = highlight_state->GetTotalHighlighted();
        s << "#selected=" << n_total_hl << "  ";
        
        int n_undefs = 0;
        for (int i=0; i<num_obs; i++) {
            if (cat_var_undef[t][i] && hl[i]) {
                n_undefs += 1;
            }
        }
        if (n_undefs> 0) {
            s << "(undefined:" << n_undefs << ") ";
        }
    }
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "hover obs " << hover_obs[0]+1 << " = ";
			s << data[CAT_VAR][t][hover_obs[0]];
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = ";
			s << data[CAT_VAR][t][hover_obs[1]];
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = ";
			s << data[CAT_VAR][t][hover_obs[2]];
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
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
}


IMPLEMENT_CLASS(ConditionalMapFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalMapFrame, ConditionalNewFrame)
	EVT_ACTIVATE(ConditionalMapFrame::OnActivate)	
END_EVENT_TABLE()

ConditionalMapFrame::ConditionalMapFrame(wxFrame *parent, Project* project,
									 const vector<GdaVarTools::VarInfo>& var_info,
									 const vector<int>& col_ids,
									 const wxString& title, const wxPoint& pos,
									 const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,
					  size, style)
{
    
    wxLogMessage("Open ConditionalNewFrame.");
	int width, height;
	GetClientSize(&width, &height);

		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
	template_canvas = new ConditionalMapCanvas(rpanel, this, project,
											 var_info, col_ids,
											 wxDefaultPosition,
											 wxDefaultSize);
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
	wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new ConditionalMapLegend(lpanel, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
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
}

ConditionalMapFrame::~ConditionalMapFrame()
{
	DeregisterAsActive();
}

void ConditionalMapFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In ConditionalMapFrame::OnActivate()");
		RegisterAsActive("ConditionalMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalMapFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
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
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
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


/** Implementation of TimeStateObserver interface */
void  ConditionalMapFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Recreate();
}

void ConditionalMapFrame::OnNewCustomCatClassifA()
{
	((ConditionalMapCanvas*) template_canvas)->NewCustomCatClassifMap();
}

void ConditionalMapFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, 4, cc_title);
}

void ConditionalMapFrame::OnThemeless()
{
	ChangeThemeType(CatClassification::no_theme, 1);
}

void ConditionalMapFrame::OnHinge15()
{
	ChangeThemeType(CatClassification::hinge_15, 6);
}

void ConditionalMapFrame::OnHinge30()
{
	ChangeThemeType(CatClassification::hinge_30, 6);
}

void ConditionalMapFrame::OnQuantile(int num_cats)
{
	ChangeThemeType(CatClassification::quantile, num_cats);
}

void ConditionalMapFrame::OnPercentile()
{
	ChangeThemeType(CatClassification::percentile, 6);
}

void ConditionalMapFrame::OnStdDevMap()
{
	ChangeThemeType(CatClassification::stddev, 6);
}

void ConditionalMapFrame::OnUniqueValues()
{
	ChangeThemeType(CatClassification::unique_values, 6);
}

void ConditionalMapFrame::OnNaturalBreaks(int num_cats)
{
	ChangeThemeType(CatClassification::natural_breaks, num_cats);
}

void ConditionalMapFrame::OnEqualIntervals(int num_cats)
{
	ChangeThemeType(CatClassification::equal_intervals, num_cats);
}

void ConditionalMapFrame::OnSaveCategories()
{
	((ConditionalMapCanvas*) template_canvas)->OnSaveCategories();
}

void ConditionalMapFrame::ChangeThemeType(
								CatClassification::CatClassifType new_theme,
								int num_categories,
								const wxString& custom_classif_title)
{
	ConditionalMapCanvas* cc = (ConditionalMapCanvas*) template_canvas;
	cc->ChangeCatThemeType(new_theme, num_categories, custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	template_legend->Refresh();
}
