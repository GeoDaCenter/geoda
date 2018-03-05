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

/**
 *  Note: This file is the place to hold all constants such as default colors,
 *  as well as common text strings used in various dialogs.  Everything
 *  should be kept in the GdaConst namespace
 *
 */

#ifndef __GEODA_CENTER_GDA_CONST_H__
#define __GEODA_CENTER_GDA_CONST_H__

#include <stdint.h> 
#include <map>
#include <set>
#include <string>
#include <vector>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/colour.h>
#include <wx/font.h>
#include <wx/pen.h>

class GdaConst {	
public:
	static const int EMPTY = -1;
	
	// This should be called only once in GdaApp::OnInit()
	static void init();
	
	// Types for use in Table
	enum FieldType {
		unknown_type,
		double_type, // N or F with decimals > 0 in DBF
		long64_type, // N or F with decimals = 0 in DBF
		string_type, // C in DBF, max 254 characters
		date_type, // D in DBF, YYYYMMDD format
        time_type, // HH:MM:SS
        datetime_type, // YYYY-MM-DD HH:MM:SS geodatabase
		placeholder_type
	};
	static wxString FieldTypeToStr(GdaConst::FieldType ft);
	
	struct FieldInfo {
		FieldInfo() : type(unknown_type), field_len(0),
			decimals(0) //, displayed_decimals(0)
		{}
		FieldType type;
		wxString name;
		int field_len;
		int decimals;
		//int displayed_decimals;
	};
	
	enum DataSourceType { ds_esri_arc_obj, ds_esri_file_geodb,
		ds_esri_personal_gdb, ds_esri_arc_sde,
		ds_csv, ds_dbf, ds_geo_json, ds_gml, ds_kml,
		ds_mapinfo, ds_mysql, ds_ms_sql, ds_oci, ds_odbc, ds_postgresql,
		ds_shapefile, ds_sqlite, ds_gpkg, ds_wfs, ds_xls, ds_xlsx, ds_osm, ds_ods, ds_cartodb, ds_unknown };
	
	static std::map<std::string, DataSourceType> datasrc_str_to_type;
	static std::map<DataSourceType, std::string> datasrc_type_to_str;
	static std::map<DataSourceType, std::string> datasrc_type_to_prefix;
	static std::map<DataSourceType, std::string> datasrc_type_to_fullname;
	static std::map<DataSourceType, std::set<std::string> > datasrc_req_flds;
	static std::map<DataSourceType, std::set<std::string> > datasrc_opt_flds;
     
	static wxString no_field_warning, db_field_warning, default_field_warning;
    static wxString default_field_name_regex, default_field_name_illegal_regex;
    static wxString db_field_name_regex, db_field_name_illegal_regex;
	static std::map<DataSourceType, int> datasrc_field_lens;
	static std::map<DataSourceType, int> datasrc_table_lens;
	static std::map<DataSourceType, wxString> datasrc_field_regex;
	static std::map<DataSourceType, wxString> datasrc_field_warning;
	static std::map<DataSourceType, wxString> datasrc_field_illegal_regex;
	static std::map<DataSourceType, bool> datasrc_field_casesensitive;
	
	static const int max_dbf_numeric_len = 20; // same for long and double
	static const int max_dbf_long_len = 20;
	static const int min_dbf_long_len = 1;
	static const int default_dbf_long_len = 18;
	static const int max_dbf_double_len = 20;
	static const int min_dbf_double_len = 2; // allow for "0." always
	static const int default_dbf_double_len = 18;
	static const int max_dbf_double_decimals = 15;
	static const int min_dbf_double_decimals = 1;
	static const int default_display_decimals = 4;
	static const int default_dbf_double_decimals = 7;
	static const int max_dbf_string_len = 254;
	static const int min_dbf_string_len = 1;
	static const int default_dbf_string_len = 50;
	static const int max_dbf_date_len = 8;
	static const int min_dbf_date_len = 8;
	static const int default_dbf_date_len = 8;
    
    
	
	// Resource Files
	static const wxString gda_prefs_fname_json;
	static const wxString gda_prefs_fname_sqlite;
	static const wxString gda_prefs_html_table;
	static const wxString gda_prefs_html_table_menu;
	static const wxString gda_prefs_html_table_url;
	
	// Shared menu ids
	static const int ID_TIME_SYNC_VAR1 = wxID_HIGHEST + 1000;
	static const int ID_TIME_SYNC_VAR2 = wxID_HIGHEST + 1001;
	static const int ID_TIME_SYNC_VAR3 = wxID_HIGHEST + 1002;
	static const int ID_TIME_SYNC_VAR4 = wxID_HIGHEST + 1004;
	
	static const int ID_FIX_SCALE_OVER_TIME_VAR1 = wxID_HIGHEST + 2000;
	static const int ID_FIX_SCALE_OVER_TIME_VAR2 = wxID_HIGHEST + 2001;
	static const int ID_FIX_SCALE_OVER_TIME_VAR3 = wxID_HIGHEST + 2002;
	static const int ID_FIX_SCALE_OVER_TIME_VAR4 = wxID_HIGHEST + 2004;
	
	static const int ID_PLOTS_PER_VIEW_1 = wxID_HIGHEST + 3000;
	static const int ID_PLOTS_PER_VIEW_2 = wxID_HIGHEST + 3001;
	static const int ID_PLOTS_PER_VIEW_3 = wxID_HIGHEST + 3002;
	static const int ID_PLOTS_PER_VIEW_4 = wxID_HIGHEST + 3003;
	static const int ID_PLOTS_PER_VIEW_5 = wxID_HIGHEST + 3004;
	static const int ID_PLOTS_PER_VIEW_6 = wxID_HIGHEST + 3005;
	static const int ID_PLOTS_PER_VIEW_7 = wxID_HIGHEST + 3006;
	static const int ID_PLOTS_PER_VIEW_8 = wxID_HIGHEST + 3007;
	static const int ID_PLOTS_PER_VIEW_9 = wxID_HIGHEST + 3008;
	static const int ID_PLOTS_PER_VIEW_10 = wxID_HIGHEST + 3009;
	static const int max_plots_per_view_menu_items = 10;
	static const int ID_PLOTS_PER_VIEW_OTHER = wxID_HIGHEST + 3100;
	static const int ID_PLOTS_PER_VIEW_ALL = wxID_HIGHEST + 3200;
	static const int ID_HISTOGRAM_CLASSIFICATION = wxID_HIGHEST + 3300;
    
	static const int ID_CUSTOM_CAT_CLASSIF_CHOICE_A0 = wxID_HIGHEST + 4000;
	static const int ID_CUSTOM_CAT_CLASSIF_CHOICE_B0 = wxID_HIGHEST + 4100;
	static const int ID_CUSTOM_CAT_CLASSIF_CHOICE_C0 = wxID_HIGHEST + 4200;
	
	static const wxString html_submenu_title;
	static const int ID_HTML_SUBMENU = wxID_HIGHEST + 5000;
	static const int ID_HTML_MENU_ENTRY_CHOICE_0 = ID_HTML_SUBMENU + 1;
	static const int ID_HTML_MENU_ENTRY_CHOICE_1 = ID_HTML_SUBMENU + 2;
	static const int ID_HTML_MENU_ENTRY_CHOICE_2 = ID_HTML_SUBMENU + 3;
	static const int ID_HTML_MENU_ENTRY_CHOICE_3 = ID_HTML_SUBMENU + 4;
	static const int ID_HTML_MENU_ENTRY_CHOICE_4 = ID_HTML_SUBMENU + 5;
	static const int ID_HTML_MENU_ENTRY_CHOICE_5 = ID_HTML_SUBMENU + 6;
	static const int ID_HTML_MENU_ENTRY_CHOICE_6 = ID_HTML_SUBMENU + 7;
	static const int ID_HTML_MENU_ENTRY_CHOICE_7 = ID_HTML_SUBMENU + 8;
	static const int ID_HTML_MENU_ENTRY_CHOICE_8 = ID_HTML_SUBMENU + 9;
	static const int ID_HTML_MENU_ENTRY_CHOICE_9 = ID_HTML_SUBMENU + 10;

    static const int ID_CONNECT_POPUP_MENU = wxID_HIGHEST + 6000;
    
	// Standard wxFont pointers.
	static wxFont* extra_small_font;
	static wxFont* small_font;
	static wxFont* medium_font;
	static wxFont* large_font;
	
	// GdaShape constants
	static const wxPen* default_myshape_pen;
	static const wxBrush* default_myshape_brush;
	
	// GdaPoint radius to give a larger target for clicking on
	static const int my_point_click_radius = 2;
	
	// Shared Colours
	static std::vector<wxColour> qualitative_colors;
    
    static std::vector<wxColour> unique_colors_60;
	
	//background color -- this is light gray
	static const wxColour backColor;
	// background color -- this is light gray
	static const wxColour darkColor;
	// color of text, frames, points -- this is dark cherry
	static const wxColour textColor;
	// outliers color (also used for regression, etc.) -- blue
	static const wxColour outliers_colour;
	// envelope color (also used for regression, etc.) -- red
	static const wxColour envelope_colour;
	
	// Template Canvas shared by Map, Scatterplot, PCP, etc
	static const int default_virtual_screen_marg_left = 20;
	static const int default_virtual_screen_marg_right = 20;
	static const int default_virtual_screen_marg_top = 20;
	static const int default_virtual_screen_marg_bottom = 20;
	static const int shps_min_width = 100;
	static const int shps_min_height = 100;
	static const int shps_max_width = 12000;
	static const int shps_max_height = 12000;
	static const int shps_max_area = 50000000; // 50 million
	
	static const wxColour selectable_outline_color; // black
	static const wxColour selectable_fill_color; // forest green
	static const wxColour highlight_color; // yellow
	static const wxColour canvas_background_color; // white
	static const wxColour legend_background_color; // white

    // Strings
    static const wxString gda_lbl_not_sig;
    static const wxString gda_lbl_undefined;
    static const wxString gda_lbl_neighborless;
    static const wxString gda_lbl_highhigh;
    static const wxString gda_lbl_lowlow;
    static const wxString gda_lbl_lowhigh;
    static const wxString gda_lbl_highlow;
    static const wxString gda_lbl_otherpos;
    static const wxString gda_lbl_negative;
    static const wxString gda_lbl_positive;
    static const wxString gda_lbl_1p;
    static const wxString gda_lbl_1p_10p;
    static const wxString gda_lbl_10p_50p;
    static const wxString gda_lbl_50p_90p;
    static const wxString gda_lbl_90p_99p;
    static const wxString gda_lbl_99p;
    static const wxString gda_lbl_loweroutlier;
    static const wxString gda_lbl_25p;
    static const wxString gda_lbl_25p_50p;
    static const wxString gda_lbl_50p_75p;
    static const wxString gda_lbl_75p;
    static const wxString gda_lbl_upperoutlier;
    static const wxString gda_lbl_n2sigma;
    static const wxString gda_lbl_n2sigma_n1sigma;
    static const wxString gda_lbl_n1sigma;
    static const wxString gda_lbl_1sigma;
    static const wxString gda_lbl_1sigma_2sigma;
    static const wxString gda_lbl_2sigma;
    
	// Preferences
    static double gda_eigen_tol;
    static int gda_cpu_cores;
    static bool gda_set_cpu_cores;
    static wxString gda_user_email;
    static uint64_t gda_user_seed;
    static bool use_gda_user_seed;
    static int gdal_http_timeout;
    static bool enable_high_dpi_support;
    static bool show_csv_configure_in_merge;
    static bool show_recent_sample_connect_ds_dialog;
    static bool use_cross_hatching;
    static int transparency_highlighted;
    static int transparency_unhighlighted;
    static int transparency_map_on_basemap;
    static bool use_basemap_by_default;
    static int default_basemap_selection;
    static bool hide_sys_table_postgres;
    static bool hide_sys_table_sqlite;
    static bool disable_crash_detect;
    static bool disable_auto_upgrade;
    static int plot_transparency_highlighted;
    static int plot_transparency_unhighlighted;
    static std::vector<wxString> gda_datetime_formats;
    static wxString gda_datetime_formats_str;
    static wxString gda_display_datetime_format;
    /*
    static wxString gda_basemap_carto_light_all;
    static wxString gda_basemap_carto_dark_all;
    static wxString gda_basemap_carto_light_nolabels;
    static wxString gda_basemap_carto_dark_nolabels;
    static wxString gda_basemap_nokia_normal_day;
    static wxString gda_basemap_nokia_normal_night;
    static wxString gda_basemap_nokia_hybrid_day;
    static wxString gda_basemap_nokia_satellite_day;
    */
    static int gda_ogr_csv_header;
    
	static const wxSize map_default_size;
	static const int map_default_legend_width;
	// this is a light forest green
	static const wxColour map_default_fill_colour;
	static const wxColour map_default_outline_colour;
	static const int map_default_outline_width = 1;
	static const wxColour map_default_highlight_colour;
	
	// Connectivity Map
	static const wxSize conn_map_default_size;
	static const wxColour conn_map_default_fill_colour;
	static const wxColour conn_map_default_outline_colour;
	static const wxColour conn_map_default_highlight_colour;
    
    static const wxColour conn_graph_outline_colour;
    static const wxColour conn_neighbor_outline_colour;
    static const wxColour conn_neighbor_fill_colour;
	
	// Map Movie
	static const wxColour map_movie_default_fill_colour;
	static const wxColour map_movie_default_highlight_colour;
	
	// Histogram
	static const wxSize hist_default_size;
	
	// Table
	static const wxString placeholder_str;
	static const wxString table_frame_title;
	static const wxSize table_default_size;
	static const wxColour table_no_edit_color;
	static const wxColour table_row_sel_color;
	static const wxColour table_col_sel_color;
	static const wxColour table_row_and_col_sel_color;
		
	// Scatterplot
	static const wxSize scatterplot_default_size;
	static const wxColour scatterplot_scale_color; // black
	static const wxColour scatterplot_regression_color; // purple
	static const wxColour scatterplot_regression_selected_color; // red
	static const wxColour scatterplot_regression_excluded_color; // blue
	static const wxColour scatterplot_origin_axes_color; // grey
	static wxPen* scatterplot_reg_pen;
	static wxPen* scatterplot_reg_selected_pen;
	static wxPen* scatterplot_reg_excluded_pen;
	static wxPen* scatterplot_scale_pen;
	static wxPen* scatterplot_origin_axes_pen;

	// Bubble Chart
	static const wxSize bubble_chart_default_size;
	static const int bubble_chart_default_legend_width;
	
	// 3D Plot
	static const wxColour three_d_plot_default_highlight_colour;
	static const wxColour three_d_plot_default_point_colour;
	static const wxColour three_d_plot_default_background_colour;
	static const wxSize three_d_default_size;
	
	// Boxplot
	static const wxSize boxplot_default_size;
	static const wxColour boxplot_point_color;
	static const wxColour boxplot_median_color;
	static const wxColour boxplot_mean_point_color;
	static const wxColour boxplot_q1q2q3_color;
	
	// PCP (Parallel Coordinate Plot)
	static const wxSize pcp_default_size;
	static const wxColour pcp_line_color;
	static const wxColour pcp_horiz_line_color;
	
	// Averages Chart (Line Chart)
	// Legend:
	//   ln_cht: line chart
	//   clr: color
	//   sel: selected
	//   exl: excluded
	//   tm1: time subset 1
	//   tm2: time subset 2
	//   regimes_hl: regimes highlight
	// Use GenUtils.h: wxString GdaColorUtils::ToHexStr(wxColour)
	//     to produce HTML color string.
	// Use GdaColorUtils::ChangeBrightness to make darker
	static const wxSize line_chart_default_size;
	static wxColour ln_cht_clr_regimes_hl; // yellow
	static wxColour ln_cht_clr_sel_dark; // red
	static wxColour ln_cht_clr_exl_dark; // blue
	static wxColour ln_cht_clr_tm1_dark; // purple
	static wxColour ln_cht_clr_tm2_dark; // brown
	static wxColour ln_cht_clr_sel_light;
	static wxColour ln_cht_clr_exl_light;
	static wxColour ln_cht_clr_tm1_light;
	static wxColour ln_cht_clr_tm2_light;
	
	// Conditional View
	static const wxSize cond_view_default_size;

	// Category Classification
	static const wxSize cat_classif_default_size;

	// Weights Manager Dialog
	static const wxSize weights_man_dlg_default_size;
	
	// Data Change Type Frame
	static const wxSize data_change_type_frame_default_size;
	
	// General Global Constants
	static const int FileNameLen = 512; // max length of file names
	static const int RealWidth = 19;    // default width of output for reals
	static const int ShpHeaderSize = 50; // size of the header record in Shapefile
	static const int ShpObjIdLen = 20;    // length of the ID of shape object

    static wxCursor zoomInCursor;
    static wxCursor zoomOutCursor;

	static const char* raw_zoom_out[65];
	static const char* raw_zoom_in[65];
	static const char* delete_icon_xpm[48];
    
	static const char* sample_names[255];
	static const char* sample_layer_names[255];
	static const char* sample_datasources[255];
	static const char* sample_meta_urls[255];
};

#endif
