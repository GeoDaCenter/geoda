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

#include "../rc/GeoDaIcon-16x16.xpm"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/OGRTable.h"
#include "../DataViewer/DataSource.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "../GdaCartoDB.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../Project.h"
#include "ConnectDatasourceDlg.h"
#include "ExportDataDlg.h"

using namespace std;

BEGIN_EVENT_TABLE( ExportDataDlg, wxDialog )
    EVT_BUTTON( XRCID("IDC_OPEN_IASC"), ExportDataDlg::OnBrowseDSfileBtn )
    EVT_BUTTON( wxID_OK, ExportDataDlg::OnOkClick )
    EVT_BUTTON( XRCID("IDC_OPEN_CRS"), ExportDataDlg::OnOpenCRS )
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
is_save_centroids(false),
spatial_ref(NULL)
{
    
    if( project_p ) {
        project_file_name = project_p->GetProjectTitle();
        table_p = project_p->GetTableInt();
    }
    Init(parent, pos);
}

ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             vector<GdaShape*>& _geometries,
                             Shapefile::ShapeType _shape_type,
                             Project* _project,
                             bool isSelectedOnly,
                             const wxPoint& pos,
                             const wxSize& size)
: is_selected_only(isSelectedOnly), project_p(_project),
  geometries(_geometries), shape_type(_shape_type), is_saveas_op(false),
  is_geometry_only(true), table_p(NULL), is_table_only(false),
  is_save_centroids(false),spatial_ref(NULL)
{
    if( project_p) {
        project_file_name = project_p->GetProjectTitle();
        table_p = project_p->GetTableInt();
    }
    Init(parent, pos);
}

ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             Shapefile::ShapeType _shape_type,
                             std::vector<GdaShape*>& _geometries,
                             OGRSpatialReference* _spatial_ref,
                             OGRTable* table,
                             const wxPoint& pos, const wxSize& size)
: is_selected_only(false), project_p(NULL), geometries(_geometries),
  shape_type(_shape_type), is_saveas_op(true), is_geometry_only(false),
  table_p(table), is_table_only(false), is_save_centroids(false),
  spatial_ref(_spatial_ref)
{
    Init(parent, pos);
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
: is_selected_only(isSelectedOnly), project_p(_project), is_saveas_op(false),
  shape_type(_shape_type),is_geometry_only(true), table_p(NULL),
  is_table_only(false), is_save_centroids(true), spatial_ref(NULL)
{
    
    if( project_p) {
        project_file_name = project_p->GetProjectTitle();
        table_p = project_p->GetTableInt();
    }

    for(size_t i=0; i<_geometries.size(); i++) {
        geometries.push_back((GdaShape*)_geometries[i]);
    }
    Init(parent, pos);
}

// Export in-memory table (e.g. space-time table)
ExportDataDlg::ExportDataDlg(wxWindow* parent,
                             TableInterface* _table,
                             const wxPoint& pos,
                             const wxSize& size)
: is_selected_only(false), project_p(NULL), is_saveas_op(false),
  shape_type(Shapefile::NULL_SHAPE), is_geometry_only(false),
  is_table_only(true), table_p(_table), is_save_centroids(false),
  spatial_ref(NULL)
{
    Init(parent, pos);
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
    
    if( GeneralWxUtils::isWindows())
		ds_names.Remove("ESRI Personal Geodatabase (*.mdb)|*.mdb");
    
    Bind(wxEVT_COMMAND_MENU_SELECTED, &ExportDataDlg::BrowseExportDataSource,
         this, GdaConst::ID_CONNECT_POPUP_MENU,
         GdaConst::ID_CONNECT_POPUP_MENU + ds_names.Count());
    
    SetParent(parent);
	CreateControls();
	SetPosition(pos);
	Centre();
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
}

void ExportDataDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_EXPORT_OGRDATA");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
    m_database_table = XRCCTRL(*this, "IDC_CDS_DB_TABLE",AutoTextCtrl);
    m_chk_create_project = XRCCTRL(*this, "IDC_CREATE_PROJECT_FILE",wxCheckBox);
    if (!is_create_project) {
        m_chk_create_project->SetValue(false);
        m_chk_create_project->Hide();
    }
    m_crs_input = XRCCTRL(*this, "IDC_FIELD_CRS", wxTextCtrl);
    if (project_p == NULL || project_p->IsTableOnlyProject()) {
        if (project_p == NULL && spatial_ref) {
            // for case of creating grid, a spatial reference could be there
            // from existing map layer
            char* tmp = new char[1024];
            if (spatial_ref->exportToProj4(&tmp) == OGRERR_NONE) {
                wxString str_prj4 = tmp;
                m_crs_input->SetValue(str_prj4);
            }
            delete[] tmp;

        } else if (spatial_ref){
            // if table only ds, disable CRS controls
            m_crs_input->Disable();
            XRCCTRL(*this, "IDC_OPEN_CRS", wxBitmapButton)->Disable();
        }
    } else {
        OGRSpatialReference*  sr = project_p->GetSpatialReference();
        if (sr) {
            char* tmp = new char[1024];
            if (sr->exportToProj4(&tmp) == OGRERR_NONE) {
                wxString str_prj4 = tmp;
                m_crs_input->SetValue(str_prj4);
            }
            delete[] tmp;
        }
    }
    // Create the rest controls from parent
    DatasourceDlg::CreateControls();
}

void ExportDataDlg::OnOpenCRS( wxCommandEvent& event )
{
    ConnectDatasourceDlg connect_dlg(this, wxDefaultPosition, wxDefaultSize);
    connect_dlg.SetTitle("Load CRS from Data Source");
    if (connect_dlg.ShowModal() != wxID_OK) {
        return;
    }
    wxString proj_title = connect_dlg.GetProjectTitle();
    wxString layer_name = connect_dlg.GetLayerName();
    IDataSource* datasource = connect_dlg.GetDataSource();
    wxString datasource_name = datasource->GetOGRConnectStr();
    GdaConst::DataSourceType ds_type = datasource->GetType();

    OGRDatasourceProxy* proxy = NULL;
    try {
        proxy = OGRDataAdapter::GetInstance().GetDatasourceProxy(datasource_name, ds_type);
    } catch (GdaException& e) {
        proxy = NULL;
    }
    if (proxy == NULL) {
        wxMessageDialog dlg(this, _("GeoDa can not open the input data source. Please try another data source."),
                            _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    OGRLayerProxy* p_layer = proxy->GetLayerProxy(layer_name);
    if (p_layer == NULL || p_layer->CheckIsTableOnly()) {
        wxMessageDialog dlg(this, _("GeoDa can not get valid spatial reference from input data source. Please try another data source."),
                            _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    OGRSpatialReference* input_sr = p_layer->GetSpatialReference();
    if (input_sr) {
        char* tmp = new char[1024];
        if (input_sr->exportToProj4(&tmp) == OGRERR_NONE) {
            wxString str_prj4 = tmp;
            m_crs_input->SetValue(str_prj4);
        }
        delete[] tmp;
    }
}

void ExportDataDlg::BrowseExportDataSource ( wxCommandEvent& event )
{
    wxLogMessage("In ExportDataDlg::BrowseExportDataSource()");
    
    // for datasource file, we should support many file types:
    // SHP, DBF, CSV, GML, ...
    //bool table_only = m_chk_table_only->IsChecked();
    int index = event.GetId() - GdaConst::ID_CONNECT_POPUP_MENU;
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
    if (ds_file_path.GetExt().IsEmpty()) {
        wxString msg = wxString::Format(_("Can't get datasource type from: %s\n\nPlease select datasource supported by GeoDa or add extension  to file datasource."),  ds_file_path.GetFullPath());
        wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING);
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
   
    wxLogMessage("%s", _("ds:") + ds_name);
    if (ds_name.length() <= 0 ) {
        wxMessageDialog dlg(this, _("Please specify a valid data source name."),
                            _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    bool is_update = false;
    wxString tmp_ds_name;
    
	try{
        if ( project_p == NULL ) {
            //project does not exist, could be created a datasource from
            //geometries only, e.g. boundray file or in-memory geometries&table
            if (spatial_ref && table_p) {
                // https://github.com/GeoDaCenter/geoda/issues/1046
                // do nothing here
            }
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
                        col_x = table_p->InsertCol(GdaConst::double_type,
                                                   x_field_name);
                    if (col_y == wxNOT_FOUND)
                        col_y = table_p->InsertCol(GdaConst::double_type,
                                                   y_field_name);
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
        //
        bool is_table = IDataSource::IsTableOnly(ds_type);
        if( !CreateOGRLayer(ds_name, is_table, spatial_ref, is_update) ) {
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
                if (dlg.ShowModal() != wxID_OK)
                    return;
                wxFileName f(dlg.GetPath());
                f.SetExt("gda");
                proj_fname = f.GetFullPath();
                // copy a project_conf for exporting, will be deleted later
                project_conf = project_p->GetProjectConf()->Clone();
            }
            // save project file
            wxFileName new_proj_fname(proj_fname);
            wxString proj_title = new_proj_fname.GetName();
            if (project_conf) {
                LayerConfiguration* layer_conf = project_conf->GetLayerConfiguration();
                if (layer_conf) {
                    layer_conf->SetName(layer_name);
                    layer_conf->UpdateDataSource(datasource);
                }
                project_conf->Save(proj_fname);

                // in export case, delete cloned project_conf
                if ( proj_fname.empty() ) {
                    delete project_conf;
                    //delete datasource; Note: it is deleted in project_conf
                }
            }
        }
	} catch (GdaException& e) {
        if (e.type() == GdaException::NORMAL) return;
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
    wxMessageDialog dlg(this, "Saved Successfully.", _("Success"), wxOK);
    dlg.ShowModal();
    EndDialog(wxID_OK);
}

/**
 * Exporting (in-memory) geometries/table in project to another datasource name
 * This function will be called by OnOKClick (When user clicks OK)
 */
bool
ExportDataDlg::CreateOGRLayer(wxString& ds_name, bool is_table,
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
                if (pc->x == 0 && pc->y==0 &&
                    (pc->x < project_p->main_data.header.bbox_x_min ||
                     pc->x > project_p->main_data.header.bbox_x_max) &&
                    (pc->y < project_p->main_data.header.bbox_y_min ||
                     pc->y > project_p->main_data.header.bbox_y_max))
                {
                    geometries.push_back(new GdaPoint());
                } else
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
        } //shape_type = project_p->GetGdaGeometries(geometries);
    } else {
        // create datasource from geometries only
        size_t nn = geometries.size();
        
        // create datasource from table only (no geometry)
        if (nn == 0 && table_p)
            nn = table_p->GetNumberRows();
        
        for(size_t i=0; i < nn; i++)
            selected_rows.push_back(i);
    }
    
	// convert to OGR geometries, reproject if needed
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
	vector<OGRGeometry*> ogr_geometries;
    OGRwkbGeometryType geom_type = wkbNone;
    OGRSpatialReference new_ref;
    if (is_table) {
        spatial_ref = NULL; // table only data, void creating e.g. prj file
    } else {
        geom_type = ogr_adapter.MakeOGRGeometries(geometries, shape_type,
                                                  ogr_geometries, selected_rows);
        wxString str_crs = m_crs_input->GetValue();
        bool valid_input_crs = false;
        if (!str_crs.IsEmpty()) {
            if (new_ref.importFromProj4(str_crs.c_str()) == OGRERR_NONE) {
                valid_input_crs = true;
            }
        }
        if (ds_name.EndsWith(".json") || ds_name.EndsWith(".geojson")) {
            // for GeoJSON, force transform to EPSG4326 automatically
            new_ref.importFromEPSG(4326);
            valid_input_crs = true;
        }
        if (ogr_geometries.size() > 0 && valid_input_crs) {
            if (spatial_ref && spatial_ref->IsSame(&new_ref) == false) {
                // transform geometries from old CRS to new CRS
                OGRCoordinateTransformation *poCT;
                poCT = OGRCreateCoordinateTransformation(spatial_ref, &new_ref);
                for (size_t i=0; i < ogr_geometries.size(); i++) {
                    ogr_geometries[i]->transform(poCT);
                }
                OGRCoordinateTransformation::DestroyCT(poCT);
            }
            spatial_ref = &new_ref; // use new CRS
        }
    }
    wxString cpg_encode;
    if (project_p) cpg_encode = project_p->GetCpgEncode();
    if (cpg_encode.IsEmpty() && table_p) {
        cpg_encode = table_p->GetEncodingName();
    }
	// take care of empty layer name
    if (layer_name.empty()) {
        layer_name = table_p ? table_p->GetTableName() : "NO_NAME";
    }
    int prog_n_max = selected_rows.size();
    if (prog_n_max == 0 && table_p) prog_n_max = table_p->GetNumberRows();
    OGRLayerProxy* new_layer;
    new_layer = ogr_adapter.ExportDataSource(ds_format, ds_name, layer_name,
                                             geom_type, ogr_geometries,
                                             table_p, selected_rows,
                                             spatial_ref, is_update, cpg_encode);
    if (new_layer == NULL) return false;
#ifdef __WXOSX__
    wxProgressDialog prog_dlg(_("Save data source progress dialog"),
                              _("Saving data..."),
                              prog_n_max, this,
                              wxPD_CAN_ABORT|wxPD_AUTO_HIDE|wxPD_APP_MODAL);
#endif
    bool cont = true;
    while (new_layer->export_progress < prog_n_max) {
        wxMilliSleep(100);
        if ( new_layer->stop_exporting == true )
            return false;
        // update progress bar
#ifdef __WXOSX__
        cont = prog_dlg.Update(new_layer->export_progress);
        if (!cont ) {
            new_layer->stop_exporting = true;
            OGRDataAdapter::GetInstance().CancelExport(new_layer);
            return false;
        }
#endif
        if (new_layer->export_progress == -1) {
            wxString tmp = _("Saving to data source (%s) failed.\n\nDetails: %s");
            wxString msg = wxString::Format(tmp, ds_name,
                                            new_layer->error_message);
            throw GdaException(msg.c_str());
        }
    }
    
    OGRDataAdapter::GetInstance().StopExport(); //here new_layer will be deleted

    if (!is_geometry_only && table_p != NULL) {
        for (size_t i=0; i < geometries.size(); i++) {
			delete geometries[i];
        }
    }
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
            OGRDataAdapter::GetInstance().AddHistory("db_host",  dbhost);
        if (!dbname.IsEmpty())
            OGRDataAdapter::GetInstance().AddHistory("db_name", dbname);
        if (!dbport.IsEmpty())
            OGRDataAdapter::GetInstance().AddHistory("db_port", dbport);
        if (!dbuser.IsEmpty())
            OGRDataAdapter::GetInstance().AddHistory("db_user", dbuser);
        if (!layer_name.IsEmpty())
            OGRDataAdapter::GetInstance().AddHistory("tbl_name", layer_name);
        
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
        if (dbhost.IsEmpty()) error_msg = _("Please input database host.");
        else if (dbname.IsEmpty()) error_msg= _("Please input database name.");
        else if (dbport.IsEmpty()) error_msg= _("Please input database port.");
        else if (dbuser.IsEmpty()) error_msg= _("Please input user name.");
        else if (layer_name.IsEmpty()) error_msg= _("Please input table name.");
        
        if (!error_msg.IsEmpty()) throw GdaException(error_msg.mb_str());
        
        ds_format = IDataSource::GetDataTypeNameByGdaDSType(ds_type);
        return new DBDataSource(ds_type, dbname,dbhost,dbport,dbuser,dbpwd);

    }
    return NULL;
}


bool ExportDataDlg::IsTableOnly()
{
	//return m_chk_table_only->IsChecked();
	return false;
}
