//
//  MapLayer.hpp
//  GeoDa
//
//  Created by Xun Li on 8/24/18.
//

#ifndef MapLayer_hpp
#define MapLayer_hpp

#include <wx/wx.h>
#include <vector>

#include "../GdaShape.h"
#include "../ShapeOperations/OGRLayerProxy.h"

class BackgroundMapLayer
{
    Shapefile::ShapeType shape_type;
    vector<wxString> field_names;
    vector<wxString> key_names;
    wxString primary_key;
    wxString foreign_key;
    BackgroundMapLayer* foreign_layer;
    
    wxString layer_name;
    wxColour pen_color;
    wxColour brush_color;
    int point_radius;
    int opacity;
    int pen_size;
    bool show_boundary;
    bool is_hide;
    
public:
    OGRLayerProxy* layer_proxy;
    GdaPolygon* map_boundary;
    vector<GdaShape*> shapes;
    vector<OGRGeometry*> geoms;
    vector<bool> highlight_flags;
    
    BackgroundMapLayer();
    BackgroundMapLayer(wxString name, OGRLayerProxy* layer_proxy, OGRSpatialReference* sr);
    ~BackgroundMapLayer();
    // clone all except shapes and geoms, which are owned by Project* instance;
    // so that different map window can configure the multi-layers
    BackgroundMapLayer* Clone(bool clone_style=false);
    
    void CleanMemory();
    bool HasForeignKey();
    BackgroundMapLayer* GetForeignLayer();
    void SetPrimaryKey(wxString key);
    void SetForeignKey(BackgroundMapLayer* layer, wxString key);
    void SetHighlight(int idx);
    void SetUnHighlight(int idx);
    void DrawHighlight(wxDC& dc);
    void SetName(wxString name);
    void SetHide(bool flag);
    bool IsHide();
    void SetPenColour(wxColour& color);
    void SetBrushColour(wxColour& color);
    void SetPointRadius(int radius);
    void SetOpacity(int opacity);
    void SetPenSize(int size);
    void SetShapeType(Shapefile::ShapeType type);
    void ShowBoundary(bool show);
    void SetShowBoundary(bool flag);
    wxColour GetBrushColour();
    wxColour GetPenColour();
    int GetPenSize();
    int GetPointRadius();
    int GetOpacity();
    bool IsShowBoundary();
    wxString GetName();
    wxString GetPrimaryKey();
    wxString GetForeignKey();
    int GetNumRecords();
    vector<wxString> GetIntegerFieldNames();
    vector<wxString> GetKeyNames();
    vector<GdaShape*>& GetShapes();
    Shapefile::ShapeType GetShapeType();
    bool GetIntegerColumnData(wxString field_name, vector<wxInt64>& data);
    void drawLegend(wxDC& dc, int x, int y, int w, int h);    
};

class GdaShapeLayer : public GdaShape  {
    wxString name;
    BackgroundMapLayer* ml;
    
public:
    GdaShapeLayer(wxString name, BackgroundMapLayer* ml);
    ~GdaShapeLayer();
    
    virtual GdaShape* clone();
    virtual void Offset(double dx, double dy);
    virtual void Offset(int dx, int dy);
    virtual void applyScaleTrans(const GdaScaleTrans& A);
    virtual void projectToBasemap(GDA::Basemap* basemap, double scale_factor = 1.0);
    virtual void paintSelf(wxDC& dc);
    virtual void paintSelf(wxGraphicsContext* gc);
};


#endif /* MapLayer_hpp */
