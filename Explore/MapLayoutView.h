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

class MapLayoutFrame: public wxFrame
{
    wxDiagram * diagram;
    wxShape * shape;
    wxBitmapShape * map_shape;
    wxBitmapShape * legend_shape;
    
    wxBitmap* legend;
    wxBitmap* map;
    
public:
    
    MapLayoutFrame(wxBitmap* legend, wxBitmap* map, const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MapLayoutFrame();
};

#endif /* MapLayoutView_h */
