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

#include <iostream>
#include <sstream>
#include <algorithm>
#include "stdio.h"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <wx/math.h>
#include <wx/tokenzr.h>
#include <wx/dcbuffer.h>
#include <wx/bitmap.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/graphics.h>
#include <wx/stream.h>
#include <wx/wfstream.h>
#include <wx/url.h>
#include <ogr_spatialref.h>

#include <curl/curl.h>
#include "../ShapeOperations/OGRDataAdapter.h"
#include "Basemap.h"

using namespace std;
using namespace Gda;

BasemapItem Gda::GetBasemapSelection(int idx, wxString basemap_sources)
{
    BasemapItem basemap_item;
 
    idx = idx - 1; // first item [0] is choice "no basemap"
    wxString encoded_str= wxString::FromUTF8((const char*)basemap_sources.mb_str());
    if (encoded_str.IsEmpty() == false) {
        basemap_sources = encoded_str;
    }
    vector<wxString> keys;
    wxString newline;
    if (basemap_sources.Find("\r\n") != wxNOT_FOUND) {
        newline = "\r\n";
    } else if (basemap_sources.Find("\r") != wxNOT_FOUND) {
        newline = "\r";
    } else if (basemap_sources.Find("\n") != wxNOT_FOUND) {
        newline = "\n";
    }
    if (newline.IsEmpty() == false) {
        wxStringTokenizer tokenizer(basemap_sources, newline);
        while ( tokenizer.HasMoreTokens() ) {
            wxString token = tokenizer.GetNextToken();
            keys.push_back(token.Trim());
        }
        if (idx >= 0 && idx < keys.size()) {
            wxString basemap_source = keys[idx];
            wxUniChar comma = ',';
            int comma_pos = basemap_source.Find(comma);
            if ( comma_pos != wxNOT_FOUND ) {
                // group.name,url
                wxString group_n_name = basemap_source.BeforeFirst(comma);
                wxString url = basemap_source.AfterFirst(comma);
                wxUniChar dot = '.';
                wxString group = group_n_name.Before(dot);
                wxString name = group_n_name.After(dot);
                if (!group.IsEmpty() && !name.IsEmpty()) {
                    basemap_item.group = group;
                    basemap_item.name = name;
                    basemap_item.url = url;
                }
            }
        }
    }
    return basemap_item;
}

vector<BasemapGroup> Gda::ExtractBasemapResources(wxString basemap_sources) {
    vector<wxString> group_names;
    map<wxString, BasemapGroup> group_dict;
    
    wxString encoded_str= wxString::FromUTF8((const char*)basemap_sources.mb_str());
    if (encoded_str.IsEmpty() == false) {
        basemap_sources = encoded_str;
    }
    vector<wxString> keys;
    wxString newline;
    if (basemap_sources.Find("\r\n") != wxNOT_FOUND) {
        newline = "\r\n";
    } else if (basemap_sources.Find("\r") != wxNOT_FOUND) {
        newline = "\r";
    } else if (basemap_sources.Find("\n") != wxNOT_FOUND) {
        newline = "\n";
    }
    if (newline.IsEmpty() == false) {
        wxStringTokenizer tokenizer(basemap_sources, newline);
        while ( tokenizer.HasMoreTokens() ) {
            wxString token = tokenizer.GetNextToken();
            keys.push_back(token.Trim());
        }
        for (int i=0; i<keys.size(); i++) {
            wxString basemap_source = keys[i];
            wxUniChar comma = ',';
            int comma_pos = basemap_source.Find(comma);
            if ( comma_pos != wxNOT_FOUND ) {
                // group.name,url
                wxString group_n_name = basemap_source.BeforeFirst(comma);
                wxString url = basemap_source.AfterFirst(comma);
                wxUniChar dot = '.';
                wxString group = group_n_name.Before(dot);
                wxString name = group_n_name.After(dot);
                if (group.IsEmpty() || name.IsEmpty()) {
                    continue;
                }
                if (group_dict.find(group) == group_dict.end()) {
                    group_names.push_back(group);
                    BasemapGroup bg(group);
                    group_dict[group] = bg;
                }
                BasemapItem item(group, name, url);
                group_dict[group].AddItem(item);
            }
        }
    }
    vector<BasemapGroup> groups;
    for (int i=0; i<group_names.size(); i++) {
        groups.push_back( group_dict[group_names[i]] );
    }
    return groups;
}

XYFraction::XYFraction(double _x, double _y)
{
    x = _x;
    y = _y;
    xfrac = modf(_x, &xint);
    yfrac = modf(_y, &yint);
}


Basemap::Basemap(BasemapItem& _basemap_item,
                 Screen* _screen,
                 MapLayer* _map,
                 MapLayer* _origMap,
                 wxString _cachePath,
                 OGRCoordinateTransformation *_poCT,
                 double _scale_factor)
{
    basemap_item = _basemap_item;
    screen = _screen;
    map = _map;
    origMap = _origMap;
    cachePath = _cachePath;
    poCT = _poCT;
    scale_factor = _scale_factor;

    bDownload = false;
    downloadThread = NULL;
    isPan = false;
    panX = 0;
    panY = 0;
    
    isTileReady = false;
    isTileDrawn = false;
    
    nokia_id = "oRnRceLPyM8OFQQA5LYH";
    nokia_code = "uEt3wtyghaTfPdDHdOsEGQ";
    
    wxInitAllImageHandlers();
    
    GetEasyZoomLevel();
    SetupMapType(basemap_item);
}

Basemap::~Basemap() {
    //done = true;
    if (screen) {
        delete screen;
        screen  = 0;
    } 
    if (map) {
        delete map;
        map = 0;
    }
    if (origMap) {
        delete origMap;
        origMap = 0;
    }
    if (poCT) {
        delete poCT;
        poCT = 0;
    }
    if (bDownload && downloadThread) {
        bDownload = false;
		downloadThread->join();
        delete downloadThread;
        downloadThread = NULL;
    }
}

void Basemap::CleanCache()
{
    wxString filename;
    filename << cachePath << "basemap_cache"<< separator();
    wxDir dir(filename);
    if (dir.IsOpened() ) {
        wxString file;
        bool cont = dir.GetFirst(&file);
        while ( cont ) {
            file = filename + wxFileName::GetPathSeparator()+ file;
            if(wxFileName::FileExists(file)) wxRemoveFile(file);
            cont = dir.GetNext(&file);
        }
    }
}

void Basemap::SetupMapType(BasemapItem& _basemap_item)
{
    basemap_item = _basemap_item;
    basemapName = basemap_item.group + "." + basemap_item.name;
    basemapUrl = basemap_item.url;
    
    if (basemapUrl.Find("png") != wxNOT_FOUND ||
        basemapUrl.Find("PNG") != wxNOT_FOUND) {
        imageSuffix = ".png";
    } else if (basemapUrl.Find("jpeg") != wxNOT_FOUND ||
               basemapUrl.Find("JPEG") != wxNOT_FOUND) {
        imageSuffix = ".jpeg";
    } else {
        imageSuffix = ".jpeg";
    }
    // if ( !hdpi ) {
    //     basemapUrl.Replace("@2x", "");
    // }
    
    // get a latest CartoDB account
    vector<wxString> nokia_user = OGRDataAdapter::GetInstance().GetHistory("nokia_user");
    if (!nokia_user.empty()) {
        wxString user = nokia_user[0];
        if (!user.empty()) {
            nokia_id = user;
        }
    }
    
    vector<wxString> nokia_key = OGRDataAdapter::GetInstance().GetHistory("nokia_key");
    if (!nokia_key.empty()) {
        wxString key = nokia_key[0];
        if (!key.empty()) {
            nokia_code = key;
        }
    }
    
    GetTiles();
}

void Basemap::Reset()
{
    map->north = origMap->north;
    map->south= origMap->south;
    map->west= origMap->west;
    map->east= origMap->east;
    GetEasyZoomLevel();
    SetupMapType(basemap_item);
}

void Basemap::Reset(int map_type)
{
    map->north = origMap->north;
    map->south= origMap->south;
    map->west= origMap->west;
    map->east= origMap->east;
    GetEasyZoomLevel();
    SetupMapType(basemap_item);
}

void Basemap::Extent(double _n, double _w, double _s, double _e,
                     OGRCoordinateTransformation *_poCT)
{
    if (poCT!= NULL) {
        poCT->Transform(1, &_w, &_n);
        poCT->Transform(1, &_e, &_s);
    }
    map->north = _n;
    map->south= _s;
    map->west= _w;
    map->east= _e;
    origMap->north = _n;
    origMap->south= _s;
    origMap->west= _w;
    origMap->east= _e;
    GetEasyZoomLevel();
    SetupMapType(basemap_item);
}

void Basemap::ResizeScreen(int _width, int _height)
{
    if (screen) {
        screen->width = _width;
        screen->height = _height;
    }

    //isTileDrawn = false;
    GetEasyZoomLevel();
    
    SetupMapType(basemap_item);
}

void Basemap::Pan(int x0, int y0, int x1, int y1)
{
    XYFraction origXY((x0 + leftP + offsetX)/256.0, (y0 + topP + offsetY)/256.0);
    XYFraction newXY((x1 + leftP + offsetX)/256.0, (y1 + topP + offsetY)/256.0);
    
    LatLng* p0 = XYToLatLng(origXY, true);
    LatLng* p1 = XYToLatLng(newXY, true);
    
    double offsetLat = p1->lat - p0->lat;
    double offsetLon = p1->lng - p0->lng;
    
    if (map->Pan(-offsetLat, -offsetLon)) {
        isTileDrawn = false;
        isTileReady = false;
        GetTiles();
    }
}

bool Basemap::Zoom(bool is_zoomin, int x0, int y0, int x1, int y1)
{
    if (is_zoomin == false && zoom <= 1)
        return false;
    
    int left = x0 < x1 ? x0 : x1;
    int right = x0 < x1 ? x1 : x0;
    int top = y0 > y1 ? y1 : y0;
    int bottom = y0 > y1 ? y0 : y1;
    
    if (is_zoomin == false) {
        left = 0 - left;
        top = 0 - top;
        right = screen->width * 2 - right;
        bottom = screen->height * 2 - bottom;
    }
    
    XYFraction origXY((left + leftP + offsetX)/256.0, (top + topP + offsetY)/256.0);
    XYFraction newXY((right + leftP + offsetX)/256.0, (bottom + topP + offsetY)/256.0);
    
    LatLng* p0 = XYToLatLng(origXY, true);
    LatLng* p1 = XYToLatLng(newXY, true);
    
    double north = p0->lat;
    double west = p0->lng;
    double south = p1->lat;
    double east = p1->lng;
    
    map->UpdateExtent(west, south, east, north);
    
    isTileDrawn = false;
    isTileReady = false;
    GetEasyZoomLevel();
    GetTiles();
    return true;
}

bool Basemap::IsExtentChanged()
{
    bool no_change = (origMap->north == map->north &&
                      origMap->south == map->south &&
                      origMap->east == map->east &&
                      origMap->west == map->west);
    return !no_change;
}

void Basemap::ZoomIn(int mouseX, int mouseY)
{
    // 2X by default
    map->ZoomIn();
    GetEasyZoomLevel();
    
    int x0 = screen->width / 2.0;
    int y0 = screen->height / 2.0;
    
    //isTileDrawn = false;
    //isTileReady = false;
    Pan(mouseX, mouseY, x0, y0);
    
}

void Basemap::ZoomOut(int mouseX, int mouseY)
{
    // 2X by default
    map->ZoomOut();
    GetEasyZoomLevel();
    
    int x0 = screen->width / 2.0;
    int y0 = screen->height / 2.0;
    
    //isTileDrawn = false;
    //isTileReady = false;
    Pan(mouseX, mouseY, x0, y0);
}

int Basemap::GetOptimalZoomLevel(double paddingFactor)
{
    double ry1 = log(sin(Deg2Rad(map->south)) + 1) / cos(Deg2Rad(map->south));
    double ry2 = log(sin(Deg2Rad(map->north)) + 1) / cos(Deg2Rad(map->north));
    double ryc = (ry1 + ry2) / 2.0;
    double centerY = Rad2Deg(atan(sinh(ryc)));
    double resolutionHorizontal = map->GetWidth() / screen->width;
    
    double vy0 = log(tan(M_PI * (0.25 + centerY / 360.0)));
    double vy1 = log(tan(M_PI * (0.25 + map->north / 360.0)));
    double viewHeightHalf = screen->height / 2.0;
    
    double zoomFactorPowered = viewHeightHalf / (40.7436654315252*(vy1-vy0));
    double resolutionVertical = 360.0 / (zoomFactorPowered * 256);
    
    double resolution = max(resolutionHorizontal, resolutionVertical) * paddingFactor;
    
    zoom = log2(360.0 / (resolution * 256));
    
    return zoom;
}

int Basemap::GetEasyZoomLevel()
{
    double degreeRatio = 360.0 / map->GetWidth();
    double zoomH = (int)ceil(log2(degreeRatio * screen->width / 256.0));
    
    degreeRatio = 85.0511 * 2.0 / map->GetHeight();
    double zoomV = (int)ceil(log2(degreeRatio * screen->height / 256.0));
    
    if (zoomH > 0 && zoomV > 0) {
        zoom = min(zoomH, zoomV);
    } else {
        if (zoomH > 0)
            zoom = zoomH;
        if (zoomV > 0)
            zoom = zoomV;
    }
    
    if (zoom > 18)
        zoom = 18;
    
    nn = pow(2.0, zoom);
    
    return zoom;
}

void Basemap::Refresh()
{
    GetTiles();
}

void Basemap::GetTiles()
{
    if (zoom < 1)
        return;
    
    // following: http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    // top-left / north-west
    LatLng nw(map->north, map->west);
    XYFraction* topleft = LatLngToRawXY(nw);
    
    // bottom-right / south-east
    LatLng se(map->south, map->east);
    XYFraction* bottomright = LatLngToRawXY(se);
    
    startX = topleft->GetXInt();
    startY = topleft->GetYInt();
    
    leftP = startX * 256;
    topP = startY * 256;
    
    endX = bottomright->GetXInt();
    endY = bottomright->GetYInt();
    
    // crosss 180 border
    if (endX < startX) {
        endX = nn + endX;
    }
    widthP = (endX - startX + 1) * 256;
    heightP = (endY - startY + 1) * 256;
    
    if (widthP < screen->width) {
        int x_addition = (int)ceil((screen->width - widthP)/ 256.0);
        endX += x_addition;
        widthP = (endX - startX + 1) * 256;
    }
    if (heightP < screen->height) {
        int y_addition = (int)ceil((screen->height - heightP)/ 256.0);
        endY += y_addition;
        heightP = (endY - startY + 1) * 256;
    }
    
    // position of first tile (top-left)
    double map_wp = 0;
    if (endX <= nn) {
        map_wp = (bottomright->x - topleft->x) * 255;
    } else {
        map_wp = (nn - topleft->x + bottomright->x) * 255;
    }
    int map_offx = (int)(ceil) ((screen->width - map_wp) / 2.0);
    
    // if offset to left, need to zoom out
    if (map_offx < 0 && zoom > 0) {
        zoom = zoom -1;
        nn = pow(2.0, zoom);
        GetTiles();
        return;
    }
    
    offsetX = topleft->GetXFrac() * 255 - map_offx;
    // if offset to right, need to patch empty tiles
    if (offsetX < 0) {
        while (offsetX < 0) {
            offsetX += 256;
            startX = startX -1;
            widthP = widthP + 256;
            leftP = startX * 256;
        }
    }
    
    double map_hp = (bottomright->y - topleft->y) * 255;
    int map_offy = (int) ((screen->height - map_hp) / 2.0);
    offsetY = topleft->GetYFrac() * 255 - map_offy;
    
    // if offset down, need to patch empty tiles
    if (offsetY < 0 ) {
        while (offsetY < 0) {
            startY = startY -1;
            offsetY = offsetY + 256;
            heightP = heightP + 256;
            topP = startY * 256;
        }
    }
    
    // check tiles again after offset
    if (widthP - offsetX < screen->width) {
        endX += 1;
        widthP = (endX - startX + 1) * 256;
    }
    if (heightP - offsetY < screen->height) {
        endY += 1;
        heightP = (endY - startY + 1) * 256;
    }
    
    offsetX = offsetX - panX;
    offsetY = offsetY - panY;

    isTileReady = false;
    isTileDrawn = false;
    
    if (bDownload && downloadThread) {
        bDownload = false;
		downloadThread->join();
        delete downloadThread;
        downloadThread = NULL;
    }
    
    if (downloadThread == NULL) {
        bDownload = true;
        downloadThread = new boost::thread(boost::bind(&Basemap::_GetTiles,this, startX, startY, endX, endY));
    }

    delete topleft;
    delete bottomright;
}

bool Basemap::IsReady()
{
    return isTileReady;
}

void Basemap::_GetTiles(int start_x, int start_y, int end_x, int end_y)
{
    boost::thread_group threadPool;
    for (int i=start_x; i<=end_x; i++) {
        if (bDownload == false) {
            return;
        }
        int start_i = i > nn ? nn - i : i;
            
        boost::thread* worker = new boost::thread(boost::bind(&Basemap::_GetTiles,this, start_i, startY, endY));
        threadPool.add_thread(worker);
    }
    threadPool.join_all();
    isTileReady = true;
}


void Basemap::_GetTiles(int i, int start_y, int end_y)
{
    for (int j=start_y; j<=end_y; j++) {
        if (bDownload == false) {
            return;
        }
        int idx_x = i < 0 ? nn + i : i;
        int idx_y = j < 0 ? nn + j : j;
        if (idx_x > nn)
            idx_x = idx_x - nn;
        DownloadTile(idx_x, idx_y);
    }
}

size_t curlCallback(void *ptr, size_t size, size_t nmemb, void* userdata)
{
    FILE* stream = (FILE*)userdata;
    if (!stream)
    {
        printf("!!! No stream\n");
        return 0;
    }
    
    size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
    return written;
}

void Basemap::DownloadTile(int x, int y)
{
    if (x < 0 || y < 0)
        return;
    
    // detect if file exists in temp/ directory
    wxString filepathStr = GetTilePath(x, y);
    std::string filepath = GET_ENCODED_FILENAME(filepathStr);

    if (!wxFileExists(filepathStr)) {
        // otherwise, download the image
        wxString urlStr = GetTileUrl(x, y);
        char* url = new char[urlStr.length() + 1];
        std::strcpy(url, urlStr.c_str());
        
        FILE* fp;
        CURL* image;
        CURLcode imgResult;

        image = curl_easy_init();
        if (image) {
#ifdef __WIN32__
			fp = _wfopen(filepathStr.wc_str(), L"wb");
#else
            fp = fopen(GET_ENCODED_FILENAME(filepathStr), "wb");
#endif
            if (fp)
            {
                curl_easy_setopt(image, CURLOPT_URL, url);
                curl_easy_setopt(image, CURLOPT_WRITEFUNCTION, curlCallback);
                curl_easy_setopt(image, CURLOPT_WRITEDATA, fp);
                //curl_easy_setopt(image, CURLOPT_FOLLOWLOCATION, 1);
                curl_easy_setopt(image, CURLOPT_CONNECTTIMEOUT, 10L);
                curl_easy_setopt(image, CURLOPT_NOSIGNAL, 1L);
            
                // Grab image
                imgResult = curl_easy_perform(image);
           
                curl_easy_cleanup(image);
                fclose(fp);
            }
        }
        
        delete[] url;
        
    }
    isTileReady = false; // notice template_canvas to draw
    //canvas->Refresh(true);
}


LatLng* Basemap::XYToLatLng(XYFraction &xy, bool isLL)
{
    double x = xy.x;
    if (x > nn)
        x = x - nn;
    double lng = x * 360.0 / nn - 180.0;
    double n = M_PI - 2.0 * M_PI * xy.y / nn;
    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    
    if (!isLL && poCT) {
        poCT->TransformEx(1, &lng, &lat);
    }
    return new LatLng(lat, lng);
}

XYFraction* Basemap::LatLngToRawXY(LatLng &latlng)
{
    double lat_rad = latlng.GetLatRad();
    double x = (latlng.GetLngDeg() + 180.0 ) / 360.0 * nn;
    double y = (1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * nn;
    return new XYFraction(x, y);
}

XYFraction* Basemap::LatLngToXY(LatLng &latlng)
{
    double lat_rad = latlng.GetLatRad();
    double x = (latlng.GetLngDeg() + 180.0 ) / 360.0 * nn;
    double y = (1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * nn;
    int xp = (int)(x * 256 - leftP) - offsetX;
    int yp = (int)(y * 256 - topP) - offsetY;
    return new XYFraction(xp, yp);
}

// This function is used to convert lng/lat to screen (x,y)
// This function will be called by projectToBasemap()
void Basemap::LatLngToXY(double lng, double lat, int &x, int &y)
{
    if (poCT!= NULL) {
        poCT->Transform(1, &lng, &lat);
    }
    
    double lat_rad = lat * M_PI / 180.0;
    double yy = (1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * nn;
    y = (int)(yy * 256 - topP) - offsetY;
   
    double xx = (lng + 180.0 );
    xx = xx / 360.0 * nn;

    x = (int)(xx *256 - leftP);
    if (x < -256) x = nn*256 + x;
    x = x - offsetX;
    
    if ( endX > nn) {
        if (x <0) {
            x = nn*256 + x;
        }
    }
}

wxString Basemap::GetTileUrl(int x, int y)
{
    wxString url = basemapUrl;
    url.Replace("{z}", wxString::Format("%d", zoom));
    url.Replace("{x}", wxString::Format("%d", x));
    url.Replace("{y}", wxString::Format("%d", y));
    url.Replace("HERE_APP_ID", nokia_id);
    url.Replace("HERE_APP_CODE", nokia_code);
    return url;
}

wxString Basemap::GetTilePath(int x, int y)
{
    //std::ostringstream filepathBuf;
    wxString filepathBuf;
    filepathBuf << cachePath << "basemap_cache"<< separator();
    filepathBuf << basemapName << "-";
    filepathBuf << zoom << "-" << x <<  "-" << y << imageSuffix;
    
	wxString newpath;
	for (int i = 0; i < filepathBuf.length() ;i++) {
		if(filepathBuf[i] == '\\') {
			newpath += filepathBuf[i];
			newpath += filepathBuf[i];
        } else {
			newpath += filepathBuf[i];
        }
	}
    return newpath;
}

bool Basemap::Draw(wxBitmap* buffer)
{
	// when tiles pngs are ready, draw them on a buffer
	wxMemoryDC dc(*buffer);
	dc.SetBackground(*wxWHITE);
    dc.Clear();
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return false;
   
    int x0 = startX;
    int x1 = endX;
	for (int i=x0; i<=x1; i++) {
		for (int j=startY; j<=endY; j++ ) {
            int pos_x = (i-startX) * 256 - offsetX;
            int pos_y = (j-startY) * 256 - offsetY;
            int idx_x = i;
            
            if ( i >= nn) {
                idx_x = i - nn;
            } else if (i < 0) {
                idx_x = nn + i;
            }
            
            int idx_y = j;
            wxString wxFilePath = GetTilePath(idx_x, idx_y);
            wxFileName fp(wxFilePath);
			wxBitmap bmp;
            if (imageSuffix.CmpNoCase(".png") == 0) {
                bmp.LoadFile(wxFilePath, wxBITMAP_TYPE_PNG);
            } else if (imageSuffix.CmpNoCase(".jpeg") == 0 ||
                       imageSuffix.CmpNoCase(".jpg") == 0 ) {
                bmp.LoadFile(wxFilePath, wxBITMAP_TYPE_JPEG);
            }
            if (bmp.IsOk()) {
                gc->DrawBitmap(bmp, pos_x, pos_y, 257,257);
                //dc.DrawRectangle((i-startX) * 256 - offsetX, (j-startY) * 256 - offsetY, 256, 256);
            }
		}
	}
    delete gc;
    isTileDrawn = true;
    return isTileReady;
}
