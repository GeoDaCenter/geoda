/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_CONNECTIVITY_HIST_VIEW_H__
#define __GEODA_CENTER_CONNECTIVITY_HIST_VIEW_H__

#include <vector>
#include <wx/menu.h>
#include "CatClassification.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "../Generic/GdaShape.h"

class GalWeight;
class ConnectivityHistCanvas;
class ConnectivityHistFrame;

class ConnectivityHistCanvas : public TemplateCanvas {
	DECLARE_CLASS(ConnectivityHistCanvas)	
public:
	ConnectivityHistCanvas(wxWindow *parent, TemplateFrame* t_frame,
						   Project* project, GalWeight* gal,
						   const wxPoint& pos = wxDefaultPosition,
						   const wxSize& size = wxDefaultSize);
	virtual ~ConnectivityHistCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void update(HighlightState* o);
	virtual wxString GetCanvasTitle();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void DetermineMouseHoverObjects();
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
protected:
	virtual void PopulateCanvas();
	
public:	
	void DisplayStatistics(bool display_stats);
	void ShowAxes(bool show_axes);
	
	bool IsDisplayStats() { return display_stats; }
	bool IsShowAxes() { return show_axes; }
	
	bool HasIsolates() { return has_isolates; }
	void SelectIsolates();
	void SaveConnectivityToTable();
	void HistogramIntervals();
	void InitIntervals();
	void UpdateIvalSelCnts();
protected:
	virtual void UpdateStatusBar();

	Project* project;
	HighlightState* highlight_state;
	int num_obs;
	wxString weights_name;
	bool has_isolates;
	int num_isolates;
	std::vector<int> connectivity;
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
	int cur_intervals;
	std::vector<int> ival_obs_cnt; // size = cur_num_intervals
	std::vector<int> ival_obs_sel_cnt;  // size = cur_num_intervals
	std::vector<int> obs_id_to_ival; // size = num_obs
	std::vector<std::list<int> > ival_to_obs_ids;
	
	int max_intervals; // min of num_obs and MAX_INTERVALS
	static const int MAX_INTERVALS;
	int default_intervals;
	static const double left_pad_const;
	static const double right_pad_const;
	static const double interval_width_const;
	static const double interval_gap_const;
	
	DECLARE_EVENT_TABLE()
};


class ConnectivityHistFrame : public TemplateFrame {
    DECLARE_CLASS(ConnectivityHistFrame)
public:
    ConnectivityHistFrame(wxFrame *parent, Project* project, GalWeight* gal,
						  const wxString& title = "Connectivity Histogram",
						  const wxPoint& pos = wxDefaultPosition,
						  const wxSize& size = GdaConst::hist_default_size,
						  const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConnectivityHistFrame();
	
public:
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	void OnShowAxes(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
	void OnHistogramIntervals(wxCommandEvent& event);
	void OnSaveConnectivityToTable(wxCommandEvent& event);
	void OnSelectIsolates(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};


#endif