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

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "../logger.h"
#include "ShapeUtils.h"

ShapeUtils::RingNode::RingNode(ring_2d* s_ring_p)
	: area(0), clockwise(true), ring_p(s_ring_p) {
	if (ring_p ) {
		area = boost::geometry::area(*s_ring_p);
		if (area < 0) { area = -area; clockwise = false; }
	}
}

void ShapeUtils::polygonContentsToRingList(Shapefile::PolygonContents& pc,
										   std::list<ring_2d>& l)
{
	int pnts_cnt = 0;
	for (int i = 0; i < pc.num_parts; i++) {
		ring_2d r;
		int pmax = i == pc.num_parts-1 ? pc.num_points : pc.parts[i+1];
		for (int j = pc.parts[i]; j < pmax; j++) {
			append(r, make<point_2d>(pc.points.at(j).x,
									 pc.points.at(j).y));
		}
		l.push_back(r);
	}
}

/** Note, this method allocates memory for every RingNode.  Use
 ShapeUtils::deleteRingTree to free this memory. */
ShapeUtils::RingNode* ShapeUtils::ringListToRingTree(std::list<ring_2d>& l)
{
	RingNode* tree = new RingNode(0); // the ring tree empty root
	BOOST_FOREACH( ring_2d& i, l ) {
		insertRingNode(new RingNode(&i), tree);
	}
	return tree;
}

void ShapeUtils::polygonContentsToPolyList(Shapefile::PolygonContents& pc,
										   std::list<polygon_2d>& l)
{
	using namespace std;
	using namespace Shapefile;
	
	list<ring_2d> ring_list;
	polygonContentsToRingList(pc, ring_list);
	
	if (pc.num_parts == 1) {
		ring_2d r;
		BOOST_FOREACH( Shapefile::Point& pt, pc.points ) {
			append(r, make<point_2d>(pt.x, pt.y));
		}
		ring_list.push_back(r);
		polygon_2d p;
		assign(p, r);
		l.push_back(p);
	} else {
		RingNode* ring_tree = 0;
		ring_tree = ringListToRingTree(ring_list);
		correctRings(ring_tree);
		ringTreeToPolygons(ring_tree, l);
		deleteRingTree(ring_tree);
	}
}

void ShapeUtils::printPolyList(const std::list<polygon_2d>& l, std::ostream& s,
							   int indent)
{
	std::string pre = Shapefile::getIndentString(indent);
	std::string pre1 = Shapefile::getIndentString(indent+1);
	int i =0;
	BOOST_FOREACH( const polygon_2d& p, l ) {
		s << pre << "polygon_2d " << i << ": " << std::endl;
		s << pre1 << dsv(p) << std::endl;
		i++;
	}
}
	

point_2d ShapeUtils::centroid(const std::list<polygon_2d>& poly_list)
{
	double total_area = 0;
	double total_x = 0;
	double total_y = 0;
	point_2d cent;
	double A = 0;
	BOOST_FOREACH( const polygon_2d& p, poly_list ) {
		boost::geometry::centroid<polygon_2d, point_2d>(p, cent);
		A = area(p);
		total_x += A*cent.x();
		total_y += A*cent.y();
		total_area += A;
	}
	return make<point_2d>(total_x/total_area, total_y/total_area);
}

point_2d ShapeUtils::meanCenter(const std::list<polygon_2d>& poly_list)
{
	double total_x = 0;
	double total_y = 0;
	int total_pts = 0;
	BOOST_FOREACH( const polygon_2d& p, poly_list ) {
		BOOST_FOREACH( const point_2d& pt, p.outer() ) {
			total_x += pt.x();
			total_y += pt.y();
			total_pts++;
		}
		BOOST_FOREACH( const ring_2d& r, p.inners() ) {
			BOOST_FOREACH( const point_2d& pt, r ) {
				total_x += pt.x();
				total_y += pt.y();
				total_pts++;
			}
		}		
	}
	return make<point_2d>(total_x/total_pts, total_y/total_pts);
}



void ShapeUtils::appendChildrenToPolyList(RingNode* tree,
										  std::list<polygon_2d>& l)
{
	BOOST_FOREACH( RingNode* n, tree->children ) {
		polygon_2d p;
		assign(p, *n->ring_p);
		p.inners().resize(n->children.size());
		int i=0;
		BOOST_FOREACH( RingNode* c, n->children ) {
			// make c an inner ring of p
			assign( p.inners().at(i++), *c->ring_p);
		}
		correct(p);
		l.push_back(p);
		BOOST_FOREACH( RingNode* c, n->children ) {
			appendChildrenToPolyList( c, l );
		}
	}
}	

void ShapeUtils::ringTreeToPolygons(RingNode* tree, std::list<polygon_2d>& l)
{
	appendChildrenToPolyList(tree, l);
}

/** We actually don't need the world to be correct since we never look at the
 tree->ring_p. */
bool ShapeUtils::insertRingNode(RingNode* node, RingNode* tree)
{
	using namespace std;
	//cout << "ring " << dsv(*node->ring_p) << " being inserted." << endl;
	
	// We need to compare ourselves to each child, if there are any
	// calculate list of all children I intersect with.  Q is a list
	// of iterators for a RingNode* list.
	list<list<RingNode*>::iterator> Q;
	for (list<RingNode*>::iterator child_iter=tree->children.begin(),
		 iend=tree->children.end(); child_iter != iend; child_iter++) {
		//cout << "Ring intersects " << dsv(*(*child_iter)->ring_p) << "? ";
		if (intersects(*node->ring_p, *(*child_iter)->ring_p)) {
			Q.push_front(child_iter);
			//cout << "true" << endl;
		}
		else {
			//cout << "false" << endl;
		}
		
	}
	
	//cout << "Q.size() = " << Q.size() << endl;
	
	if (Q.empty()) {
		// I don't intersect with anybody, so I'm a peer of the current nodes
		tree->children.push_front(node);
		return true;
	} else {
		// If there is more than one item in Q, then I'm a parent of all
		// items in Q.  If there's only one item, then we have to check
		// to see if we are a child of item or not.
		// I'm either a parent of all items in Q, or I'm a child of
		// one of them.
		if (Q.size() > 1 || (*Q.front())->area < node->area ) {
			// make every item referenced in Q a child of node
			BOOST_FOREACH( list<RingNode*>::iterator ii, Q ) {
				tree->children.erase(ii);  // this is an O(1) operation
				node->children.push_front(*ii);
			}
			tree->children.push_front(node);
			return true;
		} else {
			// there can only be one item in Q.  Insert me into the subtree
			// rooted at the only item in Q.
			return insertRingNode(node, *Q.front());
		}
	}
}

void ShapeUtils::deleteRingTree(RingNode* tree)
{
	if (tree) {
		BOOST_FOREACH( RingNode* c, tree->children ) {
			ShapeUtils::deleteRingTree( c );
		}
		delete tree;
		tree = 0;
	}
}

void ShapeUtils::reverseRing(ring_2d& r)
{
	std::list<point_2d> temp;
	BOOST_FOREACH( point_2d p, r ) { temp.push_front(p); }
	r.clear();
	BOOST_FOREACH( point_2d p, temp ) { append(r, p); }
}

bool correctRingsHelper(ShapeUtils::RingNode* tree, int curr_depth,
									bool& any_changed)
{
	using namespace ShapeUtils;
	if (tree->ring_p && (curr_depth % 2) == 1 && !tree->clockwise) {
		// ring_p is of odd parity and should be corrected to clockwise
		reverseRing(*tree->ring_p);
		tree->clockwise = true;
		any_changed = true;
	} else if (tree->ring_p && (curr_depth % 2) == 0 && tree->clockwise) {
		// ring_p is of even parity and should be corrected to
		// counter-clockwise.
		reverseRing(*tree->ring_p);
		tree->clockwise = false;
		any_changed = true;
	}
	BOOST_FOREACH( ShapeUtils::RingNode* r, tree->children ) {
		correctRingsHelper(r, curr_depth+1, any_changed);
	}
	return true;
}

bool ShapeUtils::correctRings(RingNode* tree)
{
	if (!tree) return false;
	bool any_changed = false;
	correctRingsHelper(tree, 0, any_changed);
	return any_changed;
}

void printTreeHelper(ShapeUtils::RingNode* tree, int indent)
{
	using namespace ShapeUtils;
	std::cout << Shapefile::getIndentString(indent) << dsv(*(tree->ring_p));
	std::cout << ", area = " << tree->area << ", clockwise = "
	<< boolToString(tree->clockwise) << std::endl;
	BOOST_FOREACH( RingNode* r, tree->children ) {
		printTreeHelper(r, indent+1);
	}
}

void ShapeUtils::printTree(RingNode* tree)
{
	if (!tree) return;
	std::cout << "()" << std::endl;
	BOOST_FOREACH( RingNode* r, tree->children ) {
		printTreeHelper(r, 1);
	}
}

void ShapeUtils::populatePointSet(const PolygonContents& pc1,
								  Point_set& pc1_set)
{
	// we assume the pc1_set was sized correctly upon
	// construction.
	BOOST_FOREACH( const Shapefile::Point& pt, pc1.points ) {
		pc1_set.insert(pt);
	}
}

bool ShapeUtils::isQueenNeighbors(const Point_set& pc1_set,
								  const Point_set& pc2_set)
{
	if (pc1_set.size() > pc2_set.size()) {
		BOOST_FOREACH( const Shapefile::Point& pt, pc2_set ) {
			if (pc1_set.find(pt) != pc1_set.end() ) return true;
		}
	} else {
		BOOST_FOREACH( const Shapefile::Point& pt, pc1_set ) {
			if (pc2_set.find(pt) != pc2_set.end() ) return true;
		}
	}
	return false;
}

bool ShapeUtils::isQueenNeighbors(const Point_set& pc1_set,
								  const PolygonContents& pc2)
{
	BOOST_FOREACH( const Shapefile::Point& pt, pc2.points ) {
		if (pc1_set.find(pt) != pc1_set.end() ) return true;
	}
	return false;
}

bool ShapeUtils::isQueenNeighbors(const PolygonContents& pc1,
								  const PolygonContents& pc2)
{
	Point_set pc1_set(pc1.num_points);
	BOOST_FOREACH( const Shapefile::Point& pt, pc1.points ) {
		pc1_set.insert(pt);
	}
	BOOST_FOREACH( const Shapefile::Point& pt, pc2.points ) {
		if (pc1_set.find(pt) != pc1_set.end() ) return true;
	}
	return false;
}

bool ShapeUtils::isRookNeighbors(const PolygonContents& pc1,
								 const PolygonContents& pc2)
{
	using namespace std;
	Edge_set pc1_set(pc1.num_points);
	// add every edge in pc1 to pc1_set
	int pts_offset = 0;
	for (int i=0, prt_end=pc1.num_parts; i<prt_end; i++) {
		int prt_len = pc1.num_points;
		if (pc1.num_parts > 1) {
			if (i == prt_end-1) {
				prt_len = pc1.num_points - pc1.parts[i];
			} else {
				prt_len = pc1.parts[i+1] - pc1.parts[i];
			}
		}
		// note that endpoints are repeated in Shapefiles
		for (int j=0; j<prt_len-1; j++) {
			Edge e(pc1.points[pts_offset+j],
				   pc1.points[pts_offset+j+1]);
			//cout << "adding " << edgeToString(e) << endl;
			pc1_set.insert(e);
		}
		pts_offset += prt_len;
	}
	
	// check if any edge in pc2 is in pc1_set

	pts_offset = 0;
	for (int i=0, prt_end=pc2.num_parts; i<prt_end; i++) {
		int prt_len = pc2.num_points;
		if (pc2.num_parts > 1) {
			if (i == prt_end-1) {
				prt_len = pc2.num_points - pc2.parts[i];
			} else {
				prt_len = pc2.parts[i+1] - pc2.parts[i];
			}
		}
		// note that endpoints are repeated in Shapefiles
		for (int j=0; j<prt_len-1; j++) {
			Edge e(pc2.points[pts_offset+j],
				   pc2.points[pts_offset+j+1]);
			//cout << "checking " << edgeToString(e) << endl;
			if ( pc1_set.find(e) != pc1_set.end() ) return true;
		}
		pts_offset += prt_len;
	}
	
	
	return false;
}

double ShapeUtils::CalcMapWidth(Shapefile::Header header)
{
	return header.bbox_x_max - header.bbox_x_min;
}

double ShapeUtils::CalcMapHeight(Shapefile::Header header)
{
	return header.bbox_y_max - header.bbox_y_min;
}

double ShapeUtils::CalcAspectRatio(Shapefile::Header header)
{
	double map_h = CalcMapHeight(header);
	if (map_h != 0) {
		return CalcMapWidth(header) / map_h;
	}
	return 0;
}

void ShapeUtils::populatePointShpFile(const std::vector<double>& x,
									  const std::vector<double>& y,
									  Shapefile::Index& index,
									  Shapefile::Main& main)
{
	initHeader(index.header);
	initHeader(main.header);
	index.header.shape_type = Shapefile::POINT_TYP;
	main.header.shape_type = Shapefile::POINT_TYP;
	double xmin = x[0];
	double xmax = x[0];
	double ymin = y[0];
	double ymax = y[0];
	int records = x.size();
	for (int i=0; i<records; i++) {
		if (x[i] < xmin) xmin = x[i];
		if (x[i] > xmax) xmax = x[i];
		if (y[i] < ymin) ymin = y[i];
		if (y[i] > ymax) ymax = y[i];
	}
	index.header.bbox_x_min = xmin;
	index.header.bbox_y_min = ymin;
	index.header.bbox_x_max = xmax;
	index.header.bbox_y_max = ymax;
	main.header.bbox_x_min = xmin;
	main.header.bbox_y_min = ymin;
	main.header.bbox_x_max = xmax;
	main.header.bbox_y_max = ymax;
	
	index.header.file_length = 50 + records*4;
	main.header.file_length = 50 + records*14;
	
	// each main record has:
	// rec number: 4 bytes
	// content length: 4 bytes
	// shape type: 4 bytes
	// x and y doubles: 16 bytes
	// total = 28 bytes = 14 words (16-bit words)
	
	// main record content length: 16 bytes for data plus 4 bytes for
	// shape type = 20 bytes = 10 (16-bit words)
	
	index.records.clear();
	main.records.clear();
	index.records.resize(records);
	main.records.resize(records);
	for (int i=0; i<records; i++) {
		index.records[i].offset = 50 + i*14;
		index.records[i].content_length = 10;
		
		main.records[i].header.record_number = i+1;
		main.records[i].header.content_length = 10;
		Shapefile::PointContents* pc = new Shapefile::PointContents();
		main.records[i].contents_p = pc;
		pc->x = x[i];
		pc->y = y[i];
	}
}

void ShapeUtils::convertGdaPolygonsToShpFile(
						const std::vector<GdaPolygon*>& voronoi_polygons,
						const double& voronoi_bb_xmin,
						const double& voronoi_bb_ymin,
						const double& voronoi_bb_xmax,
						const double& voronoi_bb_ymax,
						Shapefile::Index& index, Shapefile::Main& main)
{
	initHeader(index.header);
	initHeader(main.header);
	index.header.shape_type = Shapefile::POLYGON;
	main.header.shape_type = Shapefile::POLYGON;
	index.header.bbox_x_min = voronoi_bb_xmin;
	index.header.bbox_y_min = voronoi_bb_ymin;
	index.header.bbox_x_max = voronoi_bb_xmax;
	index.header.bbox_y_max = voronoi_bb_ymax;
	main.header.bbox_x_min = voronoi_bb_xmin;
	main.header.bbox_y_min = voronoi_bb_ymin;
	main.header.bbox_x_max = voronoi_bb_xmax;
	main.header.bbox_y_max = voronoi_bb_ymax;
	int records = voronoi_polygons.size();
	
	index.header.file_length = 50 + records*4;

	// each main record has:
	// rec number: 4 bytes // part of polygon header
	// content length: 4 bytes // part of polygon header
	
	// shape type: 4 bytes
	// bounding box: (4*8) bytes = 32 bytes
	// variable number of bytes in content:
	// main record content length: 4 bytes for shape type plus
	// number of parts: 4 bytes
	// number of points: 4 bytes
	// parts array: num_parts*(4 bytes per part) = 4 bytes
	// points array: num_points*(16 bytes per point: double double)
	// total = 48 bytes + num_points*(16 bytes) = 24 + num_points*8 words
	
	// 16 bytes = 8 words
	
	index.records.clear();
	main.records.clear();
	index.records.resize(records);
	main.records.resize(records);
	int total_records_words = 0;
	int cur_offset = 50;
	for (int i=0; i<records; i++) {
		main.records[i].header.record_number = i+1;
		main.records[i].header.content_length = 24 + (voronoi_polygons[i]->n)*8;
		
		index.records[i].offset = cur_offset;
		index.records[i].content_length = main.records[i].header.content_length;
		
		Shapefile::PolygonContents* pc = new Shapefile::PolygonContents;
		GdaShapeAlgs::getBoundingBoxOrig(voronoi_polygons[i], pc->box[0],
										pc->box[1], pc->box[2], pc->box[3]);
		//LOG_MSG(wxString::Format("polygon %d bounding box:", i));
		//LOG_MSG(wxString::Format("    %f, %f", pc->box[0], pc->box[1]));
		//LOG_MSG(wxString::Format("    %f, %f", pc->box[2], pc->box[3]));
		pc->num_parts = 1;
		pc->num_points = voronoi_polygons[i]->n;
		pc->parts.resize(1);
		pc->parts[0] = 0;
		pc->points.resize(voronoi_polygons[i]->n);
		//LOG_MSG(wxString::Format("polygon %d points:", i));
		for (int j=0; j<voronoi_polygons[i]->n; j++) {
			pc->points[j].x = voronoi_polygons[i]->points_o[j].x;
			pc->points[j].y = voronoi_polygons[i]->points_o[j].y;
			//LOG_MSG(wxString::Format("    %f, %f",
			//						 pc->points[j].x, pc->points[j].y));
		}
		main.records[i].contents_p = pc;
		total_records_words += 4 + main.records[i].header.content_length;
		cur_offset += 4 + main.records[i].header.content_length;
	}
	main.header.file_length = 50 + total_records_words;
}

void ShapeUtils::initHeader(Shapefile::Header& header)
{
	header.file_code = 9994;
	header.file_length = 0;
	header.version = 1000;
	header.shape_type = 0;
	header.bbox_x_min = 0;
	header.bbox_y_min = 0;
	header.bbox_x_max = 0;
	header.bbox_y_max = 0;
	header.bbox_z_min = 0;
	header.bbox_z_max = 0;
	header.bbox_m_min = 0;
	header.bbox_m_max = 0;
}
