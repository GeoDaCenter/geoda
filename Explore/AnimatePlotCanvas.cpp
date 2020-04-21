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
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/regex.h>
#include <boost/foreach.hpp>
#include "../TemplateFrame.h"
#include "../GdaShape.h"
#include "../HighlightState.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../Project.h"
#include "AnimatePlotCanvas.h"


IMPLEMENT_CLASS(AnimatePlotcanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(AnimatePlotcanvas, TemplateCanvas)
EVT_PAINT(TemplateCanvas::OnPaint)
EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

AnimatePlotcanvas::
AnimatePlotcanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project,
                const std::vector<double>& X, const std::vector<double>& Y,
                const std::vector<bool>& X_undef, const std::vector<bool>& Y_undef,
                const wxString& Xname, const wxString& Yname, int style,
                const wxString& right_click_menu_id, const wxString& title,
                const wxPoint& pos, const wxSize& size)
:TemplateCanvas(parent, t_frame, project, project->GetHighlightState(), pos, size, false, true),
X(X), Y(Y), X_undef(X_undef), Y_undef(Y_undef),
orgX(X), orgY(Y), Xname(Xname), Yname(Yname),
right_click_menu_id(right_click_menu_id), style(style), is_drawing(false)
{
    // setup colors
    use_category_brushes = true;
    draw_sel_shps_by_z_val = false;
	highlight_color = GdaConst::scatterplot_regression_selected_color;
	selectable_fill_color = GdaConst::scatterplot_regression_excluded_color;
	selectable_outline_color = GdaConst::scatterplot_regression_color;

	// setup margins
    int virtual_screen_marg_top = 20;//20;
    int virtual_screen_marg_right = 5;//20;
    int virtual_screen_marg_bottom = 5;//45;
    int virtual_screen_marg_left = 5;//45;

    if (style & show_axes) {
        virtual_screen_marg_bottom = 40;//45;
        virtual_screen_marg_left = 30;//45;
    }
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);

    // setup data range
    last_scale_trans.SetData(0, 0, 100, 100);
    n_pts = X.size();
    Xmin = DBL_MAX; Ymin = DBL_MAX;
    Xmax = DBL_MIN; Ymax = DBL_MIN;
    for (size_t i=0; i<n_pts; ++i) {
        if (Xmin > X[i]) Xmin = X[i];
        if (Xmax < X[i]) Xmax = X[i];
        if (Ymin > Y[i]) Ymin = Y[i];
        if (Ymax < Y[i]) Ymax = Y[i];
    }

	// put all scatter points in 1 category
	cat_data.CreateCategoriesAllCanvasTms(1 /*time*/, 1/*cats*/, X.size());
	cat_data.SetCategoryPenColor(0, 0, selectable_fill_color);
    cat_data.SetCategoryBrushColor(0, 0, *wxWHITE);
	for (int i=0, sz=X.size(); i<sz; i++) cat_data.AppendIdToCategory(0, 0, i);
	cat_data.SetCurrentCanvasTmStep(0);

	PopulateCanvas();
	ResizeSelectableShps();
	
	highlight_state->registerObserver(this);
}

AnimatePlotcanvas::~AnimatePlotcanvas()
{
	wxLogMessage("Entering AnimatePlotcanvas::~AnimatePlotcanvas");
    if (highlight_state) highlight_state->removeObserver(this);
	wxLogMessage("Exiting AnimatePlotcanvas::~AnimatePlotcanvas");
}


void AnimatePlotcanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	wxLogMessage("Entering AnimatePlotcanvas::DisplayRightClickMenu");
	//if (right_click_menu_id.IsEmpty())
    //    return;
    ///wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_SCATTER_PLOT_MAT_MENU_OPTIONS");
    //template_frame->UpdateContextMenuItems(optMenu);
    //template_frame->PopupMenu(optMenu, pos + GetPosition());
    //template_frame->UpdateOptionMenuItems();
	wxLogMessage("Exiting AnimatePlotcanvas::DisplayRightClickMenu");
}

void AnimatePlotcanvas::UpdateCanvas(int idx, double *data)
{
    if (data) {
        size_t new_col = 2; // hard coded to 2
        for (size_t i=0; i<new_col; i++) {
            for (int j = 0; j < n_pts; ++j) {
                if (i ==0)
                    X[j] = data[j*new_col + i];
                else
                    Y[j] = data[j*new_col + i];
            }
        }
        if (idx < X_cache.size()) X_cache[idx] = X;
        else X_cache.push_back(X);
        if (idx < Y_cache.size()) Y_cache[idx] = Y;
        else Y_cache.push_back(Y);

    } else {
        if (idx < X_cache.size())  X = X_cache[idx];
        if (idx < X_cache.size())  Y = Y_cache[idx];
    }

    Xmin = DBL_MAX; Ymin = DBL_MAX;
    Xmax = DBL_MIN; Ymax = DBL_MIN;
    for (size_t i=0; i<n_pts; ++i) {
        if (Xmin > X[i]) Xmin = X[i];
        if (Xmax < X[i]) Xmax = X[i];
        if (Ymin > Y[i]) Ymin = Y[i];
        if (Ymax < Y[i]) Ymax = Y[i];
    }
    if (is_drawing == false) {
        is_drawing = true;
        PopulateCanvas();
        Refresh();
    } else {
        std::cout << "not drawing: " << idx << std::endl;
    }
}

void AnimatePlotcanvas::DrawLayers()
{
    //mutex.Lock();
    wxMutexLocker lock(mutex_prerender); // make sure prerender lock is released
    TemplateCanvas::DrawLayers();
    //mutex.Unlock();
}

void AnimatePlotcanvas::OnPaint(wxPaintEvent& event)
{
    //TemplateCanvas::OnPaint(event);
    if (layer2_bm) {
        wxSize sz = GetClientSize();
        wxMemoryDC dc(*layer2_bm);
        wxPaintDC paint_dc(this);
        paint_dc.Blit(0, 0, sz.x, sz.y, &dc, 0, 0);
        // Draw optional control objects if needed
        PaintControls(paint_dc);
        helper_PaintSelectionOutline(paint_dc);
        is_drawing = false;
    }
    event.Skip();
}

void AnimatePlotcanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
    TemplateCanvas::UpdateSelection(shiftdown, pointsel);
}

/**
 Override of TemplateCanvas method.  We must still call the
 TemplateCanvas method after we update the regression lines
 as needed. */
void AnimatePlotcanvas::update(HLStateInt* o)
{
    TemplateCanvas::update(o);
}

void AnimatePlotcanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
}

wxString AnimatePlotcanvas::GetCanvasTitle()
{
    wxString s = _("Animate Plot- x: %s, y: %s");
    s = wxString::Format(s, Xname, Yname);
	return s;
}

wxString AnimatePlotcanvas::GetVariableNames()
{
    wxString s;
    s << Xname << ", " << Yname;
    return s;
}

void AnimatePlotcanvas::UpdateStatusBar()
{
}

void AnimatePlotcanvas::TimeSyncVariableToggle(int var_index)
{
}

void AnimatePlotcanvas::FixedScaleVariableToggle(int var_index)
{
    TemplateCanvas::FixedScaleVariableToggle(var_index);
}

void AnimatePlotcanvas::SetSelectableOutlineColor(wxColour color)
{
    selectable_outline_color = color;
    TemplateCanvas::SetSelectableOutlineColor(color);
    PopulateCanvas();
}

void AnimatePlotcanvas::SetHighlightColor(wxColour color)
{
    highlight_color = color;
    PopulateCanvas();
}

void AnimatePlotcanvas::SetSelectableFillColor(wxColour color)
{
    // In Scatter Plot, Fill color is for points
    selectable_fill_color = color;
	cat_data.SetCategoryPenColor(0, 0, selectable_fill_color);
    TemplateCanvas::SetSelectableFillColor(color);
    PopulateCanvas();
}

void AnimatePlotcanvas::ShowAxes(bool display)
{
    if (display) style = style | show_axes;
	UpdateMargins();
	PopulateCanvas();
}

std::vector<double> AnimatePlotcanvas::GetSelectX(int idx)
{
    if (idx < X_cache.size()) {
        return X_cache[idx];
    }
    return std::vector<double>();
}

std::vector<double> AnimatePlotcanvas::GetSelectY(int idx)
{
    if (idx < Y_cache.size()) {
        return Y_cache[idx];
    }
    return std::vector<double>();
}

void AnimatePlotcanvas::PopulateCanvas()
{
	LOG_MSG("Entering AnimatePlotcanvas::PopulateCanvas");
    mutex_prerender.Lock();
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

    // create axis
    double data_min_s = x_min;
    double data_max_s = x_max;
    double x_pad = 0.1 * (x_max - x_min);

    if (style & add_auto_padding_min) data_min_s = x_min - x_pad;
    if (style & add_auto_padding_max) data_max_s = x_max - x_pad;

	axis_scale_x = AxisScale(data_min_s, data_max_s, 5, axis_display_precision,
                             axis_display_fixed_point);

    //std::cout << data_min_s << "," << data_max_s << std::endl;
    double axis_min = y_min;
    double axis_max = y_max;
    double y_pad = 0.1 * (y_max - y_min);

    if (style & add_auto_padding_min) axis_min = y_min - y_pad;
    if (style & add_auto_padding_max) axis_max = y_max - y_pad;

    // check if user specifies the y-axis range
    if (!def_y_min.IsEmpty()) def_y_min.ToDouble(&axis_min);
    if (!def_y_max.IsEmpty()) def_y_max.ToDouble(&axis_max);

	axis_scale_y = AxisScale(axis_min, axis_max, 5, axis_display_precision,
                             axis_display_fixed_point);
	
	// Populate TemplateCanvas::selectable_shps
    scaleX = 100.0 / (axis_scale_x.scale_range);
    scaleY = 100.0 / (axis_scale_y.scale_range);

    if (style & show_data_points) {
        selectable_shps.resize(X.size());
        selectable_shps_undefs.resize(X.size());

        if (style & use_larger_filled_circles) {
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
                new GdaPoint(wxRealPoint((X[i]-axis_scale_x.scale_min) * scaleX,
                                         (Y[i] - axis_scale_y.scale_min) * scaleY));
            }
        }
    }
	
	// create axes
	if (style & show_axes) {
    	x_baseline = new GdaAxis(Xname, axis_scale_x,
    							 wxRealPoint(0,0), wxRealPoint(100, 0));
		x_baseline->setPen(*GdaConst::scatterplot_scale_pen);
        foreground_shps.push_back(x_baseline);
	}
    
	if (style & show_axes) {
    	y_baseline = new GdaAxis(Yname, axis_scale_y,
    							 wxRealPoint(0,0), wxRealPoint(0, 100));
        y_baseline->setPen(*GdaConst::scatterplot_scale_pen);
        foreground_shps.push_back(y_baseline);
	}
	
	// create optional axes through origin
	if ( (style & show_horiz_axis_through_origin) &&
        axis_scale_y.scale_min < 0 && axis_scale_y.scale_max > 0)
	{
		GdaPolyLine* x_axis_through_origin = new GdaPolyLine(0,50,100,50);
        double y_scale_range = axis_scale_y.scale_max-axis_scale_y.scale_min;
		double y_inter = 100.0 * ((-axis_scale_y.scale_min) / y_scale_range);
		x_axis_through_origin->operator=(GdaPolyLine(0,y_inter,100,y_inter));
		x_axis_through_origin->setPen(*GdaConst::scatterplot_origin_axes_pen);
		foreground_shps.push_back(x_axis_through_origin);
	}

	if ( (style & show_vert_axis_through_origin) &&
        axis_scale_x.scale_min < 0 && axis_scale_x.scale_max > 0)
	{
		GdaPolyLine* y_axis_through_origin = new GdaPolyLine(50,0,50,100);
        double x_scale_range = axis_scale_x.scale_max-axis_scale_x.scale_min;
		double x_inter = 100.0 * ((-axis_scale_x.scale_min) / x_scale_range);
		y_axis_through_origin->operator=(GdaPolyLine(x_inter,0,x_inter,100));
		y_axis_through_origin->setPen(*GdaConst::scatterplot_origin_axes_pen);
		foreground_shps.push_back(y_axis_through_origin);
	}

	ResizeSelectableShps();
    mutex_prerender.Unlock();
	LOG_MSG("Exiting AnimatePlotcanvas::PopulateCanvas");
}

void AnimatePlotcanvas::UpdateMargins()
{
	int virtual_screen_marg_top = 20;//20;
	int virtual_screen_marg_right = 5;//20;
	int virtual_screen_marg_bottom = 5;//45;
	int virtual_screen_marg_left = 5;//45;
    
	if (style & show_axes) {
		virtual_screen_marg_bottom = 40;//45;
		virtual_screen_marg_left = 30;//45;
	}
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);
}
