//
//  MapLayoutView.cpp
//  GeoDa
//
//  Created by Xun Li on 8/6/18.
//
#include <wx/dcsvg.h>
#include <wx/dcps.h>

#include "../TemplateLegend.h"
#include "../Explore/MapNewView.h"

#include "MapLayoutView.h"

MapExportSettingDialog::MapExportSettingDialog(int w, int h, const wxString & title)
: wxDialog(NULL, -1, title, wxDefaultPosition, wxSize(320, 230))
{
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
    
    tc1->Bind(wxEVT_TEXT, &MapExportSettingDialog::OnWidthChange, this);
    tc2->Bind(wxEVT_TEXT, &MapExportSettingDialog::OnHeightChange, this);
    tc3->Bind(wxEVT_TEXT, &MapExportSettingDialog::OnResolutionChange, this);
    m_unit->Bind(wxEVT_CHOICE, &MapExportSettingDialog::OnUnitChange, this);
}

void MapExportSettingDialog::OnUnitChange(wxCommandEvent& ev)
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
                x = inch2pixel(x);
                y = inch2pixel(y);
            } else if (unit_sel == 2) {
                // inch2mm
                x = inch2mm(x);
                y = inch2mm(y);
            }
            
        } else if (unit_choice == 2) {
            if (unit_sel == 0) {
                // mm2pixel
                x = mm2pixel(x);
                y = mm2pixel(y);
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

double MapExportSettingDialog::inch2mm(double inch)
{
    return inch * 25.4;
}

double MapExportSettingDialog::mm2inch(double mm)
{
    return mm / 25.4;
}

double MapExportSettingDialog::pixel2inch(int pixel)
{
    int res = GetMapResolution();
    if (res > 0)
        return pixel * 1.0 / res;
    return 0;
}

double MapExportSettingDialog::pixel2mm(int pixel)
{
    int res = GetMapResolution();
    if (res > 0)
        return pixel * 25.4 / res;
    return 0;
}

int MapExportSettingDialog::inch2pixel(double inch)
{
    int res = GetMapResolution();
    return inch * res;
}

int MapExportSettingDialog::mm2pixel(double mm)
{
    int res = GetMapResolution();
    return mm * res / 25.4;
}

void MapExportSettingDialog::OnResolutionChange(wxCommandEvent& ev)
{
    wxString val = tc3->GetValue();
    long w;
    if (val.ToLong(&w)) {
        
    }
}

double MapExportSettingDialog::getTextValue(wxTextCtrl* tc)
{
    wxString val = tc->GetValue();
    double w = 0;
    val.ToDouble(&w);
    return w;
}

void MapExportSettingDialog::setTextValue(wxTextCtrl*tc, double val)
{
    if (unit_choice == 0) {
        tc->SetValue(wxString::Format("%d", (int)val));
    } else {
        tc->SetValue(wxString::Format("%f", val));
    }
}

void MapExportSettingDialog::OnWidthChange(wxCommandEvent& ev)
{
    wxString val = tc1->GetValue();
    double w;
    if (val.ToDouble(&w)) {
        double h = w / aspect_ratio;
        wxString s_val;
        s_val << h;
        tc2->SetLabel(s_val);
    }
}

void MapExportSettingDialog::OnHeightChange(wxCommandEvent& ev)
{
    wxString val = tc2->GetValue();
    double h;
    if (val.ToDouble(&h)) {
        double w = h * aspect_ratio;
        wxString s_val;
        s_val << w;
        tc1->SetLabel(s_val);
    }
}

int MapExportSettingDialog::GetMapWidth()
{
    double val = getTextValue(tc1);
    if (unit_choice == 1) {
        return inch2pixel(val);
    } else if (unit_choice == 2) {
        return mm2pixel(val);
    }
    return val;
}

int MapExportSettingDialog::GetMapHeight()
{
    double val = getTextValue(tc2);
    if (unit_choice == 1) {
        return inch2pixel(val);
    } else if (unit_choice == 2) {
        return mm2pixel(val);
    }
    return val;
}

int MapExportSettingDialog::GetMapResolution()
{
    wxString val = tc3->GetValue();
    long result = 0;
    if (!val.IsEmpty()) {
        val.ToLong(&result);
    }
    return result;
}

MapLayoutDialog::MapLayoutDialog(wxString _project_name, TemplateLegend* _legend, TemplateCanvas* _canvas, const wxString& title, const wxPoint& pos, const wxSize& size)
: wxDialog(NULL, -1, title, pos, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER )
{
    legend_shape = NULL;
    legend = NULL;
    
    project_name = _project_name;
    template_canvas = _canvas;
    template_legend = _legend;
    map = template_canvas->GetPrintLayer();
    
    if (template_legend) {
        int legend_width = template_legend->GetDrawingWidth(); // 10 pix margin
        int legend_height = template_legend->GetDrawingHeight();
        legend = new wxBitmap(legend_width, legend_height);
        wxMemoryDC dc(*legend);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        template_legend->RenderToDC(dc, 1.0);
    }
    
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
    map_shape->SetBitmap(*map);
    canvas->AddShape(map_shape);
    map_shape->SetDraggable(false);
    map_shape->SetMaintainAspectRatio(true);
    map_shape->Show(true);
    
    MapLayoutEvtHandler *evthandler = new MapLayoutEvtHandler();
    evthandler->SetShape(map_shape);
    evthandler->SetPreviousHandler(map_shape->GetEventHandler());
    map_shape->SetEventHandler(evthandler);
    
    // legend
    if (legend) {
        legend_shape = new wxBitmapShape();
        legend_shape->SetBitmap(*legend);
        canvas->AddShape(legend_shape);
        
        legend_shape->SetX(50 + legend_shape->GetWidth());
        legend_shape->SetY(50 + legend_shape->GetHeight());
        legend_shape->MakeControlPoints();
        legend_shape->SetMaintainAspectRatio(true);
        legend_shape->Show(true);
        
        MapLayoutEvtHandler *evthandler1 = new MapLayoutEvtHandler();
        evthandler1->SetShape(legend_shape);
        evthandler1->SetPreviousHandler(legend_shape->GetEventHandler());
        legend_shape->SetEventHandler(evthandler1);
    }
    
    diagram->ShowAll(1);
    
    // ui
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    m_cb = new wxCheckBox(this, wxID_ANY, _("Show Legend"));
    m_cb->SetValue(true);
    wxButton *okButton = new wxButton(this, wxID_ANY, _("Save"),
                                      wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CANCEL, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    hbox->Add(okButton, 1, wxLEFT, 5);
    hbox->Add(closeButton, 1, wxLEFT, 5);
    vbox->Add(canvas, 1, wxEXPAND);
    vbox->Add(m_cb, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    SetSizer(vbox);
    Centre();
    
    Connect(wxEVT_SIZE, wxSizeEventHandler(MapLayoutDialog::OnSize));
    Connect(wxEVT_IDLE, wxIdleEventHandler(MapLayoutDialog::OnIdle));
    m_cb->Bind(wxEVT_CHECKBOX, &MapLayoutDialog::OnShowLegend, this);
    okButton->Bind(wxEVT_BUTTON, &MapLayoutDialog::OnSave, this);
}

MapLayoutDialog::~MapLayoutDialog()
{
    if (legend_shape) delete legend_shape;
    delete map_shape;
    delete diagram;
}

void MapLayoutDialog::OnSave(wxCommandEvent &event)
{
    int layout_w = GetWidth();
    int layout_h = GetHeight();
    MapExportSettingDialog setting_dlg(layout_w*2, layout_h*2, _("Image Dimension Setting"));

    if (setting_dlg.ShowModal() != wxID_OK) {
        return;
    }
    
    int out_res_x = setting_dlg.GetMapWidth();
    int out_resolution = setting_dlg.GetMapResolution();
    double lo_ar = (double)layout_w / layout_h;
    double lo_scale = (double) out_res_x / layout_w;
    int out_res_y = out_res_x / lo_ar;
    
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
    wxBitmap map_bm;
    map_bm.CreateScaled(lo_map_w, lo_map_h, 32, 1.0);
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
        wxBitmap leg_bm;
        leg_bm.CreateScaled(legend_width, legend_height, 32, leg_scale);
        wxMemoryDC leg_dc(leg_bm);
        leg_dc.SetBackground(*wxWHITE_BRUSH);
        leg_dc.Clear();
        template_legend->RenderToDC(leg_dc, 1);
        all_dc.DrawBitmap(leg_bm.ConvertToImage(), lo_leg_x, lo_leg_y);
    }
    
    // save composer to file
    wxImage output_img = all_bm.ConvertToImage();
    output_img.SetOption(wxIMAGE_OPTION_RESOLUTION, out_resolution);
    
    wxString default_fname(project_name);
    wxString filter ="BMP|*.bmp|PNG|*.png|SVG|*.svg|PostScript|*.ps";
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
            str_fname << ".bmp";
            if ( !output_img.SaveFile(str_fname, wxBITMAP_TYPE_BMP )) {
                wxMessageBox("GeoDa was unable to save the file.");
            }
        }
            break;
        case 1:
        {
            wxLogMessage("PNG selected");
            str_fname << ".png";
            if ( !output_img.SaveFile(str_fname, wxBITMAP_TYPE_PNG )) {
                wxMessageBox("GeoDa was unable to save the file.");
            }
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
    output_img.Destroy();
}

void MapLayoutDialog::SaveToImage( wxString path, int out_res_x, int out_res_y, int out_resolution)
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
    wxBitmap map_bm;
    map_bm.CreateScaled(map_width, map_height, 32, map_scale);
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
        wxBitmap leg_bm;
        leg_bm.CreateScaled(legend_width, legend_height, 32, leg_scale);
        wxMemoryDC leg_dc(leg_bm);
        leg_dc.SetBackground(*wxWHITE_BRUSH);
        leg_dc.Clear();
        template_legend->RenderToDC(leg_dc, 1);
        all_dc.DrawBitmap(leg_bm.ConvertToImage(), lo_leg_x, lo_leg_y);
    }
    
    // save composer to file
    wxImage output_img = all_bm.ConvertToImage();
    output_img.SetOption(wxIMAGE_OPTION_RESOLUTION, out_resolution);
    output_img.SaveFile(path, wxBITMAP_TYPE_PNG);
    output_img.Destroy();
}

void MapLayoutDialog::SaveToSVG(wxString path, int out_res_x, int out_res_y)
{
    wxSize canvas_sz = template_canvas->GetDrawingSize();
    int picW = canvas_sz.GetWidth() + 20;
    int picH = canvas_sz.GetHeight() + 20;
    int legend_w = 0;
    double scale = 2.0;
    if (template_legend) {
        legend_w = template_legend->GetDrawingWidth() + 20;
    }
    
    wxSVGFileDC dc(path + ".svg", picW + legend_w + 20, picH);
    
    template_canvas->RenderToDC(dc, picW + legend_w + 20, picH);
    if (template_legend) {
        template_legend->RenderToDC(dc, scale);
    }
}

void MapLayoutDialog::SaveToPS(wxString path)
{
    int layout_w = GetWidth();
    int layout_h = GetHeight();
    
    wxPrintData printData;
    printData.SetFilename(path + ".ps");
    printData.SetPrintMode(wxPRINT_MODE_FILE);
    printData.SetOrientation(wxLANDSCAPE);
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

void MapLayoutDialog::OnShowLegend(wxCommandEvent &event)
{
    bool show_legend = m_cb->GetValue();
    legend_shape->Select(show_legend);
    legend_shape->Show(show_legend);
    wxClientDC dc(canvas);
    canvas->Redraw(dc);
    is_resize = true;
}

void MapLayoutDialog::OnSize(wxSizeEvent &event)
{
    is_resize = true;
    event.Skip();
}

void MapLayoutDialog::OnIdle(wxIdleEvent& event)
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

int MapLayoutDialog::GetWidth()
{
    wxSize sz = canvas->GetClientSize();
    return sz.GetWidth() - 100;
}

int MapLayoutDialog::GetHeight()
{
    wxSize sz = canvas->GetClientSize();
    return sz.GetHeight() - 100;
}

double MapLayoutDialog::GetShapeWidth(wxBitmapShape* shape)
{
    return shape->GetWidth();
}

double MapLayoutDialog::GetShapeHeight(wxBitmapShape* shape)
{
    return shape->GetHeight();
}

double MapLayoutDialog::GetShapeStartX(wxBitmapShape* shape)
{
    double x = shape->GetX();
    x = x - shape->GetWidth() /2.0;
    return x - 50;
}

double MapLayoutDialog::GetShapeStartY(wxBitmapShape* shape)
{
    double y = shape->GetY();
    y = y - shape->GetHeight() /2.0;
    return y - 50;
}

void MapLayoutEvtHandler::OnLeftClick(double WXUNUSED(x), double WXUNUSED(y),
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

void MapLayoutEvtHandler::OnRightClick(double WXUNUSED(x), double WXUNUSED(y),
                                int keys, int WXUNUSED(attachment))
{
    
}
