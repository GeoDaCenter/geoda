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

#include <utility> // std::pair
#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/regex.h>

#include "../HighlightState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "SimpleScatterPlotCanvas.h"


IMPLEMENT_CLASS(SimpleScatterPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(SimpleScatterPlotCanvas, TemplateCanvas)
EVT_PAINT(TemplateCanvas::OnPaint)
EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

SimpleScatterPlotCanvas::
SimpleScatterPlotCanvas(wxWindow *parent,
                        TemplateFrame* t_frame,
                        Project* project,
                        HLStateInt* hl_state_int,
                        SimpleScatterPlotCanvasCbInt* ssp_canv_cb_,
                        const std::vector<double>& X_,
                        const std::vector<double>& Y_,
                        const std::vector<bool>& X_undef_,
                        const std::vector<bool>& Y_undef_,
                        const wxString& Xname_,
                        const wxString& Yname_,
                        double Xmin_, double Xmax_,
                        double Ymin_, double Ymax_,
                        bool add_auto_padding_min_,
                        bool add_auto_padding_max_,
                        bool use_larger_filled_circles_,
                        const wxString& right_click_menu_id_,
                        bool show_axes_,
                        bool show_horiz_axis_through_origin_,
                        bool show_vert_axis_through_origin_,
                        bool show_regimes_,
                        bool show_linear_smoother_,
                        bool show_lowess_smoother_,
                        bool show_slope_values_,
                        bool view_standardized_data_,
                        const wxPoint& pos,
                        const wxSize& size)
:TemplateCanvas(parent, t_frame, project, hl_state_int, pos, size, false, true),
ssp_canv_cb(ssp_canv_cb_),
X(X_), Y(Y_), X_undef(X_undef_), Y_undef(Y_undef_),
orgX(X_), orgY(Y_), Xname(Xname_), Yname(Yname_),
Xmin(Xmin_), Xmax(Xmax_), Ymin(Ymin_), Ymax(Ymax_),
add_auto_padding_min(add_auto_padding_min_),
add_auto_padding_max(add_auto_padding_max_),
use_larger_filled_circles(use_larger_filled_circles_),
right_click_menu_id(right_click_menu_id_),
show_axes(show_axes_),
show_horiz_axis_through_origin(show_horiz_axis_through_origin_),
show_vert_axis_through_origin(show_vert_axis_through_origin_),
show_regimes(show_regimes_),
show_linear_smoother(show_linear_smoother_),
show_lowess_smoother(show_lowess_smoother_),
show_slope_values(show_slope_values_),
view_standardized_data(view_standardized_data_)
{
	highlight_color = GdaConst::scatterplot_regression_selected_color;
	selectable_fill_color = GdaConst::scatterplot_regression_excluded_color;
	selectable_outline_color = GdaConst::scatterplot_regression_color;
	
    last_scale_trans.SetData(0, 0, 100, 100);
	UpdateMargins();
	
	use_category_brushes = true;
	draw_sel_shps_by_z_val = false;
	highlight_color = GdaConst::scatterplot_regression_selected_color;
	selectable_fill_color = GdaConst::scatterplot_regression_excluded_color;
	selectable_outline_color = GdaConst::scatterplot_regression_color;
	// 1 = #cats
	cat_data.CreateCategoriesAllCanvasTms(1, 1, X.size());
	cat_data.SetCategoryColor(0, 0, selectable_fill_color);
	for (int i=0, sz=X.size(); i<sz; i++) cat_data.AppendIdToCategory(0, 0, i);
	cat_data.SetCurrentCanvasTmStep(0);
	
	PopulateCanvas();
	ResizeSelectableShps();
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

SimpleScatterPlotCanvas::~SimpleScatterPlotCanvas()
{
	LOG_MSG("Entering SimpleScatterPlotCanvas::~SimpleScatterPlotCanvas");
	EmptyLowessCache();
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting SimpleScatterPlotCanvas::~SimpleScatterPlotCanvas");
}

void SimpleScatterPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering SimpleScatterPlotCanvas::DisplayRightClickMenu");
	if (right_click_menu_id.IsEmpty()) return;	
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	template_frame->OnActivate(ae);
	
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->LoadMenu(right_click_menu_id);
	if (!optMenu) return;
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting SimpleScatterPlotCanvas::DisplayRightClickMenu");
}

void SimpleScatterPlotCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
    if (IsShowRegimes() && IsShowLinearSmoother()) {
        SmoothingUtils::CalcStatsRegimes(X, Y, X_undef, Y_undef,
                                         statsX, statsY, regressionXY,
                                         highlight_state->GetHighlight(),
                                         statsXselected, statsYselected,
                                         statsXexcluded, statsYexcluded,
                                         regressionXYselected,
                                         regressionXYexcluded,
                                         sse_sel, sse_unsel);
        UpdateLinearRegimesRegLines();
    }
    
    if (IsShowRegimes() && IsShowLowessSmoother()) {
        UpdateLowessOnRegimes();
    }
    //if (IsDisplayStats() && IsShowLinearSmoother()) UpdateDisplayStats();
    
    if (IsShowRegimes()) {
        // we only need to redraw everything if the optional
        // regression lines have changed.
        Refresh();
    }
    TemplateCanvas::UpdateSelection(shiftdown, pointsel);
}
/**
 Override of TemplateCanvas method.  We must still call the
 TemplateCanvas method after we update the regression lines
 as needed. */
void SimpleScatterPlotCanvas::update(HLStateInt* o)
{
	LOG_MSG("Entering SimpleScatterPlotCanvas::update");
	
	if (IsShowRegimes() && IsShowLinearSmoother()) {
		SmoothingUtils::CalcStatsRegimes(X, Y, X_undef, Y_undef,
                                         statsX, statsY, regressionXY,
										 highlight_state->GetHighlight(),
										 statsXselected, statsYselected,
										 statsXexcluded, statsYexcluded,
										 regressionXYselected, 
										 regressionXYexcluded,
										 sse_sel, sse_unsel);
		UpdateLinearRegimesRegLines();
	}
	
	if (IsShowRegimes() && IsShowLowessSmoother()) {
		UpdateLowessOnRegimes();
	}
	//if (IsDisplayStats() && IsShowLinearSmoother()) UpdateDisplayStats();
	
	// Call TemplateCanvas::update to redraw objects as needed.
	TemplateCanvas::update(o);
	
	if (IsShowRegimes()) {
		// we only need to redraw everything if the optional
		// regression lines have changed.
		Refresh();
	}

    UpdateStatusBar();
    
	LOG_MSG("Exiting ScatterNewPlotCanvas::update");	
}

void SimpleScatterPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
}

wxString SimpleScatterPlotCanvas::GetCanvasTitle()
{
	wxString s("Scatter Plot");	
	s << " - x: " << Xname << ", y: " << Yname;
	return s;
}

void SimpleScatterPlotCanvas::UpdateStatusBar()
{
	if (template_frame) {
		wxStatusBar* sb = template_frame->GetStatusBar();
		if (mousemode == select && selectstate == start) {
			if (template_frame->GetStatusBarStringFromFrame()) {
                wxString str = template_frame->GetUpdateStatusBarString(hover_obs, total_hover_obs);
				sb->SetStatusText(str);
			}
            wxString s;
            wxString old_s = sb->GetStatusText(0);
            
            const std::vector<bool>& hl = highlight_state->GetHighlight();
            
            if (highlight_state->GetTotalHighlighted()> 0) {
                int n_total_hl = highlight_state->GetTotalHighlighted();
                s << "#selected=" << n_total_hl << "  ";
                
                int n_undefs = 0;
                for (int i=0; i<X.size(); i++) {
                    if ( (X_undef[i] || Y_undef[i]) && hl[i]) {
                        n_undefs += 1;
                    }
                }
                
                if (n_undefs> 0) {
                    s << "undefined: ";
                    wxString undef_str;
                    undef_str << "@" << Xname << "/" << Yname << "";
                    
                    wxRegEx re_select("[0-9]+@([a-zA-Z0-9_-]+)/([a-zA-Z0-9_-]+)");
                    while (re_select.Matches(old_s)) {
                        size_t start, len;
                        re_select.GetMatch(&start, &len, 0);
                        wxString f = re_select.GetMatch(old_s, 0);
                        wxString f1 = re_select.GetMatch(old_s, 1);
                        wxString f2 = re_select.GetMatch(old_s, 2);
                        if (!undef_str.Contains(f1) || !undef_str.Contains(f2)) {
                            s << f << " ";
                        }
                        
                        old_s = old_s.Mid (start + len);
                    }
                    s << n_undefs << undef_str << " ";
                }
            }
            
            if (mousemode == select && selectstate == start) {
                if (total_hover_obs >= 1) {
                    s << "hover obs " << hover_obs[0]+1 << " = (";
                    s << X[hover_obs[0]] << ", " << Y[hover_obs[0]];
                    s << ")";
                }
                if (total_hover_obs >= 2) {
                    s << ", ";
                    s << "obs " << hover_obs[1]+1 << " = (";
                    s << X[hover_obs[1]] << ", " << Y[hover_obs[1]];
                    s << ")";
                }
                if (total_hover_obs >= 3) {
                    s << ", ";
                    s << "obs " << hover_obs[2]+1 << " = (";
                    s << X[hover_obs[2]] << ", " << Y[hover_obs[2]];
                    s << ")";
                }
                if (total_hover_obs >= 4) {
                    s << ", ...";
                }
            }
            sb->SetStatusText(s);

		}
	}
	
	if (ssp_canv_cb) {
		ssp_canv_cb->notifyNewHover(hover_obs, total_hover_obs);
	}
}

void SimpleScatterPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	
}

void SimpleScatterPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	
}

void SimpleScatterPlotCanvas::SetSelectableOutlineColor(wxColour color)
{
    selectable_outline_color = color;
    TemplateCanvas::SetSelectableOutlineColor(color);
    PopulateCanvas();
}

void SimpleScatterPlotCanvas::SetHighlightColor(wxColour color)
{
    highlight_color = color;
    PopulateCanvas();
}

void SimpleScatterPlotCanvas::SetSelectableFillColor(wxColour color)
{
    // In Scatter Plot, Fill color is for points
    selectable_fill_color = color;
	cat_data.SetCategoryColor(0, 0, selectable_fill_color);
    TemplateCanvas::SetSelectableFillColor(color);
    PopulateCanvas();
    
}

void SimpleScatterPlotCanvas::ShowAxes(bool display)
{
	show_axes = display;
	UpdateMargins();
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ShowRegimes(bool display)
{
	show_regimes = display;
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ViewStandardizedData(bool display)
{
	view_standardized_data = display;
    X = orgX;
    Y = orgY;
    EmptyLowessCache();
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ViewOriginalData(bool display)
{
	view_standardized_data = !display;
    X = orgX;
    Y = orgY;
    EmptyLowessCache();
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ShowLinearSmoother(bool display)
{
	show_linear_smoother = display;
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ShowLowessSmoother(bool display)
{
	show_lowess_smoother = display;
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ShowSlopeValues(bool display)
{
	show_slope_values = display;
	PopulateCanvas();
}

void SimpleScatterPlotCanvas::ChangeLoessParams(double f, int iter, double delta_factor)
{
	EmptyLowessCache();
	lowess.SetF(f);
	lowess.SetIter(iter);
	lowess.SetDeltaFactor(delta_factor);
	if (IsShowLowessSmoother()) PopulateCanvas();
}

void SimpleScatterPlotCanvas::UpdateLinearRegimesRegLines()
{
	LOG_MSG("In SimpleScatterPlotCanvas::UpdateLinearRegimesRegLines");
	if (IsShowLinearSmoother()) {
		pens.SetPenColor(pens.GetRegSelPen(), highlight_color);
		pens.SetPenColor(pens.GetRegExlPen(), GdaConst::scatterplot_regression_excluded_color);
		double cc_degs_of_rot;
		wxRealPoint a, b;
		
		double reg_line_selected_slope;
		bool reg_line_selected_infinite_slope;
		bool reg_line_selected_defined;
		SmoothingUtils::CalcRegressionLine(*reg_line_selected,
                                           reg_line_selected_slope,
                                           reg_line_selected_infinite_slope,
                                           reg_line_selected_defined, a, b,
                                           cc_degs_of_rot,
                                           axis_scale_x, axis_scale_y,
                                           regressionXYselected,
                                           *pens.GetRegSelPen());
		ApplyLastResizeToShp(reg_line_selected);
		
		double reg_line_excluded_slope;
		bool reg_line_excluded_infinite_slope;
		bool reg_line_excluded_defined;	
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
		reg_line_selected->setPen(*wxTRANSPARENT_PEN);
	}
}

/** Called when selection changes */
void SimpleScatterPlotCanvas::UpdateLowessOnRegimes()
{
	if (!lowess_reg_line_selected && !lowess_reg_line_excluded) return;
	size_t n = X.size();
	wxString key = SmoothingUtils::LowessCacheKey(0, 0);
	LOG(key);
	SmoothingUtils::LowessCacheType::iterator it = lowess_cache.find(key);
	SmoothingUtils::LowessCacheEntry* lce = 0;
	if (it != lowess_cache.end()) {
		lce = it->second ;
	} else {
		LOG_MSG("Error: could not find LowessCacheEntry for key: " + key);
	}
	if (!lce) {
		LOG_MSG("Error: LowessCacheEntry NULL for key: " + key);
		return;
	}
	
	std::vector<double> sel_smthd_srt_x;
	std::vector<double> sel_smthd_srt_y;
	std::vector<double> unsel_smthd_srt_x;
	std::vector<double> unsel_smthd_srt_y;
    
    std::vector<bool> XY_undef;
    for (size_t i=0; i<X_undef.size(); i++) {
        XY_undef.push_back(X_undef[i] || Y_undef[i]);
    }
    
	if (IsShowRegimes()) {
		SmoothingUtils::CalcLowessRegimes(lce, lowess,
										  highlight_state->GetHighlight(),
										  sel_smthd_srt_x, sel_smthd_srt_y,
										  unsel_smthd_srt_x, unsel_smthd_srt_y,
                                          XY_undef);
	}
	if (lowess_reg_line_selected) {
		if (sel_smthd_srt_x.size() > 0 && IsShowRegimes()) {
			lowess_reg_line_selected->reInit(sel_smthd_srt_x, sel_smthd_srt_y,
											 axis_scale_x.scale_min,
											 axis_scale_y.scale_min,
											 scaleX, scaleY);
			
			lowess_reg_line_selected->setPen(*pens.GetRegSelPen());
		} else {
			lowess_reg_line_selected->operator=(GdaSpline());
		}
		ApplyLastResizeToShp(lowess_reg_line_selected);
	}
	
	if (lowess_reg_line_excluded) {
		if (unsel_smthd_srt_x.size() > 0 && IsShowRegimes()) {
			lowess_reg_line_excluded->reInit(unsel_smthd_srt_x, unsel_smthd_srt_y,
											 axis_scale_x.scale_min,
											 axis_scale_y.scale_min,
											 scaleX, scaleY);
			
			lowess_reg_line_excluded->setPen(*pens.GetRegExlPen());
		} else {
			lowess_reg_line_excluded->operator=(GdaSpline());
		}
		ApplyLastResizeToShp(lowess_reg_line_excluded);
	}
	layer2_valid = false;
}	

void SimpleScatterPlotCanvas::PopulateCanvas()
{
	LOG_MSG("Entering SimpleScatterPlotCanvas::PopulateCanvas");
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	wxSize size(GetVirtualSize());
    last_scale_trans.SetView(size.GetWidth(), size.GetHeight());
    
	// Recall: Xmin/max Ymin/max can be smaller/larger than min/max in X/Y
	//    if X/Y are particular time-slices of time-variant variables and
	//    if global scaling is being used.
	double x_min = Xmin;
	double x_max = Xmax;
	double y_min = Ymin;
	double y_max = Ymax;
	
	pens.SetPenColor(pens.GetRegPen(), selectable_outline_color);
	pens.SetPenColor(pens.GetRegSelPen(), highlight_color);
	pens.SetPenColor(pens.GetRegExlPen(),
                     GdaConst::scatterplot_regression_excluded_color);
	
	statsX = SampleStatistics(X, X_undef, Y_undef);
	statsY = SampleStatistics(Y, Y_undef, X_undef);
    
    if (view_standardized_data) {
        for (int i=0, iend=X.size(); i<iend; i++) {
            X[i] = (X[i]-statsX.mean)/statsX.sd_with_bessel;
            Y[i] = (Y[i]-statsY.mean)/statsY.sd_with_bessel;
        }
        // we are ignoring the global scaling option here
        x_max = (statsX.max - statsX.mean)/statsX.sd_with_bessel;
        x_min = (statsX.min - statsX.mean)/statsX.sd_with_bessel;
        y_max = (statsY.max - statsY.mean)/statsY.sd_with_bessel;
        y_min = (statsY.min - statsY.mean)/statsY.sd_with_bessel;
        statsX = SampleStatistics(X, X_undef, Y_undef);
        statsY = SampleStatistics(Y, Y_undef, Y_undef);
        // mean shold be 0 and biased standard deviation should be 1
        double eps = 0.000001;
        if (-eps < statsX.mean && statsX.mean < eps)
            statsX.mean = 0;
        if (-eps < statsY.mean && statsY.mean < eps)
            statsY.mean = 0;
    }
	
	regressionXY = SimpleLinearRegression(X, Y, X_undef, Y_undef,
                                          statsX.mean, statsY.mean,
										  statsX.var_without_bessel,
										  statsY.var_without_bessel);
	sse_c = regressionXY.error_sum_squares;
    
	double x_pad = 0.1 * (x_max - x_min);
	double y_pad = 0.1 * (y_max - y_min);
	axis_scale_x = AxisScale(x_min - (add_auto_padding_min ? x_pad : 0.0),
							 x_max + (add_auto_padding_max ? x_pad : 0.0),
                             5, axis_display_precision);
	axis_scale_y = AxisScale(y_min - (add_auto_padding_min ? y_pad : 0.0),
							 y_max + (add_auto_padding_max ? y_pad : 0.0),
                             5, axis_display_precision);

    /*
	// used by status bar for showing selection rectangle range
	data_scale_xmin = axis_scale_x.scale_min;
	data_scale_xmax = axis_scale_x.scale_max;
	data_scale_ymin = axis_scale_y.scale_min;
	data_scale_ymax = axis_scale_y.scale_max;
     */
	
	// Populate TemplateCanvas::selectable_shps
	selectable_shps.resize(X.size());
    selectable_shps_undefs.resize(X.size());
	scaleX = 100.0 / (axis_scale_x.scale_range);
	scaleY = 100.0 / (axis_scale_y.scale_range);
	
	if (use_larger_filled_circles) {
		selectable_shps_type = circles;
		for (size_t i=0, sz=X.size(); i<sz; ++i) {
            selectable_shps_undefs[i] = X_undef[i] || Y_undef[i];
            
			GdaCircle* c = 0;
			c = new GdaCircle(wxRealPoint((X[i]-axis_scale_x.scale_min) * scaleX,
										  (Y[i]-axis_scale_y.scale_min) * scaleY),
							  2.5);
			c->setPen(GdaConst::scatterplot_regression_excluded_color);
			c->setBrush(GdaConst::scatterplot_regression_excluded_color);
			selectable_shps[i] = c;
		}
	} else {
		selectable_shps_type = points;
		for (size_t i=0, sz=X.size(); i<sz; ++i) {
            selectable_shps_undefs[i] = X_undef[i] || Y_undef[i];
            
			selectable_shps[i] = 
			new GdaPoint(wxRealPoint((X[i] - axis_scale_x.scale_min) * scaleX,
									 (Y[i] - axis_scale_y.scale_min) * scaleY));
		}
	}
	
	// create axes
	if (show_axes) {
    	x_baseline = new GdaAxis(Xname, axis_scale_x,
    							 wxRealPoint(0,0), wxRealPoint(100, 0));
		x_baseline->setPen(*GdaConst::scatterplot_scale_pen);
        foreground_shps.push_back(x_baseline);
	}
    
	if (show_axes) {
		y_baseline->setPen(*GdaConst::scatterplot_scale_pen);
    	y_baseline = new GdaAxis(Yname, axis_scale_y,
    							 wxRealPoint(0,0), wxRealPoint(0, 100));
        foreground_shps.push_back(y_baseline);
	}
	
	// create optional axes through origin
	if (show_horiz_axis_through_origin &&
			axis_scale_y.scale_min < 0 && 0 < axis_scale_y.scale_max)
	{
		GdaPolyLine* x_axis_through_origin = new GdaPolyLine(0,50,100,50);
		double y_inter = 100.0 * ((-axis_scale_y.scale_min) /
								  (axis_scale_y.scale_max-axis_scale_y.scale_min));
		x_axis_through_origin->operator=(GdaPolyLine(0,y_inter,100,y_inter));
		x_axis_through_origin->setPen(*GdaConst::scatterplot_origin_axes_pen);
		foreground_shps.push_back(x_axis_through_origin);
	}
	if (show_vert_axis_through_origin &&
			axis_scale_x.scale_min < 0 && 0 < axis_scale_x.scale_max)
	{
		GdaPolyLine* y_axis_through_origin = new GdaPolyLine(50,0,50,100);
		double x_inter = 100.0 * ((-axis_scale_x.scale_min) /
								  (axis_scale_x.scale_max-axis_scale_x.scale_min));
		y_axis_through_origin->operator=(GdaPolyLine(x_inter,0,x_inter,100));
		y_axis_through_origin->setPen(*GdaConst::scatterplot_origin_axes_pen);
		foreground_shps.push_back(y_axis_through_origin);
	}
	
	// create lowess regression lines
	lowess_reg_line = new GdaSpline;
	lowess_reg_line_selected = new GdaSpline;
	lowess_reg_line_excluded = new GdaSpline;
	foreground_shps.push_back(lowess_reg_line);
	foreground_shps.push_back(lowess_reg_line_selected);
	foreground_shps.push_back(lowess_reg_line_excluded);
	
	if (IsShowLowessSmoother() && X.size() > 1) {
		LOG_MSG("Begin populating LOWESS curve (all obs)");
		size_t n = X.size();
		wxString key = SmoothingUtils::LowessCacheKey(0, 0);
		
        std::vector<bool> XY_undefs;
        for (size_t ii=0; ii<X_undef.size(); ii++){
            XY_undefs.push_back(X_undef[ii] || Y_undef[ii]);
        }
        
		SmoothingUtils::LowessCacheEntry* lce = SmoothingUtils::UpdateLowessCacheForTime(lowess_cache, key, lowess, X, Y, XY_undefs);
		
		if (!lce) {
			LOG_MSG("Error: could not create or find LOWESS cache entry");
		} else {
			lowess_reg_line->reInit(lce->X_srt, lce->YS_srt,
									axis_scale_x.scale_min,
									axis_scale_y.scale_min,
									scaleX, scaleY);
			lowess_reg_line->setPen(*pens.GetRegPen());
			
			LOG_MSG("End populating LOWESS curve (all obs)");
		}
		if (IsShowRegimes()) {
			UpdateLowessOnRegimes();
		}
	}
	
	// create linear regression lines
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
	
	if (IsShowLinearSmoother()) {
		double cc_degs_of_rot;
		double reg_line_slope;
		bool reg_line_infinite_slope;
		bool reg_line_defined;
		wxRealPoint a, b;
		SmoothingUtils::CalcRegressionLine(*reg_line, reg_line_slope,
										   reg_line_infinite_slope,
										   reg_line_defined, a, b, 
										   cc_degs_of_rot,
										   axis_scale_x, axis_scale_y,
										   regressionXY, *pens.GetRegPen());
	}
	
	if (IsShowRegimes()) {
		// update both selected and excluded stats
		SmoothingUtils::CalcStatsRegimes(X, Y, X_undef, Y_undef,
                                         statsX, statsY, regressionXY,
										 highlight_state->GetHighlight(),
										 statsXselected, statsYselected,
										 statsXexcluded, statsYexcluded,
										 regressionXYselected, 
										 regressionXYexcluded,
										 sse_sel, sse_unsel);
		UpdateLinearRegimesRegLines();
	}
	
	if (IsShowSlopeValues() && regressionXY.valid) {
		wxString s;
		s << GenUtils::DblToStr(regressionXY.beta);
		if (regressionXY.p_value_beta <= 0.01 && statsX.sample_size >= 3) {
			s << "**";
		} else if (regressionXY.p_value_beta <= 0.05 && statsX.sample_size >= 3) {
			s << "*";
		}
		GdaShapeText* t = new GdaShapeText(s, *GdaConst::small_font,
										   wxRealPoint(50, 100), 0,
										   GdaShapeText::h_center,
										   GdaShapeText::v_center, 0, 5);
		t->setPen(*GdaConst::scatterplot_scale_pen);
		foreground_shps.push_back(t);
	}
	
	//chow_test_text = new GdaShapeText();
	//chow_test_text->hidden = true;
	//foreground_shps.push_back(chow_test_text);
	//stats_table = new GdaShapeTable();
	//stats_table->hidden = true;
	//foreground_shps.push_back(stats_table);

	ResizeSelectableShps();
	
	LOG_MSG("Exiting SimpleScatterPlotCanvas::PopulateCanvas");
}

void SimpleScatterPlotCanvas::UpdateMargins()
{
	int virtual_screen_marg_top = 5;//20;
	int virtual_screen_marg_right = 5;//20;
	int virtual_screen_marg_bottom = 5;//45;
	int virtual_screen_marg_left = 5;//45;
    
	if (show_axes) {
		virtual_screen_marg_bottom = 45;//45;
		virtual_screen_marg_left = 45;//45;
	}
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);
}

/** Free allocated points arrays in lowess_cache and clear cache */
void SimpleScatterPlotCanvas::EmptyLowessCache()
{
	SmoothingUtils::EmptyLowessCache(lowess_cache);
}
