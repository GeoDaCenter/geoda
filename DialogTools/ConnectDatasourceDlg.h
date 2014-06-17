/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_CONNECT_DATASOURCE_DLG_H__
#define __GEODA_CENTER_CONNECT_DATASOURCE_DLG_H__


#include <vector>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>

#include <wx/checkbox.h>
#include "../DataViewer/DataSource.h"
#include "AutoCompTextCtrl.h"
#include "DatasourceDlg.h"


class ConnectDatasourceDlg: public DatasourceDlg
{
public:
	ConnectDatasourceDlg(wxWindow* parent,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize );
    ~ConnectDatasourceDlg();
    
    void CreateControls();
    void OnOkClick( wxCommandEvent& event );
	void OnLookupWSLayerBtn( wxCommandEvent& event );
	void OnLookupDSTableBtn( wxCommandEvent& event );
	IDataSource* GetDataSource(){ return datasource;}
    
private:
	wxBitmapButton* m_database_lookup_table;
	wxBitmapButton* m_database_lookup_wslayer;
    wxTextCtrl*   m_database_table;
	AutoTextCtrl*   m_webservice_url;
	IDataSource*    datasource;
    
private:
    IDataSource* CreateDataSource();
    
	DECLARE_EVENT_TABLE()
};

#endif
