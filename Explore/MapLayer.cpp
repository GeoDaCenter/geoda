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
pen_color(wxColour(80, 80, 80)),
brush_color(wxColour(192, 192, 192, 255)),
point_radius(2),
opacity(255),
pen_size(1),
show_boundary(false),
map_boundary(NULL)
{
    is_hide = true;
}

BackgroundMapLayer::BackgroundMapLayer(wxString name,
                                       OGRLayerProxy* _layer_proxy,
                                       OGRSpatialReference* sr)
  : AssociateLayerInt(),
    layer_name(name),
    layer_proxy(_layer_proxy),
    pen_color(wxColour(80, 80, 80)),
    brush_color(wxColour(192, 192, 192, 255)),
    point_radius(2),
    opacity(255),
    pen_size(1),
    show_boundary(false),
    map_boundary(NULL),
    show_connect_line(false)
{
    is_hide = false;
    num_obs = layer_proxy->GetNumRecords();
    shapes.resize(num_obs, 0);
    shape_type = layer_proxy->GetGdaGeometries(shapes, sr);
    // this is for map boundary only
    shape_type = layer_proxy->GetOGRGeometries(geoms, sr);
    field_names = layer_proxy->GetIntegerFieldNames();
    num_field_names = layer_proxy->GetNumericFieldNames();
    key_names = layer_proxy->GetIntegerAndStringFieldNames();
    layer_proxy->GetExtent(minx, miny, maxx, maxy, sr);
    for (int i=0; i<shapes.size(); i++) {
        highlight_flags.push_back(false);
    }
}

BackgroundMapLayer::~BackgroundMapLayer()
{
    if (map_boundary) {
        delete map_boundary;
    }
    for (int i=0; i<shapes.size(); ++i) {
        delete shapes[i];
    }
}

void BackgroundMapLayer::GetExtent(double &_minx, double &_miny, double &_maxx,
                                   double &_maxy)
{
    _minx = minx;
    _miny = miny;
    _maxx = maxx;
    _maxy = maxy;
}

void BackgroundMapLayer::CleanMemory()
{
    // shapes and geoms will be not deleted until the map destroyed 
    for (int i=0; i<shapes.size(); i++) {
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
    std::map<AssociateLayerInt*, Association>::iterator it;
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
    associated_layers[layer] = std::make_pair(my_key, key);
    associated_lines[layer] = show_connline;
}

bool BackgroundMapLayer::IsAssociatedWith(AssociateLayerInt* layer)
{
    std::map<AssociateLayerInt*, Association>::iterator it;
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

OGRSpatialReference* BackgroundMapLayer::GetSpatialReference()
{
    if (layer_proxy) {
        return layer_proxy->GetSpatialReference();
    }
    return NULL;
}

void BackgroundMapLayer::GetExtentOfSelected(double &_minx, double &_miny, double &_maxx,
                                             double &_maxy)
{
    bool has_selected = false;
    int cnt = 0;
    for (int i=0; i<highlight_flags.size(); i++) {
        if (highlight_flags[i] && geoms[i]) {
            has_selected = true;
            OGRwkbGeometryType eType = wkbFlatten(geoms[i]->getGeometryType());
            if (eType == wkbPoint) {
                OGRPoint* p = (OGRPoint *) geoms[i];
                if (cnt == 0) {
                    _minx = p->getX();
                    _miny = p->getY();
                    _maxx = p->getX();
                    _maxy = p->getY();
                } else {
                    if (p->getX() < _minx) _minx = p->getX();
                    if (p->getY() < _miny) _miny = p->getY();
                    if (p->getX() > _maxx) _maxx = p->getX();
                    if (p->getY() > _maxy) _maxy = p->getY();
                }
            } else {
                OGREnvelope box;
                geoms[i]->getEnvelope(&box);
                if (cnt == 0) {
                    _minx =box.MinX;
                    _miny =box.MinY;
                    _maxx =box.MaxX;
                    _maxy =box.MaxY;
                } else {
                    if (box.MinX < _minx) _minx = box.MinX;
                    if (box.MinY < _miny) _miny = box.MinY;
                    if (box.MaxX > _maxx) _maxx = box.MaxX;
                    if (box.MaxY > _maxy) _maxy = box.MaxY;
                }
            }
            cnt += 1;
        }
    }
    if (has_selected == false) {
        // fall back to layer extent
        GetExtent(_minx, _miny, _maxx, _maxy);
    }
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
    std::map<AssociateLayerInt*, Association>::iterator it;
    for (it=associated_layers.begin(); it!=associated_layers.end();it++) {
        AssociateLayerInt* associated_layer = it->first;
        Association& al = it->second;
        wxString primary_key = al.first;
        wxString associated_key = al.second;
        
        std::vector<wxString> pid(shapes.size());  // e.g. 1 2 3 4 5
        if (primary_key.IsEmpty() == false) {
            GetKeyColumnData(primary_key, pid);
        } else {
            for (int i=0; i<shapes.size(); i++) {
                pid[i] << i;
            }
        }
        std::vector<wxString> fid; // e.g. 2 2 1 1 3 5 4 4
        associated_layer->GetKeyColumnData(associated_key, fid);
        associated_layer->ResetHighlight();

        std::map<wxString, std::vector<wxInt64> > aid_idx;
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
            std::vector<wxInt64>& ids = aid_idx[aid];
            for (int j=0; j<ids.size(); j++) {
                associated_layer->SetHighlight( ids[j] );
            }
        }
        associated_layer->DrawHighlight(dc, map_canvas);
        // draw lines to associated layer
        wxPen pen(this->GetAssociatePenColour());
        dc.SetPen(pen);
        for (int i=0; i<highlight_flags.size(); i++) {
            if (!highlight_flags[i]) {
                continue;
            }
            wxString aid = pid[i];
            if (aid_idx.find(aid) == aid_idx.end()) {
                continue;
            }
            std::vector<wxInt64>& ids = aid_idx[aid];
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
    copy->SetNumericFieldNames(num_field_names);
    copy->associated_layers = associated_layers;
    copy->associated_lines = associated_lines;
    copy->minx = minx;
    copy->miny = miny;
    copy->maxx = maxx;
    copy->maxy = maxy;
    
    if (clone_style) {
        copy->SetAssociatePenColour(associate_pencolor);
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
    for (int i=0; i<shapes.size(); ++i) {
        if (shapes[i]) {
            copy->shapes.push_back(shapes[i]->clone());
        } else {
            copy->shapes.push_back(0);
        }
    }
    // not deep copy
    copy->geoms = geoms;
    copy->layer_proxy = layer_proxy;
    copy->highlight_flags = highlight_flags;
    return copy;
}

int BackgroundMapLayer::GetNumRecords()
{
    return shapes.size();
}

bool BackgroundMapLayer::GetDoubleColumnData(wxString field_name,
                                             std::vector<double>& data)
{
    if (field_name.empty()) return false;
    
    if (data.empty()) {
        data.resize(shapes.size());
    }
    // this function is for finding numeric data from multi-layer
    GdaConst::FieldType type = layer_proxy->GetFieldType(field_name);
    int col_idx = layer_proxy->GetFieldPos(field_name);
    if (type == GdaConst::double_type ||
        type == GdaConst::long64_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] = layer_proxy->data[i]->GetFieldAsDouble(col_idx);
        }
        return true;
    }
    return false;
}

bool BackgroundMapLayer::GetIntegerColumnData(wxString field_name, std::vector<wxInt64>& data)
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

bool BackgroundMapLayer::GetKeyColumnData(wxString field_name, std::vector<wxString>& data)
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
    } else if (type == GdaConst::double_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] << layer_proxy->data[i]->GetFieldAsDouble(col_idx);
        }
    } else if (type == GdaConst::string_type) {
        for (int i=0; i<shapes.size(); ++i) {
            data[i] << layer_proxy->data[i]->GetFieldAsString(col_idx);
        }
    }
    return false;
}

std::vector<wxString> BackgroundMapLayer::GetIntegerFieldNames()
{
    return field_names;
}

std::vector<wxString> BackgroundMapLayer::GetNumericFieldNames()
{
    return num_field_names;
}

void BackgroundMapLayer::SetNumericFieldNames(std::vector<wxString>& names)
{
    num_field_names = names;
}

std::vector<wxString> BackgroundMapLayer::GetKeyNames()
{
    return key_names;
}

void BackgroundMapLayer::SetKeyNames(std::vector<wxString>& names)
{
    key_names = names;
}

void BackgroundMapLayer::SetFieldNames(std::vector<wxString>& names)
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
            map_boundary = OGRLayerProxy::DissolvePolygons(geoms);
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

std::vector<GdaShape*>& BackgroundMapLayer::GetShapes()
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
    if (ml->IsShowBoundary()) {
        if (ml->map_boundary) {
            ml->map_boundary->applyScaleTrans(A);
        }
    } else {
        for (int i=0; i<ml->shapes.size(); i++) {
            if (ml->shapes[i]) {
                ml->shapes[i]->applyScaleTrans(A);
            }
        }
    }
}

void GdaShapeLayer::projectToBasemap(Gda::Basemap *basemap, double scale_factor)
{
    if (ml->IsShowBoundary()) {
        if (ml->map_boundary) {
            ml->map_boundary->projectToBasemap(basemap, scale_factor);
        }
    } else {
        for (int i=0; i<ml->shapes.size(); i++) {
            if (ml->shapes[i]) {
                ml->shapes[i]->projectToBasemap(basemap, scale_factor);
            }
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
                if (ml->shapes[i]) {
                    GdaPoint* pt = (GdaPoint*)ml->shapes[i];
                    pt->radius = ml->GetPointRadius();
                }
            }
            if (ml->shapes[i]) {
                ml->shapes[i]->setPen(pen);
                ml->shapes[i]->setBrush(brush);
                ml->shapes[i]->paintSelf(dc);
            }
        }
    }
}

void GdaShapeLayer::paintSelf(wxGraphicsContext *gc)
{
    // not implemented (using wxGCDC instead)
}
