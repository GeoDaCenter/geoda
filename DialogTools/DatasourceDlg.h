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

#ifndef __GEODA_CENTER_DATASOURCE_DLG_H__
#define __GEODA_CENTER_DATASOURCE_DLG_H__


#include <vector>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/statbmp.h>
#include <wx/frame.h>
#include <wx/evtloop.h>

#include "../DataViewer/DataSource.h"
#include "AutoCompTextCtrl.h"


class DatasourceDlg : public wxFrame
{
protected:
    enum DS_IDS
    {
        ID_DS_START=8001
    };
    
public:
    DatasourceDlg();
    virtual ~DatasourceDlg();
	virtual void OnOkClick( wxCommandEvent& event ) = 0;

    void OnExit(wxCloseEvent& e);
    int GetType();
    
	wxFileName ds_file_path;
    
protected:
    wxEventLoop* eventLoop;
    bool is_ok_clicked;
    int type; /*0 connect 1 export 2 Merge*/
    
	wxTextCtrl*     m_ds_filepath_txt;
	wxBitmapButton* m_ds_browse_file_btn;
	//wxBitmapButton* m_database_lookup_table;
	//wxBitmapButton* m_database_lookup_wslayer;
	wxChoice*       m_database_type;
	AutoTextCtrl*   m_database_name;
	AutoTextCtrl*   m_database_host;
	AutoTextCtrl*   m_database_port;
	AutoTextCtrl*   m_database_uname;
	wxTextCtrl*     m_database_upwd;
	//AutoTextCtrl*   m_webservice_url;
	wxNotebook*     m_ds_notebook;
    wxMenu*         m_ds_menu;
	wxString        layer_name;

	wxTextCtrl*     m_cartodb_uname;
	wxTextCtrl*     m_cartodb_key;
	wxTextCtrl*     m_cartodb_table;
	wxStaticText*     m_cartodb_tablename;
    
    wxArrayString   ds_names;
    
public:
    void EndDialog();
    int ShowModal();
    void OnCancelClick( wxCommandEvent& event );
    
    void Init();
    void CreateControls();
	void PromptDSLayers(IDataSource* datasource);
    void OnBrowseDSfileBtn( wxCommandEvent& event );
    void BrowseDataSource( wxCommandEvent& event );
    void OnDropFiles(wxDropFilesEvent& event);
    
	wxString GetProjectTitle();
    wxString GetLayerName();
	
protected:
    wxString DBTYPE_ORACLE;
    wxString DBTYPE_ARCSDE;
    wxString DBTYPE_POSTGIS;
    wxString DBTYPE_MYSQL;
};

#endif
