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
#include <cfloat>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
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
#include "PCPNewView.h"

IMPLEMENT_CLASS(PCPCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(PCPCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(PCPCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

PCPCanvas::PCPCanvas(wxWindow *parent, TemplateFrame* t_frame,
                     Project* project_s,
                     const std::vector<GdaVarTools::VarInfo>& v_info,
                     const std::vector<int>& col_ids,
                     const wxPoint& pos, const wxSize& size)
:TemplateCanvas(parent, t_frame, project_s, project_s->GetHighlightState(),
                pos, size, false, true),
var_info(v_info), num_obs(project_s->GetNumRecords()),
num_time_vals(1), num_vars(v_info.size()),
data(v_info.size()), data_undef(v_info.size()),
custom_classif_state(0),
display_stats(false), show_axes(true), standardized(false),
pcp_selectstate(pcp_start), show_pcp_control(false),
overall_abs_max_std_exists(false), theme_var(0),
num_categories(6), all_init(false)
{
	using namespace Shapefile;
	LOG_MSG("Entering PCPCanvas::PCPCanvas");
    
	TableInterface* table_int = project->GetTableInt();
	data_stats.resize(num_vars);
  
    // get undefined and filter data by undefined if needed
    int max_ts = 1;
	for (int v=0; v<num_vars; v++) {
        table_int->GetColUndefined(col_ids[v], data_undef[v]);
        int ts = data_undef[v].shape()[0];
        if (ts > max_ts)
            max_ts = ts;
    }
    undef_markers.resize(max_ts);
    
    for (int t=0; t<max_ts; t++) {
        undef_markers[t].resize(num_obs, false);
        
        for (int i=0; i<num_obs; i++) {
            for (int v=0; v<num_vars; v++) {
                int ts = data_undef[v].shape()[0];
                if ( t < ts) 
                    undef_markers[t][i] = undef_markers[t][i] ||
                                          data_undef[v][t][i];
            }
        }
    }
   
    // get statistics for each variable (times)
	for (int v=0; v<num_vars; v++) {
		table_int->GetColData(col_ids[v], data[v]);
        table_int->GetColUndefined(col_ids[v], data_undef[v]);
		int data_times = data[v].shape()[0];
		data_stats[v].resize(data_times);
        
        std::vector<double> temp_vec;
        
		for (int t=0; t<data_times; t++) {
			for (int i=0; i<num_obs; i++) {
                // only use valid data for stats
                if (undef_markers[t][i] == false)
                    temp_vec.push_back(data[v][t][i]);
			}
			data_stats[v][t].CalculateFromSample(temp_vec);
			double min = data_stats[v][t].min;
			double max = data_stats[v][t].max;
			if (min != max) {
				double mean = data_stats[v][t].mean;
				double sd = data_stats[v][t].sd_with_bessel;
				double s_min = (min - mean)/sd;
				double s_max = (max - mean)/sd;
				double abs_max =
					GenUtils::max<double>(GenUtils::abs<double>(s_min),
										  GenUtils::abs<double>(s_max));
				if (!overall_abs_max_std_exists) {
					overall_abs_max_std_exists = true;
					overall_abs_max_std = abs_max;
				} else if (abs_max > overall_abs_max_std) {
					overall_abs_max_std = abs_max;
				}
			}
		}
	}
	
	template_frame->ClearAllGroupDependencies();
	for (int i=0, sz=var_info.size(); i<sz; ++i) {
		template_frame->AddGroupDependancy(var_info[i].name);
	}
	
	control_labels.resize(num_vars);
	control_circs.resize(num_vars);
	control_lines.resize(num_vars);
	var_order.resize(num_vars);
    
    for (int v=0; v<num_vars; v++) {
        var_order[v] = v;
    }
    
	selectable_fill_color = GdaConst::pcp_line_color;
	highlight_color = GdaConst::highlight_color;
	
    last_scale_trans.SetFixedAspectRatio(false);
	use_category_brushes = true;
	selectable_shps_type = polylines;
	
    ChangeThemeType(CatClassification::no_theme, 6);
	
	all_init = true;
	DisplayStatistics(true);
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

PCPCanvas::~PCPCanvas()
{
	highlight_state->removeObserver(this);
	if (custom_classif_state) custom_classif_state->removeObserver(this);
}

void PCPCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((PCPFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_PCP_NEW_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void PCPCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (size_t i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) {
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
			menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}

	//menu->Prepend(wxID_ANY, "Scale Options", menu2, "Scale Options");
    menu->AppendSeparator();
    menu->Append(wxID_ANY, "Time Variable Options", menu1,
                 "Time Variable Options");
}

void PCPCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_ORIGINAL_DATA"),
								  !standardized);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_STANDARDIZED_DATA"),
								  standardized);
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
								  GetCcType() == CatClassification::no_theme);	
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_1"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_2"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_3"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_4"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_5"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_6"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_7"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_8"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_9"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_QUANTILE_10"),
								  (GetCcType() == CatClassification::quantile)
								  && GetNumCats() == 10);
	
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
								  GetCcType() == CatClassification::percentile);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_15"),
								  GetCcType() == CatClassification::hinge_15);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_30"),
								  GetCcType() == CatClassification::hinge_30);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
								  GetCcType() == CatClassification::stddev);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
								  GetCcType() == CatClassification::unique_values);
    // since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_1"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_2"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_3"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_4"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_5"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_6"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_7"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_8"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_9"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_10"),
								  (GetCcType() ==
								   CatClassification::equal_intervals)
								  && GetNumCats() == 10);
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_1"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_2"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_3"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_4"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_5"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_6"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_7"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_8"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_9"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_10"),
								  (GetCcType() ==
								   CatClassification::natural_breaks)
								  && GetNumCats() == 10);
}


wxString PCPCanvas::GetCanvasTitle()
{
	wxString s = _("Parallel Coordinate Plot: ");
	s << GetNameWithTime(var_order[0]) << ", ";
	if (num_vars > 2) s << "..., ";
	s << GetNameWithTime(var_order[num_vars-1]);
	return s;
}

wxString PCPCanvas::GetCategoriesTitle()
{
	wxString s;
	if (GetCcType() == CatClassification::no_theme) {
		s << "Themeless";
	} else if (GetCcType() == CatClassification::custom) {
		s << cat_classif_def.title << ": " << GetNameWithTime(theme_var);
	} else {
		s << CatClassification::CatClassifTypeToString(GetCcType());
		s << ": " << GetNameWithTime(theme_var);
	}
	return s;
}


wxString PCPCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= (int)var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

void PCPCanvas::NewCustomCatClassif()
{
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		Gda::dbl_int_pair_vec_type cat_var_sorted(num_obs);
        std::vector<bool> var_undefs(num_obs, false);
        
		for (int i=0; i<num_obs; i++) {
			int t = cat_data.GetCurrentCanvasTmStep();
			int tm = var_info[theme_var].is_time_variant ? t : 0;
            int ts = tm+var_info[theme_var].time_min;
            
			cat_var_sorted[i].first = data[theme_var][ts][i];
			cat_var_sorted[i].second = i;
            
            var_undefs[i] = var_undefs[i] || data_undef[theme_var][ts][i];
		}
		 // only sort data with valid data
		if (cats_valid[var_info[theme_var].time]) {
			std::sort(cat_var_sorted.begin(), cat_var_sorted.end(),
					  Gda::dbl_int_pair_cmp_less);
		}
		
		CatClassification::ChangeNumCats(cat_classif_def.num_cats,
										 cat_classif_def);
		std::vector<wxString> temp_cat_labels; // will be ignored
		CatClassification::SetBreakPoints(cat_classif_def.breaks,
										  temp_cat_labels,
										  cat_var_sorted,
                                          var_undefs,
										  cat_classif_def.cat_classif_type,
										  cat_classif_def.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def.num_cats; i++) {
			cat_classif_def.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def.names[i] = cat_data.GetCategoryLabel(time, i);
		}
		int col = project->GetTableInt()->FindColId(var_info[theme_var].name);
		int tm = var_info[theme_var].time;
		cat_classif_def.assoc_db_fld_name = 
			project->GetTableInt()->GetColName(col, tm);
	}
	
	CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame(this->useScientificNotation);
	if (!ccf) return;
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def, "",
										  var_info[0].name, var_info[0].time);
	if (!ccs) return;
	if (custom_classif_state) custom_classif_state->removeObserver(this);
	cat_classif_def = ccs->GetCatClassif();
	custom_classif_state = ccs;
	custom_classif_state->registerObserver(this);
	//wxString s;
	//CatClassification::PrintCatClassifDef(cat_classif_def, s);
	//LOG_MSG(s);
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
void
PCPCanvas::ChangeThemeType(CatClassification::CatClassifType new_cat_theme,
                           int num_categories_s,
                           const wxString& custom_classif_title)
{
	num_categories = num_categories_s;
	
	if (new_cat_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm) return;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs) return;
		if (custom_classif_state == new_ccs) return;
		if (custom_classif_state) custom_classif_state->removeObserver(this);
		custom_classif_state = new_ccs;
		custom_classif_state->registerObserver(this);
		cat_classif_def = custom_classif_state->GetCatClassif();
	} else {
		if (custom_classif_state) custom_classif_state->removeObserver(this);
		custom_classif_state = 0;
	}
	cat_classif_def.cat_classif_type = new_cat_theme;
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
   
    ResetFadedLayer();
    
	if (all_init && template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

void PCPCanvas::update(CatClassifState* o)
{
	cat_classif_def = o->GetCatClassif();
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

void PCPCanvas::OnSaveCategories()
{
	wxString t_name;
	if (GetCcType() == CatClassification::custom) {
		t_name = cat_classif_def.title;
	} else {
		t_name = CatClassification::CatClassifTypeToString(GetCcType());
	}
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
    
    std::vector<bool> undefs(num_obs, false);
    
    for (size_t i=0; i<undef_markers.size(); i++) {
        for (size_t j=0; j<undef_markers[i].size(); j++) {
            undefs[j] = undefs[j] || undef_markers[i][j];
        }
    }
    
	SaveCategories(title, label, "CATEGORIES", undefs);
}

void PCPCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	wxSize size(GetVirtualSize());
	double x_min = 0;
	double x_max = 100;
	double y_min = 0;
	double y_max = 100;

    int virtual_screen_marg_bottom = 0;
	if (!display_stats && !standardized) {
		virtual_screen_marg_bottom = 25;
	} else if (!display_stats && standardized) {
		virtual_screen_marg_bottom = 33;
	} else {
		virtual_screen_marg_bottom =  25+25;
	}
	
    last_scale_trans.SetData(0, 0, 100, 100);
    last_scale_trans.SetMargin(25,virtual_screen_marg_bottom, 135, 25);
    last_scale_trans.SetView(size.GetWidth(), size.GetHeight());
    
	selectable_shps.resize(num_obs);
    selectable_shps_undefs.resize(num_obs);
	
	GdaShape* s = 0;
	wxRealPoint* pts = new wxRealPoint[num_vars];
	double std_fact = 1;
	if (overall_abs_max_std_exists) std_fact = 100.0/(2.0*overall_abs_max_std);
	double nvf = 100.0/((double) (num_vars-1));
    
	for (int i=0; i<num_obs; i++) {
        bool valid_line = true;
		for (int v=0; v<num_vars; v++) {
			int vv = var_order[v];
			int t = var_info[vv].time;
            
            if (undef_markers[t][i])
                valid_line = false;
            
			double min = data_stats[vv][t].min;
			double max = data_stats[vv][t].max;
			if (min == max) {
				pts[v].x = (x_max-x_min)/2.0;
			} else if (!standardized) {
				double rng = (var_info[vv].fixed_scale ? 
							  (var_info[vv].max_over_time - 
							   var_info[vv].min_over_time) : max-min);
				pts[v].x = 100.0*((data[vv][t][i]-min) / rng);
			} else  {
				double mean = data_stats[vv][t].mean;
				double sd = data_stats[vv][t].sd_with_bessel;
				pts[v].x = ((data[vv][t][i]-mean)/sd)+overall_abs_max_std;
				pts[v].x *= std_fact;
			}
			pts[v].y = 100.0-(nvf*((double) v));
		}
        selectable_shps_undefs[i] = !valid_line;
        selectable_shps[i] = new GdaPolyLine(num_vars, pts);
	}
	wxPen control_line_pen(GdaConst::pcp_horiz_line_color);
	control_line_pen.SetWidth(2);
	for (int v=0; v<num_vars; v++) {
		int y_del = display_stats ? -8 : 0;
		int vv = var_order[v];
		int t = var_info[vv].time;
		double y_pos = 100.0-(nvf*((double) v));
		s = new GdaPolyLine(0, y_pos, 100, y_pos);
		s->setPen(control_line_pen);
		foreground_shps.push_back(s);
		control_lines[v] = (GdaPolyLine*) s;
		s = new GdaRay(wxRealPoint(0, y_pos), 180, 10);
		s->setPen(control_line_pen);
		foreground_shps.push_back(s);
		s = new GdaCircle(wxRealPoint(0, y_pos), 3.0);
		s->setNudge(-10, 0);
		s->setPen(control_line_pen);
		s->setBrush(*wxWHITE_BRUSH);
		foreground_shps.push_back(s);
		control_circs[v] = (GdaCircle*) s;
        s = new GdaShapeText(GetNameWithTime(vv), *GdaConst::small_font,
                             wxRealPoint(0, y_pos), 0, GdaShapeText::right,
                             GdaShapeText::v_center, -25, 0+y_del);
		foreground_shps.push_back(s);
		control_labels[v] = (GdaShapeText*) s;
		wxString m;
		double t_min = data_stats[vv][t].min;
		double t_max = data_stats[vv][t].max;
		double t_mean = data_stats[vv][t].mean;
		double t_sd = (t_min == t_max ? 0 : data_stats[vv][t].sd_with_bessel);
		if (standardized) {
			if (t_min == t_max) {
				t_min = 0;
				t_max = 0;
				t_sd = 0;
			} else {
				double mean = data_stats[vv][t].mean;
				double sd = data_stats[vv][t].sd_with_bessel;
				t_min = (t_min-mean)/sd;
				t_max = (t_max-mean)/sd;
				t_sd = 1;
			}
			t_mean = 0;
		}
		
		if (display_stats) {
			m << "[" << GenUtils::DblToStr(t_min, 4);
			m << ", " << GenUtils::DblToStr(t_max, 4) << "]";
			s = new GdaShapeText(m, *GdaConst::small_font, wxRealPoint(0, y_pos), 0,
						   GdaShapeText::right, GdaShapeText::v_center, -25, 15+y_del);
			foreground_shps.push_back(s);
			int cols = 2;
			int rows = 2;
			std::vector<wxString> vals(rows*cols);
			vals[0] << "mean";
			vals[1] << GenUtils::DblToStr(t_mean, 4);
			vals[2] << "s.d.";
			vals[3] << GenUtils::DblToStr(t_sd, 4);
			std::vector<GdaShapeTable::CellAttrib> attribs(0); // undefined
			s = new GdaShapeTable(vals, attribs, rows, cols, *GdaConst::small_font,
							wxRealPoint(0, y_pos), GdaShapeText::right,
							GdaShapeText::top, GdaShapeText::right, GdaShapeText::v_center,
							3, 7, -25, 25+y_del);
			foreground_shps.push_back(s);
		}
	}
	if (standardized) {
		// add dotted lines and labels for sd and mean
		// add dotted line for mean in center
		s = new GdaPolyLine(50, 0, 50, 100);
		s->setPen(*GdaConst::scatterplot_origin_axes_pen);
		foreground_shps.push_back(s);
		s = new GdaShapeText(wxString::Format("%d", 0),
					   *GdaConst::small_font, wxRealPoint(50, 0), 0,
					   GdaShapeText::h_center, GdaShapeText::v_center, 0, 12);
		foreground_shps.push_back(s);
		int sd_abs = overall_abs_max_std;
		for (int i=1; i<=sd_abs && overall_abs_max_std_exists; i++) {
			double sd_p = (double) i;
			sd_p += overall_abs_max_std;
			sd_p *= std_fact;
			double sd_m = (double) -i;
			sd_m += overall_abs_max_std;
			sd_m *= std_fact;
			s = new GdaPolyLine(sd_p, 0, sd_p, 100);
			s->setPen(*GdaConst::scatterplot_origin_axes_pen);
			foreground_shps.push_back(s);
			s = new GdaShapeText(wxString::Format("%d", i),
						   *GdaConst::small_font, wxRealPoint(sd_p, 0), 0,
						   GdaShapeText::h_center, GdaShapeText::v_center, 0, 12);
			foreground_shps.push_back(s);
			s = new GdaPolyLine(sd_m, 0, sd_m, 100);
			s->setPen(*GdaConst::scatterplot_origin_axes_pen);
			foreground_shps.push_back(s);
			s = new GdaShapeText(wxString::Format("%d", -i),
						   *GdaConst::small_font, wxRealPoint(sd_m, 0), 0,
						   GdaShapeText::h_center, GdaShapeText::v_center, 0, 12);
			foreground_shps.push_back(s);
		}
	}
	
	delete [] pts;
	
	ResizeSelectableShps();
}

void PCPCanvas::TimeChange()
{
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
	for (int v=0; v<num_vars; v++) {
		if (var_info[v].sync_with_global_time) {
			var_info[v].time = ref_time + var_info[v].ref_time_offset;
		}
	}
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void PCPCanvas::VarInfoAttributeChange()
{
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (size_t i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	template_frame->SetDependsOnNonSimpleGroups(is_any_time_variant);
	ref_var_index = -1;
	num_time_vals = 1;
	for (size_t i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	//GdaVarTools::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_categories and ref_var_index */
void PCPCanvas::CreateAndUpdateCategories()
{
	cats_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_valid[t] = true;
	cats_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_error_message[t] = wxEmptyString;
	
	if (GetCcType() == CatClassification::no_theme) {
		// 1 = #cats
		CatClassification::ChangeNumCats(1, cat_classif_def);
		cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
		cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t, 0,GdaConst::map_default_fill_colour);
			cat_data.SetCategoryLabel(t, 0, "");
			cat_data.SetCategoryCount(t, 0, num_obs);
			for (int i=0; i<num_obs; i++) cat_data.AppendIdToCategory(t, 0, i);
		}
		if (ref_var_index != -1) {
			cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
											- var_info[ref_var_index].time_min);
		}
		return;
	}
	
	// Everything below assumes that GetCcType() != no_theme
	std::vector<Gda::dbl_int_pair_vec_type> cat_var_sorted(num_time_vals);
    std::vector<std::vector<bool> > cat_var_undef;
    

	for (int t=0; t<num_time_vals; t++) {
        std::vector<bool> undefs(num_obs, false);
		// Note: need to be careful here: what about when a time variant
		// variable is not synced with time?  time_min should reflect this,
		// so possibly ok.
		cat_var_sorted[t].resize(num_obs);
		for (int i=0; i<num_obs; i++) {
			int tm = var_info[theme_var].is_time_variant ? t : 0;
			cat_var_sorted[t][i].first = 
				data[theme_var][tm+var_info[theme_var].time_min][i];
			cat_var_sorted[t][i].second = i;
            
            undefs[i] = undefs[i] || data_undef[theme_var][tm+var_info[theme_var].time_min][i];
		}
        cat_var_undef.push_back(undefs);
	}	
	
	// Sort each vector in ascending order
	for (int t=0; t<num_time_vals; t++) {
		if (cats_valid[t]) { // only sort data with valid data
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	}
	
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def);
	}
	cat_classif_def.color_scheme =
		CatClassification::GetColSchmForType(cat_classif_def.cat_classif_type);
	CatClassification::PopulateCatClassifData(cat_classif_def,
											  cat_var_sorted,
                                              cat_var_undef,
											  cat_data, cats_valid,
											  cats_error_message,
                                              this->useScientificNotation);
	
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int cnc = cat_data.GetNumCategories(cat_data.GetCurrentCanvasTmStep());
	CatClassification::ChangeNumCats(cnc, cat_classif_def);
}

void PCPCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPCanvas::FixedScaleVariableToggle(int var_index)
{
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPCanvas::ShowAxes(bool show_axes_s)
{
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void PCPCanvas::StandardizeData(bool standardize)
{
	if (standardize == standardized) return;
	standardized = standardize;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

// wxMouseEvent notes:
// LeftDown(): true when the left button is first pressed down
// LeftIsDown(): true while the left button is down. During a mouse dragging
//  operation, this will continue to return true, while LeftDown() is false.
// RightDown/RightIsDown: similar to Left.
// Moving(): returns true when mouse is moved, but no buttons are pressed.
// Dragging(): returns true when mouse is moved and at least one mouse button is
//   pressed.
// CmdDown(): Checks if MetaDown under Mac and ControlDown on other platforms.
// ButtonDClick(int but = wxMOUSE_BTN_ANY): checks for double click of any
//   button. Can also specify wxMOUSE_BTN_LEFT / RIGHT / MIDDLE.  Or
//   LeftDCLick(), etc.
// LeftUp(): returns true at the moment the button changed to up.

void PCPCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Capture the mouse when left mouse button is down.
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();

	if ((mousemode != select) ||
		(mousemode == select && selectstate != start))
    {
		show_pcp_control = false;
		TemplateCanvas::OnMouseEvent(event);
		return;
	}
	
	// To create the impression of an outline control moving, we will
	// create / delete an outline object in the foreground_objects.
	// We can tell the system that just the foreground objects need
	// to be redrawn at each step, or we can possibly manually draw
	// the foreground objects ourselves.
	
	if (pcp_selectstate == pcp_start) {
		if (event.LeftDown()) {
			
			// if the mouse position is at one of the control dots, then
			// proceed, otherwise call TemplateCanvas::OnMouseEvent(event)

			int label_match = -1;
			pcp_prev = GetActualPos(event);
			pcp_sel1 = pcp_prev;
			for (int v=0; v<num_vars; v++) {
				if (control_labels[v]->pointWithin(pcp_sel1)) {
					label_match = v;
					break;
				}
			}

			int circ_match = -1;
			for (int v=0; v<num_vars && label_match==-1; v++) {
				wxPoint cpt = control_circs[v]->center;
				cpt.x += control_circs[v]->getXNudge();
				cpt.y += control_circs[v]->getYNudge();
				if (GenUtils::distance(pcp_sel1, cpt) <=
					((double) control_circs[v]->radius)+1.5) {
					circ_match = v;
					break;
				}
			}
			
			if (label_match != -1) {
				control_label_sel = label_match;
				pcp_selectstate = pcp_leftdown_on_label;
                is_showing_brush = false;
			} else if (circ_match != -1) {
				control_line_sel = circ_match;
				pcp_selectstate = pcp_leftdown_on_circ;
                is_showing_brush = false;
			} else {
				show_pcp_control = false;
				TemplateCanvas::OnMouseEvent(event);
				return;
			}
		} else {
			show_pcp_control = false;
			TemplateCanvas::OnMouseEvent(event);
			return;
		}
	} else if (pcp_selectstate == pcp_leftdown_on_label) {

	} else if (pcp_selectstate == pcp_leftdown_on_circ) {
		if (event.Moving() || event.Dragging()) {
			wxPoint act_pos = GetActualPos(event);
			if (fabs((double) (pcp_prev.x - act_pos.x)) +
				fabs((double) (pcp_prev.y - act_pos.y)) > 2) {
				pcp_sel1 = pcp_prev;
				pcp_sel2 = GetActualPos(event);
				pcp_selectstate = pcp_dragging;
				
				show_pcp_control = true;
				Refresh();
			}
		} else {
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
			Refresh();
		}
	} else if (pcp_selectstate == pcp_dragging) {
		if (event.Dragging()) { // mouse moved while buttons still down
			pcp_sel2 = GetActualPos(event);
			
			show_pcp_control = true;
			Refresh();
		} else if (event.LeftUp()) {
			pcp_sel2 = GetActualPos(event);
			MoveControlLine(pcp_sel2.y); // will invalidate layer1 if needed
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
            ResetBrushing();
			Refresh();
		}  else if (event.RightDown()) {
			show_pcp_control = false;
			pcp_selectstate = pcp_start;
			Refresh();
		}			
	}
}


void PCPCanvas::VarLabelClicked()
{
	int v = var_order[control_label_sel];
	wxString msg;
	msg << "control_label_sel " << control_label_sel << " clicked which";
	msg << "\n corresponds to actual var " << v << " with name = ";
	msg << GetNameWithTime(v);
	theme_var = v;
	ChangeThemeType(GetCcType(), GetNumCats());
	TemplateLegend* tl = template_frame->GetTemplateLegend();
	if (tl) tl->Refresh();
}

void PCPCanvas::PaintControls(wxDC& dc)
{
	if (!show_pcp_control) return;
	// draw control line
	wxPen pen(*wxBLUE_PEN);
	pen.SetWidth(2);
	dc.SetPen(pen);
	dc.SetBrush(*wxWHITE_BRUSH);
	wxPoint cpt = control_circs[control_line_sel]->center;
	cpt.x += control_circs[control_line_sel]->getXNudge();
	cpt.y = pcp_sel2.y;
	int x_end = control_lines[control_line_sel]->points[1].x;
	
	dc.DrawLine(cpt.x, cpt.y, x_end, cpt.y);
	dc.DrawCircle(cpt, control_circs[control_line_sel]->radius);
}

/**
 Determines final location of control.  If order changes, update
 var_order, invalidate bitmaps and call PopulateCanvas.
 */
void PCPCanvas::MoveControlLine(int final_y)
{
	LOG_MSG("Entering PCPCanvas::MoveControlLine");
	LOG(control_line_sel);
	
	LOG_MSG("original var_order");
	for (int i=0; i<num_vars; i++) LOG(var_order[i]);
	
	std::vector<int> new_order(num_vars);
	// starting line is control_line_sel
	// determine which control lines final_y is between
	if (final_y < control_lines[0]->points[0].y) {
		if (control_line_sel == 0) return;
		LOG_MSG("Final control line pos is above control line 0");
		// move control line into first position
		new_order[0] = control_line_sel;
		for (int i=1; i<=control_line_sel; i++) new_order[i] = i-1;
		for (int i=control_line_sel+1; i<num_vars; i++) new_order[i] = i;
		//for (int i=0; i<num_vars; i++) LOG(new_order[i]);
	} else if (final_y > control_lines[num_vars-1]->points[0].y) {
		if (control_line_sel == num_vars - 1) return;
		LOG_MSG("Final control line pos is below last control line");
		// move control line into last position
		for (int i=0; i<control_line_sel; i++) new_order[i] = i;
		for (int i=control_line_sel; i<num_vars-1; i++) new_order[i] = i+1;
		new_order[num_vars-1] = control_line_sel;
	} else {
		for (int v=1; v<num_vars; v++) {
			if (final_y < control_lines[v]->points[0].y) {
				if (control_line_sel == v || control_line_sel == v-1) return;
				LOG_MSG(wxString::Format("Final control line pos is just "
										 "above control line %d", v));
				
				if (control_line_sel > v) {
					for (int i=0; i<v; i++) new_order[i] = i;
					new_order[v] = control_line_sel;
					for (int i=v+1; i<=control_line_sel; i++) new_order[i]=i-1;
					for (int i=control_line_sel+1; i<num_vars; i++) {
						new_order[i] = i;
					}
				} else {
					for (int i=0; i<control_line_sel; i++) new_order[i] = i;
					for (int i=control_line_sel; i<v-1; i++) new_order[i]=i+1;
					new_order[v-1] = control_line_sel;
					for (int i=v; i<num_vars; i++) new_order[i] = i;
				}
				break;
			}
		}
	}
	std::vector<int> old_var_order(num_vars);
	for (int i=0; i<num_vars; i++) old_var_order[i] = var_order[i];
	
	LOG_MSG("control lines reorder: ");
	for (int i=0; i<num_vars; i++) LOG(new_order[i]);
	
	for (int i=0; i<num_vars; i++) {
		var_order[i] = old_var_order[new_order[i]];
	}
	LOG_MSG("final var_order:");
	for (int i=0; i<num_vars; i++) LOG(var_order[i]);
	
	invalidateBms();
	PopulateCanvas();
}

CatClassification::CatClassifType PCPCanvas::GetCcType()
{
	return cat_classif_def.cat_classif_type;
}

void PCPCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
    
	wxString s;
    int t = cat_data.GetCurrentCanvasTmStep();
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    
    if (highlight_state->GetTotalHighlighted()> 0) {
        int n_total_hl = highlight_state->GetTotalHighlighted();
        s << "#selected=" << n_total_hl << "  ";
        
        int n_undefs = 0;
        for (int i=0; i<num_obs; i++) {
            if ( undef_markers[t][i] && hl[i]) {
                n_undefs += 1;
            }
        }
        if (n_undefs> 0) {
            s << "(undefined:" << n_undefs << ") ";
        }
    }
    

	if (mousemode == select && selectstate == start) {
		// obs: 1,3,5,... obs 1 = (1.23, 432.3, -23)
		if (total_hover_obs > 1) {
			s << "obs: " << hover_obs[0]+1 << "," << hover_obs[1]+1;
			if (total_hover_obs > 2) s << "," << hover_obs[2]+1;
			if (total_hover_obs > 3) s << "," << hover_obs[3]+1;
			if (total_hover_obs > 4) s << "," << hover_obs[4]+1;
			if (total_hover_obs > 5) s << ",...";
			s << " ";
		}
		if (total_hover_obs != 0) {
			int ob = hover_obs[0];
			s << "obs " << ob+1 << " = (";
			for (int v=0; v<num_vars-1; v++) {
				int t = var_info[var_order[v]].time;
				s << GenUtils::DblToStr(data[var_order[v]][t][ob], 3);
				s << ", ";
			}
			int t = var_info[var_order[num_vars-1]].time;
			s << GenUtils::DblToStr(data[var_order[num_vars-1]][t][ob],3);
			s << ")";
		}
	}
	sb->SetStatusText(s);
}


PCPLegend::PCPLegend(wxWindow *parent, TemplateCanvas* t_canvas,
						   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

PCPLegend::~PCPLegend()
{
    LOG_MSG("In PCPLegend::~PCPLegend");
}

IMPLEMENT_CLASS(PCPFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(PCPFrame, TemplateFrame)
	EVT_ACTIVATE(PCPFrame::OnActivate)
END_EVENT_TABLE()

PCPFrame::PCPFrame(wxFrame *parent, Project* project,
								 const std::vector<GdaVarTools::VarInfo>& var_info,
								 const std::vector<int>& col_ids,
								 const wxString& title,
								 const wxPoint& pos,
								 const wxSize& size,
								 const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	wxLogMessage("Open PCPFrame.");
	
	int width, height;
	GetClientSize(&width, &height);

	wxSplitterWindow* splitter_win = 0;
	splitter_win = new wxSplitterWindow(this,-1,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    wxPanel* rpanel = new wxPanel(splitter_win);
	template_canvas = new PCPCanvas(rpanel, this, project,
									   var_info, col_ids,
									   wxDefaultPosition,
									   wxSize(width,height));

	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	
    wxPanel* lpanel = new wxPanel(splitter_win);
	template_legend = new PCPLegend(lpanel, template_canvas,
									   wxPoint(0,0), wxSize(0,0));
	wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);
    
	splitter_win->SplitVertically(lpanel, rpanel,
								GdaConst::bubble_chart_default_legend_width);
	
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(splitter_win, 1, wxEXPAND|wxALL);
    SetSizer(sizer);
    splitter_win->SetSize(wxSize(width,height));
    SetAutoLayout(true);
	Show(true);
}

PCPFrame::~PCPFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void PCPFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("PCPFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void PCPFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_PCP_NEW_PLOT_VIEW_MENU_OPTIONS");
	((PCPCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	((PCPCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void PCPFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((PCPCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void PCPFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void PCPFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
}

void PCPFrame::OnNewCustomCatClassifA()
{
	((PCPCanvas*) template_canvas)->NewCustomCatClassif();
}

void PCPFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, 4, cc_title);
}

void PCPFrame::OnShowAxes(wxCommandEvent& event)
{
	PCPCanvas* t = (PCPCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void PCPFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	PCPCanvas* t = (PCPCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void PCPFrame::OnViewOriginalData(wxCommandEvent& event)
{
	PCPCanvas* t = (PCPCanvas*) template_canvas;
	t->StandardizeData(false);
	UpdateOptionMenuItems();
}

void PCPFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	PCPCanvas* t = (PCPCanvas*) template_canvas;
	t->StandardizeData(true);
	UpdateOptionMenuItems();
}

void PCPFrame::OnThemeless()
{
	ChangeThemeType(CatClassification::no_theme, 1);
}

void PCPFrame::OnHinge15()
{
	ChangeThemeType(CatClassification::hinge_15, 6);
}

void PCPFrame::OnHinge30()
{
	ChangeThemeType(CatClassification::hinge_30, 6);
}

void PCPFrame::OnQuantile(int num_cats)
{
	ChangeThemeType(CatClassification::quantile, num_cats);
}

void PCPFrame::OnPercentile()
{
	ChangeThemeType(CatClassification::percentile, 6);
}

void PCPFrame::OnStdDevMap()
{
	ChangeThemeType(CatClassification::stddev, 6);
}

void PCPFrame::OnUniqueValues()
{
	ChangeThemeType(CatClassification::unique_values, 6);
}

void PCPFrame::OnNaturalBreaks(int num_cats)
{
	ChangeThemeType(CatClassification::natural_breaks, num_cats);
}

void PCPFrame::OnEqualIntervals(int num_cats)
{
	ChangeThemeType(CatClassification::equal_intervals, num_cats);
}

void PCPFrame::OnSaveCategories()
{
	((PCPCanvas*) template_canvas)->OnSaveCategories();
}

void PCPFrame::ChangeThemeType(CatClassification::CatClassifType new_theme,
                               int num_categories,
                               const wxString& custom_classif_title)
{
    ((PCPCanvas*) template_canvas)->ChangeThemeType(new_theme,
                                                    num_categories,
                                                    custom_classif_title);
    UpdateTitle();
    UpdateOptionMenuItems();
    if (template_legend) template_legend->Recreate();
}

