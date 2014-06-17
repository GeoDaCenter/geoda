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

#ifndef __GEODA_CENTER_PROJECT_H__
#define __GEODA_CENTER_PROJECT_H__

#undef check // macro undefine needed for Xcode compilation with Boost.Geometry
//#include <boost/geometry/geometry.hpp>
//#include <boost/geometry/geometries/point_xy.hpp>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/multi_array.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/shared_ptr.hpp>
#include <wx/filename.h>
#include "DataViewer/DataSource.h"
#include "DataViewer/PtreeInterface.h"
#include "DataViewer/VarOrderPtree.h"
#include "ShapeOperations/ShpFile.h"
#include "ShapeOperations/OGRLayerProxy.h"
#include "Generic/HighlightState.h"
#include "ProjectConf.h"

typedef boost::multi_array<int, 2> i_array_type;

//using namespace boost::geometry;
class OGRTable;
class DbfTable;
class TableInterface;
class TableBase;
class CatClassifManager;
class FramesManager;
class TableState;
class TimeState;
class WeightsManager;
class SaveButtonManager;
class GalElement;
class TimeChooserDlg;
class GdaPoint;
class GdaPolygon;
class GdaShape;
class wxGrid;
class DataSource;

class Project {
public:
	Project(const wxString& proj_fname);
	Project(const wxString& project_title,
			const wxString& layername,
			IDataSource* p_datasource);
    virtual ~Project();

	bool IsValid() { return is_project_valid; }
	wxString GetOpenErrorMessage() { return open_err_msg; }
	IDataSource* GetDataSource() { return datasource; }
	wxString GetProjectTitle(); /// assumed to be primary layer name
	GdaConst::DataSourceType GetDatasourceType();
	bool IsTableOnlyProject();
    
	/** Get the current project filename with absolute path.  If project
	 file is not set, then empty string is returned. */
	wxString GetProjectFullPath();
	/** Indicate the location and name of the project file.  The filename
	 must be an absolute path.  The path will be decomposed into 
	 a new Working Directory (SetWorkingDir is automatically called) and
	 a name part.  */
	void SetProjectFullPath(const wxString& proj_full_path);
	/** To convert returned value to a wxString use, GetFullPath().  If
	 value is empty, then no directory has been set or no default could
	 be determined.  When a project file exists, the Working Directory
	 is the location of the project file. */
	wxFileName GetWorkingDir() { return working_dir; }
	/** Set the current working directory.  Directory must be an absolute
	 path.  If path refers to a file, the filename will be dropped.
	 False is returned on failure. */
	bool SetWorkingDir(const wxString& path);
	
	void SpecifyProjectConfFile(const wxString& proj_fname);
	void SaveProjectConf();
	void SaveDataSourceData();
	
    /** SaveAs in-memory Table+Geometries to OGR DataSource */
    void SaveDataSourceAs(const wxString& new_ds_name, bool is_update=false);

	int                 GetNumRecords() { return num_records; }
	HighlightState*     GetHighlightState() { return highlight_state; }
	TableInterface*     GetTableInt() { return table_int; }
	CatClassifManager*  GetCatClassifManager() { return cat_classif_manager; }
	WeightsManager*     GetWManager() { return w_manager; }
	SaveButtonManager*	GetSaveButtonManager() { return save_manager; }
	FramesManager*      GetFramesManager() { return frames_manager; }
	TableState*         GetTableState() { return table_state; }
	TimeState*          GetTimeState() { return time_state; }
	TimeChooserDlg*     GetTimeChooser() { return time_chooser; }
	TableBase*          FindTableBase();
	wxGrid*             FindTableGrid();
    ProjectConfiguration* GetProjectConf() { return project_conf; }
	OGRSpatialReference*  GetSpatialReference();

	void AddNeighborsToSelection();
	void ExportVoronoi();
	void ExportCenters(bool is_mean_centers);
	void SaveVoronoiDupsToTable();
	bool IsPointDuplicates();
	void DisplayPointDupsWarning();
	void GetVoronoiRookNeighborMap(std::vector<std::set<int> >& nbr_map);
	void GetVoronoiQueenNeighborMap(std::vector<std::set<int> >& nbr_map);
	GalElement* GetVoronoiRookNeighborGal();
	void AddMeanCenters();
	void AddCentroids();
    void GetSelectedRows(vector<int>& rowids);
	
	/// centroids by default
	bool GetCenters(std::vector<double>& x, std::vector<double>& y);
	const std::vector<GdaPoint*>& GetMeanCenters();
    void GetMeanCenters(std::vector<double>& x, std::vector<double>& y);
	const std::vector<GdaPoint*>& GetCentroids();
	void GetCentroids(std::vector<double>& x, std::vector<double>& y);
	const std::vector<GdaShape*>& GetVoronoiPolygons();
	
	// default variables
	wxString GetDefaultVarName(int var);
	void SetDefaultVarName(int var, const wxString& v_name);
	int GetDefaultVarTime(int var);
	void SetDefaultVarTime(int var, int time);
	i_array_type& GetSharedCategoryScratch() { return shared_category_scratch; }

    
public:
	/// main_data is the only public remaining attribute in Project
    Shapefile::Main main_data;
	    
private:
	bool CommonProjectInit();
	bool InitFromShapefileLayer();
	bool InitFromOgrLayer();
	bool OpenShpFile(wxFileName shp_fname);
    /** Save in-memory Table+Geometries to OGR DataSource */
    void SaveOGRDataSource();
    void UpdateProjectConf();
    Shapefile::ShapeType GetGdaGeometries(vector<GdaShape*>& geometries);
    
	// ".gda" project file data
	wxString layer_title; // optional project::layers::layer::title field
	wxString layername; // optional project::layers::layer::layername field
	wxString project_title; // optional project::title field;	
	// active project filename if exists.  Filename only, no directory
	// or ".gda" extension.  working_dir is the directory location of the
	// project file.
	wxString	proj_file_no_ext;
	wxFileName	working_dir;
	
    // XXX for multi-layer support, ProjectConfiguration is a container for
    // multi LayerConfiguration (layers), and each LayerConfiguration is defined
    // by a IDataSource to specify which data it connects to.
    // But, here we only support single layer, so we ignore LayerConfiguration
    // and use IDataSource directly.
    ProjectConfiguration *project_conf;
	IDataSource          *datasource;
	std::vector<wxString> default_var_name;
	std::vector<int>      default_var_time;
    OGRLayerProxy*        layer_proxy;

	bool    is_project_valid; // true if project Shapefile created successfully
	wxString open_err_msg; // error message for project open failure.
	bool     table_only_project; // data only, no geometry layers
	
	// without Shapefile.
    int					num_records;
	TableInterface*     table_int;
	CatClassifManager*  cat_classif_manager;
	WeightsManager*     w_manager;
	SaveButtonManager*  save_manager;
	FramesManager*      frames_manager;
	HighlightState*     highlight_state;
	TableState*         table_state;
	TimeState*          time_state;
	TimeChooserDlg*     time_chooser;
	
	// Voronoi Diagram related
	std::vector<GdaPoint*> mean_centers;
	std::vector<GdaPoint*> centroids;
	std::vector<GdaShape*> voronoi_polygons;

	bool point_duplicates_initialized;
	bool point_dups_warn_prev_displayed;
	
	std::list<std::list<int> > point_duplicates;
	GalElement* voronoi_rook_nbr_gal;
	double voronoi_bb_xmin;
	double voronoi_bb_ymin;
	double voronoi_bb_xmax;
	double voronoi_bb_ymax;
	
	/** The following array is not thread safe since it is shared by
	 every TemplateCanvas instance in a given project. */
	static i_array_type shared_category_scratch;
    
};

#endif

