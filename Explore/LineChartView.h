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

#ifndef __GEODA_CENTER_LINE_CHART_VIEW_H__
#define __GEODA_CENTER_LINE_CHART_VIEW_H__

#include <map>
#include <vector>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/html/htmlwin.h>
#include <wx/radiobut.h>
#include <wx/webview.h>
#include "VarsChooserDlg.h"
#include "VarsChooserObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"
#include "../HighlightStateObserver.h"
#include "../Regression/DiagnosticReport.h"
#include "../DialogTools/RegressionReportDlg.h"
#include "LineChartStats.h"

class HighlightState;
class LineChartCanvas;
class Project;

typedef std::map<wxString, vec_vec_dbl_type> data_map_type;
typedef std::map<wxString, std::vector<bool> > data_map_undef_type;

/**
 LineChartFrame manages one or more LineChartCanvas instances: one canvas
 per variable selected.
 
 Difference-in means tests:
  1) comparison of means at same time period between groups (could be
		 only selected-unselected or also include all)
  2) comparison of means between groups for all time periods
     combined (again, could be only selected-unselected or also include all)
	3) comparison of means between groups for subsets of time periods
   (this one is a little more complicated since it requires specifying the
   subsets, which don't have to include all observations, e.g. for annual data
   from 2000-2010, it could be 2000-2004 = pre-intervention years, 2004-2006
   is a gap where the intervention occurred and 2006-2010 are
   post-intervention years)
 
 Two types of comparisons: selected vs unselected averages over a given
 time period or subset of time periods.
 
 Group 1 : HOVAL 1960-1980
 Group 2 : HOVAL 1990
 
 or
 
 Group 1: HOVAL selected 1970-2000
 Group 2: HOVAL unselected 1970-2000
 
 If only one time period, then only have regimes comparison
 
 If multiple time periods, then either compare regimes across one time period,
 or compare two ranges of time periods.

 Have two timeline selectors: one for regimes, and one 

 
 Compare Regimes
 Compare Time Periods
 
 Select: Time Period 1, Time Period 2
 
 Regimes: compare all if none selected.  Should indicate visually avg for all,
   or avg for time groups
 
 Avg times 1960-1970
 
 Avg times 1950, 2000-2010
 
 Need to visually indicate time period groups.  Perhaps two colors?
 
 In general, can specify multiple regimes using radio button banner.
 */

class LineChartFrame : public TemplateFrame, public VarsChooserObserver,
public HighlightStateObserver, public LineChartCanvasCallbackInt
{
public:
	LineChartFrame(wxFrame *parent, Project* project,
                 const wxString& title = _("Averages Chart"),
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxSize(680,420));
	virtual ~LineChartFrame();
	
	virtual void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);

	void OnCompareRegimes(wxCommandEvent& event);
	void OnCompareTimePeriods(wxCommandEvent& event);
	void OnCompareRegAndTmPer(wxCommandEvent& event);
	void OnSelectPeriod0(wxCommandEvent& event);
	void OnSelectPeriod1(wxCommandEvent& event);
	void OnDisplayStatistics(wxCommandEvent& event);
    void OnUseAdjustYAxis(wxCommandEvent& event);
    void OnAdjustYAxis(wxCommandEvent& event);
    void OnAdjustYAxisPrecision(wxCommandEvent& event);

    void OnSaveDummyTable(wxCommandEvent& event);
	void OnReportClose(wxWindowDestroyEvent& event);
    
    	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of VarsChooserObserver interface */
	virtual void update(VarsChooserObservable* o);
	virtual void notifyOfClosing(VarsChooserObservable* o);
	
	/** Implementation of HighlightStateObserver interface */
	virtual void update(HLStateInt* o);

	/** Implementation of LineChartCanvasCallbackInt interface */
	virtual void notifyNewSelection(const std::vector<bool>& tms_sel,
									bool shiftdown, bool pointsel);
	virtual void notifyNewHoverMsg(const wxString& msg);
	virtual void OnMouseEvent(wxMouseEvent& event);
	

    
protected:
    void SaveDataAndResults(bool save_weights, bool save_did=false,
                            double* m_yhat1=0, double* m_resid1=0);
	void SetupPanelForNumVariables(int num_vars);
	void UpdateMessageWin();
	void UpdateTitleWin();
	void UpdateTitleText();
	void UpdateDataMapFromVarMan();
	wxString GetHelpHtml();
	void UpdateStatsWinContent(int var);
    void printAndShowClassicalResults(const wxString& yName, double* y,
                                      const wxString& datasetname,
                                      const wxString& wname,
                                      DiagnosticReport *r,
                                      int Obs, int nX,
                                      bool do_white_test);
   
    std::vector<wxString> variable_names;
    wxChoice* choice_variable;
    wxChoice* choice_groups;
    wxChoice* choice_group1;
    wxChoice* choice_group2;
    wxChoice* choice_time1;
    wxChoice* choice_time2;
    //wxCheckBox* chk_run_test;
    wxCheckBox* chk_save_did;
   
    int has_selection;
    int has_excluded;
    
    void OnSelectionChange();
    void RunDIDTest();
    void InitVariableChoiceCtrl();
    void InitGroupsChoiceCtrl();
    void InitTimeChoiceCtrl();
    void InitGroup12ChoiceCtrl();
    
    void OnVariableChoice(wxCommandEvent& event);
    void OnGroupsChoice(wxCommandEvent& event);
    void OnGroup1Choice(wxCommandEvent& event);
    void OnGroup2Choice(wxCommandEvent& event);
    void OnTime1Choice(wxCommandEvent& event);
    void OnTime2Choice(wxCommandEvent& event);
    void OnApplyButton(wxCommandEvent& event);
    
   
    wxString logReport;
    RegressionReportDlg *regReportDlg;
	
	HighlightState* highlight_state;
	GdaVarTools::Manager var_man;
	data_map_type data_map;
    data_map_undef_type data_map_undef;
    
	std::vector<LineChartCanvas*> line_charts;
	std::vector<LineChartStats*> lc_stats;
	std::vector<bool> tms_subset0;
	std::vector<bool> tms_subset1;
	std::vector<bool> tms_subset0_tm_inv;
	std::vector<bool> tms_subset1_tm_inv;
	//std::vector<wxWebView*> stats_wins;
	std::vector<wxHtmlWindow*> stats_wins;
	
	wxStaticText* title1_txt;
	wxStaticText* title2_txt;
	wxBoxSizer* panel_v_szr;
	wxBoxSizer* panel_h_szr;
	wxBoxSizer* ctrls_h_szr;
	wxBoxSizer* title1_h_szr;
	wxBoxSizer* title2_h_szr;
	wxGridBagSizer* bag_szr;
	wxPanel* panel;
	wxHtmlWindow* message_win;
	
    int def_y_precision;
    bool use_def_y_range;
    wxString def_y_min;
    wxString def_y_max;

	
	//bool show_regimes;
	bool display_stats;
	/** only one of compare_regimes, compare_time_periods
		or compare_r_and_t is true */
	bool compare_regimes;
	bool compare_time_periods;
	bool compare_r_and_t;
	int selection_period;
	
	DECLARE_EVENT_TABLE()
};


#endif
