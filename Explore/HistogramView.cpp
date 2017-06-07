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
#include <cfloat>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/HistIntervalDlg.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../FramesManager.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "CatClassifManager.h"
#include "CatClassifState.h"
#include "HistogramView.h"

IMPLEMENT_CLASS(HistogramCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(HistogramCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int HistogramCanvas::MAX_INTERVALS = 200;
const int HistogramCanvas::default_intervals = 7;
const double HistogramCanvas::left_pad_const = 0;
const double HistogramCanvas::right_pad_const = 0;
const double HistogramCanvas::interval_width_const = 10;
const double HistogramCanvas::interval_gap_const = 0;

HistogramCanvas::HistogramCanvas(wxWindow *parent, TemplateFrame* t_frame,
								 Project* project_s,
								 const std::vector<GdaVarTools::VarInfo>& v_info,
								 const std::vector<int>& col_ids,
								 const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s, project_s->GetHighlightState(),
								 pos, size, false, true),
var_info(v_info),
num_obs(project_s->GetNumRecords()),
num_time_vals(1),
x_axis(0), y_axis(0), display_stats(false), show_axes(true),
scale_x_over_time(false), scale_y_over_time(true),
custom_classif_state(0), is_custom_category(false)
{
	using namespace Shapefile;
    
    axis_display_precision = 1;
    
	table_int = project->GetTableInt();
  
    // Histogram has only one variable, so size of col_ids = 1
    col_id = col_ids[0];
    int num_var = v_info.size();
    int col_time_steps = table_int->GetColTimeSteps(col_id);
    
    // prepare statistics
	data_stats.resize(col_time_steps);
	hinge_stats.resize(col_time_steps);
	data_sorted.resize(col_time_steps);
    
    bool has_init = false;
    for (int t=0; t<col_time_steps; t++) {
        std::vector<double> sel_data;
        std::vector<bool> sel_undefs;
        table_int->GetColData(col_id, t, sel_data);
        table_int->GetColUndefined(col_id, t, sel_undefs);
        undef_tms.push_back(sel_undefs);
        
        data_sorted[t].resize(num_obs);
        // data_sorted is a pair value {double value: index}
        for (int i=0; i<num_obs; i++) {
            data_sorted[t][i].first = sel_data[i];
            data_sorted[t][i].second = i;
        }
        // sort data_sorted by value
        std::sort(data_sorted[t].begin(),
                  data_sorted[t].end(),
                  Gda::dbl_int_pair_cmp_less);
        
        data_stats[t].CalculateFromSample(data_sorted[t], sel_undefs);
        hinge_stats[t].CalculateHingeStats(data_sorted[t], sel_undefs);
       
        if (!has_init) {
            data_min_over_time = data_stats[t].min;
            data_max_over_time = data_stats[t].max;
            has_init = true;
        } else {
            // get min max values
            if (data_stats[t].min < data_min_over_time) {
                data_min_over_time = data_stats[t].min;
            }
            if (data_stats[t].max > data_max_over_time) {
                data_max_over_time = data_stats[t].max;
            }
        }
    }
    
    // Setup Group Dependency
    template_frame->ClearAllGroupDependencies();
    for (int i=0, sz=var_info.size(); i<sz; ++i) {
        template_frame->AddGroupDependancy(var_info[i].name);
    }
    
    obs_id_to_ival.resize(boost::extents[col_time_steps][num_obs]);
    max_intervals = GenUtils::min<int>(MAX_INTERVALS, num_obs);
    cur_intervals = GenUtils::min<int>(max_intervals, default_intervals);
    /*
    if (num_obs > 49) {
        int c = sqrt((double) num_obs);
        cur_intervals = GenUtils::min<int>(max_intervals, c);
        cur_intervals = GenUtils::min<int>(cur_intervals, 25);
    }
     */
    min_ival_val.resize(col_time_steps);
    max_ival_val.resize(col_time_steps);
    max_num_obs_in_ival.resize(col_time_steps);
    ival_to_obs_ids.resize(col_time_steps);
    
	highlight_color = GdaConst::highlight_color;
    
    last_scale_trans.SetFixedAspectRatio(false);
	use_category_brushes = false;
	selectable_shps_type = rectangles;
	
	ref_var_index = var_info[0].is_time_variant ? 0 : -1;	
	VarInfoAttributeChange();
	InitIntervals();
	PopulateCanvas();

	DisplayStatistics(false);
	
	highlight_state->registerObserver(this);
}

HistogramCanvas::~HistogramCanvas()
{
	highlight_state->removeObserver(this);
    if (custom_classif_state) {
        custom_classif_state->removeObserver(this);
    }
}

void HistogramCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((HistogramFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu;
    wxString menu_xrcid = "ID_HISTOGRAM_NEW_VIEW_MENU_OPTIONS";
	optMenu = wxXmlResource::Get()-> LoadMenu(menu_xrcid);
	AddTimeVariantOptionsToMenu(optMenu);
    
    CatClassifManager* cat_classif = project->GetCatClassifManager();
    int n_cat = AddClassificationOptionsToMenu(optMenu, cat_classif);
    
    template_frame->Connect(GdaConst::ID_HISTOGRAM_CLASSIFICATION,
                            wxEVT_COMMAND_MENU_SELECTED,
                            wxCommandEventHandler(HistogramFrame::OnHistClassification));
    
    for (int i=1; i<=n_cat; i++) {
        template_frame->Connect(GdaConst::ID_HISTOGRAM_CLASSIFICATION+i,
                                wxEVT_COMMAND_MENU_SELECTED,
                                wxCommandEventHandler(HistogramFrame::OnHistClassification));
    }
    
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void HistogramCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!var_info[0].is_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	{
		wxString s;
		s << "Synchronize " << var_info[0].name << " with Time Control";
		wxMenuItem* mi =
		menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+0, s, s);
		mi->Check(var_info[0].sync_with_global_time);
	}
    menu->AppendSeparator();
    menu->Append(wxID_ANY, _("Time Variable Options"),
                 menu1, _("Time Variable Options"));
	
}

int HistogramCanvas::AddClassificationOptionsToMenu(wxMenu* menu, CatClassifManager* ccm)
{
    std::vector<wxString> titles;
    ccm->GetTitles(titles);
    
	wxMenu* menu2 = new wxMenu(wxEmptyString);
    wxString s = _("Create New Custom");
    int xrcid_hist_classification = GdaConst::ID_HISTOGRAM_CLASSIFICATION;
    wxMenuItem* mi = menu2->Append(xrcid_hist_classification, s, s);
    menu2->AppendSeparator();
    
    for (size_t j=0; j<titles.size(); j++) {
        wxMenuItem* mi = menu2->Append(xrcid_hist_classification+j+1, titles[j]);
    }
    s = _("Histogram Classification");
	menu->Prepend(wxID_ANY, s, menu2, s);
    return titles.size();
}

void HistogramCanvas::OnCustomCategorySelect(wxCommandEvent& e)
{
    int custom_cat_idx = e.GetId() - GdaConst::ID_HISTOGRAM_CLASSIFICATION - 1;
    
    CatClassifManager* ccm = project->GetCatClassifManager();
    if (!ccm)
        return;
    
    std::vector<wxString> titles;
    ccm->GetTitles(titles);
    
    if (custom_cat_idx < 0 || custom_cat_idx > titles.size())
        return;
    
    wxString custom_classif_title = titles[custom_cat_idx];
    CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
    
    if (!new_ccs)
        return;
    
    if (custom_classif_state)
        custom_classif_state->removeObserver(this);
    
    custom_classif_state = new_ccs;
    custom_classif_state->registerObserver(this);
    
    cat_classif_def = custom_classif_state->GetCatClassif();
    is_custom_category = true;
    cur_intervals = cat_classif_def.num_cats;
    
    InitIntervals();
    invalidateBms();
    PopulateCanvas();
    Refresh();
}

void HistogramCanvas::update(CatClassifState* o)
{
    cat_classif_def = o->GetCatClassif();
    cur_intervals = cat_classif_def.num_cats;
    InitIntervals();
    invalidateBms();
    PopulateCanvas();
    Refresh();

}
void HistogramCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	
	if (var_info[0].is_time_variant) {
		GeneralWxUtils::CheckMenuItem(menu,
									  GdaConst::ID_TIME_SYNC_VAR1,
									  var_info[0].sync_with_global_time);
		GeneralWxUtils::CheckMenuItem(menu,
									  GdaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
									  var_info[0].fixed_scale);
	}
}

void HistogramCanvas::DetermineMouseHoverObjects(wxPoint pt)
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
void HistogramCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	bool rect_sel = (!pointsel && (brushtype == rectangle));
	
	int t = var_info[0].time;
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed =false;
	
	int total_sel_shps = selectable_shps.size();
	
	wxPoint lower_left;
	wxPoint upper_right;
	if (rect_sel) {
		GenGeomAlgs::StandardizeRect(sel1, sel2, lower_left, upper_right);
	}
    for (int i=0; i<total_sel_shps; i++) {
        GdaRectangle* rec = (GdaRectangle*) selectable_shps[i];
        bool is_intersect = GenGeomAlgs::RectsIntersect(rec->lower_left,
                                                        rec->upper_right,
                                                        lower_left,
                                                        upper_right);
        bool selected = (pointsel && rec->pointWithin(sel1)) ||
        (rect_sel && is_intersect);
        bool all_sel = (ival_obs_cnt[t][i] == ival_obs_sel_cnt[t][i]);
        
        if (pointsel && all_sel && selected) {
            // unselect all in ival
            for (std::list<int>::iterator it=ival_to_obs_ids[t][i].begin();
                 it != ival_to_obs_ids[t][i].end(); it++)
            {
                if (hs[*it] == false)
                    continue;
                hs[*it] = false;
                selection_changed  = true;
            }
            
        } else if (!all_sel && selected) {
            // select currently unselected in ival
            for (std::list<int>::iterator it=ival_to_obs_ids[t][i].begin();
                 it != ival_to_obs_ids[t][i].end(); it++)
            {
                if (hs[*it]) {
                    continue;
                }
                hs[(*it)] = true;
                selection_changed  = true;
            }
            
        } else if (!selected && !shiftdown) {
            // unselect all selected in ival
            for (std::list<int>::iterator it=ival_to_obs_ids[t][i].begin();
                 it != ival_to_obs_ids[t][i].end(); it++)
            {
                if (!hs[*it]) {
                    continue;
                }
                hs[(*it)] = false;
                selection_changed  = true;
                
            }
        }
    }
    if ( selection_changed ) {
        highlight_state->SetEventType(HLStateInt::delta);
        highlight_state->notifyObservers(this);
    }
    
	if ( selection_changed ) {
        // re-paint highlight layer (layer1_bm)
        layer1_valid = false;
        UpdateIvalSelCnts();
        DrawLayers();
        
	}
    Refresh();
	UpdateStatusBar();
}

void HistogramCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	int t = var_info[0].time;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
        if (ival_obs_cnt[t][i] == 0) {
            continue;
        }
		selectable_shps[i]->paintSelf(dc);
	}
}

void HistogramCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	int t = var_info[0].time;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
        if (ival_obs_sel_cnt[t][i] == 0 || undef_tms[t][i]) {
            continue;
        }
		double s = (((double) ival_obs_sel_cnt[t][i]) /
					((double) ival_obs_cnt[t][i]));
       
        GdaShape* shp = selectable_shps[i];
        dc.SetPen(shp->getPen());
        dc.SetBrush(shp->getBrush());
		GdaRectangle* rec = (GdaRectangle*) shp;
		dc.DrawRectangle(rec->lower_left.x,
                         rec->lower_left.y,
						 rec->upper_right.x - rec->lower_left.x,
						 (rec->upper_right.y - rec->lower_left.y)*s);
	}
}

/** Override of TemplateCanvas method. */
void HistogramCanvas::update(HLStateInt* o)
{
    ResetBrushing();
   
    HLStateInt::EventType type = o->GetEventType();
    if (type == HLStateInt::transparency) {
        ResetFadedLayer();
    }
    
	//layer0_valid = false;
	layer1_valid = false;
	//layer2_valid = false;
	UpdateIvalSelCnts();
    
	Refresh();
    UpdateStatusBar();
}

wxString HistogramCanvas::GetCanvasTitle()
{
	wxString s = _("Histogram: ");
	s << GetNameWithTime(0);
	return s;
}

wxString HistogramCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

void HistogramCanvas::GetBarPositions(std::vector<double>& x_center_pos,
                                    std::vector<double>& x_left_pos,
                                    std::vector<double>& x_right_pos)
{
    int n = x_center_pos.size();
    
    
    if (!is_custom_category) {
        for (int i=0; i<n; i++) {
            double xc = left_pad_const + interval_width_const/2.0 + i * (interval_width_const + interval_gap_const);
            double x0 = xc - interval_width_const/2.0;
            double x1 = xc + interval_width_const/2.0;
            x_center_pos[i] = xc;
            x_left_pos[i] = x0;
            x_right_pos[i] = x1;
        }
    } else {
        std::vector<double>& breaks = cat_classif_def.breaks;
        double x_max = left_pad_const + right_pad_const + interval_width_const * cur_intervals + interval_gap_const * (cur_intervals-1);
        
        double val_max = cat_classif_def.uniform_dist_max;
        double val_min = cat_classif_def.uniform_dist_min;
        double val_range = val_max - val_min;
        double left = val_min;
        
        std::vector<double> ticks;
        ticks.push_back(val_min);
        for(int i=0; i<breaks.size();i++)
            ticks.push_back(breaks[i]);
        ticks.push_back(val_max);
        
        int j=0;
        for (int i=0; i<ticks.size()-1; i++) {
            x_left_pos[j] = x_max * (ticks[i] - left) / val_range;
            x_right_pos[j] = x_max * (ticks[i+1] - left) / val_range;
            
            x_center_pos[j] = (x_right_pos[j] + x_left_pos[j]) / 2.0;
            j++;
        }
    }
}

void HistogramCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	int time = var_info[0].time;
	
	// orig_x_pos is the center of each histogram bar
	std::vector<double> orig_x_pos(cur_intervals);
    std::vector<double> orig_x_pos_left(cur_intervals);
    std::vector<double> orig_x_pos_right(cur_intervals);
    GetBarPositions(orig_x_pos, orig_x_pos_left, orig_x_pos_right);
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const +
                   interval_width_const * cur_intervals +
                   interval_gap_const * (cur_intervals-1);
    double y_min = 0;
	double y_max = scale_y_over_time ? overall_max_num_obs_in_ival : max_num_obs_in_ival[time];
    
    last_scale_trans.SetData(x_min, y_min, x_max, y_max);
    
	if (show_axes) {
		axis_scale_y = AxisScale(0, y_max, 5, axis_display_precision);
		y_max = axis_scale_y.scale_max;
		y_axis = new GdaAxis("Frequency", axis_scale_y,
                             wxRealPoint(0,0), wxRealPoint(0, y_max),
                             -9, 0);
		foreground_shps.push_back(y_axis);
		
		axis_scale_x = AxisScale(0, max_ival_val[time], 5, axis_display_precision);
		//shps_orig_xmax = axis_scale_x.scale_max;
		axis_scale_x.data_min = min_ival_val[time];
		axis_scale_x.data_max = max_ival_val[time];
		axis_scale_x.scale_min = axis_scale_x.data_min;
		axis_scale_x.scale_max = axis_scale_x.data_max;
      
        axis_scale_x.tics.resize(cur_intervals);
        axis_scale_x.tics_str.resize(cur_intervals);
        axis_scale_x.tics_str_show.resize(cur_intervals);
       
        for (int i=0; i<cur_intervals; i++) {
            double x0 = orig_x_pos_left[i];
            double x1 = orig_x_pos_right[i];
            double y0 = 0;
            double y00 = -y_max / 100.0;
            
            wxRealPoint p0;
            wxRealPoint p1;
            last_scale_trans.transform_back(wxPoint(0,0), p0);
            last_scale_trans.transform_back(wxPoint(0,4), p1);
            y00 = p1.y - p0.y;
            
            GdaPolyLine* xline = new GdaPolyLine(x0, y0, x1, y0);
            xline->setNudge(0, 10);
            foreground_shps.push_back(xline);
            
            GdaPolyLine* xdline = new GdaPolyLine(x0, y0, x0, y00);
            xdline->setNudge(0, 10);
            foreground_shps.push_back(xdline);
           
            if (i==0) {
                axis_scale_x.tics[i] = axis_scale_x.data_min;
                wxString tic_str;
                tic_str << axis_scale_x.data_min;
                axis_scale_x.tics_str[i] = tic_str;
                
                GdaShapeText* brk =
                new GdaShapeText(GenUtils::DblToStr(axis_scale_x.data_min,
                                                    axis_display_precision),
                                 *GdaConst::small_font,
                                 wxRealPoint(x0, y0), 0,
                                 GdaShapeText::h_center,
                                 GdaShapeText::v_center, 0, 25);
                foreground_shps.push_back(brk);
            }
            if (i<cur_intervals-1) {
                axis_scale_x.tics[i] = ival_breaks[time][i];
                
                wxString tic_str;
                tic_str << ival_breaks[time][i];
                axis_scale_x.tics_str[i] = tic_str;
                
                GdaShapeText* brk =
                new GdaShapeText(GenUtils::DblToStr(ival_breaks[time][i],
                                                    axis_display_precision),
                                 *GdaConst::small_font,
                                 wxRealPoint(x1, y0), 0,
                                 GdaShapeText::h_center,
                                 GdaShapeText::v_center, 0, 25);
                foreground_shps.push_back(brk);
            }
            if (i==cur_intervals-1) {
                axis_scale_x.tics[i] = axis_scale_x.data_max;
                wxString tic_str;
                tic_str << axis_scale_x.data_max;
                axis_scale_x.tics_str[i] = tic_str;
                GdaShapeText* brk =
                new GdaShapeText(GenUtils::DblToStr(axis_scale_x.data_max,
                                                    axis_display_precision),
                                 *GdaConst::small_font,
                                 wxRealPoint(x1, y0), 0,
                                 GdaShapeText::h_center,
                                 GdaShapeText::v_center, 0, 25);
                foreground_shps.push_back(brk);
                
                
                GdaPolyLine* xdline = new GdaPolyLine(x1, y0, x1, y00);
                xdline->setNudge(0, 10);
                foreground_shps.push_back(xdline);
            }
            axis_scale_x.tics_str_show[i] = true;
        }

        GdaShapeText* brk =
        new GdaShapeText(GetNameWithTime(0),
                         *GdaConst::small_font,
                         wxRealPoint((x_max -x_min)/2.0, 0), 0,
                         GdaShapeText::h_center,
                         GdaShapeText::v_center, 0, 38);
        foreground_shps.push_back(brk);

		axis_scale_x.tic_inc = axis_scale_x.tics[1]-axis_scale_x.tics[0];
	}
	
	GdaShape* s = 0;
	int table_w=0, table_h=0;
	if (display_stats) {
		int y_d = show_axes ? 0 : -32;
		int cols = 1;
		int rows = 5;
		std::vector<wxString> vals(rows);
		vals[0] << "from";
		vals[1] << "to";
		vals[2] << "#obs";
		vals[3] << "% of total";
		vals[4] << "sd from mean";
        
		std::vector<GdaShapeTable::CellAttrib> attribs(0); // undefined
		s = new GdaShapeTable(vals, attribs, rows, cols, *GdaConst::small_font,
                              wxRealPoint(0, 0), GdaShapeText::h_center,
                              GdaShapeText::top, GdaShapeText::right,
                              GdaShapeText::v_center, 3, 10, -62, 53+y_d);
		foreground_shps.push_back(s);
        
        wxClientDC dc(this);
        ((GdaShapeTable*) s)->GetSize(dc, table_w, table_h);
        
		for (int i=0; i<cur_intervals; i++) {
			int t = time;
			std::vector<wxString> vals(rows);
			double ival_min = (i == 0) ? min_ival_val[t] : ival_breaks[t][i-1];
			double ival_max = ((i == cur_intervals-1) ?
							   max_ival_val[t] : ival_breaks[t][i]);
			double p = 100.0*((double) ival_obs_cnt[t][i])/((double) num_obs);
			double sd = data_stats[t].sd_with_bessel;
			double mean = data_stats[t].mean;
			double sd_d = 0;
			if (ival_max < mean && sd > 0) {
				sd_d = (ival_max - mean)/sd;
			} else if (ival_min > mean && sd > 0) {
				sd_d = (ival_min - mean)/sd;
			}
			vals[0] << GenUtils::DblToStr(ival_min, 3);
			vals[1] << GenUtils::DblToStr(ival_max, 3);
			vals[2] << ival_obs_cnt[t][i];
			vals[3] << GenUtils::DblToStr(p, 3);
			vals[4] << GenUtils::DblToStr(sd_d, 3);
			
			std::vector<GdaShapeTable::CellAttrib> attribs(0); // undefined
            s = new GdaShapeTable(vals, attribs, rows, cols,
                                  *GdaConst::small_font,
                                  wxRealPoint(orig_x_pos[i], 0),
                                  GdaShapeText::h_center, GdaShapeText::top,
                                  GdaShapeText::h_center, GdaShapeText::v_center,
                                  3, 10, 0, 53+y_d);
			foreground_shps.push_back(s);
		}
		
		wxString sts;
		sts << "min: " << data_stats[time].min;
		sts << ", max: " << data_stats[time].max;
		sts << ", median: " << hinge_stats[time].Q2;
		sts << ", mean: " << data_stats[time].mean;
		sts << ", s.d.: " << data_stats[time].sd_with_bessel;
		sts << ", #obs: " << num_obs;
	
        s = new GdaShapeText(sts, *GdaConst::small_font,
                             wxRealPoint(x_max/2.0, 0), 0,
                             GdaShapeText::h_center, GdaShapeText::v_center,
                             0, table_h + 70 + y_d);
		foreground_shps.push_back(s);
	}
	
    last_scale_trans.SetMargin(35, 25, 25, 25);
	
	if (show_axes || display_stats) {
		if (!display_stats) {
            last_scale_trans.bottom_margin += 32;
            last_scale_trans.left_margin += 35;
		} else {
			int y_d = show_axes ? 0 : -35;
            last_scale_trans.bottom_margin += table_h + 65 + y_d;
            last_scale_trans.left_margin += 82;
		}
	}
	 
	selectable_shps.resize(cur_intervals);
    if (!is_custom_category) {
        cat_classif_def.colors.resize(cur_intervals);
        cat_classif_def.names.resize(cur_intervals);
    }
	for (int i=0; i<cur_intervals; i++) {
        double x0 = orig_x_pos_left[i];
        double x1 = orig_x_pos_right[i];
		double y0 = 0;
		double y1 = ival_obs_cnt[time][i];
		selectable_shps[i] = new GdaRectangle(wxRealPoint(x0, 0),
                                              wxRealPoint(x1, y1));
		
        if (!is_custom_category) {
            int sz = GdaConst::qualitative_colors.size();
            selectable_shps[i]->setPen(GdaConst::qualitative_colors[i%sz]);
            selectable_shps[i]->setBrush(GdaConst::qualitative_colors[i%sz]);
            cat_classif_def.colors[i] = GdaConst::qualitative_colors[i%sz];
        } else {
            selectable_shps[i]->setPen(cat_classif_def.colors[i]);
            selectable_shps[i]->setBrush(cat_classif_def.colors[i]);
        }
	}
	
	ResizeSelectableShps();
}

void HistogramCanvas::NewCustomCatClassif()
{
    int tht = var_info[0].time;
    int col = project->GetTableInt()->FindColId(var_info[0].name);
    cat_classif_def.assoc_db_fld_name = project->GetTableInt()->GetColName(col, tht);

    CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame(false);
    if (!ccf)
        return;

    CatClassifState* ccs = ccf->PromptNew(cat_classif_def,
                                          "",
                                          var_info[0].name,
                                          var_info[0].time,
                                          false);
    
    if (!ccs)
        return;
    
    if (custom_classif_state)
        custom_classif_state->removeObserver(this);
    cat_classif_def = ccs->GetCatClassif();
    custom_classif_state = ccs;
    custom_classif_state->registerObserver(this);
    
    is_custom_category = true;
    cur_intervals = cat_classif_def.num_cats;
    
    InitIntervals();
    invalidateBms();
    PopulateCanvas();
    Refresh();

}

void HistogramCanvas::TimeChange()
{
	if (!is_any_sync_with_global_time) return;
	
	var_info[0].time = project->GetTimeState()->GetCurrTime();

	invalidateBms();
	PopulateCanvas();
	Refresh();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void HistogramCanvas::VarInfoAttributeChange()
{
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
    
    if (var_info[0].is_time_variant) {
        is_any_time_variant = true;
    }
    
	if (var_info[0].sync_with_global_time) {
		is_any_sync_with_global_time = true;
	}
    
	template_frame->SetDependsOnNonSimpleGroups(is_any_time_variant);
	ref_var_index = -1;
	num_time_vals = 1;
    if (var_info[0].is_ref_variable) {
        ref_var_index = 0;
    }
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	//GdaVarTools::PrintVarInfoVector(var_info);
}

void HistogramCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::FixedScaleVariableToggle(int var_index)
{
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::HistogramIntervals()
{
	HistIntervalDlg dlg(1, cur_intervals, max_intervals, this);
	if (dlg.ShowModal () != wxID_OK)
        return;
	cur_intervals = dlg.num_intervals;
    
	isResize = true;
    is_custom_category = false;
	InitIntervals();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

/** based on data_min_over_time, data_max_over_time,
 cur_intervals, scale_x_over_time:
 calculate interval breaks and populate
 obs_id_to_ival, ival_obs_cnt and ival_obs_sel_cnt */ 
void HistogramCanvas::InitIntervals()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	int ts = obs_id_to_ival.shape()[0];
	ival_breaks.resize(boost::extents[ts][cur_intervals-1]);
	ival_obs_cnt.resize(boost::extents[ts][cur_intervals]);
	ival_obs_sel_cnt.resize(boost::extents[ts][cur_intervals]);
    
    cat_classif_def.num_cats = cur_intervals;
    
	for (int t=0; t<ts; t++)
        ival_to_obs_ids[t].clear();
	for (int t=0; t<ts; t++)
        ival_to_obs_ids[t].resize(cur_intervals);
	for (int t=0; t<ts; t++) {
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_cnt[t][i] = 0;
			ival_obs_sel_cnt[t][i] = 0;
		}
	}
    
	for (int t=0; t<ts; t++) {
		if (scale_x_over_time) {
			min_ival_val[t] = data_min_over_time;
			max_ival_val[t] = data_max_over_time;
		} else {
            
            const std::vector<bool>& undefs = undef_tms[t];
            bool has_init = false;
            for (size_t ii=0; ii<undefs.size(); ii++){
                double val = data_sorted[t][ii].first;
                int iid = data_sorted[t][ii].second;
                if (undefs[iid])
                    continue;
                if (!has_init) {
                    min_ival_val[t] = val;
                    max_ival_val[t] = val;
                    has_init = true;
                } else {
                    if ( val < min_ival_val[t] ) {
                        min_ival_val[t] = val;
                    }
                    if ( val > min_ival_val[t] ) {
                        max_ival_val[t] = val;
                    }
                }
            }
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
		
        if (!is_custom_category) {
            cat_classif_def.breaks.resize(cur_intervals-1);
            for (int i=0; i<cur_intervals-1; i++) {
                ival_breaks[t][i] = min_ival_val[t]+ival_size*((double) (i+1));
                cat_classif_def.breaks[i] = ival_breaks[t][i];
            }
        } else {
            for (int i=0; i<cur_intervals-1; i++) {
                ival_breaks[t][i] = cat_classif_def.breaks[i];
            }
        }
        
        const std::vector<bool>& undefs = undef_tms[t];
        
		for (int i=0, cur_ival=0; i<num_obs; i++) {
            
            std::pair<double, int>& data_item = data_sorted[t][i];
            double val = data_item.first;
            int idx = data_item.second;
           
            if (undefs[idx])
                continue;
            
            // detect if need to jump to next interval
			while (cur_ival <= cur_intervals-2 &&
				   val >= ival_breaks[t][cur_ival])
            {
				cur_ival++;
			}
            
            // add current [id] to ival_to_obs_ids
			ival_to_obs_ids[t][cur_ival].push_front(idx);
			obs_id_to_ival[t][idx] = cur_ival;
			ival_obs_cnt[t][cur_ival]++;
            
			if (hs[data_sorted[t][i].second]) {
				ival_obs_sel_cnt[t][cur_ival]++;
			}
		}
	}
	overall_max_num_obs_in_ival = 0;
	for (int t=0; t<ts; t++) {
		max_num_obs_in_ival[t] = 0;
		for (int i=0; i<cur_intervals; i++) {
			if (ival_obs_cnt[t][i] > max_num_obs_in_ival[t]) {
				max_num_obs_in_ival[t] = ival_obs_cnt[t][i];
			}
		}
		if (max_num_obs_in_ival[t] > overall_max_num_obs_in_ival) {
			overall_max_num_obs_in_ival = max_num_obs_in_ival[t];
		}
	}

    int sel_t = var_info[0].time;
    cat_classif_def.uniform_dist_min = data_stats[sel_t].min;
    cat_classif_def.uniform_dist_max = data_stats[sel_t].max;
}

void HistogramCanvas::UpdateIvalSelCnts()
{
	int ts = obs_id_to_ival.shape()[0];	
	HLStateInt::EventType type = highlight_state->GetEventType();
	if (type == HLStateInt::unhighlight_all) {
		for (int t=0; t<ts; t++) {
			for (int i=0; i<cur_intervals; i++) {
				ival_obs_sel_cnt[t][i] = 0;
			}
		}
	} else if (type == HLStateInt::delta) {
		std::vector<bool>& hs = highlight_state->GetHighlight();
       
		for (int t=0; t<ts; t++) {
			for (int i=0; i<cur_intervals; i++) {
				ival_obs_sel_cnt[t][i] = 0;
			}
		}
        
        for (int i=0; i< (int)hs.size(); i++) {
			for (int t=0; t<ts; t++) {
                if (hs[i] && !undef_tms[t][i]) {
                    ival_obs_sel_cnt[t][obs_id_to_ival[t][i]]++;
                }
            }
        }
	} else if (type == HLStateInt::invert) {
		for (int t=0; t<ts; t++) {
			for (int i=0; i<cur_intervals; i++) {
				ival_obs_sel_cnt[t][i] = 
					ival_obs_cnt[t][i] - ival_obs_sel_cnt[t][i];
			}
		}
	}
}

void HistogramCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::ShowAxes(bool show_axes_s)
{
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void HistogramCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
    
	int t = var_info[0].time;
	int ival = hover_obs[0];
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    
	if (total_hover_obs == 0) {
        wxString s;
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
    
	wxString s;
	double ival_min = (ival == 0) ? min_ival_val[t] : ival_breaks[t][ival-1];
	double ival_max = ((ival == cur_intervals-1) ?
					   max_ival_val[t] : ival_breaks[t][ival]);
	s << "bin: " << ival+1 << ", range: [" << ival_min << ", " << ival_max;
	s << (ival == cur_intervals-1 ? "]" : ")");
	s << ", #obs: " << ival_obs_cnt[t][ival];
	double p = 100.0*((double) ival_obs_cnt[t][ival])/((double) num_obs);
	s << ", %tot: ";
	s << wxString::Format("%.1f", p);
	s << "%, #sel: " << ival_obs_sel_cnt[t][ival];
	double sd = data_stats[t].sd_with_bessel;
	double mean = data_stats[t].mean;
	double sd_d = 0;
	if (ival_max < mean && sd > 0) {
		sd_d = (ival_max - mean)/sd;
	} else if (ival_min > mean && sd > 0) {
		sd_d = (ival_min - mean)/sd;
	}
	s << ", sd from mean: " << GenUtils::DblToStr(sd_d, 3);

	sb->SetStatusText(s);
}


IMPLEMENT_CLASS(HistogramFrame, TemplateFrame)

BEGIN_EVENT_TABLE(HistogramFrame, TemplateFrame)
	EVT_ACTIVATE(HistogramFrame::OnActivate)
END_EVENT_TABLE()

HistogramFrame::HistogramFrame(wxFrame *parent, Project* project,
							   const std::vector<GdaVarTools::VarInfo>& var_info,
							   const std::vector<int>& col_ids,
							   const wxString& title, const wxPoint& pos,
							   const wxSize& size, const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	wxLogMessage("Open HistogramFrame.");
	
	int width, height;
	GetClientSize(&width, &height);
	
	template_canvas = new HistogramCanvas(this, this, project,
										   var_info, col_ids,
										   wxDefaultPosition,
										   wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
		
	Show(true);
}

HistogramFrame::~HistogramFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}
void HistogramFrame::OnHistClassification(wxCommandEvent& event)
{
	wxLogMessage("In HistogramFrame::OnHistClassification()");
    int evtID = event.GetId();
    if (evtID == GdaConst::ID_HISTOGRAM_CLASSIFICATION) {
        ((HistogramCanvas*) template_canvas)->NewCustomCatClassif();
    } else if (evtID > GdaConst::ID_HISTOGRAM_CLASSIFICATION) {
        ((HistogramCanvas*) template_canvas)->OnCustomCategorySelect(event);
    }
    //event.Skip();
    
}
void HistogramFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In HistogramFrame::OnActivate()");
		RegisterAsActive("HistogramFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void HistogramFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_HISTOGRAM_NEW_VIEW_MENU_OPTIONS");
	((HistogramCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
    ((HistogramCanvas*) template_canvas)->AddClassificationOptionsToMenu(optMenu, project->GetCatClassifManager());
    
	((HistogramCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void HistogramFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((HistogramCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void HistogramFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void HistogramFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
}



void HistogramFrame::OnShowAxes(wxCommandEvent& event)
{
    wxLogMessage("In HistogramFrame::OnShowAxes()");
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void HistogramFrame::OnDisplayStatistics(wxCommandEvent& event)
{
    wxLogMessage("In HistogramFrame::OnDisplayStatistics()");
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void HistogramFrame::OnHistogramIntervals(wxCommandEvent& event)
{
    wxLogMessage("In HistogramFrame::OnHistogramIntervals()");
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	t->HistogramIntervals();
}

void HistogramFrame::GetVizInfo(wxString& col_name, int& num_bins)
{
	HistogramCanvas* t = (HistogramCanvas*) template_canvas;
	num_bins = t->cur_intervals;
	col_name = t->var_info[0].name;
}
