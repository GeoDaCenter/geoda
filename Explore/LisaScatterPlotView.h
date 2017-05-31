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

#ifndef __GEODA_CENTER_LISA_SCATTER_PLOT_VIEW_H__
#define __GEODA_CENTER_LISA_SCATTER_PLOT_VIEW_H__

#include "../GdaConst.h"
#include "ScatterNewPlotView.h"
#include "LisaCoordinatorObserver.h"
#include "../DialogTools/RandomizationDlg.h"

class LisaCoordinator;

class LisaScatterPlotCanvas : public ScatterNewPlotCanvas
{
	DECLARE_CLASS(LisaScatterPlotCanvas)
public:	
	LisaScatterPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
						  Project* project,
						  LisaCoordinator* lisa_coordinator,
						  const wxPoint& pos = wxDefaultPosition,
						  const wxSize& size = wxDefaultSize);
	virtual ~LisaScatterPlotCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	void SyncVarInfoFromCoordinator();
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
    virtual void update(HLStateInt* o);
    virtual void UpdateSelection(bool shiftdown, bool pointsel);
    virtual void ResizeSelectableShps(int virtual_scrn_w=0, int virtual_scrn_h=0);
    virtual void OnIdle(wxIdleEvent& event);
    
    //virtual void OnIdle(wxIdleEvent& event);
	void ShowRandomizationDialog(int permutation);
	void SaveMoranI();
    
    void UpdateRegSelectedLine();
    void UpdateRegExcludedLine();
	
    void ShowRegimesRegression(bool flag);
    
    
protected:
    void RegimeMoran(std::vector<bool>& undefs,
                     SimpleLinearRegression& regime_lreg,
                     std::vector<double>& X,
                     std::vector<double>& Y);
    void OnRandDlgClose( wxWindowDestroyEvent& event);

	virtual void PopulateCanvas();
	virtual void PopCanvPreResizeShpsHook();
	LisaCoordinator* lisa_coord;
	bool is_bi; // true = Bivariate, false = Univariate
	bool is_rate; // true = Moran Empirical Bayes Rate Smoothing
    bool is_diff;
	std::vector<GdaVarTools::VarInfo> sp_var_info;
	std::vector<GdaVarTools::VarInfo> var_info_orig;
    RandomizationDlg* rand_dlg;
    GdaShapeText* morans_i_text;
    bool is_show_regimes_regression;
    GdaShapeText* morans_sel_text;
    GdaShapeText* morans_unsel_text;
	
	DECLARE_EVENT_TABLE()
};

class LisaScatterPlotFrame : public ScatterNewPlotFrame,
	public LisaCoordinatorObserver
{
	DECLARE_CLASS(LisaScatterPlotFrame)
public:
    LisaScatterPlotFrame(wxFrame *parent, Project* project,
					LisaCoordinator* lisa_coordinator,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxSize(600, 360),
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~LisaScatterPlotFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
    void OnUseSpecifiedSeed(wxCommandEvent& event);
	void OnSpecifySeedDlg(wxCommandEvent& event);
    void OnViewRegimesRegression(wxCommandEvent& event);
    
	void RanXPer(int permutation);
	void OnRan99Per(wxCommandEvent& event);
	void OnRan199Per(wxCommandEvent& event);
	void OnRan499Per(wxCommandEvent& event);
	void OnRan999Per(wxCommandEvent& event);
	void OnRanOtherPer(wxCommandEvent& event);
	void RandomizationP(int numPermutations);  
	
	void OnSaveMoranI(wxCommandEvent& event);
	
	virtual void update(LisaCoordinator* o);
	virtual void closeObserver(LisaCoordinator* o);
	
protected:
	LisaCoordinator* lisa_coord;
	
    DECLARE_EVENT_TABLE()
};


#endif
