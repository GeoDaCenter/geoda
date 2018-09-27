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

#ifndef __GEODA_CENTER_GEOCODING_H
#define __GEODA_CENTER_GEOCODING_H

#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/gauge.h>

#include <vector>

#include <curl/curl.h>

using namespace std;

class GeoCodingInterface {
public:
    /**
     * Performs eigenvector decomposition of an affinity matrix
     *
     * @param data 		the affinity matrix
     * @param numDims	the number of dimensions to consider when clustering
     */
    GeoCodingInterface();
    
    virtual ~GeoCodingInterface();
    
    virtual void run();
    
    /**
     * Cluster by kmeans
     *
     * @param numClusters	the number of clusters to assign
     */
    virtual void geocoding(vector<wxString>& address, vector<double>& _lats, vector<double>& _lngs, vector<bool>& _undefs, wxGauge* prg, bool* stop);
    
    int* count;
    
    bool* stop;
    
    vector<wxString> addresses;
    
    vector<double> lats;
    
    vector<double> lngs;
    
    vector<bool> undefs;
    
    wxString error_msg;
    
protected:
   
    bool doGet(CURL* curl, const char* url, string& response);
    
    virtual wxString create_request_url(const wxString& addr) = 0;
    
    virtual int retrive_latlng(const string& response, double* lat, double* lng) = 0;
};


class GoogleGeoCoder : public GeoCodingInterface {
    
public:
    GoogleGeoCoder(vector<wxString>& _keys);
    
    virtual ~GoogleGeoCoder();
    
protected:
    
    virtual wxString create_request_url(const wxString& addr);
    
    virtual int retrive_latlng(const string& response, double* lat, double* lng);
   
    wxString key;
    
    vector<wxString> keys;
    
    wxString url_template;
    
    wxString get_next_key();
};


#endif
