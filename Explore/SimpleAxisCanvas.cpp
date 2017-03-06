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

#include <utility> // std::pair
#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "SimpleAxisCanvas.h"


IMPLEMENT_CLASS(SimpleAxisCanvas, TemplateCanvas)

BEGIN_EVENT_TABLE(SimpleAxisCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

SimpleAxisCanvas::SimpleAxisCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                   Project* project,
                                   HLStateInt* hl_state_int,
                                   const std::vector<double>& X_,
                                   const wxString& Xname_,
                                   double Xmin_, double Xmax_,
                                   bool horiz_orient_,
                                   bool show_axes_,
                                   bool hide_negative_labels_,
                                   bool add_auto_padding_min_,
                                   bool add_auto_padding_max_,
                                   int number_ticks_,
                                   bool force_tick_at_min_,
                                   bool force_tick_at_max_,
                                   AxisScale* custom_axis_scale_,
                                   bool is_standardized_,
                                   const wxPoint& pos,
                                   const wxSize& size)
: TemplateCanvas(parent, t_frame, project, hl_state_int,
                 pos, size, false, true),
horiz_orient(horiz_orient_), show_axes(show_axes_),
hide_negative_labels(hide_negative_labels_),
add_auto_padding_min(add_auto_padding_min_),
add_auto_padding_max(add_auto_padding_max_),
number_ticks(number_ticks_),
force_tick_at_min(force_tick_at_min_),
force_tick_at_max(force_tick_at_max_),
custom_axis_scale(custom_axis_scale_),
X(X_), Xname(Xname_), Xmin(Xmin_), Xmax(Xmax_),
is_standardized(is_standardized_)
{
    
    axis_display_precision = 1;
    last_scale_trans.SetData(0, 0, 100, 100);
    last_scale_trans.SetFixedAspectRatio(false);
	UpdateMargins();
	
	use_category_brushes = false;
	
	PopulateCanvas();
	ResizeSelectableShps();
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}
SimpleAxisCanvas::SimpleAxisCanvas(wxWindow *parent, TemplateFrame* t_frame,
								 Project* project,
								 HLStateInt* hl_state_int,
								 const std::vector<double>& X_,
								 const std::vector<bool>& X_undefs_,
								 const wxString& Xname_,
								 double Xmin_, double Xmax_,
								 bool horiz_orient_,
								 bool show_axes_,
								 bool hide_negative_labels_,
								 bool add_auto_padding_min_,
								 bool add_auto_padding_max_,
								 int number_ticks_,
								 bool force_tick_at_min_,
								 bool force_tick_at_max_,
								 AxisScale* custom_axis_scale_,
                                 bool is_standardized_,
								 const wxPoint& pos,
								 const wxSize& size)
: TemplateCanvas(parent, t_frame, project, hl_state_int,
								 pos, size, false, true),
horiz_orient(horiz_orient_), show_axes(show_axes_),
hide_negative_labels(hide_negative_labels_),
add_auto_padding_min(add_auto_padding_min_),
add_auto_padding_max(add_auto_padding_max_),
number_ticks(number_ticks_),
force_tick_at_min(force_tick_at_min_),
force_tick_at_max(force_tick_at_max_),
custom_axis_scale(custom_axis_scale_),
X(X_), X_undefs(X_undefs_), Xname(Xname_), Xmin(Xmin_), Xmax(Xmax_),
is_standardized(is_standardized_)
{
	LOG_MSG("Entering SimpleAxisCanvas::SimpleAxisCanvas");
	
    axis_display_precision = 1;
    last_scale_trans.SetData(0, 0, 100, 100);
    last_scale_trans.SetFixedAspectRatio(false);
	UpdateMargins();
	
	use_category_brushes = false;
	
	PopulateCanvas();
	ResizeSelectableShps();
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting SimpleAxisCanvas::SimpleAxisCanvas");
}

SimpleAxisCanvas::~SimpleAxisCanvas()
{
	LOG_MSG("In SimpleAxisCanvas::SimpleAxisCanvas");
}

void SimpleAxisCanvas::UpdateStatusBar()
{
	
}

void SimpleAxisCanvas::ShowAxes(bool display)
{
	show_axes = display;
	UpdateMargins();
	PopulateCanvas();
}

void SimpleAxisCanvas::ViewStandardizedData(bool display)
{
    is_standardized = display;
    PopulateCanvas();
}

void SimpleAxisCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	wxSize size(GetVirtualSize());
	int win_width = size.GetWidth();
	int win_height = size.GetHeight();
    
    last_scale_trans.SetView(win_width, win_height);
    
	// Recall: Xmin/max can be smaller/larger than min/max in X
	//    if X are particular time-slices of time-variant variables and
	//    if global scaling is being used.
	double x_min = Xmin;
	double x_max = Xmax;

    if (X_undefs.empty())
        statsX = SampleStatistics(X);
    else
        statsX = SampleStatistics(X, X_undefs);
	
    if (is_standardized) {
        for (int i=0, iend=X.size(); i<iend; i++) {
            X[i] = (X[i]-statsX.mean)/statsX.sd_with_bessel;
        }
		x_max = (statsX.max - statsX.mean)/statsX.sd_with_bessel;
		x_min = (statsX.min - statsX.mean)/statsX.sd_with_bessel;
        if (X_undefs.empty())
            statsX = SampleStatistics(X);
        else
            statsX = SampleStatistics(X, X_undefs);
		// mean shold be 0 and biased standard deviation should be 1
		double eps = 0.000001;
		if (-eps < statsX.mean && statsX.mean < eps)
            statsX.mean = 0;
    }
    
	if (custom_axis_scale) {
		axis_scale_x = *custom_axis_scale;
        
	} else {
		double x_pad = 0.1 * (x_max - x_min);
		axis_scale_x = AxisScale(x_min - (add_auto_padding_min ? x_pad : 0.0),
								 x_max + (add_auto_padding_max ? x_pad : 0.0),
								 (number_ticks < 0 ? 4 : number_ticks),
                                 axis_display_precision);
	}
	
    
	if (show_axes) {
    	// create axes
    	if (horiz_orient) {
    		x_baseline = new GdaAxis(Xname, axis_scale_x,
    								 wxRealPoint(0,0), wxRealPoint(100, 0));
    	} else {
    		x_baseline = new GdaAxis(Xname, axis_scale_x,
    								 wxRealPoint(0,0), wxRealPoint(0, 100));
    	}
    	x_baseline->autoDropScaleValues(true);
    	x_baseline->moveOuterValTextInwards(true);
    	x_baseline->hideNegativeLabels(hide_negative_labels);
		x_baseline->setPen(*GdaConst::scatterplot_scale_pen);
        foreground_shps.push_back(x_baseline);
    }
	
	ResizeSelectableShps();
	LOG_MSG("Exiting SimpleAxisCanvas::PopulateCanvas");
}

void SimpleAxisCanvas::UpdateMargins()
{
	int virtual_screen_marg_top = 5;//20;
	int virtual_screen_marg_right = 5;//20;
	int virtual_screen_marg_bottom = 5;//45;
	int virtual_screen_marg_left = 5;//45;
	if (show_axes) {
		if (horiz_orient) {
			virtual_screen_marg_bottom = 45;//45;
		} else {
			virtual_screen_marg_left = 45;//45;
		}
	}
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);
}

