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

#ifndef __GEODA_CENTER_SHAPE_UTILS_H__
#define __GEODA_CENTER_SHAPE_UTILS_H__

#undef check // macro undefine needed for Xcode compilation with Boost.Geometry
#include <list>
#include <vector>
#include <cmath> // for double abs(doube);
#include <boost/foreach.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/ring.hpp> 
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include "../ShpFile.h"
#include "../GdaShape.h"

// note: might want to use model::multi_polygon

using namespace Shapefile;
using namespace boost::geometry;
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(boost::geometry::cs::cartesian)

typedef boost::geometry::model::d2::point_xy<double> point_2d;
typedef boost::geometry::model::ring<point_2d> ring_2d;
typedef boost::geometry::model::polygon<point_2d> polygon_2d;

namespace ShapeUtils {
	// a set of Points
	typedef boost::unordered_set<Shapefile::Point, boost::hash<Shapefile::Point> > Point_set;
	// a set of Edges
	typedef boost::unordered_set<Shapefile::Edge, boost::hash<Shapefile::Edge> > Edge_set;
	
	struct RingNode {
		double area;
		bool clockwise;
		RingNode(ring_2d* s_ring_p);
		ring_2d* ring_p;
		std::list<RingNode*> children;
	};

	void polygonContentsToRingList(Shapefile::PolygonContents& pc,
								   std::list<ring_2d>& l);
	RingNode* ringListToRingTree(std::list<ring_2d>& l);
	void polygonContentsToPolyList(Shapefile::PolygonContents& pc,
								   std::list<polygon_2d>& l);
	void printPolyList(const std::list<polygon_2d>& l, std::ostream& s,
					   int indent=0);
	point_2d centroid(const std::list<polygon_2d>& poly_list);
	point_2d meanCenter(const std::list<polygon_2d>& poly_list);
	void appendChildrenToPolyList(RingNode* tree, std::list<polygon_2d>& l);
	void ringTreeToPolygons(RingNode* tree, std::list<polygon_2d>& l);
	bool insertRingNode(RingNode* node, RingNode* tree);
	void deleteRingTree(RingNode* tree);
	void reverseRing(ring_2d& r);
	bool correctRings(RingNode* tree);
	void printTree(RingNode* tree);
	void populatePointSet(const PolygonContents& pc1,
						  Point_set& pc1_set);
	bool isQueenNeighbors(const Point_set& pc1_set,
						  const Point_set& pc2_set);
	bool isQueenNeighbors(const Point_set& pc1_set,
						  const PolygonContents& pc2);
	bool isQueenNeighbors(const PolygonContents& pc1,
						  const PolygonContents& pc2);
	bool isRookNeighbors(const PolygonContents& pc1,
						 const PolygonContents& pc2);
	double CalcMapWidth(Shapefile::Header header);
	double CalcMapHeight(Shapefile::Header header);
	double CalcAspectRatio(Shapefile::Header header);
	
	void populatePointShpFile(const std::vector<double>& x,
							  const std::vector<double>& y,
							  Shapefile::Index& index, Shapefile::Main& main);
	void convertGdaPolygonsToShpFile(
							const std::vector<GdaPolygon*>& voronoi_polygons,
							const double& voronoi_bb_xmin,
							const double& voronoi_bb_ymin,
							const double& voronoi_bb_xmax,
							const double& voronoi_bb_ymax,
							Shapefile::Index& index, Shapefile::Main& main);
	
	void initHeader(Shapefile::Header& header);
}

#endif
