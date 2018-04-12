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

#include <sstream>
#include <boost/functional/hash.hpp>

#include "GenUtils.h"

bool Shapefile::operator==(Point const& a, Point const& b)
{
	return a.x == b.x && a.y == b.y;
}

std::size_t Shapefile::hash_value(Point const& p)
{
	std::size_t seed=0;
	boost::hash_combine(seed, p.x);
	boost::hash_combine(seed, p.y);
	return seed;
}

bool Shapefile::operator==(Edge const& e1, Edge const& e2)
{
	return e1.a == e2.a && e1.b == e2.b;
}

std::size_t Shapefile::hash_value(Edge const& e)
{
	std::size_t seed=0;
	boost::hash_combine(seed, e.a.x);
	boost::hash_combine(seed, e.a.y);
	boost::hash_combine(seed, e.b.x);
	boost::hash_combine(seed, e.b.y);
	return seed;
}


int Shapefile::calcNumIndexHeaderRecords(const Shapefile::Header& header)
{
  // The header length is 50, 16-byte words, and header.file_length records
  // the number of 16-byte words.  So, (header.file_length - 50) is the number
  // of non-header 16-byte words.  Since each header record is 16-bytes long,
  // we divide by 4 to get the number of records. 
  return (header.file_length - 50)/4;
}

bool Shapefile::populatePointMainRecords(std::vector<MainRecord>& mr,
										 std::ifstream& file,
										 bool skip_m, bool skip_z)
{
	bool success = true;
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	int start_seek_pos = 100; // beginning of data
	file.seekg(start_seek_pos, std::ios::beg);
	int total_records = mr.size();
	int rec_num;
	for (int i=0; i<total_records && success; i++) {
		file.read((char*) integer32p, 4);
		rec_num = myINT_SWAP_ON_LE(integer32);
		if (rec_num < 1 || rec_num > total_records || 
			mr[rec_num-1].header.record_number != 0) {
			success = false;
		} else { // we have a non-duplicated, valid record number
			mr[rec_num-1].header.record_number = rec_num;
			file.read((char*) integer32p, 4);
			mr[rec_num-1].header.content_length = myINT_SWAP_ON_LE(integer32);
			
			PointContents* pc = 
				dynamic_cast<PointContents*>(mr[rec_num-1].contents_p);
			file.read((char*) integer32p, 4);
			pc->shape_type = myINT_SWAP_ON_BE(integer32);
			
			file.read((char*) float64p, 8);
			pc->x =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->y =  myDOUBLE_SWAP_ON_BE(float64);
			
			if (skip_z) file.read((char*) float64p, 8);
			if (skip_m) file.read((char*) float64p, 8);
		}
	}
	return success;
}


bool Shapefile::populatePolyLineMainRecords(std::vector<MainRecord>& mr,
											std::ifstream& file,
											bool skip_m, bool skip_z)
{
	bool success = true;
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	int start_seek_pos = 100; // beginning of data
	file.seekg(start_seek_pos, std::ios::beg);
	int total_records = mr.size();
	int rec_num;
	for (int i=0; i<total_records && success; i++) {
		file.read((char*) integer32p, 4);
		rec_num = myINT_SWAP_ON_LE(integer32);
		if (rec_num < 1 || rec_num > total_records || 
			mr[rec_num-1].header.record_number != 0) {
			success = false;
		} else { // we have a non-duplicated, valid record number
			mr[rec_num-1].header.record_number = rec_num;
			file.read((char*) integer32p, 4);
			mr[rec_num-1].header.content_length = myINT_SWAP_ON_LE(integer32);
			
			PolyLineContents* pc =
			dynamic_cast<PolyLineContents*>(mr[rec_num-1].contents_p);
			file.read((char*) integer32p, 4);
			pc->shape_type = myINT_SWAP_ON_BE(integer32);
			
			file.read((char*) float64p, 8);
			pc->box[0] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->box[1] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->box[2] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->box[3] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) integer32p, 4);
			pc->num_parts = myINT_SWAP_ON_BE(integer32);
			pc->parts.resize(pc->num_parts);
			
			file.read((char*) integer32p, 4);
			pc->num_points = myINT_SWAP_ON_BE(integer32);
			pc->points.resize(pc->num_points);
			
			for (int j=0; j < pc->num_parts; j++) {
				file.read((char*) integer32p, 4);
				pc->parts[j] = myINT_SWAP_ON_BE(integer32);
			}
			
			for (int j=0; j < pc->num_points; j++) {
				file.read((char*) float64p, 8);
				pc->points[j].x =  myDOUBLE_SWAP_ON_BE(float64);
				
				file.read((char*) float64p, 8);
				pc->points[j].y =  myDOUBLE_SWAP_ON_BE(float64);
			}
			
			if (skip_z) {
				file.read((char*) float64p, 8); // bounding z range
				file.read((char*) float64p, 8); // bounding z range
				for (int j=0; j < pc->num_points; j++) {
					file.read((char*) float64p, 8); // read z value
				}
			}
			
			if (skip_m) {
				file.read((char*) float64p, 8); // bounding m range
				file.read((char*) float64p, 8); // bounding m range
				for (int j=0; j < pc->num_points; j++) {
					file.read((char*) float64p, 8); // read m value
				}
			}
		}
	}
	return success;
}

bool Shapefile::populatePolygonMainRecords(std::vector<MainRecord>& mr,
										   std::ifstream& file,
										   bool skip_m, bool skip_z)
{
	bool success = true;
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	int start_seek_pos = 100; // beginning of data
	file.seekg(start_seek_pos, std::ios::beg);
	int total_records = mr.size();
	int rec_num;
	for (int i=0; i<total_records && success; i++) {
		file.read((char*) integer32p, 4);
		rec_num = myINT_SWAP_ON_LE(integer32);
		if (rec_num < 1 || rec_num > total_records || 
			mr[rec_num-1].header.record_number != 0) {
			success = false;
		} else { // we have a non-duplicated, valid record number
			mr[rec_num-1].header.record_number = rec_num;
			file.read((char*) integer32p, 4);
			mr[rec_num-1].header.content_length = myINT_SWAP_ON_LE(integer32);
			PolygonContents* pc = 
			dynamic_cast<PolygonContents*>(mr[rec_num-1].contents_p);
			file.read((char*) integer32p, 4);
			pc->shape_type = myINT_SWAP_ON_BE(integer32);
			
			file.read((char*) float64p, 8);
			pc->box[0] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->box[1] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->box[2] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) float64p, 8);
			pc->box[3] =  myDOUBLE_SWAP_ON_BE(float64);
			
			file.read((char*) integer32p, 4);
			pc->num_parts = myINT_SWAP_ON_BE(integer32);
			pc->parts.resize(pc->num_parts);
			
			file.read((char*) integer32p, 4);
			pc->num_points = myINT_SWAP_ON_BE(integer32);
			pc->points.resize(pc->num_points);
			
			for (int j=0; j < pc->num_parts; j++) {
				file.read((char*) integer32p, 4);
				pc->parts[j] = myINT_SWAP_ON_BE(integer32);
			}
			
			for (int j=0; j < pc->num_points; j++) {
				file.read((char*) float64p, 8);
				pc->points[j].x =  myDOUBLE_SWAP_ON_BE(float64);
				
				file.read((char*) float64p, 8);
				pc->points[j].y =  myDOUBLE_SWAP_ON_BE(float64);
			}
			
			if (skip_z) {
				file.read((char*) float64p, 8); // bounding z range
				file.read((char*) float64p, 8); // bounding z range
				for (int j=0; j < pc->num_points; j++) {
					file.read((char*) float64p, 8); // read z value
				}
			}
			
			if (skip_m) {
				file.read((char*) float64p, 8); // bounding m range
				file.read((char*) float64p, 8); // bounding m range
				for (int j=0; j < pc->num_points; j++) {
					file.read((char*) float64p, 8); // read m value
				}
			}
		}
	}
	return success;
}


bool Shapefile::writeHeader(std::ofstream& out_file,
							const Shapefile::Header& header,
							wxString& err_msg)
{
	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Problem writing shapefile header";
		return false;
	}
	
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	
	// First write out Header bytes 0 through 99
	
	// byte 0-3
	integer32 = header.file_code;
	integer32 = myINT_SWAP_ON_LE(integer32);
	out_file.write((char*) integer32p, 4);
	
	// byte 4-23 undefined, so write out 0
	integer32 = 0;
	out_file.write((char*) integer32p, 4); // bytes 4-7
	out_file.write((char*) integer32p, 4); // bytes 8-11
	out_file.write((char*) integer32p, 4); // bytes 12-15
	out_file.write((char*) integer32p, 4); // bytes 16-19
	out_file.write((char*) integer32p, 4); // bytes 20-23
	
	// byte 24-27
	integer32 = header.file_length;
	integer32 = myINT_SWAP_ON_LE(integer32);
	out_file.write((char*) integer32p, 4);
	
	// byte 28-31
	integer32 = header.version;
	integer32 = myINT_SWAP_ON_BE(integer32);
	out_file.write((char*) integer32p, 4);
	
	// byte 32-35
	integer32 = header.shape_type;
	integer32 = myINT_SWAP_ON_BE(integer32);
	out_file.write((char*) integer32p, 4);
	
	// byte 36-44
	float64 = header.bbox_x_min;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 44-51
	float64 = header.bbox_y_min;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 52-59
	float64 = header.bbox_x_max;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 60-67
	float64 = header.bbox_y_max;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 68-75
	float64 = header.bbox_z_min;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 76-83
	float64 = header.bbox_z_max;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 84-91
	float64 = header.bbox_m_min;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	// byte 92-99
	float64 = header.bbox_m_max;
	float64 = myDOUBLE_SWAP_ON_BE(float64);
	out_file.write((char*) float64p, 8);
	
	return true;
}


bool Shapefile::writePointIndexFile(const wxString& fname,
									const Index& index, wxString& err_msg)
{
	if (index.header.shape_type != Shapefile::POINT_TYP) {
		err_msg += "Not a Point Shapefile";
		return false;
	}
	
	std::ofstream out_file;
	out_file.open(GET_ENCODED_FILENAME(fname), std::ios::out | std::ios::binary);

	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Problem opening \"" + fname + "\"";
		return false;
	}
	
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	
	// First write out Header bytes 0 through 99
	if (!writeHeader(out_file, index.header, err_msg)) return false;
	
	// Write out Index, starting from byte 100
	int total_index_records = calcNumIndexHeaderRecords(index.header);
	for (int i=0; i<total_index_records; i++) {
		integer32 = index.records[i].offset;
		integer32 = myINT_SWAP_ON_LE(integer32);
		out_file.write((char*) integer32p, 4);
		
		integer32 = index.records[i].content_length;
		integer32 = myINT_SWAP_ON_LE(integer32);
		out_file.write((char*) integer32p, 4);
	}

	out_file.close();
	return true;
}

bool Shapefile::writePointMainFile(const wxString& fname,
								   const Main& main, wxString& err_msg)
{
	if (main.header.shape_type != Shapefile::POINT_TYP) {
		err_msg += "Not a Point Shapefile";
		return false;
	}
	
	std::ofstream out_file;
	out_file.open(GET_ENCODED_FILENAME(fname),std::ios::out | std::ios::binary);

	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Problem opening \"" + fname + "\"";
		return false;
	}
	
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	
	// First write out Main Header bytes 0 through 99
	if (!writeHeader(out_file, main.header, err_msg)) return false;
	
	// Write out Main Records, starting from byte 100
	int total_records = main.records.size();
	for (int i=0; i<total_records; i++) {
		integer32 = main.records[i].header.record_number;
		integer32 = myINT_SWAP_ON_LE(integer32);
		out_file.write((char*) integer32p, 4);
		
		integer32 = main.records[i].header.content_length;
		integer32 = myINT_SWAP_ON_LE(integer32);
		out_file.write((char*) integer32p, 4);
		
		PointContents* pc = (PointContents*) main.records[i].contents_p;
		integer32 = pc->shape_type;
		integer32 = myINT_SWAP_ON_BE(integer32);
		out_file.write((char*) integer32p, 4);
		float64 = pc->x;
		float64 = myDOUBLE_SWAP_ON_BE(float64);
		out_file.write((char*) float64p, 8);
		float64 = pc->y;
		float64 = myDOUBLE_SWAP_ON_BE(float64);
		out_file.write((char*) float64p, 8);
	}

	out_file.close();
	return true;
}

bool Shapefile::writePolygonIndexFile(const wxString& fname,
									  const Index& index, wxString& err_msg)
{
	if (index.header.shape_type != Shapefile::POLYGON) {
		err_msg += "Not a Polygon Shapefile";
		return false;
	}
	
	std::ofstream out_file;
	out_file.open(GET_ENCODED_FILENAME(fname),std::ios::out | std::ios::binary);

	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Problem opening \"" + fname + "\"";
		return false;
	}
	
	wxInt32 integer32;
	wxInt32* integer32p = &integer32;
	wxFloat64 float64;
	wxFloat64* float64p = &float64;
	
	// First write out Header bytes 0 through 99
	if (!writeHeader(out_file, index.header, err_msg)) return false;
	
	// Write out Index, starting from byte 100
	int total_index_records = calcNumIndexHeaderRecords(index.header);
	for (int i=0; i<total_index_records; i++) {
		integer32 = index.records[i].offset;
		integer32 = myINT_SWAP_ON_LE(integer32);
		out_file.write((char*) integer32p, 4);
		
		integer32 = index.records[i].content_length;
		integer32 = myINT_SWAP_ON_LE(integer32);
		out_file.write((char*) integer32p, 4);
	}
	
	out_file.close();
	return true;
}



std::string Shapefile::getIndentString(int indent)
{
  std::string a("");
  std::string b("");
  for (int i=0; i<spaces_per_indent; i++) a += " ";
  for (int i=0; i<indent; i++) b += a;
  return b;
}



std::string Shapefile::pointToString(const Shapefile::Point& p)
{
  std::ostringstream s;
  s << "(" << p.x << ", " << p.y << ")";
  return s.str();
}

std::string Shapefile::edgeToString(const Shapefile::Edge& e)
{
	std::ostringstream s;
	s << "{" << pointToString(e.a) << ", " << pointToString(e.b) << "}";
	return s.str();
}

std::string Shapefile::boxToString(const std::vector<wxFloat64>& box)
{
  if (box.size() < 4) return "";
  std::ostringstream s;
  s << "(" << box[0] << ", " << box[1] << ", " << box[2]
    << ", " << box[3] << ")";
  return s.str();
}



/** The following could define a run-time and relatively robust endianess test */
//bool isBigEndian() {
//  const int i = 1;
//  return (*(char*)&i) == 0;
//}

/** Alternately, to avoid a function call, could use the following macro */
const int i = 1;
#define is_bigendian() ( (*(char*)&i) == 0 )

int Shapefile::myINT_SWAP_ON_BE( int x )
{
	if is_bigendian() {
		char* c = (char*) &x;
		union {
			wxInt32 y;
			char c[4];
		} data;
		data.c[0] = c[3];
		data.c[1] = c[2];
		data.c[2] = c[1];
		data.c[3] = c[0];
		return data.y;
	}
	return x;
}

int Shapefile::myINT_SWAP_ON_LE( int x )
{
	if is_bigendian() {
		return x;
	}
	char* c = (char*) &x;
	union {
		wxInt32 y;
		char c[4];
	} data;
	data.c[0] = c[3];
	data.c[1] = c[2];
	data.c[2] = c[1];
	data.c[3] = c[0];
	return data.y;
}

wxFloat64 Shapefile::myDOUBLE_SWAP_ON_BE( wxFloat64 x )
{
  	if is_bigendian() {
		char* c = (char*) &x;
		union {
			wxFloat64 y;
			char c[8];
		} data;
		data.c[0] = c[7];
		data.c[1] = c[6];
		data.c[2] = c[5];
		data.c[3] = c[4];
		data.c[4] = c[3];
		data.c[5] = c[2];
		data.c[6] = c[1];
		data.c[7] = c[0];
		return data.y;
	}
	return x;
}
