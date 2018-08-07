//
//  MapLayoutView.cpp
//  GeoDa
//
//  Created by Xun Li on 8/6/18.
//

#include "MapLayoutView.h"



MapLayoutFrame::MapLayoutFrame(wxBitmap* _legend, wxBitmap* _map, const wxString& title, const wxPoint& pos, const wxSize& size)
: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
    legend = _legend;
    map = _map;
    
    wxShapeCanvas *canvas = new wxShapeCanvas(this, wxID_ANY, pos, size);
    
    canvas->SetBackgroundColour(*wxWHITE);
    canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
    
    diagram = new wxDiagram();
    diagram->SetCanvas(canvas);
    canvas->SetDiagram(diagram);
    
    shape = new wxCircleShape(20.0);
    canvas->AddShape(shape);
    

    shape->SetX(25.0);
    shape->SetY(25.0);
    shape->MakeControlPoints();
    
    // map
    map_shape = new wxBitmapShape();
    double scale = map->GetWidth() / size.GetWidth();
    map_shape->SetScale(scale);
    map_shape->SetBitmap(*map);
    canvas->AddShape(map_shape);
    
    map_shape->SetX(5.0);
    map_shape->SetY(5.0);
    map_shape->MakeControlPoints();
    
    // legend
    legend_shape = new wxBitmapShape();
    legend_shape->SetBitmap(*legend);
    canvas->AddShape(legend_shape);
    
    legend_shape->SetX(5.0);
    legend_shape->SetY(5.0);
    legend_shape->MakeControlPoints();
    
    diagram->ShowAll(1);
}

MapLayoutFrame::~MapLayoutFrame()
{
    delete shape;
    delete diagram;
}

