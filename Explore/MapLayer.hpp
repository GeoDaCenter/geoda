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
#include <map>

#include "../GdaShape.h"
#include "../ShapeOperations/OGRLayerProxy.h"

using namespace std;

class MapCanvas;

class AssociateLayerInt
{
public:
    AssociateLayerInt() {}
    virtual ~AssociateLayerInt() {}
    
    virtual bool IsCurrentMap() = 0;
    virtual wxString GetName() = 0;
    virtual int  GetNumRecords() = 0;
    virtual bool GetKeyColumnData(wxString col_name, vector<wxString>& data) = 0;
    virtual void ResetHighlight() = 0;
    virtual void SetHighlight(int idx) = 0;
    virtual void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas) = 0;
};

typedef pair<wxString, AssociateLayerInt* > AssociateLayer;

class BackgroundMapLayer : public AssociateLayerInt
{
    int num_obs;
    Shapefile::ShapeType shape_type;
    vector<wxString> field_names;
    vector<wxString> key_names;
    
    // primary key : AssociateLayer
    map<wxString, AssociateLayer> associated_layers;
    
    wxString primary_key;
    wxString associated_key;
    AssociateLayerInt* associated_layer;
    bool show_connect_line;
    
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
    virtual ~BackgroundMapLayer();
    
    virtual bool IsCurrentMap();
    virtual int GetNumRecords();
    virtual bool GetKeyColumnData(wxString field_name, vector<wxString>& data);
    virtual void SetHighlight(int idx);
    virtual void SetUnHighlight(int idx);
    virtual void ResetHighlight();
    virtual void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas);

    // clone all except shapes and geoms, which are owned by Project* instance;
    // so that different map window can configure the multi-layers
    BackgroundMapLayer* Clone(bool clone_style=false);
    
    vector<GdaShape*>& GetShapes();
    GdaShape* GetShape(int idx);
    
    void CleanMemory();
    void SetMapAssociation(wxString my_key, wxString map_key);
    void SetLayerAssociation(wxString my_key, BackgroundMapLayer* layer,
                             wxString key, bool show_connline=true);
    wxString GetAssociationText();
    void RemoveAssociationRelationship(BackgroundMapLayer* ml);
    
    AssociateLayerInt* GetAssociatedLayer();
    void SetAssociatedLayer(AssociateLayerInt* val);
    
    void SetPrimaryKey(wxString key);
    wxString GetPrimaryKey();
    
    void SetAssociatedKey(wxString key);
    wxString GetAssociatedKey();
    
    void SetMapcanvasKey(wxString name);
    void SetAssoMapcanvasKey(wxString name);

    void SetName(wxString name);
    virtual wxString GetName();
    
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
