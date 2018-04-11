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
#include <wx/dcgraph.h>
#include <wx/dcsvg.h>
#include <wx/filename.h>
#include <wx/time.h>

#include "CatClassifState.h"
#include "CatClassifManager.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/CatClassifDlg.h"
#include "../DialogTools/SelectWeightsDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../DialogTools/ConnectDatasourceDlg.h"
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

using namespace std;

wxWindowID ID_SLIDER = wxID_ANY;

IMPLEMENT_CLASS(SliderDialog, wxDialog)
BEGIN_EVENT_TABLE(SliderDialog, wxDialog)
END_EVENT_TABLE()

SliderDialog::SliderDialog(wxWindow * parent,
                           MapCanvas* _canvas,
                           wxWindowID id,
                           const wxString & caption,
                           const wxPoint & position,
                           const wxSize & size,
                           long style )
: wxDialog( parent, id, caption, position, size, style)
{
    
    wxLogMessage("Open SliderDialog");
    
    canvas = _canvas;
    
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);
    
    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // A text control for the user’s name
    ID_SLIDER = wxID_ANY;
    double trasp = (double)canvas->tran_unhighlighted / 255.0;
    int trasp_scale = 100 * trasp;

	wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
    slider = new wxSlider(this, ID_SLIDER, trasp_scale, 0, 100,
                          wxDefaultPosition, wxSize(200, -1),
                          wxSL_HORIZONTAL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"1.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);
    subSizer->Add(slider, 0, wxALIGN_CENTER_VERTICAL|wxALL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"0.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);

	boxSizer->Add(subSizer);
    wxString txt_transparency = wxString::Format(_("Current Transparency: %.2f"), 1.0 - trasp);
    
    slider_text = new wxStaticText(this,
                                   wxID_ANY,
                                   txt_transparency,
                                   wxDefaultPosition,
                                   wxSize(100, -1));
    boxSizer->Add(slider_text, 0, wxGROW|wxALL, 5);
    boxSizer->Add(new wxButton(this, wxID_CANCEL, _("Close")), 0, wxALIGN_CENTER|wxALL, 10);
    
    topSizer->Fit(this);
    
    slider->Bind(wxEVT_SLIDER, &SliderDialog::OnSliderChange, this);
}

SliderDialog::~SliderDialog()
{
    
}

void SliderDialog::OnSliderChange( wxCommandEvent & event )
{
    int val = event.GetInt();
    double trasp = 1.0 - val / 100.0;
    slider_text->SetLabel(wxString::Format(_("Current Transparency: %.2f"), trasp));
    //GdaConst::transparency_unhighlighted = (1-trasp) * 255;
    canvas->tran_unhighlighted = (1-trasp) * 255;
    canvas->ReDraw();
}

IMPLEMENT_CLASS(MapCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(MapCanvas, TemplateCanvas)
	// in Linux, windows using old paint function without transparency support
	EVT_PAINT(TemplateCanvas::OnPaint)
    EVT_IDLE(MapCanvas::OnIdle)
    EVT_SIZE(MapCanvas::OnSize)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	//EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	//EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
	//EVT_KEY_DOWN(TemplateCanvas::OnKeyDown)
END_EVENT_TABLE()

bool MapCanvas::has_thumbnail_saved = false;
bool MapCanvas::has_shown_empty_shps_msg = false;
std::vector<int> MapCanvas::empty_shps_ids(0);
std::map<int, bool> MapCanvas::empty_dict;

MapCanvas::MapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                     Project* project_s,
                     const vector<GdaVarTools::VarInfo>& var_info_s,
                     const vector<int>& col_ids_s,
                     CatClassification::CatClassifType theme_type,
                     SmoothingType smoothing_type_s,
                     int num_categories_s,
                     boost::uuids::uuid weights_id_s,
                     const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s, 
                 project_s->GetHighlightState(),
                 pos, size, true, true),
num_obs(project_s->GetNumRecords()),
p_datasource(project_s->GetDataSource()),
num_time_vals(1),
custom_classif_state(0), 
data(0),
s_data(0),
var_info(0),
table_int(project_s->GetTableInt()),
smoothing_type(no_smoothing),
is_rate_smoother(false), 
full_map_redraw_needed(true),
display_mean_centers(false), 
display_centroids(false),
display_weights_graph(false),
display_neighbors(false),
display_map_with_graph(true),
display_voronoi_diagram(false),
graph_color(GdaConst::conn_graph_outline_colour),
neighbor_color(GdaConst::conn_neighbor_outline_colour),
neighbor_fill_color(GdaConst::conn_neighbor_fill_colour),
weights_graph_thickness(1),
voronoi_diagram_duplicates_exist(false),
num_categories(num_categories_s), 
weights_id(weights_id_s),
basemap(0),
isDrawBasemap(false),
basemap_bm(0),
map_type(0),
ref_var_index(-1),
tran_unhighlighted(GdaConst::transparency_unhighlighted)
{
    wxLogMessage("MapCanvas::MapCanvas()");

    layer_name = project->layername;
    ds_name = project->GetDataSource()->GetOGRConnectStr();
    
    last_scale_trans.SetData(project->main_data.header.bbox_x_min,
                             project->main_data.header.bbox_y_min,
                             project->main_data.header.bbox_x_max,
                             project->main_data.header.bbox_y_max);
    
	selectable_fill_color = GdaConst::map_default_fill_colour;
	if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
		selectable_shps_type = points;
		highlight_color = *wxRED;
	} else {
		selectable_shps_type = polygons;
		highlight_color = GdaConst::map_default_highlight_colour;
	}
    
	use_category_brushes = true;
	cat_classif_def.cat_classif_type = theme_type;
	if (!ChangeMapType(theme_type, smoothing_type_s, num_categories,
					   weights_id, true, var_info_s, col_ids_s))
    {
		// The user possibly clicked cancel, so try again with themeless map
		vector<GdaVarTools::VarInfo> vi(0);
		vector<int> cids(0);
		ChangeMapType(CatClassification::no_theme, no_smoothing, 1,
					  boost::uuids::nil_uuid(), true, vi, cids);
	}
    
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
    
    isDrawBasemap = GdaConst::use_basemap_by_default;
    if (isDrawBasemap) {
        map_type = GdaConst::default_basemap_selection;
    }
}

MapCanvas::~MapCanvas()
{
    wxLogMessage("MapCanvas::~MapCanvas()");
	if (highlight_state)
        highlight_state->removeObserver(this);
    
	if (custom_classif_state)
        custom_classif_state->removeObserver(this);
    
    if (basemap != NULL) {
        delete basemap;
        basemap = NULL;
    }
    
    w_graph.clear();
}

int MapCanvas::GetEmptyNumber()
{
    return empty_shps_ids.size();
}

void MapCanvas::ResetEmptyFlag()
{
    empty_shps_ids.clear();
    empty_dict.clear();
    has_shown_empty_shps_msg = false;
}
        
void MapCanvas::SetupColor()
{
}

void MapCanvas::DetermineMouseHoverObjects(wxPoint pointsel)
{
    TemplateCanvas::DetermineMouseHoverObjects(pointsel);
    if (layer0_bm && display_neighbors && sel1.x==0 && sel1.y==0 && sel2.x==0 && sel2.y==0) {
        vector<bool>& hs = GetSelBitVec();
        for (int i=0; i<hs.size(); i++) hs[i] = false;
        if (hover_obs.empty()) {
            highlight_state->SetTotalHighlighted(0);
        } else {
            hs[hover_obs[0]] = true;
            highlight_state->SetTotalHighlighted(1);
        }
        HighlightState& h_state = *project->GetHighlightState();
        h_state.SetEventType(HLStateInt::delta);
        h_state.notifyObservers();
        layer1_valid = false;
        DrawLayers();
    }
}

void MapCanvas::SetPredefinedColor(const wxString& lbl, const wxColor& new_color)
{
    lbl_color_dict[lbl] = new_color;
}

void MapCanvas::UpdatePredefinedColor(const wxString& lbl, const wxColor& new_color)
{
    // update predefined color, if user change the color on legend pane
    map<wxString, wxColour>::iterator it;
    for (it =lbl_color_dict.begin(); it!=lbl_color_dict.end(); it++) {
        wxString predefined_lbl = it->first;
        if ( lbl.StartsWith(predefined_lbl)) {
            lbl_color_dict[predefined_lbl] = new_color;
        }
    }
}

void MapCanvas::AddNeighborsToSelection(GalWeight* gal_weights, wxMemoryDC &dc)
{
    if (gal_weights == NULL)
        return;
    
    int ts = cat_data.GetCurrentCanvasTmStep();
    int num_obs = project->GetNumRecords();
    HighlightState& hs = *project->GetHighlightState();
    std::vector<bool>& h = hs.GetHighlight();
    std::vector<bool> add_elem(gal_weights->num_obs, false);
    std::set<int>::iterator it;
    ids_of_nbrs.clear();
    ids_wo_nbrs.clear();
    
    for (int i=0; i<h.size(); i++) if (h[i]) ids_wo_nbrs.push_back(i);
    
    for (int i=0; i<gal_weights->num_obs; i++) {
        if (h[i]) {
            GalElement& e = gal_weights->gal[i];
            for (int j=0, jend=e.Size(); j<jend; j++) {
                int obs = e[j];
                if (!h[obs] && !add_elem[obs]) {
                    add_elem[obs] = true;
                    ids_of_nbrs.insert(obs);
                }
            }
        }
    }
    if (dc.IsOk()) {
        vector<bool> new_hs(num_obs, false);
        wxPen pen(neighbor_color);
        wxBrush brush(neighbor_fill_color);
        for (it=ids_of_nbrs.begin(); it!= ids_of_nbrs.end(); it++) {
            //h[*it] = true;
            new_hs[*it] = true;
            // draw new highlighted in dc
            if (display_neighbors) {
                selectable_shps[*it]->setPen(pen);
                if (neighbor_fill_color.Alpha() == 0)
                    selectable_shps[*it]->setBrush(*wxTRANSPARENT_BRUSH);
                else
                    selectable_shps[*it]->setBrush(brush);
                selectable_shps[*it]->paintSelf(dc);
            }
        }
        if (!display_map_with_graph) {
            for (int i=0; i<gal_weights->num_obs; i++) {
                if (h[i]) new_hs[i] = true;
            }
        }
        if (display_weights_graph) {
            bool hl_only = true;
            bool revert = false;
            bool crosshatch = false;
            bool is_print = false;
            helper_DrawSelectableShapes_dc(dc, new_hs, hl_only, revert, crosshatch, is_print);
        }
    }
}

void MapCanvas::OnSize(wxSizeEvent& event)
{
    if (!ids_of_nbrs.empty() && (display_neighbors || display_weights_graph)) {
        // in case of display neighbors and weights graph, to prevent adding nbrs again when resizing window
        HighlightState& hs = *project->GetHighlightState();
        std::vector<bool>& h = hs.GetHighlight();
        for (int i=0; i<h.size(); i++) {
            h[i] = false;
        }
        for (int i=0; i<ids_wo_nbrs.size(); i++) {
            h[ids_wo_nbrs[i]] = true;
        }
    }
    
    ResetBrushing();
    isResize = true;
    event.Skip();
}

void MapCanvas::deleteLayerBms()
{
    if (basemap_bm) {
        delete basemap_bm;
        basemap_bm = 0;
    }
    
    TemplateCanvas::deleteLayerBms();
}

void MapCanvas::ResetShapes()
{
    if (faded_layer_bm) {
        delete faded_layer_bm;
        faded_layer_bm = NULL;
    }
    if (isDrawBasemap) {
        basemap->Reset();
    }
    
    TemplateCanvas::ResetShapes();
}

void MapCanvas::ZoomShapes(bool is_zoomin)
{
    if (sel2.x == 0 && sel2.y==0) {
        return;
    }
    if (faded_layer_bm) {
        delete faded_layer_bm;
        faded_layer_bm = NULL;
    }
    
    if (isDrawBasemap) {
        basemap->Zoom(is_zoomin, sel2.x, sel2.y, sel1.x, sel1.y);
        ResizeSelectableShps();
        return;
    }
    
    TemplateCanvas::ZoomShapes(is_zoomin);
}

void MapCanvas::PanShapes()
{
    if (sel2.x == 0 && sel2.y==0)
        return;

    if (faded_layer_bm) {
        delete faded_layer_bm;
        faded_layer_bm = NULL;
    }
    
    if (isDrawBasemap) {
        int delta_x = sel2.x - sel1.x;
        int delta_y = sel2.y - sel1.y;
        if (delta_x !=0 && delta_y != 0) {
            basemap->Pan(sel1.x, sel1.y, sel2.x, sel2.y);
            ResizeSelectableShps();
        }
        return;
    }
    
    TemplateCanvas::PanShapes();
}

void MapCanvas::OnIdle(wxIdleEvent& event)
{
    if (isResize) {
        isResize = false;
        
        int cs_w=0, cs_h=0;
        GetClientSize(&cs_w, &cs_h);
        
        last_scale_trans.SetView(cs_w, cs_h);
        
        resizeLayerBms(cs_w, cs_h);
        
        if (isDrawBasemap) {
            if (basemap == 0)
                InitBasemap();
            if (basemap)
                basemap->ResizeScreen(cs_w, cs_h);
        }
        
        ResizeSelectableShps();
        
        event.RequestMore(); // render continuously, not only once on idle
    } 
    
    if (!layer2_valid || !layer1_valid || !layer0_valid ||
        (isDrawBasemap && !layerbase_valid) )
    {
        DrawLayers();
        event.RequestMore();
    }
}

void MapCanvas::ResizeSelectableShps(int virtual_scrn_w,
                                     int virtual_scrn_h)
{
    if (isDrawBasemap) {
        if ( virtual_scrn_w > 0 && virtual_scrn_h> 0) {
            basemap->ResizeScreen(virtual_scrn_w, virtual_scrn_h);
        }
        
        BOOST_FOREACH( GdaShape* ms, background_shps ) {
            if (ms)
                ms->projectToBasemap(basemap);
        }
        BOOST_FOREACH( GdaShape* ms, selectable_shps ) {
            if (ms)
                ms->projectToBasemap(basemap);
        }
        BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
            if (ms)
                ms->projectToBasemap(basemap);
        }
        if (!w_graph.empty() && display_weights_graph && boost::uuids::nil_uuid() != weights_id) {
            // this is for resizing window with basemap + connectivity graph
            for (int i=0; i<w_graph.size(); i++) {
                GdaPolyLine* e = w_graph[i];
                e->projectToBasemap(basemap);
            }
        }
        
        layerbase_valid = false;
        layer0_valid = false;
        layer1_valid = false;
        layer2_valid = false;
        return;
    }
    
    TemplateCanvas::ResizeSelectableShps(virtual_scrn_w, virtual_scrn_h);
}

bool MapCanvas::InitBasemap()
{
    if (basemap == 0) {
        wxSize sz = GetClientSize();
        int screenW = sz.GetWidth();
        int screenH = sz.GetHeight();

        OGRCoordinateTransformation *poCT = NULL;
        
        if (project->sourceSR != NULL) {
            int nGCS = project->sourceSR->GetEPSGGeogCS();
            //if (nGCS != 4326) {
                OGRSpatialReference destSR;
                destSR.importFromEPSG(4326);
                poCT = OGRCreateCoordinateTransformation(project->sourceSR,
                                                         &destSR);
            //}
        }
        
        GDA::Screen* screen = new GDA::Screen(screenW, screenH);
        double shps_orig_ymax = last_scale_trans.orig_data_y_max;
        double shps_orig_xmin = last_scale_trans.orig_data_x_min;
        double shps_orig_ymin = last_scale_trans.orig_data_y_min;
        double shps_orig_xmax = last_scale_trans.orig_data_x_max;
        GDA::MapLayer* map = new GDA::MapLayer(shps_orig_ymax,
                                               shps_orig_xmin,
                                               shps_orig_ymin,
                                               shps_orig_xmax,
                                               poCT);
        if (poCT == NULL && !map->IsWGS84Valid()) {
            isDrawBasemap = false;
            wxStatusBar* sb = 0;
            if (template_frame) {
                sb = template_frame->GetStatusBar();
                if (sb) {
                    wxString s = _("GeoDa cannot find proper projection or geographic coordinate system information to add a basemap. Please update this information (e.g. in .prj file).");
                    sb->SetStatusText(s);
                }
            }
            return false;
        } else {
            basemap = new GDA::Basemap(screen, map, map_type,
                                       GenUtils::GetBasemapCacheDir(),
                                       poCT);
        }
    }
    return true;
}

bool MapCanvas::DrawBasemap(bool flag, int map_type_)
{
    //ResetShapes();
    ResetBrushing();
    map_type = map_type_;
    isDrawBasemap = flag;
    
    wxSize sz = GetClientSize();
    int screenW = sz.GetWidth();
    int screenH = sz.GetHeight();
    
    if (isDrawBasemap == true) {
        if ( basemap == 0 && InitBasemap()  == false ) {
            ResizeSelectableShps();
            return false;
        } else {
            basemap->SetupMapType(map_type);
        }
    } else {
        if ( basemap ) {
            basemap->mapType=0;
        }
    }
    
    layerbase_valid = false;
    layer0_valid = false;
    layer1_valid = false;
    layer2_valid = false;
  
    ReDraw();
    return true;
}

void MapCanvas::resizeLayerBms(int width, int height)
{
	deleteLayerBms();
    
    int vs_w, vs_h;
    GetClientSize(&vs_w, &vs_h);
    
    if (vs_w <= 0) vs_w = 1;
    if (vs_h <=0 ) vs_h = 1;
    
	basemap_bm = new wxBitmap(vs_w, vs_h, 32);
    layerbase_valid = false;
    
    layer0_bm = new wxBitmap(vs_w, vs_h, 32);
    layer1_bm = new wxBitmap(vs_w, vs_h, 32);
    layer2_bm = new wxBitmap(vs_w, vs_h, 32);
    
    layer0_valid = false;
    layer1_valid = false;
    layer2_valid = false;
}

void MapCanvas::DrawLayers()
{
    if (!layerbase_valid && isDrawBasemap)
        DrawLayerBase();
    
    if (!layer0_valid)
        DrawLayer0();
    
    if (!layer1_valid) {
        DrawLayer1();
    }
    
    if (!layer2_valid) {
        DrawLayer2();
    }
    
    wxWakeUpIdle();
    
    Refresh();
}

void MapCanvas::DrawLayerBase()
{
    if (isDrawBasemap) {
        if (faded_layer_bm) {
            delete faded_layer_bm;
            faded_layer_bm = NULL;
        }
        if (basemap != 0) {
            layerbase_valid = basemap->Draw(basemap_bm);
            // trigger to draw again, since it's drawing on ONE bitmap,
            // not multilayer with transparency support
            layer0_valid = false;
        }
    }
}

void MapCanvas::DrawLayer0()
{
    wxMemoryDC dc;
    dc.SelectObject(*layer0_bm);
    dc.Clear();
    wxSize sz = dc.GetSize();
    
	if (isDrawBasemap || (highlight_state->GetTotalHighlighted()>0 &&
						  GdaConst::use_cross_hatching == false)) 
	{
        // use a special color for mask transparency: 244, 243, 242c
        wxColour maskColor(MASK_R, MASK_G, MASK_B);
        wxBrush maskBrush(maskColor);
        dc.SetBackground(maskBrush);
    } else {
        dc.SetPen(canvas_background_color);
        dc.SetBrush(canvas_background_color);
        dc.DrawRectangle(wxPoint(0,0), sz);
    }

    BOOST_FOREACH( GdaShape* shp, background_shps ) {
        shp->paintSelf(dc);
    }
    DrawSelectableShapes_dc(dc);
    
    dc.SelectObject(wxNullBitmap);
        
    layer0_valid = true;
    layer1_valid = false;
}

void MapCanvas::DrawLayer1()
{
    if (layer1_bm == NULL)
        return;
   
    wxMemoryDC dc(*layer1_bm);
    dc.Clear();
    wxSize sz = dc.GetSize();
    
    if (isDrawBasemap) {
        dc.DrawBitmap(*basemap_bm,0,0);
	}
    
    bool revert = GdaConst::transparency_highlighted < tran_unhighlighted;
    int  alpha_value = 255;
	bool mask_needed = false;
	bool draw_highlight = highlight_state->GetTotalHighlighted() > 0;

    
	if (isDrawBasemap) {
		mask_needed = true;
        alpha_value = tran_unhighlighted;
	}

    if (draw_highlight && GdaConst::use_cross_hatching == false)
    {
		mask_needed = true;
        alpha_value = revert ? GdaConst::transparency_highlighted : tran_unhighlighted;
	}
    
	if (mask_needed)
	{
        if (faded_layer_bm == NULL) {
			wxImage image = layer0_bm->ConvertToImage();
			if (!image.HasAlpha()) {
				image.InitAlpha();
			}
        
			unsigned char *alpha=image.GetAlpha();
			unsigned char* pixel_data = image.GetData();
			int n_pixel = image.GetWidth() * image.GetHeight();
        
			int pos = 0;
			for (int i=0; i< n_pixel; i++) {
				// check rgb
				pos = i * 3;
				if (pixel_data[pos] == MASK_R && 
					pixel_data[pos+1] == MASK_G && 
					pixel_data[pos+2] == MASK_B) 
				{
					alpha[i] = 0;
				} else {
					if (alpha[i] !=0 ) alpha[i] = alpha_value;
				}
			}
            
            faded_layer_bm = new wxBitmap(image);
        }
        dc.DrawBitmap(*faded_layer_bm,0,0);

		int hl_alpha_value = revert ? tran_unhighlighted : GdaConst::transparency_highlighted;

		if ( draw_highlight ) {
            if ( hl_alpha_value == 255 || GdaConst::use_cross_hatching) {
				DrawHighlightedShapes(dc, revert);
			} else {
				// draw a highlight with transparency
				wxBitmap map_hl_bm(sz.GetWidth(), sz.GetHeight());
				wxMemoryDC _dc;

				// use a special color for mask transparency: 244, 243, 242c
				wxColour maskColor(MASK_R, MASK_G, MASK_B);
				wxBrush maskBrush(maskColor);
				_dc.SetBackground(maskBrush);

				_dc.SelectObject(map_hl_bm);
				_dc.Clear();

				DrawHighlightedShapes(_dc, revert);

				_dc.SelectObject(wxNullBitmap);

				wxImage image = map_hl_bm.ConvertToImage();
				if (!image.HasAlpha()) {
					image.InitAlpha();
				}
				unsigned char* alpha_vals = image.GetAlpha();
				unsigned char* pixel_data = image.GetData();
				int n_pixel = image.GetWidth() * image.GetHeight();

				int pos = 0;
				for (int i=0; i< n_pixel; i++) {
					// check rgb
					pos = i * 3;
					if (pixel_data[pos] == MASK_R &&
						pixel_data[pos+1] == MASK_G &&
						pixel_data[pos+2] == MASK_B)
					{
						alpha_vals[i] = 0;
					} else {
						if (alpha_vals[i] !=0) alpha_vals[i] = hl_alpha_value;
					}
				}

				wxBitmap bm(image);
				dc.DrawBitmap(bm,0,0);
			}
		}

	} else {
        if (faded_layer_bm) {
            DrawLayer0();
            ResetFadedLayer();
        }
		dc.DrawBitmap(*layer0_bm, 0, 0);
        if (GdaConst::use_cross_hatching == true) {
            DrawHighlightedShapes(dc, revert);
        }
	}

    dc.SelectObject(wxNullBitmap);
    layer1_valid = true;
    layer2_valid = false;
}

void MapCanvas::SetWeightsId(boost::uuids::uuid id)
{
    weights_id = id;
    
    bool show_graph = display_weights_graph && boost::uuids::nil_uuid() != weights_id && !w_graph.empty();
    
    if (show_graph || display_neighbors) {
        full_map_redraw_needed = true;
        PopulateCanvas();
    }
}

// draw highlighted selectable shapes
void MapCanvas::DrawHighlightedShapes(wxMemoryDC &dc, bool revert)
{
    if (selectable_shps.size() == 0)
        return;
    
    vector<bool>& hs = highlight_state->GetHighlight();
    
    if (use_category_brushes) {
        bool highlight_only = true;
        DrawSelectableShapes_dc(dc, highlight_only, revert, GdaConst::use_cross_hatching);
        
    } else {
        for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
            if (hs[i] == !revert && _IsShpValid(i)) {
                selectable_shps[i]->paintSelf(dc);
            }
        }
    }
    
    bool show_graph = display_weights_graph && boost::uuids::nil_uuid() != weights_id && !w_graph.empty();
    
    if (show_graph || display_neighbors) {
        // draw neighbors of selection if needed
        WeightsManInterface* w_man_int = project->GetWManInt();
        GalWeight* gal_weights = w_man_int->GetGal(weights_id);
        AddNeighborsToSelection(gal_weights, dc);
    }
    
    if (show_graph) {
        // draw connectivity graph if needed
        const vector<GdaPoint*>& c = project->GetMeanCenters();
        if (highlight_state->GetTotalHighlighted() >0) {
            wxPen pen(graph_color, weights_graph_thickness);
            for (int i=0; i<w_graph.size(); i++) {
                GdaPolyLine* e = w_graph[i];
                if (hs[e->from]) {
                    e->setPen(pen);
                    e->paintSelf(dc);
                } else {
                    e->setPen(*wxTRANSPARENT_PEN);
                }
            }
        }
    }
}

void MapCanvas::DrawLayer2()
{
    if (layer2_bm == NULL)
        return;
    
    wxMemoryDC dc(*layer2_bm);
    dc.Clear();
    
    dc.DrawBitmap(*layer1_bm, 0, 0);
   
    if (display_weights_graph && boost::uuids::nil_uuid() != weights_id && highlight_state->GetTotalHighlighted()==0) {
        wxPen pen(graph_color, weights_graph_thickness);
        for (int i=0; i<w_graph.size(); i++) {
            w_graph[i]->setPen(pen);
            w_graph[i]->setBrush(*wxTRANSPARENT_BRUSH);
        }
    }
    BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
        shp->paintSelf(dc);
    }
    
    dc.SelectObject(wxNullBitmap);
    
    layer2_valid = true;
   
    if ( MapCanvas::has_thumbnail_saved == false) {
        if (isDrawBasemap && layerbase_valid == false) {
            return;
        } 
        CallAfter(&MapCanvas::SaveThumbnail);
    }
}

void MapCanvas::SaveThumbnail()
{
    if (MapCanvas::has_thumbnail_saved == false) {
        RecentDatasource recent_ds;
        
        if (layer_name == recent_ds.GetLastLayerName() &&
            !ds_name.EndsWith("samples.sqlite") &&
            !ds_name.Contains("geodacenter.github.io")) {
        
            wxImage image = layer2_bm->ConvertToImage();
            
            long current_time_sec = wxGetUTCTime();
            wxString file_name;
            file_name << current_time_sec << ".png";
            wxString file_path;
            file_path << GenUtils::GetSamplesDir() << file_name;
            bool su = image.SaveFile(file_path, wxBITMAP_TYPE_PNG );
            if (su) {
                recent_ds.UpdateLastThumb(file_name);
            }
            image.Destroy();
        }
        MapCanvas::has_thumbnail_saved = true;
    }
}

void MapCanvas::DrawSelectableShapes_dc(wxMemoryDC &_dc, bool hl_only, bool revert,
                                        bool use_crosshatch)
{
    if (!display_map_with_graph) return;
    
    vector<bool>& hs = highlight_state->GetHighlight();
#ifdef __WXOSX__
    wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetDefaultRenderer();
    wxGraphicsContext* gc= renderer->CreateContext (_dc);
    
    helper_DrawSelectableShapes_gc(*gc, hs, hl_only, revert, use_crosshatch);
#else
    helper_DrawSelectableShapes_dc(_dc, hs, hl_only, revert, use_crosshatch);
    
#endif
}

int MapCanvas::GetBasemapType()
{
    if (basemap)
        return basemap->mapType;
    return 0;
}

void MapCanvas::CleanBasemapCache()
{
    if (basemap)
        basemap->CleanCache();
}

void MapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	if (MapFrame* f = dynamic_cast<MapFrame*>(template_frame)) {
		f->OnActivate(ae);
	}
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	TemplateCanvas::AppendCustomCategories(optMenu,
										   project->GetCatClassifManager());
	SetCheckMarks(optMenu);

    GeneralWxUtils::EnableMenuItem(optMenu, XRCID("ID_SAVE_CATEGORIES"),
                                   GetCcType() != CatClassification::no_theme);
	if (template_frame) {
		template_frame->UpdateContextMenuItems(optMenu);
		template_frame->PopupMenu(optMenu, pos + GetPosition());
		template_frame->UpdateOptionMenuItems();
	}
}

void MapCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
    //wxLogMessage("MapCanvas::AddTimeVariantOptionsToMenu()");
    if (!is_any_time_variant){
        return;
    }
    
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	for (size_t i=0, sz=GetNumVars(); i<sz; i++) {
		if (var_info[i].is_time_variant) {
			wxString s;
			s << _("Synchronize ") << var_info[i].name << _(" with Time Control");
			wxMenuItem* mi = menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}
    menu->AppendSeparator();
    menu->Append(wxID_ANY, _("Time Variable Options"), menu1, _("Time Variable Options"));
}


void MapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.

    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"),
					GetCcType() == CatClassification::no_theme);

    //GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_MAPANALYSIS_THEMELESS"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_QUANTILE_SUBMENU"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_15"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_MAPANALYSIS_HINGE_30"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"), !IS_VAR_STRING);
    //GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_COND_VERT_UNIQUE_VALUES"), VERT_VAR_NUM);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_EQUAL_INTERVALS_SUBMENU"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_NATURAL_BREAKS_SUBMENU"), !IS_VAR_STRING);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A"), !IS_VAR_STRING);
    
    CatClassifManager* ccm = project->GetCatClassifManager();
    vector<wxString> titles;
    ccm->GetTitles(titles);
    for (size_t j=0; j<titles.size(); j++) {
        GeneralWxUtils::EnableMenuItem(menu, GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0 +j, !IS_VAR_STRING);
    }
   
    for (int i=1; i<=10; i++) {
        wxString str_xrcid;
        bool flag;
        str_xrcid = wxString::Format("ID_QUANTILE_%d", i);
        flag = GetCcType()==CatClassification::quantile && GetNumCats()==i;
        GeneralWxUtils::CheckMenuItem(menu, XRCID(str_xrcid), flag);
        
        str_xrcid = wxString::Format("ID_EQUAL_INTERVALS_%d", i);
        flag = GetCcType()==CatClassification::equal_intervals && GetNumCats()==i;
        GeneralWxUtils::CheckMenuItem(menu, XRCID(str_xrcid), flag);
        
        str_xrcid = wxString::Format("ID_NATURAL_BREAKS_%d", i);
        flag = GetCcType()==CatClassification::natural_breaks && GetNumCats()==i;
        GeneralWxUtils::CheckMenuItem(menu, XRCID(str_xrcid), flag);
    }
    
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
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_MAPANALYSIS_COLOCATION"),
					GetCcType() == CatClassification::colocation);
	
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_RAWRATE"),
								  smoothing_type == raw_rate);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
								  smoothing_type == excess_risk);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
								  smoothing_type == empirical_bayes);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
								  smoothing_type == spatial_rate);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
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
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_WEIGHTS_GRAPH"),
                                  display_weights_graph);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_ADD_NEIGHBORS_TO_SELECTION"),
                                  display_neighbors);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_HIDE_MAP_WITH_GRAPH"),
                                  !display_map_with_graph);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_THICKNESS_LIGHT"),
                                  weights_graph_thickness==0);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_THICKNESS_NORM"),
                                  weights_graph_thickness==1);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_THICKNESS_STRONG"),
                                  weights_graph_thickness==2);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_HIDE_MAP_WITH_GRAPH"),
                                  display_weights_graph);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_THICKNESS_LIGHT"),
                                  display_weights_graph);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_THICKNESS_NORM"),
                                  display_weights_graph);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_THICKNESS_STRONG"),
                                  display_weights_graph);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_WEIGHTS_GRAPH_COLOR"),
                                   display_weights_graph);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_CONN_NEIGHBOR_COLOR"),
                                   display_neighbors);
    GeneralWxUtils::EnableMenuItem(menu, XRCID("ID_CONN_NEIGHBOR_FILL_COLOR"),
                                   display_neighbors);
}

wxString MapCanvas::GetCanvasTitle()
{
    //wxLogMessage("MapCanvas::GetCanvasTitle()");
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
    //wxLogMessage("MapCanvas::GetNameWithTime()");
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
    wxLogMessage("MapCanvas::OnSaveCategories()");
	wxString t_name;
	if (GetCcType() == CatClassification::custom) {
		t_name = cat_classif_def.title;
	} else {
		t_name = CatClassification::CatClassifTypeToString(GetCcType());
	}
    
	if (data_undef.size()>0) {
		wxString label;
		label << t_name << _(" Categories");
		wxString title;
		title << _("Save ") << label;
		vector<bool> undefs(num_obs);
		for (int t=0; t<num_time_vals; t++) {
			for (int i=0; i<num_obs; i++) {
				undefs[i] = undefs[i] || data_undef[0][t][i];
			}
		}
		SaveCategories(title, label, "CATEGORIES", undefs);
	}
}

void MapCanvas::NewCustomCatClassif()
{
    wxLogMessage("MapCanvas::NewCustomCatClassif()");
	// Begin by asking for a variable if none yet chosen
    vector<vector<bool> > var_undefs;
    
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
    }
	
    VarInfoAttributeChange();
    
    var_undefs.resize(num_time_vals);
    
    for (int t=0; t<num_time_vals; t++) {
        var_undefs[t].resize(num_obs);
    }
    
    cat_var_sorted.resize(num_time_vals);
    if (IS_VAR_STRING) cat_str_var_sorted.resize(num_time_vals);
    
    for (int t=0; t<num_time_vals; t++) {
        cat_var_sorted[t].resize(num_obs);
        
        for (int i=0; i<num_obs; i++) {
            int ts = t+var_info[0].time_min;
            cat_var_sorted[t][i].first = data[0][ts][i];
            cat_var_sorted[t][i].second = i;
            var_undefs[t][i] = var_undefs[t][i] || data_undef[0][ts][i];
        }
        std::sort(cat_var_sorted[t].begin(), cat_var_sorted[t].end(),
                  Gda::dbl_int_pair_cmp_less);
    }

    if (var_info.empty())
        return;
    
	// Fully update cat_classif_def fields according to current
	// categorization state
	if (cat_classif_def.cat_classif_type != CatClassification::custom )
    {
		CatClassification::ChangeNumCats(cat_classif_def.num_cats, cat_classif_def);
		vector<wxString> temp_cat_labels; // will be ignored
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

    GdaFrame* gda_frame = GdaFrame::GetGdaFrame();
	CatClassifFrame* ccf = gda_frame->GetCatClassifFrame(this->useScientificNotation);
    
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
			template_frame->GetTemplateLegend()->Recreate();
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
                         const vector<GdaVarTools::VarInfo>& new_var_info,
                         const vector<int>& new_col_ids,
                         const wxString& custom_classif_title)
{
    wxLogMessage("MapCanvas::ChangeMapType()");
	// We only ask for variables when changing from no_theme or smoothed (with theme).
	num_categories = num_categories_s;
    if (!weights_id_s.is_nil()) {
        weights_id = weights_id_s;
    }
	
	if (new_map_theme == CatClassification::custom) {
		new_map_smoothing = no_smoothing;
	}
	
	if (smoothing_type != no_smoothing && new_map_smoothing == no_smoothing) {
		wxString msg = _T("The new theme chosen will no longer include rates smoothing. Please use the Rates submenu to choose a theme with rates again.");
		wxMessageDialog dlg (this, msg, _("Information"), wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
        return false;
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
            s_data.resize(1);
			data_undef.resize(1);
			var_info[0] = new_var_info[0];
            
			if (template_frame) {
				template_frame->AddGroupDependancy(var_info[0].name);
			}
            GdaConst::FieldType f_type = table_int->GetColType(new_col_ids[0]);
            IS_VAR_STRING = f_type == GdaConst::string_type;
            
            if (IS_VAR_STRING) table_int->GetColData(new_col_ids[0], s_data[0]);
			else table_int->GetColData(new_col_ids[0], data[0]);
            table_int->GetColUndefined(new_col_ids[0], data_undef[0]);
            
		} else if (num_vars == 1) {
			if (use_new_var_info_and_col_ids) {
				var_info[0] = new_var_info[0];
				if (template_frame) {
					template_frame->AddGroupDependancy(var_info[0].name);
				}
                GdaConst::FieldType f_type = table_int->GetColType(new_col_ids[0]);
                IS_VAR_STRING = f_type == GdaConst::string_type;
                
                if (IS_VAR_STRING) table_int->GetColData(new_col_ids[0], s_data[0]);
                else table_int->GetColData(new_col_ids[0], data[0]);
                
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

        // for rates, the variable has to be numeric
        IS_VAR_STRING = false;
        
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
    
    if (display_weights_graph && !w_graph.empty()) {
        full_map_redraw_needed = true;
    }
	PopulateCanvas();
    
    TemplateLegend* legend = template_frame->GetTemplateLegend();
    if (legend != NULL ) {
        legend->isDragDropAllowed = new_map_theme == CatClassification::unique_values;
    }
   
    CallAfter(&MapCanvas::show_empty_shps_msgbox);
    
    return true;
}

void MapCanvas::show_empty_shps_msgbox()
{
    if (!has_shown_empty_shps_msg && !empty_shps_ids.empty()) {
        CreateAndUpdateCategories();
        if (template_frame) {
            if (template_frame->GetTemplateLegend()) {
                template_frame->GetTemplateLegend()->Recreate();
            }
        }
        wxString msg = _("These are the row numbers of the records without location information.");
        wxString empty_shps_msg = _("row:\n");
    
        for (int i=0; i<empty_shps_ids.size(); i++) {
            empty_shps_msg << empty_shps_ids[i] + 1 << "\n";
        }
        ScrolledDetailMsgDialog *dlg = new ScrolledDetailMsgDialog(_("Warning"), msg, empty_shps_msg);
        dlg->Show(true);
        has_shown_empty_shps_msg = true;
    }
}

void MapCanvas::update(CatClassifState* o)
{
	wxLogMessage("In MapCanvas::update(CatClassifState*)");
	cat_classif_def = o->GetCatClassif();
	CreateAndUpdateCategories();
	PopulateCanvas();
	if (template_frame) {
		template_frame->UpdateTitle();
		if (template_frame->GetTemplateLegend()) {
			template_frame->GetTemplateLegend()->Recreate();
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
    
    w_graph.clear();

	if (map_valid[canvas_ts]) {		
		if (full_map_redraw_needed) {
			empty_shps_ids = CreateSelShpsFromProj(selectable_shps, project);
			full_map_redraw_needed = false;
			
			if (selectable_shps_type == polygons && (display_mean_centers || display_centroids || display_weights_graph))
            {
				GdaPoint* p;
				wxPen cent_pen(wxColour(20, 20, 20));
				wxPen cntr_pen(wxColour(55, 55, 55));
				if (display_mean_centers) {
					const vector<GdaPoint*>& c = project->GetMeanCenters();
					for (int i=0; i<num_obs; i++) {
						p = new GdaPoint(*c[i]);
						p->setPen(cntr_pen);
						p->setBrush(*wxTRANSPARENT_BRUSH);
						foreground_shps.push_back(p);
					}
				}
				if (display_centroids) {
					const vector<GdaPoint*>& c = project->GetCentroids();
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
				const vector<GdaShape*>& polys = project->GetVoronoiPolygons();
				for (int i=0, num_polys=polys.size(); i<num_polys; i++) {
					p = new GdaPolygon(*(GdaPolygon*)polys[i]);
					background_shps.push_back(p);
				}
			}
            if (display_weights_graph) {
                // use men centers to draw graph
                WeightsManInterface* w_man_int = project->GetWManInt();
                GalWeight* gal_weights = w_man_int->GetGal(weights_id);
                const vector<GdaPoint*>& c = project->GetCentroids();
                vector<bool>& hs = highlight_state->GetHighlight();
                GdaPolyLine* edge;
                std::set<int> w_nodes;
                wxPen pen(graph_color, weights_graph_thickness);
                for (int i=0; gal_weights && i<gal_weights->num_obs; i++) {
                    GalElement& e = gal_weights->gal[i];
                    for (int j=0, jend=e.Size(); j<jend; j++) {
                        int nbr = e[j];
                        if (i!=nbr) {
                            // connect i<->nbr
                            edge = new GdaPolyLine(c[i]->GetX(),c[i]->GetY(), c[nbr]->GetX(), c[nbr]->GetY());
                            edge->from = i;
                            edge->to = nbr;
                            edge->setPen(pen);
                            edge->setBrush(*wxTRANSPARENT_BRUSH);
                            foreground_shps.push_back(edge);
                            w_graph.push_back(edge);
                            w_nodes.insert(i);
                            w_nodes.insert(nbr);
                        }
                    }
                }
            }
		}
	} else {
		wxRealPoint cntr_ref_pnt = last_scale_trans.GetDataCenter();
		GdaShapeText* txt_shp = new GdaShapeText(map_error_message[canvas_ts],
                                                 *GdaConst::medium_font,
                                                 cntr_ref_pnt);
		background_shps.push_back(txt_shp);
	}

    ReDraw();
}

void MapCanvas::TimeChange()
{
    wxLogMessage("MapCanvas::TimeChange()");
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
	wxLogMessage("Exiting MapCanvas::TimeChange");
}

void MapCanvas::VarInfoAttributeChange()
{
    wxLogMessage("MapCanvas::VarInfoAttributeChange()");
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (size_t i=0; i<var_info.size(); i++) {
        if (var_info[i].is_time_variant) {
            is_any_time_variant = true;
        }
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
    wxLogMessage("MapCanvas::CreateAndUpdateCategories()");
	cat_var_sorted.clear();
	map_valid.resize(num_time_vals);

    for (int t=0; t<num_time_vals; t++) {
        map_valid[t] = true;
    }
    
	map_error_message.resize(num_time_vals);
    
    for (int t=0; t<num_time_vals; t++) {
        map_error_message[t] = wxEmptyString;
    }
    
    empty_dict.clear();
    for (int i=0; i<empty_shps_ids.size(); i++) {
        empty_dict[empty_shps_ids[i]] = true;
    }

	if (GetCcType() == CatClassification::no_theme) {
		 // 1 = #cats
		//CatClassification::ChangeNumCats(1, cat_classif_def);
		//cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
		//cat_classif_def.colors[0] = GdaConst::map_default_fill_colour;
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t,0, GdaConst::map_default_fill_colour);
			cat_data.SetCategoryLabel(t, 0, "");
            int num_valid_obs = num_obs - GetEmptyNumber();
			cat_data.SetCategoryCount(t, 0, num_valid_obs);
           
            if (empty_shps_ids.empty()) {
    			for (int i=0; i<num_obs; i++)
                    cat_data.AppendIdToCategory(t, 0, i);
            } else {
                for (int i=0; i<num_obs; i++) {
                    if (empty_dict.find(i) == empty_dict.end())
                        cat_data.AppendIdToCategory(t, 0, i);
                }
            }
		}
		
		if (ref_var_index != -1) {
            int step = var_info[ref_var_index].time - var_info[ref_var_index].time_min;
			cat_data.SetCurrentCanvasTmStep(step);
		}
        if (IS_VAR_STRING) IS_VAR_STRING = false;
		return;
	}
	
	// Everything below assumes that GetCcType() != no_theme
	// We assume data has been initialized to correct data
	// for all time periods.
	
	double* P = 0;
	double* E = 0;
	double* smoothed_results = 0;
    
	if (smoothing_type != no_smoothing) {
		P = new double[num_obs];
		E = new double[num_obs];
		smoothed_results = new double[num_obs];
	}
	
	cat_var_sorted.resize(num_time_vals);
    if (IS_VAR_STRING) cat_str_var_sorted.resize(num_time_vals);
    
    vector<vector<bool> > cat_var_undef;
    
	for (int t=0; t<num_time_vals; t++) {
		cat_var_sorted[t].clear();
        if (IS_VAR_STRING) cat_str_var_sorted[t].clear();
        
        vector<bool> undef_res(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            for (int j=0; j< data_undef.size(); j++) {
                if ( data_undef[j].size() > t ) {
                    undef_res[i] =  undef_res[i] || data_undef[j][t][i];
                }
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
            vector<bool>& hs = highlight_state->GetHighlight();
            vector<bool> hs_backup = hs;
            
			for (int i=0; i<num_obs; i++) {
                if (undef_res[i])
                    continue;
                
                if (P[i] == 0) {
                    undef_res[i] = true;
                    hasZeroBaseVal = true;
                    hs[i] = false;
                } else {
                    hs[i] = true;
                }
				if (P[i] <= 0) {
					//map_valid[t] = false;
					map_error_message[t] = _("Error: Base values contain non-positive numbers which will result in undefined values.");
					continue;
				}
			}
            hs = hs_backup;
            
			
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
            if (IS_VAR_STRING) {
                for (int i=0; i<num_obs; i++) {
                    wxString val = s_data[0][t+var_info[0].time_min][i];
                    cat_str_var_sorted[t].push_back(std::make_pair(val, i));
                }
            } else {
                for (int i=0; i<num_obs; i++) {
                    double val = data[0][t+var_info[0].time_min][i];
                    cat_var_sorted[t].push_back(std::make_pair(val, i));
                }
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
            if (IS_VAR_STRING == false)
                std::sort(cat_var_sorted[t].begin(),
                          cat_var_sorted[t].end(),
                          Gda::dbl_int_pair_cmp_less);
		}
	}
	
	if (cat_classif_def.cat_classif_type != CatClassification::custom) {
		CatClassification::ChangeNumCats(GetNumCats(), cat_classif_def);
        cat_classif_def.color_scheme = CatClassification::GetColSchmForType(cat_classif_def.cat_classif_type);
	}
    
    
    if (IS_VAR_STRING)
        CatClassification::PopulateCatClassifData(cat_classif_def,
                                                  cat_str_var_sorted,
                                                  cat_var_undef,
                                                  cat_data,
                                                  map_valid,
                                                  map_error_message,
                                                  this->useScientificNotation);
    else
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
    wxLogMessage("MapCanvas::TimeSyncVariableToggle()");
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
    wxLogMessage("MapCanvas::DisplayMeanCenters()");
	full_map_redraw_needed = true;
	display_mean_centers = !display_mean_centers;
	PopulateCanvas();
}

void MapCanvas::DisplayCentroids()
{
    wxLogMessage("MapCanvas::DisplayCentroids()");
	full_map_redraw_needed = true;
	display_centroids = !display_centroids;
	PopulateCanvas();
}

void MapCanvas::DisplayWeightsGraph()
{
    wxLogMessage("MapCanvas::DisplayWeightsGraph()");
    full_map_redraw_needed = true;
    display_weights_graph = !display_weights_graph;
    if (display_weights_graph) {
        display_neighbors = false;
        
    } else {
        display_map_with_graph = true;
    }
    PopulateCanvas();
}

void MapCanvas::DisplayNeighbors()
{
    wxLogMessage("MapCanvas::DisplayNeighbors()");
    full_map_redraw_needed = true;
    display_neighbors = !display_neighbors;
    if (display_neighbors) {
        display_map_with_graph = true;
        display_weights_graph = false;
    } else {
    }
    PopulateCanvas();
}

void MapCanvas::DisplayMapWithGraph()
{
    wxLogMessage("MapCanvas::DisplayMapWithGraph()");
    if (display_weights_graph) {
        full_map_redraw_needed = true;
        display_map_with_graph = !display_map_with_graph;
        PopulateCanvas();
    }
}

void MapCanvas::ChangeGraphThickness(int val)
{
    wxLogMessage("MapCanvas::ChangeGraphThickness()");
    if (display_weights_graph) {
        weights_graph_thickness = val;
        full_map_redraw_needed = true;
        PopulateCanvas();
    }
}

void MapCanvas::ChangeGraphColor()
{
    if (display_weights_graph) {
        graph_color = GeneralWxUtils::PickColor(this, graph_color);
        full_map_redraw_needed = true;
        PopulateCanvas();
    }
}

void MapCanvas::ChangeNeighborColor()
{
    if (display_neighbors) {
        neighbor_color = GeneralWxUtils::PickColor(this, neighbor_color);
        full_map_redraw_needed = true;
        PopulateCanvas();
    }
}

void MapCanvas::ChangeNeighborFillColor()
{
    if (display_neighbors) {
        neighbor_fill_color = GeneralWxUtils::PickColor(this, neighbor_fill_color);
        full_map_redraw_needed = true;
        PopulateCanvas();
    }
}

void MapCanvas::DisplayVoronoiDiagram()
{
    wxLogMessage("MapCanvas::DisplayVoronoiDiagram()");
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
    wxLogMessage("MapCanvas::SaveRates()");
	if (smoothing_type == no_smoothing) {
		wxString msg;
		msg << _("No rates currently calculated to save.");
		wxMessageDialog dlg (this, msg, _("Information"),
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	
	vector<SaveToTableEntry> data(1);
	
    vector<bool> undefs(num_obs);
	vector<double> dt(num_obs);
    
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

	wxString title = _("Save Rates - %s over %s");
    title = wxString::Format(title, GetNameWithTime(0), GetNameWithTime(1));
	
    SaveToTableDlg dlg(project, this, data, title,
					   wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
}

void MapCanvas::update(HLStateInt* o)
{
    if (layer2_bm) {
        ResetBrushing();
        
        if (draw_sel_shps_by_z_val) {
            // force a full redraw
            layer0_valid = false;
            return;
        }
        
        HLStateInt::EventType type = o->GetEventType();
        if (type == HLStateInt::transparency) {
            tran_unhighlighted = GdaConst::transparency_unhighlighted;
            ResetFadedLayer();
        }

        // re-paint highlight layer (layer1_bm)
        layer1_valid = false;
        DrawLayers();
        
        UpdateStatusBar();
    }
}

void MapCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = 0;
	if (template_frame) {
		sb = template_frame->GetStatusBar();
	}
	if (!sb) 
        return;
    
    std::vector<bool>& hl = highlight_state->GetHighlight();
	wxString s;
    
    int selected_cnt = 0;
    int selected_idx = 0;
    
    if (GetCcType() == CatClassification::no_theme)
        s << "#obs=" << project->GetNumRecordsNoneEmpty() <<" ";
    else
        s << "#obs=" << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
        if (GetCcType() == CatClassification::no_theme) {
            for (int i=0; i<hl.size(); i++) {
                if ( hl[i] && empty_dict.find(i) == empty_dict.end()) {
                    selected_cnt += 1;
                    selected_idx = i;
                }
            }
            s << "#selected=" << selected_cnt << "  ";
        } else {
            s << "#selected=" << highlight_state->GetTotalHighlighted()<< "  ";
        }
        
    }
    if ((display_neighbors || display_weights_graph) &&
        boost::uuids::nil_uuid() != weights_id )
    {
        WeightsManInterface* w_man_int = project->GetWManInt();
        GalWeight* gal_weights = w_man_int->GetGal(weights_id);
       
        long cid = -1;
        
        if (hover_obs.size() == 1)
            cid = hover_obs[0];
        else if (selected_cnt == 1) {
            cid = selected_idx;
        }
        
        if (cid >= 0) {
            GalElement& e = gal_weights->gal[cid];
            
            s << "obs " << w_man_int->RecNumToId(GetWeightsId(), cid);
            s << " has " << e.Size() << " neighbor";
            if (e.Size() != 1) s << "s";
            if (e.Size() > 0) {
                s << ": ";
                int n_cnt = 0;
                for (int j=0, jend=e.Size(); j<jend; j++) {
                    int obs = e[j];
                    ids_of_nbrs.insert(obs);
                    s << w_man_int->RecNumToId(GetWeightsId(), obs);
                    if (n_cnt+1 < e.Size()) s << ", ";
                    ++n_cnt;
                }
                if (e.Size() > 20) s << "...";
            } else {
                s << ".";
            }
        }
    } else {
        if (mousemode == select && selectstate == start) {
            if (hover_obs.size() >= 1) {
                s << "hover obs " << hover_obs[0]+1;
            }
            if (hover_obs.size() >= 2) {
                s << ", ";
                s << "obs " << hover_obs[1]+1;
            }
            if (hover_obs.size() >= 3) {
                s << ", ";
                s << "obs " << hover_obs[2]+1;
            }
            if (hover_obs.size() >= 4) {
                s << ", ...";
            }
        }
    }
	
	sb->SetStatusText(s);
}

MapNewLegend::MapNewLegend(wxWindow *parent, TemplateCanvas* t_canvas,
						   const wxPoint& pos, const wxSize& size)
: TemplateLegend(parent, t_canvas, pos, size)
{
    Connect(TemplateLegend::ID_CATEGORY_COLOR, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapNewLegend::OnCategoryColor));
}

MapNewLegend::~MapNewLegend()
{
    LOG_MSG("In MapNewLegend::~MapNewLegend");
}

void MapNewLegend::OnCategoryColor(wxCommandEvent& event)
{
    int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
    if (opt_menu_cat < 0 || opt_menu_cat >= num_cats) return;
    
    wxColour col = template_canvas->cat_data.GetCategoryColor(c_ts, opt_menu_cat);
    wxColourData data;
    data.SetColour(col);
    data.SetChooseFull(true);
    int ki;
    for (ki = 0; ki < 16; ki++) {
        wxColour colour(ki * 16, ki * 16, ki * 16);
        data.SetCustomColour(ki, colour);
    }
    
    wxColourDialog dialog(this, &data);
    dialog.SetTitle(_("Choose Cateogry Color"));
    if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        for (int ts=0; ts<template_canvas->cat_data.GetCanvasTmSteps(); ts++) {
            if (num_cats == template_canvas->cat_data.GetNumCategories(ts)) {
                wxColor new_color = retData.GetColour();
                template_canvas->cat_data.SetCategoryColor(ts, opt_menu_cat, new_color);
                wxString lbl = template_canvas->cat_data.GetCategoryLabel(ts, opt_menu_cat);
                MapCanvas* w = dynamic_cast<MapCanvas*>(template_canvas);
                if (w) {
                    w->UpdatePredefinedColor(lbl, new_color);
                }
            }
        }
        template_canvas->invalidateBms();
        template_canvas->Refresh();
        Refresh();
    }
}

IMPLEMENT_CLASS(MapFrame, TemplateFrame)
BEGIN_EVENT_TABLE(MapFrame, TemplateFrame)
	EVT_ACTIVATE(MapFrame::OnActivate)
    EVT_CLOSE(MapFrame::OnClose )
END_EVENT_TABLE()

MapFrame::MapFrame(wxFrame *parent, Project* project,
                   const vector<GdaVarTools::VarInfo>& var_info,
                   const vector<int>& col_ids,
                   CatClassification::CatClassifType theme_type,
                   MapCanvas::SmoothingType smoothing_type,
                   int num_categories,
                   boost::uuids::uuid weights_id,
                   const wxPoint& pos, const wxSize& size,
                   const long style)
: TemplateFrame(parent, project, "Map", pos, size, style),
w_man_state(project->GetWManState()), export_dlg(NULL),
no_update_weights(false)
{
	wxLogMessage("Open MapFrame.");

    template_legend = NULL;
    template_canvas = NULL;
  
    if (weights_id.is_nil()) {
        WeightsManInterface* w_man_int = project->GetWManInt();
        weights_id = w_man_int->GetDefault();
    }
    
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
    
    if (theme_type == CatClassification::unique_values) {
        template_legend->isDragDropAllowed = true;
    }
    
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
}

MapFrame::MapFrame(wxFrame *parent, Project* project,
                   const wxPoint& pos, const wxSize& size,
                   const long style)
: TemplateFrame(parent, project, "Map", pos, size, style),
w_man_state(project->GetWManState()), export_dlg(NULL)
{
	w_man_state->registerObserver(this);
}

MapFrame::~MapFrame()
{
	if (w_man_state) {
		w_man_state->removeObserver(this);
		w_man_state = 0;
	}
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
    
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
}

void MapFrame::CleanBasemap()
{
    ((MapCanvas*)template_canvas)->CleanBasemapCache();
}

void MapFrame::SetupToolbar()
{
	Connect(XRCID("ID_SELECT_LAYER"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapSelect));
	Connect(XRCID("ID_SELECT_INVERT"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapInvertSelect));
	Connect(XRCID("ID_PAN_LAYER"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapPan));
	Connect(XRCID("ID_ZOOM_LAYER"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapZoom));
	Connect(XRCID("ID_ZOOM_OUT_LAYER"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapZoomOut));
	Connect(XRCID("ID_EXTENT_LAYER"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapExtent));
	Connect(XRCID("ID_REFRESH_LAYER"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapRefresh));
	Connect(XRCID("ID_TOOLBAR_BASEMAP"), wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(MapFrame::OnMapBasemap));
}

void MapFrame::OnDrawBasemap(bool flag, int map_type)
{
	if (!template_canvas) return;

    bool drawSuccess = ((MapCanvas*)template_canvas)->DrawBasemap(flag, map_type);
    
    if (flag == false) {
        ((MapCanvas*)template_canvas)->tran_unhighlighted = GdaConst::transparency_unhighlighted;
    }
    
    if (drawSuccess==false) {
        wxMessageBox(_("GeoDa cannot find proper projection or geographic coordinate system information to add a basemap. Please update this information (e.g. in .prj file)."));
    }
}

void MapFrame::OnMapSelect(wxCommandEvent& e)
{
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
    wxLogMessage("In MapFrame::OnMapBasemap()");
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
        
        //popupMenu->FindItem(XRCID("ID_CHANGE_TRANSPARENCY"))->Enable(idx!=0);
        
        PopupMenu(popupMenu, wxDefaultPosition);
    }
}

void MapFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("MapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void MapFrame::MapMenus()
{
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
	template_canvas->TimeChange();
	UpdateTitle();
	if (template_legend) template_legend->Recreate();
}

/** Implementation of WeightsManStateObserver interface */
void MapFrame::update(WeightsManState* o)
{
    if (!no_update_weights && o->GetWeightsId() != ((MapCanvas*) template_canvas)->GetWeightsId()) {
        // add_evt
        ((MapCanvas*) template_canvas)->SetWeightsId(o->GetWeightsId());
        return;
    }
	if (o->GetEventType() == WeightsManState::name_change_evt) {
		UpdateTitle();
		return;
	}
	if (no_update_weights == true &&
        o->GetEventType() == WeightsManState::remove_evt) {
        Destroy();
	}
}

int MapFrame::numMustCloseToRemove(boost::uuids::uuid id) const
{
    if (no_update_weights == false)
        return 0;
    else {
        if (id == ((MapCanvas*) template_canvas)->GetWeightsId())
            return 1;
        else
            return 0;
    }
}

void MapFrame::closeObserver(boost::uuids::uuid id)
{
	if (numMustCloseToRemove(id) > 0) {
		((MapCanvas*) template_canvas)->SetWeightsId(boost::uuids::nil_uuid());
		if (w_man_state) {
			w_man_state->removeObserver(this);
			w_man_state = 0;
		}
		Close(true);
	}
}

GalWeight* MapFrame::checkWeights()
{
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return NULL;
        
    }
    boost::uuids::uuid w_id = w_man_int->GetDefault();
    
    GalWeight* gal_weights = w_man_int->GetGal(w_id);
    if (gal_weights== NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return NULL;
    }
    return gal_weights;
}

void MapFrame::OnDisplayWeightsGraph(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
  
    if (event.GetString() == "Connectivity")
        no_update_weights = true;
    
    ((MapCanvas*) template_canvas)->DisplayWeightsGraph();
    UpdateOptionMenuItems();
}

void MapFrame::OnAddNeighborToSelection(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
    
    if (event.GetString() == "Connectivity")
        no_update_weights = true;
    
    ((MapCanvas*) template_canvas)->DisplayNeighbors();
    UpdateOptionMenuItems();
}

void MapFrame::OnDisplayMapWithGraph(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
   
    ((MapCanvas*) template_canvas)->DisplayMapWithGraph();
    UpdateOptionMenuItems();
}

void MapFrame::OnChangeGraphThickness(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
    
    if (event.GetId() == XRCID("ID_WEIGHTS_GRAPH_THICKNESS_LIGHT"))
        ((MapCanvas*) template_canvas)->ChangeGraphThickness(0);
    else if (event.GetId() == XRCID("ID_WEIGHTS_GRAPH_THICKNESS_NORM"))
        ((MapCanvas*) template_canvas)->ChangeGraphThickness(1);
    else if (event.GetId() == XRCID("ID_WEIGHTS_GRAPH_THICKNESS_STRONG"))
        ((MapCanvas*) template_canvas)->ChangeGraphThickness(2);
    
    UpdateOptionMenuItems();
}

void MapFrame::OnChangeGraphColor(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
  
    ((MapCanvas*) template_canvas)->ChangeGraphColor();
    UpdateOptionMenuItems();
}

void MapFrame::OnChangeNeighborColor(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
    
    ((MapCanvas*) template_canvas)->ChangeNeighborColor();
    UpdateOptionMenuItems();
}

void MapFrame::OnChangeNeighborFillColor(wxCommandEvent& event)
{
    GalWeight* gal_weights = checkWeights();
    if (gal_weights == NULL)
        return;
    
    ((MapCanvas*) template_canvas)->ChangeNeighborFillColor();
    UpdateOptionMenuItems();
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
					  false, vector<GdaVarTools::VarInfo>(0), vector<int>(0),
					  cc_title);
	}
}

void MapFrame::OnThemelessMap()
{
	ChangeMapType(CatClassification::no_theme, MapCanvas::no_smoothing, 1,
				  boost::uuids::nil_uuid(),
				  false, vector<GdaVarTools::VarInfo>(0), vector<int>(0));
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
					  false, vector<GdaVarTools::VarInfo>(0),
					  vector<int>(0));
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
					  false, vector<GdaVarTools::VarInfo>(0),
					  vector<int>(0));
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
					  vector<GdaVarTools::VarInfo>(0), vector<int>(0));
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
					  false, vector<GdaVarTools::VarInfo>(0),
					  vector<int>(0));
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
					  false, vector<GdaVarTools::VarInfo>(0),
					  vector<int>(0));
	}
}

void MapFrame::OnUniqueValues()
{
	if (((MapCanvas*) template_canvas)->GetNumVars() != 1) {
        bool show_str_var = true;
		VariableSettingsDlg dlg(project, VariableSettingsDlg::univariate,
                                // default values
                                false,false,_("Variable Settings"),
                                "","","","",false,false,false,
                                show_str_var);
		if (dlg.ShowModal() != wxID_OK) return;
		ChangeMapType(CatClassification::unique_values,
					  MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  true, dlg.var_info, dlg.col_ids);
	} else {
		ChangeMapType(CatClassification::unique_values,
					  MapCanvas::no_smoothing,
					  6, boost::uuids::nil_uuid(),
					  false, vector<GdaVarTools::VarInfo>(0),
					  vector<int>(0));
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
					  false, vector<GdaVarTools::VarInfo>(0), vector<int>(0));
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
					  false, vector<GdaVarTools::VarInfo>(0), vector<int>(0));
	}
}

void MapFrame::OnSaveCategories()
{
	((MapCanvas*) template_canvas)->OnSaveCategories();
}

void MapFrame::OnRawrate()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed, false, false,
							_("Raw Rate Smoothed Variable Settings"),
							_("Event Variable"), _("Base Variable"));
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(dlg.GetCatClassifType(),
				  MapCanvas::raw_rate, dlg.GetNumCategories(),
				  boost::uuids::nil_uuid(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnExcessRisk()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate, false, false,
							_("Excess Risk Map Variable Settings"),
							_("Event Variable"), _("Base Variable"));
	if (dlg.ShowModal() != wxID_OK) return;
	ChangeMapType(CatClassification::excess_risk_theme,
				  MapCanvas::excess_risk, 6, boost::uuids::nil_uuid(),
				  true, dlg.var_info, dlg.col_ids);
}

void MapFrame::OnEmpiricalBayes()
{
	VariableSettingsDlg dlg(project, VariableSettingsDlg::rate_smoothed, false, false,
							_("Empirical Bayes Smoothed Variable Settings"),
							_("Event Variable"), _("Base Variable"));
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
							_("Spatial Rate Smoothed Variable Settings"),
							_("Event Variable"), _("Base Variable"));
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
							_("Empirical Spatial Rate Smoothed Variable Settings"),
							_("Event Variable"), _("Base Variable"));
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
								const vector<GdaVarTools::VarInfo>& new_var_info,
								const vector<int>& new_col_ids,
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
	if (template_legend) template_legend->Recreate();
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

void MapFrame::OnClose(wxCloseEvent& event)
{
    if (export_dlg) {
        export_dlg->Close();
    }
    event.Skip();
}

void MapFrame::OnExportVoronoi()
{
    if (project->ExportVoronoi()) {
        if (export_dlg != NULL) {
            export_dlg->Destroy();
            delete export_dlg;
        }
        export_dlg = new ExportDataDlg(this, project->voronoi_polygons, Shapefile::POLYGON, project);
        export_dlg->ShowModal();
        ((MapCanvas*) template_canvas)->voronoi_diagram_duplicates_exist = project->IsPointDuplicates();
        UpdateOptionMenuItems();
    }
}

void MapFrame::OnExportMeanCntrs()
{
    project->ExportCenters(true);
    if (export_dlg != NULL) {
        export_dlg->Destroy();
        delete export_dlg;
    }
    export_dlg = new ExportDataDlg(this, project->mean_centers, Shapefile::NULL_SHAPE, "COORD", project);
    
    export_dlg->ShowModal();
}

void MapFrame::OnExportCentroids()
{
	project->ExportCenters(false);
    if (export_dlg != NULL) {
        export_dlg->Destroy();
        delete export_dlg;
    }
    export_dlg = new ExportDataDlg(this, project->centroids, Shapefile::NULL_SHAPE, "COORD", project);
    
    export_dlg->ShowModal();
}

void MapFrame::OnSaveVoronoiDupsToTable()
{
	project->SaveVoronoiDupsToTable();
}

void MapFrame::OnChangeMapTransparency()
{
	if (!template_canvas) return;
    
    //show slider dialog
    MapCanvas* map_canvs_ref = (MapCanvas*) template_canvas;
    if (map_canvs_ref->isDrawBasemap) {
        SliderDialog sliderDlg(this, map_canvs_ref);
        sliderDlg.ShowModal();
    }
}

void MapFrame::GetVizInfo(map<wxString, vector<int> >& colors)
{
	if (template_canvas) {
		template_canvas->GetVizInfo(colors);
	}
}

void MapFrame::GetVizInfo(wxString& shape_type, wxString& field_name, vector<wxString>& clrs, vector<double>& bins)
{
	if (template_canvas) {
        template_canvas->GetVizInfo(shape_type, clrs, bins);
        if (!((MapCanvas*) template_canvas)->var_info.empty()) {
            field_name = ((MapCanvas*) template_canvas)->var_info[0].name;
        }
	}
}
