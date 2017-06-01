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

#ifndef __GEODA_CENTER_PCP_NEW_VIEW_H__
#define __GEODA_CENTER_PCP_NEW_VIEW_H__

#include <boost/multi_array.hpp>
#include <wx/menu.h>
#include "CatClassification.h"
#include "CatClassifStateObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateLegend.h"
#include "../TemplateFrame.h"
#include "../GdaConst.h"
#include "../VarTools.h"
#include "../GdaShape.h"

class CatClassifState;
class PCPCanvas;
class PCPLegend;
class PCPFrame;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class PCPCanvas : public TemplateCanvas, public CatClassifStateObserver
{
	DECLARE_CLASS(PCPCanvas)
public:
	PCPCanvas(wxWindow *parent, TemplateFrame* t_frame,
				 Project* project,
				 const std::vector<GdaVarTools::VarInfo>& var_info,
				 const std::vector<int>& col_ids,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize);
	virtual ~PCPCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	//virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	virtual wxString GetCategoriesTitle(); // cats
	virtual wxString GetNameWithTime(int var);
	
	virtual void NewCustomCatClassif();
	void ChangeThemeType(CatClassification::CatClassifType new_theme,
						 int num_categories,
						 const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
	virtual void SetCheckMarks(wxMenu* menu);
	void OnSaveCategories(); // cats

protected:
	virtual void PopulateCanvas();
	virtual void TimeChange();
	void VarInfoAttributeChange();
    
public:
	void CreateAndUpdateCategories(); // cats
	
public:
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);
	
	void DisplayStatistics(bool display_stats);
	void ShowAxes(bool show_axes);
	void StandardizeData(bool standardize);
	
	bool IsDisplayStats() { return display_stats; }
	bool IsShowAxes() { return show_axes; }

	/** Used by PCP for detecting and updating PCP-specific controls */
	enum PCPSelectState { pcp_start, pcp_leftdown_on_circ,
		pcp_leftdown_on_label, pcp_dragging };
	/** The function handles all mouse events. */
	void OnMouseEvent(wxMouseEvent& event);
	void VarLabelClicked();
	/** Override PaintControls from TemplateCanvas */
	virtual void PaintControls(wxDC& dc);
	void MoveControlLine(int final_y);
	
	CatClassifDef cat_classif_def;
	CatClassification::CatClassifType GetCcType();
	int GetNumCats() { return num_categories; }
	
protected:
	virtual void UpdateStatusBar();

	CatClassifState* custom_classif_state;
	
    wxPoint pcp_prev;
    wxPoint pcp_sel1;
    wxPoint pcp_sel2;
    
	int num_obs;
	int num_vars;
	int num_time_vals;
	int num_categories;

	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	std::vector<int> var_order; // var id for position 0 to position num_vars-1
	
	std::vector<d_array_type> data;
	std::vector<b_array_type> data_undef;
    std::vector<std::vector<bool> > undef_markers; // times * num_obs
	//std::vector< std::vector<HingeStats> > hinge_stats;
	std::vector< std::vector<SampleStatistics> > data_stats;
	// overall absolute value maximum of standardized data
	double overall_abs_max_std;
	double overall_abs_max_std_exists;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> cats_valid; // cats
	std::vector<wxString> cats_error_message; // cats
	
	bool show_axes;
	bool display_stats;
	bool standardized;
	
	int theme_var; // current theme variable
	std::vector<GdaShapeText*> control_labels;
	int control_label_sel; // selected variable text label
	std::vector<GdaCircle*> control_circs;
	std::vector<GdaPolyLine*> control_lines;
	int control_line_sel; // selected control line
	PCPSelectState pcp_selectstate;
	bool show_pcp_control;
	bool all_init;
	
	DECLARE_EVENT_TABLE()
};

class PCPLegend : public TemplateLegend {
public:
	PCPLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				 const wxPoint& pos, const wxSize& size);
	virtual ~PCPLegend();
};

class PCPFrame : public TemplateFrame {
    DECLARE_CLASS(PCPFrame)
public:
    PCPFrame(wxFrame *parent, Project* project,
					const std::vector<GdaVarTools::VarInfo>& var_info,
					const std::vector<int>& col_ids,
					const wxString& title = "Parallel Coordinate Plot",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GdaConst::pcp_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~PCPFrame();
	
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	virtual void OnNewCustomCatClassifA();
	virtual void OnCustomCatClassifA(const wxString& cc_title);
	void OnShowAxes(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
	void OnViewOriginalData(wxCommandEvent& event);
    void OnViewStandardizedData(wxCommandEvent& event);

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
	
    DECLARE_EVENT_TABLE()
};


#endif
