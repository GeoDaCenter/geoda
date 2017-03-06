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
#include "ShpFile.h"
#include "ShapeOperations/OGRLayerProxy.h"
#include "SpatialIndTypes.h"
#include "HighlightState.h"
#include "Explore/DistancesCalc.h"
#include "VarCalc/WeightsMetaInfo.h"
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
class WeightsManInterface;
class WeightsManState;
class SaveButtonManager;
class GalElement;
class TimeChooserDlg;
class GdaPoint;
class GdaPolygon;
class GdaShape;
class wxGrid;
class DataSource;
class CovSpHLStateProxy;

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
	bool isTableOnly; // variable data only, no geometry layers
    bool IsDataTypeChanged();
    
    bool IsFileDataSource();
    
    bool HasUnsavedChange();
    
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
	
    void UpdateProjectConf(ProjectConfiguration* project_conf);
	void SpecifyProjectConfFile(const wxString& proj_fname);
	void SaveProjectConf();
	void SaveDataSourceData();
	
    /** SaveAs in-memory Table+Geometries to OGR DataSource */
    void SaveDataSourceAs(const wxString& new_ds_name, bool is_update=false);

	int                 GetNumRecords() { return num_records; }
	HighlightState*     GetHighlightState() { return highlight_state; }
	HighlightState*     GetConMapHlightState() { return con_map_hl_state; }
	CovSpHLStateProxy*  GetPairsHLState();
	TableInterface*     GetTableInt() { return table_int; }
	CatClassifManager*  GetCatClassifManager() { return cat_classif_manager; }
	WeightsManInterface* GetWManInt() { return w_man_int; }
	WeightsManState*	GetWManState() { return w_man_state; }
	SaveButtonManager*	GetSaveButtonManager() { return save_manager; }
	FramesManager*      GetFramesManager() { return frames_manager; }
	TableState*         GetTableState() { return table_state; }
	TimeState*          GetTimeState() { return time_state; }
	TimeChooserDlg*     GetTimeChooser() { return time_chooser; }
	TableBase*          FindTableBase();
	wxGrid*             FindTableGrid();
    ProjectConfiguration* GetProjectConf() { return project_conf; }
	OGRSpatialReference*  GetSpatialReference();

	void AddNeighborsToSelection(boost::uuids::uuid weights_id);
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
	const std::vector<GdaPoint*>& GetMeanCenters();
	void GetMeanCenters(std::vector<double>& x, std::vector<double>& y);
	const std::vector<GdaPoint*>& GetCentroids();
	void GetCentroids(std::vector<double>& x, std::vector<double>& y);
	void GetCentroids(std::vector<wxRealPoint>& pts);
	const std::vector<GdaShape*>& GetVoronoiPolygons();
	
	double GetMin1nnDistEuc();
	double GetMax1nnDistEuc();
	double GetMaxDistEuc(); // diameter of convex hull
	double GetMin1nnDistArc(); // returned as radians
	double GetMax1nnDistArc(); // returned as radians
	double GetMaxDistArc(); // returned as radians
	
	rtree_pt_2d_t& GetEucPlaneRtree();
	rtree_pt_3d_t& GetUnitSphereRtree();
	
	// default variables
	wxString GetDefaultVarName(int var);
	void SetDefaultVarName(int var, const wxString& v_name);
	int GetDefaultVarTime(int var);
	void SetDefaultVarTime(int var, int time);
	
	WeightsMetaInfo::DistanceMetricEnum GetDefaultDistMetric();
	void SetDefaultDistMetric(WeightsMetaInfo::DistanceMetricEnum dm);
	WeightsMetaInfo::DistanceUnitsEnum GetDefaultDistUnits();
	void SetDefaultDistUnits(WeightsMetaInfo::DistanceUnitsEnum du);
	
	// Fill Distances according to order specified in shared project
	// pairs order mapping.
	void FillDistances(std::vector<double>& D,
                       WeightsMetaInfo::DistanceMetricEnum dm,
                       WeightsMetaInfo::DistanceUnitsEnum du);
	
	const pairs_bimap_type& GetSharedPairsBimap();
	void CleanupPairsHLState();
	
	i_array_type* GetSharedCategoryScratch(int num_cats, int num_obs);
    
	/** NOTE: This function needs a better home. */
	static bool CanModifyGrpAndShowMsgIfNot(TableState* table_state,
                                            const wxString& grp_nm);
	
public:
	/// main_data is the only public remaining attribute in Project
	Shapefile::Main main_data;
    OGRSpatialReference* sourceSR;
    wxString project_unit;
	
	// ".gda" project file data
	wxString layer_title; // optional project::layers::layer::title field
	wxString layername; // optional project::layers::layer::layername field
	wxString project_title; // optional project::title field;
    
	// active project filename if exists.  Filename only, no directory
	// or ".gda" extension.  working_dir is the directory location of the
	// project file.
	wxString	proj_file_no_ext;
	wxFileName	working_dir;
	    
private:
	bool CommonProjectInit();
	bool InitFromOgrLayer();
   
    // only for ESRI Shapefile .cpg file
    void SetupEncoding(wxString encode_str);
    
	/** Save in-memory Table+Geometries to OGR DataSource */
	void SaveOGRDataSource();
	void UpdateProjectConf();
	Shapefile::ShapeType GetGdaGeometries(vector<GdaShape*>& geometries);
	void CalcEucPlaneRtreeStats();
	void CalcUnitSphereRtreeStats();
    
	
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

	bool is_project_valid; // true if project Shapefile created successfully
	wxString open_err_msg; // error message for project open failure.
	
	// without Shapefile.
	int					num_records;
	TableInterface*     table_int;
	CatClassifManager*  cat_classif_manager;
	WeightsManInterface* w_man_int;
	WeightsManState*    w_man_state;
	SaveButtonManager*  save_manager;
	FramesManager*      frames_manager;
	HighlightState*     highlight_state;
	HighlightState*     con_map_hl_state;
	CovSpHLStateProxy*  pairs_hl_state;
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
	
	// Rtree Related
	double min_1nn_dist_euc;
	double max_1nn_dist_euc;
	double max_dist_euc;
	double min_1nn_dist_arc; // radians
	double max_1nn_dist_arc; // radians
	double max_dist_arc; // radians
	rtree_pt_2d_t rtree_2d; // 2d Cartesian points
	rtree_pt_3d_t rtree_3d; // lon/lat points projected to unit sphere
	
	/** The following array is not thread safe since it is shared by
	 every TemplateCanvas instance in a given project. */
	static std::map<wxString, i_array_type*> shared_category_scratch;
	
	WeightsMetaInfo::DistanceMetricEnum dist_metric;
	WeightsMetaInfo::DistanceUnitsEnum dist_units;
	
	pairs_bimap_type pairs_bimap;
    
	dist_map_type cached_eucl_dist;
	dist_map_type cached_arc_dist;
    
};

#endif

