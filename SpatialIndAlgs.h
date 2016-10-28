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

#ifndef __GEODA_CENTER_SPATIAL_IND_ALGS_H__
#define __GEODA_CENTER_SPATIAL_IND_ALGS_H__

#include <list>
#include <set>
#include <sstream>
#include <vector>
#include "SpatialIndTypes.h"
#include "ShpFile.h"
#include "GdaShape.h"
#include "ShapeOperations/GwtWeight.h"


namespace SpatialIndAlgs {

void get_centroids(std::vector<pt_2d>& centroids,
				   const Shapefile::Main& main_data);
void get_centroids(std::vector<pt_lonlat>& centroids,
				   const Shapefile::Main& main_data);
void to_3d_centroids(const std::vector<pt_2d>& pt2d,
										 std::vector<pt_3d>& pt3d);
void to_3d_centroids(const std::vector<pt_lonlat>& ptll,
										 std::vector<pt_3d>& pt3d);
void get_shp_bb(Shapefile::PolygonContents* p,
				double& xmin, double& ymin, double& xmax, double& ymax);
bool comp_polys(Shapefile::PolygonContents* p1, Shapefile::PolygonContents* p2,
				bool rook, double prec);
void default_test();
void print_rtree_stats(rtree_box_2d_t& rtree);
void query_all_boxes(rtree_box_2d_t& rtree);
void knn_query(const rtree_pt_2d_t& rtree, int nn=6);
/** Will call more specialized knn_build as needed.  This routine will
 build the correct type of rtree automatically.  If is_arc false,
 then Euclidean distance is used and x, y are normal coordinates and
 is_mi ignored.  If is_arc is true, then arc distances are used and distances
 reported in either kms or miles according to is_mi. */
GwtWeight* knn_build(const std::vector<double>& x,
										 const std::vector<double>& y,
										 int nn, bool is_arc, bool is_mi);
GwtWeight* knn_build(const rtree_pt_2d_t& rtree, int nn=6);
GwtWeight* knn_build(const rtree_pt_3d_t& rtree, int nn=6,
					 bool is_arc=false, bool is_mi=true);
double est_thresh_for_num_pairs(const rtree_pt_2d_t& rtree, double num_pairs);
double est_thresh_for_avg_num_neigh(const rtree_pt_2d_t& rtree, double avg_n);
double est_avg_num_neigh_thresh(const rtree_pt_2d_t& rtree, double th,
								size_t trials=100);
/** If is_arc true, result is returned as radians. If
 * (x.size()*x.size()-1)/2 <= max_iters, exact value is returned */
double est_mean_distance(const std::vector<double>& x,
						 const std::vector<double>& y,
						 bool is_arc, size_t max_iters=300000);
/** If is_arc true, result is returned as radians. If
 * (x.size()*x.size()-1)/2 <= max_iters, exact value is returned*/
double est_median_distance(const std::vector<double>& x,
						   const std::vector<double>& y,
						   bool is_arc, size_t max_iters=300000);
/** Will call more specialized thresh_build as needed.  This routine will
 build the correct type of rtree automatically.  If is_arc false,
 then Euclidean distance is used and x, y are normal coordinates and
 is_mi ignored.  If is_arc is true, then arc distances are used and distances
 reported in either kms or miles according to is_mi.  When is_arc is true,
 the threshold input parameter is assumed to be in earth arc miles or kms
 according to is_mi. */
GwtWeight* thresh_build(const std::vector<double>& x,
												const std::vector<double>& y,
												double th, bool is_arc, bool is_mi);
GwtWeight* thresh_build(const rtree_pt_2d_t& rtree, double th);
double est_avg_num_neigh_thresh(const rtree_pt_3d_t& rtree, double th,
								size_t trials=100);
/** threshold th is the radius of intersection sphere with
  respect to the unit shpere of the 3d point rtree */
GwtWeight* thresh_build(const rtree_pt_3d_t& rtree, double th, bool is_mi);
/** Find the nearest neighbor for all points and return the maximum
 distance of all of these nearest neighbor pairs.  This is the minimum
 threshold distance such that all points have at least one neighbor.
 is_mi only relevant when is_arc is true.*/
double find_max_1nn_dist(const std::vector<double>& x,
						const std::vector<double>& y,
						bool is_arc, bool is_mi);
void get_pt_rtree_stats(const rtree_pt_2d_t& rtree,
						double& min_d_1nn, double& max_d_1nn,
						double& mean_d_1nn, double& median_d_1nn);
/** results returned in radians */
void get_pt_rtree_stats(const rtree_pt_3d_t& rtree,
						double& min_d_1nn, double& max_d_1nn,
						double& mean_d_1nn, double& median_d_1nn);
GwtWeight* knn_build(const rtree_pt_lonlat_t& rtree, int nn=6);
bool write_gwt(const GwtWeight* W, const wxString& layer_name, 
			   const wxString& ofname, const wxString& vname,
			   const std::vector<wxInt64>& id_vec);
void fill_box_rtree(rtree_box_2d_t& rtree,
					const Shapefile::Main& main_data);
void fill_pt_rtree(rtree_pt_2d_t& rtree,
				   const std::vector<pt_2d>& pts);
void fill_pt_rtree(rtree_pt_lonlat_t& rtree,
				   const std::vector<pt_lonlat>& pts);
void fill_pt_rtree(rtree_pt_3d_t& rtree,
				   const std::vector<pt_3d>& pts);
struct LonLatPt {
	LonLatPt() : lon(0), lat(0) {}
	LonLatPt(double lon_, double lat_) : lon(lon_), lat(lat_) {}
	double lon;
	double lat;
};
std::ostream& operator<< (std::ostream &out, const LonLatPt& pt);
std::ostream& operator<< (std::ostream &out, const wxRealPoint& pt);

struct XyzPt {
	XyzPt() : x(0), y(0), z(0) {}
	XyzPt(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
	double x;
	double y;
	double z;
};
std::ostream& operator<< (std::ostream &out, const XyzPt& pt);

}
	
#endif

