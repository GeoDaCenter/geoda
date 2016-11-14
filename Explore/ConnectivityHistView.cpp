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
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DialogTools/HistIntervalDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/WeightsManDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "../ShapeOperations/WeightsManState.h"
#include "ConnectivityHistView.h"

IMPLEMENT_CLASS(ConnectivityHistCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(ConnectivityHistCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConnectivityHistCanvas::MAX_INTERVALS = 200;
const double ConnectivityHistCanvas::left_pad_const = 0;
const double ConnectivityHistCanvas::right_pad_const = 0;
const double ConnectivityHistCanvas::interval_width_const = 10;
const double ConnectivityHistCanvas::interval_gap_const = 0;

ConnectivityHistCanvas::ConnectivityHistCanvas(wxWindow *parent,
									TemplateFrame* t_frame,
									Project* project_s,
									boost::uuids::uuid w_uuid_s,
									const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s, project_s->GetHighlightState(),
								 pos, size, false, true),
num_obs(project_s->GetNumRecords()),
connectivity(project_s->GetNumRecords()), num_isolates(0),
x_axis(0), y_axis(0), display_stats(false), show_axes(true),
w_uuid(w_uuid_s), w_man_int(project_s->GetWManInt())
{
	using namespace Shapefile;
	
	InitData();
	InitIntervals();
	
	highlight_color = GdaConst::highlight_color;

    last_scale_trans.SetFixedAspectRatio(false);
	use_category_brushes = false;
	selectable_shps_type = rectangles;
	
	PopulateCanvas();
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

ConnectivityHistCanvas::~ConnectivityHistCanvas()
{
	highlight_state->removeObserver(this);
}

void ConnectivityHistCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	if (ConnectivityHistFrame* f =
			dynamic_cast<ConnectivityHistFrame*>(template_frame)) {
		f->OnActivate(ae);
	} else if (WeightsManFrame* f =
			   dynamic_cast<WeightsManFrame*>(template_frame)) {
		f->OnActivate(ae);
	}
	
	// Correction for when canvas is offset in parent.
	wxPoint cp_pos(pos);
	wxPoint my_pos = this->GetPosition();
	if (my_pos != cp_pos) {
		cp_pos += my_pos;
	}
	
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CONNECTIVITY_HIST_VIEW_MENU_OPTIONS");
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, cp_pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void ConnectivityHistCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_SELECT_ISOLATES"),
								   HasIsolates());

}

void ConnectivityHistCanvas::DetermineMouseHoverObjects(wxPoint pt)
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
// all rectangles.
void ConnectivityHistCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	bool rect_sel = (!pointsel && (brushtype == rectangle));
	
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
										  lower_left, upper_right))) {
				//LOG_MSG(wxString::Format("ival %d selected", i));
				any_selected = true;
				break;
			} else {
				//LOG_MSG(wxString::Format("ival %d not selected", i));
				//LOG_MSG(wxString::Format(""));
			}
		}
		if (!any_selected) {
			highlight_state->SetEventType(HLStateInt::unhighlight_all);
			highlight_state->notifyObservers(this);
            selection_changed = true;
		}
	}

    if (selection_changed == false) {
    	for (int i=0; i<total_sel_shps; i++) {
    		GdaRectangle* rec = (GdaRectangle*) selectable_shps[i];
    		bool selected = ((pointsel && rec->pointWithin(sel1)) ||
    						 (rect_sel &&
    						  GenGeomAlgs::RectsIntersect(rec->lower_left,
    												   rec->upper_right,
    												   lower_left, upper_right)));
    		bool all_sel = (ival_obs_cnt[i] == ival_obs_sel_cnt[i]);
    		if (pointsel && all_sel && selected) {
    			// unselect all in ival
    			for (std::list<int>::iterator it=ival_to_obs_ids[i].begin();
                     it != ival_to_obs_ids[i].end(); it++) {
                    hs[(*it)] = false;
                    selection_changed = true;
    			}
    		} else if (!all_sel && selected) {
    			// select currently unselected in ival
    			for (std::list<int>::iterator it=ival_to_obs_ids[i].begin();
    				 it != ival_to_obs_ids[i].end(); it++) {
    				if (hs[*it]) continue;
                    hs[(*it)] = true;
                    selection_changed = true;
    			}
    		} else if (!selected && !shiftdown) {
    			// unselect all selected in ival
    			for (std::list<int>::iterator it=ival_to_obs_ids[i].begin();
    				 it != ival_to_obs_ids[i].end(); it++) {
                    if (!hs[*it]) continue;
                    hs[(*it)] = false;
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
    	layer1_valid = false;
    	UpdateIvalSelCnts();
        DrawLayers();
    	
    	Refresh();
	}
	UpdateStatusBar();
}

void ConnectivityHistCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (ival_obs_cnt[i] == 0) continue;
		selectable_shps[i]->paintSelf(dc);
	}
}

void ConnectivityHistCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (ival_obs_sel_cnt[i] == 0) continue;
		double s = (((double) ival_obs_sel_cnt[i]) /
					((double) ival_obs_cnt[i]));
		GdaRectangle* rec = (GdaRectangle*) selectable_shps[i];
    	dc.SetPen(rec->getPen());
        dc.SetBrush(rec->getBrush());
		dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
						 rec->upper_right.x - rec->lower_left.x,
						 (rec->upper_right.y - rec->lower_left.y)*s);
	}	
}

/** Override of TemplateCanvas method. */
void ConnectivityHistCanvas::update(HLStateInt* o)
{
    ResetBrushing();
    
	layer1_valid = false;
	UpdateIvalSelCnts();

    
	Refresh();
}

wxString ConnectivityHistCanvas::GetCanvasTitle()
{
	wxString s;
	s << "Connectivity Histogram - " << w_man_int->GetLongDispName(w_uuid);
	return s;
}

void ConnectivityHistCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	if (w_uuid.is_nil()) {
        last_scale_trans.SetData(0,0, 100, 100);
        last_scale_trans.SetMargin(0, 0, 0, 0);
		
		ResizeSelectableShps();
		return;
	}
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const + interval_width_const * cur_intervals + interval_gap_const * (cur_intervals-1);
	
	// orig_x_pos is the center of each histogram bar
	std::vector<double> orig_x_pos(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		orig_x_pos[i] = left_pad_const + interval_width_const/2.0 + i * (interval_width_const + interval_gap_const);
	}
	
    double y_max = max_num_obs_in_ival;
    
    last_scale_trans.SetData(x_min, 0, x_max, y_max);
	
	if (show_axes) {
		axis_scale_y = AxisScale(0, y_max, 5, 0);
		y_max = axis_scale_y.scale_max;
		y_axis = new GdaAxis(_("Frequency"), axis_scale_y, wxRealPoint(0,0), wxRealPoint(0, y_max), -9, 0);
		foreground_shps.push_back(y_axis);
		
		axis_scale_x = AxisScale(0, max_ival_val, 5, axis_display_precision);
        
		axis_scale_x.data_min = min_ival_val;
		axis_scale_x.data_max = max_ival_val;
		axis_scale_x.scale_min = axis_scale_x.data_min;
		axis_scale_x.scale_max = axis_scale_x.data_max;
        
		double range = axis_scale_x.scale_max - axis_scale_x.scale_min;
		axis_scale_x.scale_range = range;
		axis_scale_x.p = floor(log10(range));
		axis_scale_x.ticks = cur_intervals + 1;
		axis_scale_x.tics.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str_show.resize(axis_scale_x.tics_str.size());
		for (int i=0; i<axis_scale_x.ticks; i++) {
			axis_scale_x.tics[i] = axis_scale_x.data_min + range*((double) i)/((double) axis_scale_x.ticks-1);
			std::ostringstream ss;
			ss << std::setprecision(3) << axis_scale_x.tics[i];
			axis_scale_x.tics_str[i] = ss.str();
			axis_scale_x.tics_str_show[i] = false;
		}
		int tick_freq = ceil(((double) cur_intervals)/10.0);
		for (int i=0; i<axis_scale_x.ticks; i++) {
			if (i % tick_freq == 0) {
				axis_scale_x.tics_str_show[i] = true;
			}
		}
		axis_scale_x.tic_inc = axis_scale_x.tics[1]-axis_scale_x.tics[0];
		x_axis = new GdaAxis(_("Number of Neighbors"), axis_scale_x, wxRealPoint(0,0), wxRealPoint(x_max, 0), 0, 9);
		foreground_shps.push_back(x_axis);
	}
	
	GdaShape* s = 0;
	if (HasIsolates()) {
		wxString msg;
		if (num_isolates > 1) {
            msg = wxString::Format(_("Warning: %d observations are neighborless."), num_isolates);
		} else {
            msg = wxString::Format(_("Warning: %d observations is neighborless."), num_isolates);
		}
        s = new GdaShapeText(msg, *GdaConst::small_font,
                             wxRealPoint(((double) x_max)/2.0, y_max),
                             0, GdaShapeText::h_center,
                             GdaShapeText::bottom, 0, -15);
		foreground_shps.push_back(s);
	}
		
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
							  GdaShapeText::top,
						GdaShapeText::right, GdaShapeText::v_center,
							  3, 10, -62, 53+y_d);
		foreground_shps.push_back(s);
		{
			wxClientDC dc(this);
			((GdaShapeTable*) s)->GetSize(dc, table_w, table_h);
		}
		for (int i=0; i<cur_intervals; i++) {
			std::vector<wxString> vals(rows);
			double ival_min = (i == 0) ? min_ival_val : ival_breaks[i-1];
			double ival_max = ((i == cur_intervals-1) ?
							   max_ival_val : ival_breaks[i]);
			double p = 100.0*((double) ival_obs_cnt[i])/((double) num_obs);
			double sd = data_stats.sd_with_bessel;
			double mean = data_stats.mean;
			double sd_d = 0;
			if (ival_max < mean && sd > 0) {
				sd_d = (ival_max - mean)/sd;
			} else if (ival_min > mean && sd > 0) {
				sd_d = (ival_min - mean)/sd;
			}
			vals[0] << GenUtils::DblToStr(ival_min, 3);
			vals[1] << GenUtils::DblToStr(ival_max, 3);
			vals[2] << ival_obs_cnt[i];
			vals[3] << GenUtils::DblToStr(p, 3);
			vals[4] << GenUtils::DblToStr(sd_d, 3);
			
			std::vector<GdaShapeTable::CellAttrib> attribs(0); // undefined
			s = new GdaShapeTable(vals, attribs, rows, cols,
								  *GdaConst::small_font,
								  wxRealPoint(orig_x_pos[i], 0),
								  GdaShapeText::h_center, GdaShapeText::top,
								  GdaShapeText::h_center, GdaShapeText::v_center,
								  3, 10, 0,
							53+y_d);
			foreground_shps.push_back(s);
		}
		
		wxString sts;
		sts << "min: " << data_stats.min;
		sts << ", max: " << data_stats.max;
		sts << ", median: " << hinge_stats.Q2;
		sts << ", mean: " << data_stats.mean;
		sts << ", s.d.: " << data_stats.sd_with_bessel;
		sts << ", #obs: " << num_obs;
	
		s = new GdaShapeText(sts, *GdaConst::small_font,
					   wxRealPoint(x_max/2.0, 0), 0,
					   GdaShapeText::h_center, GdaShapeText::v_center, 0,
					   table_h + 70 + y_d); //145+y_d);
		foreground_shps.push_back(s);
	}
	
	int virtual_screen_marg_top = 25;
	if (HasIsolates()) virtual_screen_marg_top += 20;
	int virtual_screen_marg_bottom = 25;
	int virtual_screen_marg_left = 25;
	int virtual_screen_marg_right = 25;
	
	if (show_axes || display_stats) {
		if (!display_stats) {
			virtual_screen_marg_bottom += 32;
			virtual_screen_marg_left += 35;
		} else {
			int y_d = show_axes ? 0 : -35;
			virtual_screen_marg_bottom += table_h + 65 + y_d; //135 + y_d;
			virtual_screen_marg_left += 82;
		}
	}
    
    last_scale_trans.top_margin = virtual_screen_marg_top;
    last_scale_trans.bottom_margin = virtual_screen_marg_bottom;
    last_scale_trans.left_margin = virtual_screen_marg_left;
    last_scale_trans.right_margin = virtual_screen_marg_right;

	selectable_shps.resize(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		double x0 = orig_x_pos[i] - interval_width_const/2.0;
		double x1 = orig_x_pos[i] + interval_width_const/2.0;
		double y0 = 0;
		double y1 = ival_obs_cnt[i];
		selectable_shps[i] = new GdaRectangle(wxRealPoint(x0, 0),
											 wxRealPoint(x1, y1));
		int sz = GdaConst::qualitative_colors.size();
		selectable_shps[i]->setPen(GdaConst::qualitative_colors[i%sz]);
		selectable_shps[i]->setBrush(GdaConst::qualitative_colors[i%sz]);
	}
	
	ResizeSelectableShps();
}

void ConnectivityHistCanvas::ChangeWeights(boost::uuids::uuid new_id)
{
	if (new_id == w_uuid) return;
	w_uuid = new_id;
	InitData();
	InitIntervals();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConnectivityHistCanvas::SelectIsolates()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	for (int i=0; i<num_obs; i++) {
		if (!hs[i] && connectivity[i]==0) {
            hs[i] = true;
            selection_changed = true;
		} else if (hs[i] && connectivity[i]!=0) {
            hs[i] = false;
            selection_changed = true;
		}
	}
    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers(this);
        
        // trigger to draw highlighted
    	layer1_valid = false;
    	UpdateIvalSelCnts();
    	
    	Refresh();
	}	
}

void ConnectivityHistCanvas::SaveConnectivityToTable()
{
	std::vector<wxInt64> t_con(num_obs);
	for (int i=0; i<num_obs; i++) t_con[i] = connectivity[i];
	std::vector<SaveToTableEntry> data(1);
	data[0].l_val = &t_con;
	data[0].label << "Connectivity of " << w_man_int->GetLongDispName(w_uuid);
	data[0].field_default = "NUM_NBRS";
	data[0].type = GdaConst::long64_type;
	
	SaveToTableDlg dlg(project, this, data,
					   "Save Connectivity to Table",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void ConnectivityHistCanvas::HistogramIntervals()
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

void ConnectivityHistCanvas::InitData()
{
	w_man_int->GetCounts(w_uuid, connectivity);
	data_sorted.resize(num_obs);
	num_isolates = 0;
	for (int i=0; i<num_obs; i++) {
		data_sorted[i].first = connectivity[i];
		data_sorted[i].second = i;
		if (connectivity[i] == 0) num_isolates++;
	}
	
	std::sort(data_sorted.begin(), data_sorted.end(),
			  Gda::dbl_int_pair_cmp_less);
	has_isolates = data_sorted[0].first == 0;
	
    std::vector<bool> undefs(num_obs, false);
	data_stats.CalculateFromSample(data_sorted, undefs);
	hinge_stats.CalculateHingeStats(data_sorted);
	int min_connectivity = data_sorted[0].first;
	int max_connectivity = data_sorted[num_obs-1].first;
	int range = max_connectivity - min_connectivity;
	if (range == 0) {
		range = 1;
		default_intervals = 1;
	} else {
		default_intervals = GenUtils::min<int>(MAX_INTERVALS, range+1);
	}
	
	obs_id_to_ival.resize(num_obs);
	max_intervals = GenUtils::min<int>(MAX_INTERVALS, num_obs);
	cur_intervals = GenUtils::min<int>(max_intervals, default_intervals);
}

/** based on cur_intervals, calculate interval breaks and populate
 obs_id_to_ival, ival_obs_cnt and ival_obs_sel_cnt */ 
void ConnectivityHistCanvas::InitIntervals()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	ival_breaks.resize(cur_intervals-1);
	ival_obs_cnt.resize(cur_intervals);
	ival_obs_sel_cnt.resize(cur_intervals);
	ival_to_obs_ids.clear();
	ival_to_obs_ids.resize(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		ival_obs_cnt[i] = 0;
		ival_obs_sel_cnt[i] = 0;
	}

	min_ival_val = data_sorted[0].first;
	max_ival_val = data_sorted[num_obs-1].first+1;
	double range = (max_ival_val - min_ival_val);
	double ival_size = range/((double) cur_intervals);
		
	for (int i=0; i<cur_intervals-1; i++) {
		ival_breaks[i] = min_ival_val+ival_size*((double) (i+1));
	}
	for (int i=0, cur_ival=0; i<num_obs; i++) {
		while (cur_ival <= cur_intervals-2 &&
				data_sorted[i].first >= ival_breaks[cur_ival]) {
			cur_ival++;
		}
		ival_to_obs_ids[cur_ival].push_front(data_sorted[i].second);
		obs_id_to_ival[data_sorted[i].second] = cur_ival;
		ival_obs_cnt[cur_ival]++;
		if (hs[data_sorted[i].second]) {
			ival_obs_sel_cnt[cur_ival]++;
		}
	}
	
	max_num_obs_in_ival = 0;
	for (int i=0; i<cur_intervals; i++) {
		if (ival_obs_cnt[i] > max_num_obs_in_ival) {
			max_num_obs_in_ival = ival_obs_cnt[i];
		}
	}
	
}

void ConnectivityHistCanvas::UpdateIvalSelCnts()
{
	HLStateInt::EventType type = highlight_state->GetEventType();
	if (type == HLStateInt::unhighlight_all) {
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_sel_cnt[i] = 0;
		}
	} else if (type == HLStateInt::delta) {
		std::vector<bool>& hs = highlight_state->GetHighlight();
       
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_sel_cnt[i] = 0;
		}
        
        for (int i=0; i< (int)hs.size(); i++) {
            if (hs[i]) {
                ival_obs_sel_cnt[obs_id_to_ival[i]]++;
            }
        }
	} else if (type == HLStateInt::invert) {
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_sel_cnt[i] = 
				ival_obs_cnt[i] - ival_obs_sel_cnt[i];
		}
	}
}

void ConnectivityHistCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConnectivityHistCanvas::ShowAxes(bool show_axes_s)
{
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void ConnectivityHistCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	if (total_hover_obs == 0) {
		sb->SetStatusText("");
		return;
	}
	int ival = hover_obs[0];
	wxString s;
	double ival_min = (ival == 0) ? min_ival_val : ival_breaks[ival-1];
	double ival_max = ((ival == cur_intervals-1) ?
					   max_ival_val : ival_breaks[ival]);
	s << "bin: " << ival+1 << ", range: [" << ival_min << ", " << ival_max;
	s << ")";
	s << ", #obs: " << ival_obs_cnt[ival];
	double p = 100.0*((double) ival_obs_cnt[ival])/((double) num_obs);
	s << ", %tot: ";
	s << wxString::Format("%.1f", p);
	s << "%, #sel: " << ival_obs_sel_cnt[ival];
	double sd = data_stats.sd_with_bessel;
	double mean = data_stats.mean;
	double sd_d = 0;
	if (ival_max < mean && sd > 0) {
		sd_d = (ival_max - mean)/sd;
	} else if (ival_min > mean && sd > 0) {
		sd_d = (ival_min - mean)/sd;
	}
	s << ", sd from mean: " << GenUtils::DblToStr(sd_d, 3);
	
	sb->SetStatusText(s);
}


IMPLEMENT_CLASS(ConnectivityHistFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(ConnectivityHistFrame, TemplateFrame)
	EVT_ACTIVATE(ConnectivityHistFrame::OnActivate)
END_EVENT_TABLE()

ConnectivityHistFrame::ConnectivityHistFrame(wxFrame *parent, Project* project,
											 boost::uuids::uuid w_uuid_s,
											 const wxString& title,
											 const wxPoint& pos,
											 const wxSize& size,
											 const long style)
: TemplateFrame(parent, project, title, pos, size, style),
w_man_state(project->GetWManState()), w_man_int(project->GetWManInt()),
w_uuid(w_uuid_s)
{
	wxLogMessage("Open ConnectivityHistFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	
	template_canvas = new ConnectivityHistCanvas(this, this, project,
												 w_uuid, wxDefaultPosition,
												 wxSize(width,height));
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	w_man_state->registerObserver(this);
		
	Show(true);
}

ConnectivityHistFrame::~ConnectivityHistFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
	if (w_man_state) {
		w_man_state->removeObserver(this);
		w_man_state = 0;
	}
}

void ConnectivityHistFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("ConnectivityHistFrame::OnActivate");
        
		RegisterAsActive("ConnectivityHistFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConnectivityHistFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CONNECTIVITY_HIST_VIEW_MENU_OPTIONS");
	((ConnectivityHistCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConnectivityHistFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((ConnectivityHistCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConnectivityHistFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void ConnectivityHistFrame::update(TimeState* o)
{
	UpdateTitle();
}

/** Implementation of WeightsManStateObserver interface */
void ConnectivityHistFrame::update(WeightsManState* o)
{
	if (o->GetWeightsId() != w_uuid) return;
	if (o->GetEventType() == WeightsManState::name_change_evt) {
		UpdateTitle();
		return;
	}
	if (o->GetEventType() == WeightsManState::remove_evt) {
		Destroy();
	}
}

void ConnectivityHistFrame::closeObserver(boost::uuids::uuid id)
{
	if (numMustCloseToRemove(id) > 0) {
		if (w_man_state) {
			w_man_state->removeObserver(this);
			w_man_state = 0;
		}
		Close(true);
	}
}

void ConnectivityHistFrame::ChangeWeights(boost::uuids::uuid new_id)
{
	if (new_id == w_uuid || new_id.is_nil()) return;
	w_uuid = new_id;
	UpdateTitle();
	((ConnectivityHistCanvas*) template_canvas)->ChangeWeights(new_id);
}

void ConnectivityHistFrame::OnShowAxes(wxCommandEvent& event)
{
	wxLogMessage("In ConnectivityHistFrame::OnShowAxes");
	ConnectivityHistCanvas* t = (ConnectivityHistCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void ConnectivityHistFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	wxLogMessage("In ConnectivityHistFrame::OnDisplayStatistics");
	ConnectivityHistCanvas* t = (ConnectivityHistCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void ConnectivityHistFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	wxLogMessage("In ConnectivityHistFrame::OnDisplayStatistics");
	ConnectivityHistCanvas* t = (ConnectivityHistCanvas*) template_canvas;
	t->HistogramIntervals();
}

void ConnectivityHistFrame::OnSaveConnectivityToTable(wxCommandEvent& event)
{
	wxLogMessage("In ConnectivityHistFrame::OnSaveConnectivityToTable");
	ConnectivityHistCanvas* t = (ConnectivityHistCanvas*) template_canvas;
	t->SaveConnectivityToTable();
}

void ConnectivityHistFrame::OnSelectIsolates(wxCommandEvent& event)
{
	wxLogMessage("In ConnectivityHistFrame::OnSelectIsolates");
	ConnectivityHistCanvas* t = (ConnectivityHistCanvas*) template_canvas;
	t->SelectIsolates();
}
