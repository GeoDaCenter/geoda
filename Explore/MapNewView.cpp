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
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../DialogTools/SelectWeightsDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/WeightsManState.h"
#include "Basemap.h"
#include "MapNewView.h"

wxWindowID ID_SLIDER = wxID_ANY;

IMPLEMENT_CLASS(SliderDialog, wxDialog)
BEGIN_EVENT_TABLE(SliderDialog, wxDialog)
    EVT_COMMAND_SCROLL_THUMBRELEASE( ID_SLIDER, SliderDialog::OnSliderChange)
#ifdef __WIN32__
    EVT_COMMAND_SCROLL_CHANGED(ID_SLIDER, SliderDialog::OnSliderChange)
#endif
END_EVENT_TABLE()

SliderDialog::SliderDialog(wxWindow * parent,
                           TemplateCanvas* _canvas,
                           wxWindowID id,
                           const wxString & caption,
                           const wxPoint & position,
                           const wxSize & size,
                           long style )
: wxDialog( parent, id, caption, position, size, style)
{
    canvas = _canvas;
    
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);
    
    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // A text control for the user’s name
    ID_SLIDER = wxID_ANY;
    double trasp = canvas->GetTransparency();
    int trasp_scale = 100 * trasp;

	wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
    slider = new wxSlider(this, ID_SLIDER, trasp_scale, 0, 100,
                          wxDefaultPosition, wxSize(200, -1),
                          wxSL_HORIZONTAL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);
    subSizer->Add(slider, 0, wxALIGN_CENTER_VERTICAL|wxALL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"1.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);

	boxSizer->Add(subSizer);
    wxString txt_transparency = wxString::Format("Current Transparency: %.1f", trasp);
    
    slider_text = new wxStaticText(this,
                                   wxID_ANY,
                                   txt_transparency,
                                   wxDefaultPosition,
                                   wxSize(100, -1));
    boxSizer->Add(slider_text, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW|wxALL, 5);
    
    topSizer->Fit(this);
}

SliderDialog::~SliderDialog()
{
    
}

void SliderDialog::OnSliderChange( wxScrollEvent & event )
{
    int val = event.GetInt();
    double trasp = val / 100.0;
    slider_text->SetLabel(wxString::Format("Current Transparency: %.1f", trasp));
    canvas->SetTransparency(trasp);
    canvas->ReDraw();
}

IMPLEMENT_CLASS(MapCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(MapCanvas, TemplateCanvas)
#ifdef __linux__
	// in Linux, using old paint function without transparency support
	EVT_PAINT(TemplateCanvas::OnPaint)
#else
	EVT_PAINT(MapCanvas::OnPaint)
#endif
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
	//EVT_KEY_DOWN(TemplateCanvas::OnKeyDown)
END_EVENT_TABLE()


MapCanvas::MapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                     Project* project_s,
                     const std::vector<GdaVarTools::VarInfo>& var_info_s,
                     const std::vector<int>& col_ids_s,
                     CatClassification::CatClassifType theme_type,
                     SmoothingType smoothing_type_s,
                     int num_categories_s,
                     boost::uuids::uuid weights_id_s,
                     const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s, 
                 project_s->GetHighlightState(),
                 pos, size, true, true),
num_obs(project_s->GetNumRecords()),
num_time_vals(1),
custom_classif_state(0), 
data(0), 
var_info(0),
table_int(project_s->GetTableInt()),
smoothing_type(no_smoothing),
is_rate_smoother(false), 
full_map_redraw_needed(true),
display_mean_centers(false), 
display_centroids(false),
display_voronoi_diagram(false), 
voronoi_diagram_duplicates_exist(false),
num_categories(num_categories_s), 
weights_id(weights_id_s)
{
	using namespace Shapefile;
	LOG_MSG("Entering MapCanvas::MapCanvas");
	
	cat_classif_def.cat_classif_type = theme_type;
	if (theme_type == CatClassification::no_theme) {
		//cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
		//CatClassification::ChangeNumCats(1, cat_classif_def);
		//cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
	}
	selectable_fill_color = GdaConst::map_default_fill_colour;
	
	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 25;
	virtual_screen_marg_left = 25;
	virtual_screen_marg_right = 25;	
	shps_orig_xmin = project->main_data.header.bbox_x_min;
	shps_orig_ymin = project->main_data.header.bbox_y_min;
	shps_orig_xmax = project->main_data.header.bbox_x_max;
	shps_orig_ymax = project->main_data.header.bbox_y_max;
    
	int vs_w = GetVirtualSize().GetWidth();
	int vs_h = GetVirtualSize().GetHeight();

	SetScrollbars(1, 1, vs_w, vs_h, 0, 0);

	double scale_x, scale_y, trans_x, trans_y;
    GdaScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
                                    shps_orig_xmax, shps_orig_ymax,
                                    virtual_screen_marg_top,
                                    virtual_screen_marg_bottom,
                                    virtual_screen_marg_left,
                                    virtual_screen_marg_right,
                                    vs_w, vs_h,
                                    fixed_aspect_ratio_mode,
                                    fit_to_window_mode,
                                    &scale_x, &scale_y,
                                    &trans_x, &trans_y,
                                    0, 0,
                                    &current_shps_width,
                                    &current_shps_height);
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;

	if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GdaConst::map_default_highlight_colour;
	}
	
	use_category_brushes = true;
	if (!ChangeMapType(theme_type, smoothing_type_s, num_categories,
					   weights_id,
					   true, var_info_s, col_ids_s)) {
		// The user possibly clicked cancel.  Try again with
		// themeless map
		std::vector<GdaVarTools::VarInfo> vi(0);
		std::vector<int> cids(0);
		ChangeMapType(CatClassification::no_theme, no_smoothing, 1,
					  boost::uuids::nil_uuid(),
					  true, vi, cids);
	}

	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting MapCanvas::MapCanvas");
}

MapCanvas::~MapCanvas()
{
	LOG_MSG("Entering MapCanvas::~MapCanvas");
	if (highlight_state) highlight_state->removeObserver(this);
	if (custom_classif_state) custom_classif_state->removeObserver(this);
	LOG_MSG("Exiting MapCanvas::~MapCanvas");
}

bool MapCanvas::DrawBasemap(bool flag, int map_type)
{
    isDrawBasemap = flag;
    
    if (isDrawBasemap == true) {
        if (basemap == 0) {
            wxSize sz = GetVirtualSize();
            int screenW = sz.GetWidth();
            int screenH = sz.GetHeight();
            OGRCoordinateTransformation *poCT = NULL;
            
            if (project->sourceSR != NULL) {
                int nGCS = project->sourceSR->GetEPSGGeogCS();
                if (nGCS != 4326) {
                    OGRSpatialReference destSR;
                    destSR.importFromEPSG(4326);
                    poCT = OGRCreateCoordinateTransformation(project->sourceSR,
                                                             &destSR);
                }
            }
            
            GDA::Screen* screen = new GDA::Screen(screenW, screenH);
            GDA::MapLayer* map = new GDA::MapLayer(shps_orig_ymax, shps_orig_xmin,
                                                   shps_orig_ymin, shps_orig_xmax,
                                                   poCT);
            if (poCT == NULL && !map->IsWGS84Valid()) {
                isDrawBasemap = false;
                return false;
            } else {
                basemap = new GDA::Basemap(screen, map, map_type,
                                           GenUtils::GetBasemapCacheDir(),
                                           poCT);
            }
            ResizeSelectableShps();
        } else {
            basemap->SetupMapType(map_type);
        }
        
    } else {
        // isDrawBasemap == false
        if (basemap)
            basemap->mapType=0;
    }
    
    layerbase_valid = false;
    layer0_valid = false;
    layer1_valid = false;
    layer2_valid = false;
    
    DrawLayers();
    return true;
}

void MapCanvas::DrawLayers()
{
    if (layerbase_valid && layer2_valid && layer1_valid && layer0_valid)
        return;
    
    wxSize sz = GetVirtualSize();
    if (!layer0_bm)
        resizeLayerBms(sz.GetWidth(), sz.GetHeight());
    
    if (!layerbase_valid && isDrawBasemap)
        DrawLayerBase();
    
    if (!layer0_valid)
        DrawLayer0();
    
    if (!layer1_valid)
        DrawLayer1();
    
    if (!layer2_valid) {
        DrawLayer1();
        DrawLayer2();
    }
    
    wxWakeUpIdle();
    isRepaint = true;
    Refresh(true);
}

void MapCanvas::DrawLayerBase()
{
    if (isDrawBasemap) {
        if (basemap != 0) {
            layerbase_valid = basemap->Draw(basemap_bm);
#ifdef __linux__
            // trigger to draw again, since it's drawing on ONE bitmap, 
            // not multilayer with transparency support
            layer0_valid = false;	 
#endif
        }
    }
}


#ifdef __linux__
void MapCanvas::resizeLayerBms(int width, int height)
{
	deleteLayerBms();
	basemap_bm = new wxBitmap(width, height);
	layer0_bm = new wxBitmap(width, height);
	layer1_bm = new wxBitmap(width, height);
	layer2_bm = new wxBitmap(width, height);
	final_bm = new wxBitmap(width, height);

	layerbase_valid = false;	
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
}

void MapCanvas::DrawLayer0()
{
    //LOG_MSG("In TemplateCanvas::DrawLayer0");
    wxSize sz = GetVirtualSize();
    wxMemoryDC dc(*layer0_bm);

    dc.SetPen(canvas_background_color);
    dc.SetBrush(canvas_background_color);
    dc.DrawRectangle(wxPoint(0,0), sz);

    if (isDrawBasemap)
		dc.DrawBitmap(*basemap_bm, 0, 0);

    BOOST_FOREACH( GdaShape* shp, background_shps ) {
        shp->paintSelf(dc);
    }
    if (draw_sel_shps_by_z_val) {
        DrawSelectableShapesByZVal(dc);
    } else {
        DrawSelectableShapes(dc);
    }
    
    layer0_valid = true;
    layer1_valid = false;
}

// in Linux, following 3 functions will be inherited from TemplateCanvas
//void MapCanvas::DrawLayer1()
//void MapCanvas::DrawLayer2()
//void MapCanvas::OnPaint(wxPaintEvent& event)

#else
void MapCanvas::resizeLayerBms(int width, int height)
{
	deleteLayerBms();
	basemap_bm = new wxBitmap(width, height);
	layer0_bm = new wxBitmap(width, height, 32);
	layer1_bm = new wxBitmap(width, height, 32);
	layer2_bm = new wxBitmap(width, height, 32);
	final_bm = new wxBitmap(width, height);
	layer0_bm->UseAlpha();
	layer1_bm->UseAlpha();
	layer2_bm->UseAlpha();
	
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
}

// Draw all solid background, background decorations and unhighlighted shapes.
void MapCanvas::DrawLayer0()
{
	//LOG_MSG("In TemplateCanvas::DrawLayer0");
	wxSize sz = GetVirtualSize();
    if (layer0_bm) {
        delete layer0_bm;
        layer0_bm = NULL;
    }
    layer0_bm = new wxBitmap(sz.GetWidth(), sz.GetHeight(), 32);
	layer0_bm->UseAlpha();
	wxMemoryDC dc(*layer0_bm);
   
    dc.SetBackground( *wxTRANSPARENT_BRUSH );
    dc.Clear();

	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
    
	if (draw_sel_shps_by_z_val) {
		DrawSelectableShapesByZVal(dc);
	} else {
		DrawSelectableShapes(dc);
	}
	
	layer0_valid = true;
}


// Copy in layer0_bm
// draw highlighted shapes.
void MapCanvas::DrawLayer1()
{
    // recreate highlight layer
	wxSize sz = GetVirtualSize();
    if (layer1_bm) {
        delete layer1_bm;
        layer1_bm = NULL;
    }
    layer1_bm = new wxBitmap(sz.GetWidth(), sz.GetHeight(), 32);
    layer1_bm->UseAlpha();

	wxMemoryDC dc(*layer1_bm);
    dc.SetBackground( *wxTRANSPARENT_BRUSH );
    dc.Clear();
    
	if (!draw_sel_shps_by_z_val)
        DrawHighlightedShapes(dc);
    
	layer1_valid = true;
}

void MapCanvas::DrawLayer2()
{
	wxSize sz = GetVirtualSize();

    if (layer2_bm) {
        delete layer2_bm;
        layer2_bm = NULL;
    }
    layer2_bm = new wxBitmap(sz.GetWidth(), sz.GetHeight(),32 );
    layer2_bm->UseAlpha();

	wxMemoryDC dc(*layer2_bm);
    dc.SetBackground( *wxTRANSPARENT_BRUSH );
    dc.Clear();
    
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
		shp->paintSelf(dc);
	}
	layer2_valid = true;
}

void MapCanvas::OnPaint(wxPaintEvent& event)
{
    if (layer2_bm) {
    	wxSize sz = GetClientSize();
        
        wxMemoryDC dc(*final_bm);
        dc.SetBackground(canvas_background_color);
        dc.Clear();
        
        if (isDrawBasemap) {
            dc.DrawBitmap(*basemap_bm, 0, 0, true);
        }
        
        dc.DrawBitmap(*layer0_bm, 0, 0, true);
        dc.DrawBitmap(*layer1_bm, 0, 0, true);
        dc.DrawBitmap(*layer2_bm, 0, 0, true);

        
    	wxPaintDC paint_dc(this);
        // the following line cause flicking on windows machine
    	// paint_dc.Clear();
        
    	paint_dc.Blit(0, 0, sz.x, sz.y, &dc, 0, 0);
    	
    	// Draw the the selection region "the black selection box" if needed
    	PaintSelectionOutline(paint_dc);
    	
    	// Draw optional control objects if needed, should be in memeory
    	// PaintControls(paint_dc);
    	
    	// The resize event will ruin the position of scroll bars, so we reset the
    	// position of scroll bars again.
    	//if (prev_scroll_pos_x > 0) SetScrollPos(wxHORIZONTAL, prev_scroll_pos_x);
    	//if (prev_scroll_pos_y > 0) SetScrollPos(wxVERTICAL, prev_scroll_pos_y);
        
        isRepaint = false;
    }
    event.Skip();
}
#endif


int MapCanvas::GetBasemapType()
{
    if (basemap) return basemap->mapType;
    return 0;
}

void MapCanvas::CleanBasemapCache()
{
    if (basemap) basemap->CleanCache();
}

void MapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering MapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	if (MapFrame* f = dynamic_cast<MapFrame*>(template_frame)) {
		f->OnActivate(ae);
	}
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);
	
	if (template_frame) {
		template_frame->UpdateContextMenuItems(optMenu);
		template_frame->PopupMenu(optMenu, pos + GetPosition());
		template_frame->UpdateOptionMenuItems();
	}
	LOG_MSG("Exiting MapCanvas::DisplayRightClickMenu");
}

void MapCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (size_t i=0, sz=GetNumVars(); i<sz; i++) {
		if (var_info[i].is_time_variant) {
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
				menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}
    menu->AppendSeparator();
    menu->Append(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}


void MapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
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
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_2"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_3"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_4"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_5"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_6"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_7"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_8"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_9"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_10"),
						(GetCcType() == CatClassification::equal_intervals)
								  && GetNumCats() == 10);
	
	// since XRCID is a macro, we can't make this into a loop
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_1"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_2"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_3"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_4"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 4);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_5"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 5);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_6"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 6);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_7"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 7);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_8"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 8);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_9"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 9);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_NATURAL_BREAKS_10"),
						(GetCcType() == CatClassification::natural_breaks)
								  && GetNumCats() == 10);
	
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_RAWRATE"),
								  smoothing_type == raw_rate);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
								  smoothing_type == excess_risk);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
								  smoothing_type == empirical_bayes);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
								  smoothing_type == spatial_rate);
    GeneralWxUtils::CheckMenuItem(menu,
								  XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
								  smoothing_type == spatial_empirical_bayes);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_MEAN_CENTERS"),
								  selectable_shps_type != points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_CENTROIDS"),
								  selectable_shps_type != points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_DISPLAY_VORONOI_DIAGRAM"),
								   selectable_shps_type == points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_EXPORT_VORONOI"),
								   selectable_shps_type == points);
	GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_SAVE_VORONOI_DUPS_TO_TABLE"),
								   voronoi_diagram_duplicates_exist);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_MEAN_CENTERS"),
								  display_mean_centers);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_CENTROIDS"),
								  display_centroids);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_VORONOI_DIAGRAM"),
								  display_voronoi_diagram);
}

wxString MapCanvas::GetCanvasTitle()
{
	wxString v;
	if (GetNumVars() == 1) v << GetNameWithTime(0);
	if (GetNumVars() == 2) {
		if (smoothing_type == raw_rate) {
			v << "Raw Rate ";
		} else if (smoothing_type == excess_risk) {
			// Excess Risk smoothing is a special case that comes with
			// its own theme.  See below.
			v << "";
		} else if (smoothing_type == empirical_bayes) {
			v << "EBS-Smoothed ";
		} else if (smoothing_type == spatial_rate) {
			v << "SRS-Smoothed ";
		} else if (smoothing_type == spatial_empirical_bayes) {
			v << "SEBS-Smoothed ";
		}
		v << GetNameWithTime(0) << " over " << GetNameWithTime(1);
	}
	
	wxString s;
	if (GetCcType() == CatClassification::excess_risk_theme) {
		// Excess Risk smoothing map is a special case in that it is a
		// type of smoothing, but is also its own theme.  Any theme associated
		// with Excess Risk is ignored
		s << "Excess Risk Map: " << v;
	}
	else if (GetCcType() == CatClassification::no_theme) {
		s << "Map - " << project->GetProjectTitle();
	} else if (GetCcType() == CatClassification::custom) {
		s << cat_classif_def.title << ": " << v;
	} else {
		s << CatClassification::CatClassifTypeToString(GetCcType());
		s << ": " << v;
	}
	
	return s;
}

wxString MapCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= GetNumVars()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;	
}

void MapCanvas::OnSaveCategories()
{
	wxString t_name;
	if (GetCcType() == CatClassification::custom) {
		t_name = cat_classif_def.title;
	} else {
		t_name = CatClassification::CatClassifTypeToString(GetCcType());
	}
	wxString label;
	label << t_name << " Categories";
	wxString title;
	title << "Save " << label;
    
    std::vector<bool> undefs(num_obs);
    for (int t=0; t<num_time_vals; t++) {
        for (int i=0; i<num_obs; i++) {
            undefs[i] = undefs[i] || data_undef[0][t][i];
        }
    }
	SaveCategories(title, label, "CATEGORIES", undefs);
}

void MapCanvas::NewCustomCatClassif()
{
	// Begin by asking for a variable if none yet chosen
    std::vector<std::vector<bool> > var_undefs(num_time_vals);
    
	if (var_info.size() == 0) {
		VariableSettingsDlg dlg(project, VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK)
            return;
        
		var_info.resize(1);
		data.resize(1);
		data_undef.resize(1);
		var_info[0] = dlg.var_info[0];
        
		table_int->GetColData(dlg.col_ids[0], data[0]);
		table_int->GetColUndefined(dlg.col_ids[0], data_undef[0]);
        
		VarInfoAttributeChange();
		cat_var_sorted.resize(num_time_vals);
        
		for (int t=0; t<num_time_vals; t++) {
			cat_var_sorted[t].resize(num_obs);
            var_undefs[t].resize(num_obs);
            
			for (int i=0; i<num_obs; i++) {
                int ts = t+var_info[0].time_min;
				cat_var_sorted[t][i].first = data[0][ts][i];
				cat_var_sorted[t][i].second = i;
                var_undefs[t][i] = var_undefs[t][i] || data_undef[0][ts][i];
			}
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	}
	
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def.cat_classif_type != CatClassification::custom)
    {
		CatClassification::ChangeNumCats(cat_classif_def.num_cats, cat_classif_def);
		std::vector<wxString> temp_cat_labels; // will be ignored
		CatClassification::SetBreakPoints(cat_classif_def.breaks,
										  temp_cat_labels,
										  cat_var_sorted[var_info[0].time],
                                          var_undefs[var_info[0].time],
										  cat_classif_def.cat_classif_type,
										  cat_classif_def.num_cats);
		int time = cat_data.GetCurrentCanvasTmStep();
		for (int i=0; i<cat_classif_def.num_cats; i++) {
			cat_classif_def.colors[i] = cat_data.GetCategoryColor(time, i);
			cat_classif_def.names[i] = cat_data.GetCategoryLabel(time, i);
		}
		int col = table_int->FindColId(var_info[0].name);
		int tm = var_info[0].time;
		cat_classif_def.assoc_db_fld_name = table_int->GetColName(col, tm);
	}
	
	CatClassifFrame* ccf = GdaFrame::GetGdaFrame()->GetCatClassifFrame(this->useScientificNotation);
    
	if (!ccf)
        return;
    
	CatClassifState* ccs = ccf->PromptNew(cat_classif_def, "",
                                          var_info[0].name,
                                          var_info[0].time);
    
	if (!ccs)
        return;
    
	if (custom_classif_state)
        custom_classif_state->removeObserver(this);
    
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

/** ChangeMapType should always have var_info and col_ids passed in to it
 when needed.  */

/** This method initializes data array according to values in var_info
 and col_ids.  It calls CreateAndUpdateCategories which does all of the
 category classification including any needed data smoothing. */
bool
MapCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme,
                         SmoothingType new_map_smoothing,
                         int num_categories_s,
                         boost::uuids::uuid weights_id_s,
                         bool use_new_var_info_and_col_ids,
                         const std::vector<GdaVarTools::VarInfo>& new_var_info,
                         const std::vector<int>& new_col_ids,
                         const wxString& custom_classif_title)
{
	// We only ask for variables when changing from no_theme or
	// smoothed (with theme).
	num_categories = num_categories_s;
	weights_id = weights_id_s;
	
	if (new_map_theme == CatClassification::custom) {
		new_map_smoothing = no_smoothing;
	}
	
	if (smoothing_type != no_smoothing && new_map_smoothing == no_smoothing) {
		wxString msg = _T("The new theme chosen will no longer include rates smoothing. Please use the Rates submenu to choose a theme with rates again.");
		wxMessageDialog dlg (this, msg, "Information", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	}
	
	if (new_map_theme == CatClassification::custom) {
		CatClassifManager* ccm = project->GetCatClassifManager();
		if (!ccm)
            return false;
		CatClassifState* new_ccs = ccm->FindClassifState(custom_classif_title);
		if (!new_ccs)
            return false;
		if (custom_classif_state == new_ccs)
            return false;
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
	
	if (new_map_smoothing == excess_risk) {
		new_map_theme = CatClassification::excess_risk_theme;
	}
	
	int new_num_vars = 1;
	if (new_map_smoothing != no_smoothing) {
		new_num_vars = 2;
	} else if (new_map_theme == CatClassification::no_theme) {
		new_num_vars = 0;
	}
	
	int num_vars = GetNumVars();
	
	if (new_num_vars == 0) {
		var_info.clear();
		if (template_frame)
            template_frame->ClearAllGroupDependencies();
        
	} else if (new_num_vars == 1) {
		if (num_vars == 0) {
			if (!use_new_var_info_and_col_ids)
                return false;
			var_info.resize(1);
			data.resize(1);
			data_undef.resize(1);
			var_info[0] = new_var_info[0];
            
			if (template_frame) {
				template_frame->AddGroupDependancy(var_info[0].name);
			}
			table_int->GetColData(new_col_ids[0], data[0]);
            table_int->GetColUndefined(new_col_ids[0], data_undef[0]);
            
		} else if (num_vars == 1) {
			if (use_new_var_info_and_col_ids) {
				var_info[0] = new_var_info[0];
				if (template_frame) {
					template_frame->AddGroupDependancy(var_info[0].name);
				}
				table_int->GetColData(new_col_ids[0], data[0]);
                table_int->GetColUndefined(new_col_ids[0], data_undef[0]);
			} // else reuse current variable settings and values
            
		} else { // num_vars == 2
			if (!use_new_var_info_and_col_ids)
                return false;
			var_info.resize(1);
			if (template_frame) {
				template_frame->ClearAllGroupDependencies();
			}
			data.resize(1);
            data_undef.resize(1);
			var_info[0] = new_var_info[0];
			if (template_frame) {
				template_frame->AddGroupDependancy(var_info[0].name);
			}
			table_int->GetColData(new_col_ids[0], data[0]);
            table_int->GetColUndefined(new_col_ids[0], data_undef[0]);
		}
	} else if (new_num_vars == 2) {
		// For Rates, new var_info and col_id vectors should
		// always be passed in and num_cateogries, new_map_theme and
		// new_map_smoothing are assumed to be valid.
		if (!use_new_var_info_and_col_ids)
            return false;
		
		var_info.clear();
		data.clear();
		var_info.resize(2);
		data.resize(2);
		data_undef.resize(2);
        
		if (template_frame) {
			template_frame->ClearAllGroupDependencies();
		}
		for (int i=0; i<2; i++) {
			var_info[i] = new_var_info[i];
			if (template_frame) {
				template_frame->AddGroupDependancy(var_info[i].name);
			}
			table_int->GetColData(new_col_ids[i], data[i]);
            table_int->GetColUndefined(new_col_ids[i], data_undef[i]);
		}
		if (new_map_smoothing == excess_risk) {
			new_map_theme = CatClassification::excess_risk_theme;
		}
	}
	
	if (new_map_theme != CatClassification::custom && custom_classif_state) {
		custom_classif_state->removeObserver(this);
		custom_classif_state = 0;
	}
	cat_classif_def.cat_classif_type = new_map_theme;
	smoothing_type = new_map_smoothing;
	VarInfoAttributeChange();	
	CreateAndUpdateCategories();
	PopulateCanvas();
	return true;
}

void MapCanvas::update(CatClassifState* o)
{
	LOG_MSG("In MapCanvas::update(CatClassifState*)");
	cat_classif_def = o->GetCatClassif();
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Refresh();
		}
	}
}

/** This method assumes that v1 is already set and valid.  It will
 recreate all canvas objects as needed and refresh the canvas.
 Assumes that CreateAndUpdateCategories has already been called.
 All data analysis will have been done in CreateAndUpdateCategories
 already. */
void MapCanvas::PopulateCanvas()
{
	LOG_MSG("Entering MapCanvas::PopulateCanvas");
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();

	int canvas_ts = cat_data.GetCurrentCanvasTmStep();
	if (!map_valid[canvas_ts]) full_map_redraw_needed = true;
	
	// Note: only need to delete selectable shapes if the map needs
	// to be resized.  Otherwise, just reuse.
	if (full_map_redraw_needed) {
		BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
		selectable_shps.clear();
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();

	if (map_valid[canvas_ts]) {		
		if (full_map_redraw_needed) {
			CreateSelShpsFromProj(selectable_shps, project);
			full_map_redraw_needed = false;
			
			if (selectable_shps_type == polygons &&
				(display_mean_centers || display_centroids)) {
				GdaPoint* p;
				wxPen cent_pen(wxColour(20, 20, 20));
				wxPen cntr_pen(wxColour(55, 55, 55));
				if (display_mean_centers) {
					const std::vector<GdaPoint*>& c = project->GetMeanCenters();
					for (int i=0; i<num_obs; i++) {
						p = new GdaPoint(*c[i]);
						p->setPen(cntr_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
				if (display_centroids) {
					const std::vector<GdaPoint*>& c = project->GetCentroids();
					for (int i=0; i<num_obs; i++) {
						p = new GdaPoint(*c[i]);
						p->setPen(cent_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
			}
			if (selectable_shps_type == points && display_voronoi_diagram) {
				GdaPolygon* p;
				const std::vector<GdaShape*>& polys =
					project->GetVoronoiPolygons();
				for (int i=0, num_polys=polys.size(); i<num_polys; i++) {
					p = new GdaPolygon(*(GdaPolygon*)polys[i]);
					background_shps.push_back(p);
				}
			}
		}
	} else {
		wxRealPoint cntr_ref_pnt(shps_orig_xmin +
								 (shps_orig_xmax-shps_orig_xmin)/2.0,
								 shps_orig_ymin+ 
								 (shps_orig_ymax-shps_orig_ymin)/2.0);
		GdaShapeText* txt_shp = new GdaShapeText(map_error_message[canvas_ts],
									 *GdaConst::medium_font, cntr_ref_pnt);
		background_shps.push_back(txt_shp);
	}

    ReDraw();
	//ResizeSelectableShps();
	
	LOG_MSG("Exiting MapCanvas::PopulateCanvas");
}

void MapCanvas::TimeChange()
{
	LOG_MSG("Entering MapCanvas::TimeChange");
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
	for (size_t i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	LOG_MSG("Exiting MapCanvas::TimeChange");
}

void MapCanvas::VarInfoAttributeChange()
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
	if (template_frame) {
		template_frame->SetDependsOnNonSimpleGroups(is_any_time_variant);
	}
	ref_var_index = -1;
	num_time_vals = 1;
	for (size_t i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	//GdaVarTools::PrintVarInfoVector(var_info);
}

/** Update Categories based on num_time_vals, num_categories and ref_var_index.
 This method populates cat_var_sorted from data array and performs any
 smoothing as needed, setting smoothing_valid vector as appropriate. */
void MapCanvas::CreateAndUpdateCategories()
{
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);

    for (int t=0; t<num_time_vals; t++)
        map_valid[t] = true;
    
	map_error_message.resize(num_time_vals);
    
	for (int t=0; t<num_time_vals; t++)
        map_error_message[t] = wxEmptyString;
	
	if (GetCcType() == CatClassification::no_theme) {
		 // 1 = #cats
		//CatClassification::ChangeNumCats(1, cat_classif_def);
		//cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
		//cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t,0, GdaConst::map_default_fill_colour);
			cat_data.SetCategoryLabel(t, 0, "");
			cat_data.SetCategoryCount(t, 0, num_obs);
			for (int i=0; i<num_obs; i++)
                cat_data.AppendIdToCategory(t, 0, i);
		}
		
		if (ref_var_index != -1) {
            int step = var_info[ref_var_index].time - var_info[ref_var_index].time_min;
			cat_data.SetCurrentCanvasTmStep(step);
		}
		return;
	}
	
	// Everything below assumes that GetCcType() != no_theme
	// We assume data has been initialized to correct data
	// for all time periods.
	
	double* P = 0;
	double* E = 0;
	double* smoothed_results = 0;
	//std::vector<bool> undef_res(smoothing_type == no_smoothing ? 0 : num_obs);
    
	if (smoothing_type != no_smoothing) {
		P = new double[num_obs];
		E = new double[num_obs];
		smoothed_results = new double[num_obs];
	}
	
	cat_var_sorted.resize(num_time_vals);
    std::vector<std::vector<bool> > cat_var_undef;
    
	for (int t=0; t<num_time_vals; t++) {
        
		//cat_var_sorted[t].resize(num_obs);
        
        std::vector<bool> undef_res(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            for (int j=0; j< data_undef.size(); j++) {
                undef_res[i] =  undef_res[i] || data_undef[j][t][i];
            }
        }
		
		if (smoothing_type != no_smoothing) {
            
            for (int i=0; i<num_obs; i++) {
                E[i] = data[0][var_info[0].time][i];
            }
            
			if (var_info[0].sync_with_global_time) {
				for (int i=0; i<num_obs; i++) {
					E[i] = data[0][t+var_info[0].time_min][i];
				}
			} else {
				for (int i=0; i<num_obs; i++) {
					E[i] = data[0][var_info[0].time][i];
				}
			}
			
			if (var_info[1].sync_with_global_time) {
				for (int i=0; i<num_obs; i++) {
					P[i] = data[1][t+var_info[1].time_min][i];
				}
			} else {
				for (int i=0; i<num_obs; i++) {
					P[i] = data[1][var_info[1].time][i];
				}
			}
			
            bool hasZeroBaseVal = false;
            std::vector<bool>& hs = highlight_state->GetHighlight();
            std::vector<bool> hs_backup = hs;
            
			for (int i=0; i<num_obs; i++) {
                if (undef_res[i])
                    continue;
                
                if (P[i] == 0) {
                    hasZeroBaseVal = true;
                    hs[i] = false;
                } else {
                    hs[i] = true;
                }
				if (P[i] <= 0) {
					map_valid[t] = false;
					map_error_message[t] = _T("Error: Base values contain non-positive numbers which will result in undefined values.");
					continue;
				}
			}
		
            if (hasZeroBaseVal) {
                wxString msg(_T("Base field has zero values. Do you want to save a subset of non-zeros as a new shape file? "));
                wxMessageDialog dlg (this, msg, "Warning", 
                                     wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
                if (dlg.ShowModal() == wxID_YES) {
                    ExportDataDlg exp_dlg(this, project, true);
                    exp_dlg.ShowModal();
                }
        		hs = hs_backup;
        		return;
            }
            hs = hs_backup;
            
			if (!map_valid[t])
                continue;
			
			if (smoothing_type == raw_rate) {
                GdaAlgs::RateSmoother_RawRate(num_obs, P, E,
                                              smoothed_results,
                                              undef_res);
			} else if (smoothing_type == excess_risk) {
				// Note: Excess Risk is a transformation, not a smoothing
                GdaAlgs::RateSmoother_ExcessRisk(num_obs, P, E,
                                                 smoothed_results,
                                                 undef_res);
			} else if (smoothing_type == empirical_bayes) {
                GdaAlgs::RateSmoother_EBS(num_obs, P, E,
                                          smoothed_results, undef_res);
			} else if (smoothing_type == spatial_rate) {
                GdaAlgs::RateSmoother_SRS(num_obs, project->GetWManInt(),
                                          weights_id, P, E,
                                          smoothed_results, undef_res);
			} else if (smoothing_type == spatial_empirical_bayes) {
                GdaAlgs::RateSmoother_SEBS(num_obs, project->GetWManInt(),
                                           weights_id, P, E,
                                           smoothed_results, undef_res);
			}
		
			for (int i=0; i<num_obs; i++) {
                cat_var_sorted[t].push_back(std::make_pair(smoothed_results[i],i));
			}
            
		} else {
			for (int i=0; i<num_obs; i++) {
                double val = data[0][t+var_info[0].time_min][i];
                cat_var_sorted[t].push_back(std::make_pair(val, i));
			}
		}
        
        cat_var_undef.push_back(undef_res);
	}
	
	if (smoothing_type != no_smoothing) {
		if (P) delete [] P;
		if (E) delete [] E;
		if (smoothed_results)
            delete [] smoothed_results;
	}

	// Sort each vector in ascending order
	for (int t=0; t<num_time_vals; t++) {
		if (map_valid[t]) { // only sort data with valid smoothing
			std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
					  Gda::dbl_int_pair_cmp_less);
		}
	}
	
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def);
	}
    
	cat_classif_def.color_scheme = CatClassification::GetColSchmForType(cat_classif_def.cat_classif_type);
    
	CatClassification::PopulateCatClassifData(cat_classif_def,
											  cat_var_sorted,
                                              cat_var_undef,
											  cat_data,
                                              map_valid,
											  map_error_message,
                                              this->useScientificNotation);

	if (ref_var_index != -1) {
		cat_data.SetCurrentCanvasTmStep(var_info[ref_var_index].time
										- var_info[ref_var_index].time_min);
	}
	int cnc = cat_data.GetNumCategories(cat_data.GetCurrentCanvasTmStep());
	CatClassification::ChangeNumCats(cnc, cat_classif_def);
}

void MapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In MapCanvas::TimeSyncVariableToggle");
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	
	VarInfoAttributeChange();
	// Strictly speaking, should not have to repopulate map canvas
	// when time sync changes since scale of objects never changes. To keep
	// things simple, just redraw the whole canvas.
	CreateAndUpdateCategories();
	PopulateCanvas();
}

void MapCanvas::DisplayMeanCenters()
{
	full_map_redraw_needed = true;
	display_mean_centers = !display_mean_centers;
	PopulateCanvas();
}

void MapCanvas::DisplayCentroids()
{
	full_map_redraw_needed = true;
	display_centroids = !display_centroids;
	PopulateCanvas();
}

void MapCanvas::DisplayVoronoiDiagram()
{
	full_map_redraw_needed = true;
	display_voronoi_diagram = !display_voronoi_diagram;
	PopulateCanvas();
}

int MapCanvas::GetNumVars()
{
	return var_info.size();
}

int MapCanvas::GetNumCats()
{
	return num_categories;
}

CatClassification::CatClassifType MapCanvas::GetCcType()
{
	return cat_classif_def.cat_classif_type;
}

/** Save Rates option should only be available when 
 smoothing_type != no_smoothing */
void MapCanvas::SaveRates()
{
	if (smoothing_type == no_smoothing) {
		wxString msg;
		msg << "No rates currently calculated to save.";
		wxMessageDialog dlg (this, msg, "Information",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	
	std::vector<SaveToTableEntry> data(1);
	
    std::vector<bool> undefs(num_obs);
	std::vector<double> dt(num_obs);
    
	int t = cat_data.GetCurrentCanvasTmStep();
    for (int i=0; i<num_obs; i++) {
		dt[cat_var_sorted[t][i].second] = cat_var_sorted[t][i].first;
        undefs[i] = data_undef[0][t][i];
	}
    
	data[0].type = GdaConst::double_type;
	data[0].d_val = &dt;
	data[0].label = "Rate";
    data[0].undefined = &undefs;
	
	if (smoothing_type == raw_rate) {
		data[0].field_default = "R_RAW_RT";
	} else if (smoothing_type == excess_risk) {
		data[0].field_default = "R_EXCESS";
	} else if (smoothing_type == empirical_bayes) {
		data[0].field_default = "R_EBS";
	} else if (smoothing_type == spatial_rate) {
		data[0].field_default = "R_SPAT_RT";
	} else if (smoothing_type == spatial_empirical_bayes) {
		data[0].field_default = "R_SPAT_EBS";
	} else {
		return;
	}

	wxString title = "Save Rates - ";
	title << GetNameWithTime(0) << " over " << GetNameWithTime(1);
	
    SaveToTableDlg dlg(project, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
}

void MapCanvas::update(HLStateInt* o)
{
    TemplateCanvas::update(o);
}

void MapCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = 0;
	if (template_frame) {
		sb = template_frame->GetStatusBar();
	}
	if (!sb) 
        return;
	wxString s;
    s << "#obs=" << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
		s << "#selected=" << highlight_state->GetTotalHighlighted()<< "  ";
    }
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "hover obs " << hover_obs[0]+1;
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1;
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1;
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	}
	sb->SetStatusText(s);
}

MapNewLegend::MapNewLegend(wxWindow *parent, TemplateCanvas* t_canvas,
						   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
}

MapNewLegend::~MapNewLegend()
{
    LOG_MSG("In MapNewLegend::~MapNewLegend");
}

IMPLEMENT_CLASS(MapFrame, TemplateFrame)
BEGIN_EVENT_TABLE(MapFrame, TemplateFrame)
	EVT_ACTIVATE(MapFrame::OnActivate)	
END_EVENT_TABLE()

MapFrame::MapFrame(wxFrame *parent, Project* project,
                   const std::vector<GdaVarTools::VarInfo>& var_info,
                   const std::vector<int>& col_ids,
                   CatClassification::CatClassifType theme_type,
                   MapCanvas::SmoothingType smoothing_type,
                   int num_categories,
                   boost::uuids::uuid weights_id,
                   const wxPoint& pos, const wxSize& size,
                   const long style)
: TemplateFrame(parent, project, "Map", pos, size, style),
w_man_state(project->GetWManState())
{
	LOG_MSG("Entering MapFrame::MapFrame");

    
	int width, height;
	GetClientSize(&width, &height);
    
	wxSplitterWindow* splitter_win = 0;
	splitter_win = new wxSplitterWindow(this,-1, wxDefaultPosition, wxDefaultSize,
                                        wxSP_3D |wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
	wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new MapCanvas(rpanel, this, project,
                                    var_info, col_ids,
                                    theme_type, smoothing_type,
                                    num_categories,
                                    weights_id,
                                    wxDefaultPosition,
                                    wxDefaultSize);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizerAndFit(rbox);

    wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new MapNewLegend(lpanel, template_canvas, wxPoint(0,0), wxSize(0,0));
    wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND | wxALL);
    lpanel->SetSizerAndFit(lbox);
    
	splitter_win->SplitVertically(lpanel, rpanel, GdaConst::map_default_legend_width);
    
    wxPanel* toolbar_panel = new wxPanel(this,-1, wxDefaultPosition);
	wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    wxToolBar* tb = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
	toolbar_sizer->Add(tb, 0, wxEXPAND|wxALL);
	toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL); 
	sizer->Add(splitter_win, 1, wxEXPAND|wxALL); 
	SetSizer(sizer);
    SetAutoLayout(true);
    DisplayStatusBar(true);
	
	w_man_state->registerObserver(this);

	Show(true);
	LOG_MSG("Exiting MapFrame::MapFrame");
}

MapFrame::MapFrame(wxFrame *parent, Project* project,
                   const wxPoint& pos, const wxSize& size,
                   const long style)
: TemplateFrame(parent, project, "Map", pos, size, style),
w_man_state(project->GetWManState())
{
	LOG_MSG("Entering MapFrame::MapFrame");
	w_man_state->registerObserver(this);
	LOG_MSG("Exiting MapFrame::MapFrame");
}

MapFrame::~MapFrame()
{
	LOG_MSG("In MapFrame::~MapFrame");
	if (w_man_state) {
		w_man_state->removeObserver(this);
		w_man_state = 0;
	}
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void MapFrame::CleanBasemap()
{
    ((MapCanvas*)template_canvas)->CleanBasemapCache();
}

void MapFrame::SetupToolbar()
{
	Connect(XRCID("ID_SELECT_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapSelect));
	Connect(XRCID("ID_SELECT_INVERT"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapInvertSelect));
	Connect(XRCID("ID_PAN_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapPan));
	Connect(XRCID("ID_ZOOM_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapZoom));
	Connect(XRCID("ID_ZOOM_OUT_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapZoomOut));
	Connect(XRCID("ID_EXTENT_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapExtent));
	Connect(XRCID("ID_REFRESH_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapRefresh));
	//Connect(XRCID("ID_BRUSH_LAYER"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapBrush));
	Connect(XRCID("ID_TOOLBAR_BASEMAP"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MapFrame::OnMapBasemap));
}

void MapFrame::OnDrawBasemap(bool flag, int map_type)
{
	if (!template_canvas) return;

    bool drawSuccess = ((MapCanvas*)template_canvas)->DrawBasemap(flag, map_type);
    
    if (drawSuccess==false) {
        wxMessageBox("GeoDa cannot find proper projection or geographic coordinate system information to add a basemap. Please update this information (e.g. in .prj file).");
    }
}

void MapFrame::OnMapSelect(wxCommandEvent& e)
{
    /*
    // code reserved for enable/disable toolbar buttons
    TemplateCanvas::MouseMode mousemode = template_canvas->GetMouseMode();
	if (mousemode == select) {
	} else if (mousemode == pan) {
	} else if (mousemode == zoom) {
	} else { // default
	}
    //EnableTool(XRCID("ID_NEW_PROJECT"), !proj_open);
    */
    OnSelectionMode(e);
}

void MapFrame::OnMapInvertSelect(wxCommandEvent& e)
{
	HighlightState& hs = *project->GetHighlightState();
	hs.SetEventType(HLStateInt::invert);
	hs.notifyObservers();
}

void MapFrame::OnMapPan(wxCommandEvent& e)
{
    OnPanMode(e);
}
void MapFrame::OnMapZoom(wxCommandEvent& e)
{
    OnZoomMode(e);
}
void MapFrame::OnMapZoomOut(wxCommandEvent& e)
{
    OnZoomOutMode(e);
}
void MapFrame::OnMapExtent(wxCommandEvent& e)
{
    //OnFitToWindowMode(e);
    OnResetMap(e);
}
void MapFrame::OnMapRefresh(wxCommandEvent& e)
{
    OnRefreshMap(e);
}
//void MapFrame::OnMapBrush(wxCommandEvent& e)
//{
//}
void MapFrame::OnMapBasemap(wxCommandEvent& e)
{
    
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_BASEMAP_MENU");
	
    if (popupMenu) {
        // set checkmarks
        int idx = ((MapCanvas*) template_canvas)->GetBasemapType();
        
        popupMenu->FindItem(XRCID("ID_NO_BASEMAP"))->Check(idx==0);
        popupMenu->FindItem(XRCID("ID_BASEMAP_1"))->Check(idx==1);
        popupMenu->FindItem(XRCID("ID_BASEMAP_2"))->Check(idx==2);
        popupMenu->FindItem(XRCID("ID_BASEMAP_3"))->Check(idx==3);
        popupMenu->FindItem(XRCID("ID_BASEMAP_4"))->Check(idx==4);
        popupMenu->FindItem(XRCID("ID_BASEMAP_5"))->Check(idx==5);
        popupMenu->FindItem(XRCID("ID_BASEMAP_6"))->Check(idx==6);
        popupMenu->FindItem(XRCID("ID_BASEMAP_7"))->Check(idx==7);
        popupMenu->FindItem(XRCID("ID_BASEMAP_8"))->Check(idx==8);
        
        popupMenu->FindItem(XRCID("ID_CHANGE_TRANSPARENCY"))->Enable(idx!=0);
        
        PopupMenu(popupMenu, wxDefaultPosition);
    }
    
}

void MapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In MapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("MapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void MapFrame::MapMenus()
{
	LOG_MSG("In MapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu, project->GetCatClassifManager());
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void MapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        LOG_MSG("MapFrame::UpdateOptionMenuItems: Options menu not found");
	} else {
		((MapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void MapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void  MapFrame::update(TimeState* o)
{
	LOG_MSG("In MapFrame::update(TimeState* o)");
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Refresh();
}

/** Implementation of WeightsManStateObserver interface */
void MapFrame::update(WeightsManState* o)
{
	LOG_MSG("In MapFrame::update(WeightsManState*)");
	if (o->GetWeightsId() != 
		((MapCanvas*) template_canvas)->GetWeightsId()) return;
	if (o->GetEventType() == WeightsManState::name_change_evt) {
		UpdateTitle();
		return;
	}
	if (o->GetEventType() == WeightsManState::remove_evt) {
		Destroy();
	}
}

int MapFrame::numMustCloseToRemove(boost::uuids::uuid id) const
{
	return id == ((MapCanvas*) template_canvas)->GetWeightsId() ? 1 : 0;
}

void MapFrame::closeObserver(boost::uuids::uuid id)
{
	LOG_MSG("In MapFrame::closeObserver");
	if (numMustCloseToRemove(id) > 0) {
		((MapCanvas*) template_canvas)->SetWeightsId(boost::uuids::nil_uuid());
		if (w_man_state) {
			w_man_state->removeObserver(this);
			w_man_state = 0;
		}
		Close(true);
	}
}

void MapFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
    LOG_MSG("Entering TemplateFrame::OnCopyImageToClipboard");
    if (!template_canvas) return;
    wxSize sz = template_canvas->GetVirtualSize();
    
    wxBitmap bitmap( sz.x, sz.y );
    
    wxMemoryDC dc;
    dc.SelectObject( bitmap );

	dc.SetBrush(template_canvas->canvas_background_color);
    dc.DrawRectangle(wxPoint(0,0), sz);

    if (((MapCanvas*) template_canvas)->isDrawBasemap) {
        dc.DrawBitmap(*template_canvas->GetBaseLayer(), 0, 0, true);
    }
    dc.DrawBitmap(*template_canvas->GetLayer0(), 0, 0, true);
    dc.DrawBitmap(*template_canvas->GetLayer1(), 0, 0, true);
    dc.DrawBitmap(*template_canvas->GetLayer2(), 0, 0, true);

    dc.SelectObject( wxNullBitmap );
    
    if ( !wxTheClipboard->Open() ) {
        wxMessageBox("Can't open clipboard.");
    } else {
        wxTheClipboard->AddData(new wxBitmapDataObject(bitmap));
        wxTheClipboard->Close();
    }
    LOG_MSG("Exiting TemplateFrame::OnCopyImageToClipboard");
}

void MapFrame::ExportImage(TemplateCanvas* canvas, const wxString& type)
{
    LOG_MSG("Entering TemplateFrame::ExportImage");
    
    wxString default_fname(project->GetProjectTitle() + type);
	wxString filter("BMP|*.bmp|PNG|*.png");
    int filter_index = 1;
    //
    wxFileDialog dialog(canvas, "Save Image to File", wxEmptyString,
                        default_fname, filter,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    dialog.SetFilterIndex(filter_index);
    
    if (dialog.ShowModal() != wxID_OK) return;
    
    wxSize sz =  canvas->GetVirtualSize();
    
    wxFileName fname = wxFileName(dialog.GetPath());
    wxString str_fname = fname.GetPathWithSep() + fname.GetName();
    
    switch (dialog.GetFilterIndex()) {
        case 0:
		{
			LOG_MSG("BMP selected");
			wxBitmap bitmap( sz.x, sz.y );
			wxMemoryDC dc;
			dc.SelectObject(bitmap);

			//dc.SetBrush(template_canvas->canvas_background_color);
			//dc.DrawRectangle(wxPoint(0,0), sz);

			if (((MapCanvas*) template_canvas)->isDrawBasemap) {
                dc.DrawBitmap(*template_canvas->GetBaseLayer(), 0, 0, true);
            }
            dc.DrawBitmap(*template_canvas->GetLayer0(), 0, 0, true);
            dc.DrawBitmap(*template_canvas->GetLayer1(), 0, 0, true);
            dc.DrawBitmap(*template_canvas->GetLayer2(), 0, 0, true);
			dc.SelectObject( wxNullBitmap );
			
			wxImage image = bitmap.ConvertToImage();
			
			if ( !image.SaveFile( str_fname + ".bmp", wxBITMAP_TYPE_BMP )) {
				wxMessageBox("GeoDa was unable to save the file.");
			}			
			image.Destroy();
		}
			break;
        case 1:
        {
            LOG_MSG("PNG selected");
            wxBitmap bitmap( sz.x, sz.y );
            wxMemoryDC dc(bitmap);
            //dc.SelectObject(bitmap);

			dc.SetBrush(template_canvas->canvas_background_color);
			dc.DrawRectangle(wxPoint(0,0), sz);

            if (((MapCanvas*) template_canvas)->isDrawBasemap) {
                dc.DrawBitmap(*template_canvas->GetBaseLayer(), 0, 0, true);
            }
            dc.DrawBitmap(*template_canvas->GetLayer0(), 0, 0, true);
            dc.DrawBitmap(*template_canvas->GetLayer1(), 0, 0, true);
            dc.DrawBitmap(*template_canvas->GetLayer2(), 0, 0, true);
            //dc.SelectObject( wxNullBitmap );
            
            wxImage image = bitmap.ConvertToImage();
            
            if ( !image.SaveFile( str_fname + ".png", wxBITMAP_TYPE_PNG )) {
                wxMessageBox("GeoDa was unable to save the file.");
            }
            
            image.Destroy();
        }
            break;
            
        default:
        {
            LOG_MSG("Error: A non-recognized type selected.");
        }
            break;
    }
    return;
    
    LOG_MSG("Exiting MapFrame::ExportImage");
}


void MapFrame::OnNewCustomCatClassifA()
{
	((MapCanvas*) template_canvas)->NewCustomCatClassif();
}

void MapFrame::OnCustomCatClassifA(const wxString& cc_title)
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::custom, MapCanvas::no_smoothing, 4,
					  boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids, cc_title);
	} else {
		ChangeMapType(CatClassification::custom, MapCanvas::no_smoothing, 4,
					  boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0),
					  cc_title);
	}
}

void MapFrame::OnThemelessMap()
{
	ChangeMapType(CatClassification::no_theme, MapCanvas::no_smoothing, 1,
				  boost::uuids::nil_uuid(),
				  false, std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0));
}

void MapFrame::OnHinge15()
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::hinge_15, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::hinge_15, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapFrame::OnHinge30()
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::hinge_30, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::hinge_30, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapFrame::OnQuantile(int num_cats)
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::quantile, MapCanvas::no_smoothing,
					  num_cats, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::quantile, MapCanvas::no_smoothing,
					  num_cats, boost::uuids::nil_uuid(), false,
					  std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0));
	}
}

void MapFrame::OnPercentile()
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::percentile, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::percentile, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapFrame::OnStdDevMap()
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::stddev, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::stddev, MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0),
					  std::vector<int>(0));
	}
}

void MapFrame::OnUniqueValues()
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::unique_values,
					  MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::unique_values,
					  MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0),
					  std::vector<int>(0));
	}	
}

void MapFrame::OnNaturalBreaks(int num_cats)
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::natural_breaks,
					  MapCanvas::no_smoothing, num_cats,
					  boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::natural_breaks,
					  MapCanvas::no_smoothing, num_cats,
					  boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0));
	}
}

void MapFrame::OnEqualIntervals(int num_cats)
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
		VariableSettingsDlg dlg(project,
								VariableSettingsDlg::univariate);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::equal_intervals,
					  MapCanvas::no_smoothing, num_cats,
					  boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::equal_intervals,
					  MapCanvas::no_smoothing, num_cats,
					  boost::uuids::nil_uuid(),
					  false, std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0));
	}
}

void MapFrame::OnSaveCategories()
{
	((MapCanvas*) template_canvas)->OnSaveCategories();
}

void MapFrame::OnRawrate()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed, false,
													false,
							"Raw Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapCanvas::raw_rate, dlg.GetNumCategories(),
				  boost::uuids::nil_uuid(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnExcessRisk()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate, false, false,
							"Excess Risk Map Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(CatClassification::excess_risk_theme,
				  MapCanvas::excess_risk, 6, boost::uuids::nil_uuid(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnEmpiricalBayes()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed, false,
													false,
							"Empirical Bayes Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapCanvas::empirical_bayes, dlg.GetNumCategories(),
				  boost::uuids::nil_uuid(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnSpatialRate()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed, true,
													false,
							"Spatial Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapCanvas::spatial_rate, dlg.GetNumCategories(),
				  dlg.GetWeightsId(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnSpatialEmpiricalBayes()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed, true,
													false,
							"Empirical Spatial Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapCanvas::spatial_empirical_bayes,
				  dlg.GetNumCategories(), dlg.GetWeightsId(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnSaveRates()
{
	((MapCanvas*) template_canvas)->SaveRates();
}

bool MapFrame::ChangeMapType(CatClassification::CatClassifType new_map_theme,
								MapCanvas::SmoothingType new_map_smoothing,
								int num_categories,
								boost::uuids::uuid weights_id,
								bool use_new_var_info_and_col_ids,
								const std::vector<GdaVarTools::VarInfo>& new_var_info,
								const std::vector<int>& new_col_ids,
								const wxString& custom_classif_title)
{
	bool r=((MapCanvas*) template_canvas)->
		ChangeMapType(new_map_theme, new_map_smoothing, num_categories,
					  weights_id,
					  use_new_var_info_and_col_ids,
					  new_var_info, new_col_ids,
					  custom_classif_title);
	UpdateTitle();
	UpdateOptionMenuItems();
	if (template_legend) template_legend->Refresh();
	return r;
}

void MapFrame::OnDisplayMeanCenters()
{
	((MapCanvas*) template_canvas)->DisplayMeanCenters();
	UpdateOptionMenuItems();
}

void MapFrame::OnDisplayCentroids()
{
	((MapCanvas*) template_canvas)->DisplayCentroids();
	UpdateOptionMenuItems();
}

void MapFrame::OnDisplayVoronoiDiagram()
{
	((MapCanvas*) template_canvas)->DisplayVoronoiDiagram();
	((MapCanvas*) template_canvas)->voronoi_diagram_duplicates_exist =
		project->IsPointDuplicates();
	UpdateOptionMenuItems();
}

void MapFrame::OnExportVoronoi()
{
	project->ExportVoronoi();
	((MapCanvas*) template_canvas)->voronoi_diagram_duplicates_exist =
		project->IsPointDuplicates();
	UpdateOptionMenuItems();
}

void MapFrame::OnExportMeanCntrs()
{
	project->ExportCenters(true);
}

void MapFrame::OnExportCentroids()
{
	project->ExportCenters(false);
}

void MapFrame::OnSaveVoronoiDupsToTable()
{
	project->SaveVoronoiDupsToTable();
}

void MapFrame::OnChangeMapTransparency()
{
	if (!template_canvas) return;
    
    //show slider dialog
    if (template_canvas->isDrawBasemap) {
        SliderDialog sliderDlg(this, template_canvas);
        sliderDlg.ShowModal();
    }

}

void MapFrame::GetVizInfo(std::map<wxString, std::vector<int> >& colors)
{
	if (template_canvas) {
		template_canvas->GetVizInfo(colors);
	}
}

void MapFrame::GetVizInfo(wxString& shape_type, wxString& field_name, std::vector<wxString>& clrs, std::vector<double>& bins)
{
	if (template_canvas) {
        template_canvas->GetVizInfo(shape_type, clrs, bins);
        if (!((MapCanvas*) template_canvas)->var_info.empty()) {
            field_name = ((MapCanvas*) template_canvas)->var_info[0].name;
        }
	}
}
