//
//  MapLayoutView.cpp
//  GeoDa
//
//  Created by Xun Li on 8/6/18.
//

#include "MapLayoutView.h"


MapLayoutDialog::MapLayoutDialog(wxBitmap* _legend, wxBitmap* _map, const wxString& title, const wxPoint& pos, const wxSize& size)
: wxDialog(NULL, -1, title, pos, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER )
{
    legend = _legend;
    map    = _map;
    double layout_width = size.GetWidth() * 0.9;
    double layout_height = size.GetHeight() * 0.9;
    
    double scale_x = map->GetWidth() / (size.GetWidth() * 0.9);
    double scale_y = map->GetHeight() / (size.GetHeight() * 0.9);
    double scale = scale_x > scale_y ? scale_x : scale_y;
    
    canvas = new wxShapeCanvas(this, wxID_ANY, pos, size);
    
    canvas->SetBackgroundColour(*wxWHITE);
    canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
    
    diagram = new wxDiagram();
    diagram->SetCanvas(canvas);
    canvas->SetDiagram(diagram);
    
    // map
    map_shape = new wxBitmapShape();
    map_shape->SetScale(scale);
    map_shape->SetBitmap(*map);
    canvas->AddShape(map_shape);
    
    //map_shape->SetMaintainAspectRatio(true);
    map_shape->SetX(map_shape->GetWidth()/2.0);
    map_shape->SetY(map_shape->GetHeight()/2.0);
    map_shape->Show(true);
    
    // legend
    legend_shape = new wxBitmapShape();
    legend_shape->SetBitmap(*legend);
    canvas->AddShape(legend_shape);
    
    legend_shape->SetX(50 + legend_shape->GetWidth()/2.0);
    legend_shape->SetY(20 + legend_shape->GetHeight());
    legend_shape->MakeControlPoints();
    legend_shape->Show(true);
    
    diagram->ShowAll(1);
    
    MapLayoutEvtHandler *evthandler = new MapLayoutEvtHandler();
    evthandler->SetShape(map_shape);
    evthandler->SetPreviousHandler(map_shape->GetEventHandler());
    map_shape->SetEventHandler(evthandler);
    MapLayoutEvtHandler *evthandler1 = new MapLayoutEvtHandler();
    evthandler1->SetShape(legend_shape);
    evthandler1->SetPreviousHandler(legend_shape->GetEventHandler());
    legend_shape->SetEventHandler(evthandler1);
    
    
    // ui
    //wxPanel *panel = new wxPanel(this, -1);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    
    m_cb = new wxCheckBox(this, wxID_ANY, _("Show Legend"));
    m_cb->SetValue(true);
    
    wxButton *okButton = new wxButton(this, wxID_OK, _("Ok"),
                                      wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CLOSE, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    
    hbox->Add(m_cb, 1);
    hbox->Add(okButton, 1, wxLEFT, 5);
    hbox->Add(closeButton, 1, wxLEFT, 5);
    
    vbox->Add(canvas, 1, wxEXPAND);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    
    SetSizer(vbox);
    
    Centre();
}

MapLayoutDialog::~MapLayoutDialog()
{
    delete map_shape;
    delete legend_shape;
    delete diagram;
}

int MapLayoutDialog::GetWidth()
{
    wxSize sz = canvas->GetClientSize();
    return sz.GetWidth();
}

int MapLayoutDialog::GetHeight()
{
    wxSize sz = canvas->GetClientSize();
    return sz.GetHeight();
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
    return x;
}

double MapLayoutDialog::GetShapeStartY(wxBitmapShape* shape)
{
    double y = shape->GetY();
    y = y - shape->GetHeight() /2.0;
    return y;
}

void MapLayoutEvtHandler::OnLeftClick(double WXUNUSED(x), double WXUNUSED(y),
                               int keys, int WXUNUSED(attachment))
{
    wxShape* shape = this->GetShape();
    wxShapeCanvas* canvas = shape->GetCanvas();
    wxClientDC dc(canvas);
    
    if (shape) {
        shape->Select(!shape->Selected());
        int x = shape->GetX();
        int y = shape->GetY();
        canvas->Refresh();
    }
}

void MapLayoutEvtHandler::OnRightClick(double WXUNUSED(x), double WXUNUSED(y),
                                int keys, int WXUNUSED(attachment))
{
    
}
