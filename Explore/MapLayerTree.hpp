//
//  MapLayerTree.hpp
//  GeoDa
//
//  Created by Xun Li on 8/24/18.
//

#ifndef MapLayerTree_hpp
#define MapLayerTree_hpp

#include <vector>
#include <wx/wx.h>

#include "MapLayer.hpp"

using namespace std;

class MapCanvas;

// Highlight association dialog
class SetAssociationDlg : public wxDialog
{
    static wxString LAYER_LIST_ID;
    vector<wxChoice*> layer_list;
    vector<wxChoice*> field_list;
    vector<wxChoice*> my_field_list;
    vector<wxCheckBox*> conn_list;
    AssociateLayerInt* current_ml;
    vector<AssociateLayerInt*> all_layers;
    
    int GetSelectRow(wxCommandEvent& e);
    bool CheckLayerValid(int row, wxString layer_name);
public:
    SetAssociationDlg(wxWindow* parent,
                      AssociateLayerInt* ml,
                      vector<AssociateLayerInt*>& _all_layers,
                      const wxString& title = _("Set Association Dialog"),
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxSize(480,300));
    void CreateControls(int nrows);
    void Init();
    
    wxString GetCurrentLayerFieldName(int irow);
    wxString GetSelectLayerFieldName(int irow);
    AssociateLayerInt* GetSelectMapLayer(int irow);
    AssociateLayerInt* GetMapLayer(wxString map_name);
    
    void OnLayerSelect(wxCommandEvent& e);
    void OnOk(wxCommandEvent& e);
};

// MapTreeFrame and MapTree are for the pop-up window of multi-layer management
class MapTree: public wxWindow
{
    DECLARE_ABSTRACT_CLASS(MapTree)
    DECLARE_EVENT_TABLE()
    
	bool is_resize;
    bool isDragDropAllowed;
    wxSize maxSize;
    int title_width;
    int title_height;
    int px, py;
    int leg_w;
    int leg_h;
    int leg_pad_x;
    int leg_pad_y;
    int px_switch;
    int opt_menu_cat; // last category added to Legend menu
    
    wxString current_map_title;
    vector<wxString> map_titles;
    
    MapCanvas* canvas;
    vector<BackgroundMapLayer*> bg_maps;
    vector<BackgroundMapLayer*> fg_maps;
    
    bool recreate_labels;
    std::vector<int> new_order;
    bool isLeftDown;
    bool isLeftMove;
    int select_id;
    wxString select_name;
    wxPoint move_pos;
    
public:
    MapTree(wxWindow *parent, MapCanvas* canvas, const wxPoint& pos,
            const wxSize& size);
    virtual ~MapTree();
    
    void Init();
    
protected:
    virtual void OnPaint( wxPaintEvent& event );
	virtual void OnIdle(wxIdleEvent& event);
    virtual void OnDraw(wxDC& dc);
    
    void RemoveAssociationRelationship(BackgroundMapLayer* ml);
    void OnEvent(wxMouseEvent& event);
    void OnRightClick(wxMouseEvent& event);
    void OnChangeAssociatelineColor(wxCommandEvent& event);
    void OnChangeFillColor(wxCommandEvent& event);
    void OnChangeOutlineColor(wxCommandEvent& event);
    void OnChangePointRadius(wxCommandEvent& event);
    void OnOutlineVisible(wxCommandEvent& event);
    void OnShowMapBoundary(wxCommandEvent& event);
    void OnRemoveMapLayer(wxCommandEvent& event);
    void OnSwitchClick(wxMouseEvent& event);
    void OnSpatialJoinCount(wxCommandEvent& event);
    int  GetLegendClick(wxMouseEvent& event);
    int  GetSwitchClick(wxMouseEvent& event);
    int  GetCategoryClick(wxMouseEvent& event);
    void AddCategoryColorToMenu(wxMenu* menu, int cat_clicked);
    void OnSetAssociateLayer(wxCommandEvent& event);
    void OnClearAssociateLayer(wxCommandEvent& event);
    void OnZoomToLayer(wxCommandEvent& event);
    void OnZoomToSelected(wxCommandEvent& event);
    void OnMapLayerChange();
    BackgroundMapLayer* GetMapLayer(wxString name);
    void DrawLegend(wxDC& dc, int x, int y, wxString text);
};

class MapTreeFrame : public wxFrame
{
    MapTree *tree;
    MapCanvas* canvas;
    
public:
    MapTreeFrame(wxWindow* parent, MapCanvas* canvas,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize);
    virtual ~MapTreeFrame();
    
    void OnClose( wxCloseEvent& event );
    void Recreate( );
};

#endif /* MapLayerTree_hpp */
