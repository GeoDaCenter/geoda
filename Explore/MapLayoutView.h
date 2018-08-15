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



class CanvasExportSettingDialog : public wxDialog
{
    wxTextCtrl *tc1;
    wxTextCtrl *tc2;
    wxTextCtrl *tc3;
    wxChoice* m_unit;
    int    unit_choice;
    double aspect_ratio;
    double tc1_value;
    double tc2_value;
    int width;
    int height;
    
    double inch2mm(double inch);
    double mm2inch(double mm);
    double pixel2inch(int pixel);
    double pixel2mm(int pixel);
    int    inch2pixel(double inch);
    int    mm2pixel(double mm);
    double getTextValue(wxTextCtrl* tc);
    void   setTextValue(wxTextCtrl*tc, double val);
    
public:
    CanvasExportSettingDialog(int w, int h, const wxString& title);
    
    int GetMapWidth();
    int GetMapHeight();
    int GetMapResolution();
    
    void OnWidthChange(wxCommandEvent& ev);
    void OnHeightChange(wxCommandEvent& ev);
    void OnResolutionChange(wxCommandEvent& ev);
    void OnUnitChange(wxCommandEvent& ev);
};

class CanvasLayoutEvtHandler: public wxShapeEvtHandler
{
public:
    CanvasLayoutEvtHandler(wxShapeEvtHandler *prev = NULL, wxShape *shape = NULL,
                           const wxString& lab = wxEmptyString)
    : wxShapeEvtHandler(prev, shape) { }
    
    ~CanvasLayoutEvtHandler(void) {}
    
    void OnLeftClick(double x, double y, int keys = 0, int attachment = 0);
    void OnRightClick(double x, double y, int keys = 0, int attachment = 0);
    virtual void OnEndSize(double w, double h);
};

class CanvasLayoutDialog : public wxDialog
{
protected:
    wxDiagram * diagram;
    wxShapeCanvas *canvas;
    
    wxCheckBox* m_cb;
    wxCheckBox* m_bm_scale;
    
    bool is_resize;
    bool is_initmap;
    
    TemplateCanvas* template_canvas;
    TemplateLegend* template_legend;
    
    wxString project_name;
    
public:
    CanvasLayoutDialog(wxString project_name, TemplateLegend* _legend, TemplateCanvas* _map, const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~CanvasLayoutDialog();
    
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
    
    virtual void SaveToImage( wxString path, int out_res_x, int out_res_y, int out_resolution, wxBitmapType bm_type = wxBITMAP_TYPE_PNG);
    void SaveToSVG(wxString path, int out_res_x, int out_res_y);
    void SaveToPS(wxString path);
};

class MapLayoutDialog : public CanvasLayoutDialog
{
public:
    MapLayoutDialog(wxString project_name, TemplateLegend* _legend, TemplateCanvas* _map, const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~MapLayoutDialog();
    
    virtual void SaveToImage( wxString path, int out_res_x, int out_res_y, int out_resolution, wxBitmapType bm_type = wxBITMAP_TYPE_PNG);
};

#endif /* MapLayoutView_h */
