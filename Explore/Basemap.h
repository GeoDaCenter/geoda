//
//  Basemap.h
//  GeoDa
//
//  Created by Xun Li on 5/29/15.
//  Copyright (c) 2015 __MyCompanyName__. All rights reserved.
//

#ifndef GeoDa_Basemap_h
#define GeoDa_Basemap_h

#ifdef __WIN32__
#define _USE_MATH_DEFINES 
#include <math.h>
#endif

#include <utility>
#include <boost/thread/thread.hpp>
//#include <boost/lockfree/queue.hpp>
//#include <boost/atomic.hpp>

#include <iostream>
#include <fstream>
#include <ogr_spatialref.h>

using namespace std;

namespace GDA {

inline char separator()
{
#ifdef __WIN32__
    return '\\';
#else
    return '/';
#endif
}
    
inline bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good() && (infile.peek() != std::ifstream::traits_type::eof());
}
    
inline double log2(double n)
{
	return log(n) / log(2.0);
}


class LatLng {
public:
    LatLng() {}
    LatLng(double _lat, double _lng) {
        lat = _lat;
        lng = _lng;
    }
    ~LatLng(){}
    
    double lat;
    double lng;
    
    double GetLatDeg() {return lat;}
    double GetLngDeg() {return lng;}
    double GetLatRad() {return lat * M_PI / 180.0;}
    double GetLngRad() {return lng * M_PI / 180.0;}
};

class XY {
public:
    XY(){}
    XY(int _x, int _y) {
        x = _x;
        y = _y;
        xint = _x;
        yint = _y;
        xfrac = .0;
        yfrac = .0;
    }
    XY(double _x, double _y);
    ~XY(){}
    
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
    MapLayer(){}
    MapLayer(MapLayer* _map) {
        north = _map->north;
        south = _map->south;
        west = _map->west;
        east = _map->east;
    }
    MapLayer(double _n, double _w, double _s, double _e){
        north = _n;
        south = _s;
        west = _w;
        east = _e;
    }
    MapLayer(double _n, double _w, double _s, double _e,
             OGRCoordinateTransformation *_poCT){
        north = _n;
        south = _s;
        west = _w;
        east = _e;
        poCT = _poCT;
        if (poCT!= NULL) {
            if (poCT->Transform(1, &_w, &_n)) {
                west = _w;
                north = _n;
            }
            if (poCT->Transform(1, &_e, &_s)) {
                east = _e;
                south = _s;
            }
        }
    }
    MapLayer(LatLng& nw, LatLng& se){
        north = nw.lat;
        west = nw.lng;
        south = se.lat;
        east = se.lng;
    }
    ~MapLayer(){}
   
    MapLayer* operator=(const MapLayer* other) {
        north = other->north;
        south = other->south;
        west = other->west;
        east = other->east;
    }
    double north;
    double south;
    double west;
    double east;
    OGRCoordinateTransformation *poCT;
    
    double GetWidth() {
        if (east >= west)
            return east - west;
        else
            return 180 - east + 180 + west;
    }
    double GetHeight() { return north - south; }
    
    bool IsWGS84Valid() { return north < 90 && south > -90 && east > -180 && west < 180;}
    
    void Pan(double lat, double lng) {
        north += lat;
        south += lat;
        west += lng;
        east += lng;
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
};

// only for Web mercator projection
class Basemap {
    
public:
    Basemap(){}
    Basemap(Screen *_screen,
            MapLayer *_map,
            int map_type,
            std::string _cachePath,
            OGRCoordinateTransformation *_poCT);
    ~Basemap();
    
    OGRCoordinateTransformation *poCT;
    
    int mapType;
    std::string basemapUrl;
    std::string urlSuffix; // ?a=b&c=d
    std::string imageSuffix;
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
   
    bool isTileDrawn;
    bool isTileReady;
    
    bool isPan;
    int panX;
    int panY;
    
    int zoom;
    Screen* screen;
    MapLayer* map;
    MapLayer* origMap;
    std::string cachePath;
    
    double Deg2Rad (double degree) { return degree * M_PI / 180.0; }
    double Rad2Deg (double radians) { return radians * 180.0 / M_PI;}
    XY* LatLngToXY(LatLng &latlng);
    LatLng* XYToLatLng(XY &xy, bool isLL=false);
	void LatLngToXY(double lng, double lat, int &x, int &y);
    
	std::string GetTileUrl(int x, int y);
    std::string GetTilePath(int x, int y);

	void Draw(wxBitmap* buffer);
	
    void ResizeScreen(int _width, int _height);
    void ZoomIn(int mouseX, int mouseY);
    void ZoomOut(int mouseX, int mouseY);
    void Zoom(bool is_zoomin, int x0, int y0, int x1, int y1);
    void Pan(int x0, int y0, int x1, int y1);
    void Reset(int map_type);
    void Reset();
    void Refresh();
    bool IsReady();
   
    void SetupMapType(int map_type);
   
    void CleanCache();
    
protected:
    std::string nokia_id;
    std::string nokia_code;
    
    int nn; // pow(2.0, zoom)
    
    bool bDownload;
    boost::thread* downloadThread;
    boost::thread* downloadThread1;
    
    int GetOptimalZoomLevel(double paddingFactor=1.2);
    int GetEasyZoomLevel();
    
    XY* LatLngToRawXY(LatLng &latlng);
    
    void GetTiles();
    void _GetTiles(int start_x, int start_y, int end_x, int end_y);
    void _GetTiles(int x, int start_y, int end_y);
    
    void DownloadTile(int x, int y);
    
    bool _HasInternet();
};

    
}

#endif
