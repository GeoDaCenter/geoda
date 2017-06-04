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
#include "../DialogTools/HistIntervalDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenGeomAlgs.h"
#include "../FramesManager.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ConditionalHistogramView.h"


IMPLEMENT_CLASS(ConditionalHistogramCanvas, ConditionalNewCanvas)
BEGIN_EVENT_TABLE(ConditionalHistogramCanvas, ConditionalNewCanvas)
EVT_PAINT(TemplateCanvas::OnPaint)
EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalHistogramCanvas::HIST_VAR = 2; // histogram var
const int ConditionalHistogramCanvas::default_intervals = 7;
const int ConditionalHistogramCanvas::MAX_INTERVALS = 200;
const double ConditionalHistogramCanvas::left_pad_const = 0;
const double ConditionalHistogramCanvas::right_pad_const = 0;
const double ConditionalHistogramCanvas::interval_width_const = 10;
const double ConditionalHistogramCanvas::interval_gap_const = 0;

ConditionalHistogramCanvas::
ConditionalHistogramCanvas(wxWindow *parent,
                           TemplateFrame* t_frame,
                           Project* project_s,
                           const std::vector<GdaVarTools::VarInfo>& v_info,
                           const std::vector<int>& col_ids,
                           const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   false, true, pos, size),
full_map_redraw_needed(true),
show_axes(true), scale_x_over_time(true), scale_y_over_time(true)
{
	int hist_var_tms = data[HIST_VAR].shape()[0];
    
	data_stats.resize(hist_var_tms);
	data_sorted.resize(hist_var_tms);
    
    // create bins for histogram
	for (int t=0; t<hist_var_tms; t++) {
		data_sorted[t].resize(num_obs);
        
        std::vector<bool> undefs(num_obs, false);
        
		for (int i=0; i<num_obs; i++) {
			data_sorted[t][i].first = data[HIST_VAR][t][i];
			data_sorted[t][i].second = i;
            undefs[i] = data_undef[HIST_VAR][t][i];
		}
		std::sort(data_sorted[t].begin(), data_sorted[t].end(),
				  Gda::dbl_int_pair_cmp_less);
		data_stats[t].CalculateFromSample(data_sorted[t], undefs);
        
		data_min_over_time = data_stats[t].min;
		data_max_over_time = data_stats[t].max;
        undef_tms.push_back(undefs);
	}
	
    if ( undef_tms.size() < num_time_vals) {
        // case that histogram is non time variable, but horizontal / vertical may be time variable
        for (int i=1; i<num_time_vals; i++) {
            std::vector<bool> undefs(num_obs, false);
            for (int j=0; j<num_obs; j++) undefs[j] = undef_tms[0][j];
            undef_tms.push_back(undefs);
        }
    }
    
	max_intervals = GenUtils::min<int>(MAX_INTERVALS, num_obs);
	cur_intervals = GenUtils::min<int>(max_intervals, default_intervals);
	if (num_obs > 49) {
		int c = sqrt((double) num_obs);
		cur_intervals = GenUtils::min<int>(max_intervals, c);
		cur_intervals = GenUtils::min<int>(cur_intervals, 7);
	}
	
	highlight_color = GdaConst::highlight_color;
    last_scale_trans.SetFixedAspectRatio(false);
	use_category_brushes = false;
	selectable_shps_type = rectangles;
	
	VarInfoAttributeChange();
	InitIntervals();
	PopulateCanvas();
	
	all_init = true;
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

ConditionalHistogramCanvas::~ConditionalHistogramCanvas()
{
}

void ConditionalHistogramCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ConditionalHistogramFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_COND_HISTOGRAM_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void ConditionalHistogramCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	ConditionalNewCanvas::SetCheckMarks(menu);
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
}

/** Override of TemplateCanvas method. */
void ConditionalHistogramCanvas::update(HLStateInt* o)
{
    ResetBrushing();
    
	layer1_valid = false;
	UpdateIvalSelCnts();
	Refresh();
}

wxString ConditionalHistogramCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Cond. Histogram - ";
	v << "x: " << GetNameWithTime(HOR_VAR);
	v << ", y: " << GetNameWithTime(VERT_VAR);
	v << ", histogram: " << GetNameWithTime(HIST_VAR);
	return v;
}

void ConditionalHistogramCanvas::ResizeSelectableShps(int virtual_scrn_w,
                                                      int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
	int vs_w=virtual_scrn_w, vs_h=virtual_scrn_h;
	if (vs_w <= 0 && vs_h <= 0) GetVirtualSize(&vs_w, &vs_h);
	
	//double image_width, image_height;
	bool ftwm = GetFitToWindowMode();
	
	// last_scale_trans is only used in calls made to ApplyLastResizeToShp
	// which are made in ScaterNewPlotView
	GdaScaleTrans **st;
	st = new GdaScaleTrans*[vert_num_cats];
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new GdaScaleTrans[horiz_num_cats];
	}
	
	// Total width height:  vs_w   vs_h
	// Working area margins: virtual_screen_marg_top,
	//  virtual_screen_marg_bottom,
	//  virtual_screen_marg_left,
	//  virtual_screen_marg_right
	// We need to increase these as needed for each tile area
	
	double scn_w = vs_w;
	double scn_h = vs_h;
	
	double min_pad = 10;
	if (show_axes) {
		min_pad += 38;
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
            sub_st.SetFixedAspectRatio(false);
            sub_st.SetData(shps_orig_xmin, shps_orig_ymin,
                           shps_orig_xmax, shps_orig_ymax);
            sub_st.SetMargin(mt, mb, ml, mr);
            sub_st.SetView(vs_w, vs_h);
            
			wxRealPoint ll(shps_orig_xmin, shps_orig_ymin);
			wxRealPoint ur(shps_orig_xmax, shps_orig_ymax);
			bin_extents[(vert_num_cats-1)-row][col] = GdaRectangle(ll, ur);
			bin_extents[(vert_num_cats-1)-row][col].applyScaleTrans(sub_st);
		}
	}
	
	int i=0;
	for (int r=0; r<vert_num_cats; r++) {
		for (int c=0; c<horiz_num_cats; c++) {
			for (int ival=0; ival<cur_intervals; ival++) {
				selectable_shps[i]->applyScaleTrans(st[r][c]);
				i++;
			}
		}
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	GdaShape* s;
	if (show_axes) {
		for (int r=0; r<vert_num_cats; r++) {
			for (int c=0; c<horiz_num_cats; c++) {
				s = new GdaAxis(*x_axis);
				s->applyScaleTrans(st[r][c]);
				foreground_shps.push_front(s);
				s = new GdaAxis(*y_axis);
				s->applyScaleTrans(st[r][c]);
				foreground_shps.push_front(s);
			}
		}
	}
	
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
	
	int bg_shp_cnt = 0;
	int label_offset = 25;
	if (show_axes) label_offset += 25;
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
                             GdaShapeText::h_center, GdaShapeText::bottom,
                             -label_offset, 0);
		foreground_shps.push_front(s);
		bg_shp_cnt++;
	}
	if (GetCatType(VERT_VAR) != CatClassification::no_theme) {
		wxString vert_label;
		if (GetCatType(VERT_VAR) == CatClassification::custom) {
			vert_label << cat_classif_def_vert.title;
		} else {
			vert_label << CatClassification::CatClassifTypeToString(
														GetCatType(VERT_VAR));
		}
		vert_label << " vert cat var: ";
		vert_label << GetNameWithTime(VERT_VAR);
		s = new GdaShapeText(vert_label, *GdaConst::small_font,
					   wxRealPoint(bg_xmin, bg_ymin+(bg_ymax-bg_ymin)/2.0), 90,
					   GdaShapeText::h_center, GdaShapeText::bottom, -(label_offset+15), 0);
		foreground_shps.push_front(s);
		bg_shp_cnt++;
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
					   GdaShapeText::h_center, GdaShapeText::top, 0, label_offset);
		foreground_shps.push_front(s);
		bg_shp_cnt++;
	}
	if (GetCatType(HOR_VAR) != CatClassification::no_theme) {
		wxString horiz_label;
		if (GetCatType(HOR_VAR) == CatClassification::custom) {
			horiz_label << cat_classif_def_horiz.title;
		} else {
			horiz_label << CatClassification::CatClassifTypeToString(
														GetCatType(HOR_VAR));
		}
		horiz_label << " horiz cat var: ";
		horiz_label << GetNameWithTime(HOR_VAR);
		s = new GdaShapeText(horiz_label, *GdaConst::small_font,
					   wxRealPoint(bg_xmin+(bg_xmax-bg_xmin)/2.0, bg_ymin), 0,
					   GdaShapeText::h_center, GdaShapeText::top, 0, (label_offset+15));
		foreground_shps.push_front(s);
		bg_shp_cnt++;
	}
   
    GdaScaleTrans background_st;
    background_st.SetData(marg_left, marg_bottom, scn_w-marg_right,
                          scn_h-marg_top);
    background_st.SetMargin(marg_top, marg_bottom, marg_left, marg_right);
    background_st.SetView(vs_w, vs_h);
    
    
    
	int bg_shp_i = 0;
	for (std::list<GdaShape*>::iterator it=foreground_shps.begin();
		 bg_shp_i < bg_shp_cnt && it != foreground_shps.end();
		 bg_shp_i++, it++) {
		(*it)->applyScaleTrans(background_st);
	}
	
	layer0_valid = false;
	Refresh();
	
	for (int i=0; i<vert_num_cats; i++) delete [] st[i];
	delete [] st;	
}

void ConditionalHistogramCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();

	int t = var_info[HIST_VAR].time;
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const
		+ interval_width_const * cur_intervals + 
		+ interval_gap_const * (cur_intervals-1);
	
	// orig_x_pos is the center of each histogram bar
	std::vector<double> orig_x_pos(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		orig_x_pos[i] = left_pad_const + interval_width_const/2.0
			+ i * (interval_width_const + interval_gap_const);
	}
	
	double y_max = (scale_y_over_time ? overall_max_num_obs_in_ival :
					  max_num_obs_in_ival[t]);
    last_scale_trans.SetData(x_min, 0, x_max, y_max);
    
	if (show_axes) {
		axis_scale_y = AxisScale(0, y_max, 3, axis_display_precision);
		y_max = axis_scale_y.scale_max;
		y_axis = new GdaAxis("Frequency", axis_scale_y,
							wxRealPoint(0,0), wxRealPoint(0, y_max),
							-9, 0);
		
		axis_scale_x = AxisScale(0, max_ival_val[t], 5, axis_display_precision);
		//shps_orig_xmax = axis_scale_x.scale_max;
		axis_scale_x.data_min = min_ival_val[t];
		axis_scale_x.data_max = max_ival_val[t];
		axis_scale_x.scale_min = axis_scale_x.data_min;
		axis_scale_x.scale_max = axis_scale_x.data_max;
		double range = axis_scale_x.scale_max - axis_scale_x.scale_min;
		axis_scale_x.scale_range = range;
		axis_scale_x.p = floor(log10(range));
		axis_scale_x.ticks = cur_intervals+1;
		axis_scale_x.tics.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str_show.resize(axis_scale_x.tics_str.size());
		for (int i=0; i<axis_scale_x.ticks; i++) {
			axis_scale_x.tics[i] =
			axis_scale_x.data_min +
			range*((double) i)/((double) axis_scale_x.ticks-1);
			wxString flt = wxString::Format("%.2g", axis_scale_x.tics[i]);
			axis_scale_x.tics_str[i] = flt.ToStdString();
			axis_scale_x.tics_str_show[i] = false;
		}
		int tick_freq = ceil(((double) cur_intervals)/5.0);
		for (int i=0; i<axis_scale_x.ticks; i++) {
			if (i % tick_freq == 0) {
				axis_scale_x.tics_str_show[i] = true;
			}
		}
		axis_scale_x.tic_inc = axis_scale_x.tics[1]-axis_scale_x.tics[0];
		x_axis = new GdaAxis(GetNameWithTime(2), axis_scale_x, wxRealPoint(0,0),
							wxRealPoint(x_max, 0), 0, 9);
	}
	
	selectable_shps.resize(vert_num_cats * horiz_num_cats * cur_intervals);
	int i=0;
	for (int r=0; r<vert_num_cats; r++) {
		for (int c=0; c<horiz_num_cats; c++) {
			for (int ival=0; ival<cur_intervals; ival++) {
				double x0 = orig_x_pos[ival] - interval_width_const/2.0;
				double x1 = orig_x_pos[ival] + interval_width_const/2.0;
				double y0 = 0;
				double y1 = cell_data[t][r][c].ival_obs_cnt[ival];
				selectable_shps[i] = new GdaRectangle(wxRealPoint(x0, 0),
													 wxRealPoint(x1, y1));
				int sz = GdaConst::qualitative_colors.size();
				selectable_shps[i]->
					setPen(GdaConst::qualitative_colors[ival%sz]);
				selectable_shps[i]->
					setBrush(GdaConst::qualitative_colors[ival%sz]);
				i++;
			}
		}
	}
	
	int virtual_screen_marg_top = 25;
	int virtual_screen_marg_bottom = 70;
	int virtual_screen_marg_left = 70;
	int virtual_screen_marg_right = 25;
	
	if (show_axes) {
		virtual_screen_marg_bottom += 25;
		virtual_screen_marg_left += 25;
	}
    last_scale_trans.top_margin = virtual_screen_marg_top;
    last_scale_trans.bottom_margin = virtual_screen_marg_bottom;
    last_scale_trans.left_margin = virtual_screen_marg_left;
    last_scale_trans.right_margin = virtual_screen_marg_right;
	
	ResizeSelectableShps();
}


void ConditionalHistogramCanvas::ShowAxes(bool show_axes_s)
{
	if (show_axes_s == show_axes) return;
	show_axes = show_axes_s;
	
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConditionalHistogramCanvas::DetermineMouseHoverObjects(wxPoint pt)
{
	total_hover_obs = 0;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (selectable_shps[i]->pointWithin(pt)) {
			hover_obs[total_hover_obs++] = i;
			if (total_hover_obs == max_hover_obs) break;
		}
	}
}

// The following function assumes that the set of selectable objects
// are all rectangles.
void ConditionalHistogramCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	bool rect_sel = (!pointsel && (brushtype == rectangle));
	
	int t = var_info[HIST_VAR].time;
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	int total_sel_shps = selectable_shps.size();
	
	wxPoint lower_left;
	wxPoint upper_right;
	if (rect_sel) {
		GenGeomAlgs::StandardizeRect(sel1, sel2, lower_left, upper_right);
	}
	if (!shiftdown) {
		bool any_selected = false;
		for (int i=0; i<total_sel_shps; i++) {
			GdaRectangle* rec = (GdaRectangle*) selectable_shps[i];
			if ((pointsel && rec->pointWithin(sel1)) ||
				(rect_sel &&
				 GenGeomAlgs::RectsIntersect(rec->lower_left, rec->upper_right,
                                             lower_left, upper_right)))
			{
				any_selected = true;
				break;
			}
		}
		if (!any_selected) {
			highlight_state->SetEventType(HLStateInt::unhighlight_all);
			highlight_state->notifyObservers(this);
            selection_changed = true;
		}
	}

    if (selection_changed ==  false) {
    	for (int i=0; i<total_sel_shps; i++) {
    		int r, c, ival;
    		sel_shp_to_cell(i, r, c, ival);
    		GdaRectangle* rec = (GdaRectangle*) selectable_shps[i];
    		bool selected = ((pointsel && rec->pointWithin(sel1)) ||
    						 (rect_sel &&
    						  GenGeomAlgs::RectsIntersect(rec->lower_left,
    												   rec->upper_right,
    												   lower_left, upper_right)));
    		bool all_sel = (cell_data[t][r][c].ival_obs_cnt[ival] == 
    						cell_data[t][r][c].ival_obs_sel_cnt[ival]);
    		if (pointsel && all_sel && selected) {
    			// unselect all in ival
    			for (std::list<int>::iterator it =
    					cell_data[t][r][c].ival_to_obs_ids[ival].begin();
    				 it != cell_data[t][r][c].ival_to_obs_ids[ival].end(); it++) {
                    hs[(*it)]= false;
                    selection_changed = true;
    			}
    		} else if (!all_sel && selected) {
    			// select currently unselected in ival
    			for (std::list<int>::iterator it =
    					cell_data[t][r][c].ival_to_obs_ids[ival].begin();
    				 it != cell_data[t][r][c].ival_to_obs_ids[ival].end(); it++) {
    				if (hs[*it]) continue;
                    hs[(*it)]= true;
                    selection_changed = true;
    			}
    		} else if (!selected && !shiftdown) {
    			// unselect all selected in ival
    			for (std::list<int>::iterator it =
    					cell_data[t][r][c].ival_to_obs_ids[ival].begin();
    				 it != cell_data[t][r][c].ival_to_obs_ids[ival].end(); it++) {
    				if (!hs[*it]) continue;
                    hs[(*it)]= false;
                    selection_changed = true;
    			}
    		}
    	}
    	if ( selection_changed ) {
    		highlight_state->SetEventType(HLStateInt::delta);
    		highlight_state->notifyObservers(this);
        }
    }
	
	if ( selection_changed ) {
        // re-paint highlight layer (layer1_bm)
        layer1_valid = false;
        UpdateIvalSelCnts();
        DrawLayers();
        Refresh();
	}
    
    
    
	UpdateStatusBar();
}

void ConditionalHistogramCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	int t = var_info[HIST_VAR].time;
	int i=0;
	for (int r=0; r<vert_num_cats; r++) {
		for (int c=0; c<horiz_num_cats; c++) {
			for (int ival=0; ival<cur_intervals; ival++) {
				if (cell_data[t][r][c].ival_obs_cnt[ival] != 0) {
					selectable_shps[i]->paintSelf(dc);
				}
				i++;
			}
		}
	}
}

void ConditionalHistogramCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	int t = var_info[HIST_VAR].time;
	int i=0;
	double s;
	for (int r=0; r<vert_num_cats; r++) {
		for (int c=0; c<horiz_num_cats; c++) {
			for (int ival=0; ival<cur_intervals; ival++) {
				if (cell_data[t][r][c].ival_obs_sel_cnt[ival] != 0) {
					s = (((double) cell_data[t][r][c].ival_obs_sel_cnt[ival]) /
						 ((double) cell_data[t][r][c].ival_obs_cnt[ival]));
                    
                    GdaShape* shp = selectable_shps[i];
                    dc.SetPen(shp->getPen());
                    dc.SetBrush(shp->getBrush());
                    GdaRectangle* rec = (GdaRectangle*) shp;
                    dc.DrawRectangle(rec->lower_left.x,
                                     rec->lower_left.y,
                                     rec->upper_right.x - rec->lower_left.x,
                                     (rec->upper_right.y - rec->lower_left.y)*s);
                    
				}
				i++;
			}
		}
	}
}

void ConditionalHistogramCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	InitIntervals();
	PopulateCanvas();
}

void ConditionalHistogramCanvas::HistogramIntervals()
{
	HistIntervalDlg dlg(1, cur_intervals, max_intervals, this);
	if (dlg.ShowModal () != wxID_OK) return;
	if (cur_intervals == dlg.num_intervals) return;
	cur_intervals = dlg.num_intervals;
	InitIntervals();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

/** based on horiz_cat_data, vert_cat_data, num_time_vals,
 data_min_over_time, data_max_over_time,
 cur_intervals, scale_x_over_time:
 calculate interval breaks and populate
 obs_id_to_sel_shp, ival_obs_cnt and ival_obs_sel_cnt */ 
void ConditionalHistogramCanvas::InitIntervals()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
		
	// determine correct ivals for each obs in current time period
	min_ival_val.resize(num_time_vals);
	max_ival_val.resize(num_time_vals);
	max_num_obs_in_ival.resize(num_time_vals);
	
	overall_max_num_obs_in_ival = 0;
	for (int t=0; t<num_time_vals; t++)
        max_num_obs_in_ival[t] = 0;
	
	ival_breaks.resize(boost::extents[num_time_vals][cur_intervals-1]);
	for (int t=0; t<num_time_vals; t++) {
        std::vector<bool> undefs = undef_tms[t];
		if (scale_x_over_time) {
			min_ival_val[t] = data_min_over_time;
			max_ival_val[t] = data_max_over_time;
		} else {
			min_ival_val[t] = data_sorted[t][0].first;
			max_ival_val[t] = data_sorted[t][num_obs-1].first;
		}
		if (min_ival_val[t] == max_ival_val[t]) {
			if (min_ival_val[t] == 0) {
				max_ival_val[t] = 1;
			} else {
				max_ival_val[t] += fabs(max_ival_val[t])/2.0;
			}
		}
		double range = max_ival_val[t] - min_ival_val[t];
		double ival_size = range/((double) cur_intervals);
		
		for (int i=0; i<cur_intervals-1; i++) {
			ival_breaks[t][i] = min_ival_val[t]+ival_size*((double) (i+1));
		}
	}
	
	obs_id_to_sel_shp.resize(boost::extents[num_time_vals][num_obs]);
	cell_data.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) {
		int vt = var_info[VERT_VAR].time_min;
		int ht = var_info[HOR_VAR].time_min;
        
		if (var_info[VERT_VAR].sync_with_global_time)
            vt += t;
        
		if (var_info[HOR_VAR].sync_with_global_time)
            ht += t;
        
		int rows = vert_cat_data.categories[vt].cat_vec.size();
		int cols = horiz_cat_data.categories[ht].cat_vec.size();
        
		cell_data[t].resize(boost::extents[rows][cols]);
		for (int r=0; r<rows; r++) {
			for (int c=0; c<cols; c++) {
				cell_data[t][r][c].ival_obs_cnt.resize(cur_intervals);
				cell_data[t][r][c].ival_obs_sel_cnt.resize(cur_intervals);
				cell_data[t][r][c].ival_to_obs_ids.resize(cur_intervals);
				for (int i=0; i<cur_intervals; i++) {
					cell_data[t][r][c].ival_obs_cnt[i] = 0;
					cell_data[t][r][c].ival_obs_sel_cnt[i] = 0;
					cell_data[t][r][c].ival_to_obs_ids[i].clear();
				}
			}
		}
		
		int dt = var_info[HIST_VAR].time_min;
		if (var_info[HIST_VAR].sync_with_global_time)
            dt += t;
        
		// record each obs in the correct cell and ival.
        int cur_ival = 0;
		for (int i=0; i<num_obs; i++)
        {
			int id = data_sorted[dt][i].second;
            
            if (undef_tms[t][id])
                continue;
            
			while (cur_ival <= cur_intervals-2 &&
				   data_sorted[dt][i].first >= ival_breaks[t][cur_ival])
            {
				cur_ival++;
			}
			int r = vert_cat_data.categories[vt].id_to_cat[id];
			int c = horiz_cat_data.categories[ht].id_to_cat[id];
            
			obs_id_to_sel_shp[t][id] = cell_to_sel_shp_gen(r, c, cur_ival,
														   cols, cur_intervals);
            
			cell_data[t][r][c].ival_to_obs_ids[cur_ival].push_front(id);
            
			cell_data[t][r][c].ival_obs_cnt[cur_ival]++;
            
			if (cell_data[t][r][c].ival_obs_cnt[cur_ival] >
				max_num_obs_in_ival[t])
            {
				max_num_obs_in_ival[t] =
					cell_data[t][r][c].ival_obs_cnt[cur_ival];
				if (max_num_obs_in_ival[t] > overall_max_num_obs_in_ival) {
					overall_max_num_obs_in_ival = max_num_obs_in_ival[t];
				}
			}
			
			if (hs[data_sorted[dt][i].second]) {
				cell_data[t][r][c].ival_obs_sel_cnt[cur_ival]++;
			}
		}
	}
	
}

void ConditionalHistogramCanvas::UpdateIvalSelCnts()
{
	HLStateInt::EventType type = highlight_state->GetEventType();
	if (type == HLStateInt::unhighlight_all) {
		for (int t=0; t<num_time_vals; t++) {
			int vt = var_info[VERT_VAR].time_min;
			int ht = var_info[HOR_VAR].time_min;
            
			if (var_info[VERT_VAR].sync_with_global_time) vt += t;
			if (var_info[HOR_VAR].sync_with_global_time) ht += t;
            
			int rows = vert_cat_data.categories[vt].cat_vec.size();
			int cols = horiz_cat_data.categories[ht].cat_vec.size();
			for (int r=0; r<rows; r++) {
				for (int c=0; c<cols; c++) {
					for (int i=0; i<cur_intervals; i++) {
						cell_data[t][r][c].ival_obs_sel_cnt[i] = 0;
					}
				}
			}
		}
	} else if (type == HLStateInt::delta) {
		std::vector<bool>& hs = highlight_state->GetHighlight();
		std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
		int nh_cnt = highlight_state->GetTotalNewlyHighlighted();
		int nuh_cnt = highlight_state->GetTotalNewlyUnhighlighted();
	
		for (int t=0; t<num_time_vals; t++) {
			int vt = var_info[VERT_VAR].time_min;
			int ht = var_info[HOR_VAR].time_min;
            
			if (var_info[VERT_VAR].sync_with_global_time) vt += t;
			if (var_info[HOR_VAR].sync_with_global_time) ht += t;
            
			int rows = vert_cat_data.categories[vt].cat_vec.size();
			int cols = horiz_cat_data.categories[ht].cat_vec.size();
			int ivals = cell_data[t][0][0].ival_obs_cnt.size();
			
			for (int r=0; r<rows; r++) {
				for (int c=0; c<cols; c++) {
					for (int i=0; i<cur_intervals; i++) {
						cell_data[t][r][c].ival_obs_sel_cnt[i] = 0;
					}
				}
			}
            
			int r, c, ival;
            for (int i=0; i< (int)hs.size(); i++) {
                if (hs[i] && !undef_tms[t][i]) {
    				sel_shp_to_cell_gen(obs_id_to_sel_shp[t][i],
    									r, c, ival, cols, ivals);
    				cell_data[t][r][c].ival_obs_sel_cnt[ival]++;
                }
			}
		}
	} else if (type == HLStateInt::invert) {
		for (int t=0; t<num_time_vals; t++) {
			int vt = var_info[VERT_VAR].time_min;
			int ht = var_info[HOR_VAR].time_min;
            
			if (var_info[VERT_VAR].sync_with_global_time) vt += t;
			if (var_info[HOR_VAR].sync_with_global_time) ht += t;
            
			int rows = vert_cat_data.categories[vt].cat_vec.size();
			int cols = horiz_cat_data.categories[ht].cat_vec.size();
			for (int r=0; r<rows; r++) {
				for (int c=0; c<cols; c++) {
					for (int i=0; i<cur_intervals; i++) {
						cell_data[t][r][c].ival_obs_sel_cnt[i] = 
							(cell_data[t][r][c].ival_obs_cnt[i] -
							 cell_data[t][r][c].ival_obs_sel_cnt[i]);
					}
				}
			}
		}
	}
}

void ConditionalHistogramCanvas::UserChangedCellCategories()
{
	InitIntervals();
}

void ConditionalHistogramCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
    
	int t = var_info[HIST_VAR].time;
    
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    wxString s;
    
	if (total_hover_obs == 0) {
        if (highlight_state->GetTotalHighlighted()> 0) {
            int n_total_hl = highlight_state->GetTotalHighlighted();
            s << "#selected=" << n_total_hl << "  ";
            
            int n_undefs = 0;
            for (int i=0; i<num_obs; i++) {
                if (undef_tms[t][i] && hl[i]) {
                    n_undefs += 1;
                }
            }
            if (n_undefs> 0) {
                s << "(undefined:" << n_undefs << ") ";
            }
        }
		sb->SetStatusText(s);
		return;
	}
	int r, c, ival;
	sel_shp_to_cell(hover_obs[0], r, c, ival);
	double ival_min = (ival == 0) ? min_ival_val[t] : ival_breaks[t][ival-1];
	double ival_max = ((ival == cur_intervals-1) ?
					   max_ival_val[t] : ival_breaks[t][ival]);
	s << "bin: " << ival+1 << ", range: [" << ival_min << ", " << ival_max;
	s << (ival == cur_intervals-1 ? "]" : ")");
	s << ", #obs: " << cell_data[t][r][c].ival_obs_cnt[ival];
	s << ", #sel: " << cell_data[t][r][c].ival_obs_sel_cnt[ival];
	sb->SetStatusText(s);
}

void ConditionalHistogramCanvas::sel_shp_to_cell_gen(int i,
													 int& r, int& c, int& ival,
													 int cols, int ivals)
{	
	int t = cols*ivals;
	r = i/t;
	t = i%t;
	c = t/ivals;
	ival = t%ivals;
}

void ConditionalHistogramCanvas::sel_shp_to_cell(int i, int& r,
												 int& c, int& ival)
{	
	// rows == vert_num_cats
	// cols == horiz_num_cats
	// ivals == cur_intervals
	int t = horiz_num_cats*cur_intervals;
	r = i/t;
	t = i%t;
	c = t/cur_intervals;
	ival = t%cur_intervals;
}

int ConditionalHistogramCanvas::cell_to_sel_shp_gen(int r, int c, int ival,
													int cols, int ivals)
{
	return r*cols*ivals + c*ivals + ival;
}


IMPLEMENT_CLASS(ConditionalHistogramFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalHistogramFrame, ConditionalNewFrame)
EVT_ACTIVATE(ConditionalHistogramFrame::OnActivate)	
END_EVENT_TABLE()

ConditionalHistogramFrame::ConditionalHistogramFrame(wxFrame *parent,
								Project* project,
								const std::vector<GdaVarTools::VarInfo>& var_info,
								const std::vector<int>& col_ids,
								const wxString& title, const wxPoint& pos,
								const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,
					  size, style)
{
    wxLogMessage("Open ConditionalHistogramFrame");
	int width, height;
	GetClientSize(&width, &height);
	
	template_canvas = new ConditionalHistogramCanvas(this, this, project,
													   var_info, col_ids,
													   wxDefaultPosition,
													   wxSize(width,height));
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	
	Show(true);
}

ConditionalHistogramFrame::~ConditionalHistogramFrame()
{
	DeregisterAsActive();
}

void ConditionalHistogramFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In ConditionalMapFrame::OnActivate()");
		RegisterAsActive("ConditionalHistogramFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalHistogramFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_HISTOGRAM_VIEW_MENU_OPTIONS");
	((ConditionalHistogramCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((ConditionalHistogramCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConditionalHistogramFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((ConditionalHistogramCanvas*)
		 template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConditionalHistogramFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void  ConditionalHistogramFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
}

void ConditionalHistogramFrame::OnShowAxes(wxCommandEvent& ev)
{
    wxLogMessage("In ConditionalHistogramFrame::OnShowAxes()");
	ConditionalHistogramCanvas* t =
		(ConditionalHistogramCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void ConditionalHistogramFrame::OnHistogramIntervals(wxCommandEvent& ev)
{
    wxLogMessage("In ConditionalHistogramFrame::OnHistogramIntervals()");
	ConditionalHistogramCanvas* t =
		(ConditionalHistogramCanvas*) template_canvas;
	t->HistogramIntervals();
}
