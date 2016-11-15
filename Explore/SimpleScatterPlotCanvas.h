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

#ifndef __GEODA_CENTER_SIMPLE_SCATTER_PLOT_CANVAS_VIEW_H__
#define __GEODA_CENTER_SIMPLE_SCATTER_PLOT_CANVAS_VIEW_H__

#include <map>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/html/htmlwin.h>
#include "VarsChooserDlg.h"
#include "VarsChooserObserver.h"
#include "LowessParamDlg.h"
#include "LowessParamObserver.h"
#include "../ShapeOperations/Lowess.h"
#include "../ShapeOperations/SmoothingUtils.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"

class HighlightState;
class Project;

class SimpleScatterPlotCanvasCbInt
{
public:
	virtual void notifyNewHover(const std::vector<int>& hover_obs,
															int total_hover_obs) = 0;
};

class SimpleScatterPlotCanvas : public TemplateCanvas
{
public:
	DECLARE_CLASS(SimpleScatterPlotCanvas)
	SimpleScatterPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
							Project* project,
							HLStateInt*	hl_state_int,
							SimpleScatterPlotCanvasCbInt* ssp_canv_cb, //optional
							const std::vector<double>& X,
							const std::vector<double>& Y,
                            const std::vector<bool>& X_undf,
                            const std::vector<bool>& Y_undef,
							const wxString& Xname,
							const wxString& Yname,
							double Xmin, double Xmax,
							double Ymin, double Ymax,
							bool add_auto_padding_min = true,
							bool add_auto_padding_max = true,
							bool use_larger_filled_circles = false,
							const wxString& right_click_menu_id = wxEmptyString,
							bool show_axes = false,
							bool show_horiz_axis_through_origin = false,
							bool show_vert_axis_through_origin = false,
							bool show_regimes = true,
							bool show_linear_smoother = true,
							bool show_lowess_smoother = false,
							bool show_slope_values = true,
							bool view_standardized_data = false,
							const wxPoint& pos = wxDefaultPosition,
							const wxSize& size = wxDefaultSize);
	virtual ~SimpleScatterPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	virtual void UpdateStatusBar();
    virtual void UpdateSelection(bool shiftdown, bool pointsel);
	
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);

    void SetSelectableOutlineColor(wxColour color);
    void SetSelectableFillColor(wxColour color);
    void SetHighlightColor(wxColour color);
    
	void ShowAxes(bool display);
	void ShowRegimes(bool display);
	void ViewStandardizedData(bool display);
	void ViewOriginalData(bool display);
	void ShowLinearSmoother(bool display);
	void ShowLowessSmoother(bool display);
	void ChangeLoessParams(double f, int iter, double delta_factor);
	void DisplayStatistics(bool display_stats);
	void ShowSlopeValues(bool display);
	
	bool IsShowAxes() { return show_axes; }
	bool IsShowRegimes() { return show_regimes; }
	bool IsShowLinearSmoother() { return show_linear_smoother; }
	bool IsShowLowessSmoother() { return show_lowess_smoother; }
	bool IsShowSlopeValues() { return show_slope_values; }
	
	void UpdateLinearRegimesRegLines();
	void UpdateLowessOnRegimes();
	
protected:
    
	virtual void PopulateCanvas();
	void UpdateMargins();
	
	ScatterPlotPens pens;
	
	std::vector<double> X;
	std::vector<double> Y;
	std::vector<bool> X_undef;
	std::vector<bool> Y_undef;
        
	const std::vector<double>& orgX;
	const std::vector<double>& orgY;
	wxString Xname;
	wxString Yname;
	// used for scaling, so can be smaller/larger than min/max in X/Y
	double Xmin, Xmax, Ymin, Ymax; 
	
	bool add_auto_padding_min;
	bool add_auto_padding_max;
	bool use_larger_filled_circles;
	
	// variables for Chow test
	double sse_c; // error sum of squares constrained (combined subsets)
	double sse_sel; // err sum of sqrs unconstrained for selected
	double sse_unsel; // err sum of sqrs unconstrained for unselected
	// Note: sse_u (unconstrained) is the sum for sse_sel and sse_unsel
	double chow_ratio; // Chow test or f ratio
	double chow_pval; // significance of chow_ratio
	bool chow_valid;	
	
	GdaPolyLine* reg_line;
	GdaSpline* lowess_reg_line;
	GdaShapeTable* stats_table;
	
	GdaPolyLine* reg_line_selected;
	GdaSpline* lowess_reg_line_selected;
	
	GdaPolyLine* reg_line_excluded;
	GdaSpline* lowess_reg_line_excluded;
	
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
	double scaleX;
	double scaleY;
	GdaAxis* x_baseline;
	GdaAxis* y_baseline;
	SampleStatistics statsX;
	SampleStatistics statsXselected;
	SampleStatistics statsXexcluded;
	SampleStatistics statsY;
	SampleStatistics statsYselected;
	SampleStatistics statsYexcluded;
	SampleStatistics statsZ;
	SimpleLinearRegression regressionXY;
	SimpleLinearRegression regressionXYselected;
	SimpleLinearRegression regressionXYexcluded;
	bool show_linear_smoother;
	bool show_lowess_smoother;
	bool show_regimes;
	bool show_axes;
	bool show_horiz_axis_through_origin;
	bool show_vert_axis_through_origin;
	bool show_slope_values;
    bool view_standardized_data;
    
	SmoothingUtils::LowessCacheType lowess_cache;
	void EmptyLowessCache();
	Lowess lowess;
	
	SimpleScatterPlotCanvasCbInt* ssp_canv_cb;
	wxString right_click_menu_id;
	
	DECLARE_EVENT_TABLE()
};

#endif
