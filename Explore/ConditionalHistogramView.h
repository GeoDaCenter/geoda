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

#ifndef __GEODA_CENTER_CONDITIONAL_HISTOGRAM_VIEW_H__
#define __GEODA_CENTER_CONDITIONAL_HISTOGRAM_VIEW_H__

#include <vector>
#include <map>
#include "../GenUtils.h"
#include "ConditionalNewView.h"

struct CondCellData;
class ConditionalHistogramFrame;
class ConditionalHistogramCanvas;

typedef boost::multi_array<SampleStatistics, 2> stats_array_type;
typedef boost::multi_array<GdaPolyLine, 2> polyline_array_type;

struct CondCellData {
	std::vector<int> ival_obs_cnt; // size = cur_intervals
	std::vector<int> ival_obs_sel_cnt;  // size = cur_intervals
	std::vector<std::list<int> > ival_to_obs_ids;
};

typedef boost::multi_array<CondCellData, 2> cond_cell_array_type;

class ConditionalHistogramCanvas : public ConditionalNewCanvas {
	DECLARE_CLASS(ConditionalHistogramCanvas)
public:
	ConditionalHistogramCanvas(wxWindow *parent, TemplateFrame* t_frame,
							   Project* project,
							   const std::vector<GdaVarTools::VarInfo>& var_info,
							   const std::vector<int>& col_ids,
							   const wxPoint& pos = wxDefaultPosition,
							   const wxSize& size = wxDefaultSize);
	virtual ~ConditionalHistogramCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	
	virtual void DetermineMouseHoverObjects(wxPoint pt);
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
	
protected:
	virtual void PopulateCanvas();
	
public:
	virtual void TimeSyncVariableToggle(int var_index);
	
	void ShowAxes(bool show_axes);
	bool IsShowAxes() { return show_axes; }

	void HistogramIntervals();
	void InitIntervals();
	void UpdateIvalSelCnts();
	virtual void UserChangedCellCategories();
protected:
	virtual void UpdateStatusBar();
	void sel_shp_to_cell_gen(int i, int& r, int& c, int& ival,
							 int cols, int ivals);
	void sel_shp_to_cell(int i, int& r, int& c, int& ival);
	int cell_to_sel_shp_gen(int r, int c, int ival,
							int cols, int ivals);
	
	bool full_map_redraw_needed;
	
	static const int HIST_VAR; // histogram variable
	
	// size = time_steps if HIST_VAR is time variant
	std::vector<Gda::dbl_int_pair_vec_type> data_sorted;
	std::vector<SampleStatistics> data_stats;
    std::vector<std::vector<bool> > undef_tms;
	
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	GdaAxis* x_axis;
	GdaAxis* y_axis;
	bool scale_x_over_time;
	bool scale_y_over_time;
	stats_array_type stats_x;
	stats_array_type stats_y;
	
	bool show_axes;
	
	double data_min_over_time;
	double data_max_over_time;
	d_array_type ival_breaks; // size = num_time_vals * cur_num_intervals-1
	std::vector<double> min_ival_val; // size = num_time_vals
	std::vector<double> max_ival_val; // size = num_time_vals
	std::vector<double> max_num_obs_in_ival; // size = num_time_vals
	double overall_max_num_obs_in_ival;	
	int cur_intervals;
	// a mapping from obs_id to selectable_shp id
	i_array_type obs_id_to_sel_shp; // size = num_time_vals * num_obs
	std::vector<cond_cell_array_type> cell_data;
	
	int max_intervals; // min of num_obs and MAX_INTERVALS
	static const int MAX_INTERVALS;
	static const int default_intervals;
	static const double left_pad_const;
	static const double right_pad_const;
	static const double interval_width_const;
	static const double interval_gap_const;
	
	DECLARE_EVENT_TABLE()
};


class ConditionalHistogramFrame : public ConditionalNewFrame {
	DECLARE_CLASS(ConditionalHistogramFrame)
public:
    ConditionalHistogramFrame(wxFrame *parent, Project* project,
							  const std::vector<GdaVarTools::VarInfo>& var_info,
							  const std::vector<int>& col_ids,
							  const wxString& title = _("Conditional Histogram"),
							  const wxPoint& pos = wxDefaultPosition,
							  const wxSize& size = wxDefaultSize,
							  const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalHistogramFrame();
	
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	void OnShowAxes(wxCommandEvent& event);
	void OnHistogramIntervals(wxCommandEvent& event);
	
    DECLARE_EVENT_TABLE()
};

#endif
