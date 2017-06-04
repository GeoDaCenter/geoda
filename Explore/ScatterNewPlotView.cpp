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
#include <boost/math/distributions/fisher_f.hpp>
#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/Lowess.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "ScatterNewPlotView.h"


BubbleSizeSliderDlg::BubbleSizeSliderDlg (ScatterNewPlotCanvas* _canvas,
                                          const wxString & caption )
: wxDialog( NULL, -1, caption, wxDefaultPosition, wxDefaultSize)
{
    
    wxLogMessage("Open BubbleSizeDlg.");
    
    canvas = _canvas;
    
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);
    
    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    double pos = canvas->bubble_size_scaler;
    pos = pos < 1 ? pos * 100.0 - 101 : (pos - 1.0) * 100.0;
	wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
    slider = new wxSlider(this, XRCID("ID_BUBBLE_SLIDER"), int(pos), -95, 80,
                          wxDefaultPosition, wxSize(200, -1),
                          wxSL_HORIZONTAL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"small"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);
    subSizer->Add(slider, 0, wxALIGN_CENTER_VERTICAL|wxALL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"large"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);
    
	boxSizer->Add(subSizer);
    resetBtn = new wxButton(this, XRCID("ID_RESET"), wxT("Reset"), wxDefaultPosition, wxSize(100, -1));
    topSizer->Add(resetBtn, 0, wxGROW|wxALL, 5);
    
    topSizer->Fit(this);
    
    Connect(XRCID("ID_BUBBLE_SLIDER"), wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler(BubbleSizeSliderDlg::OnSliderChange));
    Connect(XRCID("ID_RESET"), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(BubbleSizeSliderDlg::OnReset));
}

void BubbleSizeSliderDlg::OnReset(wxCommandEvent& event )
{
    wxLogMessage("In BubbleSizeSliderDlg::OnReset()");
    
    slider->SetValue(0);
    canvas->UpdateBubbleSize(1);
}
void BubbleSizeSliderDlg::OnSliderChange( wxScrollEvent & event )
{
    wxLogMessage("In BubbleSizeSliderDlg::OnSliderChange()");
    
    int val = event.GetInt();
    if (val == 0){
        canvas->UpdateBubbleSize(1);
        return;
    }
    
    double size_scaler = 0.0;
    
    if (val < 0)
        size_scaler = (100 + val) / 100.0;
    
    if (val > 0)
        size_scaler = 1.0 + val / 100.0;
    
    canvas->UpdateBubbleSize(size_scaler);
}

IMPLEMENT_CLASS(ScatterNewPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(ScatterNewPlotCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

/** This is a basic constructor that does not populate the canvas. It
 is intended to be used in derived classes. */
ScatterNewPlotCanvas::ScatterNewPlotCanvas(wxWindow *parent,
										   TemplateFrame* t_frame,
										   Project* project_s,
										   const wxPoint& pos,
										   const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s, project_s->GetHighlightState(),
                 pos, size, false, true),
project(project_s), num_obs(project_s->GetNumRecords()),
num_categories(1), num_time_vals(1),
custom_classif_state(0),
is_bubble_plot(false), axis_scale_x(), axis_scale_y(),
standardized(false), reg_line(0), lowess_reg_line(0), stats_table(0),
reg_line_selected(0), lowess_reg_line_selected(0), reg_line_selected_slope(0),
reg_line_selected_infinite_slope(false), reg_line_selected_defined(false),
reg_line_excluded(0), lowess_reg_line_excluded(0), reg_line_excluded_slope(0),
reg_line_excluded_infinite_slope(false), reg_line_excluded_defined(false),
x_axis_through_origin(0), y_axis_through_origin(0),
show_origin_axes(true), display_stats(false),
show_reg_selected(true), show_reg_excluded(true),
sse_c(0), sse_sel(0), sse_unsel(0),
chow_ratio(0), chow_pval(1), chow_valid(false), chow_test_text(0),
show_linear_smoother(true), show_lowess_smoother(false), enableLowess(true),
table_display_lines(0),
X(project_s->GetNumRecords()), Y(project_s->GetNumRecords()), Z(0),
obs_id_to_z_val_order(boost::extents[0][0]), all_init(false),
bubble_size_scaler(1.0)
{
	using namespace Shapefile;
	use_category_brushes = true;
	draw_sel_shps_by_z_val = false;
	highlight_color = GdaConst::scatterplot_regression_selected_color;
	selectable_fill_color = GdaConst::scatterplot_regression_excluded_color;
	selectable_outline_color = GdaConst::scatterplot_regression_color;
	
    last_scale_trans.SetMargin(25, 50, 50, 25);
    last_scale_trans.SetData(0, 0, 100, 100);
    
	UpdateDisplayLinesAndMargins();
	all_init = true;
		
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

/** This constructor is intended to be used directly for creating new
 scatter plots and bubble charts */
ScatterNewPlotCanvas::
ScatterNewPlotCanvas(wxWindow *parent,
                     TemplateFrame* t_frame,
                     Project* project_s,
                     const std::vector<GdaVarTools::VarInfo>& v_info,
                     const std::vector<int>& col_ids,
                     bool is_bubble_plot_s,
                     bool standardized_s,
                     const wxPoint& pos,
                     const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s,
                 project_s->GetHighlightState(),
                 pos, size, false, true),
project(project_s), var_info(v_info),
num_obs(project_s->GetNumRecords()),
num_categories(is_bubble_plot ? 1 : 3),
num_time_vals(1),
data(v_info.size()),
undef_data(v_info.size()),
custom_classif_state(0),
is_bubble_plot(is_bubble_plot_s),
axis_scale_x(), axis_scale_y(),
standardized(standardized_s), reg_line(0), lowess_reg_line(0), stats_table(0),
reg_line_selected(0), lowess_reg_line_selected(0), reg_line_selected_slope(0),
reg_line_selected_infinite_slope(false), reg_line_selected_defined(false),
reg_line_excluded(0), lowess_reg_line_excluded(0), reg_line_excluded_slope(0),
reg_line_excluded_infinite_slope(false), reg_line_excluded_defined(false),
x_axis_through_origin(0), y_axis_through_origin(0),
show_origin_axes(true), display_stats(!is_bubble_plot_s),
show_reg_selected(!is_bubble_plot_s), show_reg_excluded(!is_bubble_plot_s),
sse_c(0), sse_sel(0), sse_unsel(0),
show_linear_smoother(!is_bubble_plot_s),
show_lowess_smoother(false), enableLowess(true),
chow_ratio(0), chow_pval(1), chow_valid(false), chow_test_text(0),
table_display_lines(0),
X(project_s->GetNumRecords()),
Y(project_s->GetNumRecords()),
Z(is_bubble_plot_s ? project_s->GetNumRecords() : 0),
obs_id_to_z_val_order(boost::extents[0][0]), all_init(false),
bubble_size_scaler(1.0)
{
	using namespace Shapefile;

	TableInterface* table_int = project->GetTableInt();
	for (size_t i=0; i<var_info.size(); i++) {
		template_frame->AddGroupDependancy(var_info[i].name);
		table_int->GetColData(col_ids[i], data[i]);
        table_int->GetColUndefined(col_ids[i], undef_data[i]);
	}
	
	if (!is_bubble_plot) {
		highlight_color = GdaConst::scatterplot_regression_selected_color;
		selectable_fill_color = GdaConst::scatterplot_regression_excluded_color;
		selectable_outline_color = GdaConst::scatterplot_regression_color;
	}
    
	if (is_bubble_plot) {
		int timesteps_tbl = table_int->GetTimeSteps();
		GdaVarTools::VarInfo& sec_var_info = var_info[2];
		Gda::dbl_int_pair_vec_type v_sorted(num_obs);
		int times = sec_var_info.is_time_variant ? timesteps_tbl : 1;
		obs_id_to_z_val_order.resize(boost::extents[times][num_obs]);
		
		for (int t=0; t<times; t++) {
			for (int i=0; i<num_obs; i++) {
				v_sorted[i].first = data[2][t][i];
				v_sorted[i].second = i;
			}
			std::sort(v_sorted.begin(), v_sorted.end(),
					  Gda::dbl_int_pair_cmp_greater);
			for (int i=0; i<num_obs; i++) {
				obs_id_to_z_val_order[t][v_sorted[i].second] = i;
			}
		}
	}

    last_scale_trans.SetMargin(25, 50, 50, 25);
    last_scale_trans.SetData(0, 0, 100, 100);
	
	use_category_brushes = true;
	draw_sel_shps_by_z_val = is_bubble_plot;
    
	UpdateDisplayLinesAndMargins();
	
	if (is_bubble_plot) {
		ChangeThemeType(CatClassification::stddev, 6);
        
	} else {
		ref_var_index = -1;
		num_time_vals = 1;
		for (size_t i=0; i<var_info.size() && ref_var_index == -1; i++) {
            if (var_info[i].is_ref_variable) {
                ref_var_index = i;
            }
		}
		if (ref_var_index != -1) {
			num_time_vals = (var_info[ref_var_index].time_max -
							 var_info[ref_var_index].time_min) + 1;
		}
		
		// 1 = #cats
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t, 0, selectable_fill_color);
			for (int i=0; i<num_obs; i++) {
				cat_data.AppendIdToCategory(t, 0, i);
			}
		}
		if (ref_var_index != -1) {
            int cur_tm = var_info[ref_var_index].time - var_info[ref_var_index].time_min;
			cat_data.SetCurrentCanvasTmStep(cur_tm);
		}
		VarInfoAttributeChange();
		PopulateCanvas();
	}
	
	all_init = true;
	
	if (!is_bubble_plot_s) {
		UpdateDisplayStats();
		UpdateDisplayLinesAndMargins();
		ResizeSelectableShps();
	}
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

ScatterNewPlotCanvas::~ScatterNewPlotCanvas()
{
	EmptyLowessCache();
	highlight_state->removeObserver(this);
	if (custom_classif_state)
        custom_classif_state->removeObserver(this);
}
void ScatterNewPlotCanvas::UpdateBubbleSize(double size_scaler)
{
    bubble_size_scaler = size_scaler;
    this->PopulateCanvas();
    Refresh();
}

void ScatterNewPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ScatterNewPlotFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu;
	if (is_bubble_plot) {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_BUBBLE_CHART_VIEW_MENU_OPTIONS");
		TemplateCanvas::AppendCustomCategories(optMenu, project->GetCatClassifManager());
        optMenu->AppendSeparator();
        wxMenuItem* menu_item = optMenu->Append(XRCID("IDM_BUBBLE_SLIDER"), wxT("Adjust Bubble Size"));
        template_frame->Connect(XRCID("IDM_BUBBLE_SLIDER"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ScatterNewPlotFrame::AdjustBubbleSize));
	} else {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
	}
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void ScatterNewPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
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
	
	wxMenu* menu2 = new wxMenu(wxEmptyString);
	if (var_info[0].is_time_variant) {
		wxString s = _("Fixed x-axis scale over time");
		wxMenuItem* mi =
		menu2->AppendCheckItem(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR1, s, s);
		mi->Check(var_info[0].fixed_scale);
	}
	if (var_info[1].is_time_variant) {
		wxString s = _("Fixed y-axis scale over time");
		wxMenuItem* mi =
		menu2->AppendCheckItem(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR2, s, s);
		mi->Check(var_info[1].fixed_scale);
	}
	
	menu->Prepend(wxID_ANY, _("Scale Options"), menu2, _("Scale Options"));
    menu->AppendSeparator();
    menu->Append(wxID_ANY, _("Time Variable Options"), menu1,
				  _("Time Variable Options"));
}

void ScatterNewPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_STANDARDIZED_DATA"),
								  IsStandardized());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_ORIGINAL_DATA"),
								  !IsStandardized());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_LINEAR_SMOOTHER"), IsShowLinearSmoother());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_LOWESS_SMOOTHER"), IsShowLowessSmoother());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_VIEW_REGIMES_REGRESSION"), IsRegressionSelected());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"), IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"), IsShowOriginAxes());
	
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
    
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_VIEW_LOWESS_SMOOTHER"), enableLowess);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_EDIT_LOWESS_PARAMS"), enableLowess);
    
}

void ScatterNewPlotCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
    TemplateCanvas::UpdateSelection(shiftdown, pointsel);
    if (IsRegressionSelected() || IsRegressionExcluded()) {
        SmoothingUtils::CalcStatsRegimes(X, Y, XYZ_undef, XYZ_undef,
                                         statsX, statsY, regressionXY,
                                         highlight_state->GetHighlight(),
                                         statsXselected, statsYselected,
                                         statsXexcluded, statsYexcluded,
                                         regressionXYselected,
                                         regressionXYexcluded,
                                         sse_sel, sse_unsel);
        
        if (IsRegressionSelected()) {
            UpdateRegSelectedLine();
        }
        
        if (IsRegressionExcluded()) {
            UpdateRegExcludedLine();
        }
        
        if (IsShowLowessSmoother() && IsShowRegimes()) {
            UpdateLowessOnRegimes();
        }
    }
    
    if (IsDisplayStats() && IsShowLinearSmoother()) {
        UpdateDisplayStats();
    }

    if (IsRegressionSelected() || IsRegressionExcluded()) {
        // we only need to redraw everything if the optional
        // regression lines have changed.
        Refresh();
    }
    
}
/**
 Override of TemplateCanvas method.  We must still call the
 TemplateCanvas method after we update the regression lines
 as needed. */
void ScatterNewPlotCanvas::update(HLStateInt* o)
{
	if (IsRegressionSelected() || IsRegressionExcluded()) {
        SmoothingUtils::CalcStatsRegimes(X, Y, XYZ_undef, XYZ_undef,
                                         statsX, statsY, regressionXY,
                                         highlight_state->GetHighlight(),
                                         statsXselected, statsYselected,
                                         statsXexcluded, statsYexcluded,
                                         regressionXYselected,
                                         regressionXYexcluded,
                                         sse_sel, sse_unsel);
		
        if (IsRegressionSelected()) {
            UpdateRegSelectedLine();
        }
        
        if (IsRegressionExcluded()) {
            UpdateRegExcludedLine();
        }
        
		if (IsShowLowessSmoother() && IsShowRegimes()) {
			UpdateLowessOnRegimes();
		}
	}
    
    if (IsDisplayStats() && IsShowLinearSmoother()) {
        UpdateDisplayStats();
    }
	
	// Call TemplateCanvas::update to redraw objects as needed.
	TemplateCanvas::update(o);
	
	if (IsRegressionSelected() || IsRegressionExcluded()) {
		// we only need to redraw everything if the optional
		// regression lines have changed.
		Refresh();
	}
}

wxString ScatterNewPlotCanvas::GetCanvasTitle()
{
	wxString s(is_bubble_plot ? "Bubble Chart" : "Scatter Plot");	
	s << " - x: " << GetNameWithTime(0) << ", y: " << GetNameWithTime(1);
	if (is_bubble_plot) {
		s << ", size: " << GetNameWithTime(2);
		s << ", " << GetCategoriesTitle();
	}
	return s;
}

wxString ScatterNewPlotCanvas::GetCategoriesTitle()
{
	if (GetCcType() == CatClassification::no_theme) {
		return "Themeless";
	}
	wxString s;
	if (GetCcType() == CatClassification::custom) {
		s << cat_classif_def.title;
	} else {
		s << CatClassification::CatClassifTypeToString(GetCcType());
	}
	if (is_bubble_plot) {
		s << ": " << GetNameWithTime(3);
	}
	return s;
}

wxString ScatterNewPlotCanvas::GetNameWithTime(int var)
{
    if (var < 0 || var >= (int)var_info.size()) {
        return wxEmptyString;
    }
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

void ScatterNewPlotCanvas::NewCustomCatClassif()
{
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
        
		Gda::dbl_int_pair_vec_type cat_var_sorted(num_obs);
        std::vector<bool> var_undefs(num_obs, false);
        
		for (int i=0; i<num_obs; i++) {
			int t = cat_data.GetCurrentCanvasTmStep();
			int tm = var_info[3].is_time_variant ? t : 0;
            int ts = tm+var_info[3].time_min;
			cat_var_sorted[i].first =  data[3][ts][i];
			cat_var_sorted[i].second = i;
            
            var_undefs[i] = var_undefs[i] || undef_data[3][ts][i];
		}
        
		if (cats_valid[var_info[0].time]) { // only sort data with valid data
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
		int col = project->GetTableInt()->FindColId(var_info[3].name);
		int tm = var_info[3].time;
		cat_classif_def.assoc_db_fld_name = 
			project->GetTableInt()->GetColName(col, tm);
	}
	
	CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame(useScientificNotation);
	if (!ccf)
        return;
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def, "",
										  var_info[3].name,
										  var_info[3].time);
	if (!ccs) return;
	if (custom_classif_state) custom_classif_state->removeObserver(this);
	cat_classif_def = ccs->GetCatClassif();
	custom_classif_state = ccs;
	custom_classif_state->registerObserver(this);
    
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
ScatterNewPlotCanvas::
ChangeThemeType(CatClassification::CatClassifType new_theme,
                int num_categories_s,
                const wxString& custom_classif_title)
{
	num_categories = num_categories_s;
	
	if (new_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm)
            return;
        
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		
        if (!new_ccs)
            return;
		if (custom_classif_state == new_ccs)
            return;
		if (custom_classif_state)
            custom_classif_state->removeObserver(this);
        
		custom_classif_state = new_ccs;
		custom_classif_state->registerObserver(this);
		cat_classif_def = custom_classif_state->GetCatClassif();
        
	} else {
		if (custom_classif_state)
            custom_classif_state->removeObserver(this);
		custom_classif_state = 0;
	}
	cat_classif_def.cat_classif_type = new_theme;
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (all_init && template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}


void ScatterNewPlotCanvas::update(CatClassifState* o)
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

void ScatterNewPlotCanvas::OnSaveCategories()
{
	wxString t_name;
	if (GetCcType() == CatClassification::custom) {
		t_name = cat_classif_def.title;
	} else {
		t_name = CatClassification::CatClassifTypeToString(GetCcType());
	}
	wxString label;
	label << t_name << _(" Categories");
	wxString title;
	title << _("Save ") << label;
    
	SaveCategories(title, label, "CATEGORIES", XYZ_undef);
}

void ScatterNewPlotCanvas::SetHighlightColor(wxColour color)
{	
	highlight_color = color;
	pens.SetPenColor(pens.GetRegSelPen(), highlight_color);
	UpdateRegSelectedLine();
	if (IsShowLowessSmoother() && IsShowRegimes()) {
		UpdateLowessOnRegimes();
	}
	UpdateDisplayStats();
	TemplateCanvas::SetHighlightColor(color);
}

void ScatterNewPlotCanvas::SetSelectableFillColor(wxColour color)
{
    // In Scatter Plot, Fill color is for points
	if (!is_bubble_plot) {
		selectable_fill_color = color;
		pens.SetPenColor(pens.GetRegExlPen(), selectable_fill_color);
		for (int t=0; t<cat_data.GetCanvasTmSteps(); t++) {
			cat_data.SetCategoryColor(t, 0, selectable_fill_color);
		}
		UpdateRegExcludedLine();
		if (IsShowLowessSmoother() && IsShowRegimes()) {
			UpdateLowessOnRegimes();
		}
		UpdateDisplayStats();
	}
	TemplateCanvas::SetSelectableFillColor(color);
}

void ScatterNewPlotCanvas::SetSelectableOutlineColor(wxColour color)
{
    // In Scatter Plot, Outline color is for regression lines
	if (!is_bubble_plot) {
		selectable_outline_color = color;
		pens.SetPenColor(pens.GetRegPen(), selectable_outline_color);
		if (IsShowLinearSmoother() && reg_line) {
			reg_line->setPen(*pens.GetRegPen());
		}
		if (IsShowLinearSmoother() && lowess_reg_line) {
			lowess_reg_line->setPen(*pens.GetRegPen());
		}
		UpdateDisplayStats();
	}
	TemplateCanvas::SetSelectableOutlineColor(color);
}

/** This method assumes that x and y names are already set and are valid. It
 will populate the corresponding X and Y vectors of data from the table data,
 either standardized or not, and will recreate all canvas objects as needed
 and refresh the canvas. */
void ScatterNewPlotCanvas::PopulateCanvas()
{
	//wxSize size(GetVirtualSize());
    //int screen_w = size.GetWidth();
    //int screen_h = size.GetHeight();
    //last_scale_trans.SetView(screen_w, screen_h);

    
	pens.SetPenColor(pens.GetRegPen(), selectable_outline_color);
	pens.SetPenColor(pens.GetRegSelPen(), highlight_color);
	pens.SetPenColor(pens.GetRegExlPen(), selectable_fill_color);
	
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	int xt = var_info[0].time-var_info[0].time_min;
	int yt = var_info[1].time-var_info[1].time_min;

    // for undefined values, we have to search [min max] for both axies
    double x_max, x_min, y_max, y_min;
    bool has_init = false;
    
    XYZ_undef.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		X[i] = x_data[xt][i];
		Y[i] = y_data[yt][i];
		XYZ_undef[i] = x_undef_data[xt][i] ||y_undef_data[yt][i];
        if (!XYZ_undef[i]) {
            if (!has_init) {
                x_max = X[i];
                x_min = X[i];
                y_max = Y[i];
                y_min = Y[i];
                has_init = true;
            } else {
                if (X[i] > x_max)
                    x_max = X[i];
                if (X[i] < x_min)
                    x_min = X[i];
                if (Y[i] > y_max)
                    y_max = Y[i];
                if (Y[i] < y_min)
                    y_min = Y[i];
            }
        }
	}
	if (is_bubble_plot) {
		int zt = var_info[2].time-var_info[2].time_min;
		for (int i=0; i<num_obs; i++) {
			Z[i] = z_data[zt][i];
            XYZ_undef[i] = XYZ_undef[i] || z_undef_data[zt][i];
		}
	}
	
	statsX = SampleStatistics(X, XYZ_undef);
	statsY = SampleStatistics(Y, XYZ_undef);
    if (is_bubble_plot) {
        statsZ = SampleStatistics(Z, XYZ_undef);
    }
    
    if (standardized) {
        for (int i=0, iend=X.size(); i<iend; i++) {
            X[i] = (X[i]-statsX.mean)/statsX.sd_with_bessel;
            Y[i] = (Y[i]-statsY.mean)/statsY.sd_with_bessel;
            if (is_bubble_plot) {
                Z[i] = (Z[i]-statsZ.mean)/statsZ.sd_with_bessel;
            }
        }
        // we are ignoring the global scaling option here
        x_max = (statsX.max - statsX.mean)/statsX.sd_with_bessel;
        x_min = (statsX.min - statsX.mean)/statsX.sd_with_bessel;
        y_max = (statsY.max - statsY.mean)/statsY.sd_with_bessel;
        y_min = (statsY.min - statsY.mean)/statsY.sd_with_bessel;
        
        statsX = SampleStatistics(X, XYZ_undef);
        statsY = SampleStatistics(Y, XYZ_undef);
        if (is_bubble_plot) {
            statsZ = SampleStatistics(Z, XYZ_undef);
        }
        
        // mean shold be 0 and biased standard deviation should be 1
        double eps = 0.000001;
        if (-eps < statsX.mean && statsX.mean < eps)
            statsX.mean = 0;
        if (-eps < statsY.mean && statsY.mean < eps)
            statsY.mean = 0;
        if (is_bubble_plot) {
            if (-eps < statsZ.mean && statsZ.mean < eps) {
                statsZ.mean = 0;
            }
        }
    }

    regressionXY = SimpleLinearRegression(X, Y, XYZ_undef, XYZ_undef,
                                          statsX.mean, statsY.mean,
                                          statsX.var_without_bessel,
                                          statsY.var_without_bessel);
	
    
	sse_c = regressionXY.error_sum_squares;
	
    if (var_info[0].is_moran || (!var_info[0].fixed_scale && !standardized)) {
        x_max = var_info[0].max[var_info[0].time];
        x_min = var_info[0].min[var_info[0].time];
    }
    if (var_info[1].is_moran || (!var_info[1].fixed_scale && !standardized)) {
        y_max = var_info[1].max[var_info[1].time];
        y_min = var_info[1].min[var_info[1].time];
    }
	
	double x_pad = 0.1 * (x_max - x_min);
	double y_pad = 0.1 * (y_max - y_min);
	axis_scale_x = AxisScale(x_min - x_pad, x_max + x_pad, 5, axis_display_precision);
	axis_scale_y = AxisScale(y_min - y_pad, y_max + y_pad, 5, axis_display_precision);

    /*
	// used by status bar for showing selection rectangle range
	data_scale_xmin = axis_scale_x.scale_min;
	data_scale_xmax = axis_scale_x.scale_max;
	data_scale_ymin = axis_scale_y.scale_min;
	data_scale_ymax = axis_scale_y.scale_max;
     */
	
	// Populate TemplateCanvas::selectable_shps
	selectable_shps.resize(num_obs);
    selectable_shps_undefs.resize(num_obs);
	scaleX = 100.0 / (axis_scale_x.scale_range);
	scaleY = 100.0 / (axis_scale_y.scale_range);
    
	if (is_bubble_plot) {
		selectable_shps_type = circles;
		
		const double pi = 3.141592653589793238463;
		const double rad_mn = 10;
		const double area_mn = pi * rad_mn * rad_mn;
		const double rad_sd = 20;
		const double area_sd = pi * rad_sd * rad_sd;
		const double a = area_sd - area_mn;
		const double b = area_mn;
		const double min_rad = 3;
		const double min_area = pi * min_rad * min_rad;
		wxRealPoint pt;
		if (statsZ.max-statsZ.min <= 0.00000001 ||
			statsZ.var_without_bessel == 0) {
			for (int i=0; i<num_obs; i++) {
                selectable_shps_undefs[i] = XYZ_undef[i];
				pt.x = (X[i] - axis_scale_x.scale_min) * scaleX;
				pt.y = (Y[i] - axis_scale_y.scale_min) * scaleY;
				selectable_shps[i] = new GdaCircle(pt, rad_mn * bubble_size_scaler);
			}
		} else {
			for (int i=0; i<num_obs; i++) {
                selectable_shps_undefs[i] = XYZ_undef[i];
				double z = (Z[i] - statsZ.mean)/statsZ.sd_without_bessel;
				double area_z = (a*z + b) + min_area + area_sd;
                if (area_z < min_area) {
                    area_z = min_area;
                }
				double r = sqrt(area_z/pi);
				pt.x = (X[i] - axis_scale_x.scale_min) * scaleX;
				pt.y = (Y[i] - axis_scale_y.scale_min) * scaleY;
				selectable_shps[i] = new GdaCircle(pt, r * bubble_size_scaler);
			}
		}
	} else {
		selectable_shps_type = points;
		for (int i=0; i<num_obs; i++) {
            selectable_shps_undefs[i] = XYZ_undef[i];
			selectable_shps[i] =
			new GdaPoint(wxRealPoint((X[i] - axis_scale_x.scale_min) * scaleX,
			 						 (Y[i] - axis_scale_y.scale_min) * scaleY));
		}
	}
	
	lowess_reg_line = new GdaSpline;
	lowess_reg_line_selected = new GdaSpline;
	lowess_reg_line_excluded = new GdaSpline;
	foreground_shps.push_back(lowess_reg_line_selected);
	foreground_shps.push_back(lowess_reg_line_excluded);
    foreground_shps.push_back(lowess_reg_line);

	
	if (IsShowLowessSmoother() && X.size() > 1) {
		//Begin populating LOWESS curve (all obs)
		size_t n = X.size();
		wxString key = SmoothingUtils::LowessCacheKey(xt, yt);
		
		SmoothingUtils::LowessCacheEntry* lce =
			SmoothingUtils::UpdateLowessCacheForTime(lowess_cache, key, lowess,
                                                     X, Y, XYZ_undef);
		
		if (!lce) {
			//("Error: could not create or find LOWESS cache entry");
		} else {
			lowess_reg_line->reInit(lce->X_srt, lce->YS_srt,
                                    axis_scale_x.scale_min,
                                    axis_scale_y.scale_min,
                                    scaleX, scaleY);
			lowess_reg_line->setPen(*pens.GetRegPen());
			
			//("End populating LOWESS curve (all obs)");
		}
		if (IsShowRegimes()) {
			UpdateLowessOnRegimes();
		}
	}
	
	// create axes
	x_baseline = new GdaAxis(GetNameWithTime(0), axis_scale_x,
                             wxRealPoint(0,0), wxRealPoint(100, 0));
	x_baseline->setPen(*GdaConst::scatterplot_scale_pen);
	foreground_shps.push_back(x_baseline);
	y_baseline = new GdaAxis(GetNameWithTime(1), axis_scale_y,
                             wxRealPoint(0,0), wxRealPoint(0, 100));
	y_baseline->setPen(*GdaConst::scatterplot_scale_pen);
	foreground_shps.push_back(y_baseline);
	
	// create optional axes through origin
	x_axis_through_origin = new GdaPolyLine(0,50,100,50);
	x_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	y_axis_through_origin = new GdaPolyLine(50,0,50,100);
	y_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	foreground_shps.push_back(x_axis_through_origin);
	foreground_shps.push_back(y_axis_through_origin);
	UpdateAxesThroughOrigin();
	
	// show regression lines
	reg_line = new GdaPolyLine(0,100,0,100);
	reg_line->setPen(*wxTRANSPARENT_PEN);
	reg_line_selected = new GdaPolyLine(0,100,0,100);
	reg_line_selected->setPen(*wxTRANSPARENT_PEN);
	reg_line_selected->setBrush(*wxTRANSPARENT_BRUSH);
	reg_line_excluded = new GdaPolyLine(0,100,0,100);
	reg_line_excluded->setPen(*wxTRANSPARENT_PEN);
	reg_line_excluded->setBrush(*wxTRANSPARENT_BRUSH);

	foreground_shps.push_back(reg_line_selected);
	foreground_shps.push_back(reg_line_excluded);
    foreground_shps.push_back(reg_line);

	if (IsShowLinearSmoother() && !is_bubble_plot) {
		double cc_degs_of_rot;
		double reg_line_slope;
		bool reg_line_infinite_slope;
		bool reg_line_defined;
		wxRealPoint a, b;
        SmoothingUtils::CalcRegressionLine(*reg_line,
                                           reg_line_slope,
                                           reg_line_infinite_slope,
                                           reg_line_defined, a, b, cc_degs_of_rot,
                                           axis_scale_x, axis_scale_y,
                                           regressionXY, *pens.GetRegPen());
	}
	
	if (IsRegressionSelected() || IsRegressionExcluded()) {
		// update both selected and excluded stats
        SmoothingUtils::CalcStatsRegimes(X, Y, XYZ_undef, XYZ_undef,
                                         statsX, statsY, regressionXY,
                                         highlight_state->GetHighlight(),
                                         statsXselected, statsYselected,
                                         statsXexcluded, statsYexcluded,
                                         regressionXYselected,
                                         regressionXYexcluded,
                                         sse_sel, sse_unsel);
	}
    if (IsRegressionSelected())  {
        UpdateRegSelectedLine();
    }
    
    if (IsRegressionExcluded()) {
        UpdateRegExcludedLine();
    }

	chow_test_text = new GdaShapeText();
	chow_test_text->hidden = true;
	foreground_shps.push_back(chow_test_text);
	stats_table = new GdaShapeTable();
	stats_table->hidden = true;
	foreground_shps.push_back(stats_table);
    
	if (!is_bubble_plot) {
		UpdateDisplayStats();
	}
	
	PopCanvPreResizeShpsHook();
	
	ResizeSelectableShps();
}

void ScatterNewPlotCanvas::PopCanvPreResizeShpsHook()
{
}

void ScatterNewPlotCanvas::TimeChange()
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
	if (var_info[0].sync_with_global_time) {
		var_info[0].time = ref_time + var_info[0].ref_time_offset;
	}
	if (var_info[1].sync_with_global_time) {
		var_info[1].time = ref_time + var_info[1].ref_time_offset;
	}
	if (is_bubble_plot && var_info[2].sync_with_global_time) {
		var_info[2].time = ref_time + var_info[2].ref_time_offset;
	}
	if (is_bubble_plot && var_info[3].sync_with_global_time) {
		var_info[3].time = ref_time + var_info[3].ref_time_offset;
	}
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
    UpdateStatusBar();
    
	Refresh();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void ScatterNewPlotCanvas::VarInfoAttributeChange()
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
		if (var_info[i].is_ref_variable)
            ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
    int var0_time_max = var_info[0].time_max;
    int var0_time_min = var_info[0].time_min;
    int var1_time_max = var_info[1].time_max;
    int var1_time_min = var_info[1].time_min;
    
	int x_tms = var0_time_max - var0_time_min + 1;
	int y_tms = var1_time_max - var1_time_min + 1;
    
	x_data.resize(boost::extents[x_tms][num_obs]);
	x_undef_data.resize(boost::extents[x_tms][num_obs]);
    
	for (int t=var0_time_min; t<=var0_time_max; t++) {
		int tt = t - var0_time_min;
        for (int i=0; i<num_obs; i++) {
            x_data[tt][i] = data[0][t][i];
            x_undef_data[tt][i] = undef_data[0][t][i];
        }
	}
    
	y_data.resize(boost::extents[y_tms][num_obs]);
	y_undef_data.resize(boost::extents[y_tms][num_obs]);
    
	for (int t=var1_time_min; t<=var1_time_max; t++) {
		int tt = t-var1_time_min;
        for (int i=0; i<num_obs; i++) {
            y_data[tt][i] = data[1][t][i];
            y_undef_data[tt][i] = undef_data[1][t][i];
        }
	}
    
	if (is_bubble_plot) {
		int z_tms = (var_info[2].time_max-var_info[2].time_min) + 1;
        
		z_data.resize(boost::extents[z_tms][num_obs]);
		z_undef_data.resize(boost::extents[z_tms][num_obs]);
        
		for (int t=var_info[2].time_min; t<=var_info[2].time_max; t++) {
			int tt = t-var_info[2].time_min;
            for (int i=0; i<num_obs; i++)  {
                z_data[tt][i] = data[2][t][i];
                z_undef_data[tt][i] = undef_data[2][t][i];
            }
		}
	}
	//GdaVarTools::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_categories and ref_var_index
 */
void ScatterNewPlotCanvas::CreateAndUpdateCategories()
{
	cats_valid.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_valid[t] = true;
	cats_error_message.resize(num_time_vals);
	for (int t=0; t<num_time_vals; t++) cats_error_message[t] = wxEmptyString;
	
	if (!is_bubble_plot || GetCcType() == CatClassification::no_theme) {
		// 1 = #cats
		if (is_bubble_plot) {
			CatClassification::ChangeNumCats(1, cat_classif_def);
			cat_classif_def.color_scheme =
				CatClassification::custom_color_scheme;
			cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
		}
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t, 0, GdaConst::map_default_fill_colour);
			cat_data.SetCategoryLabel(t, 0, "");
			cat_data.SetCategoryCount(t, 0, num_obs);
			for (int i=0; i<num_obs; i++) cat_data.AppendIdToCategory(t, 0, i);
		}
		if (ref_var_index != -1) {
			cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
											- var_info[ref_var_index].time_min);
		}
		if (is_bubble_plot) {
			CreateZValArrays(num_time_vals, num_obs);
			for (int t=0; t<num_time_vals; t++) {
				int tt = var_info[2].time_min;
				if (var_info[2].sync_with_global_time) {
					tt += t;
				}
				for (int cat=0, ce=cat_data.GetNumCategories(t);
					 cat<ce; cat++) {
					for (int i=0,
						 ie=cat_data.categories[t].cat_vec[cat].ids.size();
						 i<ie; i++) {
						int obs_id = cat_data.categories[t].cat_vec[cat].ids[i];
						int ord = obs_id_to_z_val_order[tt][obs_id];
						z_val_order[t][ord][0] = obs_id;  // obs id
						z_val_order[t][ord][1] = cat;  // category id
					}
				}
			}
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
			int tm = var_info[3].is_time_variant ? t : 0;
			cat_var_sorted[t][i].first = data[3][tm+var_info[3].time_min][i];
			cat_var_sorted[t][i].second = i;
            undefs[i] = undefs[i] || undef_data[3][tm+var_info[3].time_min][i];
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
	
	CreateZValArrays(num_time_vals, num_obs);
	for (int t=0; t<num_time_vals; t++) {
		int tt = var_info[2].time_min;
		if (var_info[2].sync_with_global_time) {
			tt += t;
		}
		for (int cat=0, ce=cat_data.GetNumCategories(t); cat<ce; cat++) {
			for (int i=0, ie=cat_data.categories[t].cat_vec[cat].ids.size();
				 i<ie; i++) {
				int obs_id = cat_data.categories[t].cat_vec[cat].ids[i];
				int ord = obs_id_to_z_val_order[tt][obs_id];
				z_val_order[t][ord][0] = obs_id;  // obs id
				z_val_order[t][ord][1] = cat;  // category id
			}
		}
	}
	
	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int cnc = cat_data.GetNumCategories(cat_data.GetCurrentCanvasTmStep());
	CatClassification::ChangeNumCats(cnc, cat_classif_def);
}

void ScatterNewPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	PopulateCanvas();
}

CatClassification::CatClassifType ScatterNewPlotCanvas::GetCcType()
{
	return cat_classif_def.cat_classif_type;
}

void ScatterNewPlotCanvas::ViewStandardizedData()
{
	standardized = true;
	EmptyLowessCache();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ViewOriginalData()
{
	standardized = false;
	EmptyLowessCache();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ShowLinearSmoother(bool display)
{
	show_linear_smoother = display;
	UpdateDisplayStats();
	UpdateDisplayLinesAndMargins();
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ShowLowessSmoother(bool display)
{
	show_lowess_smoother = display;
	PopulateCanvas();
}

void ScatterNewPlotCanvas::ChangeLoessParams(double f, int iter,
                                             double delta_factor)

{
	EmptyLowessCache();
	lowess.SetF(f);
	lowess.SetIter(iter);
	lowess.SetDeltaFactor(delta_factor);
	if (IsShowLowessSmoother()) PopulateCanvas();
}

void ScatterNewPlotCanvas::ViewRegressionSelected(bool display)
{
	bool changed = false;
	if (!display) {
		reg_line_selected->setPen(*wxTRANSPARENT_PEN);
		if ((IsRegressionSelected() && !IsRegressionExcluded()) &&
            IsDisplayStats()) {
			// there is no longer anything showing, but there
			// was previously something showing
			show_reg_selected = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		} else {
			show_reg_selected = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	} else {
		if ((!IsRegressionSelected() && !IsRegressionExcluded()) &&
				IsDisplayStats()) {
			// we want to show something now, but there was previously
			// nothing showing
			show_reg_selected = true;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			PopulateCanvas();
		} else {
			show_reg_selected = true;
            SmoothingUtils::CalcStatsRegimes(X, Y, XYZ_undef, XYZ_undef,
                                             statsX, statsY, regressionXY,
                                             highlight_state->GetHighlight(),
                                             statsXselected, statsYselected,
                                             statsXexcluded, statsYexcluded,
                                             regressionXYselected, regressionXYexcluded,
                                             sse_sel, sse_unsel);
			UpdateRegSelectedLine();
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	}
	Refresh();
}

void ScatterNewPlotCanvas::UpdateRegSelectedLine()
{
	pens.SetPenColor(pens.GetRegSelPen(), highlight_color);
	if (IsShowLinearSmoother()) {
		double cc_degs_of_rot;
		wxRealPoint a, b;
        SmoothingUtils::CalcRegressionLine(*reg_line_selected,
                                           reg_line_selected_slope,
                                           reg_line_selected_infinite_slope,
                                           reg_line_selected_defined, a, b,
                                           cc_degs_of_rot,
                                           axis_scale_x, axis_scale_y,
                                           regressionXYselected,
                                           *pens.GetRegSelPen());
		ApplyLastResizeToShp(reg_line_selected);
		layer2_valid = false;
	} else {
		reg_line_selected->setPen(*wxTRANSPARENT_PEN);
	}
}

void ScatterNewPlotCanvas::ViewRegressionSelectedExcluded(bool display)
{
	bool changed = false;
	if (!display) {
		reg_line_excluded->setPen(*wxTRANSPARENT_PEN);
		if ((!IsRegressionSelected() && IsRegressionExcluded()) &&
				IsDisplayStats()) {
			// there is no longer anything showing, but there
			// was previously something showing
			show_reg_excluded = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		} else {
			show_reg_excluded = false;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	} else {
		if ((!IsRegressionSelected() && !IsRegressionExcluded()) &&
				IsDisplayStats()) {
			// we want to show something now, but there was previously
			// nothing showing
			show_reg_excluded = true;
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			PopulateCanvas();
		} else {
            SmoothingUtils::CalcStatsRegimes(X, Y, XYZ_undef, XYZ_undef,
                                             statsX, statsY, regressionXY,
                                             highlight_state->GetHighlight(),
                                             statsXselected, statsYselected,
                                             statsXexcluded, statsYexcluded,
                                             regressionXYselected,
                                             regressionXYexcluded,
                                             sse_sel, sse_unsel);
			show_reg_excluded = true;
			UpdateRegExcludedLine();
			UpdateDisplayStats();
			changed = UpdateDisplayLinesAndMargins();
			if (changed) ResizeSelectableShps();
		}
	}
	Refresh();
}

void ScatterNewPlotCanvas::UpdateRegExcludedLine()
{
	pens.SetPenColor(pens.GetRegExlPen(), selectable_fill_color);
	if (IsShowLinearSmoother()) {
		double cc_degs_of_rot;
		wxRealPoint a, b;
        SmoothingUtils::CalcRegressionLine(*reg_line_excluded,
                                           reg_line_excluded_slope,
                                           reg_line_excluded_infinite_slope,
                                           reg_line_excluded_defined, a, b,
                                           cc_degs_of_rot,
                                           axis_scale_x, axis_scale_y,
                                           regressionXYexcluded,
                                           *pens.GetRegExlPen());
		ApplyLastResizeToShp(reg_line_excluded);
		layer2_valid = false;
	} else {
		reg_line_excluded->setPen(*wxTRANSPARENT_PEN);
	}
}

void ScatterNewPlotCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	UpdateDisplayStats();
	UpdateDisplayLinesAndMargins();
	ResizeSelectableShps();
}

void ScatterNewPlotCanvas::ShowAxesThroughOrigin(bool show_origin_axes_s)
{
	show_origin_axes = show_origin_axes_s;
	UpdateAxesThroughOrigin();
	Refresh();
}

/** Called when selection changes */
void ScatterNewPlotCanvas::UpdateLowessOnRegimes()
{
	if (!lowess_reg_line_selected && !lowess_reg_line_excluded)
        return;
    
	size_t n = num_obs;
	int xt = var_info[0].time-var_info[0].time_min;
	int yt = var_info[1].time-var_info[1].time_min;
	wxString key = SmoothingUtils::LowessCacheKey(xt, yt);
    
	SmoothingUtils::LowessCacheType::iterator it = lowess_cache.find(key);
	SmoothingUtils::LowessCacheEntry* lce = 0;
	if (it != lowess_cache.end()) {
		lce = it->second ;
	}
	if (!lce) {
		return;
	}
	
	std::vector<double> sel_smthd_srt_x;
	std::vector<double> sel_smthd_srt_y;
	std::vector<double> unsel_smthd_srt_x;
	std::vector<double> unsel_smthd_srt_y;
	
	if (IsShowRegimes()) {
        SmoothingUtils::CalcLowessRegimes(lce, lowess,
                                          highlight_state->GetHighlight(),
                                          sel_smthd_srt_x,
                                          sel_smthd_srt_y,
                                          unsel_smthd_srt_x,
                                          unsel_smthd_srt_y,
                                          XYZ_undef);
	}

	layer2_valid = false;
}

void ScatterNewPlotCanvas::ComputeChowTest()
{
	wxString s = _("Chow test for sel/unsel regression subsets: ");
	int tot_sel = highlight_state->GetTotalHighlighted();
	int hl_size = highlight_state->GetHighlightSize();
	double N = X.size();
	double K = 1;
	double sse_u = sse_sel + sse_unsel;
	if (K+1 <= 0 || N-2*(K+1) <= 0 || sse_u == 0) {
		chow_valid = false;
		s << _("can't compute");
		chow_test_text->setText(s);
		return;
	}
	if (tot_sel >= 2 && tot_sel <= hl_size-2) {
		chow_valid = true;
		using namespace boost::math;
		// Compute Chow test ratio (F value) based on
		// sse_c, sse_sel and sse_unsel
		// note number of restrictions is K+1 since intercepts are constrained
		// to be equal.
		chow_ratio = ((sse_c - sse_u) * (N-2*(K+1))) / (sse_u*(K+1));
		if (chow_ratio < 0) {
			chow_valid = false;
		} else {
			// constructs and f-distribution with numerator degrees of
			// freedom K+1 and denominator degrees of freedom N-2*(K+1);
			fisher_f_distribution<> f_dist(K+1, N-2*(K+1));
			chow_pval = 1-cdf(f_dist, chow_ratio);
			chow_valid = true;
		}
	} else {
		chow_valid = false;
	}
	if (chow_valid) {
		s << "distrib=F(" << K+1 << "," << N-2*(K+1) << ")";
		s << ", ratio=" << GenUtils::DblToStr(chow_ratio, 4);
		s << ", p-val=" << GenUtils::DblToStr(chow_pval, 4);
	} else {
		s << "need two valid regressions";
	}
	chow_test_text->setText(s);
}

/** Free allocated points arrays in lowess_cache and clear cache */
void ScatterNewPlotCanvas::EmptyLowessCache()
{
	SmoothingUtils::EmptyLowessCache(lowess_cache);
}

/** This method builds up the display optional stats string from scratch every
 time. It assumes the calling function will do the screen Refresh. */
void ScatterNewPlotCanvas::UpdateDisplayStats()
{
	if (IsDisplayStats() && IsShowLinearSmoother()) {
		// fill out the regression stats table
		int rows = 2;
		if (IsRegressionSelected()) rows++;
		if (IsRegressionExcluded()) rows++;
		int cols = 10;
		std::vector<wxString> vals(rows*cols);
		std::vector<GdaShapeTable::CellAttrib> attributes(rows*cols);
		int i=0; int j=0;
		for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
			attributes[k].color = *wxBLACK;
		}
        
        const std::vector<bool>& hl = highlight_state->GetHighlight();
        int tot_obs = 0;
        int tot_sel_obs = 0;
        int tot_unsel_obs = 0;
        
        for (size_t i=0; i<XYZ_undef.size(); i++) {
            if (!XYZ_undef[i]) {
                tot_obs += 1;
                if (hl[i])
                    tot_sel_obs += 1;
                else
                    tot_unsel_obs += 1;
            }
        }
		
		vals[i*cols+j++] = "#obs";
		vals[i*cols+j++] = "R^2";
		vals[i*cols+j++] = "const a";
		vals[i*cols+j++] = "std-err a";
		vals[i*cols+j++] = "t-stat a";
		vals[i*cols+j++] = "p-value a";
		vals[i*cols+j++] = "slope b";
		vals[i*cols+j++] = "std-err b";
		vals[i*cols+j++] = "t-stat b";
		vals[i*cols+j++] = "p-value b";
		i++; j=0;
		for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
			attributes[k].color = selectable_outline_color;
		}
		vals[i*cols+j++] << tot_obs;
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.r_squared);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.std_err_of_alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.t_score_alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.p_value_alpha);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.beta);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.std_err_of_beta);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.t_score_beta);
		vals[i*cols+j++] << GenUtils::DblToStr(regressionXY.p_value_beta);
		if (IsRegressionSelected()) {
			i++; j=0;
			for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
				attributes[k].color = highlight_color;
			}
			vals[i*cols+j++] << tot_sel_obs;
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.r_squared);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.std_err_of_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.t_score_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.p_value_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.std_err_of_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.t_score_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYselected.p_value_beta);
		}
		if (IsRegressionExcluded()) {
			i++; j=0;
			for (int k=i*cols, kend=i*cols+cols; k<kend; k++) {
				attributes[k].color = selectable_fill_color;
			}
			vals[i*cols+j++] << tot_unsel_obs;
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.r_squared);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.std_err_of_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.t_score_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.p_value_alpha);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.std_err_of_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.t_score_beta);
			vals[i*cols+j++] << GenUtils::DblToStr(regressionXYexcluded.p_value_beta);		
		}
        int x_nudge = last_scale_trans.GetXNudge();
		
        stats_table->operator=(GdaShapeTable(vals, attributes, rows, cols,
                                             *GdaConst::small_font,
                                             wxRealPoint(50, 0),
                                             GdaShapeText::h_center,
                                             GdaShapeText::top,
                                             GdaShapeText::h_center,
                                             GdaShapeText::v_center,
                                             3, 8, -x_nudge, 45)); //62));
		stats_table->setPen(*wxBLACK_PEN);
		stats_table->hidden = false;
		
		if (IsRegressionSelected() && IsRegressionExcluded()) {
			int table_w=0, table_h=0;
			wxClientDC dc(this);
			stats_table->GetSize(dc, table_w, table_h);
			ComputeChowTest();
			wxString s = chow_test_text->getText();
            chow_test_text->operator=(GdaShapeText(s, *GdaConst::small_font,
                                                   wxRealPoint(50,0), 0,
                                                   GdaShapeText::h_center,
                                                   GdaShapeText::v_center,
                                                   -x_nudge,
                                                   table_h+62)); //117));
			chow_test_text->setPen(*wxBLACK_PEN);
			chow_test_text->hidden = false;
		} else {
			chow_test_text->setText("");
			chow_test_text->hidden = true;
		}
		
		ApplyLastResizeToShp(chow_test_text);
		ApplyLastResizeToShp(stats_table);
	} else {
		chow_test_text->setText("");
		chow_test_text->hidden = true;
		
		stats_table->hidden = true;
	}
	layer2_valid = false;
}

void ScatterNewPlotCanvas::UpdateAxesThroughOrigin()
{
	x_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	y_axis_through_origin->setPen(*wxTRANSPARENT_PEN);
	if (show_origin_axes &&
		axis_scale_y.scale_min < 0 && 0 < axis_scale_y.scale_max) {
		double y_inter = 100.0 * ((-axis_scale_y.scale_min) /
			(axis_scale_y.scale_max-axis_scale_y.scale_min));
		x_axis_through_origin->operator=(GdaPolyLine(0,y_inter,100,y_inter));
		x_axis_through_origin->setPen(*GdaConst::scatterplot_origin_axes_pen);
		ApplyLastResizeToShp(x_axis_through_origin);
	}
	if (show_origin_axes &&
		axis_scale_x.scale_min < 0 && 0 < axis_scale_x.scale_max) {
		double x_inter = 100.0 * ((-axis_scale_x.scale_min) /
			(axis_scale_x.scale_max-axis_scale_x.scale_min));
		y_axis_through_origin->operator=(GdaPolyLine(x_inter,0,x_inter,100));
		y_axis_through_origin->setPen(*GdaConst::scatterplot_origin_axes_pen);
		ApplyLastResizeToShp(y_axis_through_origin);
	}
	layer0_valid = false;
}

bool ScatterNewPlotCanvas::UpdateDisplayLinesAndMargins()
{
	bool changed = false;
	int lines = 0;
	int table_w=0, table_h=0;
	if (IsDisplayStats() && stats_table && IsShowLinearSmoother()) {
		wxClientDC dc(this);	
		stats_table->GetSize(dc, table_w, table_h);
		LOG(table_w);
		LOG(table_h);
	}
	last_scale_trans.bottom_margin = 50;
	if (!IsDisplayStats() || !IsShowLinearSmoother()) {
		lines = 0;
        
	} else if (!IsRegressionSelected() && !IsRegressionExcluded()) {
		lines = 1;
		last_scale_trans.bottom_margin += 10;
        
	} else if (IsRegressionSelected() != IsRegressionExcluded()) {
		lines = 2;
		last_scale_trans.bottom_margin += 10;
        
	} else {
		lines = 3;
		last_scale_trans.bottom_margin += 30;  // leave room for Chow Test
		//virtual_screen_marg_bottom
        //= 90+2*13+20;
	}
	last_scale_trans.bottom_margin += table_h;
    
	if (table_display_lines != lines) {
		layer0_valid = false;
		layer1_valid = false;
		layer2_valid = false;
		changed = true;
	}
	
	table_display_lines = lines;
	return changed;
}

void ScatterNewPlotCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
    TableInterface* table_int = project->GetTableInt();
    
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    
    if (highlight_state->GetTotalHighlighted()> 0) {
        int n_total_hl = highlight_state->GetTotalHighlighted();
        s << "#selected=" << n_total_hl << "  ";
        
        int n_undefs = 0;
        for (int i=0; i<num_obs; i++) {
            if (XYZ_undef[i] && hl[i]) {
                n_undefs += 1;
            }
        }
        if (n_undefs> 0) {
            s << "(undefined:" << n_undefs << ") ";
        }
        
		if (brushtype == rectangle) {
			wxRealPoint pt1 = MousePntToObsPnt(sel1);
			wxRealPoint pt2 = MousePntToObsPnt(sel2);
			wxString xmin = GenUtils::DblToStr(GenUtils::min<double>(pt1.x,
																	 pt2.x));
			wxString xmax = GenUtils::DblToStr(GenUtils::max<double>(pt1.x,
																	 pt2.x));
			wxString ymin = GenUtils::DblToStr(GenUtils::min<double>(pt1.y,
																	 pt2.y));
			wxString ymax = GenUtils::DblToStr(GenUtils::max<double>(pt1.y,
																	 pt2.y));
		}
        s <<"  ";
	}
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "hover obs " << hover_obs[0]+1 << " = (";
			s << X[hover_obs[0]] << ", " << Y[hover_obs[0]];
			if (is_bubble_plot) {
				s << ", " << Z[hover_obs[0]];
				s << ", " << data[3][var_info[3].time][hover_obs[0]];
			}
			s << ")";
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1 << " = (";
			s << X[hover_obs[1]] << ", " << Y[hover_obs[1]];
			if (is_bubble_plot) {
				s << ", " << Z[hover_obs[1]];
				s << ", " << data[3][var_info[3].time][hover_obs[1]];
			}
			s << ")";
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1 << " = (";
			s << X[hover_obs[2]] << ", " << Y[hover_obs[2]];
			if (is_bubble_plot) {
				s << ", " << Z[hover_obs[2]];
				s << ", " << data[3][var_info[3].time][hover_obs[2]];
			}
			s << ")";
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	} 
	sb->SetStatusText(s);
}

ScatterNewPlotLegend::ScatterNewPlotLegend(wxWindow *parent,
										   TemplateCanvas* t_canvas,
										   const wxPoint& pos,
										   const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

ScatterNewPlotLegend::~ScatterNewPlotLegend()
{
}

IMPLEMENT_CLASS(ScatterNewPlotFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(ScatterNewPlotFrame, TemplateFrame)
	EVT_ACTIVATE(ScatterNewPlotFrame::OnActivate)
END_EVENT_TABLE()

ScatterNewPlotFrame::ScatterNewPlotFrame(wxFrame *parent, Project* project,
										 const wxPoint& pos, const wxSize& size,
										 const long style)
: TemplateFrame(parent, project, "", pos, size, style),
is_bubble_plot(false), lowess_param_frame(0)
{
}


ScatterNewPlotFrame::ScatterNewPlotFrame(wxFrame *parent, Project* project,
										 const std::vector<GdaVarTools::VarInfo>& var_info,
										 const std::vector<int>& col_ids,
										 bool is_bubble_plot_s,
										 const wxString& title,
										 const wxPoint& pos,
										 const wxSize& size,
										 const long style)
: TemplateFrame(parent, project, title, pos, size, style),
var_info(var_info),
is_bubble_plot(is_bubble_plot_s), lowess_param_frame(0)
{
    wxLogMessage("Open ScatterNewPlotFrame.");
    
	int width, height;
	GetClientSize(&width, &height);
	
	wxSplitterWindow* splitter_win = 0;
	if (is_bubble_plot) {
		splitter_win = new wxSplitterWindow(this,-1,wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
		splitter_win->SetMinimumPaneSize(10);
	}
	wxPanel* rpanel = NULL;
    wxPanel* lpanel = NULL;
    
	if (is_bubble_plot) {
        rpanel = new wxPanel(splitter_win);
		template_canvas = new ScatterNewPlotCanvas(rpanel, this, project,
												   var_info, col_ids,
												   is_bubble_plot,
												   false, wxDefaultPosition,
												   wxSize(width,height));
        wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
        rbox->Add(template_canvas, 1, wxEXPAND);
        rpanel->SetSizer(rbox);
        
	} else {
		template_canvas = new ScatterNewPlotCanvas(this, this, project,
												   var_info, col_ids,
												   is_bubble_plot,
												   false, wxDefaultPosition,
												   wxSize(width,height));
	}
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	
	if (is_bubble_plot) {
        lpanel = new wxPanel(splitter_win);
		template_legend = new ScatterNewPlotLegend(lpanel,
												   template_canvas,
												   wxPoint(0,0), wxSize(0,0));
		
		splitter_win->SplitVertically(lpanel, rpanel,
								GdaConst::bubble_chart_default_legend_width);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(splitter_win, 1, wxEXPAND|wxALL);
        SetSizer(sizer);
        splitter_win->SetSize(wxSize(width,height));
        SetAutoLayout(true);
	}
	
	Show(true);
}

ScatterNewPlotFrame::~ScatterNewPlotFrame()
{
	if (lowess_param_frame) {
		lowess_param_frame->removeObserver(this);
		lowess_param_frame->closeAndDeleteWhenEmpty();
	}
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void ScatterNewPlotFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("ScatterNewPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void ScatterNewPlotFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu;
	if (is_bubble_plot) {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_BUBBLE_CHART_VIEW_MENU_OPTIONS");
		TemplateCanvas::AppendCustomCategories(optMenu,
											   project->GetCatClassifManager());
	} else {
		optMenu = wxXmlResource::Get()->
			LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
	}
	((ScatterNewPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((ScatterNewPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::ToggleLowessMenuItem(bool enabled)
{
    wxMenu* menu = wxXmlResource::Get()->LoadMenu("ID_SCATTER_NEW_PLOT_VIEW_MENU_OPTIONS");
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_VIEW_LOWESS_SMOOTHER"), enabled);
}

void ScatterNewPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((ScatterNewPlotCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
        
	}
}

void ScatterNewPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void ScatterNewPlotFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Recreate();
}

void ScatterNewPlotFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	((ScatterNewPlotCanvas*) template_canvas)->ViewStandardizedData();
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewOriginalData(wxCommandEvent& event)
{
	((ScatterNewPlotCanvas*) template_canvas)->ViewOriginalData();
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewLinearSmoother(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ShowLinearSmoother(!t->IsShowLinearSmoother());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewLowessSmoother(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ShowLowessSmoother(!t->IsShowLowessSmoother());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnEditLowessParams(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	if (lowess_param_frame) {
		lowess_param_frame->Iconize(false);
		lowess_param_frame->Raise();
		lowess_param_frame->SetFocus();
	} else {
		Lowess l = t->GetLowess();
		lowess_param_frame = new LowessParamFrame(l.GetF(), l.GetIter(),l.GetDeltaFactor(),project);
		lowess_param_frame->registerObserver(this);
	}
}

void ScatterNewPlotFrame::OnViewRegimesRegression(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	bool r_sel = t->IsRegressionSelected();
	bool r_exl = t->IsRegressionExcluded();
	if (!r_sel || !r_exl) {
		t->ViewRegressionSelected(true);
		t->ViewRegressionSelectedExcluded(true);
	} else {
		t->ViewRegressionSelected(false);
		t->ViewRegressionSelectedExcluded(false);
	}
	t->UpdateLowessOnRegimes();
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ViewRegressionSelected(!t->IsRegressionSelected());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnViewRegressionSelectedExcluded(
														wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ViewRegressionSelectedExcluded(!t->IsRegressionExcluded());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ShowAxesThroughOrigin(!t->IsShowOriginAxes());
	UpdateOptionMenuItems();
}

void ScatterNewPlotFrame::OnNewCustomCatClassifA()
{
	((ScatterNewPlotCanvas*) template_canvas)->NewCustomCatClassif();
}

void ScatterNewPlotFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	ChangeThemeType(CatClassification::custom, 4, cc_title);
}

void ScatterNewPlotFrame::OnThemeless()
{
	ChangeThemeType(CatClassification::no_theme, 1);
}

void ScatterNewPlotFrame::OnHinge15()
{
	ChangeThemeType(CatClassification::hinge_15, 6);
}

void ScatterNewPlotFrame::OnHinge30()
{
	ChangeThemeType(CatClassification::hinge_30, 6);
}

void ScatterNewPlotFrame::OnQuantile(int num_cats)
{
	ChangeThemeType(CatClassification::quantile, num_cats);
}

void ScatterNewPlotFrame::OnPercentile()
{
	ChangeThemeType(CatClassification::percentile, 6);
}

void ScatterNewPlotFrame::OnStdDevMap()
{
	ChangeThemeType(CatClassification::stddev, 6);
}

void ScatterNewPlotFrame::OnUniqueValues()
{
	ChangeThemeType(CatClassification::unique_values, 6);
}

void ScatterNewPlotFrame::OnNaturalBreaks(int num_cats)
{
	ChangeThemeType(CatClassification::natural_breaks, num_cats);
}

void ScatterNewPlotFrame::OnEqualIntervals(int num_cats)
{
	ChangeThemeType(CatClassification::equal_intervals, num_cats);
}

void ScatterNewPlotFrame::OnSaveCategories()
{
	((ScatterNewPlotCanvas*) template_canvas)->OnSaveCategories();
}

void ScatterNewPlotFrame::update(LowessParamObservable* o)
{
	ScatterNewPlotCanvas* t = (ScatterNewPlotCanvas*) template_canvas;
	t->ChangeLoessParams(o->GetF(), o->GetIter(), o->GetDeltaFactor());
}

void ScatterNewPlotFrame::notifyOfClosing(LowessParamObservable* o)
{
	lowess_param_frame = 0;
}

void ScatterNewPlotFrame::ChangeThemeType(
								CatClassification::CatClassifType new_theme,
								int num_categories,
								const wxString& custom_classif_title)
{
	((ScatterNewPlotCanvas*) template_canvas)->
		ChangeThemeType(new_theme, num_categories, custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	if (template_legend) template_legend->Recreate();
}

void ScatterNewPlotFrame::AdjustBubbleSize(wxCommandEvent& evt)
{
    BubbleSizeSliderDlg sliderDlg(dynamic_cast<ScatterNewPlotCanvas*>(template_canvas));
    sliderDlg.ShowModal();
}

void ScatterNewPlotFrame::GetVizInfo(wxString& x, wxString& y)
{
	if (var_info.size() > 1) {
		x = var_info[0].name;
		y = var_info[1].name;
	}
}

