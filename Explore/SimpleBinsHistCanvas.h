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

#ifndef __GEODA_CENTER_SIMPLE_BINS_HIST_CANVAS_VIEW_H__
#define __GEODA_CENTER_SIMPLE_BINS_HIST_CANVAS_VIEW_H__

#include <map>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/html/htmlwin.h>
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GdaShape.h"

class HighlightState;
class Project;

class SimpleBinsHistCanvasCbInt
{
public:
	virtual void notifyNewHistHover(const std::vector<int>& hover_obs,
																	int total_hover_obs) = 0;
};


struct SimpleBin {
	SimpleBin() : min(0), max(0), count(0) {}
	SimpleBin(double min_, double max_, int count_=0) :
	min(min_), max(max_), count(count_) {}
	double min; // >
	double max; // <=
	int count;
};

class SimpleBinsHistCanvas : public TemplateCanvas
{
public:
	DECLARE_CLASS(SimpleBinsHistCanvas)
    SimpleBinsHistCanvas(wxWindow *parent, TemplateFrame* t_frame,
                         Project* project, HLStateInt* hl_state_int,
                         SimpleBinsHistCanvasCbInt* sbh_canv_cb, //optional
                         const std::vector<SimpleBin>& hist_bins,
                         const wxString& Xname, double Xmin, double Xmax,
                         const wxString& right_click_menu_id = wxEmptyString,
                         bool show_axes = false,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize);
	virtual ~SimpleBinsHistCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
	
	void ShowAxes(bool show_axes);
	bool IsShowAxes() { return show_axes; }
	
protected:
	virtual void PopulateCanvas();
	virtual void UpdateStatusBar();
	
	std::vector<SimpleBin> hist_bins;
	wxString Xname;
	// used for scaling, so can be smaller/larger than min/max in X
	double Xmin, Xmax; 
	double hist_bins_max;
	
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	GdaAxis* x_axis;
	GdaAxis* y_axis;
	
	wxString right_click_menu_id;
	bool show_axes;
	
	static const int MAX_INTERVALS;
	static const double left_pad_const;
	static const double right_pad_const;
	static const double interval_width_const;
	static const double interval_gap_const;
	
	SimpleBinsHistCanvasCbInt* sbh_canv_cb;
	
	DECLARE_EVENT_TABLE()
};

#endif
