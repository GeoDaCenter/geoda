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

#ifndef __GEODA_CENTER_ANIMATEPLOT_CANVAS_VIEW_H__
#define __GEODA_CENTER_ANIMATEPLOT_CANVAS_VIEW_H__

#include <map>
#include <vector>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/thread.h>

#include "../TemplateCanvas.h"
#include "../GdaShape.h"

class HighlightState;
class Project;

class AnimatePlotcanvas : public TemplateCanvas
{
public:
	DECLARE_CLASS(AnimatePlotcanvas)

    enum Style {
        DEFAULT_STYLE = 0,
        add_auto_padding_min = 1, //0b00000001
        add_auto_padding_max = 2, //0b00000010
        use_larger_filled_circles = 4,
        show_axes = 8,
        show_horiz_axis_through_origin = 16,
        show_vert_axis_through_origin = 32,
        show_data_points = 64,
        view_standardized_data = 128
    };

	AnimatePlotcanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project,
                    const std::vector<double>& X,
                    const std::vector<double>& Y,
                    const std::vector<bool>& X_undf,
                    const std::vector<bool>& Y_undef,
                    const wxString& Xname,
                    const wxString& Yname,
                    int style,
                      const std::vector<std::vector<int> >& groups,
                    const wxString& right_click_menu_id = wxEmptyString,
                    const wxString& title = wxEmptyString,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize);
	virtual ~AnimatePlotcanvas();

	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual void UpdateStatusBar();
    virtual void UpdateSelection(bool shiftdown, bool pointsel);
	
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);

    virtual void DrawLayers();
    virtual void OnPaint(wxPaintEvent& event);
    
    void SetSelectableOutlineColor(wxColour color);
    void SetSelectableFillColor(wxColour color);
    void SetHighlightColor(wxColour color);
    
	void ShowAxes(bool display);
	void ShowRegimes(bool display);
	bool IsShowAxes() { return show_axes; }
    void UpdateCanvas(int idx, double *data);
    void CreateAndUpdateCategories(const std::vector<std::vector<int> >& groups);
    std::vector<double> GetSelectX(int idx);
    std::vector<double> GetSelectY(int idx);
protected:
    
    virtual void PopulateCanvas();
    void UpdateMargins();

    bool  is_drawing;
    wxMutex mutex_prerender;

	//ScatterPlotPens pens;
	// data
    size_t n_pts;
	std::vector<double> X;
	std::vector<double> Y;
    std::vector<double> W;
	std::vector<bool> X_undef;
	std::vector<bool> Y_undef;
	const std::vector<double>& orgX;
	const std::vector<double>& orgY;
    std::vector<std::vector<int> > groups;
    std::vector<std::vector<double> > X_cache;
    std::vector<std::vector<double> > Y_cache;
    // x,y labels or variable name
    wxString Xname;
	wxString Yname;
	// used for scaling, so can be smaller/larger than min/max in X/Y
	double Xmin, Xmax, Ymin, Ymax;

    // user defined y-axis range
    wxString def_y_min;
    wxString def_y_max;

    // axis
	AxisScale axis_scale_x;
	AxisScale axis_scale_y;

	double scaleX;
	double scaleY;

    // axes
	GdaAxis* x_baseline;
	GdaAxis* y_baseline;

    // style
    int style;
	wxString right_click_menu_id;

	DECLARE_EVENT_TABLE()
};

#endif
