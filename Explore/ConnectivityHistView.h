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

#ifndef __GEODA_CENTER_CONNECTIVITY_HIST_VIEW_H__
#define __GEODA_CENTER_CONNECTIVITY_HIST_VIEW_H__

#include <vector>
#include <wx/menu.h>
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "../GdaShape.h"
#include "../VarCalc/WeightsManInterface.h"

class ConnectivityHistCanvas;
class ConnectivityHistFrame;
class WeightsManState;

class ConnectivityHistCanvas : public TemplateCanvas {
	DECLARE_CLASS(ConnectivityHistCanvas)	
public:
	ConnectivityHistCanvas(wxWindow *parent, TemplateFrame* t_frame,
						   Project* project, boost::uuids::uuid w_uuid,
						   const wxPoint& pos = wxDefaultPosition,
						   const wxSize& size = wxDefaultSize);
	virtual ~ConnectivityHistCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void DetermineMouseHoverObjects(wxPoint pt);
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
protected:
	virtual void PopulateCanvas();
	
public:	
	void ChangeWeights(boost::uuids::uuid new_id);
	
	void DisplayStatistics(bool display_stats);
	void ShowAxes(bool show_axes);
	
	bool IsDisplayStats() { return display_stats; }
	bool IsShowAxes() { return show_axes; }
	
	bool HasIsolates() { return has_isolates; }
	void SelectIsolates();
	void SaveConnectivityToTable();
	void HistogramIntervals();
	void InitData();
	void InitIntervals();
	void UpdateIvalSelCnts();
protected:
	virtual void UpdateStatusBar();

	int num_obs;
	bool has_isolates;
	int num_isolates;
	std::vector<long> connectivity;
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
	
	WeightsManInterface* w_man_int;
	boost::uuids::uuid w_uuid;
	
	DECLARE_EVENT_TABLE()
};


class ConnectivityHistFrame : public TemplateFrame, public WeightsManStateObserver
{
    DECLARE_CLASS(ConnectivityHistFrame)
public:
    ConnectivityHistFrame(wxFrame *parent, Project* project,
						  boost::uuids::uuid w_uuid,
						  const wxString& title = _("Connectivity Histogram"),
						  const wxPoint& pos = wxDefaultPosition,
						  const wxSize& size = GdaConst::hist_default_size,
						  const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConnectivityHistFrame();
	
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return id == w_uuid ? 1 : 0; }
	virtual void closeObserver(boost::uuids::uuid id);
	
	void ChangeWeights(boost::uuids::uuid new_id);
	void OnShowAxes(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
	void OnHistogramIntervals(wxCommandEvent& event);
	void OnSaveConnectivityToTable(wxCommandEvent& event);
	void OnSelectIsolates(wxCommandEvent& event);
	
private:
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;
	boost::uuids::uuid w_uuid;

    DECLARE_EVENT_TABLE()
};


#endif
