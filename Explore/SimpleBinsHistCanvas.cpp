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
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DialogTools/HistIntervalDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "SimpleBinsHistCanvas.h"

IMPLEMENT_CLASS(SimpleBinsHistCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(SimpleBinsHistCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int SimpleBinsHistCanvas::MAX_INTERVALS = 200;
const double SimpleBinsHistCanvas::left_pad_const = 0;
const double SimpleBinsHistCanvas::right_pad_const = 0;
const double SimpleBinsHistCanvas::interval_width_const = 10;
const double SimpleBinsHistCanvas::interval_gap_const = 0;

SimpleBinsHistCanvas::SimpleBinsHistCanvas(wxWindow *parent,
									TemplateFrame* t_frame,
									Project* project,
									HLStateInt* hl_state_int,
									SimpleBinsHistCanvasCbInt* sbh_canv_cb_,
									const std::vector<SimpleBin>& hist_bins_,
									const wxString& Xname_,
									double Xmin_, double Xmax_,
									const wxString& right_click_menu_id_,
									bool show_axes_,
									const wxPoint& pos,
									const wxSize& size)
: TemplateCanvas(parent, t_frame, project, hl_state_int,
								 pos, size, false, true),
sbh_canv_cb(sbh_canv_cb_),
hist_bins(hist_bins_), Xname(Xname_), Xmin(Xmin_), Xmax(Xmax_),
x_axis(0), y_axis(0),
right_click_menu_id(right_click_menu_id_),
show_axes(show_axes_)
{
	using namespace Shapefile;
	LOG_MSG("Entering SimpleBinsHistCanvas::SimpleBinsHistCanvas");
	
	hist_bins_max = 0;
	for (size_t i=0; i<hist_bins.size(); ++i) {
		if (hist_bins[i].count > hist_bins_max) {
			hist_bins_max = hist_bins[i].count;
		}
	}
	
	highlight_color = GdaConst::highlight_color;
    last_scale_trans.SetFixedAspectRatio(false);
	use_category_brushes = false;
	selectable_shps_type = rectangles;
	
	PopulateCanvas();
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting SimpleBinsHistCanvas::SimpleBinsHistCanvas");
}

SimpleBinsHistCanvas::~SimpleBinsHistCanvas()
{
	LOG_MSG("Entering SimpleBinsHistCanvas::~SimpleBinsHistCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting SimpleBinsHistCanvas::~SimpleBinsHistCanvas");
}

void SimpleBinsHistCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering SimpleBinsHistCanvas::DisplayRightClickMenu");
	if (right_click_menu_id.IsEmpty()) return;	
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	template_frame->OnActivate(ae);

	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->LoadMenu(right_click_menu_id);

	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting SimpleBinsHistCanvas::DisplayRightClickMenu");
}

/** Override of TemplateCanvas method. */
void SimpleBinsHistCanvas::update(HLStateInt* o)
{
    ResetBrushing();
	layer1_valid = false;
	Refresh();
}

wxString SimpleBinsHistCanvas::GetCanvasTitle()
{
	wxString s("Histogram");	
	s << " - " << Xname;
	return s;
}

void SimpleBinsHistCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In SimpleBinsHistCanvas::TimeSyncVariableToggle");
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void SimpleBinsHistCanvas::FixedScaleVariableToggle(int var_index)
{
	LOG_MSG("In SimpleBinsHistCanvas::FixedScaleVariableToggle");
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void SimpleBinsHistCanvas::ShowAxes(bool show_axes_s)
{
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void SimpleBinsHistCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	double num_ivals_d = (double) hist_bins.size();
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const
	+ interval_width_const * num_ivals_d + 
	+ interval_gap_const * (num_ivals_d-1);
	
	// orig_x_pos is the center of each histogram bar
	std::vector<double> orig_x_pos(hist_bins.size());
	for (size_t i=0; i<hist_bins.size(); ++i) {
		orig_x_pos[i] = left_pad_const + interval_width_const/2.0
		+ i * (interval_width_const + interval_gap_const);
	}
	
	double y_max = hist_bins_max;
    
    last_scale_trans.SetData(x_min, 0, x_max, y_max);
	if (show_axes) {
		axis_scale_y = AxisScale(0, y_max, 5, axis_display_precision);
		y_max = axis_scale_y.scale_max;
		y_axis = new GdaAxis("Frequency", axis_scale_y,
							 wxRealPoint(0,0), wxRealPoint(0, y_max),
							 -9, 0);
		foreground_shps.push_back(y_axis);
		
		axis_scale_x = AxisScale(0, Xmax, 5, axis_display_precision);
		//shps_orig_xmax = axis_scale_x.scale_max;
		axis_scale_x.data_min = Xmin;
		axis_scale_x.data_max = Xmax;
		axis_scale_x.scale_min = axis_scale_x.data_min;
		axis_scale_x.scale_max = axis_scale_x.data_max;
		double range = axis_scale_x.scale_max - axis_scale_x.scale_min;
        
		axis_scale_x.scale_range = range;
		axis_scale_x.p = floor(log10(range));
		axis_scale_x.ticks = hist_bins.size()+1;
		axis_scale_x.tics.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str.resize(axis_scale_x.ticks);
		axis_scale_x.tics_str_show.resize(axis_scale_x.tics_str.size());
		for (int i=0; i<axis_scale_x.ticks; i++) {
			axis_scale_x.tics[i] =
			axis_scale_x.data_min +
			range*((double) i)/((double) axis_scale_x.ticks-1);
			std::ostringstream ss;
			ss << std::fixed << std::setprecision(3) << axis_scale_x.tics[i];
			axis_scale_x.tics_str[i] = ss.str();
			axis_scale_x.tics_str_show[i] = false;
		}
		int tick_freq = ceil(num_ivals_d/10.0);
		for (int i=0; i<axis_scale_x.ticks; i++) {
			if (i % tick_freq == 0) {
				axis_scale_x.tics_str_show[i] = true;
			}
		}
		axis_scale_x.tic_inc = axis_scale_x.tics[1]-axis_scale_x.tics[0];
		x_axis = new GdaAxis(Xname, axis_scale_x, wxRealPoint(0,0),
							 wxRealPoint(x_max, 0), 0, 9);
		foreground_shps.push_back(x_axis);
	}
	
	GdaShape* s = 0;
	int table_w=0, table_h=0;
	
	int virtual_screen_marg_top = 5; //25;
	int virtual_screen_marg_bottom = 5; //25;
	int virtual_screen_marg_left = 5; //25;
	int virtual_screen_marg_right = 5; //25;
	
	if (show_axes) {
		virtual_screen_marg_bottom += 32;
		virtual_screen_marg_left += 35;
	}
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);
	
	selectable_shps.resize(hist_bins.size());
	for (size_t i=0; i<hist_bins.size(); ++i) {
		double x0 = orig_x_pos[i] - interval_width_const/2.0;
		double x1 = orig_x_pos[i] + interval_width_const/2.0;
		double y0 = 0;
		double y1 = hist_bins[i].count;
		selectable_shps[i] = new GdaRectangle(wxRealPoint(x0, 0),
											  wxRealPoint(x1, y1));
		int sz = GdaConst::qualitative_colors.size();
		selectable_shps[i]->setPen(GdaConst::qualitative_colors[i%sz]);
		selectable_shps[i]->setBrush(GdaConst::qualitative_colors[i%sz]);
	}
	
	ResizeSelectableShps();
}

void SimpleBinsHistCanvas::UpdateStatusBar()
{
	if (sbh_canv_cb) {
		sbh_canv_cb->notifyNewHistHover(hover_obs, total_hover_obs);
	}
}

