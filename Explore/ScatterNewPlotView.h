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

#ifndef __GEODA_CENTER_SCATTER_NEW_PLOT_VIEW_H__
#define __GEODA_CENTER_SCATTER_NEW_PLOT_VIEW_H__

#include <boost/multi_array.hpp>
#include <wx/menu.h>
#include "CatClassification.h"
#include "CatClassifStateObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../TemplateLegend.h"
#include "../GenUtils.h"
#include "../Generic/GdaShape.h"

class CatClassifState;
class ScatterNewPlotCanvas;
class ScatterNewPlotFrame;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<int, 2> i_array_type;

class ScatterNewPlotCanvas :
	public TemplateCanvas, public CatClassifStateObserver
{
	DECLARE_CLASS(ScatterNewPlotCanvas)	
public:
	ScatterNewPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
						 Project* project,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize);
	ScatterNewPlotCanvas(wxWindow *parent,  TemplateFrame* t_frame,
						 Project* project,
						 const std::vector<GeoDaVarInfo>& var_info,
						 const std::vector<int>& col_ids,
						 bool is_bubble_plot = false,
						 bool standardized = false,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize);
	virtual ~ScatterNewPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void update(HighlightState* o);
	virtual wxString GetCanvasTitle();
	virtual wxString GetCategoriesTitle();
	virtual wxString GetNameWithTime(int var);
	virtual void NewCustomCatClassif();
	void ChangeThemeType(CatClassification::CatClassifType new_theme,
						 int num_categories,
						 const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
	virtual void SetCheckMarks(wxMenu* menu);
	void OnSaveCategories();
	
	/// Override from TemplateCanvas
	virtual void SetHighlightColor(wxColour color);
	/// Override from TemplateCanvas
	virtual void SetSelectableFillColor(wxColour color);
	/// Override from TemplateCanvas
	virtual void SetSelectableOutlineColor(wxColour color);
	
protected:
	virtual void PopulateCanvas();
	virtual void PopCanvPreResizeShpsHook();
	virtual void TimeChange();
	void VarInfoAttributeChange();
	void CreateAndUpdateCategories();
	
public:
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
	CatClassifDef cat_classif_def;
	CatClassification::CatClassifType GetCcType();
	int GetNumCats() { return num_categories; } // used by Bubble Plot
	
	void ViewStandardizedData();
	void ViewOriginalData();
	void ViewRegressionSelected(bool display);
	void ViewRegressionSelectedExcluded(bool display);
	void DisplayStatistics(bool display_stats);
	void ShowAxesThroughOrigin(bool show_origin_axes);
	
	bool IsStandardized() { return standardized; }
	bool IsDisplayStats() { return display_stats; }
	bool IsShowOriginAxes() { return show_origin_axes; }
	bool IsRegressionSelected() { return show_reg_selected; } 
	bool IsRegressionExcluded() { return show_reg_excluded; }
	
protected:
	void CalcStatsFromSelected();
	void CalcVarSdFromSumSquares(SampleStatistics& ss, double sum_squares);
	void CalcRegressionSelOrExcl(const SampleStatistics& ss_X,
								 const SampleStatistics& ss_Y,
								 SimpleLinearRegression& r,
								 bool selected);
	void ComputeChowTest();
	void UpdateRegSelectedLine();
	void UpdateRegExcludedLine();
	static void CalcRegressionLine(GdaPolyLine& reg_line, // return value
								   double& slope, // return value
								   bool& infinite_slope, // return value
								   bool& regression_defined, // return value
								   wxRealPoint& a, // return value
								   wxRealPoint& b, // return value
								   double& cc_degs_of_rot, // return value
								   const AxisScale& axis_scale_x,
								   const AxisScale& axis_scale_y,
								   const SimpleLinearRegression& reg,
								   const wxPen& pen);
	void UpdateDisplayStats();
	void UpdateAxesThroughOrigin();
	static wxString CreateStatsString(const SampleStatistics& s);
	static double RegLineToDegCCFromHoriz(double a_x, double a_y,
										  double b_x, double b_y);
	wxString RegLineTextFromReg(const SimpleLinearRegression& reg, int line);

	virtual void UpdateStatusBar();

	bool is_bubble_plot;
	Project* project;
	HighlightState* highlight_state;
	CatClassifState* custom_classif_state;
	
	int num_obs;
	int num_time_vals;
	int num_categories;
	int ref_var_index;
	std::vector<GeoDaVarInfo> var_info;
	std::vector<d_array_type> data;
	d_array_type x_data;
	d_array_type y_data;
	d_array_type z_data;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> cats_valid;
	std::vector<wxString> cats_error_message;
	bool full_plot_redraw_needed;
	std::vector<double> X;
	std::vector<double> Y;
	std::vector<double> Z;
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;
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
	bool standardized;

	// variables for Chow test
	double sse_c; // error sum of squares constrained (combined subsets)
	double sse_sel; // err sum of sqrs unconstrained for selected
	double sse_unsel; // err sum of sqrs unconstrained for unselected
	// Note: sse_u (unconstrained) is the sum for sse_sel and sse_unsel
	double chow_ratio; // Chow test or f ratio
	double chow_pval; // significance of chow_ratio
	bool chow_valid;
	
	GdaPolyLine* reg_line;
	GdaShapeTable* stats_table;
	GdaShapeText* chow_test_text;
	
	bool show_reg_selected;
	GdaPolyLine* reg_line_selected;
	double reg_line_selected_slope;
	bool reg_line_selected_infinite_slope;
	bool reg_line_selected_defined;

	bool show_reg_excluded;
	GdaPolyLine* reg_line_excluded;
	double reg_line_excluded_slope;
	bool reg_line_excluded_infinite_slope;
	bool reg_line_excluded_defined;

	GdaPolyLine* x_axis_through_origin;
	GdaPolyLine* y_axis_through_origin;
	bool show_origin_axes;
	bool display_stats;

	// this is only used for Bubble Chart as a way to sort circles from
	// largest to smallest diameter.  This is a map from observation id
	// to z_val order (ranging from 0 to num_obs-1)
	i_array_type obs_id_to_z_val_order;
	
	// table_display_lines: 0 if no table shown, 1 if just blue line
	// 2 if blue and just green or red, 3 if blue, green and red shown.
	int table_display_lines;
	bool UpdateDisplayLinesAndMargins();
	bool all_init;
	
	DECLARE_EVENT_TABLE()
};

class ScatterNewPlotLegend : public TemplateLegend {
public:
	ScatterNewPlotLegend(wxWindow *parent, TemplateCanvas* template_canvas,
						 const wxPoint& pos, const wxSize& size);
	virtual ~ScatterNewPlotLegend();
};

class ScatterNewPlotFrame : public TemplateFrame {
    DECLARE_CLASS(ScatterNewPlotFrame)
public:
	ScatterNewPlotFrame(wxFrame *parent, Project* project,
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						const long style = wxDEFAULT_FRAME_STYLE);
    ScatterNewPlotFrame(wxFrame *parent, Project* project,
						const std::vector<GeoDaVarInfo>& var_info,
						const std::vector<int>& col_ids,
						bool is_bubble_plot,
						const wxString& title = "Scatter Plot",
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ScatterNewPlotFrame();

public:
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);

	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
    void OnViewStandardizedData(wxCommandEvent& event);
    void OnViewOriginalData(wxCommandEvent& event);
	void OnViewRegimesRegression(wxCommandEvent& event);
    void OnViewRegressionSelected(wxCommandEvent& event);
    void OnViewRegressionSelectedExcluded(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
    void OnShowAxesThroughOrigin(wxCommandEvent& event);

	virtual void OnNewCustomCatClassifA();
	virtual void OnCustomCatClassifA(const wxString& cc_title);
	
	virtual void OnThemeless();
	virtual void OnQuantile(int num_cats);
	virtual void OnPercentile();
	virtual void OnHinge15();
	virtual void OnHinge30();
	virtual void OnStdDevMap();
	virtual void OnUniqueValues();
	virtual void OnNaturalBreaks(int num_cats);
	virtual void OnEqualIntervals(int num_cats);
	virtual void OnSaveCategories();
	
protected:
	void ChangeThemeType(CatClassification::CatClassifType new_theme,
						 int num_categories,
						 const wxString& custom_classif_title = wxEmptyString);
	bool is_bubble_plot;
	
    DECLARE_EVENT_TABLE()
};


#endif
