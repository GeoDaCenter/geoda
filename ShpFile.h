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

#ifndef __GEODA_CENTER_SHP_FILE_H__
#define __GEODA_CENTER_SHP_FILE_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <wx/string.h>

namespace Shapefile {
	
	/**
	 Shapefiles use only 32-bit signed integers and IEEE 64-bit
	 double-precision floating point numbers.  The wxWidgets typedef for int is
	 a signed 32-bit integer and the typedef for double is garunteed to be a
	 64-bit IEEE double.  We will therefore use these types whenever the ESRI
	 Shapefile documentation refers to Integer and Double respectively.
	 */
	
	/**
	 We use the abbreviation BE for Big Endian and LE for Little Endian byte
	 order. Values in memory are always stored in the correct byte order for
	 the computer on which the code is compiled for (ie Big Endian for Power
	 PC and Little Endian for Intel).  The annotations below are just a note
	 for how the byte order must be stored on disk.  We must therefore be
	 careful to correct the byte order when reading and writing from disk as
	 necessary.   We also note the starting byte position on disk where
	 applicable.
	 */
	struct Header {
		Header() : file_code(0), file_length(0), version(0), shape_type(0),
		bbox_x_min(0), bbox_y_min(0),
		bbox_x_max(0), bbox_y_max(0),
		bbox_z_min(0), bbox_z_max(0),
		bbox_m_min(0), bbox_m_max(0) {}
		wxInt32 file_code; // byte 0, BE, should have value 9994
		wxInt32 file_length; // byte 24, BE, the num of 16-bit words in the file
		wxInt32 version; // byte 28, LE, should have value 1000
		wxInt32 shape_type; // byte 32, one of 14 values from ShapeType enum
		wxFloat64 bbox_x_min; // byte 36, LE, bounding box for X. similar below
		wxFloat64 bbox_y_min; // byte 44, LE
		wxFloat64 bbox_x_max; // byte 52, LE
		wxFloat64 bbox_y_max; // byte 60, LE
		wxFloat64 bbox_z_min; // byte 68, LE, if unused, then has value 0.0
		wxFloat64 bbox_z_max; // byte 76, LE, if unused, then has value 0.0
		wxFloat64 bbox_m_min; // byte 84, LE, if unused, then has value 0.0
		wxFloat64 bbox_m_max; // byte 92, LE, if unused, then has value 0.0
	};
	
	enum ShapeType {
		NULL_SHAPE = 0, POINT_TYP = 1, POLY_LINE = 3, POLYGON = 5,
		MULTI_POINT = 8, POINT_Z = 11, POLY_LINE_Z = 13, POLYGON_Z = 15,
		MULTI_POINT_Z = 18, POINT_M = 21, POLY_LINE_M = 23, POLYGON_M = 25,
		MULTI_POINT_M = 28, MULTI_PATCH = 31
	};
	
	std::string shapeTypeToString(wxInt32 st);

	struct Point {
		Point() : x(0), y(0) {}
		Point(wxFloat64 x_s, wxFloat64 y_s) : x(x_s), y(y_s) {}
		wxFloat64 x;
		wxFloat64 y;
		//bool equals(Point* p){ return x==p->x && y==p->y; }
		//bool equals(Point& p){ return x==p.x && y==p.y; }
		bool equals(Point* p, double precision_threshold=0.0) {
			return (abs(x - p->x) <= precision_threshold &&
							abs(y - p->y) <= precision_threshold);
		}
		bool equals(Point& p, double precision_threshold=0.0) {
			return (abs(x - p.x) <= precision_threshold &&
							abs(y - p.y) <= precision_threshold);
		}
	};
	
	bool operator==(Point const& a, Point const& b);
	std::size_t hash_value(Point const& p);
	
	struct Edge {
		Edge() : a(0,0), b(1,1) {}
		Edge(const Point& a_s, const Point& b_s, bool ordered=false)
		: a(a_s), b(b_s)
		{
			if (!ordered && ((a_s.x > b_s.x) ||
											 (a_s.x == b_s.x && a_s.y > b_s.y ))) {
				a = b_s; b = a_s;
			}
		}
		Point a;
		Point b;
	};
	
	bool operator==(Edge const& e1, Edge const& e2);
	std::size_t hash_value(Edge const& e);
	
	struct RecordContents {
		RecordContents() : shape_type(0) {}
		virtual ~RecordContents() {}
		RecordContents(Shapefile::ShapeType st) : shape_type(st) {}
		wxInt32 shape_type; // byte 0, LE, one value from ShapeType enum
	};
	
	struct NullShapeContents : public RecordContents {
		NullShapeContents() : RecordContents(Shapefile::NULL_SHAPE) {}
		virtual ~NullShapeContents() {}
		//shape_type = 0, byte 0, LE
	};
	
	struct PointContents : public RecordContents {
		PointContents() : RecordContents(Shapefile::POINT_TYP), x(0), y(0) {}
		virtual ~PointContents() {}
		//shape_type = 1, byte 0, LE
		wxFloat64 x; // byte 4, LE
		wxFloat64 y; // byte 12, LE
	};
	
	struct PolyLineContents : public RecordContents {
		PolyLineContents() : RecordContents(Shapefile::POLY_LINE), box(4,0),
		num_parts(0), num_points(0), parts(0), points(0) {}
		virtual ~PolyLineContents() {}
		//shape_type = 3, byte 0, LE
		std::vector<wxFloat64> box; // byte 4, LE
		wxInt32 num_parts; // byte 36, LE
		wxInt32 num_points; // byte 40, LE
		std::vector<wxInt32> parts; // byte 44, array of size num_parts, LE
		// stores the first index in array for each part
		//Point points; // byte 44 + 4*num_parts, array of size num_parts, LE
		std::vector<Point> points;
	};
	
	struct PolygonContents : public RecordContents {
		PolygonContents() : RecordContents(Shapefile::POLYGON), box(4,0),
		num_parts(0), num_points(0), parts(0), points(0) {}
		virtual ~PolygonContents() {}
		//shape_type = 5, byte 0, LE
		std::vector<wxFloat64> box; // byte 4, LE
		wxInt32 num_parts; // byte 36, LE, the number of rings in the polygon
		wxInt32 num_points; // byte 40, LE, total number of points for all rings
		std::vector<wxInt32> parts; // byte 44, array of size num_parts, LE
		// stores the first index in array for each part
		std::vector<Point> points; // byte 44 + 4*num_parts, array of
		// size num_parts, LE
		
		bool intersect(PolygonContents* shp) { 
			return !(shp->box[0] > box[2] || shp->box[1] > box[3] ||
					 shp->box[2] < box[0] || shp->box[3] < box[1]);
		}
	};
	
	struct MainRecordHeader {
		MainRecordHeader() : record_number(0), content_length(0) {}
		/** Record Numbers begin at 1 and are sequential. */
		wxInt32 record_number; // byte 0, BE
		/** The number of 16-bit words in the record contents section.
		 Each record contributes (4 + content_length) 16-bit words towards the
		 total length of the file as stored at byte 24 of the Main File Header
		 (.SHP). */
		wxInt32 content_length; // byte 4, BE
	};
	
	struct MainRecord {
		MainRecord(): header(), contents_p(0) {}
		virtual ~MainRecord() { if (contents_p) delete contents_p;
			contents_p = 0; }
		MainRecordHeader header;
		RecordContents* contents_p;
	};
	
	struct Main {
		Main() : header(), records(0) {}
		virtual ~Main() {}
		Header header;
		std::vector<MainRecord> records;
	};
	
	struct IndexRecord {
		IndexRecord() : offset(0), content_length(0) {}
		/** offset of a record is the number of 16-bit words from the start of
		 the main file (.SHP) to the first byte of the record header for the
		 record.  Therefore, the offset for the first record is always 50 since
		 there are 100 bytes in the main file (and index file) header. */
		wxInt32 offset; // byte 0, BE
		/** content_length is the number of 16-bit words in the record.  It
		 starts at byte 4 from the beginning of the record and is stored with
		 Big Endian byte order. */
		wxInt32 content_length; // byte 4, BE
	};
	
	/** Contains all of the data in an Index Shapefile as described in the
	 ESRI Shapefile document. The contends of this data structure are
	 sufficient to completely create a valid .SHX file. */
	struct Index {
		Index() : header(), records(0) {}
		virtual ~Index() {}
		Header header;
		std::vector<IndexRecord> records;
	};
	
	wxFloat64 myDOUBLE_SWAP_ON_BE( wxFloat64 x );
	wxInt32 myINT_SWAP_ON_BE( int x );
	wxInt32 myINT_SWAP_ON_LE( int x );
	int calcNumIndexHeaderRecords(const Header& header);
	
	bool populateHeader(const wxString& fname, Header& header);
	bool populateIndex(const wxString& fname, Index& index_s);
	bool populateMain(const Index& index_s, const wxString& fname,
					  Main& main_s);
	
	bool writeHeader(std::ofstream& out_file,
					 const Shapefile::Header& header,
					 wxString& err_msg);
	bool writePointIndexFile(const wxString& fname, const Index& index,
							 wxString& err_msg);
	bool writePointMainFile(const wxString& fname, const Main& main,
							wxString& err_msg);
	bool writePolygonIndexFile(const wxString& fname, const Index& index,
							   wxString& err_msg);
	bool writePolygonMainFile(const wxString& fname, const Main& main,
							  wxString& err_msg);
	
	void printHeader(const Header& header, std::ostream& s, int indent=0);
	void printIndex(const Index& index_s, std::ostream& s, int indent=0);
	void printMain(const Main& main_s, std::ostream& s, int indent=0);
	void printIndexRecord(const IndexRecord& ir, std::ostream& s,
						  int indent=0);
	void printMainRecord(const MainRecord& mr, std::ostream& s,
						 int indent=0);
	void printPointContents(const PointContents& pc, std::ostream& s,
							int indent);
	void printPolyLineContents(const PolyLineContents& pc, std::ostream& s,
							   int indent);
	void printPolygonContents(const PolygonContents& pc, std::ostream& s,
							  int indent);
	void printRecordContents(const RecordContents* rc, std::ostream& s,
							 int indent);
	void printMainRecordHeader(const MainRecordHeader& mrh, std::ostream& s,
							   int indent);
	
	/** read just the shapefile type from the header */
	ShapeType readShapeType(const wxString& fname);
	
	const int spaces_per_indent = 2;
	std::string getIndentString(int indent);
	std::string pointToString(const Point& p);
	std::string edgeToString(const Edge& e);
	std::string boxToString(const std::vector<wxFloat64>& box);
	inline std::string boolToString(const bool b) {
		return b ? "true" : "false"; }
	
	/** The following are helper functions for populateMain. */
	bool populatePolygonMainRecords(std::vector<MainRecord>& mr,
									std::ifstream& file,
									bool skip_m=true, bool skip_z=true);
	bool populatePolyLineMainRecords(std::vector<MainRecord>& mr,
									 std::ifstream& file,
									 bool skip_m=true, bool skip_z=true);
	bool populatePointMainRecords(std::vector<MainRecord>& mr,
								  std::ifstream& file,
								  bool skip_m=true, bool skip_z=true);
}

#endif
