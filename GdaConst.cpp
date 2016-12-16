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

#include "GdaConst.h"
#include "GeneralWxUtils.h"
#include "GenUtils.h"
#include <wx/mstream.h>

const char* GdaConst::sample_names[] = {
    "NATREGIMES US County Homicides",
    "Baltimore: house sales prices and hedonics",
    "Baltimore: house sales prices and trend surface residuals",
    "Buenosaires: Electoral Data for 1999 Argentinean Elections",
    "Columbus: Columbus neighborhood crime",
    "SIDS2: North Carolina county SIDS death counts and rates",
    "Nepal: Health, poverty and education indicators",
    "NYC: Demographic, housing, and public assistance data 02-09",
    "Milwaukee1: WI 2000 Census Tract Data",
    "Malaria in Colombia: departments-incidence and population",
    "Malaria in Colombia: municipalities-incidence and population",
    "StLouis: St Louis Region Homicides counts and rates",
    
    "Atlanta, GA region homicide counts and rates",
    "Bostonhsg: housing and neighborhood",
    "Charleston1: SC MSA 2000 Census Tract Data",
    "Charleston1: SC counties 2000 Census Tract Data",
    "Charleston2: 1998 and 2001 Zip Code Business Patterns",
    "Grid100: Grid with simulated variables",
    "Hickory1: NC MSA 2000 Census Tract Data for Hickory",
    "Hickory1: NC counties 2000 Census Tract Data for Hickory",
    "Hickory2: 1998 and 2001 Zip Code Business Patterns ",
    "Houston: region homicide counts and rates",
    "Juvenile: Cardiff juvenile delinquent residences",
    "Lansing1: MI counties 2000 Census Tract Data",
    "Lansing1: MI MSA 2000 Census Tract Data",
    "Lansing2: 1998 and 2001 Zip Code Business Patterns",
    "Laozone: Ozone measures at monitoring stations in LA basin",
    "LasRosas1999: Corn yield, fertilizer and field data",
    "LasRosas2001: Corn yield, fertilizer and field data",
    "Milwaukee2: 1998 and 2001 Zip Code Business Patterns",
    "NCOVR: US county homicides 1960-1990",
    "NDVI: Normalized Difference Vegetation Index grid",
    "Ohiolung: UTM zone lung cancer data, 1968, 1978, 1988",
    "Ohiolung: OH counties lung cancer data, 1968, 1978, 1988",
    "Orlando1: Counties 2000 Census Tract Data",
    "Orlando1: Final Census 2000 Census Tract Data",
    "Orlando2: MSA 1998 and 2001 Zip Code Business Patterns",
    "Oz9799: Monthly ozone data, 1997-99",
    "Phoenix ACS: Phoenix American Community Survey Data 2010",
    "Pittsburgh: 1993 homicide locations",
    "Pittsburgh: 1994 homicide locations",
    "Pittsburgh: 1993 1994 homicide locations",
    "Police: Police expenditures Mississippi counties",
    "Sacramento1: 2000 Census Tract Data",
    "Sacramento2: 1998 and 2001 Zip Code Business Patterns",
    "SanFran Crime: July-Dec 2012 crime incidents",
    "Savannah1: GA MSA 2000 Census Tract Data for Savannah",
    "Savannah1: Counties 2000 Census Tract Data for Savannah",
    "Savannah2: 1998 and 2001 Zip Code Business Patterns",
    "Scotlip: Scottish districts",
    "Scotlip: Male lip cancer in Scotland, 1975-80",
    "Seattle1: WA MSA 2000 Census Tract Data",
    "Seattle1: WA counties 2000 Census Tract Data",
    "Seattle2: 1998 and 2001 Zip Code Business Patterns",
    "SIDS: North Carolina county SIDS death counts",
    "South: US Southern county homicides 1960-1990",
    "Tampa1: FL counties 2000 Census Tract Data",
    "Tampa1: FL MSA 2000 Census Tract Data"
};

const char* GdaConst::sample_layer_names[] = {
    "natregimes",
    "baltim",
    "baltimore",
    "buenosaires",
    "columbus",
    "sids2",
    "nepal",
    "nyc",
    "wi_final_census2_random4",
    "coldept",
    "colmunic",
    "stl_hom",
    
    "atl_hom",
    "boston",
    "sc_msa_counties",
    "sc_final_census2",
    "CharlestonMSA2",
    "grid100",
    "nc_final_census2",
    "nc_msa_counties",
    "HickoryMSA2",
    "hou_hom",
    "juvenile",
    "mi_msa_counties",
    "mi_final_census2",
    "LansingMSA2",
    "oz96_utm",
    "rosas1999",
    "rosas2001",
    "MilwaukeeMSA2",
    "NAT",
    "ndvigrid",
    "ohlung",
    "ohutm",
    "orlando_counties",
    "orlando_final_census2",
    "OrlandoMSA2",
    "oz9799",
    "phx",
    "pitt93",
    "pitt94",
    "pitthom",
    "police",
    "sacramentot2",
    "SacramentoMSA2",
    "sfpd_plots",
    "ga_final_census2",
    "ga_msa_counties",
    "SavannahMSA2",
    "scotdistricts",
    "scotlip",
    "wa_final_census2",
    "wa_msa_counties",
    "SeattleMSA2",
    "sids",
    "south",
    "tampa_counties",
    "tampa_final_census2"
};

const char* GdaConst::sample_datasources[] = {
    "samples.sqlite", "samples.sqlite", "samples.sqlite", "samples.sqlite",
    "samples.sqlite", "samples.sqlite", "samples.sqlite", "samples.sqlite",
    "samples.sqlite", "samples.sqlite", "samples.sqlite", "samples.sqlite",

    "http://geodacenter.github.io/sample_data/atl_hom.geojson",
    "http://geodacenter.github.io/sample_data/boston.geojson",
    "http://geodacenter.github.io/sample_data/sc_msa_counties.geojson",
    "http://geodacenter.github.io/sample_data/sc_final_census2.geojson",
    "http://geodacenter.github.io/sample_data/CharlestonMSA2.geojson",
    "http://geodacenter.github.io/sample_data/grid100.geojson",
    "http://geodacenter.github.io/sample_data/nc_final_census2.geojson",
    "http://geodacenter.github.io/sample_data/nc_msa_counties.geojson",
    "http://geodacenter.github.io/sample_data/HickoryMSA2.geojson",
    "http://geodacenter.github.io/sample_data/hou_hom.geojson",
    "http://geodacenter.github.io/sample_data/juvenile.geojson",
    "http://geodacenter.github.io/sample_data/mi_msa_counties.geojson",
    "http://geodacenter.github.io/sample_data/mi_final_census2.geojson",
    "http://geodacenter.github.io/sample_data/LansingMSA2.geojson",
    "http://geodacenter.github.io/sample_data/oz96_utm.geojson",
    "http://geodacenter.github.io/sample_data/rosas1999.geojson",
    "http://geodacenter.github.io/sample_data/rosas2001.geojson",
    "http://geodacenter.github.io/sample_data/MilwaukeeMSA2.geojson",
    "http://geodacenter.github.io/sample_data/NAT.geojson",
    "http://geodacenter.github.io/sample_data/ndvigrid.geojson",
    "http://geodacenter.github.io/sample_data/ohlung.geojson",
    "http://geodacenter.github.io/sample_data/ohutm.geojson",
    "http://geodacenter.github.io/sample_data/orlando_counties.geojson",
    "http://geodacenter.github.io/sample_data/orlando_final_census2.geojson",
    "http://geodacenter.github.io/sample_data/OrlandoMSA2.geojson",
    "http://geodacenter.github.io/sample_data/oz9799.geojson"
    "http://geodacenter.github.io/sample_data/phx.geojson",
    "http://geodacenter.github.io/sample_data/pitt93.geojson",
    "http://geodacenter.github.io/sample_data/pitt94.geojson",
    "http://geodacenter.github.io/sample_data/pitthom.geojson",
    "http://geodacenter.github.io/sample_data/police.geojson",
    "http://geodacenter.github.io/sample_data/sacramentot2.geojson",
    "http://geodacenter.github.io/sample_data/SacramentoMSA2.geojson",
    "http://geodacenter.github.io/sample_data/sfpd_plots.geojson",
    "http://geodacenter.github.io/sample_data/ga_final_census2.geojson",
    "http://geodacenter.github.io/sample_data/ga_msa_counties.geojson",
    "http://geodacenter.github.io/sample_data/SavannahMSA2.geojson",
    "http://geodacenter.github.io/sample_data/scotdistricts.geojson",
    "http://geodacenter.github.io/sample_data/scotlip.geojson",
    "http://geodacenter.github.io/sample_data/wa_final_census2.geojson",
    "http://geodacenter.github.io/sample_data/wa_msa_counties.geojson",
    "http://geodacenter.github.io/sample_data/SeattleMSA2.geojson",
    "http://geodacenter.github.io/sample_data/sids.geojson",
    "http://geodacenter.github.io/sample_data/south.geojson",
    "http://geodacenter.github.io/sample_data/tampa_counties.geojson",
    "http://geodacenter.github.io/sample_data/tampa_final_census2.geojson"
};

const char* GdaConst::raw_zoom_in[] = {

	"16 16 48 1",
	" 	g None",
	".	g #979797",
	"+	g #787878",
	"@	g #686868",
	"#	g #717171",
	"$	g #8A8A8A",
	"%	g #909090",
	"&	g #D6D6D6",
	"*	g #EFEFEF",
	"=	g #DEDEDE",
	"-	g #A3A3A3",
	";	g #6D6D6D",
	">	g #9F9F9F",
	",	g #AAAAAA",
	"'	g #F8F8F8",
	")	g #C4C4C4",
	"!	g #929292",
	"~	g #F5F5F5",
	"{	g #F2F2F2",
	"]	g #444444",
	"^	g #EEEEEE",
	"/	g #F6F6F6",
	"(	g #8E8E8E",
	"_	g #8F8F8F",
	":	g #D5D5D5",
	"<	g #E8E8E8",
	"[	g #E1E1E1",
	"}	g #F3F3F3",
	"|	g #D0D0D0",
	"1	g #707070",
	"2	g #EDEDED",
	"3	g #DCDCDC",
	"4	g #676767",
	"5	g #D8D8D8",
	"6	g #C0C0C0",
	"7	g #777777",
	"8	g #A1A1A1",
	"9	g #EBEBEB",
	"0	g #828282",
	"a	g #6B6B6B",
	"b	g #B2B2B2",
	"c	g #696969",
	"d	g #949494",
	"e	g #CACACA",
	"f	g #BEBEBE",
	"g	g #848484",
	"h	g #747474",
	"i	g #666666",
	"                ",
	"    .+@#$       ",
	"   #%&*=-;>     ",
	"  #,''''')@     ",
	" .!~'{]^{/(_    ",
	" +:'{<][<}|1    ",
	" @2']]]]]^34    ",
	" 15'^[]:[^67    ",
	" _8'{<][<90.    ",
	"  ab'}^^},;     ",
	"  >cde3fg#hii   ",
	"    _#@+.  iii  ",
	"            iii ",
	"             ii ",
	"                ",
	"                "
};

const char* GdaConst::raw_zoom_out[] = {
	"16 16 48 1",
	" 	g None",
	".	g #979797",
	"+	g #787878",
	"@	g #686868",
	"#	g #717171",
	"$	g #8A8A8A",
	"%	g #909090",
	"&	g #D6D6D6",
	"*	g #EFEFEF",
	"=	g #DEDEDE",
	"-	g #A3A3A3",
	";	g #6D6D6D",
	">	g #9F9F9F",
	",	g #AAAAAA",
	"'	g #F8F8F8",
	")	g #C4C4C4",
	"!	g #929292",
	"~	g #F5F5F5",
	"{	g #F2F2F2",
	"]	g #EDEDED",
	"^	g #EEEEEE",
	"/	g #F6F6F6",
	"(	g #8E8E8E",
	"_	g #8F8F8F",
	":	g #D5D5D5",
	"<	g #E8E8E8",
	"[	g #E1E1E1",
	"}	g #F3F3F3",
	"|	g #D0D0D0",
	"1	g #707070",
	"2	g #444444",
	"3	g #DCDCDC",
	"4	g #676767",
	"5	g #D8D8D8",
	"6	g #C0C0C0",
	"7	g #777777",
	"8	g #A1A1A1",
	"9	g #EBEBEB",
	"0	g #828282",
	"a	g #6B6B6B",
	"b	g #B2B2B2",
	"c	g #696969",
	"d	g #949494",
	"e	g #CACACA",
	"f	g #BEBEBE",
	"g	g #848484",
	"h	g #747474",
	"i	g #666666",
	"                ",
	"    .+@#$       ",
	"   #%&*=-;>     ",
	"  #,''''')@     ",
	" .!~'{]^{/(_    ",
	" +:'{<[[<}|1    ",
	" @]'22222^34    ",
	" 15'^[::[^67    ",
	" _8'{<[[<90.    ",
	"  ab'}^^},;     ",
	"  >cde3fg#hii   ",
	"    _#@+.  iii  ",
	"            iii ",
	"             ii ",
	"                ",
	"                "};

const char* GdaConst::delete_icon_xpm[] = {
    "16 16 14 1 ",
    "  c #E91611",
    ". c #EA1711",
    "X c #ED1813",
    "o c #FD1E18",
    "O c #FE1F1A",
    "+ c #F02917",
    "@ c #F02918",
    "# c #F02A19",
    "$ c #F02D1B",
    "% c #F02D1C",
    "& c #DA2E2A",
    "* c #DA302A",
    "= c #E93A2D",
    "- c None",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "---%%+#####$=---",
    "---*o.XXXX.o&---",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "----------------",
    "----------------"
};


wxString GdaConst::FieldTypeToStr(GdaConst::FieldType ft)
{
	if (ft == GdaConst::double_type) return "real";
	if (ft == GdaConst::long64_type) return "integer";
	if (ft == GdaConst::string_type) return "string";
	if (ft == GdaConst::date_type) return "date";
	if (ft == GdaConst::time_type) return "time";
	if (ft == GdaConst::datetime_type) return "datetime";
	if (ft == GdaConst::placeholder_type) return "placeholder";
	return "unknown";
}

std::map<std::string, GdaConst::DataSourceType> GdaConst::datasrc_str_to_type;
std::map<GdaConst::DataSourceType, std::string> GdaConst::datasrc_type_to_str;
std::map<GdaConst::DataSourceType,std::string> GdaConst::datasrc_type_to_prefix;
std::map<GdaConst::DataSourceType,std::string>
GdaConst::datasrc_type_to_fullname;
std::map<GdaConst::DataSourceType, std::set<std::string> >
GdaConst::datasrc_req_flds;
std::map<GdaConst::DataSourceType, std::set<std::string> >
GdaConst::datasrc_opt_flds;


wxString GdaConst::db_field_name_regex;
wxString GdaConst::db_field_name_illegal_regex;
wxString GdaConst::default_field_name_regex;
wxString GdaConst::default_field_name_illegal_regex;
wxString GdaConst::no_field_warning;
wxString GdaConst::db_field_warning;
wxString GdaConst::default_field_warning;
std::map<GdaConst::DataSourceType, int> GdaConst::datasrc_field_lens;
std::map<GdaConst::DataSourceType, int> GdaConst::datasrc_table_lens;
std::map<GdaConst::DataSourceType, wxString> GdaConst::datasrc_field_warning;
std::map<GdaConst::DataSourceType, wxString> GdaConst::datasrc_field_regex;
std::map<GdaConst::DataSourceType, wxString> GdaConst::datasrc_field_illegal_regex;
std::map<GdaConst::DataSourceType, bool> GdaConst::datasrc_field_casesensitive;

wxCursor GdaConst::zoomInCursor;
wxCursor GdaConst::zoomOutCursor;

// Resource Files
const wxString GdaConst::gda_prefs_fname_sqlite("geoda_prefs.sqlite");
const wxString GdaConst::gda_prefs_fname_json("geoda_prefs.json");
const wxString GdaConst::gda_prefs_html_table("html_entries");
const wxString GdaConst::gda_prefs_html_table_menu("menu_title");
const wxString GdaConst::gda_prefs_html_table_url("url");

wxFont* GdaConst::extra_small_font = 0;
wxFont* GdaConst::small_font = 0;
wxFont* GdaConst::medium_font = 0;
wxFont* GdaConst::large_font = 0;

bool GdaConst::use_cross_hatching = false;
int GdaConst::transparency_highlighted = 190;
int GdaConst::transparency_unhighlighted = 255;
int GdaConst::transparency_map_on_basemap = 200;
bool GdaConst::use_basemap_by_default = false;
int GdaConst::default_basemap_selection = 0;
bool GdaConst::hide_sys_table_postgres = false;
bool GdaConst::hide_sys_table_sqlite = false;
bool GdaConst::disable_crash_detect = false;
bool GdaConst::disable_auto_upgrade = false;
int GdaConst::plot_transparency_highlighted = 255;
int GdaConst::plot_transparency_unhighlighted = 50;

const wxPen* GdaConst::default_myshape_pen=0;
const wxBrush* GdaConst::default_myshape_brush=0;

//background color -- this is light gray
const wxColour GdaConst::backColor(192, 192, 192);
// background color -- this is light gray
const wxColour GdaConst::darkColor(20, 20, 20);
// color of text, frames, points -- this is dark cherry
const wxColour GdaConst::textColor(128, 0, 64);
// outliers color (also used for regression, etc.) -- blue
const wxColour GdaConst::outliers_colour(0, 0, 255);
// envelope color (also used for regression, etc.) -- blue
const wxColour GdaConst::envelope_colour(0, 0, 255);

const wxColour GdaConst::selectable_outline_color(0, 0, 0); // black
const wxColour GdaConst::selectable_fill_color(49, 163, 84); // forest green
const wxColour GdaConst::highlight_color(255, 255, 0); // yellow
const wxColour GdaConst::canvas_background_color(255, 255, 255); // white
const wxColour GdaConst::legend_background_color(255, 255, 255); // white

// Map
const wxSize GdaConst::map_default_size(600, 400);
const int GdaConst::map_default_legend_width = 150;
// this is a light forest green
const wxColour GdaConst::map_default_fill_colour(49, 163, 84);
const wxColour GdaConst::map_default_outline_colour(0, 0, 0);
const wxColour GdaConst::map_default_highlight_colour(255, 255, 0); // yellow

// Connectivity Map
const wxSize GdaConst::conn_map_default_size(480, 350);
// HTML Tan
const wxColour GdaConst::conn_map_default_fill_colour(210, 180, 140);
const wxColour GdaConst::conn_map_default_outline_colour(0, 0, 0);
// HTML DarkBlue
const wxColour GdaConst::conn_map_default_highlight_colour(0, 0, 139);

// Map Movie
const wxColour GdaConst::map_movie_default_fill_colour(49, 163, 84);
const wxColour GdaConst::map_movie_default_highlight_colour(224, 113, 182);

// Histogram
const wxSize GdaConst::hist_default_size(600, 500);

// Table
const wxString GdaConst::placeholder_str("<placeholder>");
const wxString GdaConst::table_frame_title("Table");
const wxSize GdaConst::table_default_size(750, 500);
const wxColour GdaConst::table_no_edit_color(80, 80, 80); // grey
const wxColour GdaConst::table_row_sel_color(230, 220, 40); // golden
const wxColour GdaConst::table_col_sel_color(181, 213, 251); // light blue
// following is the combination of the above two.  Light greenish
const wxColour GdaConst::table_row_and_col_sel_color(206, 217, 146);

// Scatterplot
const wxSize GdaConst::scatterplot_default_size(530, 530);
const wxColour GdaConst::scatterplot_scale_color(0, 0, 0);
//const wxColour GdaConst::scatterplot_regression_color(0, 79, 241); 
//const wxColour GdaConst::scatterplot_regression_selected_color(204, 41, 44); 
//const wxColour GdaConst::scatterplot_regression_excluded_color(0, 146, 31);
const wxColour GdaConst::scatterplot_regression_color(100, 0, 110); 
const wxColour GdaConst::scatterplot_regression_selected_color(204, 41, 44); 
const wxColour GdaConst::scatterplot_regression_excluded_color(0, 79, 241);
const wxColour GdaConst::scatterplot_origin_axes_color(120, 120, 120);
wxPen* GdaConst::scatterplot_reg_pen = 0;
wxPen* GdaConst::scatterplot_reg_selected_pen = 0;
wxPen* GdaConst::scatterplot_reg_excluded_pen = 0;
wxPen* GdaConst::scatterplot_scale_pen = 0;
wxPen* GdaConst::scatterplot_origin_axes_pen = 0;

// Bubble Chart
const wxSize GdaConst::bubble_chart_default_size(550, 400);
const int GdaConst::bubble_chart_default_legend_width = 150;

// 3D Plot
// yellow
const wxColour GdaConst::three_d_plot_default_highlight_colour(255, 255, 0);
// white
const wxColour GdaConst::three_d_plot_default_point_colour(255, 255, 255);
// black
const wxColour GdaConst::three_d_plot_default_background_colour(0, 0, 0);
const wxSize GdaConst::three_d_default_size(700, 500);

// Boxplot
const wxSize GdaConst::boxplot_default_size(300, 500);
const wxColour GdaConst::boxplot_point_color(0, 0, 255);
const wxColour GdaConst::boxplot_median_color(219, 99, 28); // orange
const wxColour GdaConst::boxplot_mean_point_color(20, 200, 20); // green
const wxColour GdaConst::boxplot_q1q2q3_color(128, 0, 64); // dark cherry

// PCP (Parallel Coordinate Plot)
const wxSize GdaConst::pcp_default_size(600, 450);
const wxColour GdaConst::pcp_line_color(128, 0, 64); // dark cherry
const wxColour GdaConst::pcp_horiz_line_color(0, 98, 0); // dark green

// Averages Chart
const wxSize GdaConst::line_chart_default_size(800, 620);

// colors defined in init()
wxColour GdaConst::ln_cht_clr_regimes_hl;
wxColour GdaConst::ln_cht_clr_sel_dark;
wxColour GdaConst::ln_cht_clr_exl_dark;
wxColour GdaConst::ln_cht_clr_tm1_dark;
wxColour GdaConst::ln_cht_clr_tm2_dark;
wxColour GdaConst::ln_cht_clr_sel_light;
wxColour GdaConst::ln_cht_clr_exl_light;
wxColour GdaConst::ln_cht_clr_tm1_light;
wxColour GdaConst::ln_cht_clr_tm2_light;

// Conditional View
const wxSize GdaConst::cond_view_default_size(700, 500);

// Category Classification
const wxSize GdaConst::cat_classif_default_size(780, 520);

const wxSize GdaConst::weights_man_dlg_default_size(700, 500);

const wxSize GdaConst::data_change_type_frame_default_size(600, 400);

std::vector<wxColour> GdaConst::qualitative_colors(10);

const wxString GdaConst::html_submenu_title("Web Plugins");


/**
 Certain objects such as wxFont objects need to be created after
 wxWidgets is sufficiently initialized.  This function will be
 called just once by GdaApp::OnInit() when the program begins.
 */
void GdaConst::init()
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
	
	// GdaShape resources
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

	wxBitmap zoomin_bitmap(GdaConst::raw_zoom_in);
    zoomInCursor = wxCursor(zoomin_bitmap.ConvertToImage());

	wxBitmap zoomout_bitmap(GdaConst::raw_zoom_out);
    zoomOutCursor = wxCursor(zoomout_bitmap.ConvertToImage());
	// Averages Chart Colors
	
	ln_cht_clr_regimes_hl = wxColour(255,255,102); // yellow #FFFF66
	ln_cht_clr_sel_dark = wxColour(204, 41, 44); // red
	ln_cht_clr_sel_light = GdaColorUtils::ChangeBrightness(ln_cht_clr_sel_dark,125);
	ln_cht_clr_exl_dark = wxColour(0, 79, 241); // blue
	ln_cht_clr_exl_light = GdaColorUtils::ChangeBrightness(ln_cht_clr_exl_dark, 125);
	ln_cht_clr_tm1_dark = wxColour(147, 36, 255); // purple
	ln_cht_clr_tm1_light = GdaColorUtils::ChangeBrightness(ln_cht_clr_tm1_dark, 125);
	ln_cht_clr_tm2_dark = wxColour(115, 61, 26); // brown
	ln_cht_clr_tm2_light = GdaColorUtils::ChangeBrightness(ln_cht_clr_tm2_dark, 125);
	
	// Following 4 colors are colour-blind safe colors from Color Brewer 2.0
	// LineChartStats::
	// wxColour _sample0_clr_light(178,223,138); // light green
	// wxColour _sample1_clr_light(166,206,227); // light blue
	// wxColour _sample0_clr_dark(51,160,44); // dark green
	// wxColour _sample1_clr_dark(31,120,180); // dark blue
	
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
	
    
	// Filenames or field names start with a letter, and they can contain any
	// combination of the letters A through Z, the digits 0 through 9,
	// the colon (:) (in dBASE II field names only), and the underscore (_)
	default_field_name_regex = "^[a-zA-Z][a-zA-Z0-9_]*$";
	default_field_name_illegal_regex = "((^[^a-zA-Z]+)|[^a-zA-Z0-9_]+)";
	// There might be a problem when field name contains ' or " when using
	// INSERT sql clause to export/save table.
	// Details: no problem in Postgresq; error in Oracle; unknown in MySQL
	db_field_name_regex = "[^'\"]+";
	db_field_name_illegal_regex = "['\"]+";
	// Warning message for valid field name of different field type
	no_field_warning = "There is no restriction of variable name.";
	db_field_warning = "A valid variable name should not contains any\n"
	"quotes (' or \").";
	default_field_warning = 
	"A valid variable name should have the first character be a\n"
	"letter, and the remaining characters be either letters,\n"
	"numbers or underscores.";
	
	datasrc_str_to_type["DBF"] = ds_dbf;
	datasrc_type_to_prefix[ds_dbf] = "";
	datasrc_type_to_fullname[ds_dbf] = "dBase";
	datasrc_table_lens[ds_dbf] = 128;
	datasrc_field_lens[ds_dbf] = 10;
	datasrc_field_warning[ds_dbf] = default_field_warning;
	datasrc_field_regex[ds_dbf] = default_field_name_regex;
	datasrc_field_illegal_regex[ds_dbf] = default_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_dbf] = false;
	
	datasrc_str_to_type["ESRI Shapefile"] = ds_shapefile;
	datasrc_type_to_prefix[ds_shapefile] = "";
	datasrc_type_to_fullname[ds_shapefile] = "ESRI Shapefile";
	// share the same with DBF
	datasrc_table_lens[ds_shapefile] = 128;
	datasrc_field_lens[ds_shapefile] = 10;
	datasrc_field_warning[ds_shapefile] = default_field_warning;
	datasrc_field_regex[ds_shapefile] = default_field_name_regex;
	datasrc_field_illegal_regex[ds_shapefile] = default_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_shapefile] = false;
	
	datasrc_str_to_type["FileGDB"] = ds_esri_file_geodb;
	datasrc_type_to_prefix[ds_esri_file_geodb] = "";
	datasrc_type_to_fullname[ds_esri_file_geodb] = "ESRI File GeoDatabase";
	//http://webhelp.esri.com/arcgisserver/9.3/java/index.htm#geodatabases/file_g-1445296021.htm
	datasrc_table_lens[ds_esri_file_geodb] = 160;
	datasrc_field_lens[ds_esri_file_geodb] = 64;
	datasrc_field_warning[ds_esri_file_geodb] = no_field_warning;
	datasrc_field_regex[ds_esri_file_geodb] = wxEmptyString;
	datasrc_field_casesensitive[ds_esri_file_geodb] = true;
	
	datasrc_str_to_type["PGeo"] = ds_esri_personal_gdb;
	datasrc_type_to_prefix[ds_esri_personal_gdb] = "PGeo";
	datasrc_type_to_fullname[ds_esri_personal_gdb] ="ESRI Personal GeoDatabase";
	//follows Microsoft Access .mdb
	datasrc_table_lens[ds_esri_personal_gdb] = 64;
	datasrc_field_lens[ds_esri_personal_gdb] = 64;
	datasrc_field_warning[ds_esri_personal_gdb] = no_field_warning;
	datasrc_field_regex[ds_esri_personal_gdb] = wxEmptyString;
	datasrc_field_casesensitive[ds_esri_personal_gdb] = true;
	
	datasrc_str_to_type["SDE"] = ds_esri_arc_sde;
	datasrc_type_to_prefix[ds_esri_arc_sde] = "SDE:";
	datasrc_type_to_fullname[ds_esri_arc_sde] = "ESRI ArcSDE";
	//http://help.arcgis.com/en/geodatabase/10.0/sdk/arcsde/api/constants_define_limits.htm
	//ArcSDE doesn't support writing, so following will be ignored
	datasrc_table_lens[ds_esri_arc_sde] = 32;
	datasrc_field_lens[ds_esri_arc_sde] = 32;
	datasrc_field_warning[ds_esri_arc_sde] = default_field_warning;
	datasrc_field_regex[ds_esri_arc_sde] = default_field_name_regex;
	datasrc_field_illegal_regex[ds_esri_arc_sde]=default_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_esri_arc_sde] = true;
	
	datasrc_str_to_type["CSV"] = ds_csv;
	datasrc_type_to_prefix[ds_csv] = "";
	datasrc_type_to_fullname[ds_csv] = "Comma Separated Value";
	//CSV should have no restriction except the comma character, but we give
	//a limitation to a reasonable 128 length
	datasrc_table_lens[ds_csv] = 128;
	datasrc_field_lens[ds_csv] = 128;
	datasrc_field_warning[ds_csv] = "Field name should not contains comma(,).";
	datasrc_field_regex[ds_csv] = "[^,]+";
	datasrc_field_illegal_regex[ds_csv] = "[,]+";
	datasrc_field_casesensitive[ds_csv] = true;
	
	datasrc_str_to_type["GeoJSON"] = ds_geo_json;
	datasrc_type_to_prefix[ds_geo_json] = "";
	datasrc_type_to_fullname[ds_geo_json] = "GeoJSON";
	//GeoJSON seams like has no restriction
	datasrc_table_lens[ds_geo_json] = 128;
	datasrc_field_lens[ds_geo_json] = 128;
	datasrc_field_warning[ds_geo_json] = db_field_warning;
	datasrc_field_regex[ds_geo_json] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_geo_json] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_geo_json] = true;
	
	datasrc_str_to_type["GML"] = ds_gml;
	datasrc_type_to_prefix[ds_gml] = "";
	datasrc_type_to_fullname[ds_gml] = "Geography Markup Language";
	//http://en.wikipedia.org/wiki/XML
	datasrc_table_lens[ds_gml] = 128;
	datasrc_field_lens[ds_gml] = 128;
	datasrc_field_warning[ds_gml] = 
	"A valid variable name should only contains either letters,\n"
	"numbers or underscores. Brackets (e.g. < or > ) are not allowed.";
	datasrc_field_regex[ds_gml] = "[^<> ]+";
	datasrc_field_illegal_regex[ds_gml] = "[<> ]+";
	datasrc_field_casesensitive[ds_gml] = true;
	
	datasrc_str_to_type["KML"] = ds_kml;
	datasrc_str_to_type["LIBKML"] = ds_kml;
	datasrc_type_to_prefix[ds_kml] = "";
	datasrc_type_to_fullname[ds_kml] = "Keyhole Markup Language";
	datasrc_table_lens[ds_kml] = 128;
	datasrc_field_lens[ds_kml] = 128;
	datasrc_field_warning[ds_kml] = db_field_warning;
	datasrc_field_regex[ds_kml] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_kml] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_kml] = true;
	
	datasrc_str_to_type["MapInfo File"] = ds_mapinfo;
	datasrc_type_to_prefix[ds_mapinfo] = "";
	datasrc_type_to_fullname[ds_mapinfo] = "MapInfo File (TAB and MIF/MID)";
	datasrc_table_lens[ds_mapinfo] = 128;
	datasrc_field_lens[ds_mapinfo] = 32;
	datasrc_field_warning[ds_mapinfo] = default_field_warning;
	datasrc_field_regex[ds_mapinfo] = default_field_name_regex;
	datasrc_field_illegal_regex[ds_mapinfo] = default_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_mapinfo] = true;
	
	datasrc_str_to_type["MySQL"] = ds_mysql;
	datasrc_type_to_prefix[ds_mysql] = "MYSQL:";
	datasrc_type_to_fullname[ds_mysql] = "MySQL";
	datasrc_table_lens[ds_mysql] = 64;
	datasrc_field_lens[ds_mysql] = 64;
	datasrc_field_warning[ds_mysql] = db_field_warning;
	datasrc_field_regex[ds_mysql] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_mysql] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_mysql] = true;
	
	datasrc_str_to_type["OCI"] = ds_oci;
	datasrc_type_to_prefix[ds_oci] = "OCI:";
	datasrc_type_to_fullname[ds_oci] = "Oracle Spatial";
	datasrc_table_lens[ds_oci] = 30;
	datasrc_field_lens[ds_oci] = 30;
	datasrc_field_warning[ds_oci] = db_field_warning;
	datasrc_field_regex[ds_oci] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_oci] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_oci] = false;
	
	datasrc_str_to_type["PostgreSQL"] = ds_postgresql;
	datasrc_type_to_prefix[ds_postgresql] = "PG:";
	datasrc_type_to_fullname[ds_postgresql] = "PostgreSQL / PostGIS";
	datasrc_table_lens[ds_postgresql] = 31;
	datasrc_field_lens[ds_postgresql] = 31;
	datasrc_field_warning[ds_postgresql] = db_field_warning;
	datasrc_field_regex[ds_postgresql] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_postgresql] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_postgresql] = true;
	
	datasrc_str_to_type["CartoDB"] = ds_cartodb;
	datasrc_type_to_prefix[ds_cartodb] = "CartoDB:";
	datasrc_type_to_fullname[ds_cartodb] = "CartoDB";
	datasrc_table_lens[ds_cartodb] = 31;
	datasrc_field_lens[ds_cartodb] = 31;
	datasrc_field_warning[ds_cartodb] = db_field_warning;
	datasrc_field_regex[ds_cartodb] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_cartodb] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_cartodb] = true;
    
	datasrc_str_to_type["SQLite"] = ds_sqlite;
	datasrc_type_to_prefix[ds_sqlite] = "";
	datasrc_type_to_fullname[ds_sqlite] = "SQLite / Spatialite";
	datasrc_table_lens[ds_sqlite] = 128;
	datasrc_field_lens[ds_sqlite] = 128;
	datasrc_field_warning[ds_sqlite] = no_field_warning;
	datasrc_field_regex[ds_sqlite] = wxEmptyString;
	datasrc_field_illegal_regex[ds_sqlite] = wxEmptyString;
	datasrc_field_casesensitive[ds_sqlite] = true;
	
	datasrc_str_to_type["GPKG"] = ds_gpkg;
	datasrc_type_to_prefix[ds_gpkg] = "";
	datasrc_type_to_fullname[ds_gpkg] = "GeoPackage";
	datasrc_table_lens[ds_gpkg] = 128;
	datasrc_field_lens[ds_gpkg] = 128;
	datasrc_field_warning[ds_gpkg] = no_field_warning;
	datasrc_field_regex[ds_gpkg] = wxEmptyString;
	datasrc_field_illegal_regex[ds_gpkg] = wxEmptyString;
	datasrc_field_casesensitive[ds_gpkg] = true;
    
	datasrc_str_to_type["WFS"] = ds_wfs;
	datasrc_type_to_prefix[ds_wfs] = "WFS:";
	datasrc_type_to_fullname[ds_wfs] = "OGC Web Feature Service";
	datasrc_table_lens[ds_wfs] = 128;
	datasrc_field_lens[ds_wfs] = 128;
	datasrc_field_warning[ds_wfs] = no_field_warning;
	datasrc_field_regex[ds_wfs] = wxEmptyString;
	datasrc_field_illegal_regex[ds_wfs] = wxEmptyString;
	datasrc_field_casesensitive[ds_wfs] = true;
	
	// Since XLS can be dBase, we use dBase as its field name limitation
	datasrc_str_to_type["XLS"] = ds_xls;
	datasrc_type_to_prefix[ds_xls] = "";
	datasrc_type_to_fullname[ds_xls] = "Microsoft Excel";
	datasrc_table_lens[ds_xls] = 128;
	datasrc_field_lens[ds_xls] = 10;
	datasrc_field_warning[ds_xls] = default_field_warning;
	datasrc_field_regex[ds_xls] = default_field_name_regex;
	datasrc_field_illegal_regex[ds_xls] = default_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_xls] = false;
    
    datasrc_str_to_type["XLSX"] = ds_xlsx;
    datasrc_type_to_prefix[ds_xlsx] = "";
    datasrc_type_to_fullname[ds_xlsx] = "Microsoft Excel Extensions";
    datasrc_table_lens[ds_xlsx] = 128;
    datasrc_field_lens[ds_xlsx] = 10;
    datasrc_field_warning[ds_xlsx] = default_field_warning;
    datasrc_field_regex[ds_xlsx] = default_field_name_regex;
    datasrc_field_illegal_regex[ds_xlsx] = default_field_name_illegal_regex;
    datasrc_field_casesensitive[ds_xlsx] = false;
    
    datasrc_str_to_type["ODS"] = ds_ods;
    datasrc_type_to_prefix[ds_ods] = "";
    datasrc_type_to_fullname[ds_ods] = "Open Office Spreadsheet";
    datasrc_table_lens[ds_ods] = 128;
    datasrc_field_lens[ds_ods] = 128;
    datasrc_field_warning[ds_ods] = default_field_warning;
    datasrc_field_regex[ds_ods] = default_field_name_regex;
    datasrc_field_illegal_regex[ds_ods] = default_field_name_illegal_regex;
    datasrc_field_casesensitive[ds_ods] = false;
	
	//not supported yet
	//datasrc_str_to_type["OSM"] = ds_osm;
	//datasrc_type_to_prefix[ds_xls] = "";
	//datasrc_type_to_fullname[ds_xls] = "OSM";
	
	//datasrc_str_to_type["MSSQLSpatial"] = ds_ms_sql;
	//datasrc_type_to_prefix[ds_ms_sql] = "MSSQL:";
	//datasrc_type_to_fullname[ds_ms_sql] = "Microsoft SQL Server";
	
	//datasrc_str_to_type["ArcObjects"] = ds_esri_arc_obj;
	//datasrc_type_to_prefix[ds_esri_arc_obj] = "AO";
	//datasrc_type_to_fullname[ds_esri_arc_obj] = "ESRI ArcObjects";
	
	//datasrc_str_to_type["ODBC"] = ds_odbc;
	//datasrc_type_to_prefix[ds_odbc] = "ODBC:";
	//datasrc_type_to_fullname[ds_odbc] = "ODBC";
	
	typedef std::map<std::string, DataSourceType> ds_map;
	for (ds_map::iterator it=datasrc_str_to_type.begin();
			 it != datasrc_str_to_type.end(); it++) {
		datasrc_type_to_str[it->second] = it->first;
		datasrc_req_flds[it->second] = std::set<std::string>();
		datasrc_opt_flds[it->second] = std::set<std::string>();
	}
	
	typedef std::map<DataSourceType, std::set<std::string> > ds_fld_map;
	for (ds_fld_map::iterator it=datasrc_req_flds.begin();
			 it != datasrc_req_flds.end(); it++) {
		DataSourceType type = it->first;
		if (type == ds_esri_file_geodb || type == ds_csv ||
            type == ds_dbf || type == ds_gml ||
            type == ds_kml || type == ds_mapinfo ||
            type == ds_shapefile || type == ds_sqlite ||
            type == ds_gpkg || type == ds_xls ||
            type == ds_geo_json || type == ds_osm)
        {
			// These are simple files, and a file name must be supplied
			it->second.insert("file");
		} else if (type == ds_esri_arc_obj || type == ds_esri_personal_gdb ||
                   type == ds_esri_arc_sde || type == ds_mysql ||
                   type == ds_ms_sql || type == ds_oci || type == ds_odbc)
        {
			it->second.insert("user");
			it->second.insert("pwd");
			it->second.insert("host");
			it->second.insert("port");
			it->second.insert("db_name");
		} else if ( type == ds_wfs) {
            
			it->second.insert("url");
		} else if (type == ds_postgresql) {
            
			it->second.insert("db_name");
		}
	}
	
	for (ds_fld_map::iterator it=datasrc_opt_flds.begin();
			 it != datasrc_opt_flds.end(); it++) {
		DataSourceType type = it->first;
		if (type == ds_postgresql) {
			it->second.insert("user");
			it->second.insert("pwd");
			it->second.insert("host");
			it->second.insert("port");
		}
	}

}


