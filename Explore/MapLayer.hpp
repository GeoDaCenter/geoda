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
class AssociateLayerInt;

// my_key, key from other layer
typedef pair<wxString, wxString> Association;

class AssociateLayerInt
{
protected:
    bool is_hide;
    
public:
    // primary key : AssociateLayer
    map<AssociateLayerInt*, Association> associated_layers;
    map<AssociateLayerInt*, bool> associated_lines;
    
    AssociateLayerInt() {}
    virtual ~AssociateLayerInt() {}
    
    virtual bool IsCurrentMap() = 0;
    virtual wxString GetName() = 0;
    virtual int  GetNumRecords() = 0;
    virtual int GetHighlightRecords() = 0;
    virtual vector<wxString> GetKeyNames() = 0;
    virtual bool GetKeyColumnData(wxString col_name, vector<wxString>& data) = 0;
    virtual void ResetHighlight() = 0;
    virtual void SetHighlight(int idx) = 0;
    virtual void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas) = 0;
    virtual void SetLayerAssociation(wxString my_key, AssociateLayerInt* layer,
                                     wxString key, bool show_connline=true) = 0;
    virtual bool IsAssociatedWith(AssociateLayerInt* layer) = 0;
    virtual void ClearLayerAssociation() {
        associated_layers.clear();
    }
    virtual GdaShape* GetShape(int i) = 0;
    virtual void SetHide(bool flag) { is_hide = flag; }
    virtual bool IsHide() { return is_hide; }
};


class BackgroundMapLayer : public AssociateLayerInt
{
    int num_obs;
    Shapefile::ShapeType shape_type;
    vector<wxString> field_names;
    vector<wxString> key_names;
    
    bool show_connect_line;
    wxString layer_name;
    wxColour pen_color;
    wxColour brush_color;
    int point_radius;
    int opacity;
    int pen_size;
    bool show_boundary;
    
    
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
    virtual int  GetNumRecords();
    virtual bool GetKeyColumnData(wxString field_name, vector<wxString>& data);
    virtual void SetHighlight(int idx);
    virtual void SetUnHighlight(int idx);
    virtual void ResetHighlight();
    virtual void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas);
    virtual void SetLayerAssociation(wxString my_key, AssociateLayerInt* layer,
                                     wxString key, bool show_connline=true);
    virtual bool IsAssociatedWith(AssociateLayerInt* layer);
    virtual void RemoveAssociatedLayer(AssociateLayerInt* layer);
    virtual int GetHighlightRecords();

    // clone all except shapes and geoms, which are owned by Project* instance;
    // so that different map window can configure the multi-layers
    BackgroundMapLayer* Clone(bool clone_style=false);
    
    vector<GdaShape*>& GetShapes();
    virtual GdaShape* GetShape(int idx);
    
    void CleanMemory();
    wxString GetAssociationText();
    
    void SetName(wxString name);
    virtual wxString GetName();
    
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
