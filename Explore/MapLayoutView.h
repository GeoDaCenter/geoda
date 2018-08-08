//
//  MapLayoutView.hpp
//  GeoDa
//
//  Created by Xun Li on 8/6/18.
//

#ifndef MapLayoutView_h
#define MapLayoutView_h

#include "wx/wx.h"
#include "../ogl/ogl.h"

class MapCanvas;
class TemplateLegend;

class MapExportSettingDialog : public wxDialog
{
    wxTextCtrl *tc1;
    wxTextCtrl *tc2;
    wxTextCtrl *tc3;
    double aspect_ratio;
    
public:
    MapExportSettingDialog(int w, int h, const wxString& title);
    
    int GetMapWidth();
    int GetMapHeight();
    int GetMapResolution();
    
    void OnWidthChange(wxCommandEvent& ev);
    void OnHeightChange(wxCommandEvent& ev);
};

class MapLayoutDialog : public wxDialog
{
    wxDiagram * diagram;
    wxShapeCanvas *canvas;
    
    wxBitmap* legend;
    wxBitmap* map;
    
    wxCheckBox* m_cb;
    
    bool is_resize;
    bool is_initmap;
    
    TemplateCanvas* template_canvas;
    TemplateLegend* template_legend;
    
public:
    MapLayoutDialog(TemplateLegend* _legend, TemplateCanvas* _map, const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MapLayoutDialog();
    
    wxBitmapShape * map_shape;
    wxBitmapShape * legend_shape;
    
    int GetWidth();
    int GetHeight();
    
    double GetShapeWidth(wxBitmapShape* shape);
    double GetShapeHeight(wxBitmapShape* shape);
    double GetShapeStartX(wxBitmapShape* shape);
    double GetShapeStartY(wxBitmapShape* shape);
    
    void OnSave( wxCommandEvent& event);
    void OnSize( wxSizeEvent& event );
    void OnIdle( wxIdleEvent& event );
    void OnShowLegend( wxCommandEvent& event );
};

class MapLayoutEvtHandler: public wxShapeEvtHandler
{
public:
    MapLayoutEvtHandler(wxShapeEvtHandler *prev = NULL, wxShape *shape = NULL,
                 const wxString& lab = wxEmptyString)
    : wxShapeEvtHandler(prev, shape) { }
    
    ~MapLayoutEvtHandler(void) {}
    
    void OnLeftClick(double x, double y, int keys = 0, int attachment = 0);
    void OnRightClick(double x, double y, int keys = 0, int attachment = 0);
};

#endif /* MapLayoutView_h */
