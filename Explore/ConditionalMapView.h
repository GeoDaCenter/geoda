/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include "../TemplateLegend.h"
#include "ConditionalNewView.h"

class ConditionalMapFrame;
class ConditionalMapCanvas;
class ConditionalMapLegend;
class DbfGridTableBase;

class ConditionalMapCanvas : public ConditionalNewCanvas {
	DECLARE_CLASS(ConditionalMapCanvas)
public:
	
	ConditionalMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
					   Project* project,
					   const std::vector<GeoDaVarInfo>& var_info,
					   const std::vector<int>& col_ids,
					   const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize);
	virtual ~ConditionalMapCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCategoriesTitle();
	virtual wxString GetCanvasTitle();

	virtual void NewCustomCatClassifMap();
	virtual void ChangeCatThemeType(
						CatClassification::CatClassifType new_theme,
						bool prompt_num_cats = true,
						const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
	
	virtual void OnSaveCategories();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TitleOrTimeChange();
	
	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
	virtual void DrawLayer0();
	
protected:
	virtual void PopulateCanvas();
	
public:
	virtual void CreateAndUpdateCategories(bool prompt_num_cats = true);
	virtual void TimeSyncVariableToggle(int var_index);
	
	CatClassifDef cat_classif_def_map;
	CatClassification::CatClassifType GetCatType();
	void SetCatType(CatClassification::CatClassifType cc_type);

protected:
	CatClassifState* cc_state_map;
	int num_cats; // current number of categories
	std::vector<GeoDa::dbl_int_pair_vec_type> cat_var_sorted;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	
	// background map related:
	wxBitmap* bin_bm;
	wxPen bin_bg_map_pen;
	wxBrush bin_bg_map_brush;
	
	bool full_map_redraw_needed;
	
	static const int CAT_VAR; // theme variable
	
	virtual void UpdateStatusBar();
		
	DECLARE_EVENT_TABLE()
};

class ConditionalMapLegend : public TemplateLegend {
public:
	ConditionalMapLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				 const wxPoint& pos, const wxSize& size);
	virtual ~ConditionalMapLegend();
};

class ConditionalMapFrame : public ConditionalNewFrame {
   DECLARE_CLASS(ConditionalMapFrame)
public:
    ConditionalMapFrame(wxFrame *parent, Project* project,
					  const std::vector<GeoDaVarInfo>& var_info,
					  const std::vector<int>& col_ids,
					  const wxString& title = "Conditional Map",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalMapFrame();

    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);

	virtual void UpdateTitle();
	
	virtual void OnNewCustomCatClassifA();
	virtual void OnCustomCatClassifA(const wxString& cc_title);

	void OnThemeless(wxCommandEvent& event);
	void OnQuantile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
	void OnStdDevMap(wxCommandEvent& event);
	void OnUniqueValues(wxCommandEvent& event);
	void OnNaturalBreaks(wxCommandEvent& event);
	void OnEqualIntervals(wxCommandEvent& event);
	void OnSaveCategories(wxCommandEvent& event);
	
	virtual void ChangeThemeType(
						CatClassification::CatClassifType new_cat_theme,
						const wxString& custom_classif_title = wxEmptyString);
		
    DECLARE_EVENT_TABLE()
};

#endif
