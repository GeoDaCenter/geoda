/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

/*
 *  Notes: All Projects will have a Weights Manager and a DBF file with
 *         associated DbfGridTableBase instance.  Optionally, there can
 *         also be a Shapefile.  So, we should have two possible constructors:
 *         one that passes in a Shapefile name and DbfGridTableBase, and
 *         another that just passes in a DbfGridTableBase.  The Weights
 *         Manager will always be created by the Project instance itself.
 *
 */

#ifndef __GEODA_CENTER_PROJECT_H__
#define __GEODA_CENTER_PROJECT_H__

#undef check // macro undefine needed for Xcode compilation with Boost.Geometry
//#include <boost/geometry/geometry.hpp>
//#include <boost/geometry/geometries/point_xy.hpp>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/filename.h>
#include "ShapeOperations/ShpFile.h"
#include "Generic/HighlightState.h"

typedef boost::multi_array<int, 2> i_array_type;

//using namespace boost::geometry;

class DbfGridTableBase;
class CatClassifManager;
class FramesManager;
class TableState;
class WeightsManager;
class GalElement;
class TimeChooserDlg;
class MyPoint;
class MyPolygon;

class Project {
public:
	Project(int num_records);
	void Init(DbfGridTableBase* grid_base, wxFileName shp_fname_s);
	void Init(DbfGridTableBase* grid_base);
	virtual ~Project();
	bool IsValid() { return is_project_valid; }
	wxString GetErrorMessage() { return open_error_message; }
	Shapefile::Main main_data;
	Shapefile::Index index_data;
	//std::vector<point_2d> polyCentroids;  // Polygon/PolyLine Centroids
	//std::vector<point_2d> polyMeanCenters; // Polygon/PolyLine Mean Centers
	//std::vector< std::list<polygon_2d> > polyLists; // Polygon parts lists
	wxString GetMainDir(); // returns the path with the trailing separator
	wxString GetMainName(); // name part only, no extension
	wxString GetFullShpName();
	wxFileName shp_fname; // including full path
	HighlightState* highlight_state;
	HighlightState* GetHighlightState() { return highlight_state; }
	double GetMapAspectRatio() { return map_aspect_ratio; }
    int GetNumRecords() { return num_records; }
	DbfGridTableBase* GetGridBase() { return grid_base; }
	CatClassifManager* GetCatClassifManager() { return cat_classif_manager; }
	WeightsManager* GetWManager() { return w_manager; }
	FramesManager* GetFramesManager() { return frames_manager; }
	TableState* GetTableState() { return table_state; }
	TimeChooserDlg* GetTimeChooser() { return time_chooser; }
	void AddNeighborsToSelection();
	bool IsAllowEnableSave() { return (allow_enable_save &&
									   !shp_file_needs_first_save);
	}
	void SetAllowEnableSave(bool v) { allow_enable_save = v; }
	bool IsTableOnlyProject() { return table_only_project; }
	int GetProjectId() { return project_id; }
	i_array_type& GetSharedCategoryScratch() { return shared_category_scratch; }
	void CreateShapefileFromPoints(const std::vector<double> x,
								   const std::vector<double> y);
	void SaveVoronoiToShapefile();
	void SaveCentersToShapefile(bool mean_centers);
	bool SaveTableSpaceNew(const wxString& new_sp_dbf);
	bool SaveTableSpaceTimeNew(const wxString& new_sp_dbf,
							   const wxString& new_tm_dbf);
	void SaveVoronoiDupsToTable();
	bool IsShpFileNeedsFirstSave() { return shp_file_needs_first_save; }
	void SetShapeFileNeedsFirstSave(bool v) { shp_file_needs_first_save = v; }
	
	bool IsVoronoiPolygonsExist() { return (voronoi_polygons.size() > 0); }
	bool IsPointDuplicates();
	void DisplayPointDupsWarning();
	void GetVoronoiRookNeighborMap(std::vector<std::set<int> >& nbr_map);
	GalElement* GetVoronoiRookNeighborGal();
	
	void AddMeanCenters();
	void AddCentroids();
	
	/// centroids by default
	bool GetCenters(std::vector<double>& x, std::vector<double>& y);
	const std::vector<MyPoint*>& GetMeanCenters();
	const std::vector<MyPoint*>& GetCentroids();
	void GetMeanCenters(std::vector<double>& x, std::vector<double>& y);
	void GetCentroids(std::vector<double>& x, std::vector<double>& y);
	const std::vector<MyPolygon*>& GetVoronoiPolygons();
	
	// One-off for Regression Dialog only.
	wxWindow* regression_dlg;
	
	// default variables
	wxString default_v1_name;
	wxString default_v2_name;
	wxString default_v3_name;
	wxString default_v4_name;
	int default_v1_time;
	int default_v2_time;
	int default_v3_time;
	int default_v4_time;

private:
	bool allow_enable_save;
	bool shp_file_needs_first_save;
	bool OpenShpFile(wxFileName shp_fname_s);
	bool is_project_valid; // true if project Shapefile created successfully
	wxString open_error_message; // error message for project open failure.
	bool table_only_project; // true iff only grid_base was passed in
	// without Shapefile.
	static int next_project_id;
	int project_id;
	double map_aspect_ratio;
    int num_records;
	DbfGridTableBase* grid_base;
	CatClassifManager* cat_classif_manager;
	WeightsManager* w_manager;
	FramesManager* frames_manager;
	TableState* table_state;
	TimeChooserDlg* time_chooser;
	
	// Voronoi Diagram related
	std::vector<MyPoint*> mean_centers;
	std::vector<MyPoint*> centroids;
	
	bool point_duplicates_initialized;
	bool point_dups_warn_prev_displayed;
	std::vector<MyPolygon*> voronoi_polygons;
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

