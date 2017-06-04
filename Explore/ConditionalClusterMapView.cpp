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

#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"

#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "LisaCoordinator.h"
#include "GStatCoordinator.h"
#include "LocalGearyCoordinator.h"
#include "ConditionalClusterMapView.h"

using namespace std;

IMPLEMENT_CLASS(ConditionalClusterMapCanvas, ConditionalNewCanvas)
BEGIN_EVENT_TABLE(ConditionalClusterMapCanvas, ConditionalNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalClusterMapCanvas::CAT_VAR = 2; // main theme variable

ConditionalClusterMapCanvas::
ConditionalClusterMapCanvas(wxWindow *parent,
                            TemplateFrame* t_frame,
                            Project* project_s,
                            const vector<GdaVarTools::VarInfo>& v_info,
                            const vector<int>& col_ids,
                            const wxString& ttl,
                            const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   true, true, pos, size),
num_categories(1),bin_bm(0),
bin_bg_map_pen(wxColor(200,200,200)),
bin_bg_map_brush(wxColor(200,200,200)),
cc_state_map(0),
full_map_redraw_needed(true),
title(ttl)
{

}

ConditionalClusterMapCanvas::~ConditionalClusterMapCanvas()
{
	if (cc_state_map) cc_state_map->removeObserver(this);
}

void ConditionalClusterMapCanvas::Init(const wxSize& size)
{
    is_any_sync_with_global_time = true;
    using namespace Shapefile;
    SetCatType(CatClassification::custom);
    
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
    ChangeCatThemeType(CatClassification::custom, 5);
    
    all_init = true;
}

void ConditionalClusterMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ConditionalClusterMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");

    wxMenuItem* first_map_menu_item = optMenu->FindItemByPosition(0);
    optMenu->Delete(first_map_menu_item);
    
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
void ConditionalClusterMapCanvas::OnScrollChanged(wxScrollWinEvent& event)
{
	event.Skip();
}

wxString ConditionalClusterMapCanvas::GetCategoriesTitle()
{
	return title;
}

wxString ConditionalClusterMapCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Conditional Map - ";
	v << "x: " << GetNameWithTime(HOR_VAR);
	v << ", y: " << GetNameWithTime(VERT_VAR);
    v << ", " << title;
	return v;
}


void ConditionalClusterMapCanvas::SetCheckMarks(wxMenu* menu)
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

void ConditionalClusterMapCanvas::OnSaveCategories()
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

void ConditionalClusterMapCanvas::NewCustomCatClassifMap()
{
    // don't allow to change classification
}

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification. */
void
ConditionalClusterMapCanvas::
ChangeCatThemeType(CatClassification::CatClassifType new_cat_theme,
                   int num_categories_s,
                   const wxString& custom_classif_title)
{
    // new_cat_theme = CatClassification::lisa_significance
    // num_categories = 5
	num_categories = num_categories_s;
	
    if (cc_state_map) {
        cc_state_map->removeObserver(this);
    }
	cc_state_map = 0;
    
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

void ConditionalClusterMapCanvas::update(CatClassifState* o)
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

void ConditionalClusterMapCanvas::ZoomShapes(bool is_zoomin)
{
    // no zoom implemented
}

void ConditionalClusterMapCanvas::ResizeSelectableShps(int virtual_scrn_w,
                                                       int virtual_scrn_h)
{
    int vs_w=virtual_scrn_w;
    int vs_h=virtual_scrn_h;
    if (vs_w <= 0 && vs_h <= 0) {
        GetVirtualSize(&vs_w, &vs_h);
    }
	
    GdaScaleTrans **st = new GdaScaleTrans*[vert_num_cats];
    
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new GdaScaleTrans[horiz_num_cats];
	}
    
	double scn_w = vs_w;
	double scn_h = vs_h;
	
	// pixels between columns/rows
	double fac = 0.02;
    if (vert_num_cats >= 4 || horiz_num_cats >=4) {
        fac = 0.015;
    }
    
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
	for (int row=0; row<vert_num_cats-1; row++) {
		double b;
		if (cat_classif_def_vert.cat_classif_type != CatClassification::custom){
			if (!vert_cat_data.HasBreakVal(vt, row))
                continue;
			b = vert_cat_data.GetBreakVal(vt, row);
		} else {
			b = cat_classif_def_vert.breaks[row];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new GdaShapeText(t, *GdaConst::small_font, v_brk_ref[row], 90,
                             GdaShapeText::h_center,
                             GdaShapeText::bottom, -7, 0);
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
	for (int col=0; col<horiz_num_cats-1; col++) {
		double b;
		if (cat_classif_def_horiz.cat_classif_type!= CatClassification::custom){
			if (!horiz_cat_data.HasBreakVal(ht, col)) continue;
			b = horiz_cat_data.GetBreakVal(ht, col);
		} else {
			b = cat_classif_def_horiz.breaks[col];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new GdaShapeText(t, *GdaConst::small_font, h_brk_ref[col], 0,
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
void ConditionalClusterMapCanvas::DrawLayer0()
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
void ConditionalClusterMapCanvas::PopulateCanvas()
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

void ConditionalClusterMapCanvas::TimeChange()
{
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetTimeState()->GetCurrTime();
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
		(cts < ref_time_min && ref_time == ref_time_min))
        return;
    
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


CatClassification::CatClassifType ConditionalClusterMapCanvas::GetCatType()
{
	return cat_classif_def_map.cat_classif_type;
}

void ConditionalClusterMapCanvas::SetCatType(CatClassification::CatClassifType t)
{
	cat_classif_def_map.cat_classif_type = t;
}




ConditionalClusterMapLegend::ConditionalClusterMapLegend(wxWindow *parent,
									   TemplateCanvas* t_canvas,
									   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

ConditionalClusterMapLegend::~ConditionalClusterMapLegend()
{
}


IMPLEMENT_CLASS(ConditionalClusterMapFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalClusterMapFrame, ConditionalNewFrame)
	EVT_ACTIVATE(ConditionalClusterMapFrame::OnActivate)	
END_EVENT_TABLE()

ConditionalClusterMapFrame::
ConditionalClusterMapFrame(wxFrame *parent, Project* project,
                           const vector<GdaVarTools::VarInfo>& var_info,
                           const vector<int>& col_ids,
                           LisaCoordinator* lisa_coord,
                           const wxString& title, const wxPoint& pos,
                           const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,size, style)
{
    
    wxLogMessage("Open ConditionalNewFrame -- LISA.");
	int width, height;
	GetClientSize(&width, &height);

		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new ConditionalLISAClusterMapCanvas(rpanel, this, project,
                                                          var_info, col_ids,
                                                          lisa_coord,
                                                          title,
                                                          wxDefaultPosition,
                                                          wxDefaultSize);
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
	wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new ConditionalClusterMapLegend(lpanel, template_canvas,
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

ConditionalClusterMapFrame::
ConditionalClusterMapFrame(wxFrame *parent, Project* project,
                           const vector<GdaVarTools::VarInfo>& var_info,
                           const vector<int>& col_ids,
                           GStatCoordinator* g_coord,
                           bool is_gi, bool is_perm,
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
    template_canvas = new ConditionalGClusterMapCanvas(rpanel, this, project,
                                                       var_info, col_ids,
                                                       g_coord, is_gi, is_perm,
                                                       title,
                                                       wxDefaultPosition,
                                                       wxDefaultSize);
    SetTitle(template_canvas->GetCanvasTitle());
    template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
    wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new ConditionalClusterMapLegend(lpanel, template_canvas,
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

ConditionalClusterMapFrame::
ConditionalClusterMapFrame(wxFrame *parent, Project* project,
                           const vector<GdaVarTools::VarInfo>& var_info,
                           const vector<int>& col_ids,
                           LocalGearyCoordinator* local_geary_coord,
                           const wxString& title, const wxPoint& pos,
                           const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,size, style)
{
    
    wxLogMessage("Open ConditionalNewFrame -- Local Geary.");
	int width, height;
	GetClientSize(&width, &height);

		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new ConditionalLocalGearyClusterMapCanvas(rpanel, this, project,
                                                          var_info, col_ids,
                                                          local_geary_coord,
                                                          title,
                                                          wxDefaultPosition,
                                                          wxDefaultSize);
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
	wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new ConditionalClusterMapLegend(lpanel, template_canvas,
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

ConditionalClusterMapFrame::~ConditionalClusterMapFrame()
{
	DeregisterAsActive();
}

void ConditionalClusterMapFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In ConditionalClusterMapFrame::OnActivate()");
		RegisterAsActive("ConditionalClusterMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalClusterMapFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");
    
    wxMenuItem* first_map_menu_item = optMenu->FindItemByPosition(0);
    optMenu->Delete(first_map_menu_item);

	((ConditionalClusterMapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu, project->GetCatClassifManager());
	((ConditionalClusterMapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConditionalClusterMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((ConditionalClusterMapCanvas*)
		 template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConditionalClusterMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}


/** Implementation of TimeStateObserver interface */
void  ConditionalClusterMapFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Recreate();
}

void ConditionalClusterMapFrame::OnNewCustomCatClassifA()
{
	((ConditionalClusterMapCanvas*) template_canvas)->NewCustomCatClassifMap();
}

void ConditionalClusterMapFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, 4, cc_title);
}

void ConditionalClusterMapFrame::OnThemeless()
{
	ChangeThemeType(CatClassification::no_theme, 1);
}

void ConditionalClusterMapFrame::OnHinge15()
{
	ChangeThemeType(CatClassification::hinge_15, 6);
}

void ConditionalClusterMapFrame::OnHinge30()
{
	ChangeThemeType(CatClassification::hinge_30, 6);
}

void ConditionalClusterMapFrame::OnQuantile(int num_cats)
{
	ChangeThemeType(CatClassification::quantile, num_cats);
}

void ConditionalClusterMapFrame::OnPercentile()
{
	ChangeThemeType(CatClassification::percentile, 6);
}

void ConditionalClusterMapFrame::OnStdDevMap()
{
	ChangeThemeType(CatClassification::stddev, 6);
}

void ConditionalClusterMapFrame::OnUniqueValues()
{
	ChangeThemeType(CatClassification::unique_values, 6);
}

void ConditionalClusterMapFrame::OnNaturalBreaks(int num_cats)
{
	ChangeThemeType(CatClassification::natural_breaks, num_cats);
}

void ConditionalClusterMapFrame::OnEqualIntervals(int num_cats)
{
	ChangeThemeType(CatClassification::equal_intervals, num_cats);
}

void ConditionalClusterMapFrame::OnSaveCategories()
{
	((ConditionalClusterMapCanvas*) template_canvas)->OnSaveCategories();
}

void ConditionalClusterMapFrame::ChangeThemeType(
								CatClassification::CatClassifType new_theme,
								int num_categories,
								const wxString& custom_classif_title)
{
	ConditionalClusterMapCanvas* cc = (ConditionalClusterMapCanvas*) template_canvas;
	cc->ChangeCatThemeType(new_theme, num_categories, custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	template_legend->Refresh();
}


///////////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////////
ConditionalLISAClusterMapCanvas::
ConditionalLISAClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                Project* project,
                                const vector<GdaVarTools::VarInfo>& var_info,
                                const vector<int>& col_ids,
                                LisaCoordinator* lisa_coordinator,
                                const wxString& title,
                                const wxPoint& pos,
                                const wxSize& size)
: ConditionalClusterMapCanvas(parent, t_frame, project, var_info, col_ids, title, pos, size),
lisa_coord(lisa_coordinator)
{
    Init(size);
}

ConditionalLISAClusterMapCanvas::~ConditionalLISAClusterMapCanvas()
{
    
}

void ConditionalLISAClusterMapCanvas::CreateAndUpdateCategories()
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
        
        for (int i=0; i<num_obs; i++) {
            cat_var_sorted[t][i].first = lisa_coord->cluster_vecs[t][i];
            cat_var_sorted[t][i].second = i;
            
            cat_var_undef[t][i] = lisa_coord->undef_data[0][t][i];
        }
    }
    
    // Sort each vector in ascending order
    sort(cat_var_sorted[0].begin(),
         cat_var_sorted[0].end(), Gda::dbl_int_pair_cmp_less);
    
    if (is_any_sync_with_global_time) {
        for (int t=1; t<num_time_vals; t++) {
            sort(cat_var_sorted[t].begin(),
                 cat_var_sorted[t].end(), Gda::dbl_int_pair_cmp_less);
        }
    } else {
        // just copy first sorted results
        for (int t=1; t<num_time_vals; t++) {
            cat_var_sorted[t] = cat_var_sorted[0];
        }
    }
    
    cat_classif_def_map.color_scheme = CatClassification::custom_color_scheme;
    
    // get cat_data
    int num_time = lisa_coord->num_time_vals;
    int num_obs = lisa_coord->num_obs;
    cat_data.CreateEmptyCategories(num_time, num_obs);
    
    for (int t=0; t<num_time_vals; t++) {
        int undefined_cat = -1;
        int isolates_cat = -1;
        int num_cats = 0;
        double stop_sig = 0;
        
        if (lisa_coord->GetHasIsolates(t))
            num_cats++;
        if (lisa_coord->GetHasUndefined(t))
            num_cats++;
        
        num_cats += 5;
        
        cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
        
        Shapefile::Header& hdr = project->main_data.header;
        
        cat_data.SetCategoryLabel(t, 0, "Not Significant");
        
        if (hdr.shape_type == Shapefile::POINT_TYP) {
            cat_data.SetCategoryColor(t, 0, wxColour(190, 190, 190));
        } else {
            cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
        }
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
        for (int cat=0; cat<num_cats; cat++) {
            cat_data.SetCategoryCount(t, cat,
                                      cat_data.GetNumObsInCategory(t, cat));
        }
    }
    
    cat_data.SetCurrentCanvasTmStep(0);
    int mt = cat_data.GetCurrentCanvasTmStep();
    num_categories = cat_data.categories[mt].cat_vec.size();
    CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
    
}

void ConditionalLISAClusterMapCanvas::TimeSyncVariableToggle(int var_index)
{
    lisa_coord->var_info[0].sync_with_global_time = !lisa_coord->var_info[0].sync_with_global_time;
    
    VarInfoAttributeChange();
    CreateAndUpdateCategories();
    PopulateCanvas();
}

void ConditionalLISAClusterMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = template_frame->GetStatusBar();
    if (!sb) return;
    
    //int t = var_info[CAT_VAR].time;
    int t = 0;
    
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
            s << lisa_coord->cluster_vecs[t][hover_obs[0]];
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << "obs " << hover_obs[1]+1 << " = ";
            s << lisa_coord->cluster_vecs[t][hover_obs[1]];
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << "obs " << hover_obs[2]+1 << " = ";
            s << lisa_coord->cluster_vecs[t][hover_obs[2]];
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    }
    sb->SetStatusText(s);
}

///////////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////////
ConditionalGClusterMapCanvas::
ConditionalGClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project,
                             const vector<GdaVarTools::VarInfo>& var_info,
                             const vector<int>& col_ids,
                             GStatCoordinator* g_coordinator,
                             bool is_gi_,
                             bool is_perm_,
                             const wxString& title,
                             const wxPoint& pos,
                             const wxSize& size)
: ConditionalClusterMapCanvas(parent, t_frame, project, var_info, col_ids, title, pos, size),
g_coord(g_coordinator), is_gi(is_gi_), is_perm(is_perm_)
{
    Init(size);
}

ConditionalGClusterMapCanvas::~ConditionalGClusterMapCanvas()
{
    
}

void ConditionalGClusterMapCanvas::CreateAndUpdateCategories()
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
        vector<wxInt64> cluster;
        g_coord->FillClusterCats(t, is_gi, is_perm, cluster);
        
        for (int i=0; i<num_obs; i++) {
            cat_var_sorted[t][i].first = cluster[i];
            cat_var_sorted[t][i].second = i;
            
            cat_var_undef[t][i] = g_coord->data_undef[0][t][i];
        }
    }
    
    // Sort each vector in ascending order
    sort(cat_var_sorted[0].begin(),
         cat_var_sorted[0].end(), Gda::dbl_int_pair_cmp_less);
    
    if (is_any_sync_with_global_time) {
        for (int t=1; t<num_time_vals; t++) {
            sort(cat_var_sorted[t].begin(),
                 cat_var_sorted[t].end(), Gda::dbl_int_pair_cmp_less);
        }
    } else {
        // just copy first sorted results
        for (int t=1; t<num_time_vals; t++) {
            cat_var_sorted[t] = cat_var_sorted[0];
        }
    }
    
    cat_classif_def_map.color_scheme = CatClassification::custom_color_scheme;
    
    // get cat_data
    int num_time = g_coord->num_time_vals;
    int num_obs = g_coord->num_obs;
    cat_data.CreateEmptyCategories(num_time, num_obs);
    
    for (int t=0; t<num_time_vals; t++) {
        int undefined_cat = -1;
        int isolates_cat = -1;
        int num_cats = 0;
        double stop_sig = 0;
        
        if (g_coord->GetHasIsolates(t))
            num_cats++;
        if (g_coord->GetHasUndefined(t))
            num_cats++;
        
        num_cats += 3;
        
        cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
        
        Shapefile::Header& hdr = project->main_data.header;
        
        cat_data.SetCategoryLabel(t, 0, "Not Significant");
        
        cat_data.SetCategoryLabel(t, 0, "Not Significant");
        cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
        cat_data.SetCategoryLabel(t, 1, "High");
        cat_data.SetCategoryColor(t, 1, wxColour(255, 0, 0));
        cat_data.SetCategoryLabel(t, 2, "Low");
        cat_data.SetCategoryColor(t, 2, wxColour(0, 0, 255));
        
        if (g_coord->GetHasIsolates(t) &&
            g_coord->GetHasUndefined(t))
        {
            isolates_cat = 3;
            undefined_cat = 4;
        } else if (g_coord->GetHasUndefined(t)) {
            undefined_cat = 3;
        } else if (g_coord->GetHasIsolates(t)) {
            isolates_cat = 3;
        }
        
        if (undefined_cat != -1) {
            cat_data.SetCategoryLabel(t, undefined_cat, "Undefined");
            cat_data.SetCategoryColor(t, undefined_cat, wxColour(70, 70, 70));
        }
        if (isolates_cat != -1) {
            cat_data.SetCategoryLabel(t, isolates_cat, "Neighborless");
            cat_data.SetCategoryColor(t, isolates_cat, wxColour(140, 140, 140));
        }
        
        vector<wxInt64> cluster;
        g_coord->FillClusterCats(t, is_gi, is_perm, cluster);
        
        for (int i=0, iend=g_coord->num_obs; i<iend; i++) {
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
        
        for (int cat=0; cat<num_cats; cat++) {
            cat_data.SetCategoryCount(t, cat,
                                      cat_data.GetNumObsInCategory(t, cat));
        }
    }
    
    cat_data.SetCurrentCanvasTmStep(0);
    int mt = cat_data.GetCurrentCanvasTmStep();
    num_categories = cat_data.categories[mt].cat_vec.size();
    CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
    
}

void ConditionalGClusterMapCanvas::TimeSyncVariableToggle(int var_index)
{
    g_coord->var_info[0].sync_with_global_time = !g_coord->var_info[0].sync_with_global_time;
    
    VarInfoAttributeChange();
    CreateAndUpdateCategories();
    PopulateCanvas();
}

void ConditionalGClusterMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = template_frame->GetStatusBar();
    if (!sb) return;
    
    //int t = var_info[CAT_VAR].time;
    int t = 0;
    
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
    /*
    if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << "hover obs " << hover_obs[0]+1 << " = ";
            s << g_coord->cluster_vecs[t][hover_obs[0]];
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << "obs " << hover_obs[1]+1 << " = ";
            s << g_coord->cluster_vecs[t][hover_obs[1]];
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << "obs " << hover_obs[2]+1 << " = ";
            s << g_coord->cluster_vecs[t][hover_obs[2]];
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    } 
     */
    sb->SetStatusText(s);
}

///////////////////////////////////////////////////////////////////////////////
//
// Local Geary Cluster
//
///////////////////////////////////////////////////////////////////////////////
ConditionalLocalGearyClusterMapCanvas::
ConditionalLocalGearyClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                Project* project,
                                const vector<GdaVarTools::VarInfo>& var_info,
                                const vector<int>& col_ids,
                                LocalGearyCoordinator* local_geary_coordinator,
                                const wxString& title,
                                const wxPoint& pos,
                                const wxSize& size)
: ConditionalClusterMapCanvas(parent, t_frame, project, var_info, col_ids, title, pos, size),
local_geary_coord(local_geary_coordinator)
{
    Init(size);
}

ConditionalLocalGearyClusterMapCanvas::~ConditionalLocalGearyClusterMapCanvas()
{
    
}

void ConditionalLocalGearyClusterMapCanvas::CreateAndUpdateCategories()
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
        
        for (int i=0; i<num_obs; i++) {
            cat_var_sorted[t][i].first = local_geary_coord->cluster_vecs[t][i];
            cat_var_sorted[t][i].second = i;
            
            cat_var_undef[t][i] = local_geary_coord->undef_data[0][t][i];
        }
    }
    
    // Sort each vector in ascending order
    sort(cat_var_sorted[0].begin(),
         cat_var_sorted[0].end(), Gda::dbl_int_pair_cmp_less);
    
    if (is_any_sync_with_global_time) {
        for (int t=1; t<num_time_vals; t++) {
            sort(cat_var_sorted[t].begin(),
                 cat_var_sorted[t].end(), Gda::dbl_int_pair_cmp_less);
        }
    } else {
        // just copy first sorted results
        for (int t=1; t<num_time_vals; t++) {
            cat_var_sorted[t] = cat_var_sorted[0];
        }
    }
    
    cat_classif_def_map.color_scheme = CatClassification::custom_color_scheme;
    
    // get cat_data
    int num_time = local_geary_coord->num_time_vals;
    int num_obs = local_geary_coord->num_obs;
    cat_data.CreateEmptyCategories(num_time, num_obs);
    
    for (int t=0; t<num_time_vals; t++) {
        int undefined_cat = -1;
        int isolates_cat = -1;
        int num_cats = 0;
        double stop_sig = 0;
        
        if (local_geary_coord->GetHasIsolates(t))
            num_cats++;
        if (local_geary_coord->GetHasUndefined(t))
            num_cats++;
        
        num_cats += 5;
        
        cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
        
        Shapefile::Header& hdr = project->main_data.header;
        
        cat_data.SetCategoryLabel(t, 0, "Not Significant");
        
        if (hdr.shape_type == Shapefile::POINT_TYP) {
            cat_data.SetCategoryColor(t, 0, wxColour(190, 190, 190));
        } else {
            cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
        }
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
        for (int cat=0; cat<num_cats; cat++) {
            cat_data.SetCategoryCount(t, cat,
                                      cat_data.GetNumObsInCategory(t, cat));
        }
    }
    
    cat_data.SetCurrentCanvasTmStep(0);
    int mt = cat_data.GetCurrentCanvasTmStep();
    num_categories = cat_data.categories[mt].cat_vec.size();
    CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
    
}

void ConditionalLocalGearyClusterMapCanvas::TimeSyncVariableToggle(int var_index)
{
    local_geary_coord->var_info[0].sync_with_global_time = !local_geary_coord->var_info[0].sync_with_global_time;
    
    VarInfoAttributeChange();
    CreateAndUpdateCategories();
    PopulateCanvas();
}

void ConditionalLocalGearyClusterMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = template_frame->GetStatusBar();
    if (!sb) return;
    
    //int t = var_info[CAT_VAR].time;
    int t = 0;
    
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
            s << local_geary_coord->cluster_vecs[t][hover_obs[0]];
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << "obs " << hover_obs[1]+1 << " = ";
            s << local_geary_coord->cluster_vecs[t][hover_obs[1]];
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << "obs " << hover_obs[2]+1 << " = ";
            s << local_geary_coord->cluster_vecs[t][hover_obs[2]];
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    }
    sb->SetStatusText(s);
}
