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

#ifndef __GEODA_CENTER_GEODAWEB_PROXY_H_
#define __GEODA_CENTER_GEODAWEB_PROXY_H_

#include <string>
#include <vector>
#include <map>
#include "Project.h"
using namespace std;


class GeoDaWebProxy {

public:
    GeoDaWebProxy();
    
    GeoDaWebProxy(const string& _user_name, const string& _api_key);
    
    ~GeoDaWebProxy();

	void SetKey(const string& key);
	void SetUserName(const string& name);
	
	void Publish(Project* p, wxString& title, wxString& description);
	
    
private:
    
    string api_key;
    string user_name;
    string api_url;
    
	string buildParameter(const char* key, string& val);
	string buildParameter(const char* key, wxString& val);
	string buildParameter(const char* key, vector<int>& val);
	string buildParameter(const char* key, vector<wxString>& val);
	string buildParameter(map<wxString, vector<int> >& val);
	
    void doGet(string parameter);
    string doPost(string parameter);

    
    string buildBaseUrl();
};

#endif