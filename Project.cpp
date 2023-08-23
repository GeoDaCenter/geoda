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

#include <list>
#include <set>
#include <sstream>
#include <vector>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/dir.h>
#include <wx/textfile.h>
#include <boost/foreach.hpp>


#include "ogr_srs_api.h"
#include "logger.h"
#include "FramesManager.h"
#include "SaveButtonManager.h"
#include "GdaException.h"
#include "DefaultVarsPtree.h"
#include "DataViewer/CustomClassifPtree.h"
#include "DataViewer/OGRTable.h"
#include "DataViewer/TableBase.h"
#include "DataViewer/TableFrame.h"
#include "DataViewer/TableInterface.h"
#include "DataViewer/TableState.h"
#include "DataViewer/TimeState.h"
#include "DataViewer/VarOrderPtree.h"
#include "DialogTools/SaveToTableDlg.h"
#include "DialogTools/ExportDataDlg.h"
#include "Explore/CatClassification.h"
#include "Explore/CatClassifManager.h"
#include "Explore/CovSpHLStateProxy.h"
#include "Explore/MapNewView.h"
#include "GdaShape.h"
#include "GenGeomAlgs.h"
#include "SpatialIndAlgs.h"
#include "PointSetAlgs.h"
#include "ShapeOperations/GalWeight.h"
#include "ShapeOperations/VoronoiUtils.h"
#include "VarCalc/WeightsManInterface.h"
#include "ShapeOperations/WeightsManState.h"
#include "ShapeOperations/WeightsManager.h"
#include "ShapeOperations/WeightsManPtree.h"
#include "ShapeOperations/OGRDataAdapter.h"
#include "GeneralWxUtils.h"
#include "MapLayerStateObserver.h"
#include "Project.h"

// used by TemplateCanvas
std::map<wxString, i_array_type*> Project::shared_category_scratch;

/** Constructor for an existing project file */
Project::Project(const wxString& proj_fname)
: is_project_valid(false),
table_int(0), table_state(0), time_state(0),
w_man_int(0), w_man_state(0), maplayer_state(0),
save_manager(0),
frames_manager(0),cat_classif_manager(0), mean_centers(0), centroids(0),
voronoi_rook_nbr_gal(0), default_var_name(4), default_var_time(4),
point_duplicates_initialized(false), point_dups_warn_prev_displayed(false),
num_records(0), layer_proxy(NULL),
highlight_state(0), con_map_hl_state(0), pairs_hl_state(0),
dist_metric(WeightsMetaInfo::DM_euclidean),
dist_units(WeightsMetaInfo::DU_mile),
min_1nn_dist_euc(-1), max_1nn_dist_euc(-1), max_dist_euc(-1),
min_1nn_dist_arc(-1), max_1nn_dist_arc(-1), max_dist_arc(-1),
sourceSR(NULL), rtree_bbox_ready(false), has_null_geometry(false)
{
    
	wxLogMessage("Entering Project::Project (existing project)");
	//wxLogMessage("%s", proj_fname);

	SetProjectFullPath(proj_fname);
	bool wd_success = SetWorkingDir(proj_fname);
	if (!wd_success) {
		//LOG_MSG("Warning: could not set Working Dir from " + proj_fname);
		// attempt to set working dir according to standard location
		wd_success = SetWorkingDir(wxGetHomeDir());
		if (!wd_success) {
			//LOG_MSG("Warning: could not set Working Dir to wxGetHomeDir()");
		}
	}
	project_conf = new ProjectConfiguration(proj_fname);
	LayerConfiguration* layer_conf = project_conf->GetLayerConfiguration();
	layername = layer_conf->GetName();
	datasource = layer_conf->GetDataSource();
    
	is_project_valid = CommonProjectInit();
	if (is_project_valid)
        save_manager->SetAllowEnableSave(true);
    
	if (is_project_valid) {
		// correct cat classifications in weights manager from Table
		GetCatClassifManager()->VerifyAgainstTable();
	}
	
	wxLogMessage("Exiting Project::Project");
}

/** Constructor for a newly connected datasource */
Project::Project(const wxString& proj_title,
                 const wxString& layername_s,
                 IDataSource* p_datasource)
: is_project_valid(false),
table_int(0), table_state(0), time_state(0),
w_man_int(0), w_man_state(0), maplayer_state(0),
save_manager(0),
frames_manager(0),cat_classif_manager(0), mean_centers(0), centroids(0),
voronoi_rook_nbr_gal(0), default_var_name(4), default_var_time(4),
point_duplicates_initialized(false), point_dups_warn_prev_displayed(false),
num_records(0), layer_proxy(NULL),
highlight_state(0), con_map_hl_state(0), pairs_hl_state(0),
dist_metric(WeightsMetaInfo::DM_euclidean),
dist_units(WeightsMetaInfo::DU_mile),
min_1nn_dist_euc(-1), max_1nn_dist_euc(-1), max_dist_euc(-1),
min_1nn_dist_arc(-1), max_1nn_dist_arc(-1), max_dist_arc(-1),
sourceSR(NULL), rtree_bbox_ready(false)
{
	wxLogMessage("Entering Project::Project (new project)");
	
	datasource    = p_datasource->Clone();
	project_title = proj_title;
	layer_title   = layername_s;
	layername     = layername_s;
	
	bool wd_success = false;
	if (FileDataSource* fds = dynamic_cast<FileDataSource*>(datasource)) {
		wxString fp = fds->GetFilePath();
		wd_success = SetWorkingDir(fp);
		if (!wd_success) {
			wxLogMessage("Warning: could not set Working Dir" );
		}
	}
	if (!wd_success) {
		// attempt to set working dir according to standard location
		wd_success = SetWorkingDir(wxGetHomeDir());
		if (!wd_success) {
			wxLogMessage("Warning: could not set Working Dir to wxGetHomeDir()");
		}
	}
	
	// variable_order instance (table information) is newly created
	// its content will be update in InitFromXXX() by calling function
	// CorrectVarGroups()
	LayerConfiguration* layer_conf = new LayerConfiguration(layername, datasource);
	project_conf = new ProjectConfiguration(proj_title, layer_conf);
	
	// Init new project from datasource
	is_project_valid = CommonProjectInit();
	if (is_project_valid) {
		// Project file needs an initial save to existing source can
		// be enabled.
		save_manager->SetAllowEnableSave(true);
		save_manager->SetMetaDataSaveNeeded(true);
	}
	
	wxLogMessage("Exiting Project::Project");
}

Project::~Project()
{
	wxLogMessage("Entering Project::~Project");
	
    if (project_conf) delete project_conf; project_conf=0;
    // datasource* has been deleted in project_conf* layer*
    datasource = 0;
    
    // Again, WeightsManInterface is not needed.
	if (WeightsNewManager* o = dynamic_cast<WeightsNewManager*>(w_man_int)) {
        if (o) {
            delete o; o = 0;
        }
    }
	for (size_t i=0, iend=mean_centers.size(); i<iend; i++)
        delete mean_centers[i];
    
	for (size_t i=0, iend=centroids.size(); i<iend; i++)
        delete centroids[i];
    
	if (voronoi_rook_nbr_gal)
        delete [] voronoi_rook_nbr_gal;
    
	for (std::map<wxString, i_array_type*>::iterator i= shared_category_scratch.begin();
         i != shared_category_scratch.end(); ++i) {
		delete i->second;
	}
	
    // clean multi-layers, the actual memory
    std::map<wxString, BackgroundMapLayer*>::iterator it;
    for (it=bg_maps.begin(); it!=bg_maps.end(); it++) {
        BackgroundMapLayer* ml = it->second;
        ml->CleanMemory();
        delete ml;
    }
    for (it=fg_maps.begin(); it!=fg_maps.end(); it++) {
        BackgroundMapLayer* ml = it->second;
        ml->CleanMemory();
        delete ml;
    }
    
	OGRDataAdapter::GetInstance().Close();
	
	//NOTE: the wxGrid instance in TableFrame has
	// ownership and is therefore responsible for deleting the
	// table_int when it closes.
	//if (table_int) delete table_int; table_int = 0;

    // clean up any global settings
    MapCanvas::ResetEmptyFlag();
    
    if (cat_classif_manager) {
        delete cat_classif_manager;
        cat_classif_manager=0;
    }
    
	wxLogMessage("Exiting Project::~Project");
}

CatClassifManager* Project::GetCatClassifManager()
{
    return cat_classif_manager;
}

int Project::GetNumFields()
{
	int n_fields = 0;
	if (layer_proxy) n_fields = layer_proxy->GetNumFields();
	return n_fields;
}

int Project::GetNumRecordsNoneEmpty()
{
    return num_records - MapCanvas::GetEmptyNumber();
}

void Project::UpdateProjectConf(ProjectConfiguration* conf)
{
	wxLogMessage("Project::UpdateProjectConf()");
    LayerConfiguration* layer_conf = conf->GetLayerConfiguration();
    wxString _layername = layer_conf->GetName();
    IDataSource* _ds = layer_conf->GetDataSource();
    
    if (layername == _layername) {
        // we only update Custom Categories
        // first correct variable_order
        std::vector<wxString> var_list = layer_proxy->GetFieldNames();
        std::vector<GdaConst::FieldType> var_types = layer_proxy->GetFieldTypes();
        VarOrderPtree* variable_order = layer_conf->GetVarOrderPtree();
        variable_order->CorrectVarGroups(var_list, var_types,
                                         IsFieldCaseSensitive());
        project_conf->GetLayerConfiguration()->SetVariableOrder(variable_order);
        table_int->Update(*variable_order);
    } else {
        wxString msg = _("Update project information failed. \n\nDetails: The layer information defined in project file does no match opened datasource.");
        throw GdaException(msg.c_str());
    }
}

GdaConst::DataSourceType Project::GetDatasourceType()
{
	return datasource->GetType();
}

wxString Project::GetProjectFullPath()
{
	wxLogMessage("Project::GetProjectFullPath()");
	wxString fp;
	if (!GetWorkingDir().GetPath().IsEmpty() && !proj_file_no_ext.IsEmpty()) {
		fp << GetWorkingDir().GetPathWithSep();
		fp << proj_file_no_ext << ".gda";
	}
	return fp;
}

void Project::SetProjectFullPath(const wxString& proj_full_path)
{
	wxLogMessage("Project::SetProjectFullPath()");
	wxFileName temp(proj_full_path);
	SetWorkingDir(proj_full_path);
	proj_file_no_ext = temp.GetName();
}

bool Project::SetWorkingDir(const wxString& path)
{
	wxFileName dir;
	if (wxDirExists(path)) {
		dir.AssignDir(path);
	} else {
		dir.Assign(path);
		wxString t = dir.GetPath();
		dir.Clear();
		dir.AssignDir(t);
	}
	if (!dir.IsOk()) {
		return false;
	}
	if (!dir.IsDir()) {
		return false;
	}
	if (!dir.DirExists()) {
		return false;
	}
	working_dir.Clear();
	working_dir = dir;
	return true;
}

Shapefile::ShapeType Project::GetShapefileType()
{
    Shapefile::ShapeType shape_type = Shapefile::NULL_SHAPE;
    if ( main_data.header.shape_type == Shapefile::POINT_TYP) {
        shape_type = Shapefile::POINT_TYP;
    } else if (main_data.header.shape_type == Shapefile::POLYGON) {
        shape_type = Shapefile::POLYGON;
    } else if (main_data.header.shape_type == Shapefile::POLY_LINE) {
        shape_type = Shapefile::POLY_LINE;
    }
    return shape_type;
}

Shapefile::ShapeType Project::GetGdaGeometries(std::vector<GdaShape*>& geometries)
{
	wxLogMessage("Project::GetGdaGeometries()");
	Shapefile::ShapeType shape_type = Shapefile::NULL_SHAPE;
	int num_geometries = main_data.records.size();
	if ( main_data.header.shape_type == Shapefile::POINT_TYP) {
		Shapefile::PointContents* pc;
		for (int i=0; i<num_geometries; i++) {
			pc = (Shapefile::PointContents*)main_data.records[i].contents_p;
            if (pc->shape_type == 0) {
                geometries.push_back(new GdaPoint());
            } else {
                geometries.push_back(new GdaPoint(wxRealPoint(pc->x, pc->y)));
            }
		}
		shape_type = Shapefile::POINT_TYP;
	} else if (main_data.header.shape_type == Shapefile::POLYGON) {
		Shapefile::PolygonContents* pc;
		for (int i=0; i<num_geometries; i++) {
			pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
            if (pc->shape_type == 0) {
                geometries.push_back(new GdaPolygon());
            } else {
                geometries.push_back(new GdaPolygon(pc));
            }
		}
		shape_type = Shapefile::POLYGON;
    } else if (main_data.header.shape_type == Shapefile::POLY_LINE) {
        Shapefile::PolyLineContents* pc;
        for (int i=0; i<num_geometries; i++) {
            pc = (Shapefile::PolyLineContents*)main_data.records[i].contents_p;
            if (pc->shape_type == 0) {
                geometries.push_back(new GdaPolyLine());
            } else {
                geometries.push_back(new GdaPolyLine(pc));
            }
        }
        shape_type = Shapefile::POLY_LINE;
    }
	return shape_type;
}

std::vector<wxFloat64> Project::GetBBox(int idx)
{
    wxLogMessage("Project::GetBBox()");
    std::vector<wxFloat64> box(4);
    if (main_data.header.shape_type == Shapefile::POINT_TYP) {
        Shapefile::PointContents* pc = (Shapefile::PointContents*)main_data.records[idx].contents_p;
        box[0] = pc->x;
        box[1] = pc->y;
        box[2] = pc->x;
        box[3] = pc->y;
    } else if (main_data.header.shape_type == Shapefile::POLYGON) {
        Shapefile::PolygonContents* pc = (Shapefile::PolygonContents*)main_data.records[idx].contents_p;
        return pc->box;
    }
    return box;
}

rtree_box_2d_t& Project::GetBBoxRtree()
{
	wxLogMessage("Project::CalcEucPlaneRtreeStats()");
    if ( rtree_bbox_ready ) {
        return rtree_bbox;
    }
    if (main_data.header.shape_type == Shapefile::POLYGON) {
        Shapefile::PolygonContents* pc;
        int num_geometries = main_data.records.size();
        for (int i=0; i<num_geometries; i++) {
            pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
            pc->box[0];
            // create a box, tl, br
            box_2d b(pt_2d(pc->box[0], pc->box[2]), pt_2d(pc->box[1], pc->box[3]));
            // insert new value
            rtree_bbox.insert(std::make_pair(b, i));
        }
        rtree_bbox_ready = true;
    }
    return rtree_bbox;
}

void Project::CalcEucPlaneRtreeStats()
{
    wxLogMessage("Project::CalcEucPlaneRtreeStats()");
    
    GetCentroids();
    size_t num_obs = centroids.size();
    std::vector<pt_2d> pts(num_obs);
    std::vector<double> x(num_obs);
    std::vector<double> y(num_obs);
    for (size_t i=0; i<num_obs; ++i) {
        pts[i] = pt_2d(centroids[i]->center_o.x, centroids[i]->center_o.y);
        x[i] = centroids[i]->center_o.x;
        y[i] = centroids[i]->center_o.y;
    }
    SpatialIndAlgs::fill_pt_rtree(rtree_2d, pts);
    double mean_d_1nn, median_d_1nn;
    SpatialIndAlgs::get_pt_rtree_stats(rtree_2d, min_1nn_dist_euc,
                                       max_1nn_dist_euc, mean_d_1nn,
                                       median_d_1nn);
    wxRealPoint pt1, pt2;
    max_dist_euc = PointSetAlgs::EstDiameter(x, y, false, pt1, pt2);
}

void Project::CalcUnitSphereRtreeStats()
{
	wxLogMessage("Project::CalcUnitSphereRtreeStats()");
	GetCentroids();
	size_t num_obs = centroids.size();
	std::vector<pt_lonlat> pts_ll(num_obs);
	std::vector<pt_3d> pts_3d(num_obs);
	std::vector<double> x(num_obs);
	std::vector<double> y(num_obs);
	for (size_t i=0; i<num_obs; ++i) {
		pts_ll[i] = pt_lonlat(centroids[i]->center_o.x, centroids[i]->center_o.y);
		x[i] = centroids[i]->center_o.x;
		y[i] = centroids[i]->center_o.y;
	}
	SpatialIndAlgs::to_3d_centroids(pts_ll, pts_3d);
	SpatialIndAlgs::fill_pt_rtree(rtree_3d, pts_3d);
	double mean_d_1nn, median_d_1nn;
	SpatialIndAlgs::get_pt_rtree_stats(rtree_3d, min_1nn_dist_arc,
                                       max_1nn_dist_arc, mean_d_1nn,
                                       median_d_1nn);
	wxRealPoint pt1, pt2;
	double d = PointSetAlgs::EstDiameter(x, y, true, pt1, pt2);
	max_dist_arc = GenGeomAlgs::DegToRad(d);
}

OGRSpatialReference* Project::GetSpatialReference()
{
	wxLogMessage("Project::GetSpatialReference()");
	OGRSpatialReference* spatial_ref = NULL;
	OGRTable* ogr_table = dynamic_cast<OGRTable*>(table_int);
	if (ogr_table != NULL) {
		// it's a OGRTable
		OGRLayerProxy* exist_layer = ogr_table->GetOGRLayer();
		spatial_ref = exist_layer->GetSpatialReference();
	} else {
		// DbfTable
		wxString ds_name = datasource->GetOGRConnectStr();
        GdaConst::DataSourceType ds_type = datasource->GetType();
        
		if (!wxFileExists(ds_name)) {
			return NULL;
		}
		OGRDatasourceProxy* ogr_ds = new OGRDatasourceProxy(ds_name, ds_type,
                                                            true);
		OGRLayerProxy* ogr_layer = ogr_ds->GetLayerProxy(layername);
		spatial_ref = ogr_layer->GetSpatialReference();
		delete ogr_ds;
	}
	return spatial_ref;
}

bool Project::CheckSpatialProjection(bool& check_again, bool is_arc)
{
    // Check if latitude and longitude are used in spatial reference
    bool cont_proceed = false;
    if (sourceSR == NULL) {
        wxString msg = _("Warning: unknown projection information, distance may be incorrect.\n\nProceed anyway?");
        CheckSpatialRefDialog dlg(NULL, msg);
        check_again = dlg.IsCheckAgain();
        if (dlg.ShowModal() == wxID_OK) {
            cont_proceed = true;
            check_again = dlg.IsCheckAgain();
        }
    } else {
        bool is_euclidean = !is_arc;
        if (is_euclidean && project_unit.CmpNoCase("degree") == 0) {
            wxString msg = _("Warning: coordinates are not projected, distance will be incorrect.\n\nProceed anyway?");
            CheckSpatialRefDialog dlg(NULL, msg);
            if (dlg.ShowModal() == wxID_OK) {
                cont_proceed = true;
                check_again = dlg.IsCheckAgain();
            }
        } else  {
    
            if (is_arc && project_unit.CmpNoCase("degree") != 0) {
                //if the data are projected and one tries to
                // create an arc distance, same warning.
                wxString msg = _("Warning: coordinates are projected, arc distance will be incorrect.\n\nProceed anyway?");
                CheckSpatialRefDialog dlg(NULL, msg);
                if (dlg.ShowModal() == wxID_OK) {
                    cont_proceed = true;
                    check_again = dlg.IsCheckAgain();
                }
            }  else {
                // GOOD! 
                cont_proceed = true;
                check_again = false; // no need to check again
            }
        }
    }
    // return if user wants to continue proceeding
    return cont_proceed;
}

void Project::SaveOGRDataSource()
{
	wxLogMessage("Project::SaveOGRDataSource()");
	// This function will only be called to save file or directory (OGR)
	wxString tmp_prefix = "GdaTmp_";
	wxArrayString all_tmp_files;
	
	try{        
		bool is_update = false;
		
		// save to a new tmp file(s) to backup original file(s)
		wxString ds_name = datasource->GetOGRConnectStr();
		wxFileName fn(ds_name);
		wxString tmp_ds_name = ds_name;
		if (wxFileExists(ds_name) && fn.GetExt().CmpNoCase("sqlite")!=0) {
			// for existing sqlite file, we add or replace layer
			tmp_ds_name = fn.GetPathWithSep()+tmp_prefix+fn.GetFullName();
			if ( wxFileExists(tmp_ds_name) ) {
				wxRemoveFile(tmp_ds_name);
			}
		}
		
		// special cases that saves by update original datasource
		if ( fn.GetExt().CmpNoCase("sqlite")==0 ) {
			is_update = true;
		}
		
		SaveDataSourceAs(tmp_ds_name, is_update);
		
		// replace old file with tmp files, then delete tmp files
		if (tmp_ds_name!=ds_name && wxFileExists(tmp_ds_name)) {
			wxString tmp_name = fn.GetName();
            wxString dirname = fn.GetPath();
            wxString filename;
            
            wxDir wdir;
			
            if ( wdir.Open( dirname ) )
            {
                bool cont = wdir.GetFirst(&filename);
                while ( cont )
                {
                    if (filename.Contains(tmp_prefix)) {
                        wxString path = dirname;
                        path << wxFileName::GetPathSeparator() << filename;
                        all_tmp_files.push_back(path);
                    }
                    cont = wdir.GetNext(&filename);
                }
            }
            
			for (size_t i=0; i< all_tmp_files.size(); i++) {
				wxString orig_fname = all_tmp_files[i];
				orig_fname.Replace(tmp_prefix, "");
				wxCopyFile(all_tmp_files[i], orig_fname);
				wxRemoveFile(all_tmp_files[i]);
			}
		}
	} catch( GdaException& e){
		for (size_t i=0; i< all_tmp_files.size(); i++) {
			wxRemoveFile(all_tmp_files[i]);
		}
		throw e;
	}
}

void Project::SaveDataSourceAs(const wxString& new_ds_name, bool is_update)
{
	wxLogMessage("Entering Project::SaveDataSourceAs");
	wxLogMessage("New Datasource Name:" + new_ds_name);
   
    std::vector<GdaShape*> geometries;
	try {
		// SaveAs only to same datasource
		GdaConst::DataSourceType ds_type = datasource->GetType();
        if (ds_type == GdaConst::ds_dbf ) {
            // OGR only support ESRI Shapefile, and doesn't support DBF separatly.
            // see : http://www.gdal.org/ogr_formats.html
            ds_type = GdaConst::ds_shapefile;
        }
        
		wxString ds_format = IDataSource::GetDataTypeNameByGdaDSType(ds_type);
		if ( !IDataSource::IsWritable(ds_type) ) {
            wxString error_message = wxString::Format(_("GeoDa does not support creating data of %s. Please try to 'Export' to other supported data source format."), ds_format);
			throw GdaException(error_message.mb_str());
		}
		// call to initial OGR instance
		OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
		
		// Get spatial reference from this project
		OGRSpatialReference* spatial_ref = GetSpatialReference();
		
		// Get Gda geometries and convert to OGR geometries from this project
		Shapefile::ShapeType shape_type = GetGdaGeometries(geometries);
		
		// Get default selected rows: all records should be saved or saveas
	    std::vector<int> selected_rows;
		for (size_t i=0; i<table_int->GetNumberRows(); i++) {
			selected_rows.push_back(i);
		}
	
        // Create in-memory OGR geometries
	    std::vector<OGRGeometry*> ogr_geometries;
        OGRwkbGeometryType geom_type;
        geom_type = ogr_adapter.MakeOGRGeometries(geometries, shape_type,
                                                  ogr_geometries, selected_rows);
        
        // NOTE: for GeoJSON, automatically transform to WGS84
        if (spatial_ref && ds_type == GdaConst::ds_geo_json) {
            OGRSpatialReference wgs84_ref;
            wgs84_ref.importFromEPSG(4326);
            OGRCoordinateTransformation *poCT;
#ifdef __PROJ6__
            spatial_ref->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
            wgs84_ref.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
            poCT = OGRCreateCoordinateTransformation(spatial_ref, &wgs84_ref);
            for (size_t i=0; i < ogr_geometries.size(); i++) {
                ogr_geometries[i]->transform(poCT);
            }
        }
		
		// Start saving
		int prog_n_max = 0;
		if (table_int) prog_n_max = table_int->GetNumberRows();
#ifdef __WXOSX__
		wxProgressDialog prog_dlg(_("Save data source progress dialog"),
                                  _("Saving data..."),
                                  prog_n_max, NULL,
                                  wxPD_CAN_ABORT|wxPD_AUTO_HIDE|wxPD_APP_MODAL);
#endif
        OGRLayerProxy* new_layer;
        new_layer = ogr_adapter.ExportDataSource(ds_format, new_ds_name,
                                                 layername, geom_type,
                                                 ogr_geometries, table_int,
                                                 selected_rows, spatial_ref,
                                                 is_update, cpg_encode);
        if (new_layer == NULL) {
            wxString msg = _("Saving data source cancelled.");
            throw GdaException(msg.mb_str());
        }
        
        bool cont = true;
        while ( new_layer && new_layer->export_progress < prog_n_max ) {
#ifdef __WXOSX__
            cont = prog_dlg.Update(new_layer->export_progress);
            if ( !cont ) {
                new_layer->stop_exporting = true;
                ogr_adapter.CancelExport(new_layer);
                return;
            }
#endif
            if ( new_layer->export_progress == -1 ) {
                wxString msg = wxString::Format(_("Save as data source (%s) failed.\n\nDetails:"),new_ds_name);
                msg << new_layer->error_message;
                throw GdaException(msg.mb_str());
            }
            wxMilliSleep(100);
        }
        ogr_adapter.StopExport();
		
		for (size_t i=0; i<geometries.size(); i++) {
			delete geometries[i];
		}
	} catch( GdaException& e ) {
        
		// clean intermedia memory
		for (size_t i=0; i < geometries.size(); i++) {
			delete geometries[i];
		}
		throw e;
	}
	wxLogMessage("Exiting Project::SaveDataSourceAs");
}

void Project::SpecifyProjectConfFile(const wxString& proj_fname)
{
	wxLogMessage("Project::SpecifyProjectConfFile()");
	if (proj_fname.IsEmpty()) {
        wxString msg = _("Project filename not specified.");
		throw GdaException(msg.mb_str());
	}
	project_conf->SetFilePath(proj_fname);
	SetProjectFullPath(proj_fname);
}

bool Project::HasUnsavedChange()
{
	wxLogMessage("Project::HasUnsavedChange()");
    TableInterface* tbl = GetTableInt();
    if (tbl && tbl->ChangedSinceLastSave())
        return true;
    return false;
}

void Project::SaveProjectConf()
{
	wxLogMessage("Entering Project::SaveProjectConf");
	if (project_conf->GetFilePath().IsEmpty() &&
        (GetTableInt()->IsTimeVariant() ||
         (w_man_int && w_man_int->GetIds().size()>0)) ) {
		
        // save project file at the same directory of the file datasource
        if ( IsFileDataSource()) {
            wxString ds_path = datasource->GetOGRConnectStr();
            bool wd_success = SetWorkingDir(ds_path);
            if (wd_success) {
                wxFileName temp(ds_path);
                proj_file_no_ext = temp.GetName();
                wxString prj_path = GetProjectFullPath();
                project_conf->SetFilePath(prj_path);
            }
        }
    }
    if (!project_conf->GetFilePath().IsEmpty()) {
        UpdateProjectConf();
        project_conf->Save(project_conf->GetFilePath());
        GetTableInt()->SetProjectChangedSinceLastSave(false);
    }
	wxLogMessage("Exiting Project::SaveProjectConf");
}

bool Project::IsFileDataSource() 
{
    if (datasource) return datasource->IsFileDataSource();
    return false;
}

void Project::SaveDataSourceData()
{
	wxLogMessage("Entering Project::SaveDataSourceData");
	// for some read-only datasources, suggest Export dialog
	GdaConst::DataSourceType ds_type = datasource->GetType();
	if (ds_type == GdaConst::ds_wfs ||
        ds_type == GdaConst::ds_kml ||
        ds_type == GdaConst::ds_xlsx ||
        ds_type == GdaConst::ds_xls ||
        ds_type == GdaConst::ds_esri_arc_sde )
    {
		wxString msg = _("The data source is read only. Please try to save as other data source.");
		throw GdaException(msg.mb_str());
	}
	
	if (table_int->ChangedSinceLastSave()) {
        
		wxString save_err_msg;
		try {
			// for saving changes in database, call OGRTableInterface::Save()
			if (
				ds_type == GdaConst::ds_esri_file_geodb ||
				ds_type == GdaConst::ds_postgresql ||
				ds_type == GdaConst::ds_oci ||
				ds_type == GdaConst::ds_mysql ||
                ds_type == GdaConst::ds_cartodb) {
				
				table_int->Save(save_err_msg);
			}
			// for other datasources, call OGR interface to save.
			else {
				SaveOGRDataSource();
			}
		} catch( GdaException& e) {
			save_err_msg = e.what();
		}
		if (!save_err_msg.empty()) {
			table_int->SetChangedSinceLastSave(true);
			throw GdaException(save_err_msg.mb_str());
		} else {
			table_int->SetChangedSinceLastSave(false);
			SaveButtonManager* sbm = GetSaveButtonManager();
			if (sbm) sbm->SetDbSaveNeeded(false);
		}
	}
	
	if (isTableOnly && main_data.records.size()>0
			&& layer_proxy != NULL) {
		// case: create geometries for table-only datasource
		// try to save the geometries (e.g. database table)
		// NOTE: OGR/GDAL 2.0 is still implementing addGeomField feature.
		layer_proxy->AddGeometries(main_data);
	}	
	wxLogMessage("Exiting Project::SaveDataSourceData");
}

void Project::UpdateProjectConf()
{
	wxLogMessage("In Project::UpdateProjectConf");
	LayerConfiguration* layer_conf = project_conf->GetLayerConfiguration();
	datasource = layer_conf->GetDataSource();
	VarOrderPtree* var_order = layer_conf->GetVarOrderPtree();
	if (var_order) var_order->ReInitFromTableInt(table_int);
	CustomClassifPtree* cc = layer_conf->GetCustClassifPtree();
	if (cc) cc->SetCatClassifList(GetCatClassifManager());
	WeightsManPtree* spatial_weights = layer_conf->GetWeightsManPtree();
	WeightsNewManager* wnm = ((WeightsNewManager*) GetWManInt());
	if (spatial_weights)
        spatial_weights->SetWeightsMetaInfoList(wnm->GetPtreeEntries());
	DefaultVarsPtree* default_vars = layer_conf->GetDefaultVarsPtree();
	{
		std::vector<wxString> def_tm_ids(default_var_time.size());
		for (int t=0, sz=default_var_time.size(); t<sz; ++t) {
			def_tm_ids[t] = table_int->GetTimeString(default_var_time[t]);
		}
		if (default_vars)
            default_vars->SetDefaultVarList(default_var_name,  def_tm_ids);
	}
}

wxString Project::GetProjectTitle()
{
	if (!project_title.IsEmpty()) return project_title;
	if (!layer_title.IsEmpty()) return layer_title;
	if (!layername.IsEmpty()) return layername;
	
	wxString current_proj_name = project_conf->GetFilePath();
	if (!current_proj_name.IsEmpty()) {
		wxFileName f(current_proj_name);
		return f.GetName();
	}
	return "";
}

bool Project::ExportVoronoi()
{
	wxLogMessage("Project::ExportVoronoi()");
	GetVoronoiPolygons();
	// generate a list of list of duplicates.  Or, better yet, have a map of
	// lists where the key is the id, and the value is a list of all ids
	// in the same set.
	// If duplicates exist, then give option to save duplicate sets to Table
	if (IsPointDuplicates()) DisplayPointDupsWarning();
	if (voronoi_polygons.size() != GetNumRecords()) return false;
	
    return true;
}

/**
 * mean_centers == true: save mean centers
 * mean_centers == false: save centroids
 */
void Project::ExportCenters(bool is_mean_centers)
{	
	wxLogMessage("Project::ExportCenters()");
	if (is_mean_centers) {
		GetMeanCenters();
	} else {
		GetCentroids();
	}
}

bool Project::IsPointDuplicates()
{
	wxLogMessage("Project::IsPointDuplicates()");

	if (!point_duplicates_initialized) {
		std::vector<double> x;
		std::vector<double> y;
		GetCentroids(x, y);
		Gda::VoronoiUtils::FindPointDuplicates(x, y, point_duplicates);
	}
	return point_duplicates.size() > 0;
}

void Project::DisplayPointDupsWarning()
{
	wxLogMessage("Project::DisplayPointDupsWarning()");

	wxString msg = _("Duplicate Thiessen polygons exist due to duplicate or near-duplicate map points. Press OK to save duplicate polygon ids to Table.");
	wxMessageDialog dlg(NULL, msg, _("Duplicate Thiessen Polygons Found"),
                        wxOK | wxCANCEL | wxICON_INFORMATION);
	if (dlg.ShowModal() == wxID_OK) SaveVoronoiDupsToTable();
	point_dups_warn_prev_displayed = true;
}

void Project::GetVoronoiRookNeighborMap(std::vector<std::set<int> >& nbr_map)
{
	wxLogMessage("Project::GetVoronoiRookNeighborMap()");

	IsPointDuplicates();
	std::vector<double> x;
	std::vector<double> y;
	GetCentroids(x, y);
	Gda::VoronoiUtils::PointsToContiguity(x, y, false, nbr_map);
}

void Project::GetVoronoiQueenNeighborMap(std::vector<std::set<int> >& nbr_map)
{
	wxLogMessage("Project::GetVoronoiQueenNeighborMap()");

	std::vector<double> x;
	std::vector<double> y;
	GetCentroids(x, y);
	Gda::VoronoiUtils::PointsToContiguity(x, y, true, nbr_map);
}

GalElement* Project::GetVoronoiRookNeighborGal()
{
	wxLogMessage("Project::GetVoronoiRookNeighborGal()");

	if (!voronoi_rook_nbr_gal) {
		std::vector<std::set<int> > nbr_map;
		GetVoronoiRookNeighborMap(nbr_map);
		voronoi_rook_nbr_gal = Gda::VoronoiUtils::NeighborMapToGal(nbr_map);
	}
	return voronoi_rook_nbr_gal;
}

void Project::SaveVoronoiDupsToTable()
{
	wxLogMessage("Project::SaveVoronoiDupsToTable()");

	if (!IsPointDuplicates()) return;
	std::vector<SaveToTableEntry> data(1);
	std::vector<wxInt64> dup_ids(num_records, -1);
	std::vector<bool> undefined(num_records, true);
	for (std::list<std::list<int> >::iterator dups_iter = point_duplicates.begin();
         dups_iter != point_duplicates.end(); dups_iter++)
    {
		int head_id = *(dups_iter->begin());
        std::list<int>::iterator iter = dups_iter->begin();
        iter++; // ignore first one
		for (; iter != dups_iter->end(); iter++) {
			undefined[*iter] = false;
			dup_ids[*iter] = head_id+1;
		}			
	}
	data[0].l_val = &dup_ids;
	data[0].undefined = &undefined;
	data[0].label = _("Duplicate IDs");
	data[0].field_default = "DUP_IDS";
	data[0].type = GdaConst::long64_type;
	
	wxString title = _("Save Duplicate Thiessen Polygon Ids");
	SaveToTableDlg dlg(this, NULL, data, title, wxDefaultPosition,
                       wxSize(400,400));
	dlg.ShowModal();	
}

CovSpHLStateProxy* Project::GetPairsHLState()
{
	if (!pairs_hl_state) {
		pairs_hl_state = new CovSpHLStateProxy(GetHighlightState(),
                                               GetSharedPairsBimap());
	}
	return pairs_hl_state;
}

TableBase* Project::FindTableBase()
{
	if (frames_manager == NULL) return NULL;
	std::list<FramesManagerObserver*> observers(frames_manager->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (TableFrame* w = dynamic_cast<TableFrame*>(*it)) {
			return w->GetTableBase();
		}
	}
	return NULL;
}

void Project::GetSelectedRows(std::vector<int>& rowids)
{
	rowids.clear();
	int n_rows = GetNumRecords();
    std::vector<bool>& hs = highlight_state->GetHighlight();
	for ( int i=0; i<n_rows; i++ ) {
		if (hs[i] ) rowids.push_back(i);
	}
}

wxGrid* Project::FindTableGrid()
{
	return FindTableBase()->GetView();
}

void Project::AddNeighborsToSelection(boost::uuids::uuid weights_id)
{
	wxLogMessage("Entering Project::AddNeighborsToSelection");
	if (!GetWManInt()) return;
	GalWeight* gal_weights = GetWManInt()->GetGal(weights_id);
	if (!gal_weights || !gal_weights->gal) {
		wxLogMessage("Warning: no current weight matrix found");
		return;
	}
	
	// go through the list of all objects in current selection
	// for each selected object and add each of its neighbor to
	// the list so long as it isn't already selected.	
	
	HighlightState& hs = *highlight_state;
	std::vector<bool>& h = hs.GetHighlight();
	int nh_cnt = 0;
	std::vector<bool> add_elem(gal_weights->num_obs, false);

    std::vector<int> new_highlight_ids;
    
	for (int i=0; i<gal_weights->num_obs; i++) {
		if (h[i]) {
			GalElement& e = gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				int obs = e[j];
				if (!h[obs] && !add_elem[obs]) {
					add_elem[obs] = true;
                    new_highlight_ids.push_back(obs);
				}
			}
		}
	}
    
    for (int i=0; i<(int)new_highlight_ids.size(); i++) {
        h[ new_highlight_ids[i] ] = true;
        nh_cnt ++;
    }
	
	if (nh_cnt > 0) {
		hs.SetEventType(HLStateInt::delta);
		hs.notifyObservers();
	}
	wxLogMessage("Exiting Project::AddNeighborsToSelection");
}

void Project::AddMeanCenters()
{
	wxLogMessage("In Project::AddMeanCenters");
	
	if (!table_int || main_data.records.size() == 0) return;
	GetMeanCenters();
	if (mean_centers.size() != num_records) return;
	
	std::vector<double> x(num_records, 0);
	std::vector<bool> x_undef(num_records, false);
	std::vector<double> y(num_records, 0);
	std::vector<bool> y_undef(num_records, false);
	for (int i=0; i<num_records; i++) {
		if (mean_centers[i]->isNull()) {
			x_undef[i] = true;
			y_undef[i] = true;
		} else {
			x[i] = mean_centers[i]->center_o.x;
			y[i] = mean_centers[i]->center_o.y;
		}
	}
	
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &x;
	data[0].undefined = &x_undef;
	data[0].label = _("X-Coordinates");
	data[0].field_default = "COORD_X";
	data[0].type = GdaConst::double_type;
	
	data[1].d_val = &y;
	data[1].undefined = &y_undef;
	data[1].label = _("Y-Coordinates");
	data[1].field_default = "COORD_Y";
	data[1].type = GdaConst::double_type;	
	
	SaveToTableDlg dlg(this, NULL, data, _("Add Mean Centers to Table"), wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void Project::AddCentroids()
{
	wxLogMessage("In Project::AddCentroids");
	
	if (!table_int || main_data.records.size() == 0) return;	
	GetCentroids();
	if (centroids.size() != num_records) return;
	
	std::vector<double> x(num_records);
	std::vector<bool> x_undef(num_records, false);
	std::vector<double> y(num_records);
	std::vector<bool> y_undef(num_records, false);
	for (int i=0; i<num_records; i++) {
		if (centroids[i]->isNull()) {
			x_undef[i] = true;
			y_undef[i] = true;
		} else {
			x[i] = centroids[i]->center_o.x;
			y[i] = centroids[i]->center_o.y;
		}
	}
	
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &x;
	data[0].undefined = &x_undef;
	data[0].label = _("X-Coordinates");
	data[0].field_default = "COORD_X";
	data[0].type = GdaConst::double_type;
	
	data[1].d_val = &y;
	data[1].undefined = &y_undef;
	data[1].label = _("Y-Coordinates");
	data[1].field_default = "COORD_Y";
	data[1].type = GdaConst::double_type;	
	
	SaveToTableDlg dlg(this, NULL, data, _("Add Centroids to Table"),
                       wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

const std::vector<GdaPoint*>& Project::GetMeanCenters()
{
	wxLogMessage("Project::GetMeanCenters()");
	int num_obs = main_data.records.size();
	if (mean_centers.size() == 0 && num_obs > 0) {
		if (main_data.header.shape_type == Shapefile::POINT_TYP) {
			mean_centers.resize(num_obs);
			Shapefile::PointContents* pc;
			for (int i=0; i<num_obs; i++) {
				pc = (Shapefile::PointContents*)
				main_data.records[i].contents_p;
				if (pc->shape_type == 0) {
					mean_centers[i] = new GdaPoint();
				} else {
					mean_centers[i] = new GdaPoint(wxRealPoint(pc->x, pc->y));
				}
			}
		} else if (main_data.header.shape_type == Shapefile::POLYGON) {
			mean_centers.resize(num_obs);
			Shapefile::PolygonContents* pc;
			for (int i=0; i<num_obs; i++) {
				pc = (Shapefile::PolygonContents*)
				main_data.records[i].contents_p;
				GdaPolygon poly(pc);
				if (poly.isNull()) {
					mean_centers[i] = new GdaPoint();
				} else {
					mean_centers[i] =
					new GdaPoint(GdaShapeAlgs::calculateMeanCenter(&poly));
				}
			}
		}
	}
	return mean_centers;
}

void Project::GetMeanCenters(std::vector<double>& x, std::vector<double>& y)
{
	wxLogMessage("Project::GetMeanCenters(std::vector<double>& x, std::vector<double>& y)");
	GetMeanCenters();
	int num_obs = mean_centers.size();
	if (x.size() < num_obs) x.resize(num_obs);
	if (y.size() < num_obs) y.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		x[i] = mean_centers[i]->center_o.x;
		y[i] = mean_centers[i]->center_o.y;
	}
}

const std::vector<GdaPoint*>& Project::GetCentroids()
{
	wxLogMessage("Project::GetCentroids()");
   
    if (layer_proxy->IsTableOnly()) {
        if (centroids.size() == 0 && num_records > 0) {
            centroids.resize(num_records);
            double x, y;
            for ( int row_idx=0; row_idx < num_records; row_idx++ ) {
                Shapefile::PointContents* pc;
                pc = (Shapefile::PointContents*)main_data.records[row_idx].contents_p;
                centroids[row_idx] = new GdaPoint(pc->x, pc->y);
            }
        }
    } else {
        layer_proxy->GetCentroids(centroids);
    }

	return centroids;	
}

GdaPolygon* Project::GetMapBoundary()
{
    wxLogMessage("Project::GetMapBoundary()");
    
    if (layer_proxy->IsTableOnly()) {
        return NULL;
    } else {
        return layer_proxy->GetMapBoundary();
    }
}

void Project::GetMapExtent(double& minx, double& miny, double& maxx, double& maxy)
{
    wxLogMessage("Project::GetMapExtent()");
    
    if (layer_proxy) {
        layer_proxy->GetExtent(minx, miny, maxx, maxy);
    }
}

void Project::GetCentroids(std::vector<double>& x, std::vector<double>& y)
{
	wxLogMessage("Project::GetCentroids(std::vector<double>& x, std::vector<double>& y)");
	GetCentroids();
	int num_obs = centroids.size();
	if (x.size() < num_obs) x.resize(num_obs);
	if (y.size() < num_obs) y.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		x[i] = centroids[i]->center_o.x;
		y[i] = centroids[i]->center_o.y;
	}
}

void Project::GetCentroids(std::vector<wxRealPoint>& pts)
{
	wxLogMessage("Project::GetCentroids(std::vector<wxRealPoint>& pts)");
	GetCentroids();
	int num_obs = centroids.size();
	if (pts.size() < num_obs) pts.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		pts[i].x = centroids[i]->center_o.x;
		pts[i].y = centroids[i]->center_o.y;
	}
}


const std::vector<GdaShape*>& Project::GetVoronoiPolygons()
{
	wxLogMessage("std::vector<GdaShape*>& Project::GetVoronoiPolygons()");
    
	if (voronoi_polygons.size() == num_records) {
		return voronoi_polygons;
	} else {
		for (int i=0; i<voronoi_polygons.size(); i++) {
			delete voronoi_polygons[i];
		}
		voronoi_polygons.clear();
	}
	
	std::vector<double> x;
	std::vector<double> y;
	GetCentroids(x,y);
	
	Gda::VoronoiUtils::MakePolygons(x, y, voronoi_polygons,
                                    voronoi_bb_xmin, voronoi_bb_ymin,
                                    voronoi_bb_xmax, voronoi_bb_ymax);
	for (size_t i=0, iend=voronoi_polygons.size(); i<iend; i++) {
		voronoi_polygons[i]->setPen(*wxBLACK_PEN);
		voronoi_polygons[i]->setBrush(*wxTRANSPARENT_BRUSH);
	}
	
	return voronoi_polygons;
}

double Project::GetMin1nnDistEuc()
{
	if (min_1nn_dist_euc >= 0) return min_1nn_dist_euc;
	CalcEucPlaneRtreeStats();
	return min_1nn_dist_euc;
}

double Project::GetMax1nnDistEuc()
{
	if (max_1nn_dist_euc >= 0) return max_1nn_dist_euc;
	CalcEucPlaneRtreeStats();
	return max_1nn_dist_euc;
}

double Project::GetMaxDistEuc()
{
	if (max_dist_euc >= 0) return max_dist_euc;
	CalcEucPlaneRtreeStats();
	return max_dist_euc;
}

double Project::GetMin1nnDistArc()
{
	if (min_1nn_dist_arc >= 0) return min_1nn_dist_arc;
	CalcUnitSphereRtreeStats();
	return min_1nn_dist_arc;
}

double Project::GetMax1nnDistArc()
{
	if (max_1nn_dist_arc >= 0) return max_1nn_dist_arc;
	CalcUnitSphereRtreeStats();
	return max_1nn_dist_arc;
}

double Project::GetMaxDistArc()
{
	if (max_dist_arc >= 0) return max_dist_arc;
	CalcUnitSphereRtreeStats();
	return max_dist_arc;
}

rtree_pt_2d_t& Project::GetEucPlaneRtree()
{
	return rtree_2d;
}

rtree_pt_3d_t& Project::GetUnitSphereRtree()
{
	return rtree_3d;
}

wxString Project::GetDefaultVarName(int var)
{
	if (var >= 0 && var < default_var_name.size())
        return default_var_name[var];
	return "";
}

void Project::SetDefaultVarName(int var, const wxString& v_name)
{
	if (var >= 0 && var < default_var_name.size()) {
		default_var_name[var] = v_name;
	}
}

int Project::GetDefaultVarTime(int var)
{
	if (var >= 0 && var < default_var_time.size())
        return default_var_time[var];
	return 0;
}

void Project::SetDefaultVarTime(int var, int time)
{
	if (var >= 0 && var < default_var_time.size())
        default_var_time[var] = time;
}

i_array_type* Project::GetSharedCategoryScratch(int num_cats, int num_obs)
{
	wxString key;
	key << num_cats << "X" << num_obs;
	i_array_type* scrtch_p = 0;
	std::map<wxString, i_array_type*>::iterator i;
	i = shared_category_scratch.find(key);
	if (i == shared_category_scratch.end()) {
		scrtch_p = new i_array_type(boost::extents[num_cats][num_obs]);
		shared_category_scratch[key] = scrtch_p;
	} else {
		scrtch_p = i->second;
	}
	return scrtch_p;
}

bool Project::CanModifyGrpAndShowMsgIfNot(TableState* table_state,
                                          const wxString& grp_nm)
{
	int n = table_state->GetNumDisallowGroupModify(grp_nm);
	if (n == 0)
        return true;
	wxString msg(table_state->GetDisallowGroupModifyMsg(grp_nm));
	wxMessageDialog dlg(NULL, msg, _("Warning"), wxOK | wxICON_WARNING);
	dlg.ShowModal();
	return false;
}

WeightsMetaInfo::DistanceMetricEnum Project::GetDefaultDistMetric()
{
	return dist_metric;
}

void Project::SetDefaultDistMetric(WeightsMetaInfo::DistanceMetricEnum dm)
{
	dist_metric = dm;
}

WeightsMetaInfo::DistanceUnitsEnum Project::GetDefaultDistUnits()
{
	return dist_units;
}

void Project::SetDefaultDistUnits(WeightsMetaInfo::DistanceUnitsEnum du)
{
	dist_units = du;
}

void Project::FillDistances(std::vector<double>& D,
                            WeightsMetaInfo::DistanceMetricEnum dm,
                            WeightsMetaInfo::DistanceUnitsEnum du)
{
	wxLogMessage("Project::FillDistances()");
	const std::vector<GdaPoint*>& c = GetCentroids();
	const pairs_bimap_type& pbm = GetSharedPairsBimap();
	typedef pairs_bimap_type::const_iterator pbt_ci;
    
    if (D.size() != pbm.size()) {
        D.resize(pbm.size());
    }
    
	if (dm == WeightsMetaInfo::DM_arc) {
		if (pbm.size() > cached_arc_dist.size()) {
			for (pbt_ci it = pbm.begin(), iend = pbm.end(); it != iend; ++it) {
				// it->left  : data : int
				// it->right : data : UnOrdIntPair
				if (cached_arc_dist.find(it->right) == cached_arc_dist.end()) {
					size_t i = it->right.i;
					size_t j = it->right.j;
					double d = GenGeomAlgs::ComputeArcDistRad(c[i]->GetX(),
                                                              c[i]->GetY(),
                                                              c[j]->GetX(),
                                                              c[j]->GetY());
					cached_arc_dist[it->right] = d;
				}
			}
			if (du == WeightsMetaInfo::DU_km) {
				for (pbt_ci it=pbm.begin(), iend=pbm.end(); it != iend; ++it) {
					D[it->left] = GenGeomAlgs::EarthRadToKm(cached_arc_dist[it->right]);
				}
			} else {
				for (pbt_ci it = pbm.begin(), iend = pbm.end(); it != iend; ++it)
				{
					D[it->left] = GenGeomAlgs::EarthRadToMi(cached_arc_dist[it->right]);
				}
			}
		}
	} else { // assume DM_euclidean
		if (pbm.size() > cached_eucl_dist.size()) {
			for (pbt_ci it = pbm.begin(), iend = pbm.end(); it != iend; ++it) {
				// it->left  : data : int
				// it->right : data : UnOrdIntPair
				if (cached_eucl_dist.find(it->right) == cached_eucl_dist.end()) {
					size_t i = it->right.i;
					size_t j = it->right.j;
					double d = GenGeomAlgs::ComputeEucDist(c[i]->GetX(), c[i]->GetY(),
                                                           c[j]->GetX(), c[j]->GetY());
					cached_eucl_dist[it->right] = d;
				}
			}
		}
		for (pbt_ci it = pbm.begin(), iend = pbm.end(); it != iend; ++it) {
			D[it->left] = cached_eucl_dist[it->right];
		}
	}
}

const pairs_bimap_type& Project::GetSharedPairsBimap()
{
	wxLogMessage("Project::GetSharedPairsBimap()");
	if (pairs_bimap.size() == 0) {
		// generate bimap int <--> coord pair
		int n_obs = highlight_state->GetHighlight().size();
		int cnt=0;
		for (int i=0; i<n_obs; ++i) {
			for (int j=i+1; j<n_obs; ++j) {
				pairs_bimap.insert(
                    pairs_bimap_type::value_type(cnt++, UnOrdIntPair(i,j)));
			}
		}
	}
	return pairs_bimap;
}

void Project::CleanupPairsHLState()
{
	wxLogMessage("Project::CleanupPairsHLState()");
	if (pairs_hl_state) pairs_hl_state->closeAndDeleteWhenEmpty();
}

/** This should only be called by the Project constructors.  This represents
 the common part a New Project object once certain initial parts are
 initialized differently. */
bool Project::CommonProjectInit()
{	
	wxLogMessage("Project::CommonProjectInit()");
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
    if (!InitFromOgrLayer()) {
        ogr_adapter.Close();
        return false;
    }
	
	num_records = table_int->GetNumberRows();
   
    if (!isTableOnly) {
        ogr_adapter.GetHistory("db_host");
    }
    
    // convert projection to WGS84 by default if there is projection
    sourceSR = GetSpatialReference();
    if (sourceSR ) {
        project_unit = sourceSR->GetAttrValue(GdaConst::gda_projection_UNIT);
        if (project_unit.CmpNoCase(GdaConst::gda_projection_metre) == 0) {
            project_unit = GdaConst::gda_projection_meter;
        }
    }
    // configurations for save gda project file
    LayerConfiguration* layer_conf = project_conf->GetLayerConfiguration();
    CustomClassifPtree* cust_classif_ptree = layer_conf->GetCustClassifPtree();
    WeightsManPtree* spatial_weights = layer_conf->GetWeightsManPtree();
	DefaultVarsPtree* default_vars = layer_conf->GetDefaultVarsPtree();
    
	// Initialize various managers
	frames_manager = new FramesManager;
	highlight_state = new HighlightState;
	con_map_hl_state = new HighlightState;
	cat_classif_manager = new CatClassifManager(table_int, GetTableState(),
                                                cust_classif_ptree);
	highlight_state->SetSize(num_records);
	con_map_hl_state->SetSize(num_records);
    
    maplayer_state = new MapLayerState;
	w_man_state = new WeightsManState;
	w_man_int = new WeightsNewManager(w_man_state, table_int);
	save_manager = new SaveButtonManager(GetTableState(), GetWManState());
    
	if (spatial_weights) {
		((WeightsNewManager*) w_man_int)->
		Init(spatial_weights->GetWeightsMetaInfoList());
	}
    // For create Variable Selection Dialog, which has maximum 4 variables
    // to select from table
	for (int i=0; i<4; i++) {
		default_var_name[i] = "";
		default_var_time[i] = 0;
	}
	if (default_vars != NULL) {
		int i=0;
		std::vector<wxString> tm_strs;
		table_int->GetTimeStrings(tm_strs);
		std::map<wxString, int> tm_map;
		for (int t=0, sz=tm_strs.size(); t<sz; ++t) tm_map[tm_strs[t]] = t;
        bool case_sensitive = true;
		BOOST_FOREACH(const DefaultVar& dv, default_vars->GetDefaultVarList()) {
			if (!table_int->DoesNameExist(dv.name, case_sensitive)) {
				default_var_name[i] = "";
				default_var_time[i] = 0;
			} else {
				default_var_name[i] = dv.name;
				if (!dv.time_id.IsEmpty()) {
					if (tm_map.find(dv.time_id) != tm_map.end()) {
						default_var_time[i] = tm_map[dv.time_id];
					} else {
						default_var_time[i] = 0;
					}
				}
			}
            if (i < default_var_name.size()) {
                ++i;
            }
		}
	}
    
	std::vector<wxString> ts;
	table_int->GetTimeStrings(ts);
	time_state->SetTimeIds(ts);
	
	// No DB or Meta data could have changed by this point, so ensure
	// Save buttons disabled.
	save_manager->SetMetaDataSaveNeeded(false);
	save_manager->SetDbSaveNeeded(false);
	
	return true;
}

bool Project::IsDataTypeChanged()
{
    bool realTableFlag = false;
    if (datasource->GetType() == GdaConst::ds_dbf) {
        realTableFlag = true;
    } else if (layer_proxy && layer_proxy->IsTableOnly()) {
        realTableFlag = true;
    }
    return isTableOnly != realTableFlag;
}

bool Project::IsFieldCaseSensitive()
{
    return OGRLayerProxy::IsFieldCaseSensitive(layer_proxy->ds_type);
}

/** Initialize the Table and Shape Layer from OGR source */
bool Project::InitFromOgrLayer()
{
	wxLogMessage("Entering Project::InitFromOgrLayer");
    
	wxString datasource_name = datasource->GetOGRConnectStr();
    
    wxLogMessage("Datasource name:");
    wxString ds_str = datasource->ToString();
    //wxLogMessage("%s", ds_str);
    
    GdaConst::DataSourceType ds_type = datasource->GetType();
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
	// ReadLayer() is running in a seperate thread.
	// This gives us a chance to get its progress for a Progress window.
    try {
        layer_proxy = ogr_adapter.T_ReadLayer(datasource_name, ds_type, layername);
    } catch (GdaException& e) {
        // remove this datasource_proxy from cache
        ogr_adapter.RemoveDatasourceProxy(datasource_name);
        throw e;
        return false;
    }

    OGRwkbGeometryType eGType = layer_proxy->GetShapeType();
    
	if ( eGType == wkbLineString || eGType == wkbMultiLineString ) {
		open_err_msg << _("GeoDa does not support datasource with line data at this time.  Please choose a datasource with either point or polygon data.");
		throw GdaException(open_err_msg.c_str());
		return false;
	}
	
	int prog_n_max = layer_proxy->n_rows;
	
    // in case read a empty datasource or n_rows is not read
    if (prog_n_max <= 0)
        prog_n_max = 2;
   
	wxProgressDialog prog_dlg(_("Open data source progress dialog"),
							  _("Loading data..."), prog_n_max,  NULL,
							  wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL);
	bool cont = true;
	while ((layer_proxy->load_progress < layer_proxy->n_rows ||
			layer_proxy->n_rows <= 0) &&
		   !layer_proxy->HasError())
	{
		if (layer_proxy->n_rows == -1) {
			// if cannot get n_rows, make the progress stay at the half position
			cont = prog_dlg.Update(1);
    
		} else{
			if (layer_proxy->load_progress >= 0 &&
				layer_proxy->load_progress < prog_n_max ) {
				cont = prog_dlg.Update(layer_proxy->load_progress);
			}
		}
		if (!cont)  { // or if cancel clicked
			ogr_adapter.T_StopReadLayer(layer_proxy);
			return false;
		}
		wxMilliSleep(100);
	}
    
	if (!layer_proxy) {
		open_err_msg << _("There was a problem reading the layer");
		throw GdaException(open_err_msg.c_str());
        
	} else if ( layer_proxy->HasError() ) {
		open_err_msg << layer_proxy->error_message;
		throw GdaException(open_err_msg.c_str());
		
	}
    
    OGRDatasourceProxy* ds_proxy = ogr_adapter.GetDatasourceProxy(datasource_name,
                                                                  ds_type);
    // for some datasource, writable flag can only be obtained after ready layer
	datasource->UpdateWritable(ds_proxy->is_writable);
    
	// Correct variable_order information, which will be used by OGRTable
	std::vector<wxString> var_list = layer_proxy->GetFieldNames();
    std::vector<GdaConst::FieldType> var_types = layer_proxy->GetFieldTypes();
	LayerConfiguration* layer_conf = project_conf->GetLayerConfiguration();
	VarOrderPtree* variable_order = layer_conf->GetVarOrderPtree();
	variable_order->CorrectVarGroups(var_list, var_types,
                                     IsFieldCaseSensitive());
	table_state = new TableState;
	time_state = new TimeState;
	table_int = new OGRTable(layer_proxy, ds_type, table_state,
                             time_state, *variable_order);
	if (!table_int) {
		open_err_msg << _("There was a problem reading the table");
		delete table_state;
        delete time_state;
		throw GdaException(open_err_msg.c_str());
	}
	if (!table_int->IsValid()) {
		open_err_msg = table_int->GetOpenErrorMessage();
		delete table_state;
        delete time_state;
		delete table_int;
		return false;
	}
    // read cpg file for ESRI shapefile to setup encoding
    if (ds_type == GdaConst::ds_shapefile) {
        wxFileName fn(datasource_name);
        wxString cpg_fn = fn.GetPathWithSep() + fn.GetName() +  ".cpg";
        if (!wxFileExists(cpg_fn)) {
            cpg_fn = fn.GetPathWithSep() + fn.GetName() + ".CPG";
        }
        if (wxFileExists(cpg_fn)) {
            wxTextFile cpg_file;
            cpg_file.Open(cpg_fn);
            // read the first line
            cpg_encode = cpg_file.GetFirstLine();
            wxString encode_str = ConvertCpgCodePage(cpg_encode);
            SetupEncoding(encode_str);
        }
    }
	isTableOnly = layer_proxy->IsTableOnly();
    if (ds_type == GdaConst::ds_dbf) isTableOnly = true;
    if (!isTableOnly) {
        has_null_geometry = layer_proxy->ReadGeometries(main_data);
    }
	return true;
}

BackgroundMapLayer* Project::AddMapLayer(wxString datasource_name,
                                         GdaConst::DataSourceType ds_type,
                                         wxString layer_name)
{
    wxLogMessage("Project::AddMapLayer()");
    BackgroundMapLayer* map_layer = NULL;
    // Use global OGR adapter to manage all datasources, so they can be reused
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
    OGRDatasourceProxy* proxy = NULL;
    try {
        proxy = ogr_adapter.GetDatasourceProxy(datasource_name, ds_type);
    } catch (GdaException& e) {
        return NULL;
    }
    if (proxy == NULL) {
		return NULL;
	}
    OGRLayerProxy* p_layer = proxy->GetLayerProxy(layer_name);
	if (p_layer == NULL || p_layer->CheckIsTableOnly()) {
        // remove this datasource_proxy from cache
        ogr_adapter.RemoveDatasourceProxy(datasource_name);
		return NULL;
	}
    if (p_layer->ReadData()) {
        if (p_layer->IsTableOnly() == false) {
            // always add to bg_maps
            if (bg_maps.find(layer_name) == bg_maps.end()) {
                bg_maps[layer_name] = new BackgroundMapLayer(layer_name, p_layer,
                                                             sourceSR);
            }
            map_layer = bg_maps[layer_name];
        }
    }
    return map_layer;
}

int Project::GetMapLayerCount()
{
    return bg_maps.size() + fg_maps.size();
}

std::map<wxString, BackgroundMapLayer*> Project::GetBackgroundMayLayers()
{
    return bg_maps;
}

void Project::SetBackgroundMayLayers(std::map<wxString, BackgroundMapLayer*>& val)
{
    bg_maps = val;
}

std::map<wxString, BackgroundMapLayer*> Project::GetForegroundMayLayers()
{
    return fg_maps;
}

void Project::SetForegroundMayLayers(std::map<wxString, BackgroundMapLayer*>& val)
{
    fg_maps = val;
}

std::vector<BackgroundMapLayer*> Project::CloneBackgroundMaps(bool clone_style)
{
    std::vector<BackgroundMapLayer*> copy_bg_maps;
    std::map<wxString, BackgroundMapLayer*>::iterator it;
    for (it=bg_maps.begin(); it!=bg_maps.end(); it++) {
        wxString name = it->first;
        BackgroundMapLayer* ml = it->second;
        BackgroundMapLayer* copy  = ml->Clone(clone_style);
        copy_bg_maps.push_back(copy);
    }
    return copy_bg_maps;
}

std::map<wxString, BackgroundMapLayer*> Project::CloneForegroundMaps(bool clone_style)
{
    std::map<wxString, BackgroundMapLayer*> copy_fg_maps;
    std::map<wxString, BackgroundMapLayer*>::iterator it;
    for (it=fg_maps.begin(); it!=fg_maps.end(); it++) {
        wxString name = it->first;
        BackgroundMapLayer* ml = it->second;
        BackgroundMapLayer* copy  = ml->Clone(clone_style);
        copy_fg_maps[name] = copy;
    }
    return copy_fg_maps;
}

BackgroundMapLayer* Project::GetMapLayer(wxString map_name)
{
    BackgroundMapLayer* ml = NULL;
    if (bg_maps.find(map_name) != bg_maps.end()) {
        ml = bg_maps[map_name];
    } else if (fg_maps.find(map_name) != fg_maps.end()) {
        ml = fg_maps[map_name];
    }
    return ml;
}

std::vector<wxString> Project::GetLayerNames()
{
    std::vector<wxString> names;
    std::map<wxString, BackgroundMapLayer*>::iterator it;
    for (it=fg_maps.begin(); it!=fg_maps.end(); it++) {
        wxString name = it->first;
        names.push_back(name);
    }
    for (it=bg_maps.begin(); it!=bg_maps.end(); it++) {
        wxString name = it->first;
        names.push_back(name);
    }
    return names;
}

void Project::RemoveLayer(wxString name)
{
    BackgroundMapLayer* ml = NULL;
    if (bg_maps.find(name) != bg_maps.end()) {
        ml = bg_maps[name];
        ml->CleanMemory();
        delete ml;
        bg_maps.erase(name);
        
    } else if (fg_maps.find(name) != fg_maps.end()) {
        ml = fg_maps[name];
        ml->CleanMemory();
        delete ml;
        fg_maps.erase(name);
    }
}

bool Project::GetStringColumnData(wxString field_name, std::vector<wxString>& data)
{
    if (data.empty()) {
        data.resize(num_records);
    }
    // this function is for finding IDs of multi-layer
    GdaConst::FieldType type = layer_proxy->GetFieldType(field_name);
    int col_idx = layer_proxy->GetFieldPos(field_name);
    if (type == GdaConst::long64_type) {
        for (int i=0; i<num_records; ++i) {
            data[i] << layer_proxy->data[i]->GetFieldAsInteger64(col_idx);
        }
        return true;
    } else if (type == GdaConst::string_type) {
        for (int i=0; i<num_records; ++i) {
            data[i] << layer_proxy->data[i]->GetFieldAsString(col_idx);
        }
    }
    return false;
}

std::vector<wxString> Project::GetIntegerAndStringFieldNames()
{
    return layer_proxy->GetIntegerAndStringFieldNames();
}

bool Project::IsTableOnlyProject()
{
    return isTableOnly;
}

void Project::SetupEncoding(wxString encode_str)
{
	wxLogMessage("Project::SetupEncoding()");
	wxLogMessage("%s", encode_str);

    if (table_int == NULL || encode_str.IsEmpty() )
        return;
    
    if (encode_str.Upper().Contains("UTF8") ||
        encode_str.Upper().Contains("UTF 8") ||
        encode_str.Upper().Contains("UTF-8")) {
        table_int->SetEncoding(wxFONTENCODING_UTF8);
        
    } else if (encode_str.Upper().Contains("UTF16") ||
        encode_str.Upper().Contains("UTF 16") ||
        encode_str.Upper().Contains("UTF-16")) {
        table_int->SetEncoding(wxFONTENCODING_UTF16LE);
        
    } else if (encode_str.Upper().Contains("1250")) {
        table_int->SetEncoding(wxFONTENCODING_CP1250);
    } else if (encode_str.Upper().Contains("1251")) {
        table_int->SetEncoding(wxFONTENCODING_CP1251);
    } else if (encode_str.Upper().Contains("1252")) {
        table_int->SetEncoding(wxFONTENCODING_CP1252);
    } else if (encode_str.Upper().Contains("1253")) {
        table_int->SetEncoding(wxFONTENCODING_CP1253);
    } else if (encode_str.Upper().Contains("1254")) {
        table_int->SetEncoding(wxFONTENCODING_CP1254);
    } else if (encode_str.Upper().Contains("1255")) {
        table_int->SetEncoding(wxFONTENCODING_CP1255);
    } else if (encode_str.Upper().Contains("1256")) {
        table_int->SetEncoding(wxFONTENCODING_CP1256);
    } else if (encode_str.Upper().Contains("1257")) {
        table_int->SetEncoding(wxFONTENCODING_CP1257);
    } else if (encode_str.Upper().Contains("1258")) {
        table_int->SetEncoding(wxFONTENCODING_CP1258);
    } else if (encode_str.Upper().Contains("437")) {
        table_int->SetEncoding(wxFONTENCODING_CP437);
    } else if (encode_str.Upper().Contains("850")) {
        table_int->SetEncoding(wxFONTENCODING_CP850);
    } else if (encode_str.Upper().Contains("855")) {
        table_int->SetEncoding(wxFONTENCODING_CP855);
    } else if (encode_str.Upper().Contains("866")) {
        table_int->SetEncoding(wxFONTENCODING_CP866);
    } else if (encode_str.Upper().Contains("874")) {
        table_int->SetEncoding(wxFONTENCODING_CP874);
    } else if (encode_str.Upper().Contains("932")) {
        table_int->SetEncoding(wxFONTENCODING_CP932);
    } else if (encode_str.Upper().Contains("936")) {
        table_int->SetEncoding(wxFONTENCODING_CP936);
    } else if (encode_str.Upper().Contains("949")) {
        table_int->SetEncoding(wxFONTENCODING_CP949);
    } else if (encode_str.Upper().Contains("950")) {
        table_int->SetEncoding(wxFONTENCODING_CP950);
    } else if (encode_str.Upper().Contains("885910") ||
               encode_str.Upper().Contains("8859_10") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_10);
    } else if (encode_str.Upper().Contains("885911") ||
               encode_str.Upper().Contains("8859_11") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_11);
    } else if (encode_str.Upper().Contains("885912") ||
               encode_str.Upper().Contains("8859_12") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_12);
    } else if (encode_str.Upper().Contains("885913") ||
               encode_str.Upper().Contains("8859_13") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_13);
    } else if (encode_str.Upper().Contains("885914") ||
               encode_str.Upper().Contains("8859_14") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_14);
    } else if (encode_str.Upper().Contains("885915") ||
               encode_str.Upper().Contains("8859_15") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_15);
    } else if (encode_str.Upper().Contains("88591") ||
               encode_str.Upper().Contains("8859_1") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_1);
    } else if (encode_str.Upper().Contains("88592") ||
               encode_str.Upper().Contains("8859_2") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_2);
    } else if (encode_str.Upper().Contains("88593") ||
               encode_str.Upper().Contains("8859_3") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_3);
    } else if (encode_str.Upper().Contains("88594") ||
               encode_str.Upper().Contains("8859_4") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_4);
    } else if (encode_str.Upper().Contains("88595") ||
               encode_str.Upper().Contains("8859_5") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_5);
    } else if (encode_str.Upper().Contains("88596") ||
               encode_str.Upper().Contains("8859_6") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_6);
    } else if (encode_str.Upper().Contains("88597") ||
               encode_str.Upper().Contains("8859_7") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_7);
    } else if (encode_str.Upper().Contains("88598") ||
               encode_str.Upper().Contains("8859_8") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_8);
    } else if (encode_str.Upper().Contains("88599") ||
               encode_str.Upper().Contains("8859_9") ) {
        table_int->SetEncoding(wxFONTENCODING_ISO8859_9);
    } else if (encode_str.Upper().Contains("GB2312") ||
               encode_str.Upper().Contains("2312") ) {
        table_int->SetEncoding(wxFONTENCODING_GB2312);
    } else if (encode_str.Upper().Contains("BIG5")) {
        table_int->SetEncoding(wxFONTENCODING_BIG5);
    } else if (encode_str.Upper().Contains("KOI8")) {
        table_int->SetEncoding(wxFONTENCODING_KOI8);
    } else if (encode_str.Upper().Contains("SHIFT_JIS") ||
               encode_str.Upper().Contains("SHIFT JIS") ) {
        table_int->SetEncoding(wxFONTENCODING_SHIFT_JIS);
    } else if (encode_str.Upper().Contains("JP")) {
        table_int->SetEncoding(wxFONTENCODING_EUC_JP);
    } else if (encode_str.Upper().Contains("KR")) {
        table_int->SetEncoding(wxFONTENCODING_EUC_KR);
    }
}

wxString Project::ConvertCpgCodePage(const wxString& code_page)
{
    if( code_page.IsEmpty() )
        return code_page;
    
    if(code_page.StartsWith("LDID/")) {
        int nCP = -1; // windows code page.
        //http://www.autopark.ru/ASBProgrammerGuide/DBFSTRUC.HTM
        const char* pszCodePage = code_page.c_str();
        switch( atoi(pszCodePage+5) )
        {
            case 1: nCP = 437;      break;
            case 2: nCP = 850;      break;
            case 3: nCP = 1252;     break;
            case 4: nCP = 10000;    break;
            case 8: nCP = 865;      break;
            case 10: nCP = 850;     break;
            case 11: nCP = 437;     break;
            case 13: nCP = 437;     break;
            case 14: nCP = 850;     break;
            case 15: nCP = 437;     break;
            case 16: nCP = 850;     break;
            case 17: nCP = 437;     break;
            case 18: nCP = 850;     break;
            case 19: nCP = 932;     break;
            case 20: nCP = 850;     break;
            case 21: nCP = 437;     break;
            case 22: nCP = 850;     break;
            case 23: nCP = 865;     break;
            case 24: nCP = 437;     break;
            case 25: nCP = 437;     break;
            case 26: nCP = 850;     break;
            case 27: nCP = 437;     break;
            case 28: nCP = 863;     break;
            case 29: nCP = 850;     break;
            case 31: nCP = 852;     break;
            case 34: nCP = 852;     break;
            case 35: nCP = 852;     break;
            case 36: nCP = 860;     break;
            case 37: nCP = 850;     break;
            case 38: nCP = 866;     break;
            case 55: nCP = 850;     break;
            case 64: nCP = 852;     break;
            case 77: nCP = 936;     break;
            case 78: nCP = 949;     break;
            case 79: nCP = 950;     break;
            case 80: nCP = 874;     break;
            case 87: nCP = 88591;   break;
            case 88: nCP = 1252;     break;
            case 89: nCP = 1252;     break;
            case 100: nCP = 852;     break;
            case 101: nCP = 866;     break;
            case 102: nCP = 865;     break;
            case 103: nCP = 861;     break;
            case 104: nCP = 895;     break;
            case 105: nCP = 620;     break;
            case 106: nCP = 737;     break;
            case 107: nCP = 857;     break;
            case 108: nCP = 863;     break;
            case 120: nCP = 950;     break;
            case 121: nCP = 949;     break;
            case 122: nCP = 936;     break;
            case 123: nCP = 932;     break;
            case 124: nCP = 874;     break;
            case 134: nCP = 737;     break;
            case 135: nCP = 852;     break;
            case 136: nCP = 857;     break;
            case 150: nCP = 10007;   break;
            case 151: nCP = 10029;   break;
            case 200: nCP = 1250;    break;
            case 201: nCP = 1251;    break;
            case 202: nCP = 1254;    break;
            case 203: nCP = 1253;    break;
            case 204: nCP = 1257;    break;
            default: break;
        }
        wxString new_code;
        if (nCP > -1 ) new_code << nCP;
        return new_code;
    }
    
    return code_page;
}
