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

#include <algorithm> // std::sort
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ConditionalMapView.h"


IMPLEMENT_CLASS(ConditionalMapCanvas, ConditionalNewCanvas)
BEGIN_EVENT_TABLE(ConditionalMapCanvas, ConditionalNewCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int ConditionalMapCanvas::CAT_VAR = 2; // main theme variable

ConditionalMapCanvas::
ConditionalMapCanvas(wxWindow *parent,
                     TemplateFrame* t_frame,
                     Project* project_s,
                     const std::vector<GdaVarTools::VarInfo>& v_info,
                     const std::vector<int>& col_ids,
                     const wxPoint& pos, const wxSize& size)
: ConditionalNewCanvas(parent, t_frame, project_s, v_info, col_ids,
					   true, true, pos, size),
num_categories(1),bin_bm(0),
bin_bg_map_pen(wxColor(200,200,200)),
bin_bg_map_brush(wxColor(200,200,200)),
cc_state_map(0),
full_map_redraw_needed(true)
{
	using namespace Shapefile;
	LOG_MSG("Entering ConditionalMapCanvas::ConditionalMapCanvas");
	SetCatType(CatClassification::no_theme);
	
	selectable_fill_color = GdaConst::map_default_fill_colour;

	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 50;
	virtual_screen_marg_left = 50;
	virtual_screen_marg_right = 25;
	shps_orig_xmin = project->main_data.header.bbox_x_min;
	shps_orig_ymin = project->main_data.header.bbox_y_min;
	shps_orig_xmax = project->main_data.header.bbox_x_max;
	shps_orig_ymax = project->main_data.header.bbox_y_max;
	
	double scale_x, scale_y, trans_x, trans_y;
    GdaScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
                                    shps_orig_xmax, shps_orig_ymax,
                                    virtual_screen_marg_top,
                                    virtual_screen_marg_bottom,
                                    virtual_screen_marg_left,
                                    virtual_screen_marg_right,
                                    GetVirtualSize().GetWidth(),
                                    GetVirtualSize().GetHeight(),
                                    fixed_aspect_ratio_mode,
                                    fit_to_window_mode,
                                    &scale_x, &scale_y,
                                    &trans_x, &trans_y, 0, 0,
                                    &current_shps_width,
                                    &current_shps_height);
    
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;

	if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GdaConst::map_default_highlight_colour;
	}

	use_category_brushes = true;
	if (num_obs >= 6) {
		ChangeCatThemeType(CatClassification::hinge_15, 6);
	} else {
		ChangeCatThemeType(CatClassification::unique_values, 4);
	}
	
	all_init = true;
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting ConditionalMapCanvas::ConditionalMapCanvas");
}

ConditionalMapCanvas::~ConditionalMapCanvas()
{
	LOG_MSG("Entering ConditionalMapCanvas::~ConditionalMapCanvas");
	if (cc_state_map) cc_state_map->removeObserver(this);
	LOG_MSG("Exiting ConditionalMapCanvas::~ConditionalMapCanvas");
}

void ConditionalMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering ConditionalMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ConditionalMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");
	
	// Due to problems with zooming, the following menu options have been
	// temporarily removed from the XRC file:
	//<object class="wxMenuItem" name="ID_SELECTION_MODE">
	//<label>Selection Mode</label>
	//<checkable>1</checkable>
    //</object>
    //<object class="wxMenuItem" name="ID_PAN_MODE">
	//<label>Panning Mode</label>
	//<checkable>1</checkable>
    //</object>
    //<object class="wxMenuItem" name="ID_ZOOM_MODE">
	//<label>Zooming Mode</label>
	//<checkable>1</checkable>
    //</object>
    //<object class="wxMenuItem" name="ID_FIT_TO_WINDOW_MODE">
	//<label>Fit-To-Window Mode</label>
	//<checkable>1</checkable>
    //</object>
    //<object class="wxMenuItem" name="ID_FIXED_ASPECT_RATIO_MODE">
	//<label>Fixed Aspect Ratio Mode</label>
	//<checkable>1</checkable>
    //</object>
	
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting ConditionalMapCanvas::DisplayRightClickMenu");
}

/**
 * Overwrite TemplaceCanvas Scroll
 */
void ConditionalMapCanvas::OnScrollChanged(wxScrollWinEvent& event)
{
	event.Skip();
}
/**
 * Overwrite TemplaceCanvas OnPaint
 */
void ConditionalMapCanvas::OnPaint(wxPaintEvent& event)
{
	DrawLayers();
	
	wxMemoryDC dc(*layer2_bm);
	wxPaintDC paint_dc(this);
	wxSize sz = GetClientSize();
	
	int xx, yy;
	CalcUnscrolledPosition(0, 0, &xx, &yy);
	paint_dc.Blit(0, 0, sz.x, sz.y, &dc, xx, yy);
	
	// Draw the the selection region if needed
	PaintSelectionOutline(paint_dc);

	// Draw optional control objects if needed
	PaintControls(paint_dc);
}

void ConditionalMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	ConditionalNewCanvas::SetCheckMarks(menu);
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
					GetCatType() == CatClassification::no_theme);
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_1"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_2"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_3"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_4"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_5"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_6"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_7"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_8"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_9"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_10"),
								  (GetCatType() == CatClassification::quantile)
								  && GetNumCats() == 10);
	
    GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
					GetCatType() == CatClassification::percentile);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_15"),
					GetCatType() == CatClassification::hinge_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_30"),
					GetCatType() == CatClassification::hinge_30);
    GeneralWxUtils::CheckMenuItem(menu,
					XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
					GetCatType() == CatClassification::stddev);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
					GetCatType() == CatClassification::unique_values);
	
    // since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_1"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_2"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_3"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_4"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_5"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_6"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_7"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_8"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_9"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_10"),
								  (GetCatType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 10);
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_1"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_2"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_3"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_4"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_5"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_6"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_7"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_8"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_9"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_10"),
								  (GetCatType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 10);
	
}

wxString ConditionalMapCanvas::GetCategoriesTitle()
{
	wxString v;
	if (GetCatType() == CatClassification::custom) {
		v << cat_classif_def_map.title;
		v << ": " << GetNameWithTime(CAT_VAR);
	} else if (GetCatType() == CatClassification::no_theme) {
		v << "Themeless";
	} else {
		v << CatClassification::CatClassifTypeToString(GetCatType());
		v << ": " << GetNameWithTime(CAT_VAR);
	}
	return v;	
}

wxString ConditionalMapCanvas::GetCanvasTitle()
{
	wxString v;
	v << "Conditional Map - ";
	v << "x: " << GetNameWithTime(HOR_VAR);
	v << ", y: " << GetNameWithTime(VERT_VAR);
	if (GetCatType() == CatClassification::custom) {
		v << ", " << cat_classif_def_map.title;
		v << ": " << GetNameWithTime(CAT_VAR);
	} else if (GetCatType() != CatClassification::no_theme) {
		v << ", " << CatClassification::CatClassifTypeToString(GetCatType());
		v << ": " << GetNameWithTime(CAT_VAR);
	}
	return v;
}

void ConditionalMapCanvas::OnSaveCategories()
{
	wxString t_name = CatClassification::CatClassifTypeToString(GetCatType());
	if (GetCatType() == CatClassification::custom) {
		t_name = cat_classif_def_map.title;
	}
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
    
    std::vector<bool> undefs(num_obs, false);
    
    for (size_t i=0; i<cat_var_undef.size(); i++) {
        for (size_t j=0; j<cat_var_undef[i].size(); j++) {
            undefs[j] = undefs[j] || cat_var_undef[i][j];
        }
    }
    
	SaveCategories(title, label, "CATEGORIES", undefs);
}

void ConditionalMapCanvas::NewCustomCatClassifMap()
{
	// we know that all three var_info variables are defined, so need
	// need to prompt user as with MapCanvas
	
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def_map.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(cat_classif_def_map.num_cats,
										 cat_classif_def_map);
		std::vector<wxString> temp_cat_labels; // will be ignored
		CatClassification::SetBreakPoints(cat_classif_def_map.breaks,
										  temp_cat_labels,
										  cat_var_sorted[var_info[CAT_VAR].time],
                                          cat_var_undef[var_info[CAT_VAR].time],
										  cat_classif_def_map.cat_classif_type,
										  cat_classif_def_map.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def_map.num_cats; i++) {
			cat_classif_def_map.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def_map.names[i] = cat_data.GetCategoryLabel(time, i);
		}
	}
	
	CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame(this->useScientificNotation);
    
	if (!ccf)
        return;
    
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def_map, "",
										  var_info[CAT_VAR].name,
										  var_info[CAT_VAR].time);
	if (!ccs)
        return;
    
	if (cc_state_map)
        cc_state_map->removeObserver(this);
    
	cat_classif_def_map = ccs->GetCatClassif();
	cc_state_map = ccs;
	cc_state_map->registerObserver(this);
	
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification. */
void ConditionalMapCanvas::ChangeCatThemeType(
							CatClassification::CatClassifType new_cat_theme,
							int num_categories_s,
							const wxString& custom_classif_title)
{
	num_categories = num_categories_s;
	
	if (new_cat_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm) return;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs) return;
		if (cc_state_map == new_ccs) return;
		if (cc_state_map) cc_state_map->removeObserver(this);
		cc_state_map = new_ccs;
		cc_state_map->registerObserver(this);
		cat_classif_def_map = cc_state_map->GetCatClassif();
	} else {
		if (cc_state_map) cc_state_map->removeObserver(this);
		cc_state_map = 0;
	}
	SetCatType(new_cat_theme);
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	UserChangedCellCategories();
	PopulateCanvas();
	if (all_init && template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

void ConditionalMapCanvas::update(CatClassifState* o)
{
	LOG_MSG("In ConditionalMapCanvas::update(CatClassifState*)");
	if (cc_state_map == o) {
		cat_classif_def_map = o->GetCatClassif();
		CreateAndUpdateCategories();
		UserChangedCellCategories();
		PopulateCanvas();
		if (template_frame) {
			template_frame->UpdateTitle();
			if (template_frame->GetTemplateLegend()) {
				template_frame->GetTemplateLegend()->Refresh();
			}
		}
	} else {
		ConditionalNewCanvas::update(o);
	}
}

void ConditionalMapCanvas::OnSize(wxSizeEvent& event)
{
    //LOG_MSG("Entering TemplateCanvas::OnSize");
	// we know there has been a change in the client size
	int cs_w=0, cs_h=0;
	GetClientSize(&cs_w, &cs_h);
	int vs_w, vs_h;
	GetVirtualSize(&vs_w, &vs_h);
	
	if (GetFitToWindowMode()) {
		double new_w = (cs_w-(virtual_screen_marg_left +
							  virtual_screen_marg_right));
		double new_h = (cs_h-(virtual_screen_marg_top +
							  virtual_screen_marg_bottom));
		double new_ar = (double) new_w / (double) new_h;
		//LOG(new_w);
		//LOG(new_h);
		//LOG(new_ar);
		//LOG(fixed_aspect_ratio_mode);
		//LOG(fixed_aspect_ratio_val);
		if (fixed_aspect_ratio_mode) {
			if (fixed_aspect_ratio_val >= new_ar) {
				current_shps_width = new_w;
				current_shps_height = new_w / fixed_aspect_ratio_val;
			} else {
				current_shps_height = new_h;
				current_shps_width = new_h * fixed_aspect_ratio_val;
			}
		} else {
			current_shps_width = new_w;
			current_shps_height = new_h;
		}
		//LOG(current_shps_width);
		//LOG(current_shps_height);
		resizeLayerBms(cs_w, cs_h);
		//SetVirtualSize(cs_w, cs_h);
		ResizeSelectableShps();
	} else {
		int margs_vert = virtual_screen_marg_top + virtual_screen_marg_bottom;
		int margs_horiz = virtual_screen_marg_left + virtual_screen_marg_right;
		int shps_n_margs_w = current_shps_width + margs_horiz;
		int shps_n_margs_h = current_shps_height + margs_vert;
		
		if (shps_n_margs_w <= cs_w && shps_n_margs_h <= cs_h) {
			//LOG_MSG("No Scroll Bars");
			resizeLayerBms(cs_w, cs_h);
			ResizeSelectableShps(cs_w, cs_h);
			SetVirtualSize(cs_w, cs_h);
			scrollbarmode = none;
		}
		if (shps_n_margs_w <= cs_w && shps_n_margs_h > cs_h) {
			//LOG_MSG("Vertical Scroll Bars Only");
			resizeLayerBms(cs_w, shps_n_margs_h);
			ResizeSelectableShps(cs_w, shps_n_margs_h);
			SetVirtualSize(cs_w, shps_n_margs_h);
#ifdef __WXMSW__
			Update();  // Only needed in Windows to get Vertical SB to
			// draw automatically
#endif
			scrollbarmode = vert_only;
		}
		if (shps_n_margs_w > cs_w && shps_n_margs_h <= cs_h) {
			LOG_MSG("Horizontal Scroll Bars Only");
			resizeLayerBms(shps_n_margs_w, cs_h);
			ResizeSelectableShps(shps_n_margs_w, cs_h);
			SetVirtualSize(shps_n_margs_w, cs_h);
			scrollbarmode = horiz_only;
#ifdef __WXMSW__
			Update(); // Only needed in Windows to get Vertical SB to
			// draw automatically
#endif
		}
		if (shps_n_margs_w > cs_w && shps_n_margs_h > cs_h) {
			LOG_MSG("Vertical and Horizontal Scroll Bars");
			resizeLayerBms(shps_n_margs_w, shps_n_margs_h);
			SetVirtualSize(shps_n_margs_w, shps_n_margs_h);
			if (scrollbarmode != horiz_and_vert) {
				LOG_MSG("One-time shps resize");
				ResizeSelectableShps(shps_n_margs_w, shps_n_margs_h);
			}
			scrollbarmode = horiz_and_vert;
		}
	}
	
	event.Skip();
	//LOG_MSG("Exiting TemplateCanvas::OnSize");
}
// use virtual canvas style code
void ConditionalMapCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Capture the mouse when left mouse button is down.
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();
	int wheel_rotation = event.GetWheelRotation();
	int wheel_delta = GenUtils::max<int>(event.GetLinesPerAction(), 1);
	int wheel_lines_per_action =GenUtils::max<int>(event.GetLinesPerAction(),1);
	if (abs(wheel_rotation) >= wheel_delta) {
		LOG(wheel_rotation);
		LOG(wheel_delta);
		LOG(wheel_lines_per_action);
	}
	
	if (mousemode == select) {
		if (selectstate == start) {
			if (event.LeftDown()) {
				prev = GetActualPos(event);
				sel1 = prev;
				selectstate = leftdown;
			} else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			} else {
				if (template_frame->IsStatusBarVisible()) {
					prev = GetActualPos(event);
					sel1 = prev; // sel1 now has current mouse position
					DetermineMouseHoverObjects();
					UpdateStatusBar();
				}
			}
		} else if (selectstate == leftdown) {
			if (event.Moving() || event.Dragging()) {
				wxPoint act_pos = GetActualPos(event);
				if (fabs((double) (prev.x - act_pos.x)) +
					fabs((double) (prev.y - act_pos.y)) > 2) {
					sel1 = prev;
					sel2 = GetActualPos(event);
					selectstate = dragging;
					remember_shiftdown = event.ShiftDown();
					UpdateSelectRegion();
					UpdateSelection(remember_shiftdown);
					UpdateStatusBar();
					Refresh();
				}
			} else if (event.LeftUp()) {
				UpdateSelectRegion();
				UpdateSelection(event.ShiftDown(), true);
				selectstate = start;
				Refresh();
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
				UpdateSelectRegion();
				UpdateSelection(remember_shiftdown);
				UpdateStatusBar();
				Refresh();
			} else if (event.LeftUp() && !event.CmdDown()) {
				sel2 = GetActualPos(event);
				UpdateSelectRegion();
				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				selectstate = start;
				Refresh();
			} else if (event.LeftUp() && event.CmdDown()) {
				selectstate = brushing;
				sel2 = GetActualPos(event);
				wxPoint diff = wxPoint(0,0);
				UpdateSelectRegion(false, diff);
				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				Refresh();
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}			
		} else if (selectstate == brushing) {
			if (event.LeftIsDown()) {
			} else if (event.LeftUp()) {
				selectstate = start;
				Refresh();
			}
			else if (event.RightDown()) {
				selectstate = start;
				Refresh();
			} else if (event.Moving()) {
				wxPoint diff = GetActualPos(event) - sel2;
				sel1 += diff;
				sel2 = GetActualPos(event);
				UpdateStatusBar();
				UpdateSelectRegion(true, diff);
				UpdateSelection();
				Refresh();
			}
		} else { // unknown state
			LOG_MSG("TemplateCanvas::OnMouseEvent: ERROR, unknown SelectState");
		}
		
	} else if (mousemode == zoom) {
		// we will allow zooming in up to a maximum virtual screen area
		if (event.LeftUp()) {
			SetFitToWindowMode(false);
			int client_screen_w, client_screen_h;
			GetClientSize(&client_screen_w, &client_screen_h);
			int virtual_screen_w, virtual_screen_h;
			GetVirtualSize(&virtual_screen_w, &virtual_screen_h);
			wxSize v_size(GetVirtualSize()); 
			bool zoom_changed = false;
			if (!event.CmdDown()) {  // zoom in
				LOG_MSG("Entering TemplateCanvas::OnMouseEvent zoom in");				
				if ( (int) (current_shps_width * current_shps_height * 4) <=
					GdaConst::shps_max_area &&
					(int) (current_shps_width*2)<=GdaConst::shps_max_width &&
					(int) (current_shps_height*2)<=GdaConst::shps_max_height){
					current_shps_width *= 2;
					current_shps_height *= 2;
					
					int new_w = (int) current_shps_width +
					virtual_screen_marg_left + virtual_screen_marg_right;
					int new_h = (int) current_shps_height +
					virtual_screen_marg_top + virtual_screen_marg_bottom;
					if ( new_h > client_screen_w || new_h > client_screen_h ) {
					}
					SetVirtualSize(GenUtils::max<int>(new_w,client_screen_w),
								    GenUtils::max<int>(new_h,client_screen_h));
					zoom_changed = true;
				}
			} else {                 // zoom out
				LOG_MSG("Entering TemplateCanvas::OnMouseEvent zoom out");				
				if ( (int)(current_shps_width/2)>=GdaConst::shps_min_width 
					&&(int)(current_shps_height/2)>=GdaConst::shps_min_height) {
					current_shps_width /= 2;
					current_shps_height /= 2;
					int new_w = (int) current_shps_width +
						virtual_screen_marg_left + virtual_screen_marg_right;
					int new_h = (int) current_shps_height +
						virtual_screen_marg_top + virtual_screen_marg_bottom;
					int new_vs_w = GenUtils::max<int>(new_w, client_screen_w);
					int new_vs_h = GenUtils::max<int>(new_h, client_screen_h);
					LOG(new_vs_w);
					LOG(new_vs_h);
					SetVirtualSize(new_vs_w, new_vs_h);
					zoom_changed = true;
				}
			}
			if (zoom_changed) {
				//LOG_MSG(GetCanvasStateString());
				int margs_vert = virtual_screen_marg_top + virtual_screen_marg_bottom;
				int margs_horiz = virtual_screen_marg_left + virtual_screen_marg_right;
				int shps_n_margs_w = current_shps_width + margs_horiz;
				int shps_n_margs_h = current_shps_height + margs_vert;
				resizeLayerBms(shps_n_margs_w, shps_n_margs_h);
				ResizeSelectableShps();
				Refresh();
			}
			LOG_MSG("Exiting TemplateCanvas::OnMouseEvent zoom");
		} else if (event.RightDown()) {
			DisplayRightClickMenu(event.GetPosition());
		}
	} else if (mousemode == pan) {
		if (event.Moving()) {
			// in start state, do nothing
		} else if (event.LeftDown()) {
			prev = event.GetPosition();
			// temporarily set scroll rate to 1
			SetScrollRate(1,1);
		} else if (event.Dragging()&& !event.LeftUp() && !event.LeftDown()) {
			int xViewStart, yViewStart;
			GetViewStart(&xViewStart, &yViewStart);
			wxPoint diff = event.GetPosition() - prev;
			prev = event.GetPosition();
			Scroll(xViewStart-diff.x, yViewStart-diff.y);
		} else if (event.LeftUp()) {
			// restore original scroll rate
			SetScrollRate(1,1);
		} else if (event.RightDown()) {
			DisplayRightClickMenu(event.GetPosition());
		}		
	}
}


void ConditionalMapCanvas::ZoomShapes(bool is_zoomin)
{
	if (sel2.x == 0 && sel2.y==0) return;
	
	// get current selected extent/view in map coordinates
	// topLeft, bottomRight
	//double resize_xmin, resize_ymin, resize_xmax, resize_ymax;
	if (!is_pan_zoom ) {
		current_map_x_min = shps_orig_xmin;
		current_map_y_min = shps_orig_ymin;
		current_map_x_max = shps_orig_xmax;
		current_map_y_max = shps_orig_ymax;
	}
	
	int vs_w=0, vs_h=0;
	GetVirtualSize(&vs_w, &vs_h);

	double scn_w = (double) vs_w;
	double scn_h = (double) vs_h;
	
	double image_width, image_height;
	bool ftwm = GetFitToWindowMode();
	
	// pixels between columns/rows
	double fac = 0.02;
	if (vert_num_cats >= 4 || horiz_num_cats >=4) fac = 0.015;
	double pad_w = scn_w * fac;
	double pad_h = scn_h * fac;
	if (pad_w < 1) pad_w = 1;
	if (pad_h < 1) pad_h = 1;
	double pad = GenUtils::min<double>(pad_w, pad_h);
	
	double marg_top = virtual_screen_marg_top;
	double marg_bottom = virtual_screen_marg_bottom;
	double marg_left = virtual_screen_marg_left;
	double marg_right = virtual_screen_marg_right;
	
	double d_rows = vert_num_cats;
	double d_cols = horiz_num_cats;
	
	double individual_map_scn_w = (scn_w - marg_left - marg_right) / horiz_num_cats;
	double individual_map_scn_h = (scn_h - marg_bottom - marg_top) / vert_num_cats;
	
	int scn_map_idx_h_1 = (int)((sel1.x - marg_left)/individual_map_scn_w);
	int scn_map_idx_v_1 = (int)((sel1.y - marg_top) / individual_map_scn_h);
	int scn_map_idx_h_2 = (int)((sel2.x - marg_left)/individual_map_scn_w);
	int scn_map_idx_v_2 = (int)((sel2.y - marg_top) / individual_map_scn_h);
	if (scn_map_idx_h_1 < 0 || scn_map_idx_v_1 < 0 ||
		scn_map_idx_h_2 < 0 || scn_map_idx_v_2 < 0 ||
		(scn_map_idx_h_1!=scn_map_idx_h_2) ||
		(scn_map_idx_v_1!=scn_map_idx_v_2)) return;
	
	wxPoint scn1, scn2;
	scn1.x = sel1.x - marg_left - scn_map_idx_h_1 * individual_map_scn_w;
	scn1.y = sel1.y - marg_top - scn_map_idx_v_1 * individual_map_scn_h;
	scn2.x = sel2.x - marg_left - scn_map_idx_h_1 * individual_map_scn_w;
	scn2.y = sel2.y - marg_top - scn_map_idx_v_1 * individual_map_scn_h;
	
	double s_x, s_y, t_x, t_y;
	GdaScaleTrans::calcAffineParams(current_map_x_min, current_map_y_min,
									current_map_x_max, current_map_y_max,
									pad, pad, pad, pad,
									individual_map_scn_w, individual_map_scn_h, 
									fixed_aspect_ratio_mode,
									ftwm,
									&s_x, &s_y, &t_x, &t_y,
									ftwm ? 0 : current_shps_width,
									ftwm ? 0 : current_shps_height,
									&image_width, &image_height);	
	wxRealPoint map_sel1, map_sel2;
	map_sel1.x = (scn1.x - t_x) / s_x;
	map_sel1.y = (scn1.y - t_y) / s_y;
	map_sel2.x = (scn2.x - t_x) / s_x;
	map_sel2.y = (scn2.y - t_y) / s_y;
	

	
	if (!is_zoomin) {
		double current_map_w = current_map_x_max - current_map_x_min;
		double current_map_h = current_map_y_max - current_map_y_min;
		double w_ratio = current_map_w / abs( map_sel1.x - map_sel2.x);
		double h_ratio = current_map_h / abs( map_sel1.y - map_sel2.y);
		double ratio = w_ratio > h_ratio ? h_ratio : w_ratio;
		
		double x_expand = current_map_w * (ratio - 1) / 2.0;
		double y_expand = current_map_h * (ratio - 1) / 2.0;
		current_map_x_min = current_map_x_min - x_expand;
		current_map_x_max = current_map_x_max + x_expand;
		current_map_y_min = current_map_y_min - y_expand;
		current_map_y_max = current_map_y_max + y_expand;
	} else {
		current_map_x_min = std::min( map_sel1.x, map_sel2.x);
		current_map_x_max = std::max( map_sel1.x, map_sel2.x);
		current_map_y_min = std::min( map_sel1.y, map_sel2.y);
		current_map_y_max = std::max( map_sel1.y, map_sel2.y);
	}
	is_pan_zoom = true;
	ResizeSelectableShps();
}

void ConditionalMapCanvas::ResizeSelectableShps(int virtual_scrn_w,
												int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
	LOG_MSG("Entering ConditionalMapCanvas::ResizeSelectableShps");
	int vs_w=virtual_scrn_w, vs_h=virtual_scrn_h;
	if (vs_w <= 0 && vs_h <= 0) GetVirtualSize(&vs_w, &vs_h);
	
	double image_width, image_height;
	bool ftwm = GetFitToWindowMode();
	
	// last_scale_trans is only used in calls made to ApplyLastResizeToShp
	// which are made in ScaterNewPlotView
	GdaScaleTrans **st;
	st = new GdaScaleTrans*[vert_num_cats];
	for (int i=0; i<vert_num_cats; i++) {
		st[i] = new GdaScaleTrans[horiz_num_cats];
	}
	
	// Total width height:  vs_w   vs_h
	// Working area margins: virtual_screen_marg_top,
	//  virtual_screen_marg_bottom,
	//  virtual_screen_marg_left,
	//  virtual_screen_marg_right
	// We need to increase these as needed for each tile area
	
	double scn_w = vs_w;
	double scn_h = vs_h;
	
	// pixels between columns/rows
	double fac = 0.02;
	if (vert_num_cats >= 4 || horiz_num_cats >=4) fac = 0.015;
	double pad_w = scn_w * fac;
	double pad_h = scn_h * fac;
	if (pad_w < 1) pad_w = 1;
	if (pad_h < 1) pad_h = 1;
	double pad = GenUtils::min<double>(pad_w, pad_h);
	
	double marg_top = virtual_screen_marg_top;
	double marg_bottom = virtual_screen_marg_bottom;
	double marg_left = virtual_screen_marg_left;
	double marg_right = virtual_screen_marg_right;
	
	double d_rows = vert_num_cats;
	double d_cols = horiz_num_cats;
	
	double tot_width = scn_w - ((d_cols-1)*pad + marg_left + marg_right);
	double tot_height = scn_h - ((d_rows-1)*pad + marg_top + marg_bottom);
	double del_width = tot_width / d_cols;
	double del_height = tot_height / d_rows;
	
	bin_extents.resize(boost::extents[vert_num_cats][horiz_num_cats]);
	for (int row=0; row<vert_num_cats; row++) {
		double drow = row;
		for (int col=0; col<horiz_num_cats; col++) {
			double dcol = col;
			double ml = marg_left + col*(pad+del_width);
			double mr = marg_right + ((d_cols-1)-col)*(pad+del_width);
			double mt = marg_top + row*(pad+del_height);
			double mb = marg_bottom + ((d_rows-1)-row)*(pad+del_height);
			
			double s_x, s_y, t_x, t_y;
			GdaScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
											shps_orig_xmax, shps_orig_ymax,
										   mt, mb, ml, mr,
										   vs_w, vs_h, fixed_aspect_ratio_mode,
										   ftwm,
										   &s_x, &s_y, &t_x, &t_y,
										   ftwm ? 0 : current_shps_width,
										   ftwm ? 0 : current_shps_height,
										   &image_width, &image_height);
			st[(vert_num_cats-1)-row][col].scale_x = s_x;
			st[(vert_num_cats-1)-row][col].scale_y = s_y;
			st[(vert_num_cats-1)-row][col].trans_x = t_x;
			st[(vert_num_cats-1)-row][col].trans_y = t_y;
			st[(vert_num_cats-1)-row][col].max_scale =
				GenUtils::max<double>(s_x, s_y);
			
			wxRealPoint ll(shps_orig_xmin, shps_orig_ymin);
			wxRealPoint ur(shps_orig_xmax, shps_orig_ymax);
			bin_extents[(vert_num_cats-1)-row][col] = GdaRectangle(ll, ur);
			bin_extents[(vert_num_cats-1)-row][col].applyScaleTrans(
											st[(vert_num_cats-1)-row][col]);
		}
	}
	
	bin_w = bin_extents[0][0].upper_right.x-bin_extents[0][0].lower_left.x;
	if (bin_w < 0) bin_w = -bin_w;
	bin_h = bin_extents[0][0].upper_right.y-bin_extents[0][0].lower_left.y;
	if (bin_h < 0) bin_h = -bin_h;
	
	bool bin_bm_redraw_needed = false;
	if (bin_bm &&
		(bin_bm->GetWidth() != bin_w || bin_bm->GetHeight() != bin_h)) {
		delete bin_bm; bin_bm = 0;
		bin_bm_redraw_needed = true;
	} else if (!bin_bm) {
		bin_bm_redraw_needed = true;
	}
	
	if (bin_bm_redraw_needed) {
		bin_bm = new wxBitmap(bin_w, bin_h);
		wxMemoryDC dc(*bin_bm);
		dc.SetPen(*wxWHITE_PEN);
		dc.SetBrush(*wxWHITE_BRUSH);
		dc.DrawRectangle(0, 0, bin_w, bin_h);

		double s_x, s_y, t_x, t_y;
		GdaScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
										shps_orig_xmax, shps_orig_ymax,
									   0, 0, 0, 0,
									   bin_w, bin_h, fixed_aspect_ratio_mode,
									   ftwm,
									   &s_x, &s_y, &t_x, &t_y,
									   ftwm ? 0 : current_shps_width,
									   ftwm ? 0 : current_shps_height,
									   &image_width, &image_height);
		GdaScaleTrans bin_st(s_x, s_y, t_x, t_y);
		for (int i=0; i<num_obs; i++) {
			selectable_shps[i]->applyScaleTrans(bin_st);
		}
		BOOST_FOREACH( GdaShape* shp, selectable_shps ) {
			shp->paintSelf(dc);
		}
		bin_bm_redraw_needed = false;
	}
	
	int row_c;
	int col_c;
	for (int i=0; i<num_obs; i++) {
		row_c = vert_cat_data.categories[var_info[VERT_VAR].time].id_to_cat[i];
		col_c = horiz_cat_data.categories[var_info[HOR_VAR].time].id_to_cat[i];
		selectable_shps[i]->applyScaleTrans(st[row_c][col_c]);
	}
	if (selectable_shps_type == polygons) {
		int proj_to_pnt_cnt = 0;
		for (int i=0; i<num_obs; i++) {
			if (((GdaPolygon*) selectable_shps[i])->all_points_same) {
				proj_to_pnt_cnt++;
			}
		}
		double perc = proj_to_pnt_cnt*100;
		perc /= (double) num_obs;
		wxString s;
		s << "ResizeSelectableShps: " << proj_to_pnt_cnt << "/" << num_obs;
		s << ", " << perc << "% project to single point";
		LOG_MSG(s);
	}
	
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();	
	
	double bg_xmin = marg_left;
	double bg_xmax = scn_w-marg_right;
	double bg_ymin = marg_bottom;
	double bg_ymax = scn_h-marg_top;
		
	std::vector<wxRealPoint> v_brk_ref(vert_num_cats-1);
	std::vector<wxRealPoint> h_brk_ref(horiz_num_cats-1);
	
	for (int row=0; row<vert_num_cats-1; row++) {
		double y = (bin_extents[row][0].lower_left.y +
					bin_extents[row+1][0].upper_right.y)/2.0;
		v_brk_ref[row].x = bg_xmin;
		v_brk_ref[row].y = scn_h-y;
	}

	for (int col=0; col<horiz_num_cats-1; col++) {
		double x = (bin_extents[0][col].upper_right.x +
					bin_extents[0][col+1].lower_left.x)/2.0;
		h_brk_ref[col].x = x;
		h_brk_ref[col].y = bg_ymin;
	}
	
	GdaShape* s;
	int vt = var_info[VERT_VAR].time;
	for (int row=0; row<vert_num_cats-1; row++) {
		double b;
		if (cat_classif_def_vert.cat_classif_type != CatClassification::custom){
			if (!vert_cat_data.HasBreakVal(vt, row)) continue;
			b = vert_cat_data.GetBreakVal(vt, row);
		} else {
			b = cat_classif_def_vert.breaks[row];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new GdaShapeText(t, *GdaConst::small_font, v_brk_ref[row], 90,
					   GdaShapeText::h_center, GdaShapeText::bottom, -7, 0);
		background_shps.push_back(s);
	}
	if (ConditionalNewCanvas::GetCatType(VERT_VAR)
		!= CatClassification::no_theme) {
		s = new GdaShapeText(ConditionalNewCanvas::GetCategoriesTitle(VERT_VAR),
					   *GdaConst::small_font,
					   wxRealPoint(bg_xmin, bg_ymin+(bg_ymax-bg_ymin)/2.0), 90,
					   GdaShapeText::h_center, GdaShapeText::bottom, -(7+18), 0);
		background_shps.push_back(s);
	}
	
	int ht = var_info[HOR_VAR].time;
	for (int col=0; col<horiz_num_cats-1; col++) {
		double b;
		if (cat_classif_def_horiz.cat_classif_type!= CatClassification::custom){
			if (!horiz_cat_data.HasBreakVal(ht, col)) continue;
			b = horiz_cat_data.GetBreakVal(ht, col);
		} else {
			b = cat_classif_def_horiz.breaks[col];
		}
		wxString t(GenUtils::DblToStr(b));
		s = new GdaShapeText(t, *GdaConst::small_font, h_brk_ref[col], 0,
					   GdaShapeText::h_center, GdaShapeText::top, 0, 7);
		background_shps.push_back(s);
	}
	if (ConditionalNewCanvas::GetCatType(HOR_VAR)
		!= CatClassification::no_theme) {
		s = new GdaShapeText(ConditionalNewCanvas::GetCategoriesTitle(HOR_VAR),
					   *GdaConst::small_font,
					   wxRealPoint(bg_xmin+(bg_xmax-bg_xmin)/2.0, bg_ymin), 0,
					   GdaShapeText::h_center, GdaShapeText::top, 0, (7+18));
		background_shps.push_back(s);
	}
	
	GdaScaleTrans::calcAffineParams(marg_left, marg_bottom,
								   scn_w-marg_right,
								   scn_h-marg_top,
								   marg_top, marg_bottom,
								   marg_left, marg_right,
								   vs_w, vs_h, false,
								   ftwm,
								   &last_scale_trans.scale_x,
								   &last_scale_trans.scale_y,
								   &last_scale_trans.trans_x,
								   &last_scale_trans.trans_y,
								   0, 0, &image_width, &image_height);
	last_scale_trans.max_scale =
	GenUtils::max<double>(last_scale_trans.scale_x,
						  last_scale_trans.scale_y);
	BOOST_FOREACH( GdaShape* ms, background_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}
	BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}
	
	layer0_valid = false;
	Refresh();
	
	for (int i=0; i<vert_num_cats; i++) delete [] st[i];
	delete [] st;
	
	LOG_MSG("Exiting ConditionalMapCanvas::ResizeSelectableShps");
}

// Draw all solid background, background decorations and unhighlighted
// shapes.
void ConditionalMapCanvas::DrawLayer0()
{
	LOG_MSG("In ConditionalMapCanvas::DrawLayer0");
	wxSize sz = GetVirtualSize();
	if (!layer0_bm) resizeLayerBms(sz.GetWidth(), sz.GetHeight());
	wxMemoryDC dc(*layer0_bm);
	dc.SetPen(canvas_background_color);
	dc.SetBrush(canvas_background_color);
	dc.DrawRectangle(wxPoint(0,0), sz);
	
	// using bin_extents, tile bin_bm at every cell position
	if (bin_bm) {
		for (size_t i=0; i<bin_extents.shape()[0]; i++) {
			for (size_t j=0; j<bin_extents.shape()[1]; j++) {
				int x = bin_extents[i][j].lower_left.x;
				int y = bin_extents[i][j].upper_right.y;
				dc.DrawBitmap(*bin_bm, x, y);
				
				// Draw a red rectangle for debugging purposes
				//int w = (bin_extents[i][j].upper_right.x -
				//		 bin_extents[i][j].lower_left.x);
				//if (w < 0) w = -w;
				//int h = (bin_extents[i][j].upper_right.y -
				//		 bin_extents[i][j].lower_left.y);
				//if (h < 0) h = -h;
				//dc.SetBrush(*wxRED_BRUSH);
				//dc.DrawRectangle(wxPoint(x,y), wxSize(w,h));
			}
		}
	}
		
	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
	if (draw_sel_shps_by_z_val) {
		DrawSelectableShapesByZVal(dc);
	} else {
		DrawSelectableShapes(dc);
	}
	
	layer0_valid = true;
	layer1_valid = false;
	layer2_valid = false;
}



/** This method assumes that v1 is already set and valid.  It will
 recreate all canvas objects as needed and refresh the canvas.
 Assumes that CreateAndUpdateCategories has already been called.
 All data analysis will have been done in CreateAndUpdateCategories
 already. */
void ConditionalMapCanvas::PopulateCanvas()
{
	LOG_MSG("Entering ConditionalMapCanvas::PopulateCanvas");
	
	int canvas_ts = cat_data.GetCurrentCanvasTmStep();
	if (!map_valid[canvas_ts]) full_map_redraw_needed = true;
	
	// Note: only need to delete selectable shapes if the cartogram
	// relative positions change.  Otherwise, just reuse.
	if (full_map_redraw_needed) {
		BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
		selectable_shps.clear();
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();

	if (map_valid[canvas_ts]) {
		if (full_map_redraw_needed) {
			CreateSelShpsFromProj(selectable_shps, project);
			BOOST_FOREACH( GdaShape* shp, selectable_shps ) {
				shp->setPen(bin_bg_map_pen);
				shp->setBrush(bin_bg_map_brush);
			}
			full_map_redraw_needed = false;
		}
	} else {
		wxRealPoint cntr_ref_pnt(shps_orig_xmin +
								 (shps_orig_xmax-shps_orig_xmin)/2.0,
								 shps_orig_ymin+ 
								 (shps_orig_ymax-shps_orig_ymin)/2.0);
		GdaShapeText* txt_shp = new GdaShapeText(map_error_message[canvas_ts],
									 *GdaConst::medium_font, cntr_ref_pnt);
		background_shps.push_back(txt_shp);
	}
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting ConditionalMapCanvas::PopulateCanvas");
}

void ConditionalMapCanvas::TimeChange()
{
	LOG_MSG("Entering ConditionalMapCanvas::TimeChange");
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetTimeState()->GetCurrTime();
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
		(cts < ref_time_min && ref_time == ref_time_min)) return;
	if (cts > ref_time_max) {
		ref_time = ref_time_max;
	} else if (cts < ref_time_min) {
		ref_time = ref_time_min;
	} else {
		ref_time = cts;
	}
	for (size_t i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	
	// Note: although horiz_cat_data and vert_cat_data have canvas time steps,
	// these are not the same as map canvas time steps which can change
	// according to the time periods chosen for each variable in the
	// map.  The number of time steps for the horizontal and vertical
	// variables remains the same, but only a subset of time periods might
	// be accessed at a certain time.
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);

	UpdateNumVertHorizCats();
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting ConditionalMapCanvas::TimeChange");
}

/** Update Categories based on num_time_vals, num_categories and ref_var_index.
 This method populates cat_var_sorted from data array. */
void ConditionalMapCanvas::CreateAndUpdateCategories()
{
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++)
        map_valid[t] = true;
    
	map_error_message.resize(num_time_vals);
    
	for (int t=0; t<num_time_vals; t++)
        map_error_message[t] = wxEmptyString;
	
	//NOTE: cat_var_sorted is sized to current num_time_vals, but
	// cat_var_sorted_vert and horiz is sized to all available number time
	// vals.  Perhaps this should be moved into the constructor since
	// we do not allow smoothing with multiple time variables.
	cat_var_sorted.resize(num_time_vals);
    cat_var_undef.resize(num_time_vals);
    
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].resize(num_obs);
        cat_var_undef[t].resize(num_obs);
        
		int thm_t = (var_info[CAT_VAR].sync_with_global_time ? 
					 t + var_info[CAT_VAR].time_min : var_info[CAT_VAR].time);
        
		for (int i=0; i<num_obs; i++) {
			cat_var_sorted[t][i].first = data[CAT_VAR][thm_t][i];
			cat_var_sorted[t][i].second = i;
            
            cat_var_undef[t][i] = data_undef[CAT_VAR][thm_t][i];
		}
	}
	
	// Sort each vector in ascending order
	std::sort(cat_var_sorted[0].begin(), cat_var_sorted[0].end(),
			  Gda::dbl_int_pair_cmp_less);
    
	if (var_info[CAT_VAR].sync_with_global_time) {
		for (int t=1; t<num_time_vals; t++) {
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	} else {
		// just copy first sorted results
		for (int t=1; t<num_time_vals; t++) {
			cat_var_sorted[t] = cat_var_sorted[0];
		}
	}
	
	if (cat_classif_def_map.cat_classif_type !=
		CatClassification::custom) {
		CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
	}
	cat_classif_def_map.color_scheme =
		CatClassification::GetColSchmForType(
							 cat_classif_def_map.cat_classif_type);
	CatClassification::PopulateCatClassifData(cat_classif_def_map,
											  cat_var_sorted,
                                              cat_var_undef,
											  cat_data, map_valid,
											  map_error_message,
                                              this->useScientificNotation);
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int mt = cat_data.GetCurrentCanvasTmStep();
	num_categories = cat_data.categories[mt].cat_vec.size();
	CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def_map);
}

void ConditionalMapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In ConditionalMapCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

CatClassification::CatClassifType ConditionalMapCanvas::GetCatType()
{
	return cat_classif_def_map.cat_classif_type;
}

void ConditionalMapCanvas::SetCatType(CatClassification::CatClassifType t)
{
	cat_classif_def_map.cat_classif_type = t;
}

void ConditionalMapCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
    if (highlight_state->GetTotalHighlighted()> 0) {
		s << "#selected=" << highlight_state->GetTotalHighlighted() << "  ";
	}
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "hover obs " << hover_obs[0]+1 << " = ";
			s << data[CAT_VAR][var_info[CAT_VAR].time][hover_obs[0]];
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = ";
			s << data[CAT_VAR][var_info[CAT_VAR].time][hover_obs[1]];
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = ";
			s << data[CAT_VAR][var_info[CAT_VAR].time][hover_obs[2]];
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	} 
	sb->SetStatusText(s);
}


ConditionalMapLegend::ConditionalMapLegend(wxWindow *parent,
									   TemplateCanvas* t_canvas,
									   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

ConditionalMapLegend::~ConditionalMapLegend()
{
    LOG_MSG("In ConditionalMapLegend::~ConditionalMapLegend");
}


IMPLEMENT_CLASS(ConditionalMapFrame, ConditionalNewFrame)
BEGIN_EVENT_TABLE(ConditionalMapFrame, ConditionalNewFrame)
	EVT_ACTIVATE(ConditionalMapFrame::OnActivate)	
END_EVENT_TABLE()

ConditionalMapFrame::ConditionalMapFrame(wxFrame *parent, Project* project,
									 const std::vector<GdaVarTools::VarInfo>& var_info,
									 const std::vector<int>& col_ids,
									 const wxString& title, const wxPoint& pos,
									 const wxSize& size, const long style)
: ConditionalNewFrame(parent, project, var_info, col_ids, title, pos,
					  size, style)
{
	LOG_MSG("Entering ConditionalMapFrame::ConditionalMapFrame");

	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
		
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
	template_canvas = new ConditionalMapCanvas(rpanel, this, project,
											 var_info, col_ids,
											 wxDefaultPosition,
											 wxDefaultSize);
	SetTitle(template_canvas->GetCanvasTitle());
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
	wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new ConditionalMapLegend(lpanel, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);

	splitter_win->SplitVertically(lpanel, rpanel,
								  GdaConst::map_default_legend_width);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(splitter_win, 1, wxEXPAND|wxALL);
    SetSizer(sizer);
    splitter_win->SetSize(wxSize(width,height));
    SetAutoLayout(true);
    DisplayStatusBar(true);
	Show(true);
	LOG_MSG("Exiting ConditionalMapFrame::ConditionalMapFrame");
}

ConditionalMapFrame::~ConditionalMapFrame()
{
	LOG_MSG("In ConditionalMapFrame::~ConditionalMapFrame");
	DeregisterAsActive();
}

void ConditionalMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In ConditionalMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("ConditionalMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConditionalMapFrame::MapMenus()
{
	LOG_MSG("In ConditionalMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_COND_MAP_VIEW_MENU_OPTIONS");
	((ConditionalMapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((ConditionalMapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConditionalMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("ConditionalMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((ConditionalMapCanvas*)
		 template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConditionalMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void  ConditionalMapFrame::update(TimeState* o)
{
	LOG_MSG("In ConditionalMapFrame::update(TimeState* o)");
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

void ConditionalMapFrame::OnNewCustomCatClassifA()
{
	((ConditionalMapCanvas*) template_canvas)->NewCustomCatClassifMap();
}

void ConditionalMapFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, 4, cc_title);
}

void ConditionalMapFrame::OnThemeless()
{
	ChangeThemeType(CatClassification::no_theme, 1);
}

void ConditionalMapFrame::OnHinge15()
{
	ChangeThemeType(CatClassification::hinge_15, 6);
}

void ConditionalMapFrame::OnHinge30()
{
	ChangeThemeType(CatClassification::hinge_30, 6);
}

void ConditionalMapFrame::OnQuantile(int num_cats)
{
	ChangeThemeType(CatClassification::quantile, num_cats);
}

void ConditionalMapFrame::OnPercentile()
{
	ChangeThemeType(CatClassification::percentile, 6);
}

void ConditionalMapFrame::OnStdDevMap()
{
	ChangeThemeType(CatClassification::stddev, 6);
}

void ConditionalMapFrame::OnUniqueValues()
{
	ChangeThemeType(CatClassification::unique_values, 6);
}

void ConditionalMapFrame::OnNaturalBreaks(int num_cats)
{
	ChangeThemeType(CatClassification::natural_breaks, num_cats);
}

void ConditionalMapFrame::OnEqualIntervals(int num_cats)
{
	ChangeThemeType(CatClassification::equal_intervals, num_cats);
}

void ConditionalMapFrame::OnSaveCategories()
{
	((ConditionalMapCanvas*) template_canvas)->OnSaveCategories();
}

void ConditionalMapFrame::ChangeThemeType(
								CatClassification::CatClassifType new_theme,
								int num_categories,
								const wxString& custom_classif_title)
{
	ConditionalMapCanvas* cc = (ConditionalMapCanvas*) template_canvas;
	cc->ChangeCatThemeType(new_theme, num_categories, custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	template_legend->Refresh();
}
