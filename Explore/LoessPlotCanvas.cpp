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
#include "LoessPlotCanvas.h"


IMPLEMENT_CLASS(LoessPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(LoessPlotCanvas, TemplateCanvas)
EVT_PAINT(TemplateCanvas::OnPaint)
EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

LoessPlotCanvas::
LoessPlotCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project,
                HLStateInt* hl_state_int,
                const std::vector<double>& X, const std::vector<double>& Y,
                const std::vector<bool>& X_undef, const std::vector<bool>& Y_undef,
                const wxString& Xname, const wxString& Yname, int style,
                const wxString& right_click_menu_id, const wxString& title,
                const wxPoint& pos, const wxSize& size)
:TemplateCanvas(parent, t_frame, project, hl_state_int, pos, size, false, true),
X(X), Y(Y), X_undef(X_undef), Y_undef(Y_undef),
orgX(X), orgY(Y), Xname(Xname), Yname(Yname),
right_click_menu_id(right_click_menu_id), style(style)
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
        virtual_screen_marg_bottom = 45;//45;
        virtual_screen_marg_left = 45;//45;
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

    // run loess
    long p = 1; // p    number of variables/predictors.
    W.resize(n_pts, 1);
    loess_setup((double*)&X.at(0), (double*)&Y.at(0), &W.at(0), (long)X.size(), p, &lo);
    //lo.model.span = 0.75;
    //lo.model.family = "gaussian";
    //lo.model.degree = 2;
    RunLoess();

	//PopulateCanvas();
	//ResizeSelectableShps();
	
	highlight_state->registerObserver(this);
}

LoessPlotCanvas::~LoessPlotCanvas()
{
	wxLogMessage("Entering LoessPlotCanvas::~LoessPlotCanvas");
    if (highlight_state) highlight_state->removeObserver(this);
    loess_free_mem(&lo);
    pred_free_mem(&pre);
	wxLogMessage("Exiting LoessPlotCanvas::~LoessPlotCanvas");
}

void LoessPlotCanvas::RunLoess()
{
    loess_fit(&lo);
    loess_summary(&lo);

    loess_pts_sz = 20;

    fit_x.clear();
    fit_x.resize(loess_pts_sz);
    double x_itv = (Xmax - Xmin) / loess_pts_sz;
    for (size_t i=0; i<loess_pts_sz; ++i) {
        fit_x[i] = Xmin + x_itv * i;
    }
    pre.m = loess_pts_sz;
    pre.se = 0; // 1 will crash the program for some dataset
    predict(&fit_x.at(0), &lo,  &pre);

    //double coverage = 0.99;
    //confidence_intervals ci;
    //pointwise(&pre, coverage, &ci);

    fit_y.clear();
    fit_y.resize(loess_pts_sz);
    //fit_y_upper.resize(loess_pts_sz);
    //fit_y_lower.resize(loess_pts_sz);
    for (size_t i=0; i<loess_pts_sz; ++i) {
        fit_y[i] = pre.fit[i];
        //fit_y_upper[i] = ci.upper[i];
        //fit_y_lower[i] = ci.lower[i];
    }
    
    PopulateCanvas();
}

void LoessPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	wxLogMessage("Entering LoessPlotCanvas::DisplayRightClickMenu");
	if (right_click_menu_id.IsEmpty())
        return;
    //wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_SCATTER_PLOT_MAT_MENU_OPTIONS");
    //template_frame->UpdateContextMenuItems(optMenu);
    //template_frame->PopupMenu(optMenu, pos + GetPosition());
    //template_frame->UpdateOptionMenuItems();
	wxLogMessage("Exiting LoessPlotCanvas::DisplayRightClickMenu");
}

void LoessPlotCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
}

/**
 Override of TemplateCanvas method.  We must still call the
 TemplateCanvas method after we update the regression lines
 as needed. */
void LoessPlotCanvas::update(HLStateInt* o)
{
}

void LoessPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
}

wxString LoessPlotCanvas::GetCanvasTitle()
{
    wxString s = _("Scatter Plot- x: %s, y: %s");
    s = wxString::Format(s, Xname, Yname);
	return s;
}

wxString LoessPlotCanvas::GetVariableNames()
{
    wxString s;
    s << Xname << ", " << Yname;
    return s;
}

void LoessPlotCanvas::UpdateStatusBar()
{
	if (template_frame) {
		wxStatusBar* sb = template_frame->GetStatusBar();
        if (sb == NULL) return;
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
                s << _("#selected=") << n_total_hl << "  ";
                
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
                    s << _("#hover obs ") << hover_obs[0]+1 << " = (";
                    s << X[hover_obs[0]] << ", " << Y[hover_obs[0]];
                    s << ")";
                }
                if (total_hover_obs >= 2) {
                    s << ", ";
                    s << _("obs ") << hover_obs[1]+1 << " = (";
                    s << X[hover_obs[1]] << ", " << Y[hover_obs[1]];
                    s << ")";
                }
                if (total_hover_obs >= 3) {
                    s << ", ";
                    s << _("obs ") << hover_obs[2]+1 << " = (";
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
}

void LoessPlotCanvas::TimeSyncVariableToggle(int var_index)
{
}

void LoessPlotCanvas::FixedScaleVariableToggle(int var_index)
{
}

void LoessPlotCanvas::SetSelectableOutlineColor(wxColour color)
{
    selectable_outline_color = color;
    TemplateCanvas::SetSelectableOutlineColor(color);
    PopulateCanvas();
}

void LoessPlotCanvas::SetHighlightColor(wxColour color)
{
    highlight_color = color;
    PopulateCanvas();
}

void LoessPlotCanvas::SetSelectableFillColor(wxColour color)
{
    // In Scatter Plot, Fill color is for points
    selectable_fill_color = color;
	cat_data.SetCategoryPenColor(0, 0, selectable_fill_color);
    TemplateCanvas::SetSelectableFillColor(color);
    PopulateCanvas();
}

void LoessPlotCanvas::ShowAxes(bool display)
{
    if (display) style = style | show_axes;
	UpdateMargins();
	PopulateCanvas();
}


void LoessPlotCanvas::OnEditLoess(wxCommandEvent& evt)
{
    LoessSettingsDlg set_dlg(this);
    set_dlg.ShowModal();
}

void LoessPlotCanvas::PopulateCanvas()
{
	LOG_MSG("Entering LoessPlotCanvas::PopulateCanvas");
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
	
	// create lowess regression lines
	if ((style & show_lowess_smoother) && n_pts > 1) {
        lowess_reg_line = new GdaSpline;
        foreground_shps.push_back(lowess_reg_line);
        lowess_reg_line->reInit(fit_x, fit_y, axis_scale_x.scale_min,
                                axis_scale_y.scale_min, scaleX, scaleY);
        lowess_reg_line->setPen(wxPen(selectable_outline_color, 2));

        if ((style & show_confidence_interval) ) {
            lowess_reg_upper_line = new GdaSpline;

            wxPen* loess_confidence_pen = new wxPen(GdaConst::scatterplot_origin_axes_color, 1, wxSHORT_DASH);
            foreground_shps.push_back(lowess_reg_upper_line);
            lowess_reg_upper_line->reInit(fit_x, fit_y_upper, axis_scale_x.scale_min,
                                          axis_scale_y.scale_min, scaleX, scaleY);
            lowess_reg_upper_line->setPen(*loess_confidence_pen);

            lowess_reg_lower_line = new GdaSpline;
            foreground_shps.push_back(lowess_reg_lower_line);
            lowess_reg_lower_line->reInit(fit_x, fit_y_lower, axis_scale_x.scale_min,
                                          axis_scale_y.scale_min, scaleX, scaleY);
            lowess_reg_lower_line->setPen(*loess_confidence_pen);
        }
    }



	ResizeSelectableShps();
	
	LOG_MSG("Exiting LoessPlotCanvas::PopulateCanvas");
}

void LoessPlotCanvas::UpdateMargins()
{
	int virtual_screen_marg_top = 20;//20;
	int virtual_screen_marg_right = 5;//20;
	int virtual_screen_marg_bottom = 5;//45;
	int virtual_screen_marg_left = 5;//45;
    
	if (style & show_axes) {
		virtual_screen_marg_bottom = 45;//45;
		virtual_screen_marg_left = 45;//45;
	}
    last_scale_trans.SetMargin(virtual_screen_marg_top,
                               virtual_screen_marg_bottom,
                               virtual_screen_marg_left,
                               virtual_screen_marg_right);
}

////////////////////////////////////////////////////////////////////////////////

LoessSettingsDlg::LoessSettingsDlg(LoessPlotCanvas* canvas)
: wxDialog(NULL, wxID_ANY, _("Loess Settings"), wxDefaultPosition,
           wxSize(420, 450), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
canvas(canvas)
{
    CreateControls();
}
LoessSettingsDlg::~LoessSettingsDlg()
{

}

void LoessSettingsDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,5,0);

    // span
    wxStaticText* st_span = new wxStaticText(panel, wxID_ANY, _("Degree of smoothing (span):"));
    m_span = new wxTextCtrl(panel, wxID_ANY, "0.75", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st_span, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_span, 1, wxEXPAND);

    // span
    wxStaticText* st_degree = new wxStaticText(panel, wxID_ANY, _("Degree of the polynomials:"));
    wxString choices13[] = {"1", "2"};
    m_degree = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_degree->SetSelection(1);
    gbox->Add(st_degree, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_degree, 1, wxEXPAND);

    // span
    wxStaticText* st_family = new wxStaticText(panel, wxID_ANY, _("Family:"));
    wxString choices14[] = {"gaussian", "symmetric"};
    m_family = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices14);
    m_family->SetSelection(0);
    gbox->Add(st_family, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_family, 1, wxEXPAND);

    // normalize
    wxStaticText* st_norm = new wxStaticText(panel, wxID_ANY, _("Normalize predictors:"));
    m_normalize = new wxCheckBox(panel, wxID_ANY, "");
    m_normalize->SetValue(true);
    gbox->Add(st_norm, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_normalize, 1, wxEXPAND);

    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Loess Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);


    // control Parameters
    wxFlexGridSizer* gbox1 = new wxFlexGridSizer(9,2,5,0);

    // surface  interpolatedirect
    wxStaticText* st_surface = new wxStaticText(panel, wxID_ANY, _("Surface:"));
    wxString choices15[] = {"interpolate", "direct"};
    m_surface = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices15);
    m_surface->SetSelection(0);
    gbox1->Add(st_surface, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(m_surface, 1, wxEXPAND);

    // statistics = c("approximate", "exact", "none"),
    wxStaticText* st_statistics = new wxStaticText(panel, wxID_ANY, _("Statistics:"));
    wxString choices16[] = {"approximate", "exact", "none"};
    m_statistics = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 3, choices16);
    m_statistics->SetSelection(0);
    gbox1->Add(st_statistics, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(m_statistics, 1, wxEXPAND);

    // trace.hat = c("exact", "approximate"),
    wxStaticText* st_tracehat = new wxStaticText(panel, wxID_ANY, _("Trace Hat:"));
    wxString choices_tracehat[] = {"approximate", "exact"};
    m_tracehat = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices_tracehat);
    m_tracehat->SetSelection(0);
    gbox1->Add(st_tracehat, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(m_tracehat, 1, wxEXPAND);

    // cell 0.2
    wxStaticText* st_cell = new wxStaticText(panel, wxID_ANY, _("Cell:"));
    m_cell = new wxTextCtrl(panel, wxID_ANY, "0.2", wxDefaultPosition, wxSize(200,-1));
    gbox1->Add(st_cell, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(m_cell, 1, wxEXPAND);

    // iterations 4
    wxStaticText* st_iteration = new wxStaticText(panel, wxID_ANY, _("Iterations:"));
    m_iterations = new wxTextCtrl(panel, wxID_ANY, "4", wxDefaultPosition, wxSize(200,-1));
    gbox1->Add(st_iteration, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(m_iterations, 1, wxEXPAND);

    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Loess Control Parameters:"));
    hbox1->Add(gbox1, 1, wxEXPAND);

    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Apply"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);

    vbox->Add(hbox, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox1, 1, wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 1, wxALIGN_CENTER | wxALL, 10);


    panel->SetSizer(vbox);

    Centre();

    // Content
    //InitVariableCombobox(box);
    //InitWeightsCombobox(box3);

    //combo_var = box;
    //combo_weights = box3;

    // Events
    okButton->Bind(wxEVT_BUTTON, &LoessSettingsDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &LoessSettingsDlg::OnClose, this);
    
    Setup(canvas->GetLoess());
}


void LoessSettingsDlg::OnClose(wxCommandEvent& event )
{
    wxLogMessage("In MultiVariableSettingsDlg::OnClose");

    event.Skip();
    EndDialog(wxID_CANCEL);
}

void LoessSettingsDlg::Setup(loess* lo)
{
    wxString str_span;
    str_span << lo->model->span;
    m_span->SetValue(str_span);
    
    if(lo->model->degree == 1)
        m_degree->SetSelection(0);
    else
        m_degree->SetSelection(1);

    if (strcmp(lo->model->family, "gaussian") ==0)
        m_family->SetSelection(0);
    else
        m_family->SetSelection(1);

    if (lo->model->normalize == 1)
        m_normalize->SetValue(true);
    else
        m_normalize->SetValue(false);
    
    wxString str_cell;
    str_cell <<  lo->control->cell;
    m_cell->SetValue(str_cell);
    
    wxString str_iter;
    str_iter << lo->control->iterations;
    m_iterations->SetValue(str_iter);

    // "approximate", "exact", "none"
    if (strcmp(lo->control->statistics, "approximate") ==0)
        m_statistics->SetSelection(0);
    else if (strcmp(lo->control->statistics, "exact") ==0)
        m_statistics->SetSelection(1);
    else
        m_statistics->SetSelection(2);

    // surface  interpolate direct
    if (strcmp(lo->control->surface, "interpolate") ==0)
        m_surface->SetSelection(0);
    else
        m_surface->SetSelection(1);

    // "exact", "approximate"
    if (strcmp(lo->control->trace_hat, "approximate") ==0)
        m_tracehat->SetSelection(0);
    else
        m_tracehat->SetSelection(1);
}

void LoessSettingsDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Entering MultiVariableSettingsDlg::OnOK");
    if (canvas) {
        bool success = false;
        double v_span;
        success =  m_span->GetValue().ToDouble(&v_span);
        if (!success || v_span <= 0 || v_span > 1) {
            wxMessageDialog dlg(this, _("Please input a valid numeric value for span."), _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        double cell;
        success = m_cell->GetValue().ToDouble(&cell);
        if (!success) {
            wxMessageDialog dlg(this, _("Please input a valid numeric value for cell."), _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        long iterations;
        success = m_iterations->GetValue().ToLong(&iterations);
        if (!success || iterations < 2) {
            wxMessageDialog dlg(this, _("Please input a valid numeric value for iterations."), _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }

        long degree = m_degree->GetSelection() + 1;
        const char* family = m_family->GetStringSelection().c_str();
        bool normalize = m_normalize->GetValue();
        const char* surface = m_surface->GetStringSelection().c_str();
        const char* statistics = m_statistics->GetStringSelection().c_str();
        const char* trace_hat = m_tracehat->GetStringSelection().c_str();

        
        loess* lo = canvas->GetLoess();
        lo->model->span = v_span;
        lo->model->degree = degree;

        if (m_family->GetSelection()==0) lo->model->family = "gaussian";
        else lo->model->family ="symmetric";

        lo->model->normalize = (long)normalize;
        lo->control->cell = cell;
        lo->control->iterations = iterations;

        // "approximate", "exact", "none"
        if (m_statistics->GetSelection() == 0) lo->control->statistics = "approximate";
        else if (m_statistics->GetSelection() == 1) lo->control->statistics = "exact";
        else lo->control->statistics = "none";

        // surface  interpolate direct
        if (m_surface->GetSelection() == 0) lo->control->surface ="interpolate";
        else lo->control->surface = "direct";

        // "exact", "approximate"
        if (m_tracehat->GetSelection() == 0) lo->control->trace_hat = "approximate";
        else lo->control->trace_hat = "exact";
         
        canvas->RunLoess();
    }
}
