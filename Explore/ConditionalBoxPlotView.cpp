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
#include "../GenGeomAlgs.h"
#include "../Project.h"

#include "ConditionalBoxPlotView.h"


IMPLEMENT_CLASS(ConditionalBoxPlotCanvas, ConditionalNewCanvas)
	BEGIN_EVENT_TABLE(ConditionalBoxPlotCanvas, ConditionalNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalBoxPlotCanvas::BOX_VAR = 2; // box var
const double ConditionalBoxPlotCanvas::left_pad_const = 20;

ConditionalBoxPlotCanvas::
ConditionalBoxPlotCanvas(wxWindow *parent,
                         TemplateFrame* t_frame,
                         Project* project_s,
                         const std::vector<GdaVarTools::VarInfo>& v_info,
                         const std::vector<int>& col_ids,
                         const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   false, true, pos, size)
{
    full_map_redraw_needed = true;
    hinge_15 = true;
    show_axes = true;
    // NOTE: define Box Plot defaults
    selectable_fill_color = GdaConst::boxplot_point_color;
    highlight_color = GdaConst::highlight_color;

    last_scale_trans.SetData(0, 0, 100, 100);
    last_scale_trans.SetMargin(25, 75, 75, 25);
    last_scale_trans.SetFixedAspectRatio(false); // stretch with screen

    VarInfoAttributeChange();
    InitBoxPlot();
    PopulateCanvas();

	all_init = true;
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

ConditionalBoxPlotCanvas::~ConditionalBoxPlotCanvas()
{
}

void ConditionalBoxPlotCanvas::InitBoxPlot()
{
    // Init box plots (data and stats) by rows and columns
    for (int t=0; t<num_time_vals; t++) {
        // get current time step
        int vt = var_info[VERT_VAR].time_min;
        int ht = var_info[HOR_VAR].time_min;
        int dt = var_info[BOX_VAR].time_min;
        if (var_info[BOX_VAR].sync_with_global_time) dt += t;
        if (var_info[VERT_VAR].sync_with_global_time) vt += t;
        if (var_info[HOR_VAR].sync_with_global_time) ht += t;

        // get number of rows and columns
        int rows = 1, cols = 1; // vert_num_cats
        if (!vert_cat_data.categories.empty()) {
            rows = vert_cat_data.categories[vt].cat_vec.size();
        }
        if (!horiz_cat_data.categories.empty()) {
            cols = horiz_cat_data.categories[ht].cat_vec.size();
        }

        // init cell data collection
        cell_data.clear();
        cell_data.resize(rows * cols); // order by row_idx*n + col_idx

        // get data points for each cell
        for (int i=0; i<num_obs; i++) {
            double val = data[BOX_VAR][dt][i];
            // which cell this value belongs to
            int row_idx = 0, col_idx = 0;
            if (!vert_cat_data.categories.empty()) {
                row_idx = vert_cat_data.categories[vt].id_to_cat[i];
            }
            if (!horiz_cat_data.categories.empty()) {
                col_idx = horiz_cat_data.categories[ht].id_to_cat[i];
            }
            cell_data[row_idx*cols + col_idx].push_back(std::make_pair(val,i));
        }
        // undef is full with size = num_obs
        std::vector<bool> undefs(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            undefs[i] = undefs[i] || data_undef[BOX_VAR][t][i];
        }
        // compute hinge stats for each cell
        hinge_stats.clear();
        hinge_stats.resize(rows * cols);
        data_stats.clear();
        data_stats.resize(rows * cols);
        data_valid.clear();
        data_valid.resize(rows * cols);
        data_sorted.clear();
        data_sorted.resize(rows * cols);
        for (int i=0; i<hinge_stats.size(); ++i) {
            data_valid[i] = cell_data[i];
            data_sorted[i] = cell_data[i];
            std::sort(data_sorted[i].begin(), data_sorted[i].end(), Gda::dbl_int_pair_cmp_less);
            std::sort(data_valid[i].begin(), data_valid[i].end(), Gda::dbl_int_pair_cmp_less);

            hinge_stats[i].CalculateHingeStats(data_sorted[i], undefs);
            data_stats[i].CalculateFromSample(data_valid[i], undefs);
        }
    }
}

void ConditionalBoxPlotCanvas::UserChangedCellCategories()
{
    InitBoxPlot();
}

void ConditionalBoxPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ConditionalBoxPlotFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_COND_BOXPLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu, project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void ConditionalBoxPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
    ConditionalNewCanvas::SetCheckMarks(menu);

    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"), show_axes);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_BOXPLOT_HINGE15"), hinge_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_BOXPLOT_HINGE30"), !hinge_15);
}

void ConditionalBoxPlotCanvas::OnShowAxes(wxCommandEvent& evt)
{
    show_axes = !show_axes;

    PopulateCanvas();
}

void ConditionalBoxPlotCanvas::OnHinge15(wxCommandEvent& evt)
{
    hinge_15 = true;

    PopulateCanvas();
}

void ConditionalBoxPlotCanvas::OnHinge30(wxCommandEvent& evt)
{
    hinge_15 = false;

    PopulateCanvas();
}

/** Override of TemplateCanvas method. */
void ConditionalBoxPlotCanvas::update(HLStateInt* o)
{
    ResetBrushing();

    layer1_valid = false;
    Refresh();
}

wxString ConditionalBoxPlotCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Cond. Box Plot - ";
	v << "x: " << GetNameWithTime(HOR_VAR);
	v << ", y: " << GetNameWithTime(VERT_VAR);
	v << ", Box plot: " << GetNameWithTime(BOX_VAR);
	return v;
}

wxString ConditionalBoxPlotCanvas::GetVariableNames()
{
    wxString v;
    v <<  GetNameWithTime(BOX_VAR);
    return v;
}

void ConditionalBoxPlotCanvas::ResizeSelectableShps(int virtual_scrn_w,
                                                    int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
    int vs_w = virtual_scrn_w;
    int vs_h = virtual_scrn_h;
    
    if (vs_w <= 0 && vs_h <= 0) {
        GetVirtualSize(&vs_w, &vs_h);
    }

	// last_scale_trans is only used in calls made to ApplyLastResizeToShp
	// which are made in ScaterNewPlotView
	GdaScaleTrans **st = new GdaScaleTrans*[vert_num_cats];
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new GdaScaleTrans[horiz_num_cats];
	}
		
	double scn_w = vs_w;
	double scn_h = vs_h;
	double min_pad = 10;

    if (show_axes) {
        min_pad += 38;
    }

	// pixels between columns/rows
	double fac = 0.01;
	double pad_w = scn_w * fac;
	double pad_h = scn_h * fac;
	if (pad_w < 1) pad_w = 0;
	if (pad_h < 1) pad_h = 0;
	double pad_bump = std::min(pad_w, pad_h);
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

    // calculate extent for each cell
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
            sub_st.SetFixedAspectRatio(false);
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

    // get ready for foreground shapes and background shapes
    BOOST_FOREACH( GdaShape* shp , foreground_shps ) { delete shp; }
    foreground_shps.clear();

    BOOST_FOREACH( GdaShape* shp , selectable_shps ) { delete shp; }
    selectable_shps.clear();
    selectable_shps.resize(num_obs);

    // place global verticle and horizontal axes
    bool is_vert_number = VERT_VAR_NUM && cat_classif_def_vert.cat_classif_type != CatClassification::unique_values;
    bool is_horz_number = HOR_VAR_NUM && cat_classif_def_horiz.cat_classif_type != CatClassification::unique_values;

	double bg_xmin = marg_left;
	double bg_xmax = scn_w - marg_right;
	double bg_ymin = marg_bottom;
	double bg_ymax = scn_h - marg_top;

    int n_rows = is_vert_number ? vert_num_cats-1 : vert_num_cats;
    int n_cols = is_horz_number ? horiz_num_cats-1 : horiz_num_cats;
    std::vector<wxRealPoint> v_brk_ref(n_rows);
    std::vector<wxRealPoint> h_brk_ref(n_cols);
	
	for (int row=0; row<n_rows; row++) {
        double bin_height = bin_extents[row][0].lower_left.y -bin_extents[row][0].upper_right.y;
        double y = 0;
        if (is_vert_number)
            y = (bin_extents[row][0].lower_left.y + bin_extents[row+1][0].upper_right.y)/2.0;
        else
            y = bin_extents[row][0].upper_right.y + bin_height / 2.0;
		v_brk_ref[row].x = bg_xmin;
		v_brk_ref[row].y = scn_h - y;
	}
	
	for (int col=0; col<n_cols; col++) {
        double bin_width = bin_extents[0][col].upper_right.x - bin_extents[0][col].lower_left.x;
        double x = 0;
        if (is_horz_number)
            x = (bin_extents[0][col].upper_right.x + bin_extents[0][col+1].lower_left.x)/2.0;
        else
            x = bin_extents[0][col].lower_left.x + bin_width / 2.0;
        h_brk_ref[col].x = x;
		h_brk_ref[col].y = bg_ymin;
	}
	
	int label_offset = 12;
	
    GdaShape* s;
	int vt = var_info[VERT_VAR].time;
	for (int row=0; row<n_rows; row++) {
        wxString tmp_lbl;
        if (is_vert_number) {
            double b;
            if (cat_classif_def_vert.cat_classif_type != CatClassification::custom) {
                if (!vert_cat_data.HasBreakVal(vt, row))
                    continue;
                b = vert_cat_data.GetBreakVal(vt, row);
            } else {
                b = cat_classif_def_vert.breaks[row];
            }
            tmp_lbl = GenUtils::DblToStr(b, display_precision, display_precision_fixed_point);
        } else {
            tmp_lbl << vert_cat_data.GetCategoryLabel(vt, row);
        }
		s = new GdaShapeText(tmp_lbl, *GdaConst::small_font, v_brk_ref[row], 90,
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
		vert_label << _(" vert cat var: ");
		vert_label << GetNameWithTime(VERT_VAR);
		vert_label << ",   ";
	}
	vert_label << _("Box plot var: ") << GetNameWithTime(BOX_VAR);
	s = new GdaShapeText(vert_label, *GdaConst::small_font,
				   wxRealPoint(bg_xmin, bg_ymin+(bg_ymax-bg_ymin)/2.0), 90,
				   GdaShapeText::h_center, GdaShapeText::bottom, -(label_offset+18), 0);
	foreground_shps.push_back(s);
	
	int ht = var_info[HOR_VAR].time;
	for (int col=0; col<n_cols; col++) {
        wxString tmp_lbl;
        if (is_horz_number) {
            double b;
            if (cat_classif_def_horiz.cat_classif_type!= CatClassification::custom){
                if (!horiz_cat_data.HasBreakVal(ht, col))
                    continue;
                b = horiz_cat_data.GetBreakVal(ht, col);
            } else {
                b = cat_classif_def_horiz.breaks[col];
            }
            tmp_lbl = GenUtils::DblToStr(b, display_precision, display_precision_fixed_point);
        } else {
            tmp_lbl << horiz_cat_data.GetCategoryLabel(ht, col);
        }
		s = new GdaShapeText(tmp_lbl, *GdaConst::small_font, h_brk_ref[col], 0,
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
		horiz_label << _(" horiz cat var: ");
		horiz_label << GetNameWithTime(HOR_VAR);
		horiz_label << ",   ";
	}
	horiz_label << _("Box plot var: ") << GetNameWithTime(BOX_VAR);
	s = new GdaShapeText(horiz_label, *GdaConst::small_font,
				   wxRealPoint(bg_xmin+(bg_xmax-bg_xmin)/2.0, bg_ymin), 0,
				   GdaShapeText::h_center, GdaShapeText::top, 0, (label_offset+18));
	foreground_shps.push_back(s);

    // format background shapes: labels etc.
    GdaScaleTrans background_st;
    background_st.SetData(marg_left, marg_bottom, scn_w-marg_right, scn_h-marg_top);
    background_st.SetMargin(marg_top, marg_bottom, marg_left, marg_right);
    background_st.SetView(vs_w, vs_h);

    BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
        ms->applyScaleTrans(background_st);
    }

    // draw box plots in each cell
    int t = var_info[BOX_VAR].time;

    // need to scale height data so that y_min and y_max are between 0 and 100
    double y_min = hinge_stats[0].min_val;
    double y_max = hinge_stats[0].max_val;
    for (int r=0; r<vert_num_cats; r++) {
        for (int c=0; c<horiz_num_cats; c++) {
            int h_idx = r*horiz_num_cats + c;
            double ext_upper = hinge_stats[h_idx].max_val;
            double ext_lower = hinge_stats[h_idx].min_val;
            if (ext_upper > y_max) y_max = ext_upper;
            if (ext_lower < y_min) y_min = ext_lower;
            ext_upper = (hinge_15 ? hinge_stats[h_idx].extreme_upper_val_15 : hinge_stats[h_idx].extreme_upper_val_30);
            ext_lower = (hinge_15 ? hinge_stats[h_idx].extreme_lower_val_15 : hinge_stats[h_idx].extreme_lower_val_30);
            if (ext_upper > y_max) y_max = ext_upper;
            if (ext_lower < y_min) y_min = ext_lower;
        }
    }
    double scaleY = 100.0 / (y_max-y_min);
    axis_scale_y = AxisScale(y_min, y_max, 3, axis_display_precision, axis_display_fixed_point);

    // set box plot in each cell takes half the width
    double box_width = del_width / 2.0;
    int i=0;
    for (int r=0; r<vert_num_cats; r++) {
        for (int c=0; c<horiz_num_cats; c++) {
            st[r][c].SetData(0, 0, del_width, 100);
            // create y axis
            if (show_axes) {
                GdaAxis* y_ax = new GdaAxis("", axis_scale_y, wxRealPoint(0,0), wxRealPoint(0, 100), 20, 0);
                y_ax->applyScaleTrans(st[r][c]);
                foreground_shps.push_back(y_ax);
            }

            // create boxplot in a cell
            int h_idx = r*horiz_num_cats + c;
            double xM = left_pad_const + box_width; // center of cell
            double x0r = xM - box_width/2.2;
            double x1r = xM + box_width/2.2;
            double x0 = xM - box_width/2.0;
            double y0 = (hinge_15 ? hinge_stats[h_idx].extreme_lower_val_15 : hinge_stats[h_idx].extreme_lower_val_30);
            double x1 = xM + box_width/2.0;
            double y1 = (hinge_15 ? hinge_stats[h_idx].extreme_upper_val_15 : hinge_stats[h_idx].extreme_upper_val_30);

            s = new GdaPolyLine(xM-box_width/3.0, (y0-y_min)*scaleY,
                                xM+box_width/3.0, (y0-y_min)*scaleY);
            s->applyScaleTrans(st[r][c]);
            foreground_shps.push_back(s);
            s = new GdaPolyLine(xM-box_width/3.0, (y1-y_min)*scaleY,
                                xM+box_width/3.0, (y1-y_min)*scaleY);
            s->applyScaleTrans(st[r][c]);
            foreground_shps.push_back(s);
            s = new GdaPolyLine(xM, (y0-y_min)*scaleY,
                                xM, (y1-y_min)*scaleY);
            s->applyScaleTrans(st[r][c]);
            foreground_shps.push_back(s);
            // mean circle
            s = new GdaCircle(wxRealPoint(xM, (data_stats[h_idx].mean-y_min)*scaleY), 5.0);
            s->setPen(selectable_fill_color);
            s->setBrush(GdaConst::boxplot_mean_point_color);
            s->applyScaleTrans(st[r][c]);
            foreground_shps.push_back(s);

            double y0m = (hinge_stats[h_idx].Q2-y_min)*scaleY - 0.5;
            double y1m = (hinge_stats[h_idx].Q2-y_min)*scaleY + 0.5;
            s = new GdaRectangle(wxRealPoint(x0, y0m), wxRealPoint(x1, y1m));
            s->setPen(GdaConst::boxplot_median_color);
            s->setBrush(GdaConst::boxplot_median_color);
            s->applyScaleTrans(st[r][c]);
            foreground_shps.push_back(s);

            // draw details in IQRs
            for (int i=0; i<data_sorted[h_idx].size(); i++) {
                double val = data_sorted[h_idx][i].first;
                int orig_idx = data_sorted[h_idx][i].second;
                if (val < hinge_stats[h_idx].Q1 || val > hinge_stats[h_idx].Q3) {
                    s = new GdaPoint(xM, (val-y_min) * scaleY);
                    s->setPen(selectable_fill_color);
                    s->setBrush(*wxWHITE_BRUSH);
                    s->applyScaleTrans(st[r][c]);
                    selectable_shps[orig_idx] = s;
                } else {
                    y0 = (((data_sorted[h_idx][i].first + data_sorted[h_idx][i-1].first)/2.0) - y_min)*scaleY;
                    y1 = (((data_sorted[h_idx][i].first + data_sorted[h_idx][i+1].first)/2.0) - y_min)*scaleY;
                    s= new GdaRectangle(wxRealPoint(x0r, y0), wxRealPoint(x1r, y1));
                    s->setPen(GdaConst::boxplot_q1q2q3_color);
                    s->setBrush(GdaConst::boxplot_q1q2q3_color);
                    s->applyScaleTrans(st[r][c]);
                    selectable_shps[orig_idx] = s;
                }
            }
        }
    }

	layer0_valid = false;
	Refresh();
	
	for (int i=0; i<vert_num_cats; i++) delete [] st[i];
	delete [] st;
}

void ConditionalBoxPlotCanvas::PopulateCanvas()
{
	ResizeSelectableShps();
    isResize = true;
}

void ConditionalBoxPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	PopulateCanvas();
}

void ConditionalBoxPlotCanvas::SetSelectableFillColor(wxColour color)
{
	selectable_fill_color = color;
	for (int t=0; t<cat_data.GetCanvasTmSteps(); t++) {
		//cat_data.SetCategoryColor(t, 0, selectable_fill_color);
        cat_data.SetCategoryPenColor(t, 0, selectable_fill_color);
        cat_data.SetCategoryBrushColor(t, 0, *wxWHITE);
	}
	TemplateCanvas::SetSelectableFillColor(color);
}

void ConditionalBoxPlotCanvas::SetSelectableOutlineColor(wxColour color)
{
	selectable_outline_color = color;
	invalidateBms();
	PopulateCanvas();
	Refresh();
	TemplateCanvas::SetSelectableOutlineColor(color);
}

void ConditionalBoxPlotCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
    
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    
    if (highlight_state->GetTotalHighlighted()> 0) {
        int n_total_hl = highlight_state->GetTotalHighlighted();
        s << _("#selected=") << n_total_hl << "  ";
    }
    
	if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << _("#hover obs ") << hover_obs[0]+1 << " = ";
            s << data[BOX_VAR][var_info[BOX_VAR].time][hover_obs[0]];
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << _("obs ") << hover_obs[1]+1 << " = ";
            s << data[BOX_VAR][var_info[BOX_VAR].time][hover_obs[1]];
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << _("obs ") << hover_obs[2]+1 << " = ";
            s << data[BOX_VAR][var_info[BOX_VAR].time][hover_obs[2]];
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
	}
	sb->SetStatusText(s);
}


IMPLEMENT_CLASS(ConditionalBoxPlotFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalBoxPlotFrame, ConditionalNewFrame)
	EVT_ACTIVATE(ConditionalBoxPlotFrame::OnActivate)
END_EVENT_TABLE()

ConditionalBoxPlotFrame::ConditionalBoxPlotFrame(wxFrame *parent,
                                                 Project* project,
                                                 const std::vector<GdaVarTools::VarInfo>& var_info,
                                                 const std::vector<int>& col_ids,
                                                 const wxString& title, const wxPoint& pos,
                                                 const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,
					  size, style)
{
    wxLogMessage("Open ConditionalBoxPlotFrame.");
	int width, height;
	GetClientSize(&width, &height);
	
    template_canvas = new ConditionalBoxPlotCanvas(this, this, project,
                                                   var_info, col_ids,
                                                   wxDefaultPosition,
                                                   wxSize(width,height));
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
		
	Show(true);
}

ConditionalBoxPlotFrame::~ConditionalBoxPlotFrame()
{
	DeregisterAsActive();
}

void ConditionalBoxPlotFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("ConditionalBoxPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalBoxPlotFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_COND_BOXPLOT_VIEW_MENU_OPTIONS");
	((ConditionalBoxPlotCanvas*)template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu, project->GetCatClassifManager());
	((ConditionalBoxPlotCanvas*)template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);	
	UpdateOptionMenuItems();

    // connect menu event handler
    wxMenuItem* axes_menu = optMenu->FindItem(XRCID("ID_COND_BOXPLOT_SHOW_AXES"));
    Connect(axes_menu->GetId(), wxEVT_MENU, wxCommandEventHandler(ConditionalBoxPlotFrame::OnShowAxes));

    wxMenuItem* hinge15_menu = optMenu->FindItem(XRCID("ID_BOXPLOT_HINGE15"));
    Connect(hinge15_menu->GetId(), wxEVT_MENU, wxCommandEventHandler(ConditionalBoxPlotFrame::OnHinge15));

    wxMenuItem* hinge30_menu = optMenu->FindItem(XRCID("ID_BOXPLOT_HINGE30"));
    Connect(hinge30_menu->GetId(), wxEVT_MENU, wxCommandEventHandler(ConditionalBoxPlotFrame::OnHinge30));
}

void ConditionalBoxPlotFrame::OnShowAxes(wxCommandEvent& evt)
{
    ((ConditionalBoxPlotCanvas*)template_canvas)->OnShowAxes(evt);
}

void ConditionalBoxPlotFrame::OnHinge15(wxCommandEvent& evt)
{
    ((ConditionalBoxPlotCanvas*)template_canvas)->OnHinge15(evt);
}

void ConditionalBoxPlotFrame::OnHinge30(wxCommandEvent& evt)
{
    ((ConditionalBoxPlotCanvas*)template_canvas)->OnHinge30(evt);
}

void ConditionalBoxPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu(_("Options"));
    if (menu == wxNOT_FOUND) {
	} else {
		((ConditionalBoxPlotCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConditionalBoxPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void  ConditionalBoxPlotFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
}

void ConditionalBoxPlotFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
    if (!template_canvas) return;
    wxString title = project->GetProjectTitle();
    GeneralWxUtils::SaveWindowAsImage(template_canvas, title);
}

