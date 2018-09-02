//
//  MapLayerTree.cpp
//  GeoDa
//
//  Created by Xun Li on 8/24/18.
//

#include <vector>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcbuffer.h>
#include <wx/colordlg.h>

#include "../DialogTools/SaveToTableDlg.h"
#include "../logger.h"
#include "../SpatialIndTypes.h"
#include "MapNewView.h"
#include "MapLayer.hpp"
#include "MapLayerTree.hpp"


IMPLEMENT_ABSTRACT_CLASS(MapTree, wxWindow)
BEGIN_EVENT_TABLE(MapTree, wxWindow)
EVT_MENU(XRCID("IDC_CHANGE_POINT_RADIUS"), MapTree::OnChangePointRadius)
EVT_MOUSE_EVENTS(MapTree::OnEvent)
END_EVENT_TABLE()

MapTree::MapTree(wxWindow *parent, MapCanvas* _canvas, const wxPoint& pos, const wxSize& size)
: wxWindow(parent, wxID_ANY, pos, size),
select_id(-1),
canvas(_canvas),
isLeftDown(false),
recreate_labels(true),
isDragDropAllowed(false)
{
    SetBackgroundColour(GdaConst::legend_background_color);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    px_switch = 30;
    px = 60;
    py = 40;
    leg_h = 15;
    leg_w = 20;
    leg_pad_x = 10;
    leg_pad_y = 5;
    
    current_map_title = canvas->GetCanvasTitle() + " (current map)";
    
    Init();
    Connect(wxEVT_PAINT, wxPaintEventHandler(MapTree::OnPaint));
}

MapTree::~MapTree()
{
}

void MapTree::Init()
{
    int w, h;
    GetClientSize(&w, &h);
    
    map_titles.clear();
    new_order.clear();
    
    bg_maps = canvas->GetBackgroundMayLayers();
    fg_maps = canvas->GetForegroundMayLayers();
    
    int n_maps = bg_maps.size() + fg_maps.size() + 1;
    h = n_maps * 25  + 60;
    SetSize(w, h);
    
    map<wxString, BackgroundMapLayer*>::iterator it;
    for (it=fg_maps.begin(); it!=fg_maps.end(); it++) {
        wxString lbl = it->first;
        map_titles.push_back(lbl);
    }
    map_titles.push_back(current_map_title);
    for (it=bg_maps.begin(); it!=bg_maps.end(); it++) {
        wxString lbl = it->first;
        map_titles.push_back(lbl);
    }
    
    for (int i=0; i<map_titles.size(); i++) {
        wxString lbl = map_titles[i];
        int x =  px;
        int y =  py + (leg_h + leg_pad_y) * i;
        wxPoint pt(x, py);
        wxSize sz(w - x, leg_h);
        new_order.push_back(i);
    }
    Refresh();
}

void MapTree::OnPaint( wxPaintEvent& event )
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    OnDraw(dc);
}

void MapTree::OnRemoveMapLayer(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = NULL;
    if (bg_maps.find(map_name) != bg_maps.end()) {
        ml = bg_maps[map_name];
        ml->CleanMemory();
        bg_maps.erase(map_name);
    } else if (fg_maps.find(map_name) != fg_maps.end()) {
        ml = fg_maps[map_name];
        ml->CleanMemory();
        fg_maps.erase(map_name);
    }
    
    int oid = new_order[select_id];
    map_titles.erase(map_titles.begin() + new_order[select_id]);
    new_order.erase(new_order.begin() + select_id);
    for (int i=0; i<new_order.size(); i++) {
        if (new_order[i] > oid) {
            new_order[i] -= 1;
        }
    }
    select_id = select_id > 0 ? select_id-1 : 0;
    Refresh();
    OnMapLayerChange();
}

void MapTree::OnSpatialJoinCount(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        // create rtree using points (normally, more points than polygons)
        rtree_pt_2d_t rtree;
        Shapefile::ShapeType shp_type = ml->GetShapeType();
        if (shp_type == Shapefile::POINT_TYP) {
            int n = ml->shapes.size();
            double x, y;
            for (int i=0; i<n; i++) {
                x = ml->shapes[i]->center_o.x;
                y = ml->shapes[i]->center_o.y;
                rtree.insert(std::make_pair(pt_2d(x,y), i));
            }
        }
        
        // for each polygon in map, query points
        Shapefile::Main& main_data = canvas->GetGeometryData();
        OGRLayerProxy* ogr_layer = canvas->GetOGRLayerProxy();
        Shapefile::PolygonContents* pc;
        int n_polygons = main_data.records.size();
        vector<wxInt64> spatial_counts(n_polygons, 0);
        for (int i=0; i<n_polygons; i++) {
            pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
            // create a box, tl, br
            box_2d b(pt_2d(pc->box[0], pc->box[1]),
                     pt_2d(pc->box[2], pc->box[3]));
            // query points in this box
            std::vector<pt_2d_val> q;
            rtree.query(bgi::within(b), std::back_inserter(q));
            OGRGeometry* ogr_poly = ogr_layer->GetGeometry(i);
            for (int j=0; j<q.size(); j++) {
                const pt_2d_val& v = q[j];
                double x = v.first.get<0>();
                double y = v.first.get<1>();
                OGRPoint ogr_pt(x, y);
                if (ogr_pt.Within(ogr_poly)) {
                    spatial_counts[i] += 1;
                }
            }
        }
        
        // save results
        int new_col = 1;
        std::vector<SaveToTableEntry> new_data(new_col);
        vector<bool> undefs(n_polygons, false);
        new_data[0].l_val = &spatial_counts;
        new_data[0].label = "Spatial Counts";
        new_data[0].field_default = "SC";
        new_data[0].type = GdaConst::long64_type;
        new_data[0].undefined = &undefs;
        SaveToTableDlg dlg(canvas->GetProject(), this, new_data,
                           "Save Results: Spatial Counts",
                           wxDefaultPosition, wxSize(400,400));
        dlg.ShowModal();
    }
}

void MapTree::OnChangeFillColor(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        wxColour clr;
        clr = wxGetColourFromUser(this, ml->GetBrushColour());
        ml->SetBrushColour(clr);
        Refresh();
        CallAfter(&MapCanvas::ReDraw);
    }
}

void MapTree::OnChangeOutlineColor(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        wxColour clr;
        clr = wxGetColourFromUser(this, ml->GetPenColour());
        ml->SetPenColour(clr);
        Refresh();
        CallAfter(&MapCanvas::ReDraw);
    }
}
void MapTree::OnChangePointRadius(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        int old_radius = ml->GetPointRadius();
        PointRadiusDialog dlg(_("Change Point Radius"), old_radius);
        if (dlg.ShowModal() == wxID_OK) {
            int new_radius = dlg.GetRadius();
            ml->SetPointRadius(new_radius);
            Refresh();
            CallAfter(&MapCanvas::ReDraw);
        }
    }
}
void MapTree::OnOutlineVisible(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        int pen_size = ml->GetPenSize();
        if (pen_size > 0) {
            // not show outline
            ml->SetPenSize(0);
        } else {
            // show outline
            ml->SetPenSize(1);
        }
        Refresh();
        CallAfter(&MapCanvas::ReDraw);
    }
}
void MapTree::OnShowMapBoundary(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        bool show_bnd = ml->IsShowBoundary();
        ml->ShowBoundary(!show_bnd);
        Refresh();
        CallAfter(&MapCanvas::ReDraw);
    }
}

void MapTree::OnEvent(wxMouseEvent& event)
{
    int cat_clicked = GetCategoryClick(event);
    if (event.RightUp()) {
        OnRightClick(event);
        return;
    }
    
    if (event.LeftDown()) {
        isLeftDown = true;
        select_id = cat_clicked;
        if (select_id > -1) {
            select_name = map_titles[new_order[select_id]];
            move_pos = event.GetPosition();
        }
        
    } else if (event.Dragging()) {
        if (isLeftDown) {
            isLeftMove = true;
            // moving
            if (select_id > -1 ) {
                // paint selected label with mouse
                int label_id = new_order[select_id];
                move_pos = event.GetPosition();
                if (cat_clicked > -1 && select_id != cat_clicked) {
                    // reorganize new_order
                    if (select_id > cat_clicked) {
                        new_order.insert(new_order.begin()+cat_clicked, new_order[select_id]);
                        new_order.erase(new_order.begin() + select_id + 1);
                    } else {
                        new_order.insert(new_order.begin()+cat_clicked+1, new_order[select_id]);
                        new_order.erase(new_order.begin() + select_id);
                    }
                    select_id = cat_clicked;
                }
            }
            Refresh();
        }
    } else if (event.LeftUp()) {
        if (isLeftMove) {
            isLeftMove = false;
            // stop move
            OnMapLayerChange();
            select_id = -1;
            select_name = "";
            Refresh();
        } else {
            // only left click
            OnSwitchClick(event);
        }
        isLeftDown = false;
    }
}

void MapTree::OnRightClick(wxMouseEvent& event)
{
    select_id = GetLegendClick(event);
    if (select_id < 0) {
        OnSwitchClick(event);
        return;
    }
    wxMenu* popupMenu = new wxMenu(wxEmptyString);
    
    wxString map_name = map_titles[ new_order[select_id] ];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml == NULL) {
        // no other options
        return;
    }
    
    if (canvas->GetShapeType() == MapCanvas::polygons &&
        ml->GetShapeType() == Shapefile::POINT_TYP)
    {
        popupMenu->Append(XRCID("MAPTREE_POINT_IN_POLYGON"), _("Spatial Join Count"));
        Connect(XRCID("MAPTREE_POINT_IN_POLYGON"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnSpatialJoinCount));
        popupMenu->AppendSeparator();
    }
    
    popupMenu->Append(XRCID("MAPTREE_CHANGE_FILL_COLOR"), _("Change Fill Color"));
    popupMenu->Append(XRCID("MAPTREE_CHANGE_OUTLINE_COLOR"), _("Change Outline Color"));
    popupMenu->Append(XRCID("MAPTREE_OUTLINE_VISIBLE"), _("Outline Visible"));
        
    // check menu items
    wxMenuItem* outline = popupMenu->FindItem(XRCID("MAPTREE_OUTLINE_VISIBLE"));
    if (outline) {
        outline->SetCheckable(true);
        if (ml->GetPenSize() > 0) outline->Check();
    }
    
    Connect(XRCID("MAPTREE_CHANGE_FILL_COLOR"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnChangeFillColor));
    Connect(XRCID("MAPTREE_CHANGE_OUTLINE_COLOR"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnChangeOutlineColor));
    Connect(XRCID("MAPTREE_OUTLINE_VISIBLE"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnOutlineVisible));
    
    if (ml->GetShapeType() == Shapefile::POINT_TYP) {
        popupMenu->Append(XRCID("MAPTREE_CHANGE_POINT_RADIUS"), _("Change Point Radius"));
        Connect(XRCID("MAPTREE_CHANGE_POINT_RADIUS"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnChangePointRadius));
        
    } else {
        popupMenu->Append(XRCID("MAPTREE_BOUNDARY_ONLY"), _("Only Map Boundary"));
        Connect(XRCID("MAPTREE_BOUNDARY_ONLY"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnShowMapBoundary));
        wxMenuItem* boundary = popupMenu->FindItem(XRCID("MAPTREE_BOUNDARY_ONLY"));
        if (boundary) {
            boundary->SetCheckable(true);
            if (ml->IsShowBoundary()) boundary->Check();
        }
    }
    
    popupMenu->AppendSeparator();
    popupMenu->Append(XRCID("MAPTREE_REMOVE"), _("Remove"));
    Connect(XRCID("MAPTREE_REMOVE"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnRemoveMapLayer));
    
    PopupMenu(popupMenu, event.GetPosition());
}

void MapTree::OnDraw(wxDC& dc)
{
    dc.SetFont(*GdaConst::small_font);
    dc.SetPen(*wxBLACK_PEN);
    wxCoord w, h;
    dc.GetSize(&w, &h);
    
    dc.DrawText(_("Map Layer Setting"), 5, 10);
    
    if ( !select_name.IsEmpty() ) {
        DrawLegend(dc, px_switch, move_pos.y, select_name);
    }
    
    for (int i=0; i<new_order.size(); i++) {
        int idx = new_order[i];
        wxString map_name = map_titles[idx];
        
        if (select_name != map_name) {
            int y =  py + (leg_h + leg_pad_y) * i;
            LOG_MSG(y);
            DrawLegend(dc, px_switch, y, map_name);
        }
    }
    wxPen pen(*wxBLACK, 1, wxPENSTYLE_DOT);
    dc.SetPen(pen);
    dc.DrawLine(10, 24, 10, (24 + (leg_h + leg_pad_y) * new_order.size()));
}

void MapTree::DrawLegend(wxDC& dc, int x, int y, wxString text)
{
    int x_org = x;
    
    wxPen pen(*wxBLACK, 1, wxPENSTYLE_DOT);
    dc.SetPen(pen);
    dc.DrawLine(10, y+2, x, y+2);
    
    BackgroundMapLayer* ml = NULL;
    if (bg_maps.find(text) != bg_maps.end()) {
        ml = bg_maps[text];
    } else if (fg_maps.find(text) != fg_maps.end()) {
        ml = fg_maps[text];
    }
    
    x = x + 45; // switch width
    dc.SetPen(*wxBLACK_PEN);
    if (text == current_map_title) {
        dc.SetPen(*wxLIGHT_GREY);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
    } else {
        wxPen pen(ml->GetPenColour(), ml->GetPenSize());
        wxBrush brush(ml->GetBrushColour());
        dc.SetPen(pen);
        dc.SetBrush(brush);
        dc.DrawRectangle(x, y, leg_w, leg_h);
    }
    
    x = x + leg_w + leg_pad_x;
    dc.SetTextForeground(*wxBLACK);
    if (text == current_map_title) {
        dc.SetTextForeground(*wxLIGHT_GREY);
    }
    dc.DrawText(text, x, y);
    
    // draw switch button
    if (ml == NULL) {
        return;
    }
    
    wxString ds_thumb = "switch-on.png";
    if (ml->IsHide()) ds_thumb = "switch-off.png";
    wxString file_path_str = GenUtils::GetSamplesDir() + ds_thumb;
    
    wxImage img;
    if (!wxFileExists(file_path_str)) {
        return;
    }
    
    img.LoadFile(file_path_str);
    if (!img.IsOk()) {
        return;
    }
    
    wxBitmap bmp(img);
    dc.DrawBitmap(bmp, x_org, y);
}

int MapTree::GetCategoryClick(wxMouseEvent& event)
{
    wxPoint pt(event.GetPosition());
    int x = pt.x, y = pt.y;
    wxCoord w, h;
    GetClientSize(&w, &h);
    for (int i = 0; i<map_titles.size(); i++) {
        int cur_y = py + (leg_h + leg_pad_y) * i;
        if ((x > px) && (x < w - px) &&
            (y > cur_y) && (y < cur_y + leg_h))
        {
            return i;
        }
    }
    return -1;
}

int MapTree::GetSwitchClick(wxMouseEvent& event)
{
    wxPoint pt(event.GetPosition());
    int x = pt.x, y = pt.y;
    
    for (int i = 0; i<map_titles.size(); i++) {
        int cur_y = py + (leg_h + leg_pad_y) * i;
        if ((x > px_switch) && (x < px_switch + 30) &&
            (y > cur_y) && (y < cur_y + leg_h))
        {
            return i;
        }
    }
    return -1;
}

int MapTree::GetLegendClick(wxMouseEvent& event)
{
    wxPoint pt(event.GetPosition());
    int x = pt.x, y = pt.y;
    
    for (int i = 0; i<map_titles.size(); i++) {
        int cur_y = py + (leg_h + leg_pad_y) * i;
        if (x > px_switch + 30 &&
            (y > cur_y) && (y < cur_y + leg_h))
        {
            return i;
        }
    }
    return -1;
}

void MapTree::OnSwitchClick(wxMouseEvent& event)
{
    int switch_idx = GetSwitchClick(event);
    if (switch_idx > -1) {
        BackgroundMapLayer* ml = NULL;
        wxString map_name = map_titles[new_order[switch_idx]];
        if (bg_maps.find(map_name) != bg_maps.end()) {
            ml = bg_maps[map_name];
        } else if (fg_maps.find(map_name) != fg_maps.end()) {
            ml = fg_maps[map_name];
        }
        if (ml) {
            ml->SetHide(!ml->IsHide());
            canvas->ReDraw();
            Refresh();
        }
    }
}

BackgroundMapLayer* MapTree::GetMapLayer(wxString map_name)
{
    BackgroundMapLayer* ml = NULL;
    if (bg_maps.find(map_name) != bg_maps.end()) {
        ml = bg_maps[map_name];
    } else if (fg_maps.find(map_name) != fg_maps.end()) {
        ml = fg_maps[map_name];
    }
    return ml;
}

void MapTree::OnMapLayerChange()
{
    map<wxString, BackgroundMapLayer*> new_bg_maps;
    map<wxString, BackgroundMapLayer*> new_fg_maps;
    
    bool is_fgmap = true;
    for (int i=0; i<new_order.size(); i++) {
        wxString name = map_titles[ new_order[i] ];
        if (name == current_map_title) {
            is_fgmap = false;
            continue;
        }
        if (is_fgmap) {
            new_fg_maps[name] = GetMapLayer(name);
        } else {
            new_bg_maps[name] = GetMapLayer(name);
        }
    }
    
    bg_maps = new_bg_maps;
    fg_maps = new_fg_maps;
    canvas->SetForegroundMayLayers(fg_maps);
    canvas->SetBackgroundMayLayers(bg_maps);
    canvas->DisplayMapLayers();
}

void MapTree::AddCategoryColorToMenu(wxMenu* menu, int cat_clicked)
{
    
}

MapTreeDlg::MapTreeDlg(wxWindow* parent, MapCanvas* canvas, const wxPoint& pos, const wxSize& size)
: wxDialog(parent, -1, _("Map Layer Setting"), pos, size)
{
    MapTree *tree = new MapTree(this, canvas, pos, size);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(tree, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
}

MapTreeDlg::~MapTreeDlg()
{
    
}

MapTreeFrame::MapTreeFrame(wxWindow* parent, MapCanvas* _canvas, const wxPoint& pos, const wxSize& size)
: wxFrame(parent, -1, _canvas->GetCanvasTitle(), pos, size)
{
    SetBackgroundColour(*wxWHITE);
    canvas = _canvas;
    tree = new MapTree(this, canvas, pos, size);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(tree, 1, wxEXPAND|wxALL, 10);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    Centre();
    
    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MapTreeFrame::OnClose));
}

MapTreeFrame::~MapTreeFrame()
{
    delete tree;
}

void MapTreeFrame::OnClose( wxCloseEvent& event )
{
    Destroy();
    event.Skip();
}

void MapTreeFrame::Recreate()
{
    int w, h;
    GetClientSize(&w, &h);
    
    int n_maps = canvas->GetBackgroundMayLayers().size() + canvas->GetForegroundMayLayers().size() + 1;
    h = n_maps * 25  + 80;
    SetSize(w, h);
    Layout();
    
    tree->Init();
}