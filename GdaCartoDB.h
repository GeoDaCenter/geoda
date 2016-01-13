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

#ifndef __GEODA_CENTER_GDA_CARTODB_H_
#define __GEODA_CENTER_GDA_CARTODB_H_

#include <string>
#include <vector>

using namespace std;

class CartoDBProxy {

public:
	static CartoDBProxy& GetInstance() {
		static CartoDBProxy instance;
		return instance;
	}
   
	void Close();
    
    void SetKey(const string& key);
    
    void SetUserName(const string& name);
    
	string GetKey() const;
	string GetUserName() const;

    void UpdateColumn(const string& table_name,
                      const string& col_name,
                      vector<wxString>& vals);
    
    void UpdateColumn(const string& table_name,
                      const string& col_name,
                      vector<double>& vals);
    
    void UpdateColumn(const string& table_name,
                      const string& col_name,
                      vector<long long>& vals);
    
private:
    CartoDBProxy();
    
    CartoDBProxy(const string& _user_name, const string& _api_key);
    
    ~CartoDBProxy();
    
    string api_key;
    string user_name;
    string api_url;
    
    void doGet(string parameter);
    void doPost(string parameter);
    void _doGet(string parameter);
    void _doPost(string parameter);
    
    string buildUpdateSQL(const string& table_name,
                          const string& col_name,
                          const string &new_table);
    
    string buildBaseUrl();
};

#endif