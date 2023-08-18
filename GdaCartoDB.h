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

class CartoDBProxy {

public:
	static CartoDBProxy& GetInstance() {
		static CartoDBProxy instance;
		return instance;
	}
   
	void Close();
    
    void SetKey(const wxString& key);
    
    void SetUserName(const wxString& name);
    
	wxString GetKey() const;
	wxString GetUserName() const;

    void UpdateColumn(const wxString& table_name,
                      const wxString& col_name,
                      std::vector<wxString>& vals);
    
    void UpdateColumn(const wxString& table_name,
                      const wxString& col_name,
                      std::vector<double>& vals);
    
    void UpdateColumn(const wxString& table_name,
                      const wxString& col_name,
                      std::vector<long long>& vals);
    
private:
    CartoDBProxy();
    
    CartoDBProxy(const wxString& _user_name, const wxString& _api_key);
    
    ~CartoDBProxy();
    
    wxString api_key;
    wxString user_name;
    wxString api_url;
    
    void doGet(wxString parameter);
    void doPost(wxString parameter);
    void _doGet(wxString parameter);
    void _doPost(wxString parameter);
    
    wxString buildUpdateSQL(const wxString& table_name,
                          const wxString& col_name,
                          const wxString &new_table);
    
    wxString buildBaseUrl();
};

#endif
