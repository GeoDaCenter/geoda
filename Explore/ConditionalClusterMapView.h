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

#ifndef __GEODA_CENTER_CONDITIONAL_MAP_VIEW_H__
#define __GEODA_CENTER_CONDITIONAL_MAP_VIEW_H__

#include <vector>
#include "../TemplateLegend.h"
#include "ConditionalNewView.h"

using namespace std;

class LisaCoordinator;
class GStatCoordinator;
class LocalGearyCoordinator;

class ConditionalClusterMapFrame;
class ConditionalClusterMapCanvas;
class ConditionalClusterMapLegend;
class TableInterface;

class ConditionalClusterMapCanvas : public ConditionalNewCanvas {
	DECLARE_CLASS(ConditionalClusterMapCanvas)
public:
	
    ConditionalClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                Project* project,
                                const vector<GdaVarTools::VarInfo>& var_info,
                                const vector<int>& col_ids,
                                const wxString& title,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize);
    
	virtual ~ConditionalClusterMapCanvas();
    
    virtual void CreateAndUpdateCategories() = 0;
    virtual void TimeSyncVariableToggle(int var_index) = 0;
    
    virtual wxString GetCategoriesTitle();
    virtual wxString GetCanvasTitle();

    void Init(const wxSize& size);
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void NewCustomCatClassifMap();
	virtual void ChangeCatThemeType(
						CatClassification::CatClassifType new_theme,
						int num_categories,
						const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
    //virtual void update(HLStateInt* o);
	
	virtual void OnSaveCategories();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	
	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
	virtual void DrawLayer0();
	virtual void ZoomShapes(bool is_zoomin = true);
	//virtual void OnMouseEvent(wxMouseEvent& event);
	virtual void OnScrollChanged(wxScrollWinEvent& event);
    
	
	void SetCatType(CatClassification::CatClassifType cc_type);
	int GetNumCats() { return num_categories; }
    
	CatClassifDef cat_classif_def_map;
	CatClassification::CatClassifType GetCatType();

protected:
    virtual void UpdateStatusBar() = 0;
    virtual void PopulateCanvas();
    
    wxString title;
	CatClassifState* cc_state_map;
	int num_categories; // current number of categories
	vector<Gda::dbl_int_pair_vec_type> cat_var_sorted;
    vector<vector<bool> > cat_var_undef;
    
	vector<bool> map_valid;
	vector<wxString> map_error_message;
	
	// background map related:
	wxBitmap* bin_bm;
	wxPen bin_bg_map_pen;
	wxBrush bin_bg_map_brush;
	
	bool full_map_redraw_needed;
	
	static const int CAT_VAR; // theme variable
	
		
	DECLARE_EVENT_TABLE()
};

class ConditionalClusterMapLegend : public TemplateLegend {
public:
	ConditionalClusterMapLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				 const wxPoint& pos, const wxSize& size);
	virtual ~ConditionalClusterMapLegend();
};

class ConditionalClusterMapFrame : public ConditionalNewFrame {
   DECLARE_CLASS(ConditionalClusterMapFrame)
public:
    ConditionalClusterMapFrame(wxFrame *parent, Project* project,
                               const vector<GdaVarTools::VarInfo>& var_info,
                               const vector<int>& col_ids,
                               LisaCoordinator* lisa_coord,
                               const wxString& title = _("Conditional LISA Map"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               const long style = wxDEFAULT_FRAME_STYLE);
    ConditionalClusterMapFrame(wxFrame *parent, Project* project,
                               const vector<GdaVarTools::VarInfo>& var_info,
                               const vector<int>& col_ids,
                               GStatCoordinator* g_coord,
                               bool is_gi, bool is_perm,
                               const wxString& title = _("Conditional GetisOrd Map"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               const long style = wxDEFAULT_FRAME_STYLE);
    ConditionalClusterMapFrame(wxFrame *parent, Project* project,
                               const vector<GdaVarTools::VarInfo>& var_info,
                               const vector<int>& col_ids,
                               LocalGearyCoordinator* local_geary_coord,
                               const wxString& title = _("Conditional Local Geary Map"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalClusterMapFrame();

    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
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
	
	virtual void ChangeThemeType(
						CatClassification::CatClassifType new_cat_theme,
						int num_categories,
						const wxString& custom_classif_title = wxEmptyString);
    
    DECLARE_EVENT_TABLE()
};


////////////////////////////////////////////////////////////////////////////////
//
// LISA Cluster Conditional map
//
////////////////////////////////////////////////////////////////////////////////
class ConditionalLISAClusterMapCanvas : public ConditionalClusterMapCanvas {
    //DECLARE_CLASS(ConditionalLISAClusterMapCanvas)
public:
    
    ConditionalLISAClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                Project* project,
                                const vector<GdaVarTools::VarInfo>& var_info,
                                const vector<int>& col_ids,
                                LisaCoordinator* lisa_coordinator,
                                    const wxString& title,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize);
    
    virtual ~ConditionalLISAClusterMapCanvas();
    
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
    
protected:
    LisaCoordinator* lisa_coord;
    virtual void UpdateStatusBar();
};

////////////////////////////////////////////////////////////////////////////////
//
// GetisOrder Cluster Conditional map
//
////////////////////////////////////////////////////////////////////////////////
class ConditionalGClusterMapCanvas : public ConditionalClusterMapCanvas {
    //DECLARE_CLASS(ConditionalGClusterMapCanvas)
public:
    
    ConditionalGClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                 Project* project,
                                 const vector<GdaVarTools::VarInfo>& var_info,
                                 const vector<int>& col_ids,
                                 GStatCoordinator* g_coordinator,
                                 bool is_gi,
                                 bool is_perm,
                                 const wxString& title,
                                 const wxPoint& pos = wxDefaultPosition,
                                 const wxSize& size = wxDefaultSize);
    
    virtual ~ConditionalGClusterMapCanvas();
    
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
protected:
    bool is_perm;
    bool is_gi;
    GStatCoordinator* g_coord;
    virtual void UpdateStatusBar();
};

////////////////////////////////////////////////////////////////////////////////
//
// LocalGeary Cluster Conditional map
//
////////////////////////////////////////////////////////////////////////////////
class ConditionalLocalGearyClusterMapCanvas : public ConditionalClusterMapCanvas {
    //DECLARE_CLASS(ConditionalLocalGearyClusterMapCanvas)
public:
    
    ConditionalLocalGearyClusterMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                Project* project,
                                const vector<GdaVarTools::VarInfo>& var_info,
                                const vector<int>& col_ids,
                                LocalGearyCoordinator* local_geary_coordinator,
                                    const wxString& title,
                                const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize);
    
    virtual ~ConditionalLocalGearyClusterMapCanvas();
    
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
    
protected:
    LocalGearyCoordinator* local_geary_coord;
    virtual void UpdateStatusBar();
};
#endif
