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

// Interfaces for map layer setting highlight association to any other map layer
// It is implemented by: BackgroundMapLayer and MapCanvas
class AssociateLayerInt
{
protected:
    bool is_hide;
    wxColour associate_pencolor;

public:
    // primary key : AssociateLayer
    map<AssociateLayerInt*, Association> associated_layers;
    map<AssociateLayerInt*, bool> associated_lines;
    
    AssociateLayerInt() : associate_pencolor(wxColour(192, 192, 192)) {}
    virtual ~AssociateLayerInt() {}
    
    virtual bool IsCurrentMap() = 0;
    virtual wxString GetName() = 0;
    virtual int  GetNumRecords() = 0;
    
    virtual vector<wxString> GetKeyNames() = 0;
    virtual bool GetKeyColumnData(wxString col_name, vector<wxString>& data) = 0;
    //virtual bool GetColumnData(wxString col_name, vector<double>& data) = 0;
    
    virtual void ResetHighlight() = 0;
    virtual void SetHighlight(int idx) = 0;
    virtual void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas) = 0;
    virtual int GetHighlightRecords() = 0;
    
    virtual GdaShape* GetShape(int i) = 0;
    //virtual vector<GdaShape*> GetShapes() = 0;
    virtual void GetExtent(double &minx, double &miny, double &maxx,
                           double &maxy) = 0;
    virtual void GetExtentOfSelected(double &minx, double &miny, double &maxx,
                                     double &maxy) = 0;
    virtual OGRSpatialReference* GetSpatialReference() = 0;
    
    virtual void SetLayerAssociation(wxString my_key, AssociateLayerInt* layer,
                                     wxString key, bool show_connline=true) = 0;
    virtual bool IsAssociatedWith(AssociateLayerInt* layer) = 0;
    virtual void ClearLayerAssociation() {
        associated_layers.clear();
        ResetHighlight();
    }
    
    virtual void SetHide(bool flag) { is_hide = flag; }
    virtual bool IsHide() { return is_hide; }

    virtual wxColour GetAssociatePenColour() {
        return associate_pencolor;
    }
    virtual void SetAssociatePenColour(wxColour& color) {
        associate_pencolor = color;
    }
};

// BackgroundMapLayer is similar to MapCanvas, but much simpler
// MapCanvas has vector of BackgroundMapLayers as foreground map layers and
// background layers, which are rendered as GdaShapeLayer:GdaShape
class BackgroundMapLayer : public AssociateLayerInt
{
    int num_obs;
    Shapefile::ShapeType shape_type;
    vector<wxString> field_names;
    vector<wxString> num_field_names;
    vector<wxString> key_names;
    
    bool show_connect_line;
    wxString layer_name;
    wxColour pen_color;
    wxColour brush_color;
    int point_radius;
    int opacity;
    int pen_size;
    bool show_boundary;
    double minx;
    double miny;
    double maxx;
    double maxy;
    
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
    virtual void GetExtent(double &minx, double &miny, double &maxx, double &maxy);
    virtual void GetExtentOfSelected(double &minx, double &miny, double &maxx,
                                     double &maxy);
    virtual OGRSpatialReference* GetSpatialReference();
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

    void SetNumericFieldNames(vector<wxString>& names);
    vector<wxString> GetNumericFieldNames();
    bool GetDoubleColumnData(wxString field_name, vector<double>& data);

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
    virtual void projectToBasemap(Gda::Basemap* basemap, double scale_factor = 1.0);
    virtual void paintSelf(wxDC& dc);
    virtual void paintSelf(wxGraphicsContext* gc);
};
#endif /* MapLayer_hpp */
