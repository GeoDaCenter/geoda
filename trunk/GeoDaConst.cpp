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

#include "GeoDaConst.h"
#include "GeneralWxUtils.h"

wxFont* GeoDaConst::extra_small_font = 0;
wxFont* GeoDaConst::small_font = 0;
wxFont* GeoDaConst::medium_font = 0;
wxFont* GeoDaConst::large_font = 0;

const wxPen* GeoDaConst::default_myshape_pen=0;
const wxBrush* GeoDaConst::default_myshape_brush=0;

// The following are defined in shp2cnt and should be moved from there.
//background color -- this is light gray
const wxColour GeoDaConst::backColor(192, 192, 192);
// background color -- this is light gray
const wxColour GeoDaConst::darkColor(20, 20, 20);
// color of text, frames, points -- this is dark cherry
const wxColour GeoDaConst::textColor(128, 0, 64);
// outliers color (also used for regression, etc.) -- blue
const wxColour GeoDaConst::outliers_colour(0, 0, 255);
// envelope color (also used for regression, etc.) -- blue
const wxColour GeoDaConst::envelope_colour(0, 0, 255);

const wxColour GeoDaConst::selectable_outline_color(0, 0, 0); // black
const wxColour GeoDaConst::selectable_fill_color(49, 163, 84); // forest green
const wxColour GeoDaConst::highlight_color(255, 255, 0); // yellow
const wxColour GeoDaConst::canvas_background_color(255, 255, 255); // white
const wxColour GeoDaConst::legend_background_color(255, 255, 255); // white

// Map
const wxSize GeoDaConst::map_default_size(550, 300);
const int GeoDaConst::map_default_legend_width = 150;
// this is a light forest green
const wxColour GeoDaConst::map_default_fill_colour(49, 163, 84);
const wxColour GeoDaConst::map_default_outline_colour(0, 0, 0);
const wxColour GeoDaConst::map_default_highlight_colour(255, 255, 0); // yellow

// Map Movie
const wxColour GeoDaConst::map_movie_default_fill_colour(49, 163, 84);
const wxColour GeoDaConst::map_movie_default_highlight_colour(224, 113, 182);

// Histogram
const wxSize GeoDaConst::hist_default_size(500, 300);

// Table
const wxString GeoDaConst::table_frame_title("Table");
const wxSize GeoDaConst::table_default_size(750, 500);
const wxColour GeoDaConst::table_no_edit_color(80, 80, 80); // grey

// Scatterplot
const wxSize GeoDaConst::scatterplot_default_size(400, 400);
const wxColour GeoDaConst::scatterplot_scale_color(0, 0, 0);
//const wxColour GeoDaConst::scatterplot_regression_color(0, 79, 241); 
//const wxColour GeoDaConst::scatterplot_regression_selected_color(204, 41, 44); 
//const wxColour GeoDaConst::scatterplot_regression_excluded_color(0, 146, 31);
const wxColour GeoDaConst::scatterplot_regression_color(100, 0, 110); 
const wxColour GeoDaConst::scatterplot_regression_selected_color(204, 41, 44); 
const wxColour GeoDaConst::scatterplot_regression_excluded_color(0, 79, 241);
const wxColour GeoDaConst::scatterplot_origin_axes_color(120, 120, 120);
wxPen* GeoDaConst::scatterplot_reg_pen = 0;
wxPen* GeoDaConst::scatterplot_reg_selected_pen = 0;
wxPen* GeoDaConst::scatterplot_reg_excluded_pen = 0;
wxPen* GeoDaConst::scatterplot_scale_pen = 0;
wxPen* GeoDaConst::scatterplot_origin_axes_pen = 0;

// Bubble Chart
const wxSize GeoDaConst::bubble_chart_default_size(550, 400);
const int GeoDaConst::bubble_chart_default_legend_width = 150;

// 3D Plot
// yellow
const wxColour GeoDaConst::three_d_plot_default_highlight_colour(255, 255, 0);
// white
const wxColour GeoDaConst::three_d_plot_default_point_colour(255, 255, 255);
// black
const wxColour GeoDaConst::three_d_plot_default_background_colour(0, 0, 0);
const wxSize GeoDaConst::three_d_default_size(700, 500);

// Boxplot
const wxSize GeoDaConst::boxplot_default_size(300, 500);
const wxColour GeoDaConst::boxplot_point_color(0, 0, 255);
const wxColour GeoDaConst::boxplot_median_color(219, 99, 28); // orange
const wxColour GeoDaConst::boxplot_mean_point_color(20, 200, 20); // green
const wxColour GeoDaConst::boxplot_q1q2q3_color(128, 0, 64); // dark cherry

// PCP (Parallel Coordinate Plot)
const wxSize GeoDaConst::pcp_default_size(500, 300);
const wxColour GeoDaConst::pcp_line_color(128, 0, 64); // dark cherry
const wxColour GeoDaConst::pcp_horiz_line_color(0, 98, 0); // dark green

// Conditional View
const wxSize GeoDaConst::cond_view_default_size(700, 500);

// Category Classification
const wxSize GeoDaConst::cat_classif_default_size(780, 520);

std::vector<wxColour> GeoDaConst::qualitative_colors(10);

/**
 Certain objects such as wxFont objects need to be created after
 wxWidgets is sufficiently initialized.  This function will be
 called just once by MyApp::OnInit() when the program begins.
 */
void GeoDaConst::init()
{
	// standard GeoDa font creation.  Through experimentation, as of the
	// wxWidgets 2.9.1 release, it appears that neither wxFont::SetPixelSize()
	// nor wxFont::SetPointSize() results in fonts of nearly similar vertical
	// height on all of our three supported platforms, Mac, Linux and Windows.
	// Therefore, at present we will specify sizes differently on each
	// platform according to experimentation.  When we specify the font
	// using point size, it seems that Linux and Windows are very similar, but
	// Mac is considerably smaller.
	int ref_extra_small_pt_sz = 6;
	int ref_small_pt_sz = 8;
	int ref_medium_pt_sz = 12;
	int ref_large_pt_sz = 16;
	
	if (GeneralWxUtils::isMac()) {
		ref_extra_small_pt_sz += 4;
		ref_small_pt_sz += 4;
		ref_medium_pt_sz += 5;
		ref_large_pt_sz += 5;
	}
	
	extra_small_font = wxFont::New(ref_extra_small_pt_sz,
								   wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
								   wxFONTWEIGHT_NORMAL, false, wxEmptyString,
								   wxFONTENCODING_DEFAULT);
	small_font = wxFont::New(ref_small_pt_sz, wxFONTFAMILY_SWISS,
							 wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
							 wxEmptyString, wxFONTENCODING_DEFAULT);
	medium_font = wxFont::New(ref_medium_pt_sz, wxFONTFAMILY_SWISS,
							  wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
							  wxEmptyString, wxFONTENCODING_DEFAULT);
	large_font = wxFont::New(ref_large_pt_sz, wxFONTFAMILY_SWISS,
							 wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
							 wxEmptyString, wxFONTENCODING_DEFAULT);
		
	// MyShape resources
	default_myshape_pen = wxBLACK_PEN;
	default_myshape_brush = wxTRANSPARENT_BRUSH;
	
	//ScatterPlot and ScatterNewPlot resources
	scatterplot_reg_pen = new wxPen(scatterplot_regression_color, 2);
	scatterplot_reg_selected_pen =
	new wxPen(scatterplot_regression_selected_color, 2);
	scatterplot_reg_excluded_pen =
	new wxPen(scatterplot_regression_excluded_color, 2);
	scatterplot_scale_pen =	new wxPen(scatterplot_scale_color);
	scatterplot_origin_axes_pen =
	new wxPen(scatterplot_origin_axes_color, 1, wxSHORT_DASH);
	
	// From Colorbrewer 2.0
	qualitative_colors[0] = wxColour(166, 206, 227);
	qualitative_colors[1] = wxColour(31, 120, 180);
	qualitative_colors[2] = wxColour(178, 223, 138);
	qualitative_colors[3] = wxColour(51, 160, 44);
	qualitative_colors[4] = wxColour(251, 154, 153);
	qualitative_colors[5] = wxColour(227, 26, 28);
	qualitative_colors[6] = wxColour(253, 191, 111);
	qualitative_colors[7] = wxColour(255, 127, 0);
	qualitative_colors[8] = wxColour(202, 178, 214);
	qualitative_colors[9] = wxColour(106, 61, 154);
}
