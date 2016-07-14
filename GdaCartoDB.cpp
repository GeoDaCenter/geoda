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

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <boost/thread/thread.hpp>

#include <curl/curl.h>
#include <wx/string.h>

#include "GdaCartoDB.h"

using namespace std;

CartoDBProxy::CartoDBProxy()
{
    user_name = "";
	api_key = "";
}


CartoDBProxy::CartoDBProxy(const string& _user_name, const string& _api_key)
{
    user_name = _user_name;
    api_key = _api_key;
    
}

CartoDBProxy::~CartoDBProxy() {
    
}

string CartoDBProxy::GetKey() const {
	return api_key;
}

string CartoDBProxy::GetUserName() const {
	return user_name;
}

void CartoDBProxy::Close() {
    
}

void CartoDBProxy::SetKey(const string& key) {
    api_key = key;
}

void CartoDBProxy::SetUserName(const string& name) {
    user_name = name;
}

string CartoDBProxy::buildBaseUrl()
{
    ostringstream url;
    url << "https://" << user_name << ".cartodb.com/api/v2/sql";
    return url.str();
}

string CartoDBProxy::buildUpdateSQL(const string& table_name, const string& col_name, const string &new_table)
{
    /**
     update test as t set
     column_a = c.column_a
     from (values
     ('123', 1),
     ('345', 2)
     ) as c(column_b, column_a)
     where c.column_b = t.column_b;
     */
    
    ostringstream sql;
    sql << "UPDATE " << table_name << " t "
    << "SET " << col_name << " = c.val  FROM (VALUES"
    << new_table.substr(0, new_table.length()-1)
    << ") AS c(id,val) "
    << "WHERE c.id = t.cartodb_id ";
   
    return sql.str();
}

void CartoDBProxy::UpdateColumn(const string& table_name, const string& col_name, vector<wxString>& vals)
{
    ostringstream ss_newtable;
    for (size_t i=0, n=vals.size(); i<n; i++) {
        ss_newtable << "(" << i+1 << ", '" << vals[i] << "'),";
    }
    
    string sql = buildUpdateSQL(table_name, col_name, ss_newtable.str());
    _doPost("q=" + sql);
}

void CartoDBProxy::UpdateColumn(const string& table_name, const string& col_name, vector<double>& vals)
{
    ostringstream ss_newtable;
    ss_newtable.precision(std::numeric_limits<double>::digits10);
    
    for (size_t i=0, n=vals.size(); i<n; i++) {
        ss_newtable << "(" << i+1 << ", " << vals[i] << "),";
    }
    
    string sql = buildUpdateSQL(table_name, col_name, ss_newtable.str());
    _doPost("q=" + sql);
}

void CartoDBProxy::UpdateColumn(const string& table_name, const string& col_name, vector<long long>& vals)
{
    ostringstream ss_newtable;
    
    for (size_t i=0, n=vals.size(); i<n; i++) {
        ss_newtable << "(" << i+1 << ", " << vals[i] << "),";
    }
    
    string sql = buildUpdateSQL(table_name, col_name, ss_newtable.str());
    _doPost("q=" + sql);
}

void CartoDBProxy::doGet(string parameter)
{
    boost::thread t(boost::bind(&CartoDBProxy::_doGet, this, parameter));
    t.join();
}
void CartoDBProxy::doPost(string parameter)
{
    boost::thread t(boost::bind(&CartoDBProxy::_doPost, this, parameter));
    t.join();
}

void CartoDBProxy::_doGet(string parameter)
{
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    
    curl = curl_easy_init();
    if (curl) {
        string url = buildBaseUrl() + "?api_key=" + api_key +"&" + parameter;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        
        // Grab image 
        res = curl_easy_perform(curl);
        if( res ) {
            printf("Cannot connect carto.com!\n");
        } 
        
        int res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %d\n", res_code);
            return;
        }
    }
    // Clean up the resources 
    curl_easy_cleanup(curl);
   
    curl_global_cleanup();
    
}
void CartoDBProxy::_doPost(string parameter)
{
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    
    curl = curl_easy_init();
    if (curl) {
        string url = buildBaseUrl();
        parameter = "api_key=" + api_key + "&" + parameter;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, parameter.c_str());
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        //curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        
        // Grab image 
        res = curl_easy_perform(curl);
        if( res ) {
            printf("Cannot connect cartodb.com!\n");
        } 
        
        int res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %d\n", res_code);
        }
		// Clean up the resources 
		curl_easy_cleanup(curl);
    }
    
   
    curl_global_cleanup();
    
}