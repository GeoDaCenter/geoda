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

#ifndef __GEODA_CENTER_CONDITIONAL_SCATTER_PLOT_VIEW_H__
#define __GEODA_CENTER_CONDITIONAL_SCATTER_PLOT_VIEW_H__

#include "LowessParamDlg.h"
#include "LowessParamObserver.h"
#include "../ShapeOperations/Lowess.h"
#include "../ShapeOperations/SmoothingUtils.h"
#include "../GenUtils.h"
#include "ConditionalNewView.h"

class ConditionalScatterPlotFrame;
class ConditionalScatterPlotCanvas;

typedef boost::multi_array<SampleStatistics, 2> stats_array_type;
typedef boost::multi_array<SimpleLinearRegression, 2> slr_array_type;
typedef boost::multi_array<GdaPolyLine, 2> polyline_array_type;
typedef boost::multi_array<GdaSpline, 2> spline_array_type;

class ConditionalScatterPlotCanvas : public ConditionalNewCanvas {
	DECLARE_CLASS(ConditionalScatterPlotCanvas)
public:
	
	ConditionalScatterPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
						 Project* project,
						 const std::vector<GdaVarTools::VarInfo>& var_info,
						 const std::vector<int>& col_ids,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize);
	virtual ~ConditionalScatterPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
	
	virtual void SetCheckMarks(wxMenu* menu);

	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
	
protected:
	virtual void PopulateCanvas();
	virtual void CalcCellsRegression();
	
public:
	virtual void TimeSyncVariableToggle(int var_index);
	bool IsShowLinearSmoother();
	bool IsShowLowessSmoother();
	void ShowLinearSmoother(bool display);
	void ShowLowessSmoother(bool display);
	void ChangeLoessParams(double f, int iter, double delta_factor);
	void DisplayAxesScaleValues(bool display);
	void DisplaySlopeValues(bool display);
	bool IsDisplayAxesScaleValues();
	bool IsDisplaySlopeValues();
	Lowess GetLowess() { return lowess; }
	/// Override from TemplateCanvas
	virtual void SetSelectableFillColor(wxColour color);
	/// Override from TemplateCanvas
	virtual void SetSelectableOutlineColor(wxColour color);
	
protected:
	bool full_map_redraw_needed;
	std::vector<double> X;
	std::vector<double> Y;
    std::vector<bool> XY_undef;
	
	static const int IND_VAR; // scatter plot x-axis
	static const int DEP_VAR; // scatter plot y-axis

	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	
	stats_array_type stats_x;
	stats_array_type stats_y;
	slr_array_type regression;
	polyline_array_type reg_line;
	spline_array_type reg_line_lowess;
	
	bool show_linear_smoother;
	bool show_lowess_smoother;
	bool display_axes_scale_values;
	bool display_slope_values;
	
	SmoothingUtils::LowessCacheType lowess_cache;
	void EmptyLowessCache();
	Lowess lowess;
	
	virtual void UpdateStatusBar();
	
	DECLARE_EVENT_TABLE()
};


class ConditionalScatterPlotFrame : public ConditionalNewFrame,
	public LowessParamObserver
{
	DECLARE_CLASS(ConditionalScatterPlotFrame)
public:
    ConditionalScatterPlotFrame(wxFrame *parent, Project* project,
						const std::vector<GdaVarTools::VarInfo>& var_info,
						const std::vector<int>& col_ids,
						const wxString& title = _("Conditional Map"),
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalScatterPlotFrame();
	
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	/** Implementation of LowessParamObserver interface */
	virtual void update(LowessParamObservable* o);
	virtual void notifyOfClosing(LowessParamObservable* o);
	
	void OnViewLinearSmoother(wxCommandEvent& event);
	void OnViewLowessSmoother(wxCommandEvent& event);
	void OnEditLowessParams(wxCommandEvent& event);
	void OnDisplayAxesScaleValues(wxCommandEvent& event);
	void OnDisplaySlopeValues(wxCommandEvent& event);
	
protected:
		LowessParamFrame* lowess_param_frame;
	
    DECLARE_EVENT_TABLE()
};

#endif
