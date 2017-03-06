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

#ifndef __GEODA_CENTER_SIMPLE_AXIS_CANVAS_VIEW_H__
#define __GEODA_CENTER_SIMPLE_AXIS_CANVAS_VIEW_H__

#include <vector>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GdaShape.h"

class Project;

class SimpleAxisCanvas : public TemplateCanvas
{
	DECLARE_CLASS(SimpleAxisCanvas)
    SimpleAxisCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project,
                     HLStateInt* hl_state_int,
                     const std::vector<double>& X,
                     const wxString& Xname,
                     double Xmin, double Xmax,
                     bool horizontal_orientation, // if false then vert
                     bool show_axes = true,
                     bool hide_negative_labels = false,
                     bool add_auto_padding_min = true,
                     bool add_auto_padding_max = true,
                     int number_ticks = -1, // -1 for default
                     bool force_tick_at_min = false,
                     bool force_tick_at_max = false,
                     AxisScale* custom_axis_scale = 0, // overrides many params
                     bool is_standardized = false,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);
    
	SimpleAxisCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project,
					 HLStateInt* hl_state_int,
					 const std::vector<double>& X,
					 const std::vector<bool>& X_undefs,
					 const wxString& Xname,
					 double Xmin, double Xmax,
					 bool horizontal_orientation, // if false then vert
					 bool show_axes = true,
					 bool hide_negative_labels = false,
					 bool add_auto_padding_min = true,
					 bool add_auto_padding_max = true,
					 int number_ticks = -1, // -1 for default
					 bool force_tick_at_min = false,
					 bool force_tick_at_max = false,
					 AxisScale* custom_axis_scale = 0, // overrides many params
                     bool is_standardized = false,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize);
	virtual ~SimpleAxisCanvas();
	
	virtual void UpdateStatusBar();
	void ShowAxes(bool display);
	bool IsShowAxes() { return show_axes; }
    
    void ViewStandardizedData(bool display);
	
protected:
	virtual void PopulateCanvas();
	void UpdateMargins();
	
    bool is_standardized;
	bool horiz_orient;
	std::vector<double> X;
	std::vector<bool> X_undefs;
	wxString Xname;
	double Xmin, Xmax;
	bool show_axes;
	bool hide_negative_labels;
	bool add_auto_padding_min;
	bool add_auto_padding_max;
	int number_ticks;
	bool force_tick_at_min;
	bool force_tick_at_max;
	AxisScale* custom_axis_scale; // use if present
	
	AxisScale axis_scale_x;
	double scaleX;
	GdaAxis* x_baseline;
	SampleStatistics statsX;
	
	DECLARE_EVENT_TABLE()
};

#endif
