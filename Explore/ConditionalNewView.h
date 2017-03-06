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

#ifndef __GEODA_CENTER_CONDITIONAL_NEW_VIEW_H__
#define __GEODA_CENTER_CONDITIONAL_NEW_VIEW_H__

#include <vector>
#include <wx/thread.h>
#include "CatClassification.h"
#include "CatClassifStateObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"

class CatClassifState;
class ConditionalNewFrame;
class ConditionalNewCanvas;
class ConditionalNewLegend;
class TableInterface;

typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

typedef boost::multi_array<GdaRectangle, 2> rec_array_type;

class ConditionalNewCanvas
	: public TemplateCanvas, public CatClassifStateObserver
{
	DECLARE_CLASS(ConditionalNewCanvas)
public:
	
	ConditionalNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
						 Project* project,
						 const std::vector<GdaVarTools::VarInfo>& var_info,
						 const std::vector<int>& col_ids,
						 bool fixed_aspect_ratio_mode = false,
						 bool fit_to_window_mode = true,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize);
	virtual ~ConditionalNewCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual wxString GetCategoriesTitle(int var_id);
	virtual wxString GetNameWithTime(int var);

	static const int HOR_VAR; // horizontal variable
	static const int VERT_VAR; // vertical variable
	virtual void NewCustomCatClassifVert();
	virtual void NewCustomCatClassifHoriz();
	virtual void ChangeThemeType(int var_id,
						CatClassification::CatClassifType new_theme,
						int num_categories,
						const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
	
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
		
protected:
	virtual void PopulateCanvas();
	virtual void VarInfoAttributeChange();
	
public:
	virtual void CreateAndUpdateCategories(int var_id);
	virtual void UpdateNumVertHorizCats();
	virtual void UserChangedCellCategories() {}
	
	virtual void TimeSyncVariableToggle(int var_index);
	
	CatClassifDef cat_classif_def_horiz;
	CatClassifDef cat_classif_def_vert;
	CatClassification::CatClassifType GetCatType(int var_id);
	void SetCatType(int var_id, CatClassification::CatClassifType cc_type,
					int num_categories);
	int GetHorizNumCats() { return horiz_num_cats; }
	int GetVertNumCats() { return vert_num_cats; }

protected:
	TableInterface* table_int;
	CatClassifState* cc_state_vert;
	CatClassifState* cc_state_horiz;
	
	int num_obs;
	int num_time_vals; // current number of valid variable combos
	int vert_num_time_vals;
	int horiz_num_time_vals;
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	std::vector<d_array_type> data;
	std::vector<b_array_type> data_undef;
	
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	
	int horiz_num_cats; // number of horizontal categories
	int vert_num_cats; // number of vertical categories
	std::vector<Gda::dbl_int_pair_vec_type> horiz_var_sorted;
	std::vector<Gda::dbl_int_pair_vec_type> vert_var_sorted;
    
    std::vector<std::vector<bool> > horiz_undef_tms; // undef tms
    std::vector<std::vector<bool> > vert_undef_tms; // undef tms
    
	CatClassifData horiz_cat_data;
	CatClassifData vert_cat_data;
	std::vector<bool> horiz_cats_valid;
	std::vector<bool> vert_cats_valid;
	std::vector<wxString> horiz_cats_error_message;
	std::vector<wxString> vert_cats_error_message;

	// background map related:
	rec_array_type bin_extents;
	int bin_w;
	int bin_h;
	
	bool all_init;
	virtual void UpdateStatusBar();
		
	DECLARE_EVENT_TABLE()
};

class ConditionalNewFrame : public TemplateFrame {
   DECLARE_CLASS(ConditionalNewFrame)
public:
    ConditionalNewFrame(wxFrame *parent, Project* project,
					  const std::vector<GdaVarTools::VarInfo>& var_info,
					  const std::vector<int>& col_ids,
					  const wxString& title = "Conditional Map",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalNewFrame();

    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	virtual void OnNewCustomCatClassifA(); // Map Theme
	virtual void OnNewCustomCatClassifB(); // Horizontal Cats
	virtual void OnNewCustomCatClassifC(); // Vertical Cats
	virtual void OnCustomCatClassifA(const wxString& cc_title);
	virtual void OnCustomCatClassifB(const wxString& cc_title);
	virtual void OnCustomCatClassifC(const wxString& cc_title);
	
	virtual void ChangeVertThemeType(
								CatClassification::CatClassifType new_theme,
								int num_categories,
								const wxString& cc_title = wxEmptyString);
	virtual void ChangeHorizThemeType(
								CatClassification::CatClassifType new_theme,
								int num_categories,
								const wxString& cc_title = wxEmptyString);
	
    DECLARE_EVENT_TABLE()
};

#endif
