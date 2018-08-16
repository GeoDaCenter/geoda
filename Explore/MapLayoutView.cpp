//
//  MapLayoutView.cpp
//  GeoDa
//
//  Created by Xun Li on 8/6/18.
//
#include <stdio.h>
#include <wx/dcsvg.h>
#include <wx/dcps.h>

#include "../TemplateLegend.h"
#include "../Explore/MapNewView.h"

#include "MapLayoutView.h"

using namespace std;


CanvasExportSettingDialog::CanvasExportSettingDialog(int w, int h, const wxString & title)
: wxDialog(NULL, -1, title, wxDefaultPosition, wxSize(320, 230))
{
    width = w;
    height = h;
    tc1_value = w;
    tc2_value = h;
    unit_choice = 0;
    aspect_ratio = (double) w / h;
    wxPanel *panel = new wxPanel(this, -1);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    
    wxFlexGridSizer *fgs = new wxFlexGridSizer(3, 3, 9, 25);
    
    wxStaticText *thetitle = new wxStaticText(panel, -1, _("Width:"));
    wxStaticText *author = new wxStaticText(panel, -1, _("Height:"));
    wxStaticText *review = new wxStaticText(panel, -1, _("Resolution(dpi):"));
    
    tc1 = new wxTextCtrl(panel, -1, wxString::Format("%d", w));
    tc2 = new wxTextCtrl(panel, -1, wxString::Format("%d", h));
    tc3 = new wxTextCtrl(panel, -1, "300");
    
    wxString choices[] = {_("pixels"), _("inches"), _("mm")};
    m_unit = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1), 3, choices);
    m_unit->SetSelection(0);
    
    fgs->Add(thetitle);
    fgs->Add(tc1, 1, wxEXPAND);
    fgs->Add(m_unit, 1, wxEXPAND);
    fgs->Add(author);
    fgs->Add(tc2, 1, wxEXPAND);
    fgs->AddSpacer(1);
    fgs->Add(review, 1, wxEXPAND);
    fgs->Add(tc3, 1, wxEXPAND);
    fgs->AddSpacer(1);
    
    fgs->AddGrowableRow(2, 1);
    fgs->AddGrowableCol(1, 1);
    
    wxButton *okButton = new wxButton(this, wxID_OK, _("Ok"),
                                      wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CANCEL, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    
    hbox->Add(okButton, 1);
    hbox->Add(closeButton, 1, wxLEFT, 5);
    
    wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
    panel_v_szr->Add(fgs, 1, wxALL|wxEXPAND, 5);
    panel->SetSizer(panel_v_szr);
    
    vbox->Add(panel, 1, wxALL | wxEXPAND, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    
    SetSizer(vbox);
    SetAutoLayout(true);
    vbox->Fit(this);
    Centre();
    
    tc1->Bind(wxEVT_TEXT, &CanvasExportSettingDialog::OnWidthChange, this);
    tc2->Bind(wxEVT_TEXT, &CanvasExportSettingDialog::OnHeightChange, this);
    tc3->Bind(wxEVT_TEXT, &CanvasExportSettingDialog::OnResolutionChange, this);
    m_unit->Bind(wxEVT_CHOICE, &CanvasExportSettingDialog::OnUnitChange, this);
}

void CanvasExportSettingDialog::OnUnitChange(wxCommandEvent& ev)
{
    // https://www.pixelcalculator.com/
    int unit_sel = m_unit->GetSelection();
    if (unit_sel >= 0) {
        if (unit_choice == unit_sel)
            return;
        
        double x = getTextValue(tc1);
        double y = getTextValue(tc2);
        
        if (unit_choice == 0) {
            if (unit_sel == 1) {
                // pixel2inch
                x = pixel2inch(x);
                y = pixel2inch(y);
            } else if (unit_sel == 2) {
                // pixel2mm
                x = pixel2mm(x);
                y = pixel2mm(y);
            }
            
        } else if (unit_choice == 1) {
            if (unit_sel == 0) {
                // inch2pixel
                x = width;
                y = height;
            } else if (unit_sel == 2) {
                // inch2mm
                x = inch2mm(x);
                y = inch2mm(y);
            }
            
        } else if (unit_choice == 2) {
            if (unit_sel == 0) {
                // mm2pixel
                x = width;
                y = height;
            } else if (unit_sel == 1) {
                // mm2inch
                x = mm2inch(x);
                y = mm2inch(y);
            }
        }
        unit_choice = unit_sel;
        setTextValue(tc1, x);
        setTextValue(tc2, y);
        
    }
}

double CanvasExportSettingDialog::inch2mm(double inch)
{
    return inch * 25.4;
}

double CanvasExportSettingDialog::mm2inch(double mm)
{
    return mm / 25.4;
}

double CanvasExportSettingDialog::pixel2inch(int pixel)
{
    int res = GetMapResolution();
    if (res > 0)
        return pixel * 1.0 / res;
    return 0;
}

double CanvasExportSettingDialog::pixel2mm(int pixel)
{
    int res = GetMapResolution();
    if (res > 0)
        return pixel * 25.4 / res;
    return 0;
}

int CanvasExportSettingDialog::inch2pixel(double inch)
{
    int res = GetMapResolution();
    return inch * res;
}

int CanvasExportSettingDialog::mm2pixel(double mm)
{
    int res = GetMapResolution();
    return mm * res / 25.4;
}

void CanvasExportSettingDialog::OnResolutionChange(wxCommandEvent& ev)
{
    if (unit_choice > 0 ) {
        width = GetMapWidth();
        height = GetMapHeight();
    }
}

double CanvasExportSettingDialog::getTextValue(wxTextCtrl* tc)
{
    wxString val = tc->GetValue();
    double w = 0;
    val.ToDouble(&w);
    return w;
}

void CanvasExportSettingDialog::setTextValue(wxTextCtrl* tc, double val)
{
    wxString tmp;
    if (unit_choice == 0) {
        tmp = wxString::Format("%d", (int)(val) );
        tc->SetLabel(tmp);
    } else {
        tmp = wxString::Format("%.2f", val);
        tc->SetLabel(tmp);
    }
    if (tc == tc1) {
        tc1_value = val;
    } else if (tc == tc2) {
        tc2_value = val;
    }
}

void CanvasExportSettingDialog::OnWidthChange(wxCommandEvent& ev)
{
    wxString val = tc1->GetValue();
    double w;
    if (val.ToDouble(&w)) {
        double h = w / aspect_ratio;
        tc1_value = w;
        setTextValue(tc2, h);
        width = GetMapWidth();
        height = GetMapHeight();
    }
}

void CanvasExportSettingDialog::OnHeightChange(wxCommandEvent& ev)
{
    wxString val = tc2->GetValue();
    double h;
    if (val.ToDouble(&h)) {
        double w = h * aspect_ratio;
        tc2_value = h;
        setTextValue(tc1, w);
        width = GetMapWidth();
        height = GetMapHeight();
    }
}

int CanvasExportSettingDialog::GetMapWidth()
{
    double val = tc1_value;
    if (unit_choice == 1) {
        return inch2pixel(val);
    } else if (unit_choice == 2) {
        return mm2pixel(val);
    }
    return val;
}

int CanvasExportSettingDialog::GetMapHeight()
{
    double val = tc2_value;
    if (unit_choice == 1) {
        return inch2pixel(val);
    } else if (unit_choice == 2) {
        return mm2pixel(val);
    }
    return val;
}

int CanvasExportSettingDialog::GetMapResolution()
{
    wxString val = tc3->GetValue();
    long result = 0;
    if (!val.IsEmpty()) {
        val.ToLong(&result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////


void CanvasLayoutEvtHandler::OnLeftClick(double WXUNUSED(x), double WXUNUSED(y),
                                         int keys, int WXUNUSED(attachment))
{
    wxShape* shape = this->GetShape();
    wxShapeCanvas* canvas = shape->GetCanvas();
    
    wxNode* node = canvas->GetDiagram()->GetShapeList()->GetFirst();
    while (node)
    {
        wxShape* shape = (wxShape*) node->GetData();
        shape->Select(false);
        shape->SetDraggable(false);
        node = node->GetNext();
    }
    
    wxClientDC dc(canvas);
    
    if (shape) {
        shape->Select(!shape->Selected());
        shape->SetHighlight(shape->Selected());
        shape->SetDraggable(shape->Selected());
        int x = shape->GetX();
        int y = shape->GetY();
        canvas->Refresh();
    }
}

void CanvasLayoutEvtHandler::OnRightClick(double WXUNUSED(x), double WXUNUSED(y),
                                          int keys, int WXUNUSED(attachment))
{
    
}

void CanvasLayoutEvtHandler::OnEndSize(double w, double h)
{

}

////////////////////////////////////////////////////////////////////////////////


CanvasLayoutDialog::CanvasLayoutDialog(wxString _project_name, TemplateLegend* _legend, TemplateCanvas* _canvas, const wxString& title, const wxPoint& pos, const wxSize& size)
: wxDialog(NULL, -1, title, pos, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER )
{
    legend_shape = NULL;
    
    project_name = _project_name;
    template_canvas = _canvas;
    template_legend = _legend;
    
    is_resize = false;
    is_initmap = false;
    
    canvas = new wxShapeCanvas(this, wxID_ANY, pos, size);
    canvas->SetBackgroundColour(*wxWHITE);
    canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
    
    diagram = new wxDiagram();
    diagram->SetCanvas(canvas);
    canvas->SetDiagram(diagram);
    
    // map
    map_shape = new wxBitmapShape();
    map_shape->SetCanvas(template_canvas);
    canvas->AddShape(map_shape);
    map_shape->SetDraggable(false);
    map_shape->SetMaintainAspectRatio(true);
    map_shape->Show(true);
    
    CanvasLayoutEvtHandler *evthandler = new CanvasLayoutEvtHandler();
    evthandler->SetShape(map_shape);
    evthandler->SetPreviousHandler(map_shape->GetEventHandler());
    map_shape->SetEventHandler(evthandler);
    
    // legend
    if (template_legend) {
        legend_shape = new wxBitmapShape();
        legend_shape->SetCanvas(template_legend);
        canvas->AddShape(legend_shape);
        
        legend_shape->SetX(50 + legend_shape->GetWidth());
        legend_shape->SetY(50 + legend_shape->GetHeight());
        legend_shape->MakeControlPoints();
        legend_shape->SetMaintainAspectRatio(true);
        legend_shape->Show(true);
        
        CanvasLayoutEvtHandler *evthandler1 = new CanvasLayoutEvtHandler();
        evthandler1->SetShape(legend_shape);
        evthandler1->SetPreviousHandler(legend_shape->GetEventHandler());
        legend_shape->SetEventHandler(evthandler1);
    }
    
    diagram->ShowAll(1);
    
    // ui
    m_cb = new wxCheckBox(this, wxID_ANY, _("Show Legend"));
    m_cb->SetValue(true);
    if (template_legend == NULL) m_cb->Hide();
    
    wxBoxSizer *bmbox = new wxBoxSizer(wxHORIZONTAL);
    //m_bm_scale = new wxCheckBox(this, wxID_ANY, _("Scale Basemap"));
    //m_bm_scale->SetValue(true);
    
    bmbox->Add(m_cb);
    //bmbox->Add(m_bm_scale);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    wxButton *okButton = new wxButton(this, wxID_ANY, _("Save"),
                                      wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CANCEL, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    hbox->Add(okButton, 1, wxLEFT, 5);
    hbox->Add(closeButton, 1, wxLEFT, 5);
    vbox->Add(canvas, 1, wxEXPAND);
    vbox->Add(bmbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    SetSizer(vbox);
    Centre();
    
    Connect(wxEVT_SIZE, wxSizeEventHandler(CanvasLayoutDialog::OnSize));
    Connect(wxEVT_IDLE, wxIdleEventHandler(CanvasLayoutDialog::OnIdle));
    m_cb->Bind(wxEVT_CHECKBOX, &CanvasLayoutDialog::OnShowLegend, this);
    okButton->Bind(wxEVT_BUTTON, &CanvasLayoutDialog::OnSave, this);
}

CanvasLayoutDialog::~CanvasLayoutDialog()
{
    if (legend_shape) delete legend_shape;
    delete map_shape;
    delete diagram;
}

void CanvasLayoutDialog::OnSave(wxCommandEvent &event)
{
    int layout_w = GetWidth();
    int layout_h = GetHeight();
    CanvasExportSettingDialog setting_dlg(layout_w*2, layout_h*2, _("Image Dimension Setting"));

    if (setting_dlg.ShowModal() != wxID_OK) {
        return;
    }
    
    int out_res_x = setting_dlg.GetMapWidth();
    int out_resolution = setting_dlg.GetMapResolution();
    double lo_ar = (double)layout_w / layout_h;
    int out_res_y = out_res_x / lo_ar;
    
    wxString default_fname(project_name);
    wxString filter ="BMP|*.bmp|PNG|*.png|SVG|*.svg";
    int filter_index = 1;
    wxFileDialog dialog(canvas, _("Save Image to File"), wxEmptyString,
                        default_fname, filter,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    dialog.SetFilterIndex(filter_index);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    wxFileName fname = wxFileName(dialog.GetPath());
    wxString str_fname = fname.GetPathWithSep() + fname.GetName();
    
    switch (dialog.GetFilterIndex()) {
        case 0:
        {
            wxLogMessage("BMP selected");
            SaveToImage( str_fname, out_res_x, out_res_y, out_resolution);
        }
            break;
        case 1:
        {
            wxLogMessage("PNG selected");
            SaveToImage( str_fname, out_res_x, out_res_y, out_resolution);
        }
            break;
        
        case 2:
        {
            wxLogMessage("SVG selected");
            SaveToSVG(str_fname, layout_w, layout_h);
        }
            break;
        case 3:
        {
            wxLogMessage("SVG selected");
            SaveToPS(str_fname);
        }
            break;
        default:
        {
        }
            break;
    }
}

void CanvasLayoutDialog::SaveToImage( wxString path, int out_res_x, int out_res_y, int out_resolution, wxBitmapType bm_type)
{
    double lo_scale = (double) out_res_x / GetWidth();
    
    // composer of map and legend
    wxBitmap all_bm(out_res_x, out_res_y);
    wxMemoryDC all_dc(all_bm);
    all_dc.SetBackground(*wxWHITE_BRUSH);
    all_dc.Clear();
    
    // print canvas
    wxSize canvas_sz = template_canvas->GetClientSize();
    int canvas_width = canvas_sz.GetWidth();
    int canvas_height = canvas_sz.GetHeight();
    int lo_map_w = GetShapeWidth(map_shape) * lo_scale;
    int lo_map_h = GetShapeHeight(map_shape) * lo_scale;
    int lo_map_x = GetShapeStartX(map_shape) * lo_scale;
    int lo_map_y = GetShapeStartY(map_shape) * lo_scale;
    double canvas_scale = (double)lo_map_w / canvas_width;
    wxBitmap canvas_bm;
    canvas_bm.CreateScaled(canvas_width, canvas_height, 32, canvas_scale);
    wxMemoryDC canvas_dc(canvas_bm);
    canvas_dc.SetBackground(*wxWHITE_BRUSH);
    canvas_dc.Clear();
    template_canvas->RenderToDC(canvas_dc, lo_map_w, lo_map_h);
    all_dc.DrawBitmap(canvas_bm.ConvertToImage(), lo_map_x, lo_map_y);
    
    // print legend
    if (template_legend && legend_shape && legend_shape->IsShown()) {
        int legend_width = template_legend->GetDrawingWidth();
        int legend_height = template_legend->GetDrawingHeight();
        int lo_leg_w = GetShapeWidth(legend_shape) * lo_scale;
        int lo_leg_h = GetShapeHeight(legend_shape) * lo_scale;
        int lo_leg_x = GetShapeStartX(legend_shape) * lo_scale;
        int lo_leg_y = GetShapeStartY(legend_shape) * lo_scale;
        double leg_scale = (double)lo_leg_w / legend_width;
        wxBitmap leg_bm;
        leg_bm.CreateScaled(legend_width, legend_height, 32, leg_scale);
        wxMemoryDC leg_dc(leg_bm);
        leg_dc.SetBackground(*wxWHITE_BRUSH);
        leg_dc.Clear();
        template_legend->RenderToDC(leg_dc, 1);
        all_dc.DrawBitmap(leg_bm.ConvertToImage(), lo_leg_x, lo_leg_y);
    }
    
    // save composer to file
    if (bm_type == wxBITMAP_TYPE_PNG) path << ".png";
    else if (bm_type == wxBITMAP_TYPE_BMP) path << ".bmp";
    
    wxImage output_img = all_bm.ConvertToImage();
    output_img.SetOption(wxIMAGE_OPTION_RESOLUTION, out_resolution);
    output_img.SaveFile(path, bm_type);
    output_img.Destroy();
}

void CanvasLayoutDialog::SaveToSVG(wxString path, int out_res_x, int out_res_y)
{
    int legend_w = 0;
    double scale = 2.0;
    if (template_legend) {
        legend_w = template_legend->GetDrawingWidth() + 20;
    }
    
    wxSVGFileDC dc(path + ".svg", out_res_x + legend_w + 20, out_res_y);
    dc.SetDeviceOrigin(legend_w, 0);
    template_canvas->RenderToSVG(dc, out_res_x, out_res_y);
    if (template_legend) {
        dc.SetDeviceOrigin(0, 20);
        template_legend->RenderToDC(dc, 1.0);
    }
}

void CanvasLayoutDialog::SaveToPS(wxString path)
{
    int layout_w = GetWidth();
    int layout_h = GetHeight();
    
    wxPrintData printData;
    printData.SetFilename(path + ".ps");
    printData.SetPrintMode(wxPRINT_MODE_FILE);
    wxPostScriptDC dc(printData);
    
    int w, h;
    dc.GetSize(&w, &h);  // A4 paper like?
    wxLogMessage(wxString::Format("wxPostScriptDC GetSize = (%d,%d)", w, h));
    
    if (dc.IsOk()) {
        dc.StartDoc("printing...");
        int paperW, paperH;
        dc.GetSize(&paperW, &paperH);
        
        double marginFactor = 0.03;
        int marginW = (int) (paperW*marginFactor/2.0);
        int marginH = (int) (paperH*marginFactor);
        
        int workingW = paperW - 2*marginW;
        int workingH = paperH - 2*marginH;
        
        int originX = marginW+1; // experimentally obtained tweak
        int originY = marginH+300; // experimentally obtained tweak
        
        // 1/5 use for legend;  4/5 use for map
        int legend_w = 0;
        double scale = 1.0;
        
        if (template_legend) {
            legend_w = workingW * 0.2;
            int legend_orig_w = template_legend->GetDrawingWidth();
            scale = legend_orig_w / (double) legend_w;
            
            dc.SetDeviceOrigin(originX, originY);
            
            template_legend->RenderToDC(dc, scale);
        }
        
        wxSize canvas_sz = template_canvas->GetDrawingSize();
        int picW = canvas_sz.GetWidth();
        int picH = canvas_sz.GetHeight();
        
        int map_w = 0;
        int map_h = 0;
        
        // landscape
        map_w = workingW - legend_w;
        map_h = map_w * picH / picW;
        
        if (picW < picH) {
            // portrait
            map_w = map_w * (map_w / (double) map_h);
            map_h = map_w * picH / picW;
        }
        
        dc.SetDeviceOrigin( originX + legend_w + 100, originY);
        
        template_canvas->RenderToDC(dc, map_w, map_h);
        
        dc.EndDoc();
    }
}

void CanvasLayoutDialog::OnShowLegend(wxCommandEvent &event)
{
    bool show_legend = m_cb->GetValue();
    legend_shape->Select(show_legend);
    legend_shape->Show(show_legend);
    wxClientDC dc(canvas);
    canvas->Redraw(dc);
    is_resize = true;
}

void CanvasLayoutDialog::OnSize(wxSizeEvent &event)
{
    is_resize = true;
    event.Skip();
}

void CanvasLayoutDialog::OnIdle(wxIdleEvent& event)
{
    if (is_resize) {
        if (!is_initmap) {
            is_initmap = true;
            int w, h;
            canvas->GetClientSize(&w, &h);
            double layout_width = w - 100;
            double layout_height = h -  100;
            double new_map_w, new_map_h;
            double lo_ratio = layout_width / layout_height;
            double map_ratio = map_shape->GetWidth() / (double)map_shape->GetHeight();
            if (lo_ratio > map_ratio) {
                new_map_h = layout_height * 1.05;
                new_map_w = map_ratio * new_map_h;
            } else {
                new_map_w = layout_width * 1.05;
                new_map_h = new_map_w / map_ratio;
            }
            map_shape->SetSize(new_map_w, new_map_h);
            map_shape->SetX(w/2.0);
            map_shape->SetY(h/2.0);
            map_shape->Show(true);
        }
        is_resize = false;
        Refresh();
    }
}

int CanvasLayoutDialog::GetWidth()
{
    wxSize sz = canvas->GetClientSize();
    return sz.GetWidth() - 100;
}

int CanvasLayoutDialog::GetHeight()
{
    wxSize sz = canvas->GetClientSize();
    return sz.GetHeight() - 100;
}

double CanvasLayoutDialog::GetShapeWidth(wxBitmapShape* shape)
{
    return shape->GetWidth();
}

double CanvasLayoutDialog::GetShapeHeight(wxBitmapShape* shape)
{
    return shape->GetHeight();
}

double CanvasLayoutDialog::GetShapeStartX(wxBitmapShape* shape)
{
    double x = shape->GetX();
    x = x - shape->GetWidth() /2.0;
    return x - 50;
}

double CanvasLayoutDialog::GetShapeStartY(wxBitmapShape* shape)
{
    double y = shape->GetY();
    y = y - shape->GetHeight() /2.0;
    return y - 50;
}

////////////////////////////////////////////////////////////////////////////////

MapLayoutDialog::MapLayoutDialog(wxString _project_name, TemplateLegend* _legend, TemplateCanvas* _canvas, const wxString& title, const wxPoint& pos, const wxSize& size)
: CanvasLayoutDialog(_project_name, _legend, _canvas, title, pos, size)
{
}

MapLayoutDialog::~MapLayoutDialog()
{
    
}

void MapLayoutDialog::SaveToImage( wxString path, int out_res_x, int out_res_y, int out_resolution, wxBitmapType bm_type )
{
    double lo_scale = (double) out_res_x / GetWidth();
    
    // composer of map and legend
    wxBitmap all_bm(out_res_x, out_res_y);
    wxMemoryDC all_dc(all_bm);
    all_dc.SetBackground(*wxWHITE_BRUSH);
    all_dc.Clear();
    
    // print map
    wxSize map_sz = template_canvas->GetClientSize();
    int map_width = map_sz.GetWidth();
    int map_height = map_sz.GetHeight();
    int lo_map_w = GetShapeWidth(map_shape) * lo_scale;
    int lo_map_h = GetShapeHeight(map_shape) * lo_scale;
    int lo_map_x = GetShapeStartX(map_shape) * lo_scale;
    int lo_map_y = GetShapeStartY(map_shape) * lo_scale;
    double map_scale = (double)lo_map_w / map_width;
    wxBitmap map_bm(lo_map_w, lo_map_h);
    wxMemoryDC map_dc(map_bm);
    map_dc.SetBackground(*wxWHITE_BRUSH);
    map_dc.Clear();
    template_canvas->RenderToDC(map_dc, lo_map_w, lo_map_h);
    all_dc.DrawBitmap(map_bm.ConvertToImage(), lo_map_x, lo_map_y);
    
    // print legend
    if (template_legend && legend_shape && legend_shape->IsShown()) {
        int legend_width = template_legend->GetDrawingWidth();
        int legend_height = template_legend->GetDrawingHeight();
        int lo_leg_w = GetShapeWidth(legend_shape) * lo_scale;
        int lo_leg_h = GetShapeHeight(legend_shape) * lo_scale;
        int lo_leg_x = GetShapeStartX(legend_shape) * lo_scale;
        int lo_leg_y = GetShapeStartY(legend_shape) * lo_scale;
        double leg_scale = (double)lo_leg_w / legend_width;
#ifdef __WIN32__
		wxBitmap leg_bm(lo_leg_w, lo_leg_h);
        wxMemoryDC leg_dc(leg_bm);
        leg_dc.SetBackground(*wxWHITE_BRUSH);
        leg_dc.Clear();
        template_legend->RenderToDC(leg_dc, leg_scale);
        all_dc.DrawBitmap(leg_bm.ConvertToImage(), lo_leg_x, lo_leg_y);
#else
        wxBitmap leg_bm;
        leg_bm.CreateScaled(legend_width, legend_height, 32, leg_scale);
        wxMemoryDC leg_dc(leg_bm);
        leg_dc.SetBackground(*wxWHITE_BRUSH);
        leg_dc.Clear();
        template_legend->RenderToDC(leg_dc, 1);
        all_dc.DrawBitmap(leg_bm.ConvertToImage(), lo_leg_x, lo_leg_y);
#endif
    }
    
    // save composer to file
    if (bm_type == wxBITMAP_TYPE_PNG) path << ".png";
    else if (bm_type == wxBITMAP_TYPE_BMP) path << ".bmp";
    
    wxImage output_img = all_bm.ConvertToImage();
    output_img.SetOption(wxIMAGE_OPTION_RESOLUTION, out_resolution);
    output_img.SaveFile(path, bm_type);
    output_img.Destroy();
}
