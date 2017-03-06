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

#ifndef __GEODA_CENTER_COV_SP_VIEW_H__
#define __GEODA_CENTER_COV_SP_VIEW_H__

#include <map>
#include <vector>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/html/htmlwin.h>
#include "DistancesCalc.h"
#include "LowessParamDlg.h"
#include "LowessParamObserver.h"
#include "../ShapeOperations/Lowess.h"
#include "../ShapeOperations/SmoothingUtils.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"
#include "../VarCalc/WeightsMetaInfo.h"

class SimpleAxisCanvas;
class SimpleHistCanvas;
class SimpleScatterPlotCanvas;
class CovSpFrame;
class CovSpHLStateProxy;
class Project;
typedef std::vector<double> vec_dbl_type;
typedef std::vector<vec_dbl_type> vec_vec_dbl_type;

typedef std::vector<bool> vec_bool_type;
typedef std::vector<vec_bool_type> vec_vec_bool_type;

/**
 Non-parametric spatial autocorrelation:
 This requires computing all the
 pairwise distances, creating a scatterplot of zi.zj on the y-axis
 and dij on the x-axis (with zi as a standardized value), and
 fitting a lowess curve through the scatterplot.
 Linking and brushing in this scatterplot would be a bit tricky,
 since is connects a value in the scatterplot to two values
 in every other plot, one corresponding to i and one to j.
 Similarly, selecting a given i in a plot would link to all the
 i-j pairs that i is involved in.
 
 CovSpFrame manages all of the cells.  Its state depends on the
 number of variables currently set.
 Zero variables: a message asking for two or more variables is shown
 One variable: a message showing the current variable and asking
 for one more is shown
 Two or more variables: scatter plot matrix is shown.
 Any of the following changes will result in full reinit of everything:
 - scaling options
 - color options
 - axis/etc options
 - variable changes (add/remove/rename/data-change/order-change)
 
 When this happens:
 - delete all existing cells.  should have a function
 to do this.
 - get variables from table
 - repopulate all cells
 
 Note: we might want real-time regimes in the future.  In this case,
 we certainly can't afford to redraw everything.  Will just want to
 redraw overlay, but not selectable shapes.
 
 Question: do I draw a bunch of individual canvases tiled together
 or do I have one canvas and manage drawing seperately?  Might
 need a custom highlight state
 
 */
class CovSpFrame : public TemplateFrame, public LowessParamObserver
{
public:
	CovSpFrame(wxFrame *parent, Project* project,
						 const GdaVarTools::Manager& var_man,
						 WeightsMetaInfo::DistanceMetricEnum dist_metric,
						 WeightsMetaInfo::DistanceUnitsEnum dist_units,
						 const wxString& title = _("Nonparametric Spatial Autocorrelation"),
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = GdaConst::scatterplot_default_size);
	virtual ~CovSpFrame();
	
	void OnMouseEvent(wxMouseEvent& event);
	virtual void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);
	virtual void UpdateTitle();
    virtual wxString GetUpdateStatusBarString(const std::vector<int>& hover_obs,
                                              int total_hover_obs);
	
	void OnViewLinearSmoother(wxCommandEvent& event);
	void OnViewLowessSmoother(wxCommandEvent& event);
	void OnEditLowessParams(wxCommandEvent& event);
	void OnShowVarsChooser(wxCommandEvent& event);
	void OnViewRegimesRegression(wxCommandEvent& event);
	void OnDisplayStatistics(wxCommandEvent& event);
	void OnDisplaySlopeValues(wxCommandEvent& event);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	/** Implementation of LowessParamObserver interface */
	virtual void update(LowessParamObservable* o);
	virtual void notifyOfClosing(LowessParamObservable* o);
	
protected:
	void UpdatePanel();
	void UpdateMessageWin();
	void UpdateDataFromVarMan();
	wxString GetHelpHtml();
	
	LowessParamFrame* lowess_param_frame;
	GdaVarTools::Manager var_man;
	vec_vec_dbl_type Z; // size tms*n
	vec_vec_bool_type Z_undef;
	vec_vec_dbl_type Zprod; // size tms*n*(n-1)/2
    vec_vec_bool_type Zprod_undef; // size tms * n * (n-1)/2
    
	std::vector<wxString> Z_error_msg; // size tms
	std::vector<double> Zprod_min;
	std::vector<double> Zprod_max;
	std::vector<double> D; // size n*(n-1)/2
	double D_min;
	double D_max;
	std::vector<double> MeanZ;
	std::vector<double> VarZ;
	SimpleScatterPlotCanvas* scatt_plot;
	SimpleAxisCanvas* vert_label;
	SimpleAxisCanvas* horiz_label;
	
	wxBoxSizer* top_h_sizer;
	wxPanel* panel;
	wxBoxSizer* panel_v_szr;
	//wxFlexGridSizer* fg_szr;
	wxGridBagSizer* bag_szr;
	//wxWebView* message_win;
	wxHtmlWindow* message_win;
	
	bool show_outside_titles;
	bool show_regimes;
	bool show_linear_smoother;
	bool show_lowess_smoother;
	bool show_slope_values;
	
	CovSpHLStateProxy* pairs_hl_state;
	
	WeightsMetaInfo::DistanceMetricEnum dist_metric;
	WeightsMetaInfo::DistanceUnitsEnum dist_units;
	
	bool too_many_obs;
	
	DECLARE_EVENT_TABLE()
};


#endif
