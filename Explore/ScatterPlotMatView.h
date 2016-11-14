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

#ifndef __GEODA_CENTER_SCATTER_PLOT_MAT_VIEW_H__
#define __GEODA_CENTER_SCATTER_PLOT_MAT_VIEW_H__

#include <map>
#include <vector>
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
class SimpleAxisCanvas;
class SimpleHistCanvas;
class SimpleScatterPlotCanvas;
class ScatterPlotMatFrame;
class Project;
typedef std::vector<double> vec_dbl_type;
typedef std::vector<vec_dbl_type> vec_vec_dbl_type;
typedef std::map<wxString, vec_vec_dbl_type> data_map_type;

typedef std::vector<std::vector<bool> > vec_vec_bool_type;
typedef std::map<wxString ,vec_vec_bool_type> data_undef_map_type;

/**
 ScatterPlotMatFrame manages all of the cells.  Its state depends
 on the number of variables currently set.
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
class ScatterPlotMatFrame : public TemplateFrame, public LowessParamObserver, public VarsChooserObserver
{
public:
	ScatterPlotMatFrame(wxFrame *parent, Project* project,
                        const wxString& title = _("Scatter Plot Matrix"),
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
	virtual ~ScatterPlotMatFrame();
	
	void OnMouseEvent(wxMouseEvent& event);
	virtual void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);

    void OnSelectableOutlineColor(wxCommandEvent& event);
    void OnSelectableFillColor(wxCommandEvent& event);
    void OnHighlightColor(wxCommandEvent& event);
    
    void OnSelectWithRect(wxCommandEvent& event);
    void OnSelectWithCircle(wxCommandEvent& event);
    void OnSelectWithLine(wxCommandEvent& event);
    
    void OnViewStandardizedData(wxCommandEvent& event);
    void OnViewOriginalData(wxCommandEvent& event);
    
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
	
	/** Implementation of VarsChooserObserver interface */
	virtual void update(VarsChooserObservable* o);
	virtual void notifyOfClosing(VarsChooserObservable* o);

    virtual void OnSetDisplayPrecision(wxCommandEvent& event);
    
	void GetVizInfo(vector<wxString>& vars);
	
protected:
	void SetupPanelForNumVariables(int num_vars);
	void UpdateMessageWin();
	void UpdateDataMapFromVarMan();
	wxString GetHelpHtml();

    int axis_display_precision;
	LowessParamFrame* lowess_param_frame;
	VarsChooserFrame* vars_chooser_frame;
	GdaVarTools::Manager var_man;
	data_map_type data_map;
	data_undef_map_type data_undef_map;
	std::vector<SimpleScatterPlotCanvas*> scatt_plots;
	std::vector<SimpleAxisCanvas*> vert_labels;
	std::vector<SimpleAxisCanvas*> horiz_labels;
	std::vector<SimpleHistCanvas*> hist_plots;
	
	wxBoxSizer* top_h_sizer;
	wxPanel* panel;
	wxBoxSizer* panel_v_szr;
	//wxFlexGridSizer* fg_szr;
	wxGridBagSizer* bag_szr;
	//wxWebView* message_win;
	wxHtmlWindow* message_win;

    bool view_standardized_data;
	bool show_outside_titles;
	bool show_regimes;
	bool show_linear_smoother;
	bool show_lowess_smoother;
	bool show_slope_values;
    
    bool brush_rectangle;
    bool brush_circle;
    bool brush_line;
    
    wxColour selectable_outline_color;
    wxColour selectable_fill_color;
    wxColour highlight_color;
	
	DECLARE_EVENT_TABLE()
};


#endif
