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
    //wxString str;
    //str.Printf( wxT("%d files dropped"), (int)nFiles);
    
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

ConnectDatasourceDlg::ConnectDatasourceDlg(wxWindow* parent, const wxPoint& pos,
										   const wxSize& size)
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
    
    Bind(wxEVT_COMMAND_MENU_SELECTED, &ConnectDatasourceDlg::BrowseDataSource,
         this, DatasourceDlg::ID_DS_START, ID_DS_START + ds_names.Count());
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
        if (ds_file_path.GetExt().Lower() == "gda") {
            GdaFrame* gda_frame = GdaFrame::GetGdaFrame();
            if (gda_frame) {
                gda_frame->OpenProject(ds_file_path.GetFullPath());
                EndDialog(wxID_CANCEL);
            }
            return;
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
        
        if (layername.IsEmpty()) return;
        
		// At this point, there is a valid datasource and layername.
        if (layer_name.IsEmpty()) layer_name = layername;
        
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
                    wxString("Layer/Table name could not be empty. Please select"
                             " a layer/table.").mb_str());
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
            wxString msg = "The selected database driver is not supported "
            "on this platform. Please check GeoDa website "
            "for more information about database support "
            " and connection.";
            throw GdaException(msg.mb_str());
        }
        
        // save user inputs to history table
        if (!dbhost.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_host", dbhost.ToStdString());
        if (!dbname.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_name", dbname.ToStdString());
        if (!dbport.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_port", dbport.ToStdString());
        if (!dbuser.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_user", dbuser.ToStdString());
        
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
        
        datasource = new DBDataSource(dbname, dbhost, dbport, dbuser, dbpwd, ds_type);
        
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
        datasource = new WebServiceDataSource(ws_url, GdaConst::ds_wfs);
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
        
        datasource = new WebServiceDataSource(url, GdaConst::ds_cartodb);
    }
	
	return datasource;
}
