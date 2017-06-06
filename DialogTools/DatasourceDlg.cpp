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

#include <vector>
#include <string>
#include <fstream>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/progdlg.h>
#include <wx/regex.h>
#include <wx/textdlg.h>
#include <wx/xrc/xmlres.h>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>

#include "../Project.h"
#include "../DataViewer/DataSource.h"
#include "../DataViewer/DbfTable.h"
#include "../DataViewer/TableInterface.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "../GdaJson.h"
#include "../GdaCartoDB.h"
#include "DatasourceDlg.h"

using namespace std;

DatasourceDlg::DatasourceDlg()
: is_ok_clicked(false), eventLoop(NULL)
{
	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler(DatasourceDlg::OnExit) );
}

DatasourceDlg::~DatasourceDlg()
{
    if (eventLoop) {
        delete eventLoop;
        eventLoop = NULL;
    }
}

int DatasourceDlg::GetType()
{
    return type;
}

int DatasourceDlg::ShowModal()
{
    Show(true);
    
    // mow to stop execution start a event loop
    eventLoop = new wxEventLoop;
    if (eventLoop == NULL)
        return wxID_CANCEL;
    
    eventLoop->Run();
    
    if (is_ok_clicked)
        return wxID_OK;
    else
        return wxID_CANCEL;
}

void DatasourceDlg::EndDialog()
{
    eventLoop->Exit();
    Show(false);
    //Destroy();
}

void DatasourceDlg::OnCancelClick( wxCommandEvent& event )
{
    EndDialog();
}

void DatasourceDlg::OnExit(wxCloseEvent& e)
{
    EndDialog();
}

void DatasourceDlg::Init()
{
    m_ds_menu = NULL;
	ds_file_path = wxFileName("");
    
    // create file type dataset pop-up menu dynamically
	ds_names.Add("ESRI Shapefile (*.shp)|*.shp");
    ds_names.Add("ESRI File Geodatabase (*.gdb)|*.gdb");
    ds_names.Add("GeoJSON (*.geojson;*.json)|*.geojson;*.json");
    ds_names.Add("GeoPackage (*.gpkg)|*.gpkg");
    ds_names.Add("SQLite/SpatiaLite (*.sqlite)|*.sqlite");

    if( GeneralWxUtils::isWindows()){
        ds_names.Add("ESRI Personal Geodatabase (*.mdb)|*.mdb");
    }
    
    ds_names.Add("Geography Markup Language (*.gml)|*.gml");
    ds_names.Add("Keyhole Markup Language (*.kml)|*.kml");
    ds_names.Add("MapInfo (*.tab;*.mif;*.mid)|*.tab;*.mif;*.mid");
    ds_names.Add("dBase Database File (*.dbf)|*.dbf");
    ds_names.Add("Comma Separated Value (*.csv)|*.csv");
    ds_names.Add("MS Excel (*.xls)|*.xls");
    ds_names.Add("Open Document Spreadsheet (*.ods)|*.ods");

    //ds_names.Add("Idrisi Vector (*.vct)|*.vct");
    //ds_names.Add("MS Office Open XML Spreadsheet (*.xlsx)|*.xlsx");
    //ds_names.Add("OpenStreetMap XML and PBF (*.osm)|*.OSM;*.osm");
    //XXX: looks like tiger data are downloaded as Shapefile, geodatabase etc.
    //ds_names.Add("U.S. Census TIGER/Line (*.tiger)|*.tiger");
    
    // create database tab drop-down list items dynamically
    DBTYPE_ORACLE = "Oracle Spatial Database";
    if( GeneralWxUtils::isX64() ) {
        DBTYPE_ARCSDE = "ESRI ArcSDE (ver 10.x)";
    } else if ( GeneralWxUtils::isX86() ) {
        DBTYPE_ARCSDE = "ESRI ArcSDE (ver 9.x)";
    }
    DBTYPE_POSTGIS = "PostgreSQL/PostGIS Database";
    DBTYPE_MYSQL = "MySQL Spatial Database";
}

void DatasourceDlg::CreateControls()
{
#ifdef __WIN32__
    SetBackgroundColour(*wxWHITE);
#endif
    
    m_ds_filepath_txt = XRCCTRL(*this, "IDC_FIELD_ASC",wxTextCtrl);
	m_database_type = XRCCTRL(*this, "IDC_CDS_DB_TYPE",wxChoice);
	m_database_name = XRCCTRL(*this, "IDC_CDS_DB_NAME",AutoTextCtrl);
	m_database_host = XRCCTRL(*this, "IDC_CDS_DB_HOST",AutoTextCtrl);
	m_database_port = XRCCTRL(*this, "IDC_CDS_DB_PORT",AutoTextCtrl);
	m_database_uname = XRCCTRL(*this, "IDC_CDS_DB_UNAME",AutoTextCtrl);
	m_database_upwd = XRCCTRL(*this, "IDC_CDS_DB_UPWD",wxTextCtrl);
	//m_database_table = XRCCTRL(*this, "IDC_CDS_DB_TABLE",AutoTextCtrl);
	m_ds_notebook = XRCCTRL(*this, "IDC_DS_NOTEBOOK", wxNotebook);
    
#ifdef __WIN32__
    m_ds_notebook->SetBackgroundColour(*wxWHITE);
#endif
	m_ds_browse_file_btn = XRCCTRL(*this, "IDC_OPEN_IASC",wxBitmapButton);
	
    m_cartodb_uname = XRCCTRL(*this, "IDC_CARTODB_USERNAME",wxTextCtrl);
    m_cartodb_key = XRCCTRL(*this, "IDC_CARTODB_KEY",wxTextCtrl);
    m_cartodb_table = XRCCTRL(*this, "IDC_CARTODB_TABLE_NAME",wxTextCtrl);
    m_cartodb_tablename = XRCCTRL(*this, "IDC_STATIC_CARTODB_TABLE_NAME",wxStaticText);
    
	m_database_type->Append(DBTYPE_POSTGIS);
    m_database_type->Append(DBTYPE_ORACLE);
    if(GeneralWxUtils::isWindows()) m_database_type->Append(DBTYPE_ARCSDE);
    m_database_type->Append(DBTYPE_MYSQL);
    m_database_type->SetSelection(0);
    
    // for autocompletion of input boxes in Database Tab
	vector<string> host_cands =
		OGRDataAdapter::GetInstance().GetHistory("db_host");
	vector<string> port_cands =
        OGRDataAdapter::GetInstance().GetHistory("db_port");
	vector<string> uname_cands =
        OGRDataAdapter::GetInstance().GetHistory("db_user");
	vector<string> name_cands =
        OGRDataAdapter::GetInstance().GetHistory("db_name");

	m_database_host->SetAutoList(host_cands);
	m_database_port->SetAutoList(port_cands);
	m_database_uname->SetAutoList(uname_cands);
	m_database_name->SetAutoList(name_cands);
    
    // get a latest input DB information
    vector<string> db_infos = OGRDataAdapter::GetInstance().GetHistory("db_info");
    if (db_infos.size() > 0) {
        string db_info = db_infos[0];
        json_spirit::Value v;
        // try to parse as JSON
        try {
            if (!json_spirit::read( db_info, v)) {
                throw runtime_error("Could not parse title as JSON");
            }
            json_spirit::Value json_db_type;
            if (GdaJson::findValue(v, json_db_type, "db_type")) {
                wxString db_type(json_db_type.get_str());
                if(db_type == DBTYPE_POSTGIS)
                     m_database_type->SetSelection(0);
                if(db_type == DBTYPE_ORACLE)
                    m_database_type->SetSelection(1);
                if(db_type == DBTYPE_MYSQL)
                    m_database_type->SetSelection(2);
                if(db_type == DBTYPE_ARCSDE)
                    m_database_type->SetSelection(3);
            }
            json_spirit::Value json_db_host;
            if (GdaJson::findValue(v, json_db_host, "db_host")) {
                m_database_host->SetValue(json_db_host.get_str());
            }
            json_spirit::Value json_db_port;
            if (GdaJson::findValue(v, json_db_port, "db_port")) {
                m_database_port->SetValue(json_db_port.get_str());
            }
            json_spirit::Value json_db_name;
            if (GdaJson::findValue(v, json_db_name, "db_name")) {
                m_database_name->SetValue(json_db_name.get_str());
            }
            json_spirit::Value json_db_user;
            if (GdaJson::findValue(v, json_db_user, "db_user")) {
                m_database_uname->SetValue(json_db_user.get_str());
            }
            json_spirit::Value json_db_pwd;
            if (GdaJson::findValue(v, json_db_pwd, "db_pwd")) {
                m_database_upwd->SetValue(json_db_pwd.get_str());
            }

        } catch (std::runtime_error e) {
            wxString msg;
            msg << "Get Latest DB infor: JSON parsing failed: ";
            msg << e.what();
            LOG_MSG(msg);
        }
    }
    
    // get a latest CartoDB account
    vector<string> cartodb_user = OGRDataAdapter::GetInstance().GetHistory("cartodb_user");
    if (!cartodb_user.empty()) {
        string user = cartodb_user[0];
        CartoDBProxy::GetInstance().SetUserName(user);
        // control
        m_cartodb_uname->SetValue(user);
    }
    
    vector<string> cartodb_key = OGRDataAdapter::GetInstance().GetHistory("cartodb_key");
    if (!cartodb_key.empty()) {
        string key = cartodb_key[0];
        CartoDBProxy::GetInstance().SetKey(key);
        // control
        m_cartodb_key->SetValue(key);
    }
    
    m_cartodb_table->Hide();
    m_cartodb_tablename->Hide();
}

void DatasourceDlg::OnDropFiles(wxDropFilesEvent& event)
{
    if (event.GetNumberOfFiles() > 0) {
        
        wxString* dropped = event.GetFiles();         
        wxString name;
       
        for (int i = 0; i < event.GetNumberOfFiles(); i++) {
            name = dropped[i];
            if (wxFileExists(name)) {
                
			} else if (wxDirExists(name)) {
				//wxArrayString files;
				//wxDir::GetAllFiles(name, &files);
			}
        }
    }
}

/**
 * Prompt layer names of input datasource to user for selection.
 * This function is used by OnLookupDSTableBtn, OnOkClick
 */
void DatasourceDlg::PromptDSLayers(IDataSource* datasource)
{
	wxString ds_name = datasource->GetOGRConnectStr();
    GdaConst::DataSourceType ds_type = datasource->GetType();

	if (ds_name.IsEmpty()) {
        wxString msg = _("Can't get layers from unknown datasource. Please complete the datasource fields.");
		throw GdaException(msg.mb_str());
	}
    
	vector<wxString> table_names =  OGRDataAdapter::GetInstance().GetLayerNames(ds_name, ds_type);
    
    int n_tables = table_names.size();
    
	if ( n_tables > 0 ) {
		wxString *choices = new wxString[n_tables];
        for	(int i=0; i<n_tables; i++)  {
			choices[i] = table_names[i];
        }
		wxSingleChoiceDialog choiceDlg(NULL, _("Please select the layer name to connect:"), _("Layer names"), n_tables, choices);
        
		if (choiceDlg.ShowModal() == wxID_OK) {
			if (choiceDlg.GetSelection() >= 0) {
				layer_name = choiceDlg.GetStringSelection();
			}
		}
		delete[] choices;
        
	} else if ( n_tables == 0) {
		wxMessageDialog dlg(NULL, _("No layer was found in the selected data source."), _("Info"), wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
        
	} else {
        wxString msg = _("No layer has been selected. Please select a layer.");
		throw GdaException(msg.mb_str());
	}
}

/**
 * This function handles the event of user click the open button in File Tab
 * When click browse datasource file button
 */
void DatasourceDlg::OnBrowseDSfileBtn ( wxCommandEvent& event )
{
	// create pop-up datasource menu dynamically
    if ( m_ds_menu == NULL ){
        m_ds_menu = new wxMenu;
        for ( size_t i=0; i < ds_names.GetCount(); i++ ) {
            if (ds_names[i].IsEmpty()) {
                m_ds_menu->AppendSeparator();
            } else {
                m_ds_menu->Append( ID_DS_START + i, ds_names[i].BeforeFirst('|'));
            }
        }
    }

    wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
    OnActivate(ae);
    
    PopupMenu(m_ds_menu);
    
    event.Skip();
}

/**
 * This function handles the event of user click the pop-up menu of file type
 * data sources.
 * For some menu options (e.g. ESR File GDB, or ODBC based mdb), a directory
 * selector or input dialog need to be prompted to users for further information
 * Otherwise, prompt a file selector to user.
 */
void DatasourceDlg::BrowseDataSource( wxCommandEvent& event)
{
    
	int index = event.GetId() - ID_DS_START;
    wxString name = ds_names[index];
    
    if (name.Contains("gdb")) {
        // directory data source, such as ESRI .gdb directory
        wxDirDialog dlg(this, "Choose a spatial diretory to open","");
        if (dlg.ShowModal() != wxID_OK) return;
        ds_file_path = dlg.GetPath();
        m_ds_filepath_txt->SetValue(ds_file_path.GetFullPath());
        FindWindow(XRCID("wxID_OK"))->Enable(true);
		OnOkClick(event);
    }
#if defined(_WIN64) || defined(__amd64__)
    else if (name.Contains("mdb")){
        // 64bit Windows only accept DSN ODBC for ESRI PGeo
        wxTextEntryDialog dlg(this, "Input the DSN name of ESRI Personal "
                              "Geodatabase (.mdb) that is created in ODBC Data "
                              "Source Administrator dialog box.", "DSN name:",
                              "");
        if (dlg.ShowModal() != wxID_OK) return;
        ds_file_path = dlg.GetValue().Trim();
        m_ds_filepath_txt->SetValue( "PGeo:"+dlg.GetValue() );
		FindWindow(XRCID("wxID_OK"))->Enable(true);
    }
#endif
    else {
        // file data source
        wxString wildcard;
        wildcard << name;
        wxFileDialog dlg(this,"Choose a spatial file to open", "","",wildcard);
        if (dlg.ShowModal() != wxID_OK) return;
        
        ds_file_path = dlg.GetPath();
        m_ds_filepath_txt->SetValue(ds_file_path.GetFullPath());
		
        FindWindow(XRCID("wxID_OK"))->Enable(true);
		OnOkClick(event);
    }
}

wxString DatasourceDlg::GetProjectTitle()
{
    return wxString(layer_name);
}
wxString DatasourceDlg::GetLayerName()
{
    return wxString(layer_name);
}
