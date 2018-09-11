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

class MapCanvas;

class BackgroundMapLayer
{
    int num_obs;
    Shapefile::ShapeType shape_type;
    vector<wxString> field_names;
    vector<wxString> key_names;
    
    wxString primary_key;
    wxString associated_key;
    BackgroundMapLayer* associated_layer;

    wxString mapcanvas_key;
    wxString asso_mapcanvas_key;
    
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
    void SetMapAssociation(wxString my_key, wxString map_key);
    void SetLayerAssociation(wxString my_key, BackgroundMapLayer* layer, wxString key);
    wxString GetAssociationText();
    void RemoveAssociationRelationship(BackgroundMapLayer* ml);
    
    BackgroundMapLayer* GetAssociatedLayer();
    void SetAssociatedLayer(BackgroundMapLayer* val);
    
    void SetPrimaryKey(wxString key);
    wxString GetPrimaryKey();
    
    void SetAssociatedKey(wxString key);
    wxString GetAssociatedKey();
    
    void SetMapcanvasKey(wxString name);
    void SetAssoMapcanvasKey(wxString name);

    void SetName(wxString name);
    wxString GetName();
    
    void SetHide(bool flag);
    bool IsHide();
    void SetPenColour(wxColour& color);
    wxColour GetPenColour();
    void SetBrushColour(wxColour& color);
    wxColour GetBrushColour();
    void SetPointRadius(int radius);
    int GetPointRadius();
    void SetOpacity(int opacity);
    int GetOpacity();
    void SetPenSize(int size);
    int GetPenSize();
    void SetShapeType(Shapefile::ShapeType type);
    Shapefile::ShapeType GetShapeType();
    void ShowBoundary(bool show);
    bool IsShowBoundary();
    void SetShowBoundary(bool flag);
    void SetKeyNames(vector<wxString>& names);
    vector<wxString> GetKeyNames();
    void SetFieldNames(vector<wxString>& names);
    vector<wxString> GetIntegerFieldNames();
    int GetNumRecords();
    vector<GdaShape*>& GetShapes();
    bool GetIntegerColumnData(wxString field_name, vector<wxInt64>& data);
    bool GetKeyColumnData(wxString field_name, vector<wxString>& data);
    void SetHighlight(int idx);
    void SetUnHighlight(int idx);
    void ResetHighlight();
    void drawLegend(wxDC& dc, int x, int y, int w, int h);
    void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas);
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
