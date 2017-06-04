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

#include <algorithm> // std::sort
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ConditionalScatterPlotView.h"


IMPLEMENT_CLASS(ConditionalScatterPlotCanvas, ConditionalNewCanvas)
	BEGIN_EVENT_TABLE(ConditionalScatterPlotCanvas, ConditionalNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalScatterPlotCanvas::IND_VAR = 2; // x-axis independent var
const int ConditionalScatterPlotCanvas::DEP_VAR = 3; // y-axis dependent var

ConditionalScatterPlotCanvas::
ConditionalScatterPlotCanvas(wxWindow *parent,
                             TemplateFrame* t_frame,
                             Project* project_s,
                             const std::vector<GdaVarTools::VarInfo>& v_info,
                             const std::vector<int>& col_ids,
                             const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   false, true, pos, size),
full_map_redraw_needed(true),
X(project_s->GetNumRecords()),
Y(project_s->GetNumRecords()),
XY_undef(project_s->GetNumRecords()),
display_axes_scale_values(true),
display_slope_values(true),
show_linear_smoother(true),
show_lowess_smoother(false)
{
	double x_max = var_info[IND_VAR].max_over_time;
	double x_min = var_info[IND_VAR].min_over_time;
	double y_max = var_info[DEP_VAR].max_over_time;
	double y_min = var_info[DEP_VAR].min_over_time;
	double x_pad = 0.1 * (x_max - x_min);
	double y_pad = 0.1 * (y_max - y_min);
	axis_scale_x = AxisScale(x_min - x_pad, x_max + x_pad, 4, axis_display_precision);
	axis_scale_x.SkipEvenTics();
	axis_scale_y = AxisScale(y_min - y_pad, y_max + y_pad, 4, axis_display_precision);
	axis_scale_y.SkipEvenTics();
	
	highlight_color = GdaConst::scatterplot_regression_selected_color;
	selectable_fill_color = GdaConst::scatterplot_regression_excluded_color;
	selectable_outline_color = GdaConst::scatterplot_regression_color;
	
    last_scale_trans.SetData(0, 0, 100, 100);
    last_scale_trans.SetMargin(25, 75, 75, 25);
	
	selectable_shps_type = points;
	use_category_brushes = true;
	
	ref_var_index = -1;
	num_time_vals = 1;
	for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable)
            ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	// 1 = #cats
	cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
	for (int t=0; t<num_time_vals; t++) {
		cat_data.SetCategoryColor(t, 0, selectable_fill_color);
		for (int i=0; i<num_obs; i++) {
			cat_data.AppendIdToCategory(t, 0, i);
		}
	}
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
    
	VarInfoAttributeChange();
	PopulateCanvas();
	
	all_init = true;
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

ConditionalScatterPlotCanvas::~ConditionalScatterPlotCanvas()
{
	EmptyLowessCache();
}

void ConditionalScatterPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ConditionalScatterPlotFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void ConditionalScatterPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	ConditionalNewCanvas::SetCheckMarks(menu);

	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_LINEAR_SMOOTHER"),
                                  IsShowLinearSmoother());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_LOWESS_SMOOTHER"),
                                  IsShowLowessSmoother());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_AXES_SCALE_VALUES"),
                                  IsDisplayAxesScaleValues());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_SLOPE_VALUES"),
                                  IsDisplaySlopeValues());
}

wxString ConditionalScatterPlotCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Cond. Scatter Plot - ";
	v << "x: " << GetNameWithTime(HOR_VAR);
	v << ", y: " << GetNameWithTime(VERT_VAR);
	v << ", x (ind.): " << GetNameWithTime(IND_VAR);
	v << ", y (dep.): " << GetNameWithTime(DEP_VAR);
	return v;
}


void ConditionalScatterPlotCanvas::ResizeSelectableShps(int virtual_scrn_w,
														int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
    int vs_w = virtual_scrn_w;
    int vs_h = virtual_scrn_h;
    
    if (vs_w <= 0 && vs_h <= 0) {
        GetVirtualSize(&vs_w, &vs_h);
    }
	
	//double image_width, image_height;
	
	// last_scale_trans is only used in calls made to ApplyLastResizeToShp
	// which are made in ScaterNewPlotView
	GdaScaleTrans **st;
	st = new GdaScaleTrans*[vert_num_cats];
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new GdaScaleTrans[horiz_num_cats];
	}
		
	double scn_w = vs_w;
	double scn_h = vs_h;
	
	double min_pad = 10;
	if (display_axes_scale_values) {
		min_pad += 20;
	}
	if (!display_axes_scale_values && display_slope_values) {
		min_pad += 4;
	}
	
	// pixels between columns/rows
	double fac = 0.01;
	//if (vert_num_cats >= 4 || horiz_num_cats >=4) fac = 0.05;
	double pad_w = scn_w * fac;
	double pad_h = scn_h * fac;
	if (pad_w < 1) pad_w = 0;
	if (pad_h < 1) pad_h = 0;
	double pad_bump = GenUtils::min<double>(pad_w, pad_h);
	double pad = min_pad + pad_bump;
	
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
		
            GdaScaleTrans& sub_st = st[(vert_num_cats-1)-row][col];
            sub_st.SetData(shps_orig_xmin, shps_orig_ymin,
                           shps_orig_xmax, shps_orig_ymax);
            sub_st.SetView(vs_w, vs_h);
            sub_st.SetMargin(mt, mb, ml, mr);
            
			wxRealPoint ll(shps_orig_xmin, shps_orig_ymin);
			wxRealPoint ur(shps_orig_xmax, shps_orig_ymax);
			bin_extents[(vert_num_cats-1)-row][col] = GdaRectangle(ll, ur);
			bin_extents[(vert_num_cats-1)-row][col].applyScaleTrans(sub_st);
		}
	}

    BOOST_FOREACH( GdaShape* shp , foreground_shps ) { delete shp; }
    foreground_shps.clear();
	
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
	
	int label_offset = 12;
	if (display_axes_scale_values)
        label_offset = 2+25;
	
    GdaShape* s;
	int vt = var_info[VERT_VAR].time;
	for (int row=0; row<vert_num_cats-1; row++) {
		double b;
		if (cat_classif_def_vert.cat_classif_type !=
            CatClassification::custom)
        {
			if (!vert_cat_data.HasBreakVal(vt, row))
                continue;
			b = vert_cat_data.GetBreakVal(vt, row);
		} else {
			b = cat_classif_def_vert.breaks[row];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new GdaShapeText(t, *GdaConst::small_font, v_brk_ref[row], 90,
                             GdaShapeText::h_center, GdaShapeText::bottom,
                             -label_offset, 0);
		foreground_shps.push_back(s);
	}
	
	wxString vert_label;
	if (GetCatType(VERT_VAR) != CatClassification::no_theme) {
		if (GetCatType(VERT_VAR) == CatClassification::custom) {
			vert_label << cat_classif_def_vert.title;
		} else {
			vert_label << CatClassification::CatClassifTypeToString(
														GetCatType(VERT_VAR));
		}
		vert_label << " vert cat var: ";
		vert_label << GetNameWithTime(VERT_VAR);
		vert_label << ",   ";
	}
	vert_label << "dep. var: " << GetNameWithTime(DEP_VAR);
	s = new GdaShapeText(vert_label, *GdaConst::small_font,
				   wxRealPoint(bg_xmin, bg_ymin+(bg_ymax-bg_ymin)/2.0), 90,
				   GdaShapeText::h_center, GdaShapeText::bottom, -(label_offset+18), 0);
	foreground_shps.push_back(s);
	
	int ht = var_info[HOR_VAR].time;
	for (int col=0; col<horiz_num_cats-1; col++) {
		double b;
		if (cat_classif_def_horiz.cat_classif_type!= CatClassification::custom){
			if (!horiz_cat_data.HasBreakVal(ht, col))
                continue;
			b = horiz_cat_data.GetBreakVal(ht, col);
		} else {
			b = cat_classif_def_horiz.breaks[col];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new GdaShapeText(t, *GdaConst::small_font, h_brk_ref[col], 0,
					   GdaShapeText::h_center, GdaShapeText::top, 0, label_offset);
		foreground_shps.push_back(s);
	}
	
	wxString horiz_label;
	if (GetCatType(HOR_VAR) != CatClassification::no_theme) {
		if (GetCatType(HOR_VAR) == CatClassification::custom) {
			horiz_label << cat_classif_def_horiz.title;
		} else {
			horiz_label << CatClassification::CatClassifTypeToString(
														GetCatType(HOR_VAR));
		}
		horiz_label << " horiz cat var: ";
		horiz_label << GetNameWithTime(HOR_VAR);
		horiz_label << ",   ";
	}
	horiz_label << "ind. var: " << GetNameWithTime(IND_VAR);
	s = new GdaShapeText(horiz_label, *GdaConst::small_font,
				   wxRealPoint(bg_xmin+(bg_xmax-bg_xmin)/2.0, bg_ymin), 0,
				   GdaShapeText::h_center, GdaShapeText::top, 0, (label_offset+18));
	foreground_shps.push_back(s);

    GdaScaleTrans background_st;
    background_st.SetData(marg_left, marg_bottom, scn_w-marg_right,
                             scn_h-marg_top);
    background_st.SetMargin(marg_top, marg_bottom, marg_left, marg_right);
    background_st.SetView(vs_w, vs_h);
    
	BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
		ms->applyScaleTrans(background_st);
	}

    
    for (int row=0; row<vert_num_cats; row++) {
        for (int col=0; col<horiz_num_cats; col++) {
            GdaPolyLine* p = new GdaPolyLine(reg_line[row][col]);
            p->applyScaleTrans(st[row][col]);
            foreground_shps.push_back(p);
            
            // Note: It would be better to not have to re-allocate
            // potentially large GdaSpline objects on resize.  This could
            // be avoided by not deleting the GdaSpline objects in the above
            // loop.  But, memory allocation / freeing would have to be
            // handled carefully.
            GdaSpline* sp = new GdaSpline(reg_line_lowess[row][col]);
            sp->applyScaleTrans(st[row][col]);
            foreground_shps.push_back(sp);
            
            
            GdaAxis* x_ax = new GdaAxis("", axis_scale_x,
                                        wxRealPoint(0,0), wxRealPoint(100, 0));
            if (!display_axes_scale_values)
                x_ax->hideScaleValues(true);
            x_ax->setPen(*GdaConst::scatterplot_scale_pen);
            x_ax->applyScaleTrans(st[row][col]);
            foreground_shps.push_back(x_ax);
            
            GdaAxis* y_ax = new GdaAxis("", axis_scale_y,
                                        wxRealPoint(0,0), wxRealPoint(0, 100));
            if (!display_axes_scale_values)
                y_ax->hideScaleValues(true);
            y_ax->setPen(*GdaConst::scatterplot_scale_pen);
            y_ax->applyScaleTrans(st[row][col]);
            foreground_shps.push_back(y_ax);
            
            if (display_slope_values && regression[row][col].valid) {
                wxString s;
                s << GenUtils::DblToStr(regression[row][col].beta);
                if (regression[row][col].p_value_beta <= 0.01 &&
                    stats_x[row][col].sample_size >= 3) {
                    s << "**";
                } else if (regression[row][col].p_value_beta <= 0.05 &&
                           stats_x[row][col].sample_size >= 3) {
                    s << "*";
                }
                GdaShapeText* t =new GdaShapeText(s, *GdaConst::small_font,
                                                  wxRealPoint(50, 100), 0,
                                                  GdaShapeText::h_center,
                                                  GdaShapeText::v_center);
                t->setPen(*GdaConst::scatterplot_scale_pen);
                t->applyScaleTrans(st[row][col]);
                foreground_shps.push_back(t);
            }
        }
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
    
	layer0_valid = false;
	Refresh();
	
	for (int i=0; i<vert_num_cats; i++) delete [] st[i];
	delete [] st;
}

void ConditionalScatterPlotCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	selectable_shps.resize(num_obs);
	selectable_shps_undefs.clear();
	selectable_shps_undefs.resize(num_obs);
    
	for (int i=0; i<num_obs; i++) {
		X[i] = data[IND_VAR][var_info[IND_VAR].time][i];
		Y[i] = data[DEP_VAR][var_info[DEP_VAR].time][i];
        XY_undef[i] = data_undef[IND_VAR][var_info[IND_VAR].time][i] ||
                      data_undef[DEP_VAR][var_info[DEP_VAR].time][i];
	}
	double scaleX = 100.0 / (axis_scale_x.scale_range);
	double scaleY = 100.0 / (axis_scale_y.scale_range);	
	for (int i=0; i<num_obs; i++) {
		selectable_shps[i] = 
		new GdaPoint(wxRealPoint((X[i] - axis_scale_x.scale_min) * scaleX,
								(Y[i] - axis_scale_y.scale_min) * scaleY));
        selectable_shps_undefs[i] = XY_undef[i];
	}
	CalcCellsRegression();
	ResizeSelectableShps();
}

void ConditionalScatterPlotCanvas::CalcCellsRegression()
{
	typedef std::vector<double>* dvec_ptr;
	typedef boost::multi_array<dvec_ptr, 2> dvec_ptr_array_type;
	typedef boost::multi_array<int, 2> i_array_type;
	i_array_type sizes(boost::extents[vert_num_cats][horiz_num_cats]);
	i_array_type ind(boost::extents[vert_num_cats][horiz_num_cats]);
	dvec_ptr_array_type dvec_xp(boost::extents[vert_num_cats][horiz_num_cats]);
	dvec_ptr_array_type dvec_yp(boost::extents[vert_num_cats][horiz_num_cats]);
	for (int i=0; i<vert_num_cats; i++) {
		for (int j=0; j<horiz_num_cats; j++) {
			sizes[i][j]=0;
			ind[i][j]=0;
		}
	}
	stats_x.resize(boost::extents[0][0]);
	stats_x.resize(boost::extents[vert_num_cats][horiz_num_cats]);
	stats_y.resize(boost::extents[0][0]);
	stats_y.resize(boost::extents[vert_num_cats][horiz_num_cats]);
	regression.resize(boost::extents[0][0]);
	regression.resize(boost::extents[vert_num_cats][horiz_num_cats]);
	reg_line.resize(boost::extents[0][0]);
	reg_line.resize(boost::extents[vert_num_cats][horiz_num_cats]);
	reg_line_lowess.resize(boost::extents[0][0]);
	reg_line_lowess.resize(boost::extents[vert_num_cats][horiz_num_cats]);
    
	int vt=var_info[VERT_VAR].time;
	int ht=var_info[HOR_VAR].time;
	int xt = var_info[IND_VAR].time;
	int yt = var_info[DEP_VAR].time;
    
	for (int i=0; i<num_obs; i++) {
        if (XY_undef[i])
            continue;
		int row = vert_cat_data.categories[vt].id_to_cat[i];
		int col = horiz_cat_data.categories[ht].id_to_cat[i];		
		sizes[row][col]++;
	}
	for (int i=0; i<vert_num_cats; i++) {
		for (int j=0; j<horiz_num_cats; j++) {
			dvec_xp[i][j] = new std::vector<double>(sizes[i][j]);
			dvec_yp[i][j] = new std::vector<double>(sizes[i][j]);
		}
	}
	for (int i=0; i<num_obs; i++) {
        if (XY_undef[i])
            continue;
		double x = data[IND_VAR][xt][i];
		double y = data[DEP_VAR][yt][i];
		int row = vert_cat_data.categories[vt].id_to_cat[i];
		int col = horiz_cat_data.categories[ht].id_to_cat[i];
		std::vector<double>& xref = *dvec_xp[row][col];
		std::vector<double>& yref = *dvec_yp[row][col];
		int index = ind[row][col];
		xref[index] = x;
		yref[index] = y;
		ind[row][col]++;
	}
	
	wxPen linear_smth_pen = (IsShowLinearSmoother() ?
                             wxPen(selectable_outline_color) : *wxTRANSPARENT_PEN);
	wxPen lowess_smth_pen = (IsShowLowessSmoother() ?
                             wxPen(selectable_outline_color) : *wxTRANSPARENT_PEN);
    
	for (int i=0; i<vert_num_cats; i++) {
		for (int j=0; j<horiz_num_cats; j++) {
			const std::vector<double>& xref = *dvec_xp[i][j];
			const std::vector<double>& yref = *dvec_yp[i][j];
			stats_x[i][j].CalculateFromSample(xref);
			stats_y[i][j].CalculateFromSample(yref);
			regression[i][j].CalculateRegression(xref, yref,
										stats_x[i][j].mean, stats_y[i][j].mean,
										stats_x[i][j].var_without_bessel,
										stats_y[i][j].var_without_bessel);
			
			double cc_degs_of_rot;
			double reg_line_slope;
			bool   reg_line_infinite_slope;
			bool   reg_line_defined;
			wxRealPoint a, b;
            SmoothingUtils::CalcRegressionLine(reg_line[i][j],
                                               reg_line_slope,
                                               reg_line_infinite_slope,
                                               reg_line_defined, a, b,
                                               cc_degs_of_rot,
                                               axis_scale_x, axis_scale_y,
                                               regression[i][j],
                                               linear_smth_pen);
			
			if (IsShowLowessSmoother() && xref.size() > 1) {
				size_t n = xref.size();
				wxString key = SmoothingUtils::LowessCacheKey(xt, yt);
				key << "_row" << i << "_col" << j;
			
                int t = var_info[IND_VAR].time;
               
                std::vector<bool> ref_undefs(xref.size(), false);
				SmoothingUtils::LowessCacheEntry* lce =
				SmoothingUtils::UpdateLowessCacheForTime(lowess_cache,
                                                         key, lowess,
                                                         xref, yref,
                                                         ref_undefs);
				
				if (!lce) {
				} else {
					// scaling factors to transform to screen coordinates.
					double scaleX = 100.0 / (axis_scale_x.scale_range);
					double scaleY = 100.0 / (axis_scale_y.scale_range);
                    reg_line_lowess[i][j].reInit(lce->X_srt, lce->YS_srt,
                                                 axis_scale_x.scale_min,
                                                 axis_scale_y.scale_min,
                                                 scaleX, scaleY);
					reg_line_lowess[i][j].setPen(lowess_smth_pen);					
				}
			}
			
			
		}
	}
	
	for (int i=0; i<vert_num_cats; i++) {
		for (int j=0; j<horiz_num_cats; j++) {
			delete dvec_xp[i][j];
			delete dvec_yp[i][j];
		}
	}
}


void ConditionalScatterPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	PopulateCanvas();
}

bool ConditionalScatterPlotCanvas::IsShowLinearSmoother()
{
	return show_linear_smoother;
}

bool ConditionalScatterPlotCanvas::IsShowLowessSmoother()
{
	return show_lowess_smoother;
}

void ConditionalScatterPlotCanvas::ShowLinearSmoother(bool display)
{
	show_linear_smoother = display;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConditionalScatterPlotCanvas::ShowLowessSmoother(bool display)
{
	show_lowess_smoother = display;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConditionalScatterPlotCanvas::ChangeLoessParams(double f, int iter, 
                                                     double delta_factor)
{
	EmptyLowessCache();
	lowess.SetF(f);
	lowess.SetIter(iter);
	lowess.SetDeltaFactor(delta_factor);
	if (IsShowLowessSmoother()) {
		invalidateBms();
		PopulateCanvas();
		Refresh();
	}		
}

void ConditionalScatterPlotCanvas::DisplayAxesScaleValues(bool display)
{
	if (display == display_axes_scale_values)
        return;
    
	display_axes_scale_values = display;
   
    int virtual_screen_marg_top = last_scale_trans.top_margin;
    int virtual_screen_marg_left = 60;
    int virtual_screen_marg_bottom = 60;
    int virtual_screen_marg_right = last_scale_trans.right_margin;
    
	if (display_axes_scale_values) {
        virtual_screen_marg_bottom = 75;
        virtual_screen_marg_left = 75;
	}
	
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConditionalScatterPlotCanvas::DisplaySlopeValues(bool display)
{
	if (display == display_slope_values) return;
	display_slope_values = display;
	
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

bool ConditionalScatterPlotCanvas::IsDisplayAxesScaleValues()
{
	return display_axes_scale_values;
}

bool ConditionalScatterPlotCanvas::IsDisplaySlopeValues()
{
	return display_slope_values;
}

void ConditionalScatterPlotCanvas::SetSelectableFillColor(wxColour color)
{
	selectable_fill_color = color;
	for (int t=0; t<cat_data.GetCanvasTmSteps(); t++) {
		cat_data.SetCategoryColor(t, 0, selectable_fill_color);
	}
	TemplateCanvas::SetSelectableFillColor(color);
}

void ConditionalScatterPlotCanvas::SetSelectableOutlineColor(wxColour color)
{
	selectable_outline_color = color;
	invalidateBms();
	PopulateCanvas();
	Refresh();
	TemplateCanvas::SetSelectableOutlineColor(color);
}

/** Free allocated points arrays in lowess_cache and clear cache */
void ConditionalScatterPlotCanvas::EmptyLowessCache()
{
	SmoothingUtils::EmptyLowessCache(lowess_cache);
}

void ConditionalScatterPlotCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
    
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    
    if (highlight_state->GetTotalHighlighted()> 0) {
        int n_total_hl = highlight_state->GetTotalHighlighted();
        s << "#selected=" << n_total_hl << "  ";
        
        int n_undefs = 0;
        for (int i=0; i<num_obs; i++) {
            if (XY_undef[i] && hl[i]) {
                n_undefs += 1;
            }
        }
        if (n_undefs> 0) {
            s << "(undefined:" << n_undefs << ") ";
        }
    }
    
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "hover obs " << hover_obs[0]+1 << " = (";
			s << data[IND_VAR][var_info[IND_VAR].time][hover_obs[0]] << ",";
			s << data[DEP_VAR][var_info[DEP_VAR].time][hover_obs[0]] << ")";
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = (";
			s << data[IND_VAR][var_info[IND_VAR].time][hover_obs[1]] << ",";
			s << data[DEP_VAR][var_info[DEP_VAR].time][hover_obs[1]] << ")";
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = (";
			s << data[IND_VAR][var_info[IND_VAR].time][hover_obs[2]] << ",";
			s << data[DEP_VAR][var_info[DEP_VAR].time][hover_obs[2]] << ")";
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	}
	sb->SetStatusText(s);
}


IMPLEMENT_CLASS(ConditionalScatterPlotFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalScatterPlotFrame, ConditionalNewFrame)
	EVT_ACTIVATE(ConditionalScatterPlotFrame::OnActivate)	
END_EVENT_TABLE()

ConditionalScatterPlotFrame::ConditionalScatterPlotFrame(wxFrame *parent,
									Project* project,
									const std::vector<GdaVarTools::VarInfo>& var_info,
									const std::vector<int>& col_ids,
									const wxString& title, const wxPoint& pos,
									const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,
					  size, style), lowess_param_frame(0)
{
    wxLogMessage("Open ConditionalScatterPlotFrame.");
	int width, height;
	GetClientSize(&width, &height);
	
	template_canvas = new ConditionalScatterPlotCanvas(this, this, project,
													   var_info, col_ids,
													   wxDefaultPosition,
													   wxSize(width,height));
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
		
	Show(true);
}

ConditionalScatterPlotFrame::~ConditionalScatterPlotFrame()
{
	if (lowess_param_frame) {
		lowess_param_frame->removeObserver(this);
		lowess_param_frame->closeAndDeleteWhenEmpty();
	}
	DeregisterAsActive();
}

void ConditionalScatterPlotFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("ConditionalScatterPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalScatterPlotFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_SCATTER_PLOT_VIEW_MENU_OPTIONS");
	((ConditionalScatterPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((ConditionalScatterPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConditionalScatterPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((ConditionalScatterPlotCanvas*)
		 template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConditionalScatterPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void  ConditionalScatterPlotFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
}

void ConditionalScatterPlotFrame::update(LowessParamObservable* o)
{
	ConditionalScatterPlotCanvas* c =
		(ConditionalScatterPlotCanvas*) template_canvas;
	c->ChangeLoessParams(o->GetF(), o->GetIter(), o->GetDeltaFactor());
}

void ConditionalScatterPlotFrame::notifyOfClosing(LowessParamObservable* o)
{
	lowess_param_frame = 0;
}

void ConditionalScatterPlotFrame::OnViewLinearSmoother(wxCommandEvent& event)
{
	ConditionalScatterPlotCanvas* c =
		(ConditionalScatterPlotCanvas*) template_canvas;
	c->ShowLinearSmoother(!c->IsShowLinearSmoother());
	UpdateOptionMenuItems();
}

void ConditionalScatterPlotFrame::OnViewLowessSmoother(wxCommandEvent& event)
{
	ConditionalScatterPlotCanvas* c =
		(ConditionalScatterPlotCanvas*) template_canvas;
	c->ShowLowessSmoother(!c->IsShowLowessSmoother());
	UpdateOptionMenuItems();
}

void ConditionalScatterPlotFrame::OnEditLowessParams(wxCommandEvent& event)
{
	ConditionalScatterPlotCanvas* c =
		(ConditionalScatterPlotCanvas*) template_canvas;
	if (lowess_param_frame) {
		lowess_param_frame->Iconize(false);
		lowess_param_frame->Raise();
		lowess_param_frame->SetFocus();
	} else {
		Lowess l = c->GetLowess();
        lowess_param_frame = new LowessParamFrame(l.GetF(), l.GetIter(),
                                                  l.GetDeltaFactor(),
                                                  project);
		lowess_param_frame->registerObserver(this);
	}
}

void ConditionalScatterPlotFrame::OnDisplayAxesScaleValues(wxCommandEvent& ev)
{
	ConditionalScatterPlotCanvas* c =
		(ConditionalScatterPlotCanvas*) template_canvas;
	c->DisplayAxesScaleValues(!c->IsDisplayAxesScaleValues());
	UpdateOptionMenuItems();
}

void ConditionalScatterPlotFrame::OnDisplaySlopeValues(wxCommandEvent& ev)
{
	ConditionalScatterPlotCanvas* c =
		(ConditionalScatterPlotCanvas*) template_canvas;
	c->DisplaySlopeValues(!c->IsDisplaySlopeValues());
	UpdateOptionMenuItems();
}
