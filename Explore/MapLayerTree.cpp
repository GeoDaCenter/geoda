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

#include "../rc/GeoDaIcon-16x16.xpm"
#include "../DialogTools/SaveToTableDlg.h"
#include "../logger.h"
#include "../Project.h"
#include "../SpatialIndTypes.h"
#include "MapNewView.h"
#include "MapLayer.hpp"
#include "MapLayerTree.hpp"

wxString SetAssociationDlg::LAYER_LIST_ID = "SETASSOCIATIONDLG_LAYER_LIST";

SetAssociationDlg::SetAssociationDlg(wxWindow* parent, AssociateLayerInt* ml,vector<AssociateLayerInt*>& _all_layers, const wxString& title, const wxPoint& pos, const wxSize& size)
: wxDialog(parent, wxID_ANY, title, pos, size)
{
    current_ml = ml;
    all_layers = _all_layers;
    int nrows = all_layers.size();
    if (nrows == 0) nrows = 1;
    for (int i=0; i<nrows; i++) {
        wxString layer_id = LAYER_LIST_ID;
        layer_id << i;
        layer_list.push_back(new wxChoice(this, XRCID(layer_id), wxDefaultPosition, wxSize(100,-1)));
        field_list.push_back(new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100,-1)));
        my_field_list.push_back(new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100,-1)));
        conn_list.push_back(new wxCheckBox(this, wxID_ANY, _("Show connect line")));
        
        layer_list[i]->Bind(wxEVT_CHOICE, &SetAssociationDlg::OnLayerSelect, this);
    }
    CreateControls(nrows);
    Center();
    Init();
}

void SetAssociationDlg::CreateControls(int nrows)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    top_sizer->AddSpacer(10);
    
    wxFlexGridSizer *fg_sizer = new wxFlexGridSizer(nrows, 8, 3, 3);
    for (int i=0; i<nrows; i++) {
        fg_sizer->Add(new wxStaticText(this, wxID_ANY, _("Select layer")), 0, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxLEFT, 5);
        fg_sizer->Add(layer_list[i], 0, wxALL|wxALIGN_CENTER, 5);
        fg_sizer->Add(new wxStaticText(this, wxID_ANY, _("and field")), 0, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxLEFT, 5);
        fg_sizer->Add(field_list[i], 0, wxALL|wxALIGN_CENTER, 5);
        fg_sizer->Add(new wxStaticText(this, wxID_ANY, _("is associated to")), 0, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxLEFT, 5);
        fg_sizer->Add(my_field_list[i], 0, wxALL|wxALIGN_CENTER, 5);
        fg_sizer->Add(new wxStaticText(this, wxID_ANY, _("in current layer.")), 0, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxLEFT, 5);
        fg_sizer->Add(conn_list[i], 0, wxALIGN_CENTER | wxTOP | wxBOTTOM | wxLEFT, 5);
    }
    
    top_sizer->Add(fg_sizer, 0, wxALL|wxALIGN_CENTER, 15);
    
    wxButton* ok_btn = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition,  wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);
    
    top_sizer->Add(hbox, 0, wxALL|wxALIGN_CENTER, 15);
    
    SetSizerAndFit(top_sizer);
    
    ok_btn->Bind(wxEVT_BUTTON, &SetAssociationDlg::OnOk, this);
}

int SetAssociationDlg::GetSelectRow(wxCommandEvent& e)
{
    int sel_row = -1;
    int ctrl_id = e.GetId();
    int nrows = all_layers.size();
    for (int i=0; i<nrows; i++) {
        wxString layer_id = LAYER_LIST_ID;
        layer_id << i;
        if (ctrl_id == XRCID(layer_id)) {
            sel_row = i;
            break;
        }
    }
    return sel_row;
}

void SetAssociationDlg::Init()
{
    int nrows = all_layers.size();
    for (int i=0; i<nrows; i++) {
        wxString layer_id = LAYER_LIST_ID;
        layer_id << i;
        int ctrl_id = XRCID(layer_id);
        layer_list[i]->Clear();
        layer_list[i]->Append("");
        for (int j=0; j<all_layers.size(); j++) {
            wxString name = all_layers[j]->GetName();
            layer_list[i]->Append(name);
        }
        
        wxCommandEvent e;
        e.SetId(ctrl_id);
        OnLayerSelect(e);
        
        my_field_list[i]->Clear();
        my_field_list[i]->Append("");
        my_field_list[i]->Append(_("(Use Sequences)"));
        vector<wxString> my_fieldnames = current_ml->GetKeyNames();
        for (int j=0; j<my_fieldnames.size(); j++) {
            my_field_list[i]->Append(my_fieldnames[j]);
        }
    }
    
    int i = 0;
    map<AssociateLayerInt*, Association>& asso = current_ml->associated_layers;
    map<AssociateLayerInt*, Association>::iterator it;
    for (it = asso.begin(); it!=asso.end(); it++) {
        AssociateLayerInt* layer = it->first;
        Association& lyr = it->second;
        wxString my_key = lyr.first;
        wxString key = lyr.second;
        
        wxString layer_name;
        for (int j=0; j<all_layers.size(); j++) {
            wxString name = all_layers[j]->GetName();
            if (layer->GetName() == name) {
                layer_name = name;
                layer_list[i]->SetSelection(j+1);
                
                wxString layer_id = LAYER_LIST_ID;
                layer_id << i;
                int ctrl_id = XRCID(layer_id);
                wxCommandEvent e;
                e.SetId(ctrl_id);
                OnLayerSelect(e);
                
                AssociateLayerInt* ml = GetMapLayer(layer_name);
                if (ml) {
                    if (key.IsEmpty()) {
                        field_list[i]->SetSelection(1);
                    } else {
                        vector<wxString> names = ml->GetKeyNames();
                        for (int j=0; j<names.size(); j++) {
                            if (key == names[j]) {
                                field_list[i]->SetSelection(j+2);
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
        
        vector<wxString> my_fieldnames = current_ml->GetKeyNames();
        for (int j=0; j<my_fieldnames.size(); j++) {
            if (my_key == my_fieldnames[j]) {
                my_field_list[i]->SetSelection(j+2);
                break;
            }
        }
        
        conn_list[i]->SetValue(current_ml->associated_lines[layer]);
        
        i++;
    }
}

bool SetAssociationDlg::CheckLayerValid(int irow, wxString layer_name)
{
    int nrows = all_layers.size();
    for (int i=0; i<nrows; i++) {
        if (i != irow) {
            int idx = layer_list[i]->GetSelection();
            wxString check_name = layer_list[i]->GetString(idx);
            if (check_name == layer_name) {
                return false;
            }
        }
    }
    return true;
}

void SetAssociationDlg::OnLayerSelect(wxCommandEvent& e)
{
    int sel_row = GetSelectRow(e);
    int idx = layer_list[sel_row]->GetSelection();
    if (idx >=0) {
        field_list[sel_row]->Clear();
        wxString map_name = layer_list[sel_row]->GetString(idx);
        if (map_name.IsEmpty())
            return;
        if (CheckLayerValid(sel_row, map_name) == false) {
            wxMessageDialog dlg (this, _("Current layer has already been associated with selected layer. Please select another layer to associate with."), _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            layer_list[sel_row]->SetSelection(0);
            return;
        }
        AssociateLayerInt* ml = GetMapLayer(map_name);
        if (ml) {
            field_list[sel_row]->Append("");
            field_list[sel_row]->Append(_("(Use Sequences)"));
            vector<wxString> names = ml->GetKeyNames();
            for (int i=0; i<names.size(); i++) {
                field_list[sel_row]->Append(names[i]);
            }
        }
    }
}

AssociateLayerInt* SetAssociationDlg::GetMapLayer(wxString map_name)
{
    bool found = false;
    AssociateLayerInt* ml = NULL;
    for (int i=0; i<all_layers.size(); i++) {
        if (all_layers[i]->GetName() == map_name) {
            ml = all_layers[i];
            found = true;
            break;
        }
    }
    return ml;
}

void SetAssociationDlg::OnOk(wxCommandEvent& e)
{
    current_ml->ClearLayerAssociation();
    
    int nrows = all_layers.size();
    for (int i=0; i<nrows; i++) {
        int lyr_idx = layer_list[i]->GetSelection();
        if (lyr_idx > 0) {
            wxString mykey = GetCurrentLayerFieldName(i);
            wxString key = GetSelectLayerFieldName(i);
            AssociateLayerInt* layer = GetSelectMapLayer(i);
            bool conn_flag = conn_list[i]->IsChecked();
            if (!mykey.IsEmpty() && !key.IsEmpty()) {
                current_ml->SetLayerAssociation(mykey, layer, key, conn_flag);
            }
        }
    }
    
    EndDialog(wxID_OK);
}

wxString SetAssociationDlg::GetCurrentLayerFieldName(int irow)
{
    int idx = my_field_list[irow]->GetSelection();
    if (idx > 0) {
        return my_field_list[irow]->GetString(idx);
    }
    return wxEmptyString;
}

wxString SetAssociationDlg::GetSelectLayerFieldName(int irow)
{
    int idx = field_list[irow]->GetSelection();
    if (idx > 0) { // first row is "(Use Sequences)"
        return field_list[irow]->GetString(idx);
    }
    return wxEmptyString;
}

AssociateLayerInt* SetAssociationDlg::GetSelectMapLayer(int irow)
{
    int idx = layer_list[irow]->GetSelection();
    if (idx > 0) {
        wxString map_name = layer_list[irow]->GetString(idx);
        bool found = false;
        AssociateLayerInt* ml = NULL;
        for (int i=0; i<all_layers.size(); i++) {
            if (all_layers[i]->GetName() == map_name) {
                ml = all_layers[i];
                found = true;
                break;
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
    
    current_map_title = canvas->GetName() + _(" (current map)");
    
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
        //int y =  py + (leg_h + leg_pad_y) * i;
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
            tmp->RemoveAssociatedLayer(ml);
        }
    }
    for (int i=0; i<fg_maps.size(); i++) {
        tmp = fg_maps[i];
        if (tmp != ml) {
            tmp->RemoveAssociatedLayer(ml);
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
                           _("Save Results: Spatial Counts"),
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
        canvas->RedrawMap();
    }
}

void MapTree::OnChangeAssociatelineColor(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    AssociateLayerInt* ml = GetMapLayer(map_name);
    if (ml == NULL) {
        ml = canvas;
    }
    if (ml) {
        wxColour clr;
        clr = wxGetColourFromUser(this, ml->GetAssociatePenColour());
        ml->SetAssociatePenColour(clr);
        Refresh();
        canvas->RedrawMap();
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
        canvas->RedrawMap();
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
            canvas->RedrawMap();
        }
    }
}

void MapTree::OnClearAssociateLayer(wxCommandEvent& event)
{
    wxString map_name = map_titles[new_order[select_id]];
    AssociateLayerInt* ml = GetMapLayer(map_name);
    
    if (ml == NULL) {
        // selection is current map
        ml = canvas;
        map_name = canvas->GetName();
    }
    
    ml->ClearLayerAssociation();
    
}

void MapTree::OnZoomToLayer(wxCommandEvent& event)
{
    // set map extent to selected layer
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    double minx, miny, maxx, maxy;
    if (ml == NULL) {
        // selected layer is current map
        canvas->GetExtent(minx, miny, maxx, maxy);
    } else {
        // other layer
        ml->GetExtent(minx, miny, maxx, maxy);
    }
    canvas->ExtentTo(minx, miny, maxx, maxy);
    canvas->RedrawMap();
}

void MapTree::OnZoomToSelected(wxCommandEvent& event)
{
    // set map extent to selected objects in mouse clicked layer
    wxString map_name = map_titles[new_order[select_id]];
    BackgroundMapLayer* ml = GetMapLayer(map_name);
    double minx, miny, maxx, maxy;
    if (ml == NULL) {
        // selected layer is current map
        canvas->GetExtentOfSelected(minx, miny, maxx, maxy);
    } else {
        // other layer
        ml->GetExtentOfSelected(minx, miny, maxx, maxy);
        // re projection if needed
        OGRSpatialReference* destSR = canvas->GetSpatialReference();
        OGRSpatialReference* sourceSR = ml->GetSpatialReference();
#ifdef __PROJ6__
        if (sourceSR) {
            sourceSR->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
        }
        if (destSR) {
            destSR->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
        }
#endif
        OGRCoordinateTransformation *poCT = OGRCreateCoordinateTransformation(sourceSR, destSR);
        if (poCT!= NULL) {
            //poCT->Transform(1, &minx, &miny);
            //poCT->Transform(1, &maxx, &maxx);
        }
    }
    canvas->ExtentTo(minx, miny, maxx, maxy);
    canvas->RedrawMap();
    canvas->ResetBrushing();
}

void MapTree::OnSetAssociateLayer(wxCommandEvent& event)
{
    vector<AssociateLayerInt*> all_layers;
    wxString map_name = map_titles[new_order[select_id]];
    AssociateLayerInt* ml = GetMapLayer(map_name);
    
    if (ml == NULL) {
        // selection is current map
        ml = canvas;
        map_name = canvas->GetName();
    }
    for (int i=0; i<fg_maps.size(); i++) {
        if (fg_maps[i]->GetName() != ml->GetName()) {
            all_layers.push_back(fg_maps[i]);
        }
    }
    if (ml != canvas) {
        all_layers.push_back(canvas);
    }
    for (int i=0; i<bg_maps.size(); i++) {
        if (bg_maps[i]->GetName() != ml->GetName()) {
            all_layers.push_back(bg_maps[i]);
        }
    }
    
    wxString title = _("Set Association Dialog");
    title << " (" << ml->GetName() << ")";
    SetAssociationDlg dlg(this, ml, all_layers, title);
    
    if (dlg.ShowModal() == wxID_OK) {
        bool check_flag = false;
        map<AssociateLayerInt*, Association>::iterator it;
        
        for (int i=0; i<all_layers.size(); i++) {
            AssociateLayerInt* ml = all_layers[i];
            
            // check if loop happens start from this layer
            map<wxString, int> checker;
            vector<AssociateLayerInt*> stack;
            
            stack.push_back(ml);
            
            while (!stack.empty()) {
                AssociateLayerInt* tmp_ml = stack.back();
                stack.pop_back();
                
                if (checker.find(tmp_ml->GetName()) != checker.end()) {
                    check_flag = true;
                    break;
                } else {
                    checker[tmp_ml->GetName()] = true;
                }
                map<AssociateLayerInt*, Association>& asso = tmp_ml->associated_layers;
                for (it = asso.begin(); it!= asso.end(); it++) {                    
                    AssociateLayerInt* layer = it->first;
                    Association& lyr = it->second;
                    wxString key = lyr.second;
                    stack.push_back(layer);
                }
            }
            
            if (check_flag)
                break;
        }
        
        if (check_flag) {
            for (int i=0; i<all_layers.size(); i++) {
                AssociateLayerInt* ml = all_layers[i];
                ml->ClearLayerAssociation();
            }
            wxMessageDialog dlg (this, _("Invalid layer association has been detected, which will cause infinite highlighting loop. Please try to reset highlight association between layers."), _("Error"), wxOK | wxICON_ERROR);
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
        canvas->RedrawMap();
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
        canvas->RedrawMap();
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
            // moving
            if (select_id > -1 ) {
                isLeftMove = true;
                // paint selected label with mouse
                //int label_id = new_order[select_id];
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

    wxString menu_name = _("Layer Full Extent");
    popupMenu->Append(XRCID("MAPTREE_SET_LAYER_EXTENT"), menu_name);
    Connect(XRCID("MAPTREE_SET_LAYER_EXTENT"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnZoomToLayer));
    menu_name = _("Zoom to Selected");
    popupMenu->Append(XRCID("MAPTREE_ZOOM_TO_SELECTED"), menu_name);
    Connect(XRCID("MAPTREE_ZOOM_TO_SELECTED"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnZoomToSelected));
    popupMenu->AppendSeparator();

    menu_name =  _("Set Highlight Association");
    popupMenu->Append(XRCID("MAPTREE_SET_FOREIGN_KEY"), menu_name);
    Connect(XRCID("MAPTREE_SET_FOREIGN_KEY"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnSetAssociateLayer));
    
    popupMenu->Append(XRCID("MAPTREE_CLEAR_FOREIGN_KEY"),
                      _("Clear Highlight Association"));
    Connect(XRCID("MAPTREE_CLEAR_FOREIGN_KEY"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnClearAssociateLayer));

    popupMenu->Append(XRCID("MAPTREE_CHANGE_ASSOCIATELINE_COLOR"), _("Change Associate Line Color"));
    Connect(XRCID("MAPTREE_CHANGE_ASSOCIATELINE_COLOR"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnChangeAssociatelineColor));
    if (ml == NULL) {
        // if it's current map, then no other options other than "Set Highlight"
        PopupMenu(popupMenu, event.GetPosition());
        return;
    }
    
    popupMenu->AppendSeparator();
    popupMenu->Append(XRCID("MAPTREE_CHANGE_FILL_COLOR"), _("Change Fill Color"));
    popupMenu->Append(XRCID("MAPTREE_CHANGE_OUTLINE_COLOR"), _("Change Outline Color"));
    popupMenu->Append(XRCID("MAPTREE_OUTLINE_VISIBLE"), _("Outline Visible"));

    // check menu items
    wxMenuItem* outline = popupMenu->FindItem(XRCID("MAPTREE_OUTLINE_VISIBLE"));
    if (outline && outline->IsCheckable()) {
        outline->SetCheckable(true);
        if (ml->GetPenSize() > 0) outline->Check();
    }
    Connect(XRCID("MAPTREE_CHANGE_FILL_COLOR"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnChangeFillColor));
    Connect(XRCID("MAPTREE_CHANGE_OUTLINE_COLOR"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnChangeOutlineColor));
    Connect(XRCID("MAPTREE_OUTLINE_VISIBLE"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnOutlineVisible));
    
    if (ml->GetShapeType() == Shapefile::POINT_TYP) {
        popupMenu->Append(XRCID("MAPTREE_CHANGE_POINT_RADIUS"),
                          _("Change Point Radius"));
        Connect(XRCID("MAPTREE_CHANGE_POINT_RADIUS"),
                wxEVT_COMMAND_MENU_SELECTED,
                wxCommandEventHandler(MapTree::OnChangePointRadius));
        
    } else {
        popupMenu->Append(XRCID("MAPTREE_BOUNDARY_ONLY"), _("Only Map Boundary"));
        Connect(XRCID("MAPTREE_BOUNDARY_ONLY"), wxEVT_COMMAND_MENU_SELECTED,
                wxCommandEventHandler(MapTree::OnShowMapBoundary));
        wxMenuItem* boundary = popupMenu->FindItem(XRCID("MAPTREE_BOUNDARY_ONLY"));
        if (boundary && boundary->IsCheckable()) {
            boundary->SetCheckable(true);
            if (ml->IsShowBoundary()) boundary->Check();
        }
    }
    popupMenu->AppendSeparator();
    
    popupMenu->Append(XRCID("MAPTREE_REMOVE"), _("Remove"));
    Connect(XRCID("MAPTREE_REMOVE"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(MapTree::OnRemoveMapLayer));
    
    PopupMenu(popupMenu, event.GetPosition());
}

void MapTree::OnDraw(wxDC& dc)
{
    dc.SetFont(*GdaConst::small_font);
    dc.SetPen(*wxBLACK_PEN);
    wxCoord w, h;
    dc.GetSize(&w, &h);
    
    dc.DrawText(_("Map Layer Settings"), 5, 10);
    
    if ( !select_name.IsEmpty() ) {
        DrawLegend(dc, px_switch, move_pos.y, select_name);
    }
    
    for (int i=0; i<new_order.size(); i++) {
        int idx = new_order[i];
        wxString map_name = map_titles[idx];
        if (i == 0) {
            dc.SetTextForeground(*wxBLACK);
        } else {
            wxColour color(80,80,80);
            dc.SetTextForeground(color);
        }
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
    
    // add highlight/all
    AssociateLayerInt* ml_int;
    if (ml) {
        ml_int = ml;
    } else {
        ml_int = canvas;
    }
    int hl_cnt = ml_int->GetHighlightRecords();
    int all_cnt = ml_int->GetNumRecords();
    wxString hl_str = wxString::Format(" (%d/%d selected)", hl_cnt, all_cnt);
    text = text + hl_str;
    dc.DrawText(text, x, y);
    
    // draw switch button
    wxString ds_thumb = "switch-on.png";
    if (ml_int->IsHide()) ds_thumb = "switch-off.png";
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
        if ((x > px_switch) && (x < px_switch + 40) &&
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
        AssociateLayerInt* ml_int;
        if (ml) {
            ml_int = ml;
        } else {
            ml_int = canvas;
        }
        
        if (ml_int) {
            ml_int->SetHide(!ml_int->IsHide());
            canvas->RedrawMap();
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
    if (ml==NULL) {
        
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
        BackgroundMapLayer* lyr = GetMapLayer(name);
        lyr->ResetHighlight();
        if (is_fgmap) {
            new_fg_maps.push_back(lyr);
        } else {
            new_bg_maps.push_back(lyr);
        }
    }
    
    bg_maps = new_bg_maps;
    fg_maps = new_fg_maps;
	is_resize = true;
    canvas->SetForegroundMayLayers(fg_maps);
    canvas->SetBackgroundMayLayers(bg_maps);

    // update selection if selection box is made
    //if (fg_maps.empty() == false) {
    //    canvas->UpdateSelectionPoints();
    //}
    
    // update the canvas
    canvas->RedrawMap();
}

void MapTree::AddCategoryColorToMenu(wxMenu* menu, int cat_clicked)
{
    
}

MapTreeFrame::MapTreeFrame(wxWindow* parent, MapCanvas* _canvas, const wxPoint& pos, const wxSize& size)
: wxFrame(parent, wxID_ANY, _canvas->GetCanvasTitle(), pos, size)
{
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
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
