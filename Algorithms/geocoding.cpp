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
 *
 * Created 5/30/2017 lixun910@gmail.com
 */

#include <map>
#include <math.h>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/thread/thread.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/chrono.hpp>

#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include <json_spirit/json_spirit_reader.h>
#include <wx/thread.h>

#include <curl/curl.h>
#include <wx/string.h>
#include <wx/gauge.h>

#include "../logger.h"
#include "../GdaJson.h"
#include "geocoding.h"

using namespace std;


GeoCodingInterface::GeoCodingInterface()
{
}

GeoCodingInterface::~GeoCodingInterface()
{
}

size_t dump_to_string(void *ptr, size_t size, size_t count, void *stream) {
    ((string*)stream)->append((char*)ptr, 0, size*count);
    return size*count;
}

bool GeoCodingInterface::doGet(CURL* curl, const char* url, string& response)
{
    wxLogMessage("AutoUpdate::ReadUrlContent()");
    
    CURLcode res;
    int res_code = 0;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dump_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        
        res = curl_easy_perform(curl);
        
        //curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
    }
    
    //if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
    //{
    //    return false;
    //}
    return true;
}

void GeoCodingInterface::run(){
}

void GeoCodingInterface::geocoding(vector<wxString>& _addresses, vector<double>& _lats, vector<double>& _lngs, vector<bool>& _undefs, wxGauge* _prg, bool* _stop)
{
    lats.clear();
    lngs.clear();
    undefs.clear();
    addresses.clear();
    
    lats = _lats;
    lngs = _lngs;
    undefs = _undefs;
    addresses = _addresses;
    stop = _stop;
    
    CURL* curl = curl_easy_init();
    
    int out_limit_count = 0;
    
    for ( int i=0, n=addresses.size(); i<n; i++) {
        if (*stop)
            break;
        if (out_limit_count>10)
            break;
        if (undefs[i] == false && lats[i] != 0 && lngs[i] != 0)
            continue;
        const wxString& addr = addresses[i];
        wxString url = create_request_url(addr);
        string response;
        // send request to server
        doGet(curl, url.c_str(), response);
        double lat=0;
        double lng=0;
        int rtn = retrive_latlng(response, &lat, &lng);
        if (rtn==-1) {
            out_limit_count ++;
            wxSleep(2);
        }
        lats[i] = lat;
        lngs[i] = lng;
        undefs[i] = (rtn != 1);
        _prg->SetValue(i+1);
        LOG_MSG(url);
        LOG_MSG(lat);
        //wxMilliSleep(50);
    }
    
    curl_easy_cleanup(curl);
}


/////////////////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////////////////

GoogleGeoCoder::GoogleGeoCoder(vector<wxString>& _keys)
{
    keys = _keys;
    key = get_next_key();
    url_template = "https://maps.googleapis.com/maps/api/geocode/json?address=%s&key=%s";
}

GoogleGeoCoder::~GoogleGeoCoder()
{
    
}

wxString GoogleGeoCoder::create_request_url(const wxString &_addr)
{
    wxString addr = _addr;
    //addr.Replace(" ", "+");
    wxString url = wxString::Format(url_template, addr, key);
    return url;
}

wxString GoogleGeoCoder::get_next_key()
{
    if (!keys.empty()) {
        key = keys.back();
        keys.pop_back();
    }
    return key;
}

int GoogleGeoCoder::retrive_latlng(const string& response,  double* lat, double* lng)
{
    /*
     {
       "results" : [
         { 
           "geometry" : {
             "location" : {
               "lat" : 37.4224764,
               "lng" : -122.0842499
             }
     */
    json_spirit::Value v;
    try {
        if (!json_spirit::read(response, v)) {
            throw std::runtime_error("Could not parse recent ds string");
        }
        
        json_spirit::Value json_results;
        json_spirit::Value json_status;
        if (GdaJson::findValue(v, json_status, "status")) {
            string stat = json_status.get_str();
            if (stat.compare("OVER_QUERY_LIMIT")==0 ||
                stat.compare("REQUEST_DENIED") ==0) {
                error_msg << stat << ": " << key << "\n\n";
                key = get_next_key();
                return -1;
            }
        }
        if (GdaJson::findValue(v, json_results, "results")) {
            const json_spirit::Array& results = json_results.get_array();
            if (results.size() == 0)
                return 0;
            const json_spirit::Object& o = results[0].get_obj();
            
            json_spirit::Value json_geometry;
            if (GdaJson::findValue(o, json_geometry, "geometry")) {
                json_spirit::Value json_location;
                if (GdaJson::findValue(json_geometry, json_location, "location")) {
                    json_spirit::Value json_lat;
                    json_spirit::Value json_lng;
                    GdaJson::findValue(json_location, json_lat, "lat");
                    GdaJson::findValue(json_location, json_lng, "lng");
                    *lat = json_lat.get_real();
                    *lng = json_lng.get_real();
                    return 1;
                }
            }
        }
        
    } catch (std::runtime_error e) {
        return 0;
    }
    return 0;
}
