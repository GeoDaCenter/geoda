//
//  MapLayerTree.cpp
//  GeoDa
//
//  Created by Xun Li on 8/24/18.
//

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcbuffer.h>
#include <wx/colordlg.h>

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
isDragDropAllowed(false),
bg_maps(canvas->GetBackgroundMayLayers()),
fg_maps(canvas->GetForegroundMayLayers())
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
        wxSize sz(size.GetWidth() - x, leg_h);
        new_order.push_back(i);
    }
    
    Connect(wxEVT_PAINT, wxPaintEventHandler(MapTree::OnPaint));
}

MapTree::~MapTree()
{
}

void MapTree::OnPaint( wxPaintEvent& event )
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    OnDraw(dc);
}

void MapTree::OnChangeFillColor(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        wxColour clr;
        clr = wxGetColourFromUser(this, ml->GetBrushColour());
        ml->SetBrushColour(clr);
        canvas->ReDraw();
        Refresh();
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
        canvas->ReDraw();
        Refresh();
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
            canvas->ReDraw();
            Refresh();
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
        canvas->ReDraw();
        Refresh();
    }
}
void MapTree::OnShowMapBoundary(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml) {
        bool show_bnd = ml->IsShowBoundary();
        ml->ShowBoundary(!show_bnd);
        canvas->ReDraw();
        Refresh();
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
    popupMenu->Append(XRCID("MAPTREE_CHANGE_FILL_COLOR"), _("Change Fill Color"));
    popupMenu->Append(XRCID("MAPTREE_CHANGE_OUTLINE_COLOR"), _("Change Outline Color"));
    popupMenu->Append(XRCID("MAPTREE_OUTLINE_VISIBLE"), _("Outline Visible"));
    
    wxString map_name = map_titles[ new_order[select_id] ];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    if (ml == NULL) {
        return;
    }
    
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
    
    PopupMenu(popupMenu, event.GetPosition());
}

void MapTree::OnDraw(wxDC& dc)
{
    dc.SetFont(*GdaConst::small_font);
    dc.SetPen(*wxBLACK_PEN);
    wxCoord w, h;
    dc.GetSize(&w, &h);
    
    if ( !select_name.IsEmpty() ) {
        DrawLegend(dc, px_switch, move_pos.y, select_name);
    }
    
    for (int i=0; i<new_order.size(); i++) {
        int idx = new_order[i];
        wxString map_name = map_titles[idx];
        
        if (select_name != map_name) {
            int y =  py + (leg_h + leg_pad_y) * i;
            DrawLegend(dc, px_switch, y, map_name);
        }
    }
    wxPen pen(*wxBLACK, 1, wxPENSTYLE_DOT);
    dc.SetPen(pen);
    dc.DrawLine(10, py, 10, (py + (leg_h + leg_pad_y) * new_order.size()));
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
    }
    dc.DrawRectangle(x, y, leg_w, leg_h);
    
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
    canvas->PopulateCanvas();
    canvas->ReDraw();
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
