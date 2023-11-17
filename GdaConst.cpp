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
#include <wx/tokenzr.h>

const std::vector<wxString> GdaConst::sample_names({
    "Chicago Community Areas",
    "Chicago Carjackings",
    "Chicago SDOH",
    "Ceara Zika",
    "Oaxaca Development",
    "Italy Community Banks",
    "Spirals",
    "Moral Statistics of France (1833)",
    "US County Homicides",
    "House Prices Baltimore",
    "House Prices Boston",
    "NYC Data"
});

const std::vector<wxString> GdaConst::sample_layer_names({
    "Chicago Community Areas",
    "Chicago Carjackings",
    "Chicago SDOH",
    "Ceara Zika",
    "Oaxaca Development",
    "Italy Community Banks",
    "Spirals",
    "Guerry",
    "US Homicides",
    "Baltimore Home Sales",
    "Boston Home Sales",
    "NYC Data"
});

const std::vector<wxString> GdaConst::sample_datasources({
    "samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
	"samples.sqlite",
    "samples.sqlite",
	"samples.sqlite"
});

const std::vector<wxString> GdaConst::sample_meta_urls({
    "https://geodacenter.github.io/data-and-lab/Chi-CCA/",
    "https://geodacenter.github.io/data-and-lab/Chi-Carjackings/",
    "https://geodacenter.github.io/data-and-lab/Chi-SDOH/",
    "https://geodacenter.github.io/data-and-lab/Ceara-Zika/",
    "https://geodacenter.github.io/data-and-lab/Oaxaca-Development/",
    "https://geodacenter.github.io/data-and-lab/Italy-Community-Banks/",
    "https://geodacenter.github.io/data-and-lab/Spirals/",
    "https://geodacenter.github.io/data-and-lab/Guerry/",
    "https://geodacenter.github.io/data-and-lab/ncovr/",
    "https://geodacenter.github.io/data-and-lab/baltim/",
    "https://geodacenter.github.io/data-and-lab/boston-housing/",
    "https://geodacenter.github.io/data-and-lab/nyc/"
});

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
    "16 16 31 1 ",
    "  c #CD5050",
    ". c #D76262",
    "X c #DA6868",
    "o c #DB6868",
    "O c #DB6969",
    "+ c #DC6969",
    "@ c #DC6B6B",
    "# c #DE6B6B",
    "$ c #DD6C6C",
    "% c #E17070",
    "& c #E07171",
    "* c #E07272",
    "= c #E17272",
    "- c #E27373",
    "; c #E37373",
    ": c #E37474",
    "> c #EB7C7C",
    ", c #F88E8E",
    "< c #FB9191",
    "1 c #FB9292",
    "2 c #FA9393",
    "3 c #FD9191",
    "4 c #FF9191",
    "5 c #FF9393",
    "6 c #FD9494",
    "7 c #FE9494",
    "8 c #FF9494",
    "9 c #FF9595",
    "0 c #FF9696",
    "q c #FF9898",
    "w c None",
    "wwwwwwwwwwwwwwww",
    "www wwwwwwwwwwww",
    "ww#8#wwwwwww-5.w",
    "ww&88Owwwww&88Ow",
    "www&88Owww-88Oww",
    "wwww-88$w-88Owww",
    "wwwww&q,>88Owwww",
    "wwwwww-111Owwwww",
    "wwwwww&125Owwwww",
    "wwwww&q,>18Owwww",
    "wwww&85$w&88Owww",
    "www&88#www-88Oww",
    "ww&q8Owwwww&88Ow",
    "ww#8$wwwwwww-5.w",
    "www wwwwwwwwwwww",
    "wwwwwwwwwwwwwwww"
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

bool GdaConst::gda_draw_map_labels = false;
int GdaConst::gda_map_label_font_size = 6;
bool GdaConst::gda_create_csvt = false;
bool GdaConst::gda_enable_set_transparency_windows = false;
int GdaConst::default_display_decimals = 6; // move in preference
double GdaConst::gda_autoweight_stop = 0.0001; // move in preference
bool GdaConst::gda_use_gpu = false;
int GdaConst::gda_ui_language = 0;
double GdaConst::gda_eigen_tol = 0.00000001;
bool GdaConst::gda_set_cpu_cores = true;
int GdaConst::gda_cpu_cores = 6;
wxString GdaConst::gda_user_email = "";
uint64_t GdaConst::gda_user_seed = 123456789;
bool GdaConst::use_gda_user_seed = true;
int GdaConst::gdal_http_timeout = 5;

#ifdef __WXOSX__
bool GdaConst::enable_high_dpi_support = true;
#else
bool GdaConst::enable_high_dpi_support = false;
#endif

bool GdaConst::show_csv_configure_in_merge = true;
bool GdaConst::show_recent_sample_connect_ds_dialog = true;
bool GdaConst::use_cross_hatching = false;
int GdaConst::transparency_highlighted = 255;
int GdaConst::transparency_unhighlighted = 80;
int GdaConst::transparency_map_on_basemap = 200;
bool GdaConst::use_basemap_by_default = false;
int GdaConst::default_basemap_selection = 0;
bool GdaConst::hide_sys_table_postgres = false;
bool GdaConst::hide_sys_table_sqlite = false;
bool GdaConst::disable_crash_detect = false;
bool GdaConst::disable_auto_upgrade = false;
int GdaConst::plot_transparency_highlighted = 255;
int GdaConst::plot_transparency_unhighlighted = 50;
int GdaConst::gda_ogr_csv_header = 1;
wxString GdaConst::gda_ogr_csv_x_name = "";
wxString GdaConst::gda_ogr_csv_y_name = "";
wxString GdaConst::gda_display_datetime_format = "";
std::vector<wxString> GdaConst::gda_datetime_formats;
wxString GdaConst::gda_datetime_formats_str = "%Y-%m-%d %H:%M:%S,"
    "%Y/%m/%d %H:%M:%S,"
    "%d.%m.%Y %H:%M:%S,"
    "%m/%d/%Y %H:%M:%S,"
    "%Y-%m-%d,"
    "%m/%d/%Y,"
    "%Y/%m/%d,"
    "%H:%M:%S,"
    "%H:%M,"
    "%Y/%m/%d %H:%M %p,"
    "%m/%d/%Y %I:%M:%S %p";
wxString GdaConst::gda_basemap_sources =
"Carto.Light,https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}@2x.png"
"\nCarto.Dark,https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}@2x.png"
"\nCarto.Light(No Label),https://{s}.basemaps.cartocdn.com/light_nolabels/{z}/{x}/{y}@2x.png"
"\nCarto.Dark(No Label),https://{s}.basemaps.cartocdn.com/dark_nolabels/{z}/{x}/{y}@2x.png"
"\nESRI.WorldStreetMap,https://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/{z}/{y}/{x}"
"\nESRI.WorldTopoMap,https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/{z}/{y}/{x}"
"\nESRI.WorldTerrain,https://server.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/{z}/{y}/{x}"
"\nESRI.Ocean,https://server.arcgisonline.com/ArcGIS/rest/services/Ocean_Basemap/MapServer/tile/{z}/{y}/{x}"
"\nHERE.Day,http://{s}.base.maps.api.here.com/maptile/2.1/maptile/newest/normal.day/{z}/{x}/{y}/256/png8?app_id=HERE_APP_ID&app_code=HERE_APP_CODE"
"\nHERE.Night,http://{s}.base.maps.api.here.com/maptile/2.1/maptile/newest/normal.night/{z}/{x}/{y}/256/png8?app_id=HERE_APP_ID&app_code=HERE_APP_CODE"
"\nHERE.Hybrid,http://{s}.aerial.maps.api.here.com/maptile/2.1/maptile/newest/hybrid.day/{z}/{x}/{y}/256/png8?app_id=HERE_APP_ID&app_code=HERE_APP_CODE"
"\nHERE.Satellite,http://{s}.aerial.maps.api.here.com/maptile/2.1/maptile/newest/satellite.day/{z}/{x}/{y}/256/png8?app_id=HERE_APP_ID&app_code=HERE_APP_CODE"
"\nOpenStreetMap.Mapnik,https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
"\nStamen.Toner,https://stamen-tiles-{s}.a.ssl.fastly.net/toner/{z}/{x}/{y}@2x.png"
"\nStamen.TonerLite,https://stamen-tiles-{s}.a.ssl.fastly.net/toner-lite/{z}/{x}/{y}@2x.png"
"\nStamen.Watercolor,https://stamen-tiles-{s}.a.ssl.fastly.net/watercolor/{z}/{x}/{y}.jpg"
"\nOther (China).GaoDe,http://webst{s}.is.autonavi.com/appmaptile?style=8&x={x}&y={y}&z={z}"
"\nOther (China).GaoDe(Satellite),http://webst{s}.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}"
;
const wxString GdaConst::gda_basemap_osm_useragent = "GeoDa 1.14 contact spatial@uchiago.edu";
const wxString GdaConst::gda_basemap_win_useragent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36";
const wxString GdaConst::gda_basemap_mac_useragent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36";
const wxString GdaConst::gda_basemap_linux_useragent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36";

const wxString GdaConst::gda_lbl_not_sig = _("Not Significant");
const wxString GdaConst::gda_lbl_undefined = _("Undefined");
const wxString GdaConst::gda_lbl_neighborless = _("Neighborless");
const wxString GdaConst::gda_lbl_highhigh = _("High-High");
const wxString GdaConst::gda_lbl_lowlow = _("Low-Low");
const wxString GdaConst::gda_lbl_lowhigh = _("Low-High");
const wxString GdaConst::gda_lbl_highlow = _("High-Low");
const wxString GdaConst::gda_lbl_otherpos = _("Other Pos");
const wxString GdaConst::gda_lbl_negative = _("Negative");
const wxString GdaConst::gda_lbl_positive = _("Positive");
const wxString GdaConst::gda_lbl_1p = "< 1%";
const wxString GdaConst::gda_lbl_1p_10p = "1% - 10%";
const wxString GdaConst::gda_lbl_10p_50p = "10% - 50%";
const wxString GdaConst::gda_lbl_50p_90p = "50% - 90%";
const wxString GdaConst::gda_lbl_90p_99p = "90% - 99%";
const wxString GdaConst::gda_lbl_99p = "> 99%";
const wxString GdaConst::gda_lbl_loweroutlier = _("Lower outlier");
const wxString GdaConst::gda_lbl_25p = "< 25%";
const wxString GdaConst::gda_lbl_25p_50p = "25% - 50%";
const wxString GdaConst::gda_lbl_50p_75p = "50% - 75%";
const wxString GdaConst::gda_lbl_75p = "> 75%";
const wxString GdaConst::gda_lbl_upperoutlier = _("Upper outlier");
const wxString GdaConst::gda_lbl_n2sigma = "< -2Std";
const wxString GdaConst::gda_lbl_n2sigma_n1sigma = "[-2Std, -1Std)";
const wxString GdaConst::gda_lbl_n1sigma = "[-1Std, Mean)";
const wxString GdaConst::gda_lbl_1sigma = "(Mean, 1Std]";
const wxString GdaConst::gda_lbl_1sigma_2sigma = "(1Std, 2Std]";
const wxString GdaConst::gda_lbl_2sigma = "> 2Std";
const wxString GdaConst::gda_projection_UNIT = "UNIT";
const wxString GdaConst::gda_projection_metre = "metre";
const wxString GdaConst::gda_projection_meter = "meter";

const wxPen* GdaConst::default_myshape_pen=0;
const wxBrush* GdaConst::default_myshape_brush=0;

const wxString GdaConst::gda_lang_english = "English";
const wxString GdaConst::gda_lang_chinese = "Chinese (Simplified)";
const wxString GdaConst::gda_lang_french = "French";
const wxString GdaConst::gda_lang_portuguese = "Portuguese";
const wxString GdaConst::gda_lang_russian = "Russian";
const wxString GdaConst::gda_lang_spanish = "Spanish";

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
const wxColour GdaConst::map_dark_gray(70, 70, 70);
const wxColour GdaConst::map_white(255, 255, 255);
wxColour GdaConst::map_undefined_colour(GdaConst::map_dark_gray);
const wxString GdaConst::map_undefined_label(_("undefined"));
const wxString GdaConst::map_unmatched_label(_("unmatched"));
wxString GdaConst::map_undefined_category(GdaConst::map_undefined_label);

// Connectivity Map
const wxSize GdaConst::conn_map_default_size(480, 350);
const wxColour GdaConst::conn_graph_outline_colour(55,55,55,100);
const wxColour GdaConst::conn_select_outline_colour(55,55,55,0);
const wxColour GdaConst::conn_neighbor_fill_colour(255,255,255,0);

// Heat Map
const wxColour GdaConst::heatmap_fill_colour(0, 0, 0, 10);
const wxColour GdaConst::heatmap_outline_colour(255, 255, 255, 0);
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
const wxString GdaConst::table_frame_title(_("Table"));
const wxSize GdaConst::table_default_size(750, 500);
const wxColour GdaConst::table_no_edit_color(80, 80, 80); // grey
const wxColour GdaConst::table_row_sel_color(230, 220, 40); // golden
const wxColour GdaConst::table_col_sel_color(181, 213, 251); // light blue
// following is the combination of the above two.  Light greenish
const wxColour GdaConst::table_row_and_col_sel_color(206, 217, 146);

// Scatterplot
const wxSize GdaConst::scatterplot_default_size(500, 500);
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
const wxSize GdaConst::three_d_default_size(820, 620);

// Boxplot
const wxSize GdaConst::boxplot_default_size(380, 500);
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
std::vector<wxColour> GdaConst::unique_colors_60(60);
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

	if (GeneralWxUtils::isWindows()) {
		ref_extra_small_pt_sz += 2;
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

    wxStringTokenizer tokenizer(GdaConst::gda_datetime_formats_str, ",");
    while (tokenizer.HasMoreTokens())
    {
        wxString token = tokenizer.GetNextToken();
        // process token here
        GdaConst::gda_datetime_formats.push_back(token);
    }
    
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
    
    // From http://phrogz.net/css/distinct-colors.html
   
    unique_colors_60[0] = wxColour(166,206,227),
    unique_colors_60[1] = wxColour(31,120,180),
    unique_colors_60[2] = wxColour(178,223,138),
    unique_colors_60[3] = wxColour(51,160,44),
    unique_colors_60[4] = wxColour(251,154,153),
    unique_colors_60[5] = wxColour(227,26,28),
    unique_colors_60[6] = wxColour(253,191,111),
    unique_colors_60[7] = wxColour(255,127,0),
    unique_colors_60[8] = wxColour(106,61,154),
    unique_colors_60[9] = wxColour(255,255,153),
    unique_colors_60[10] = wxColour(177,89,40),
    unique_colors_60[11] = wxColour(255,255,179),
    unique_colors_60[12] = wxColour(190,186,218),
    unique_colors_60[13] = wxColour(251,128,114),
    unique_colors_60[14] = wxColour(128,177,211),
    unique_colors_60[15] = wxColour(179,222,105),
    unique_colors_60[16] = wxColour(252,205,229),
    unique_colors_60[17] = wxColour(217,217,217),
    unique_colors_60[18] = wxColour(188,128,189),
    unique_colors_60[19] = wxColour(204,235,197),
    unique_colors_60[20] = wxColour(140,70,70);
    unique_colors_60[21] = wxColour(229,126,57);
    unique_colors_60[22] = wxColour(242,162,0);
    unique_colors_60[23] = wxColour(166,160,83);
    unique_colors_60[24] = wxColour(0,77,41);
    unique_colors_60[25] = wxColour(0,48,51);
    unique_colors_60[26] = wxColour(0,61,115);
    unique_colors_60[27] = wxColour(198,182,242);
    unique_colors_60[28] = wxColour(87,26,102);
    unique_colors_60[29] = wxColour(255,191,217);
    unique_colors_60[30] = wxColour(229,130,115);
    unique_colors_60[31] = wxColour(140,98,70);
    unique_colors_60[32] = wxColour(76,51,0);
    unique_colors_60[33] = wxColour(52,115,29);
    unique_colors_60[34] = wxColour(96,128,113);
    unique_colors_60[35] = wxColour(64,217,255);
    unique_colors_60[36] = wxColour(153,173,204);
    unique_colors_60[37] = wxColour(31,0,77);
    unique_colors_60[38] = wxColour(166,0,111);
    unique_colors_60[39] = wxColour(255,64,115);
    unique_colors_60[40] = wxColour(166,44,0);
    unique_colors_60[41] = wxColour(217,184,163);
    unique_colors_60[42] = wxColour(89,85,67);
    unique_colors_60[43] = wxColour(170,217,163);
    unique_colors_60[44] = wxColour(0,191,128);
    unique_colors_60[45] = wxColour(57,103,115);
    unique_colors_60[46] = wxColour(0,58,217);
    unique_colors_60[47] = wxColour(98,86,115);
    unique_colors_60[48] = wxColour(242,121,186);
    unique_colors_60[49] = wxColour(242,0,32);
    unique_colors_60[50] = wxColour(76,20,0);
    unique_colors_60[51] = wxColour(51,39,26);
    unique_colors_60[52] = wxColour(229,218,57);
    unique_colors_60[53] = wxColour(61,242,61);
    unique_colors_60[54] = wxColour(102,204,197);
    unique_colors_60[55] = wxColour(0,162,242);
    unique_colors_60[56] = wxColour(89,101,179);
    unique_colors_60[57] = wxColour(184,54,217);
    unique_colors_60[58] = wxColour(89,0,36);
    unique_colors_60[59] = wxColour(0,0,36);


	
    
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
	datasrc_field_regex[ds_dbf] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_dbf] = db_field_name_illegal_regex;
	datasrc_field_casesensitive[ds_dbf] = false;
	
    datasrc_str_to_type["Parquet"] = ds_parquet;
    datasrc_type_to_prefix[ds_parquet] = "";
    datasrc_type_to_fullname[ds_parquet] = "Parquet";
    // share the same with DBF
    datasrc_table_lens[ds_parquet] = 128;
    datasrc_field_lens[ds_parquet] = 10;
    datasrc_field_warning[ds_parquet] = default_field_warning;
    datasrc_field_regex[ds_parquet] = db_field_name_regex;
    datasrc_field_illegal_regex[ds_parquet] = db_field_name_illegal_regex;
    datasrc_field_casesensitive[ds_parquet] = false;
    
    datasrc_str_to_type["Arrow"] = ds_arrow;
    datasrc_type_to_prefix[ds_arrow] = "";
    datasrc_type_to_fullname[ds_arrow] = "Arrow";
    // share the same with DBF
    datasrc_table_lens[ds_arrow] = 128;
    datasrc_field_lens[ds_arrow] = 10;
    datasrc_field_warning[ds_arrow] = default_field_warning;
    datasrc_field_regex[ds_arrow] = db_field_name_regex;
    datasrc_field_illegal_regex[ds_arrow] = db_field_name_illegal_regex;
    datasrc_field_casesensitive[ds_arrow] = false;
    
	datasrc_str_to_type["ESRI Shapefile"] = ds_shapefile;
	datasrc_type_to_prefix[ds_shapefile] = "";
	datasrc_type_to_fullname[ds_shapefile] = "ESRI Shapefile";
	// share the same with DBF
	datasrc_table_lens[ds_shapefile] = 128;
	datasrc_field_lens[ds_shapefile] = 10;
	datasrc_field_warning[ds_shapefile] = default_field_warning;
	datasrc_field_regex[ds_shapefile] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_shapefile] = db_field_name_illegal_regex;
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
	datasrc_field_regex[ds_esri_arc_sde] = db_field_name_regex;
	datasrc_field_illegal_regex[ds_esri_arc_sde]=db_field_name_illegal_regex;
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
	
	datasrc_str_to_type["Carto"] = ds_cartodb;
	datasrc_type_to_prefix[ds_cartodb] = "Carto:";
	datasrc_type_to_fullname[ds_cartodb] = "Carto";
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


