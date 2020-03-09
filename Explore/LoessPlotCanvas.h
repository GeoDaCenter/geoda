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

#ifndef __GEODA_CENTER_LOESS_CANVAS_VIEW_H__
#define __GEODA_CENTER_LOESS_CANVAS_VIEW_H__

#include <map>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/html/htmlwin.h>

#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GdaShape.h"
#include "../Algorithms/loess.h"

class HighlightState;
class Project;

class LoessPlotCanvas : public TemplateCanvas
{
public:
	DECLARE_CLASS(LoessPlotCanvas)

    enum Style {
        DEFAULT_STYLE = 0,
        add_auto_padding_min = 1, //0b00000001
        add_auto_padding_max = 2, //0b00000010
        use_larger_filled_circles = 4,
        show_axes = 8,
        show_horiz_axis_through_origin = 16,
        show_vert_axis_through_origin = 32,
        show_data_points = 64,
        show_confidence_interval = 128,
        show_lowess_smoother = 256,
        view_standardized_data = 512,
        show_slope_values = 1024
    };

	LoessPlotCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project,
                    HLStateInt* hl_state_int,
                    const std::vector<double>& X,
                    const std::vector<double>& Y,
                    const std::vector<bool>& X_undf,
                    const std::vector<bool>& Y_undef,
                    const wxString& Xname,
                    const wxString& Yname,
                    int style,
                    const wxString& right_click_menu_id = wxEmptyString,
                    const wxString& title = wxEmptyString,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize);
	virtual ~LoessPlotCanvas();

    virtual void OnEditLoess(wxCommandEvent& evt);
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual void UpdateStatusBar();
    virtual void UpdateSelection(bool shiftdown, bool pointsel);
	
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void FixedScaleVariableToggle(int var_index);

    void SetSelectableOutlineColor(wxColour color);
    void SetSelectableFillColor(wxColour color);
    void SetHighlightColor(wxColour color);
    
	void ShowAxes(bool display);
	void ShowRegimes(bool display);
	bool IsShowAxes() { return show_axes; }

    void RunLoess();
    //loess_struct* GetLoess() { return &lo; }
    loess* GetLoess() { return &lo; }
    
protected:
    
    virtual void PopulateCanvas();
    void UpdateMargins();


protected:
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

    // x,y labels or variable name
    wxString Xname;
	wxString Yname;
	// used for scaling, so can be smaller/larger than min/max in X/Y
	double Xmin, Xmax, Ymin, Ymax;

    // user defined y-axis range
    wxString def_y_min;
    wxString def_y_max;

    // shapes
	GdaSpline* lowess_reg_line;
    GdaSpline* lowess_reg_upper_line;
    GdaSpline* lowess_reg_lower_line;

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

    // loess
    loess lo;
    prediction pre;
    double loess_span;
    double loess_coverage;
    int loess_degree;
    wxString loess_family;
    std::vector<double> fit_x;
    std::vector<double> fit_y;
    std::vector<double> fit_y_upper;
    std::vector<double> fit_y_lower;
    long int loess_pts_sz;

	DECLARE_EVENT_TABLE()
};

class LoessSettingsDlg : public wxDialog
{
public:
    LoessSettingsDlg(LoessPlotCanvas* canvas);
    virtual ~LoessSettingsDlg();

    void CreateControls();
    bool Init();

    void OnOK( wxCommandEvent& event );
    void OnClose( wxCommandEvent& event );

    //void Setup(loess_struct* lo);
    void Setup(loess* lo);
    
protected:
    wxTextCtrl* m_span;
    wxChoice* m_degree;
    wxChoice* m_family;
    wxCheckBox* m_normalize;
    wxChoice* m_surface;
    wxChoice* m_statistics;
    wxChoice* m_tracehat;
    wxTextCtrl* m_cell;
    wxTextCtrl* m_iterations;

    LoessPlotCanvas* canvas;
};

#endif
