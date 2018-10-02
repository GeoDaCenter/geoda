//
//  MapLayer.cpp
//  GeoDa
//
//  Created by Xun Li on 8/24/18.
//

#include "MapNewView.h"
#include "../Project.h"
#include "MapLayer.hpp"

BackgroundMapLayer::BackgroundMapLayer()
: AssociateLayerInt(),
pen_color(wxColour(192, 192, 192)),
brush_color(wxColour(255, 255, 255, 255)),
point_radius(2),
opacity(255),
pen_size(1),
show_boundary(false),
map_boundary(NULL)
{
    is_hide = true;
}

BackgroundMapLayer::BackgroundMapLayer(wxString name, OGRLayerProxy* _layer_proxy, OGRSpatialReference* sr)
: AssociateLayerInt(),
layer_name(name),
layer_proxy(_layer_proxy),
pen_color(wxColour(192, 192, 192)),
brush_color(wxColour(255, 255, 255, 255)),
point_radius(2),
opacity(255),
pen_size(1),
show_boundary(false),
map_boundary(NULL),
show_connect_line(false)
{
    is_hide = false;
    num_obs = layer_proxy->GetNumRecords();
    shape_type = layer_proxy->GetGdaGeometries(shapes, sr);
    // this is for map boundary only
    shape_type = layer_proxy->GetOGRGeometries(geoms, sr);
    field_names = layer_proxy->GetIntegerFieldNames();
    key_names = layer_proxy->GetIntegerAndStringFieldNames();
    for (int i=0; i<shapes.size(); i++) {
        highlight_flags.push_back(false);
    }
}

BackgroundMapLayer::~BackgroundMapLayer()
{
    if (map_boundary) {
        delete map_boundary;
    }
}

void BackgroundMapLayer::CleanMemory()
{
    // shapes and geoms will be not deleted until the map destroyed 
    for (int i=0; i<shapes.size(); i++) {
        delete shapes[i];
        delete geoms[i];
    }
}

bool BackgroundMapLayer::IsCurrentMap()
{
    // not MapCanvas
    return false;
}

wxString BackgroundMapLayer::GetAssociationText()
{
    wxString txt;
    return txt;
}

void BackgroundMapLayer::RemoveAssociatedLayer(AssociateLayerInt* layer)
{
    AssociateLayerInt* del_key = NULL;
    map<AssociateLayerInt*, Association>::iterator it;
    for (it=associated_layers.begin(); it!=associated_layers.end();it++) {
        AssociateLayerInt* asso_layer = it->first;
        if (layer->GetName() == asso_layer->GetName()) {
            del_key = asso_layer;
        }
    }
    if (del_key) {
        associated_layers.erase(del_key);
    }
}

void BackgroundMapLayer::SetLayerAssociation(wxString my_key, AssociateLayerInt* layer, wxString key, bool show_connline)
{
    associated_layers[layer] = make_pair(my_key, key);
    associated_lines[layer] = show_connline;
}

bool BackgroundMapLayer::IsAssociatedWith(AssociateLayerInt* layer)
{
    map<AssociateLayerInt*, Association>::iterator it;
    for (it=associated_layers.begin(); it!=associated_layers.end();it++) {
        AssociateLayerInt* asso_layer = it->first;
        if (layer->GetName() == asso_layer->GetName()) {
            return true;
        }
    }
    return false;
}

GdaShape* BackgroundMapLayer::GetShape(int idx)
{
    return shapes[idx];
}

int BackgroundMapLayer::GetHighlightRecords()
{
    int hl_cnt = 0;
    for (int i=0; i<highlight_flags.size(); i++) {
        if (highlight_flags[i]) {
            hl_cnt += 1;
        }
    }
    return hl_cnt;
}

void BackgroundMapLayer::DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas)
{
    // draw any connected layers
    map<AssociateLayerInt*, Association>::iterator it;
    for (it=associated_layers.begin(); it!=associated_layers.end();it++) {
        AssociateLayerInt* associated_layer = it->first;
        Association& al = it->second;
        wxString primary_key = al.first;
        wxString associated_key = al.second;
        
        vector<wxString> pid(shapes.size());  // e.g. 1 2 3 4 5
        if (primary_key.IsEmpty() == false) {
            GetKeyColumnData(primary_key, pid);
        } else {
            for (int i=0; i<shapes.size(); i++) {
                pid[i] << i;
            }
        }
        vector<wxString> fid; // e.g. 2 2 1 1 3 5 4 4
        associated_layer->GetKeyColumnData(associated_key, fid);
        associated_layer->ResetHighlight();
        
        map<wxString, vector<wxInt64> > aid_idx;
        for (int i=0; i<fid.size(); i++) {
            aid_idx[fid[i]].push_back(i);
        }
        
        for (int i=0; i<highlight_flags.size(); i++) {
            if (!highlight_flags[i]) {
                continue;
            }
            wxString aid = pid[i];
            if (aid_idx.find(aid) == aid_idx.end()) {
                continue;
            }
            vector<wxInt64>& ids = aid_idx[aid];
            for (int j=0; j<ids.size(); j++) {
                associated_layer->SetHighlight( ids[j] );
            }
        }
        associated_layer->DrawHighlight(dc, map_canvas);
        for (int i=0; i<highlight_flags.size(); i++) {
            if (!highlight_flags[i]) {
                continue;
            }
            wxString aid = pid[i];
            if (aid_idx.find(aid) == aid_idx.end()) {
                continue;
            }
            vector<wxInt64>& ids = aid_idx[aid];
            for (int j=0; j<ids.size(); j++) {
                if (associated_lines[associated_layer] && !associated_layer->IsHide()) {
                    dc.DrawLine(shapes[i]->center, associated_layer->GetShape(ids[j])->center);
                }
            }
        }
    }
    
    // draw self highlight
    for (int i=0; i<highlight_flags.size(); i++) {
        if (highlight_flags[i] && IsHide() == false) {
            shapes[i]->paintSelf(dc);
        }
    }
}

void BackgroundMapLayer::SetHighlight(int idx)
{
    highlight_flags[idx] = true;
}

void BackgroundMapLayer::SetUnHighlight(int idx)
{
    highlight_flags[idx] = false;
}

void BackgroundMapLayer::ResetHighlight()
{
    for (int i=0; i<shapes.size(); i++) {
        highlight_flags[i] = false;
    }
}

void BackgroundMapLayer::SetName(wxString name)
{
    layer_name = name;
}

wxString BackgroundMapLayer::GetName()
{
    return layer_name;
}

BackgroundMapLayer* BackgroundMapLayer::Clone(bool clone_style)
{
    BackgroundMapLayer* copy =  new BackgroundMapLayer();
    copy->SetName(layer_name);
    copy->SetShapeType(shape_type);
    copy->SetKeyNames(key_names);
    copy->SetFieldNames(field_names);
    copy->associated_layers = associated_layers;
    copy->associated_lines = associated_lines;

    if (clone_style) {
        copy->SetPenColour(pen_color);
        copy->SetBrushColour(brush_color);
        copy->SetPointRadius(point_radius);
        copy->SetOpacity(opacity);
        copy->SetPenSize(pen_size);
        copy->SetShowBoundary(show_boundary);
        copy->SetHide(is_hide);
    }
    // deep copy
    copy->highlight_flags = highlight_flags;
    if (map_boundary) {
        copy->map_boundary = map_boundary->clone();
    }
    // not deep copy 
    copy->shapes = shapes;
    copy->geoms = geoms;
    copy->layer_proxy = layer_proxy;
    copy->highlight_flags = highlight_flags;
    return copy;
}

int BackgroundMapLayer::GetNumRecords()
{
    return shapes.size();
}

bool BackgroundMapLayer::GetIntegerColumnData(wxString field_name, vector<wxInt64>& data)
{
    if (data.empty()) {
        data.resize(shapes.size());
    }
    // this function is for finding IDs of multi-layer
    GdaConst::FieldType type = layer_proxy->GetFieldType(field_name);
    int col_idx = layer_proxy->GetFieldPos(field_name);
    if (type == GdaConst::long64_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] = layer_proxy->data[i]->GetFieldAsInteger64(col_idx);
        }
        return true;
    } else if (type == GdaConst::string_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] = layer_proxy->data[i]->GetFieldAsInteger64(col_idx);
        }
    }
    return false;
}

bool BackgroundMapLayer::GetKeyColumnData(wxString field_name, vector<wxString>& data)
{
    if (data.empty()) {
        data.resize(shapes.size());
    }
    // this function is for finding IDs of multi-layer
    if (field_name.IsEmpty() || field_name == "(Use Sequences)") {
        for (int i=0; i<shapes.size(); i++) {
            data[i] << i;
        }
        return true;
    }
    GdaConst::FieldType type = layer_proxy->GetFieldType(field_name);
    int col_idx = layer_proxy->GetFieldPos(field_name);
    if (type == GdaConst::long64_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] << layer_proxy->data[i]->GetFieldAsInteger64(col_idx);
        }
        return true;
    } else if (type == GdaConst::string_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] << layer_proxy->data[i]->GetFieldAsString(col_idx);
        }
    }
    return false;
}

vector<wxString> BackgroundMapLayer::GetIntegerFieldNames()
{
    return field_names;
}

vector<wxString> BackgroundMapLayer::GetKeyNames()
{
    return key_names;
}

void BackgroundMapLayer::SetKeyNames(vector<wxString>& names)
{
    key_names = names;
}

void BackgroundMapLayer::SetFieldNames(vector<wxString>& names)
{
    field_names = names;
}

void BackgroundMapLayer::SetShapeType(Shapefile::ShapeType type)
{
    shape_type = type;
}

Shapefile::ShapeType BackgroundMapLayer::GetShapeType()
{
    return shape_type;
}

void BackgroundMapLayer::drawLegend(wxDC& dc, int x, int y, int w, int h)
{
    wxPen pen(pen_color);
    int r = brush_color.Red();
    int g = brush_color.Green();
    int b = brush_color.Blue();
    wxColour b_color(r,g,b,opacity);
    dc.SetPen(pen);
    dc.SetBrush(b_color);
    dc.DrawRectangle(x, y, w, h);
}

void BackgroundMapLayer::SetOpacity(int val)
{
    opacity = val;
}

int BackgroundMapLayer::GetOpacity()
{
    return opacity;
}

int BackgroundMapLayer::GetPenSize()
{
    return pen_size;
}

void BackgroundMapLayer::SetPenSize(int val)
{
    pen_size = val;
}

void BackgroundMapLayer::SetPenColour(wxColour &color)
{
    pen_color = color;
}

wxColour BackgroundMapLayer::GetPenColour()
{
    return pen_color;
}

void BackgroundMapLayer::SetBrushColour(wxColour &color)
{
    brush_color = color;
}

wxColour BackgroundMapLayer::GetBrushColour()
{
    return brush_color;
}

void BackgroundMapLayer::ShowBoundary(bool show)
{
    show_boundary = show;
    if (show) {
        if (map_boundary == NULL) {
            map_boundary = OGRLayerProxy::GetMapBoundary(geoms);
        }
    }
}

void BackgroundMapLayer::SetShowBoundary(bool flag)
{
    show_boundary = flag;
}

bool BackgroundMapLayer::IsShowBoundary()
{
    return show_boundary;
}

void BackgroundMapLayer::SetPointRadius(int radius)
{
    point_radius = radius;
}

int BackgroundMapLayer::GetPointRadius()
{
    return point_radius;
}

vector<GdaShape*>& BackgroundMapLayer::GetShapes()
{
    return shapes;
}







GdaShapeLayer::GdaShapeLayer(wxString _name, BackgroundMapLayer* _ml)
: name(_name), ml(_ml)
{
}

GdaShapeLayer::~GdaShapeLayer()
{
}

GdaShape* GdaShapeLayer::clone()
{
    // not implemented
    return NULL;
}

void GdaShapeLayer::Offset(double dx, double dy)
{
    
}

void GdaShapeLayer::Offset(int dx, int dy)
{
    
}

void GdaShapeLayer::applyScaleTrans(const GdaScaleTrans &A)
{
    if (ml->map_boundary) {
        ml->map_boundary->applyScaleTrans(A);
    } else {
        for (int i=0; i<ml->shapes.size(); i++) {
            ml->shapes[i]->applyScaleTrans(A);
        }
    }
}

void GdaShapeLayer::projectToBasemap(GDA::Basemap *basemap, double scale_factor)
{
    if (ml->map_boundary) {
        ml->map_boundary->projectToBasemap(basemap, scale_factor);
    } else {
        for (int i=0; i<ml->shapes.size(); i++) {
            ml->shapes[i]->projectToBasemap(basemap, scale_factor);
        }
    }
}

void GdaShapeLayer::paintSelf(wxDC &dc)
{
    if (ml->IsHide() == false) {
        wxPen pen(ml->GetPenColour(), ml->GetPenSize());
        if (ml->GetPenSize() == 0 ) {
            pen.SetColour(ml->GetBrushColour());
        }
        
        wxBrush brush(ml->GetBrushColour());
        
        if (ml->IsShowBoundary()) {
            ml->map_boundary->paintSelf(dc);
            return;
        }
        
        for (int i=0; i<ml->shapes.size(); i++) {
            if (ml->GetShapeType() == Shapefile::POINT_TYP) {
                GdaPoint* pt = (GdaPoint*)ml->shapes[i];
                pt->radius = ml->GetPointRadius();
            }
            ml->shapes[i]->setPen(pen);
            ml->shapes[i]->setBrush(brush);
            ml->shapes[i]->paintSelf(dc);
        }
    }
}

void GdaShapeLayer::paintSelf(wxGraphicsContext *gc)
{
    // not implemented (using wxGCDC instead)
}
