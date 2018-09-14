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
#include <wx/choicdlg.h>

#include "../DialogTools/SaveToTableDlg.h"
#include "../logger.h"
#include "../Project.h"
#include "../SpatialIndTypes.h"
#include "MapNewView.h"
#include "MapLayer.hpp"
#include "MapLayerTree.hpp"

SetAssociationDlg::SetAssociationDlg(wxWindow* parent, wxString _current_map_title, vector<wxString>& _current_map_fieldnames, BackgroundMapLayer* ml,vector<BackgroundMapLayer*>& _bg_maps, vector<BackgroundMapLayer*>& _fg_maps, const wxPoint& pos, const wxSize& size)
: wxDialog(parent, -1, _("Set Association Dialog"), pos, size)
{
    current_map_title = _current_map_title;
    current_map_fieldnames = _current_map_fieldnames;
    current_ml = ml;
    bg_maps = _bg_maps;
    fg_maps = _fg_maps;
    
    wxPanel* panel = new wxPanel(this, -1);
    
    wxStaticText* st1 = new wxStaticText(panel, -1, _("Select layer"));
    layer_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxStaticText* st = new wxStaticText(panel, -1, _("and field"));
    field_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxStaticText* st2 = new wxStaticText(panel, -1, _("is associated to"));
    my_field_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxStaticText* st3 = new wxStaticText(panel, -1, _("in current layer."));
    wxBoxSizer* mbox = new wxBoxSizer(wxHORIZONTAL);
    mbox->Add(st1, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(layer_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(st, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(st2, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(my_field_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(st3, 0, wxALIGN_CENTER | wxALL, 5);
    
    wxBoxSizer* cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(mbox, 0, wxALIGN_CENTER | wxTOP, 15);
    panel->SetSizerAndFit(cbox);
    
    wxButton* ok_btn = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition,  wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);
    
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(panel, 1, wxALL | wxEXPAND, 15);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(vbox);
    vbox->Fit(this);
    
    Center();
    
    layer_list->Bind(wxEVT_CHOICE, &SetAssociationDlg::OnLayerSelect, this);
    
    Init();
}

void SetAssociationDlg::Init()
{
    layer_list->Clear();
    if (current_ml) {
        layer_list->Append(current_map_title);
        for (int i=0; i<bg_maps.size(); i++) {
            wxString name = bg_maps[i]->GetName();
            if (name != current_ml->GetName()) {
                layer_list->Append(name);
            }
        }
        for (int i=0; i<fg_maps.size(); i++) {
            wxString name = fg_maps[i]->GetName();
            if (name != current_ml->GetName()) {
                layer_list->Append(name);
            }
        }
    } else {
        // map canvas
        for (int i=0; i<bg_maps.size(); i++) {
            wxString name = bg_maps[i]->GetName();
            layer_list->Append(name);
        }
        for (int i=0; i<fg_maps.size(); i++) {
            wxString name = fg_maps[i]->GetName();
            layer_list->Append(name);
        }
    }
    
    wxCommandEvent e;
    OnLayerSelect(e);
    
    my_field_list->Clear();
    my_field_list->Append(_("(Use Sequences)"));
    if (current_ml) {
        vector<wxString> my_fieldnames = current_ml->GetKeyNames();
        for (int i=0; i<my_fieldnames.size(); i++) {
            my_field_list->Append(my_fieldnames[i]);
        }
    } else {
        for (int i=0; i<current_map_fieldnames.size(); i++) {
            my_field_list->Append(current_map_fieldnames[i]);
        }
    }
}

void SetAssociationDlg::OnLayerSelect(wxCommandEvent& e)
{
    int idx = layer_list->GetSelection();
    if (idx >=0) {
        field_list->Clear();
        field_list->Append(_("(Use Sequences)"));
        wxString map_name = layer_list->GetString(idx);
        BackgroundMapLayer* ml = GetMapLayer(map_name);
        if (ml) {
            vector<wxString> names = ml->GetKeyNames();
            for (int i=0; i<names.size(); i++) {
                field_list->Append(names[i]);
            }
        } else if (idx == 0){
            for (int i=0; i<current_map_fieldnames.size(); i++) {
                field_list->Append(current_map_fieldnames[i]);
            }
        }
    }
}

BackgroundMapLayer* SetAssociationDlg::GetMapLayer(wxString map_name)
{
    bool found = false;
    BackgroundMapLayer* ml = NULL;
    for (int i=0; i<bg_maps.size(); i++) {
        if (bg_maps[i]->GetName() == map_name) {
            ml = bg_maps[i];
            found = true;
            break;
        }
    }
    if (found == false) {
        for (int i=0; i<fg_maps.size(); i++) {
            if (fg_maps[i]->GetName() == map_name) {
                ml = fg_maps[i];
                found = true;
                break;
            }
        }
    }
    return ml;
}

wxString SetAssociationDlg::GetCurrentLayerFieldName()
{
    int idx = my_field_list->GetSelection();
    if (idx > 0) {
        return my_field_list->GetString(idx);
    }
    return wxEmptyString;
}

wxString SetAssociationDlg::GetSelectLayerFieldName()
{
    int idx = field_list->GetSelection();
    if (idx > 0) {
        return field_list->GetString(idx);
    }
    return wxEmptyString;
}

BackgroundMapLayer* SetAssociationDlg::GetSelectMapLayer()
{
    int idx = layer_list->GetSelection();
    if (idx >=0) {
        wxString map_name = layer_list->GetString(idx);
        bool found = false;
        BackgroundMapLayer* ml = NULL;
        for (int i=0; i<bg_maps.size(); i++) {
            if (bg_maps[i]->GetName() == map_name) {
                ml = bg_maps[i];
                found = true;
                break;
            }
        }
        if (found == false) {
            for (int i=0; i<fg_maps.size(); i++) {
                if (fg_maps[i]->GetName() == map_name) {
                    ml = fg_maps[i];
                    found = true;
                    break;
                }
            }
        }
        return ml;
    }
    return NULL;
}

IMPLEMENT_ABSTRACT_CLASS(MapTree, wxWindow)
BEGIN_EVENT_TABLE(MapTree, wxWindow)
EVT_MENU(XRCID("IDC_CHANGE_POINT_RADIUS"), MapTree::OnChangePointRadius)
EVT_MOUSE_EVENTS(MapTree::OnEvent)
END_EVENT_TABLE()

MapTree::MapTree(wxWindow *parent, MapCanvas* _canvas, const wxPoint& pos, const wxSize& size)
: wxWindow(parent, wxID_ANY, pos, size),
select_id(-1),
canvas(_canvas),
is_resize(false),
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

	Connect(wxEVT_IDLE, wxIdleEventHandler(MapTree::OnIdle));
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
    
    for (int i=0; i<fg_maps.size(); i++) {
        wxString lbl = fg_maps[i]->GetName();
        map_titles.push_back(lbl);
    }
    map_titles.push_back(current_map_title);
    for (int i=0; i<bg_maps.size(); i++) {
        wxString lbl = bg_maps[i]->GetName();
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

void MapTree::OnIdle(wxIdleEvent& event)
{
	if (is_resize) {
		is_resize = false;
		Refresh();
	}
}
void MapTree::OnPaint( wxPaintEvent& event )
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    OnDraw(dc);
}

void MapTree::OnRemoveMapLayer(wxCommandEvent& event)
{
    bool found = false;
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = NULL;
    for (int i=0; i<bg_maps.size(); i++) {
        ml = bg_maps[i];
        if (ml->GetName() == map_name) {
            // remove if it is associated with other layer
            RemoveAssociationRelationship(ml);
            canvas->RemoveLayer(map_name);
            bg_maps.erase(bg_maps.begin() + i);
            found = true;
            break;
        }
    }
    if (found == false) {
        for (int i=0; i<fg_maps.size(); i++) {
            ml = fg_maps[i];
            if (ml->GetName() == map_name) {
                // remove if it is associated with other layer
                RemoveAssociationRelationship(ml);
                canvas->RemoveLayer(map_name);
                fg_maps.erase(fg_maps.begin() + i);
                found = true;
                break;
            }
        }
    }
    
    if (found) {
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
}

void MapTree::RemoveAssociationRelationship(BackgroundMapLayer* ml)
{
    BackgroundMapLayer* tmp = NULL;
    for (int i=0; i<bg_maps.size(); i++) {
        tmp = bg_maps[i];
        if (tmp != ml) {
            tmp->RemoveAssociationRelationship(ml);
        }
    }
    for (int i=0; i<fg_maps.size(); i++) {
        tmp = fg_maps[i];
        if (tmp != ml) {
            tmp->RemoveAssociationRelationship(ml);
        }
    }
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
        canvas->DisplayMapLayers();
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
        canvas->DisplayMapLayers();
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
            canvas->DisplayMapLayers();
        }
    }
}

void MapTree::OnSetPrimaryKey(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    if (current_map_title == map_name) {
        // work on current map
    } else {
        BackgroundMapLayer* ml = GetMapLayer(map_name);
        if (ml) {
            wxArrayString choices;
            vector<wxString> field_names = ml->GetKeyNames();
            if (field_names.empty()) {
                wxMessageDialog dlg (this, _("Select map layer has no integer or string field."), _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                return;
            }
            for (int i=0; i<field_names.size(); i++) {
                choices.Add(field_names[i]);
            }
            wxSingleChoiceDialog dialog(this, _("Please select a field as primary key:\n(Cancel if use default sequence id) "), _("Set Primary Key"), choices);
            if (dialog.ShowModal() == wxID_OK) {
                wxString key = dialog.GetStringSelection();
                ml->SetPrimaryKey(key);
            }
        }
    }
}

void MapTree::OnSetAssociateLayer(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    vector<wxString> map_fieldnames = canvas->GetProject()->GetIntegerAndStringFieldNames();
    SetAssociationDlg dlg(this, current_map_title, map_fieldnames, ml, bg_maps, fg_maps);
    if (dlg.ShowModal() == wxID_OK) {
        wxString my_key = dlg.GetCurrentLayerFieldName();
        wxString asso_key = dlg.GetSelectLayerFieldName();
        BackgroundMapLayer* asso_ml =  dlg.GetSelectMapLayer();
        if (asso_ml == NULL && !my_key.IsEmpty()) {
            // association with map_canvas
            ml->SetMapAssociation(my_key, asso_key);
            
        } else if (asso_ml && !asso_key.IsEmpty()) {
            // association with other layer
            if (asso_ml->GetAssociatedLayer() == ml) {
                wxMessageDialog dlg (this, _("Current layer has already been associated with selected layer. Please select another layer to associate with."), _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
            }
            ml->SetLayerAssociation(my_key, asso_ml, asso_key);
            
        } else {
            wxMessageDialog dlg (this, _("Can't setup highlight association with selected layer and field. Please try again."), _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
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
        canvas->DisplayMapLayers();
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
        canvas->DisplayMapLayers();
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
        Refresh();
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
					int val = new_order[select_id];
                    if (select_id > cat_clicked) {
                        new_order.insert(new_order.begin()+cat_clicked, val);
                        new_order.erase(new_order.begin() + select_id + 1);
                    } else {
                        new_order.insert(new_order.begin()+cat_clicked+1, val);
                        new_order.erase(new_order.begin() + select_id);
                    }
                    select_id = cat_clicked;
                }
            }
        }
		Refresh();
    } else if (event.LeftUp()) {
        if (isLeftMove) {
            isLeftMove = false;
            // stop move
            OnMapLayerChange();
            select_id = -1;
            select_name = "";
			
        } else {
            // only left click
            OnSwitchClick(event);
        }
		select_name = "";
		Refresh(false);
        isLeftDown = false;
    } else {
		select_name = "";
		Refresh(false);
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
    
    wxString menu_name =  _("Set Highlight Association");
    if (ml) {
        if (!ml->GetAssociationText().IsEmpty()) {
            menu_name += "(" + ml->GetAssociationText() + ")";
        }
    }
    popupMenu->Append(XRCID("MAPTREE_SET_FOREIGN_KEY"), menu_name);
    Connect(XRCID("MAPTREE_SET_FOREIGN_KEY"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnSetAssociateLayer));
    
    if (ml == NULL) {
        // no other options
        return;
    }
    
    popupMenu->AppendSeparator();
    
    if (canvas->GetShapeType() == MapCanvas::polygons &&
        ml->GetShapeType() == Shapefile::POINT_TYP)
    {
        //popupMenu->Append(XRCID("MAPTREE_POINT_IN_POLYGON"), _("Spatial Join Count"));
        //Connect(XRCID("MAPTREE_POINT_IN_POLYGON"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MapTree::OnSpatialJoinCount));
        //popupMenu->AppendSeparator();
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
    
    BackgroundMapLayer* ml = GetMapLayer(text);
    
    x = x + 45; // switch width
    dc.SetPen(*wxBLACK_PEN);
    if (text == current_map_title) {
        dc.SetPen(*wxLIGHT_GREY);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
    } else {
        if (ml == NULL)
            return; // in case of removed layer
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
        wxString map_name = map_titles[new_order[switch_idx]];
        
        BackgroundMapLayer* ml = GetMapLayer(map_name);
        
        if (ml) {
            ml->SetHide(!ml->IsHide());
            canvas->DisplayMapLayers();
            Refresh();
        }
    }
}

BackgroundMapLayer* MapTree::GetMapLayer(wxString map_name)
{
    bool found = false;
    BackgroundMapLayer* ml = NULL;
    for (int i=0; i<bg_maps.size(); i++) {
        if (bg_maps[i]->GetName() == map_name) {
            ml = bg_maps[i];
            found = true;
            break;
        }
    }
    if (found == false) {
        for (int i=0; i<fg_maps.size(); i++) {
            if (fg_maps[i]->GetName() == map_name) {
                ml = fg_maps[i];
                found = true;
                break;
            }
        }
    }
    return ml;
}

void MapTree::OnMapLayerChange()
{
    vector<BackgroundMapLayer*> new_bg_maps;
    vector<BackgroundMapLayer*> new_fg_maps;
    
    bool is_fgmap = true;
    for (int i=0; i<new_order.size(); i++) {
        wxString name = map_titles[ new_order[i] ];
        if (name == current_map_title) {
            is_fgmap = false;
            continue;
        }
        if (is_fgmap) {
            new_fg_maps.push_back(GetMapLayer(name));
        } else {
            new_bg_maps.push_back(GetMapLayer(name));
        }
    }
    
    bg_maps = new_bg_maps;
    fg_maps = new_fg_maps;
	is_resize = true;
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
    h = n_maps * 25  + 120;
    SetSize(w, h);
    Layout();
    
    tree->Init();
}
