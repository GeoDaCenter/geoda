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

#ifndef GeoDa_Basemap_h
#define GeoDa_Basemap_h

#include <wx/tokenzr.h>
#include <wx/math.h>
#include <wx/dcgraph.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <ogr_spatialref.h>

#include "../Algorithms/threadpool.h"

using namespace std;

namespace Gda {


    /**
     * BasemapItem is for "Basemap Source Configuration" dialog
     * Each basemap source can be represented in the form of:
     *   GroupName.Name,url
     *   For example:
     *   Carto.Light,https://a.basemaps.cartocdn.com/light_all/{z}/{x}/{y}@2x.png
     *   Carto.Dark,https://a.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}@2x.png
     * GeoDa will create a menu from a collection of BasemapItems
     */
    class BasemapItem {
    public:
        BasemapItem() {}

        BasemapItem(wxString _group, wxString _name, wxString _url) {
            group = _group;
            name = _name;
            url = _url;
        }

        ~BasemapItem() {}

        BasemapItem& operator=(const BasemapItem& other) {
            group = other.group;
            name = other.name;
            url = other.url;
            return *this;
        }

        bool operator==(const BasemapItem& other) {
            return (group == other.group && name == other.name && url == other.url);
        }

        void Reset() {
            group = "";
            name = "";
            url = "";
        }

        wxString group;
        wxString name;
        wxString url;
    };

    /**
     * BasemapGroup is a collection of BasemapItems.
     * GeoDa will create a menu from an instance of BasemapGroup
     * For example:
     *   Carto
     *    |__Light (https://a.basemaps.cartocdn.com/light_all/{z}/{x}/{y}@2x.png)
     *    |__Dark (https://a.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}@2x.png)
     */
    class BasemapGroup {
    public:
        BasemapGroup() {}
        BasemapGroup(wxString _name) {
            name = _name;
        }
        ~BasemapGroup() {}
        BasemapGroup& operator=(const BasemapGroup& other) {
            name = other.name;
            items = other.items;
            return *this;
        }
        void AddItem(BasemapItem item) {
            items.push_back(item);
        }
        wxString name;
        vector<BasemapItem> items;
    };

    // Return an instance of BasemapItem based on the basemap_source, which is
    // at idx-th row of basemap_sources (e.g. GdaConst::gda_basemap_sources)
    BasemapItem GetBasemapSelection(int idx, wxString basemap_sources);

    // Construct a std::vector of BasemapGroup (which is a collection of
    // BasemapItems) using basemap_sources (e.g. GdaConst::gda_basemap_sources
    // or the value in "Basemap Sources:" TextCtrl in Basemap Configuration Dialog
    vector<BasemapGroup> ExtractBasemapResources(wxString basemap_sources) ;

    // inline function return separator for local file path
    inline char separator() {
#ifdef __WIN32__
        return '\\';
#else
        return '/';
#endif
    }
    
    inline bool is_file_exist(const char *fileName) {
        std::ifstream infile(fileName);
        return infile.good() && (infile.peek() != std::ifstream::traits_type::eof());
    }
    
    inline double log2(double n) {
        return log(n) / log(2.0);
    }
    
    class LatLng {
    public:
        LatLng() {}
        LatLng(double _lat, double _lng) {
            lat = _lat;
            lng = _lng;
            if (lat > 85.0511) lat = 85.0511;
            if (lat < -85.0511) lat = -85.0511;
        }
        ~LatLng(){}
        
        double lat;
        double lng;
        
        double GetLatDeg() {return lat;}
        double GetLngDeg() {return lng;}
        double GetLatRad() {return lat * M_PI / 180.0;}
        double GetLngRad() {return lng * M_PI / 180.0;}
    };
    
    class XYFraction {
    public:
        XYFraction(){}
        XYFraction(int _x, int _y) {
            x = _x;
            y = _y;
            xint = _x;
            yint = _y;
            xfrac = .0;
            yfrac = .0;
        }
        XYFraction(double _x, double _y);
        ~XYFraction(){}
        
        double x;
        double y;
        double xfrac;
        double yfrac;
        double xint;
        double yint;
        
        int GetXInt() { return (int)xint;}
        int GetYInt() { return (int)yint;}
        double GetXFrac() { return xfrac;}
        double GetYFrac() { return yfrac;}
    };
    
    class Screen {
    public:
        Screen(){}
        Screen(int _w, int _h) { width=_w; height= _h;}
        ~Screen(){}
        
        int width;
        int height;
    };
    
    class MapLayer {
    public:
        double north;
        double south;
        double west;
        double east;
        OGRCoordinateTransformation *poCT;
        OGRCoordinateTransformation *poCT_rev;
        
        MapLayer(){
            poCT = NULL;
            poCT_rev = NULL;
        }
        MapLayer(MapLayer* _map) {
            north = _map->north;
            south = _map->south;
            west = _map->west;
            east = _map->east;
            poCT = NULL;
            poCT_rev = NULL;
        }
        MapLayer(double _n, double _w, double _s, double _e){
            north = _n;
            south = _s;
            west = _w;
            east = _e;
            poCT = NULL;
            poCT_rev = NULL;
        }
        MapLayer(double _n, double _w, double _s, double _e,
                 OGRCoordinateTransformation *_poCT){
            north = _n;
            south = _s;
            west = _w;
            east = _e;
            poCT = _poCT;
            poCT_rev = NULL;
            if (poCT!= NULL) {
                if (poCT->Transform(1, &_w, &_n)) {
                    west = _w;
                    north = _n;
                }
                if (poCT->Transform(1, &_e, &_s)) {
                    east = _e;
                    south = _s;
                }
                OGRSpatialReference* s1 = poCT->GetTargetCS();
                OGRSpatialReference* s2 = poCT->GetSourceCS();
#ifdef __PROJ6__
                s1->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                s2->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
                poCT_rev = OGRCreateCoordinateTransformation(s1, s2);
            }
        }
        MapLayer(LatLng& nw, LatLng& se){
            north = nw.lat;
            west = nw.lng;
            south = se.lat;
            east = se.lng;
            poCT = NULL;
            poCT_rev = NULL;
        }
        ~MapLayer(){

        }
        
        void GetWestNorthEastSouth(double& w, double& n, double& e, double& s) {
            w = west;
            n = north;
            e = east;
            s = south;
            if (poCT_rev) {
                poCT_rev->Transform(1, &w, &n);
                poCT_rev->Transform(1, &e, &s);
            }
        }

        double GetWidth() {
            if (east >= west)
                return east - west;
            else
                return 180 - east + 180 + west;
        }
        
        double GetHeight() { return north - south; }
        
        bool IsWGS84Valid() { return north < 90 && south > -90 && east > -180 && west < 180;}
        
        bool Pan(double lat, double lng) {
            north += lat;
            south += lat;
            west += lng;
            east += lng;
            if (north > 90 || south < -90 || east > 180 || west < -180)
                return false;
            return true;
        }
        
        void UpdateExtent(double _w, double _s, double _e, double _n)  {
            north = _n;
            south = _s;
            east = _e;
            west = _w;
        }
        
        void ZoomIn() {
            // 2X by default
            double w = east - west;
            double h = north - south;
            double offsetW = w / 4.0;
            double offsetH = h / 4.0;
            west = west + offsetW;
            east = east - offsetW;
            north = north - offsetH;
            south = south + offsetH;
        }
        
        void ZoomOut() {
            // 2X by default
            double w = east - west;
            double h = north - south;
            double offsetW = w / 2.0;
            double offsetH = h / 2.0;
            west = west - offsetW;
            east = east + offsetW;
            north = north + offsetH;
            south = south - offsetH;
        }
        MapLayer* operator=(const MapLayer* other) {
            north = other->north;
            south = other->south;
            west = other->west;
            east = other->east;
            poCT = NULL;
            poCT_rev = NULL;
            if (other) {
                if (other->poCT) {
                    OGRSpatialReference* s1 = other->poCT->GetSourceCS();
                    OGRSpatialReference* s2 = other->poCT->GetTargetCS();
#ifdef __PROJ6__
                    s1->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                    s2->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
                    poCT = OGRCreateCoordinateTransformation(s1, s2);
                }
                if (other->poCT_rev) {
                    OGRSpatialReference* s1 = other->poCT_rev->GetSourceCS();
                    OGRSpatialReference* s2 = other->poCT_rev->GetTargetCS();
#ifdef __PROJ6__
                    s1->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                    s2->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
                    poCT_rev = OGRCreateCoordinateTransformation(s1, s2);
                }
            }
            return this;
        }
    };

    // only for Web mercator projection
    class Basemap {
        int nn; // pow(2.0, zoom)

        thread_pool pool;
        boost::mutex mutex;

        bool start_download;
        int n_tasks;
        int complete_tasks;

        wxString GetRandomSubdomain(wxString url);
        int GetOptimalZoomLevel(double paddingFactor=1.2);
        int GetEasyZoomLevel();
        void GetTiles();
        void DownloadTile(int x, int y);

    public:
        Basemap() {}
        Basemap(BasemapItem& basemap_item,
                Screen* _screen,
                MapLayer* _map,
                MapLayer* _origMap,
                wxString _cachePath,
                OGRCoordinateTransformation* _poCT,
                double scale_factor = 1.0);
        ~Basemap();

        static const char* USER_AGENT;
        OGRCoordinateTransformation *poCT;
        BasemapItem basemap_item;
        wxString basemapName;
        wxString basemapUrl;
        wxString imageSuffix;
        wxString cachePath;
        wxString nokia_id;
        wxString nokia_code;
        
        int startX;
        int startY;
        int endX;
        int endY;
        int offsetX;
        int offsetY;
        int widthP; // width of all tiles in pixel
        int heightP; // height of all tiles in pixel
        int leftP;
        int topP;
        double scale_factor;
        
        bool isTileDrawn;
        bool isTileReady;
        
        bool isPan;
        int panX;
        int panY;
        
        int zoom;
        Screen* screen;
        MapLayer* map;
        MapLayer* origMap;

        double Deg2Rad (double degree) { return degree * M_PI / 180.0; }
        double Rad2Deg (double radians) { return radians * 180.0 / M_PI;}
        XYFraction* LatLngToXY(LatLng &latlng);
        XYFraction* LatLngToRawXY(LatLng &latlng);
        LatLng* XYToLatLng(XYFraction &xy, bool isLL=false);
        void LatLngToXY(double lng, double lat, int &x, int &y);
        
        wxString GetTileUrl(int x, int y);
        wxString GetTilePath(int x, int y);
        
        bool Draw(wxBitmap* buffer);
        void Extent(double _n, double _w, double _s, double _e,
                    OGRCoordinateTransformation *_poCT);
        void ResizeScreen(int _width, int _height);
        void ZoomIn(int mouseX, int mouseY);
        void ZoomOut(int mouseX, int mouseY);
        bool Zoom(bool is_zoomin, int x0, int y0, int x1, int y1);
        void Pan(int x0, int y0, int x1, int y1);
        void Reset(int map_type);
        void Reset();
        void Refresh();
        bool IsDownloading();
        void SetReady(bool flag);
        bool IsExtentChanged();
        void SetupMapType(BasemapItem& basemap_item);
        
        void CleanCache();
        wxString GetContentType();
    };
    
}

#endif
