//
//  MapLayerTree.hpp
//  GeoDa
//
//  Created by Xun Li on 8/24/18.
//

#ifndef MapLayerTree_hpp
#define MapLayerTree_hpp

#include <wx/wx.h>

#include "MapLayer.hpp"

class MapCanvas;

class SetAssociationDlg : public wxDialog
{
    wxString current_map_title;
    wxChoice* layer_list;
    wxChoice* field_list;
    wxChoice* my_field_list;
    BackgroundMapLayer* current_ml;
    vector<BackgroundMapLayer*> bg_maps;
    vector<BackgroundMapLayer*> fg_maps;
    vector<wxString> current_map_fieldnames;
    
public:
    SetAssociationDlg(wxWindow* parent,
                      wxString current_map_title,
                      vector<wxString>& current_map_fieldnames,
                     BackgroundMapLayer* ml,
                     vector<BackgroundMapLayer*>& bg_maps,
                     vector<BackgroundMapLayer*>& fg_maps,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxSize(400,300));
    void Init();
    wxString GetCurrentLayerFieldName();
    wxString GetSelectLayerFieldName();
    BackgroundMapLayer* GetSelectMapLayer();
    
    void OnLayerSelect(wxCommandEvent& e);
    BackgroundMapLayer* GetMapLayer(wxString map_name);
};

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
    void OnSetPrimaryKey(wxCommandEvent& event);
    void OnSetAssociateLayer(wxCommandEvent& event);
    void OnMapLayerChange();
    BackgroundMapLayer* GetMapLayer(wxString name);
    void DrawLegend(wxDC& dc, int x, int y, wxString text);
};


class MapTreeDlg : public wxDialog
{
public:
    MapTreeDlg(wxWindow* parent, MapCanvas* canvas,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize);
    virtual ~MapTreeDlg();
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
