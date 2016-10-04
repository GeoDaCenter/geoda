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

#ifndef __GEODA_CENTER_SIMPLE_HIST_CANVAS_VIEW_H__
#define __GEODA_CENTER_SIMPLE_HIST_CANVAS_VIEW_H__

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

class SimpleHistCanvas : public TemplateCanvas
{
public:
	DECLARE_CLASS(SimpleHistCanvas)
    SimpleHistCanvas(wxWindow *parent, TemplateFrame* t_frame,
                     Project* project, HLStateInt* hl_state_int,
                     const std::vector<double>& X,
                     const std::vector<bool>& X_undef,
                     const wxString& Xname, double Xmin, double Xmax,
                     bool show_axes = false,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);
	virtual ~SimpleHistCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
	
	virtual void UpdateSelection(bool shiftdown = false,
                                 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
	void DisplayStatistics(bool display_stats);
	void ShowAxes(bool show_axes);
	bool IsDisplayStats() { return display_stats; }
	bool IsShowAxes() { return show_axes; }
	
	void HistogramIntervals();
	void InitIntervals();
	void UpdateIvalSelCnts();
protected:
	virtual void PopulateCanvas();
	virtual void UpdateStatusBar();
	
	const std::vector<double>& X;
	const std::vector<bool>& X_undef;
	wxString Xname;
	// used for scaling, so can be smaller/larger than min/max in X
	double Xmin, Xmax; 
	Gda::dbl_int_pair_vec_type data_sorted;
	SampleStatistics data_stats;
	HingeStats hinge_stats;
	
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	GdaAxis* x_axis;
	GdaAxis* y_axis;
	
	bool show_axes;
	bool display_stats;
	
	std::vector<double> ival_breaks; // size = cur_num_intervals-1
	double min_ival_val;
	double max_ival_val;
	double max_num_obs_in_ival;
	double overall_max_num_obs_in_ival;
	int cur_intervals;
	std::vector<int> ival_obs_cnt; // size = cur_num_intervals
	std::vector<int> ival_obs_sel_cnt;  // size = cur_num_intervals
	std::vector<int> obs_id_to_ival; // size = num_obs
	std::vector<std::list<int> > ival_to_obs_ids;
	
	int max_intervals; // min of num_obs and MAX_INTERVALS
	static const int MAX_INTERVALS;
	static const int default_intervals;
	static const double left_pad_const;
	static const double right_pad_const;
	static const double interval_width_const;
	static const double interval_gap_const;
	
	DECLARE_EVENT_TABLE()
};

#endif
