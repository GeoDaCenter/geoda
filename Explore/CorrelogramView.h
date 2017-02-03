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

#ifndef __GEODA_CENTER_CORRELOGRAM_VIEW_H__
#define __GEODA_CENTER_CORRELOGRAM_VIEW_H__

#include <map>
#include <vector>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/html/htmlwin.h>
#include "CorrelParamsDlg.h"
#include "CorrelParamsObserver.h"
#include "SimpleScatterPlotCanvas.h"
#include "SimpleBinsHistCanvas.h"
#include "../ShapeOperations/Lowess.h"
#include "../ShapeOperations/SmoothingUtils.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"
#include "CorrelogramAlgs.h"

class SimpleHistStatsCanvas;
class HighlightState;
class SimpleAxisCanvas;
class CorrelogramFrame;
class Project;
typedef std::vector<double> vec_dbl_type;
typedef std::vector<vec_dbl_type> vec_vec_dbl_type;
typedef std::map<wxString, vec_vec_dbl_type> data_map_type;

typedef std::vector<std::vector<bool> > vec_vec_bool_type;
typedef std::map<wxString, vec_vec_bool_type> data_undef_map_type;

/**
 CorrelogramFrame manages all of its canvas child windows.
 */
class CorrelogramFrame : public TemplateFrame, public CorrelParamsObserver,
public SimpleScatterPlotCanvasCbInt, public SimpleBinsHistCanvasCbInt
{
public:
    CorrelogramFrame(wxFrame *parent, Project* project,
                     const wxString& title = _("Scatter Plot Matrix"),
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);
	virtual ~CorrelogramFrame();
	
	void OnMouseEvent(wxMouseEvent& event);
	virtual void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);
		
	void OnShowCorrelParams(wxCommandEvent& event);
	void OnDisplayStatistics(wxCommandEvent& event);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	/** Implementation of CorrelParamsObserver interface */
	virtual void update(CorrelParamsObservable* o);
	virtual void notifyOfClosing(CorrelParamsObservable* o);
	
	/** Implementation of SimpleScatterPlotCanvasCbInt interface */	
	virtual void notifyNewHover(const std::vector<int>& hover_obs,
                                int total_hover_obs);
	
	/** Implementation of SimpleScatterPlotCanvasCbInt interface */	
	virtual void notifyNewHistHover(const std::vector<int>& hover_obs,
                                    int total_hover_obs);
	
    virtual void OnRightClick(const wxPoint& pos);
    
    void OnSaveResult(wxCommandEvent& event);
    
    
protected:
    void ReDraw();
	void SetupPanelForNumVariables(int num_vars);
	void UpdateMessageWin();
	void UpdateDataMapFromVarMan();
	bool UpdateCorrelogramData();
    double GetEstDistWithZeroAutocorr(double& rng_left, double& rng_right);
	
    Project* project;
	CorrelParamsFrame* correl_params_frame;
	CorrelParams par;
	GdaVarTools::Manager var_man;
	data_map_type data_map;
	data_undef_map_type data_undef_map;
	std::vector<CorrelogramAlgs::CorreloBin> cbins;
	std::vector<SimpleScatterPlotCanvas*> scatt_plots;
	std::vector<SimpleAxisCanvas*> vert_labels;
	std::vector<SimpleAxisCanvas*> horiz_labels;
	SimpleBinsHistCanvas* hist_plot;
    SimpleHistStatsCanvas* shs_plot;
	HLStateInt* local_hl_state;
	
	wxBoxSizer* top_h_sizer;
	wxPanel* panel;
	wxBoxSizer* panel_v_szr;
	wxGridBagSizer* bag_szr;
	wxHtmlWindow* message_win;
	//wxWindow* message_win;
	
	DECLARE_EVENT_TABLE()
};


#endif
