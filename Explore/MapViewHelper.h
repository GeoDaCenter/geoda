/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEODA_CENTER_MAP_VIEW_HELPER_H__
#define __GEODA_CENTER_MAP_VIEW_HELPER_H__

#include <vector>
#include <list>
#include <wx/wx.h>
#include "../Algorithms/skater.h" // for reusing adjacency_list<>Graph

class CatClassifData;
class MapCanvas;
class TableInterface;
class HeatMapHelper;
class GdaShape;
class Project;

// Transparency SliderBar dialog for Basemap
class MapTransparencyDlg: public wxDialog
{
public:
    MapTransparencyDlg ();
    MapTransparencyDlg (wxWindow* parent,
                        MapCanvas* canvas,
                        wxWindowID id=wxID_ANY,
                        const wxString & caption=_("Transparency Setup Dialog"),
                        const wxPoint & pos = wxDefaultPosition,
                        const wxSize & size = wxDefaultSize,
                        long style = wxDEFAULT_DIALOG_STYLE );
    virtual ~MapTransparencyDlg ();
    virtual void OnSliderChange(wxCommandEvent& event);

protected:
    // reference to MapCanvas (no MEM mgnt)
    MapCanvas* canvas;

    // UI: slider (no MEM mgnt)
    wxSlider* slider;

    // UI: text input control under slider (no MEM mgnt)
    wxStaticText* slider_text;
};

// Setup transparency for heatmap
class HeatMapTransparencyDlg : public MapTransparencyDlg
{
public:
    HeatMapTransparencyDlg(HeatMapHelper* heatmap, MapCanvas* canvas);
    virtual ~HeatMapTransparencyDlg();
    virtual void OnSliderChange(wxCommandEvent& event);

protected:
    // reference to HeatMap instance in MapCanvas (no MEM mgnt)
    HeatMapHelper* heatmap;
};

// Setup bandwidth for heatmap
class HeatMapBandwidthDlg: public wxDialog
{
public:
    HeatMapBandwidthDlg();
    HeatMapBandwidthDlg(HeatMapHelper* _heatmap,
                        MapCanvas* _canvas,
                        double min_band, double max_band,
                        wxWindowID id = wxID_ANY,
                        const wxString & caption =_("Heat Map Bandwidth Setup Dialog"),
                        const wxPoint & pos = wxDefaultPosition,
                        const wxSize & size = wxDefaultSize,
                        long style = wxDEFAULT_DIALOG_STYLE );

    virtual ~HeatMapBandwidthDlg();

    void OnSliderChange(wxCommandEvent& event );

    void OnTextChange(wxCommandEvent& event );

private:
    // reference to MapCanvas (no MEM mgnt)
    MapCanvas* canvas;

    // reference to HeatMap instance in MapCanvas (no MEM mgnt)
    HeatMapHelper* heatmap;

    // UI: slider (no MEM mgnt)
    wxSlider* slider;

    // UI: text label under slider (no MEM mgnt)
    wxTextCtrl* slider_text;

    // selected bandwidth
    double sel_band;

    // maximum bandwidth
    double max_band;

    // max tick for slider
    int max_tick;
};

// Helper class for drawing heat map on MapCanvas
class HeatMapHelper
{
public:
    HeatMapHelper();

    virtual ~HeatMapHelper();

    // Draw a heat map by making points with radius, color and transparency
    // and saving the points to background_shps, which will be rendered by
    // MapCanvas;
    // The color could be defined by CatClassifData in MapCanvas, if user
    // doesn't specify heat_map_fill_color and heat_map_outline_color;
    // The heat map can be created by using either bandwidth, or a variable
    // that contains the values of radius for all points.
    void Draw(const std::vector<GdaShape*>& selectable_shps,
              std::list<GdaShape*>& background_shps,
              CatClassifData& cat_data);

    // Prompt user to set bandwidth value to create a heat map
    void SetBandwidth(MapCanvas* canvas, Project* project);

    // Prompt user to select a variable to set radius value for points
    // in a heat map
    void SetRadiusVariable(MapCanvas* canvas, Project* project);

    // Change property value: bandwidth
    void UpdateBandwidth(double bandwidth);

    // Prompt user to select a fill color
    void ChangeFillColor(MapCanvas* canvas);

    // Prompt user to select a outline color
    void ChangeOutlineColor(MapCanvas* canvas);

    // User select to reset original heat map
    // not using user specified fill color/outline color
    void Reset();

    // Prompt user to specify transparency
    void ChangeTransparency(MapCanvas* canvas);

    // Change transparency of (point) fill color
    void UpdateTransparency(int tran);

    // Get current transparency
    int GetTransparency();

    // If HeatMapHelper has been initialized
    bool HasInitialized();

protected:
    bool has_init;

    // flag if use fill color, false will use current map's fill color
    bool use_fill_color;

    // flag if use outline color, false will use TRANSPARENT (no) color
    bool use_outline_color;

    // Fill colour (wxBrush)
    wxColour fill_color;

    // Outline colour (wxPen)
    wxColour outline_color;

    // The bandwidth used to create a heat map
    double bandwidth;

    // flag if use bandwidth
    bool use_bandwidth;

    // The array contains radius values for point
    std::vector<double> radius_arr;

    // flag if use radius variable
    bool use_radius_variable;

    // transparency 0-255
    int transparency;

    // check if lat/lng is used
    static bool check_spatial_ref;
};

// Helper class for adding Mimimum Spanning Tree for points map canvas
class MSTMapHelper
{
public:
    MSTMapHelper();
    virtual~MSTMapHelper();

    // Create a MST from points, and show it on canvas
    bool Create(Project* project);

    void Draw(std::list<GdaShape*>& foreground_shps,
              CatClassifData& cat_data);

    // Prompt user to select a line color
    void ChangeColor(MapCanvas* canvas);

    // Prompt user to select a line thickness
    void ChangeThickness(MapCanvas* canvas, int thickness);

    // Return line thickness
    int GetThickness();

    // Save MST to a spatial weights file *.gwt
    void SaveToWeightsFile(Project* project);

protected:
    void CreateDistMatrix(const std::vector<GdaPoint*>& points);

    std::vector<std::vector<double> > dist_matrix;
    
    std::vector<std::pair<int, int> > mst_edges;

    std::vector<GdaPoint*> nodes;

    // Flag to use user specified pen color
    bool use_custom_pen;

    // Pen thickness (wxBrush)
    int outline_thickness;

    // Outline colour (wxPen)
    wxColour outline_color;

    // check if lat/lng is used
    static bool check_spatial_ref;
};

#endif
