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



#include <fstream>
#include <wx/wx.h>
#include <wx/checkbox.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/progdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/frame.h>

#include <cpl_error.h>
#include <ogrsf_frmts.h>

#include "ogr_srs_api.h"

#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/DbfTable.h"
#include "../DataViewer/DataSource.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "../GdaCartoDB.h"
#include "../rc/GeoDaIcon-16x16.xpm"
#include "ExportDataDlg.h"

using namespace std;

BEGIN_EVENT_TABLE( ExportDataDlg, wxFrame )
    EVT_BUTTON( XRCID("IDC_OPEN_IASC"), ExportDataDlg::OnBrowseDSfileBtn )
    EVT_BUTTON( wxID_OK, ExportDataDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, ExportDataDlg::OnCancelClick )
END_EVENT_TABLE()

ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             Project* _project,
                             bool isSelectedOnly,
                             wxString projectFileName,
                             const wxPoint& pos,
                             const wxSize& size )
: is_selected_only(isSelectedOnly),
project_p(_project),
project_file_name(projectFileName),
is_saveas_op(true),
is_geometry_only(false),
is_table_only(false),
is_save_centroids(false)
{
    
    if( project_p ) {
        project_file_name = project_p->GetProjectTitle();
        table_p = project_p->GetTableInt();
    }
    Init(parent, pos);
    type = 2;
}

ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             vector<GdaShape*>& _geometries,
                             Shapefile::ShapeType _shape_type,
                             Project* _project,
                             bool isSelectedOnly,
                             const wxPoint& pos,
                             const wxSize& size)
: is_selected_only(isSelectedOnly), project_p(_project), geometries(_geometries), shape_type(_shape_type), is_saveas_op(false), is_geometry_only(true), table_p(NULL), is_table_only(false), is_save_centroids(false)
{
    if( project_p) {
        project_file_name = project_p->GetProjectTitle();
        table_p = project_p->GetTableInt();
    }
    Init(parent, pos);
    type = 2;
}

// Export POINT data only, e.g. centroids/mean centers
ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             vector<GdaPoint*>& _geometries,
                             Shapefile::ShapeType _shape_type,
                             wxString _point_name,
                             Project* _project,
                             bool isSelectedOnly,
                             const wxPoint& pos,
                             const wxSize& size)
: is_selected_only(isSelectedOnly), project_p(_project), is_saveas_op(false), shape_type(_shape_type),is_geometry_only(true), table_p(NULL), is_table_only(false), is_save_centroids(true)
{
    
    if( project_p) {
        project_file_name = project_p->GetProjectTitle();
        table_p = project_p->GetTableInt();
    }

    for(size_t i=0; i<_geometries.size(); i++) {
        geometries.push_back((GdaShape*)_geometries[i]);
    }
    Init(parent, pos);
    type = 2;
}

// Export in-memory table (e.g. space-time table)
ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             TableInterface* _table,
                             const wxPoint& pos,
                             const wxSize& size)
: is_selected_only(false), project_p(NULL), is_saveas_op(false), shape_type(Shapefile::NULL_SHAPE), is_geometry_only(false), is_table_only(true), table_p(_table), is_save_centroids(false)
{
    Init(parent, pos);
    type = 2;
}


void ExportDataDlg::Init(wxWindow* parent, const wxPoint& pos)
{
    wxLogMessage("Open ExportDataDlg in its Init()");
    DatasourceDlg::Init();
    
    is_create_project = project_file_name.empty() ? false : true;
    ds_file_path = wxFileName("");
    
    if (is_table_only)
        ds_names.Remove("ESRI Shapefile (*.shp)|*.shp");
    //ds_names.Remove("dBase database file (*.dbf)|*.dbf");
    ds_names.Remove("MS Excel (*.xls)|*.xls");
	//ds_names.Remove("MS Office Open XML Spreadsheet (*.xlsx)|*.xlsx");
    //ds_names.Remove("U.S. Census TIGER/Line (*.tiger)|*.tiger");
    //ds_names.Remove("Idrisi Vector (*.vct)|*.vct");
    if( GeneralWxUtils::isWindows()){
		ds_names.Remove("ESRI Personal Geodatabase (*.mdb)|*.mdb");
	}
    
    Bind( wxEVT_COMMAND_MENU_SELECTED, 
         &ExportDataDlg::BrowseExportDataSource,
         this, DatasourceDlg::ID_DS_START, 
         ID_DS_START + ds_names.Count());
    SetParent(parent);
	CreateControls();
	SetPosition(pos);
	Centre();
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
}

void ExportDataDlg::CreateControls()
{
    wxXmlResource::Get()->LoadFrame(this, GetParent(), "IDD_EXPORT_OGRDATA");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
    m_database_table = XRCCTRL(*this, "IDC_CDS_DB_TABLE",AutoTextCtrl);
    m_chk_create_project = XRCCTRL(*this, "IDC_CREATE_PROJECT_FILE",wxCheckBox);
    if (!is_create_project) {
        m_chk_create_project->SetValue(false);
        m_chk_create_project->Hide();
    }
    // Create the rest controls from parent
    DatasourceDlg::CreateControls();
    
    m_cartodb_table->Show();
    m_cartodb_tablename->Show();
}


void ExportDataDlg::BrowseExportDataSource ( wxCommandEvent& event )
{
    wxLogMessage("In ExportDataDlg::BrowseExportDataSource()");
    
    // for datasource file, we should support many file types:
    // SHP, DBF, CSV, GML, ...
    //bool table_only = m_chk_table_only->IsChecked();
    int index = event.GetId() - ID_DS_START;
    wxString name = ds_names[index];
    wxString wildcard;
    wildcard << name ;
    wxString filegdb_ext = "gdb";
    wxString tmp;
    if (name.Contains(filegdb_ext)) {
        // directory data source, such as ESRI .gdb directory
        //do {
        wxString msg = _("Select an existing *.gdb directory, or create an New Folder named *.gdb");
        wxDirDialog dlg(this, msg,"",
                        wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) 
            return;
        ds_file_path = dlg.GetPath();
        if (ds_file_path.GetExt()!=filegdb_ext)
            ds_file_path.SetExt(filegdb_ext);
        //} while (!ds_file_path.EndsWith(".gdb"))
    } else {
        wxFileDialog dlg(this, _("Export or save layer to"), "", "",
                         wildcard, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) 
            return;
        ds_file_path = dlg.GetPath();
    	if (ds_file_path.GetExt().IsEmpty()) {
            wxString ext_str = wildcard.AfterLast('.');
            ds_file_path.SetExt(ext_str);
        }
        tmp = dlg.GetName();
    }
    // construct the export datasource file name
    wxString ext_str;
    if (ds_file_path.GetExt().IsEmpty()){
        wxString msg = wxString::Format(_("Can't get datasource type from: %s\n\nPlease select datasource supported by GeoDa or add extension  to file datasource."),  ds_file_path.GetFullPath());
        wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    } else { 
        m_ds_filepath_txt->SetValue(ds_file_path.GetFullPath()); 
    }
    m_ds_filepath_txt->SetEditable(true);
	FindWindow(XRCID("wxID_OK"))->Enable(true);
}

/**
 * When user choose a data source, validate it first, 
 * then create a Project() that will be used by the 
 * main program.
 */
void ExportDataDlg::OnOkClick( wxCommandEvent& event )
{
    wxLogMessage("In ExportDataDlg::OnOkClick()");
    
    int datasource_type = m_ds_notebook->GetSelection();
    IDataSource* datasource = GetDatasource();
    wxString ds_name = datasource->GetOGRConnectStr();
    datasource_name = ds_name;
	GdaConst::DataSourceType ds_type = datasource->GetType();
   
    wxLogMessage(_("ds:") + ds_name);
    if (ds_name.length() <= 0 ) {
        wxMessageDialog dlg(this, _("Please specify a valid data source name."),
                            _("Warning"),
                            wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    bool is_update = false;
    wxString tmp_ds_name;
    
	try{
        OGRSpatialReference* spatial_ref = NULL;
        
        if ( project_p == NULL ) {
            //project does not exist, could be created a datasource from
            //geometries only, e.g. boundray file
        } else {
            //case: save current open datasource as a new datasource
            
            spatial_ref = project_p->GetSpatialReference();
            // warning if saveas not compaptible
            GdaConst::DataSourceType o_ds_type = project_p->GetDatasourceType();
            bool o_ds_table_only = IDataSource::IsTableOnly(o_ds_type);
            bool n_ds_table_only = IDataSource::IsTableOnly(ds_type);
           
            if (is_save_centroids) {
                if (n_ds_table_only == false) {
                    // make sure geometries are saved as well if needed for
                    // non-table-only datasource
                    shape_type = Shapefile::POINT_TYP;
                }
                // Add points to Table
                if (table_p) {
                    wxString x_field_name = "COORD_X";
                    wxString y_field_name = "COORD_Y";
                    x_field_name.UpperCase();
                    y_field_name.UpperCase();
                    
                    int col_x = table_p->FindColId(x_field_name);
                    int col_y = table_p->FindColId(y_field_name);
                    
                    if (col_x == wxNOT_FOUND)
                        col_x = table_p->InsertCol(GdaConst::double_type, x_field_name);
                    
                    if (col_y == wxNOT_FOUND)
                        col_y = table_p->InsertCol(GdaConst::double_type, y_field_name);
                    
                    vector<double> x_data;
                    vector<double> y_data;
                    for(size_t i=0; i<geometries.size(); i++) {
                        x_data.push_back(((GdaPoint*)(geometries[i]))->GetX());
                        y_data.push_back(((GdaPoint*)(geometries[i]))->GetY());
                    }
                    
                    table_p->SetColData(col_x, 0, x_data);
                    table_p->SetColData(col_y, 0, y_data);
                    
                }
            }
            
            if (o_ds_table_only && !n_ds_table_only) {
                if (project_p && project_p->main_data.records.size() ==0) {
                    if (ds_type == GdaConst::ds_geo_json ||
                        ds_type == GdaConst::ds_kml ||
                        ds_type == GdaConst::ds_shapefile) {
                        // can't save a table-only ds to non-table-only ds,
                        // if there is no new geometries to be saved.
                        wxString msg = _("GeoDa can't save a Table-only data source as a Geometry enabled data source. Please try to add a geometry layer and then use File->Save As.");
                        throw GdaException(msg.mb_str());
                    }
                }
            } else if ( !o_ds_table_only && n_ds_table_only) {
                if (is_save_centroids == false && shape_type == Shapefile::NULL_SHAPE) {
                    // possible loss geom data save a non-table ds to table-only ds
                    wxString msg = _("The geometries will not be saved when exporting to a Table-only data source.\n\nDo you want to continue?");
                    wxMessageDialog dlg(this, msg, _("Warning: loss data"),
                                        wxYES_NO | wxICON_WARNING);
                    if (dlg.ShowModal() != wxID_YES)
                        return;
                }
            }
		}

		// by default the datasource will be re-created, except for some special
        // cases: e.g. sqlite, ESRI FileGDB
		if (datasource_type == 0) {
			if (wxFileExists(ds_name)) {
				if (ds_name.EndsWith(".sqlite") || ds_name.EndsWith(".gpkg")) {
					// add new layer to existing sqlite
					is_update = true;
				} else {
					wxRemoveFile(ds_name);
				}
			} else if (wxDirExists(ds_name)) {
				// only for adding new layer to ESRI File Geodatabase
				is_update = true;
				wxDir dir(ds_name);
				wxString first_filename;
				if (!dir.GetFirst(&first_filename)) {
					// for an empty .gdb directory, create a new FileGDB datasource
					is_update = false;
					wxRmDir(ds_name);
				}
			}
		}


        
        if( !CreateOGRLayer(ds_name, spatial_ref, is_update) ) {
            wxString msg = _("Save As has been cancelled.");
            throw GdaException(msg.mb_str(), GdaException::NORMAL);
        }
        // save project file
        if (m_chk_create_project->IsChecked() ) {
            //wxString proj_fname = project_file_name;
            wxString proj_fname = wxEmptyString;
            ProjectConfiguration* project_conf = NULL;
            
            if ( m_chk_create_project->IsChecked() ){
                // Export case: create a project file
                // Export means exporting current datasource to a new one, and
                // create a new project file that is based on this datasource.
                // E.g. export a shape file to PostgreGIS layer, then the new
                // project file should has <datasource> content of database
                // configuration
                wxString file_dlg_title = _("GeoDa Project to Save As");
                wxString file_dlg_type =  "GeoDa Project (*.gda)|*.gda";
                wxFileDialog dlg(this, file_dlg_title, wxEmptyString,
                                 wxEmptyString, file_dlg_type,
                                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
                if (dlg.ShowModal() != wxID_OK) return;
                wxFileName f(dlg.GetPath());
                f.SetExt("gda");
                proj_fname = f.GetFullPath();
                // copy a project_conf for exporting, will be deleted later
                project_conf = project_p->GetProjectConf()->Clone();
            }
            // save project file
            wxFileName new_proj_fname(proj_fname);
            wxString proj_title = new_proj_fname.GetName();
            LayerConfiguration* layer_conf
                = project_conf->GetLayerConfiguration();
            layer_conf->SetName(layer_name);
            layer_conf->UpdateDataSource(datasource);
            project_conf->Save(proj_fname);
          
            // in export case, delete cloned project_conf
            if ( proj_fname.empty() ) {
                delete project_conf;
                //delete datasource; Note: it is deleted in project_conf
            }
        }
	} catch (GdaException& e) {
        if (e.type() == GdaException::NORMAL)
            return;
        
        // special clean up for file datasource
        if ( !tmp_ds_name.empty() ) {
            if ( wxFileExists(tmp_ds_name) &&
                !tmp_ds_name.EndsWith(".sqlite") &&
                !tmp_ds_name.EndsWith(".gpkg") )
            {
                wxRemoveFile(ds_name);
                wxCopyFile(tmp_ds_name, ds_name);
                wxRemoveFile(tmp_ds_name);
            }
        }
		wxMessageDialog dlg(this, e.what() , _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	//wxString msg = "Export successfully.";
    //msg << "\n\nTips: if you want to use exported project/datasource, please"
    //    << " close current project and then open exported project/datasource.";
	//wxMessageDialog dlg(this, msg , "Info", wxOK | wxICON_INFORMATION);
    //dlg.ShowModal();
    
    is_ok_clicked = true;
    EndDialog();
}

/**
 * Exporting OGR layer in project to another datasource name
 * This function will be called by OnOKClick (When user clicks OK)
 */
void ExportDataDlg::ExportOGRLayer(wxString& ds_name, bool is_update)
{
    // Exporting layer in multi-layer cases will not use OGRTable anymore,
    // we will use OGRDataAdapter to get ogr layer by layer name
    OGRTable* tbl = dynamic_cast<OGRTable*>(project_p->GetTableInt());
	if (!tbl) {
		// DBFTable case, try to read into
        wxString msg = _("Only OGR datasource can be saved as.");
		throw GdaException(msg.mb_str());
	}

    OGRLayerProxy* layer = tbl->GetOGRLayer();
    
    layer->T_Export(ds_format.ToStdString(), ds_name.ToStdString(),
                    layer_name.ToStdString(), is_update);
    int prog_n_max = project_p->GetNumRecords();
    wxProgressDialog prog_dlg(_("Save As progress dialog"),
                              _("Saving data..."),
                              prog_n_max, // range
                              this,
                              wxPD_CAN_ABORT|wxPD_AUTO_HIDE|wxPD_APP_MODAL);
    bool cont = true;
    while (layer->export_progress < prog_n_max) {
        wxSleep(1);
        cont = prog_dlg.Update(layer->export_progress);
        
        if (!cont ) {
            layer->T_StopExport();
            return;
        }
        if (layer->export_progress == -1){
            wxString msg = wxString::Format(_("Saving to data source (%s) failed.\n\nDetails: %s"), ds_name, layer->error_message.str());
            throw GdaException(msg.c_str());
        }
    }
}

/**
 * Exporting (in-memory) geometries/table in project to another datasource name
 * This function will be called by OnOKClick (When user clicks OK)
 */
bool
ExportDataDlg::CreateOGRLayer(wxString& ds_name,
							  OGRSpatialReference* spatial_ref,
                              bool is_update)
{
    // The reason that we don't use Project::Main directly is for creating
    // datasource from centroids/centers directly, we have to use
    // vector<GdaShape*>. Therefore, we use it as a uniform interface.
    // for shp/dbf reading, we need to convert Main data to GdaShape first
    // this will spend some time, but keep the rest of code clean.
    // Note: potential speed/memory performance issue
    
    vector<int> selected_rows;
    
    if ( project_p != NULL && geometries.empty() && !is_save_centroids ) {
        shape_type = Shapefile::NULL_SHAPE;
        int num_obs = project_p->main_data.records.size();
        if (num_obs == 0) num_obs = project_p->GetNumRecords();
        if (num_obs == 0) {
            ostringstream msg;
            msg << _("Saving failed: GeoDa can't save as empty datasource.");
            throw GdaException(msg.str().c_str());
        }
        
        if (is_selected_only) { 
			project_p->GetSelectedRows(selected_rows);
		} else {
			for( int i=0; i<num_obs; i++) selected_rows.push_back(i);
		}

        if (project_p->main_data.header.shape_type == Shapefile::POINT_TYP) {
            PointContents* pc;
            for (int i=0; i<num_obs; i++) {
                pc = (PointContents*)project_p->main_data.records[i].contents_p;
                geometries.push_back(new GdaPoint(wxRealPoint(pc->x, pc->y)));
            }
            shape_type = Shapefile::POINT_TYP;
        }
        else if (project_p->main_data.header.shape_type == Shapefile::POLYGON) {
            PolygonContents* pc;
            for (int i=0; i<num_obs; i++) {
                pc = (PolygonContents*)project_p->main_data.records[i].contents_p;
                geometries.push_back(new GdaPolygon(pc));
            }
			shape_type = Shapefile::POLYGON;
        }
    } else {
        // create datasource from geometries only
        size_t nn = geometries.size();
        
        // create datasource from table only (no geometry)
        if (nn == 0 && table_p)
            nn = table_p->GetNumberRows();
        
        for(size_t i=0; i < nn; i++)
            selected_rows.push_back(i);
    }

    /*
    // explictly set SRS with EPSG information
    wxString cstype = "PROJCS";
    if (poOutputSRS->IsGeographic() == 1) {
        cstype = "GEOGCS";
    }
    
    wxString authname= poOutputSRS->GetAuthorityName(cstype.mb_str());
    wxString authCode = poOutputSRS->GetAuthorityCode(cstype.mb_str());
    
    if (authname.IsEmpty()) {
        int epsg = poOutputSRS->GetEPSGGeogCS();
        poOutputSRS->SetAuthority(cstype.mb_str(), "EPSG", epsg);
    }
     */
    
	// convert to OGR geometries
	vector<OGRGeometry*> ogr_geometries;
	OGRwkbGeometryType geom_type =  OGRDataAdapter::GetInstance().MakeOGRGeometries(geometries, shape_type, ogr_geometries, selected_rows);

    // NOTE: for GeoJSON, transform to WGS84 automatically
    if (spatial_ref && (ds_name.EndsWith(".json") || ds_name.EndsWith(".geojson"))) {
        int epsg = spatial_ref->GetEPSGGeogCS();
        if (epsg != 4326) {
            OGRSpatialReference wgs84_ref;
            wgs84_ref.importFromEPSG(4326);
            OGRCoordinateTransformation *poCT = OGRCreateCoordinateTransformation( spatial_ref, &wgs84_ref );
            for (size_t i=0; i < ogr_geometries.size(); i++) {
                ogr_geometries[i]->transform(poCT);
            }
        }
    }
    
	// take care of empty layer name
    if (layer_name.empty()) {
        layer_name = table_p ? table_p->GetTableName() : "NO_NAME";
    }
    
    int prog_n_max = selected_rows.size();
    
    if (prog_n_max == 0 && table_p)
        prog_n_max = table_p->GetNumberRows();
   
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
    OGRLayerProxy* new_layer =
    ogr_adapter.ExportDataSource(ds_format.ToStdString(),
                                 ds_name,
                                 layer_name.ToStdString(),
                                 geom_type,
                                 ogr_geometries,
                                 table_p,
                                 selected_rows,
                                 spatial_ref,
                                 is_update);
    if (new_layer == NULL)
        return false;
    
    wxProgressDialog prog_dlg(_("Save data source progress dialog"),
                              _("Saving data..."),
                              prog_n_max, this,
                              wxPD_CAN_ABORT|wxPD_AUTO_HIDE|wxPD_APP_MODAL);
    bool cont = true;
    while (new_layer->export_progress < prog_n_max) {
        wxMilliSleep(100);
        if ( new_layer->stop_exporting == true )
            return false;
        // update progress bar
        cont = prog_dlg.Update(new_layer->export_progress);
        if (!cont ) {
            new_layer->stop_exporting = true;
            OGRDataAdapter::GetInstance().CancelExport(new_layer);
            return false;
        }
        if (new_layer->export_progress == -1){
            wxString msg = wxString::Format(_("Saving to data source (%s) failed.\n\nDetails: %s"), ds_name, new_layer->error_message.str());
            throw GdaException(msg.c_str());
        }
    }
    
    OGRDataAdapter::GetInstance().StopExport(); //here new_layer will be deleted

	if (!is_geometry_only)
		for (size_t i=0; i < geometries.size(); i++) 
			delete geometries[i];

	//NOTE: export_ds will take ownership of ogr_geometries 
	//for (size_t i=0; i<ogr_geometries.size(); i++) {
	//	OGRGeometryFactory::destroyGeometry( ogr_geometries[i]);
	//}
    return true;
}

/**
 * Get data source connection string in OGR style from this dialog
 */
IDataSource* ExportDataDlg::GetDatasource()
{
    wxString error_msg;
	int datasource_type = m_ds_notebook->GetSelection();
    
	if (0 == datasource_type)
	{
		// file datasource tab is selected
		wxString ds_name = m_ds_filepath_txt->GetValue().Trim();
        ds_file_path = ds_name;
        wxString ext = ds_file_path.GetExt();
        if ( ext.CmpNoCase("DBF") == 0 ) {
            ds_file_path.SetExt("SHP");
            ext = ds_file_path.GetExt();
        }
        ds_format = IDataSource::GetDataTypeNameByExt(ext);
        return new FileDataSource(ds_name);
			
    } else if (1 == datasource_type) {
		// database sources tab is selected
        wxString cur_sel;
        wxString dbname, dbhost, dbport, dbuser, dbpwd;
        cur_sel = m_database_type->GetStringSelection();
        dbname = m_database_name->GetValue().Trim();
        dbhost = m_database_host->GetValue().Trim();
        dbport = m_database_port->GetValue().Trim();
        dbuser = m_database_uname->GetValue().Trim();
        dbpwd  = m_database_upwd->GetValue().Trim();
        layer_name = m_database_table->GetValue().Trim();
        
        // save user inputs to history table
        if (!dbhost.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_host",  dbhost.ToStdString());
        if (!dbname.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_name", dbname.ToStdString());
        if (!dbport.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_port", dbport.ToStdString());
        if (!dbuser.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("db_user", dbuser.ToStdString());
        if (!layer_name.IsEmpty())
            OGRDataAdapter::GetInstance()
            .AddHistory("tbl_name", layer_name.ToStdString());
        
        
        GdaConst::DataSourceType ds_type = GdaConst::ds_unknown;
        if (cur_sel == DBTYPE_ORACLE) ds_type = GdaConst::ds_oci;
        else if (cur_sel == DBTYPE_ARCSDE) ds_type = GdaConst::ds_esri_arc_sde;
        else if (cur_sel == DBTYPE_POSTGIS) ds_type = GdaConst::ds_postgresql;
        else if (cur_sel == DBTYPE_MYSQL) ds_type = GdaConst::ds_mysql;
        //else if (cur_sel == 4) ds_type = GdaConst::ds_ms_sql;
        else {
            wxString msg = _("The selected database driver is not supported on this platform. Please check GeoDa website for more information about database support and connection.");
            throw GdaException(msg.mb_str());
        }
        
        // check if empty, prompt user to input
        if (dbhost.IsEmpty()) error_msg = "Please input database host.";
        else if (dbname.IsEmpty()) error_msg= "Please input database name.";
        else if (dbport.IsEmpty()) error_msg= "Please input database port.";
        else if (dbuser.IsEmpty()) error_msg= "Please input user name.";
        else if (layer_name.IsEmpty()) error_msg= "Please input table name.";
        
        if (!error_msg.IsEmpty()) throw GdaException(error_msg.mb_str());
        
        ds_format = IDataSource::GetDataTypeNameByGdaDSType(ds_type);
        return new DBDataSource(ds_type, dbname,dbhost,dbport,dbuser,dbpwd);
        //ds_name = ds.GetOGRConnectStr();
        
    } else {
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
        
        ds_format = IDataSource::GetDataTypeNameByGdaDSType(GdaConst::ds_cartodb);
        
        return new WebServiceDataSource(GdaConst::ds_cartodb, url);
    }
    return NULL;
}


bool ExportDataDlg::IsTableOnly()
{
	//return m_chk_table_only->IsChecked();
	return false;
}
