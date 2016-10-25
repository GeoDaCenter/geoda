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
#include <algorithm>
#include <string>
#include <fstream>
#include <wx/progdlg.h>
#include <wx/filedlg.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/regex.h>
#include <wx/dnd.h>
#include <wx/statbmp.h>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include <json_spirit/json_spirit_reader.h>

#include "../DialogTools/CsvFieldConfDlg.h"
#include "../DataViewer/DataSource.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "../GdaCartoDB.h"
#include "../GeoDa.h"
#include "ConnectDatasourceDlg.h"
#include "DatasourceDlg.h"

class DnDFile : public wxFileDropTarget
{
public:
    DnDFile(ConnectDatasourceDlg *pOwner = NULL) { m_pOwner = pOwner; }
    
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
    
private:
    ConnectDatasourceDlg *m_pOwner;
};

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
    size_t nFiles = filenames.GetCount();
    
    if (m_pOwner != NULL && nFiles > 0)
    {
        wxFileName fn = wxFileName::FileName(filenames[0]);
        m_pOwner->ds_file_path = fn;
        wxCommandEvent ev;
        m_pOwner->OnOkClick(ev);
    }
    
    return true;
}


BEGIN_EVENT_TABLE( ConnectDatasourceDlg, wxDialog )
    EVT_BUTTON(XRCID("IDC_OPEN_IASC"), ConnectDatasourceDlg::OnBrowseDSfileBtn)
	EVT_BUTTON(XRCID("ID_BTN_LOOKUP_TABLE"), ConnectDatasourceDlg::OnLookupDSTableBtn)
	//EVT_BUTTON(XRCID("ID_CARTODB_LOOKUP_TABLE"), ConnectDatasourceDlg::OnLookupCartoDBTableBtn)
	//EVT_BUTTON(XRCID("ID_BTN_LOOKUP_WSLAYER"), ConnectDatasourceDlg::OnLookupWSLayerBtn)
    EVT_BUTTON(wxID_OK, ConnectDatasourceDlg::OnOkClick )
END_EVENT_TABLE()

using namespace std;

ConnectDatasourceDlg::ConnectDatasourceDlg(wxWindow* parent, const wxPoint& pos, const wxSize& size)
:datasource(0)
{
    // init controls defined in parent class
    DatasourceDlg::Init();
    ds_names.Add("GeoDa Project File (*.gda)|*.gda");

	SetParent(parent);
	CreateControls();
	SetPosition(pos);
	Centre();
   
    m_drag_drop_box->SetDropTarget(new DnDFile(this));
    
    Bind(wxEVT_COMMAND_MENU_SELECTED, &ConnectDatasourceDlg::BrowseDataSource, this, DatasourceDlg::ID_DS_START, ID_DS_START + ds_names.Count());
}

ConnectDatasourceDlg::~ConnectDatasourceDlg()
{
	if (datasource) delete datasource;
}

void ConnectDatasourceDlg::CreateControls()
{
    
    bool test = wxXmlResource::Get()->LoadDialog(this, GetParent(),"IDD_CONNECT_DATASOURCE");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
    // init db_table control that is unique in this class
    m_drag_drop_box = XRCCTRL(*this, "IDC_DRAG_DROP_BOX",wxStaticBitmap);
	m_webservice_url = XRCCTRL(*this, "IDC_CDS_WS_URL",AutoTextCtrl);
	m_database_lookup_table = XRCCTRL(*this, "ID_BTN_LOOKUP_TABLE",  wxBitmapButton);
    m_database_lookup_table->Hide();
	//m_database_lookup_wslayer = XRCCTRL(*this, "ID_BTN_LOOKUP_WSLAYER", wxBitmapButton);
	m_database_table = XRCCTRL(*this, "IDC_CDS_DB_TABLE", wxTextCtrl);
    m_database_table->Hide(); // don't need this
    XRCCTRL(*this, "IDC_STATIC_DB_TABLE", wxStaticText)->Hide();
    
    // create controls defined in parent class
    DatasourceDlg::CreateControls();
	
    // setup WSF auto-completion
	std::vector<std::string> ws_url_cands = OGRDataAdapter::GetInstance().GetHistory("ws_url");
	m_webservice_url->SetAutoList(ws_url_cands);
}

/**
 * This functions handles the event of user click the "lookup" button in
 * Web Service tab
 */
void ConnectDatasourceDlg::OnLookupWSLayerBtn( wxCommandEvent& event )
{
    //XXX handling invalid wfs url:
    //http://walkableneighborhoods.org/geoserver/wfs
	OnLookupDSTableBtn(event);
}

/**
 * This functions handles the event of user click the "lookup" button in
 * Database Tab.
 * When click "lookup" button in Datasource Table, this will call
 * PromptDSLayers() function
 */
void ConnectDatasourceDlg::OnLookupDSTableBtn( wxCommandEvent& event )
{
	try {
        CreateDataSource();
        PromptDSLayers(datasource);
        m_database_table->SetValue(layer_name );
	} catch (GdaException& e) {
		wxString msg;
		msg << e.what();
		wxMessageDialog dlg(this, msg , "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        if( datasource!= NULL &&
            msg.StartsWith("Failed to open data source") ) {
            if ( datasource->GetType() == GdaConst::ds_oci ){
               wxExecute("open http://geodacenter.github.io/setup-oracle.html");
            } else if ( datasource->GetType() == GdaConst::ds_esri_arc_sde ){
               wxExecute("open http://geodacenter.github.io/etup-arcsde.html");
			} else if ( datasource->GetType() == GdaConst::ds_esri_file_geodb ){
				wxExecute("open http://geodacenter.github.io/setup-esri-fgdb.html");
            } else {
				wxExecute("open http://geodacenter.github.io/formats.html");
			}
        }
	}
}

void ConnectDatasourceDlg::OnLookupCartoDBTableBtn( wxCommandEvent& event )
{
	try {
        CreateDataSource();
        PromptDSLayers(datasource);
    } catch(GdaException& e) {
        
    }
}


/**
 * This function handles the event of user click OK button.
 * When user chooses a data source, validate it first,
 * then create a Project() that will be used by the
 * main program.
 */
void ConnectDatasourceDlg::OnOkClick( wxCommandEvent& event )
{
	LOG_MSG("Entering ConnectDatasourceDlg::OnOkClick");
	try {
        // Open GeoDa project file direclty
        if (ds_file_path.GetExt().Lower() == "gda") {
            GdaFrame* gda_frame = GdaFrame::GetGdaFrame();
            if (gda_frame) {
                gda_frame->OpenProject(ds_file_path.GetFullPath());
                EndDialog(wxID_CANCEL);
            }
            return;
        }
       
        // For csv file, if no csvt file, pop-up a field definition dialog and create a csvt file
        if (ds_file_path.GetExt().Lower() == "csv") {
            wxString csv_path = ds_file_path.GetFullPath();
            CsvFieldConfDlg csvDlg(this, csv_path);
            csvDlg.ShowModal();
        }
        
		CreateDataSource();
        
        // Check to make sure to get a layer name
        wxString layername;
		int datasource_type = m_ds_notebook->GetSelection();
		if (datasource_type == 0) {
            // File table is selected
			if (layer_name.IsEmpty()) {
				layername = ds_file_path.GetName();
			} else {
                // user may select a layer name from Popup dialog that displays
                // all layer names, see PromptDSLayers()
				layername = layer_name;
			}
            
		} else if (datasource_type == 1) {
            // Database tab is selected
			layername = m_database_table->GetValue();
            if (layername.IsEmpty()) PromptDSLayers(datasource);
			layername = layer_name;
            
		} else if (datasource_type == 2) {
            // Web Service tab is selected
            if (layer_name.IsEmpty()) PromptDSLayers(datasource);
			layername = layer_name;
            
		} else if (datasource_type == 3) {
            // CartoDB Service tab is selected
            if (layer_name.IsEmpty()) PromptDSLayers(datasource);
			layername = layer_name;
            
		} else {
            // Should never be here
			return;
		}
        
        if (layername.IsEmpty())
            return;
        
		// At this point, there is a valid datasource and layername.
        if (layer_name.IsEmpty())
            layer_name = layername;
       
        SaveRecentDataSource(datasource, layer_name);
        
        EndDialog(wxID_OK);
		
	} catch (GdaException& e) {
		wxString msg;
		msg << e.what();
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        
	} catch (...) {
		wxString msg = "Unknow exception. Please contact GeoDa support.";
		wxMessageDialog dlg(this, msg , "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
	LOG_MSG("Exiting ConnectDatasourceDlg::OnOkClick");
}

/**
 * After user click OK, create a data source connection string based on user
 * inputs
 * Throw GdaException()
 */
IDataSource* ConnectDatasourceDlg::CreateDataSource()
{
	if (datasource) {
		delete datasource;
		datasource = NULL;
	}
    
	int datasource_type = m_ds_notebook->GetSelection();
	
	if (datasource_type == 0) {
        // File  tab selected
		wxString fn = ds_file_path.GetFullPath();
		if (fn.IsEmpty()) {
			throw GdaException(
                wxString("Please select a datasource file.").mb_str());
		}
#if defined(_WIN64) || defined(__amd64__)
        if (m_ds_filepath_txt->GetValue().StartsWith("PGeo:")) {
			fn = "PGeo:" + fn;
		}
#endif
        datasource = new FileDataSource(fn);
        
		// a special case: sqlite is a file based database, so we need to get
		// avalible layers and prompt for user selecting
        if (datasource->GetType() == GdaConst::ds_sqlite ||
            datasource->GetType() == GdaConst::ds_gpkg||
            datasource->GetType() == GdaConst::ds_osm ||
            datasource->GetType() == GdaConst::ds_esri_personal_gdb||
            datasource->GetType() == GdaConst::ds_esri_file_geodb)
        {
            PromptDSLayers(datasource);
            if (layer_name.IsEmpty()) {
                throw GdaException(
                    wxString("Layer/Table name could not be empty. Please select a layer/table.").mb_str());
            }
        }
		
	} else if (datasource_type == 1) {
        // Database tab selected
		//int cur_sel = m_database_type->GetSelection();
        wxString cur_sel = m_database_type->GetStringSelection();
		wxString dbname = m_database_name->GetValue().Trim();
		wxString dbhost = m_database_host->GetValue().Trim();
		wxString dbport = m_database_port->GetValue().Trim();
		wxString dbuser = m_database_uname->GetValue().Trim();
		wxString dbpwd  = m_database_upwd->GetValue().Trim();
        

        
        GdaConst::DataSourceType ds_type = GdaConst::ds_unknown;
        if (cur_sel == DBTYPE_ORACLE) ds_type = GdaConst::ds_oci;
        else if (cur_sel == DBTYPE_ARCSDE) ds_type = GdaConst::ds_esri_arc_sde;
        else if (cur_sel == DBTYPE_POSTGIS) ds_type = GdaConst::ds_postgresql;
        else if (cur_sel == DBTYPE_MYSQL) ds_type = GdaConst::ds_mysql;
        //else if (cur_sel == 4) ds_type = GdaConst::ds_ms_sql;
        else {
            wxString msg = "The selected database driver is not supported on this platform. Please check GeoDa website for more information about database support and connection.";
            throw GdaException(msg.mb_str());
        }
        
        // save user inputs to history table
        OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
        if (!dbhost.IsEmpty())
            ogr_adapter.AddHistory("db_host", dbhost.ToStdString());
        
        if (!dbname.IsEmpty())
            ogr_adapter.AddHistory("db_name", dbname.ToStdString());
        
        if (!dbport.IsEmpty())
            ogr_adapter.AddHistory("db_port", dbport.ToStdString());
        
        if (!dbuser.IsEmpty())
            ogr_adapter.AddHistory("db_user", dbuser.ToStdString());
        
        // check if empty, prompt user to input
        wxRegEx regex;
        regex.Compile("[0-9]+");
        if (!regex.Matches( dbport )){
            throw GdaException(wxString("Database port is empty. Please input one.").mb_str());
        }
		wxString error_msg;
		if (dbhost.IsEmpty()) error_msg = "Please input database host.";
		else if (dbname.IsEmpty()) error_msg = "Please input database name.";
		else if (dbport.IsEmpty()) error_msg = "Please input database port.";
		else if (dbuser.IsEmpty()) error_msg = "Please input user name.";
        else if (dbpwd.IsEmpty()) error_msg = "Please input password.";
		if (!error_msg.IsEmpty()) {
			throw GdaException(error_msg.mb_str() );
		}
        
        // save current db info
        json_spirit::Object ret_obj;
        ret_obj.push_back(json_spirit::Pair("db_host", dbhost.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_port", dbport.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_name", dbname.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_user", dbuser.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_pwd", dbpwd.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_type", cur_sel.ToStdString()));
        
        std::string json_str = json_spirit::write(ret_obj);
        OGRDataAdapter::GetInstance().AddEntry("db_info", json_str);
        
        datasource = new DBDataSource(ds_type, dbname, dbhost, dbport, dbuser, dbpwd);
        
	} else if ( datasource_type == 2 ) {
        // Web Service tab selected
        wxString ws_url = m_webservice_url->GetValue().Trim();
        // detect if it's a valid url string
        wxRegEx regex;
        regex.Compile("^(https|http)://");
        if (!regex.Matches( ws_url )){
            throw GdaException(
                wxString("Please input a valid WFS url address.").mb_str());
        }
        if (ws_url.IsEmpty()) {
            throw GdaException(
                wxString("Please input WFS url.").mb_str());
        } else {
            OGRDataAdapter::GetInstance().AddHistory("ws_url", ws_url.ToStdString());
        }
        if ((!ws_url.StartsWith("WFS:") || !ws_url.StartsWith("wfs:"))
            && !ws_url.EndsWith("SERVICE=WFS")) {
            ws_url = "WFS:" + ws_url;
        }
        datasource = new WebServiceDataSource(GdaConst::ds_wfs, ws_url);
        // prompt user to select a layer from WFS
        //if (layer_name.IsEmpty()) PromptDSLayers(datasource);
        
	} else if ( datasource_type == 3 ) {
        
        std::string user(m_cartodb_uname->GetValue().Trim().mb_str());
        std::string key(m_cartodb_key->GetValue().Trim().mb_str());
        
        if (user.empty()) {
           throw GdaException("Please input Carto User Name.");
        }
        if (key.empty()) {
           throw GdaException("Please input Carto App Key.");
        }
        
        CPLSetConfigOption("CARTODB_API_KEY", key.c_str());
        OGRDataAdapter::GetInstance().AddEntry("cartodb_key", key.c_str());
        OGRDataAdapter::GetInstance().AddEntry("cartodb_user", user.c_str());
        CartoDBProxy::GetInstance().SetKey(key);
        CartoDBProxy::GetInstance().SetUserName(user);
        
        wxString url = "CartoDB:" + user;
        
        datasource = new WebServiceDataSource(GdaConst::ds_cartodb, url);
    }
    
    
	return datasource;
}

void ConnectDatasourceDlg::SaveRecentDataSource(IDataSource* ds,
                                                const wxString& layer_name)
{
    LOG_MSG("Entering ConnectDatasourceDlg::SaveRecentDataSource");
    try {
        RecentDatasource recent_ds;
        recent_ds.Add(ds, layer_name);
        recent_ds.Save();
    } catch( GdaException ex) {
        LOG_MSG(ex.what());
    }
    LOG_MSG("Exiting ConnectDatasourceDlg::SaveRecentDataSource");
}

////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////

const int RecentDatasource::N_MAX_ITEMS = 10;
const std::string RecentDatasource::KEY_NAME_IN_GDA_HISTORY = "recent_ds";

RecentDatasource::RecentDatasource()
{
    // get a latest input DB information
    std::vector<std::string> ds_infos = OGRDataAdapter::GetInstance().GetHistory(KEY_NAME_IN_GDA_HISTORY);
    
    if (ds_infos.size() > 0) {
        ds_json_str = ds_infos[0];
        Init(ds_json_str);
    }
}

RecentDatasource::~RecentDatasource()
{
    
}

void RecentDatasource::Init(wxString json_str_)
{
    if (json_str_.IsEmpty())
        return;
    
    // "recent_ds" : [{"ds_name":"/data/test.shp", "layer_name":"test", "ds_config":"..."}, ]
    std::string json_str(json_str_.mb_str());
    json_spirit::Value v;
    
    try {
        if (!json_spirit::read(json_str, v)) {
            throw std::runtime_error("Could not parse recent ds string");
        }
        
        const json_spirit::Array& ds_list = v.get_array();
       
        n_ds = ds_list.size();
        
        for (size_t i=0; i<n_ds; i++) {
            const json_spirit::Object& o = ds_list[i].get_obj();
            wxString ds_name, ds_conf, layer_name;
            
            for (json_spirit::Object::const_iterator i=o.begin(); i!=o.end(); ++i)
            {
                json_spirit::Value val;
                if (i->name_ == "ds_name") {
                    val = i->value_;
                    ds_name = val.get_str();
                }
                else if (i->name_ == "layer_name") {
                    val = i->value_;
                    layer_name = val.get_str();
                }
                else if (i->name_ == "ds_config") {
                    val = i->value_;
                    ds_conf = val.get_str();
                }
            }
            ds_names.push_back(ds_name);
            ds_layernames.push_back(layer_name);
            ds_confs.push_back(ds_conf);
        }
        
        
    } catch (std::runtime_error e) {
        wxString msg;
        msg << "Get Latest DB infor: JSON parsing failed: ";
        msg << e.what();
        throw GdaException(msg.mb_str());
    }
}

void RecentDatasource::Save()
{
    // update ds_json_str from ds_names & ds_values
    json_spirit::Array ds_list_obj;
   
    for (int i=0; i<n_ds; i++) {
        json_spirit::Object ds_obj;
        std::string ds_name( GET_ENCODED_FILENAME(ds_names[i]));
        std::string layer_name( GET_ENCODED_FILENAME(ds_layernames[i]));
        std::string ds_conf( ds_confs[i].mb_str() );
        ds_obj.push_back( json_spirit::Pair("ds_name", ds_name) );
        ds_obj.push_back( json_spirit::Pair("layer_name", layer_name) );
        ds_obj.push_back( json_spirit::Pair("ds_config", ds_conf) );
        ds_list_obj.push_back( ds_obj);
    }

    std::string json_str = json_spirit::write(ds_list_obj);
    ds_json_str = json_str;
    
    OGRDataAdapter::GetInstance().AddEntry(KEY_NAME_IN_GDA_HISTORY, json_str);
}

void RecentDatasource::Add(IDataSource* ds, const wxString& layer_name)
{
    wxString ds_name = ds->GetOGRConnectStr();
    wxString ds_conf = ds->GetJsonStr();
    
    // remove existed one
    std::vector<wxString>::iterator it;
    it = std::find(ds_names.begin(), ds_names.end(), ds_name);
    if (it != ds_names.end()) {
        ds_names.erase(it);
    }
    it = std::find(ds_confs.begin(), ds_confs.end(), ds_conf);
    if (it != ds_confs.end()) {
        ds_confs.erase(it);
    }
    it = std::find(ds_layernames.begin(), ds_layernames.end(), layer_name);
    if (it != ds_layernames.end()) {
        ds_layernames.erase(it);
    }
    
    n_ds = ds_names.size();
    
    if (n_ds < N_MAX_ITEMS) {
        ds_names.push_back(ds_name);
        ds_confs.push_back(ds_conf);
        ds_layernames.push_back(layer_name);
        
        n_ds = ds_names.size();
        
        
    } else {
        ds_names.erase(ds_names.begin());
        ds_confs.erase(ds_confs.begin());
        ds_layernames.erase(ds_layernames.begin());
        
        ds_names.push_back(ds_name);
        ds_confs.push_back(ds_conf);
        ds_layernames.push_back(layer_name);
    }
    
    Save();
}

void RecentDatasource::Clear()
{
    OGRDataAdapter::GetInstance().AddEntry(KEY_NAME_IN_GDA_HISTORY, "");
}

std::vector<wxString> RecentDatasource::GetList()
{
    return ds_names;
}

IDataSource* RecentDatasource::GetDatasource(wxString ds_name)
{
    for (int i=0; i<n_ds; i++) {
        if (ds_names[i] == ds_name) {
            wxString ds_conf = ds_confs[i];
            return IDataSource::CreateDataSource(ds_conf);
        }
    }
    return NULL;
}

wxString RecentDatasource::GetLayerName(wxString ds_name)
{
    for (int i=0; i<n_ds; i++) {
        if (ds_names[i] == ds_name) {
            wxString ds_layername = ds_layernames[i];
            return ds_layername;
        }
    }
    return wxEmptyString;
}
