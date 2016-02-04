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
#include "ShpFile.h"
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


bool Shapefile::populateIndex(const wxString& fname,
							  Shapefile::Index& index_s)
{
  bool success = populateHeader(fname, index_s.header);
  if (!success) return false;

  std::ifstream file;
  file.open(GET_ENCODED_FILENAME(fname), std::ios::in | std::ios::binary);

  if (!(file.is_open() && file.good())) {
    return false;
  }

  int total_index_records = calcNumIndexHeaderRecords(index_s.header);
  index_s.records.resize(total_index_records);

  int start_seek_pos = 100; // beginning of data

  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;

  file.seekg(start_seek_pos, std::ios::beg);
  for (int i=0; i<total_index_records; i++) {
    file.read((char*) integer32p, 4);
    index_s.records[i].offset = myINT_SWAP_ON_LE(integer32);
    file.read((char*) integer32p, 4);
    index_s.records[i].content_length = myINT_SWAP_ON_LE(integer32);
  }

  file.close();
  return true;
}

bool Shapefile::populateHeader(const wxString& fname,
							   Shapefile::Header& header)
{
  std::ifstream file;
  file.open(GET_ENCODED_FILENAME(fname), std::ios::in | std::ios::binary);

  if (!(file.is_open() && file.good())) {
    return false;
  }
  
  wxInt32 integer32;
  wxInt32* integer32p = &integer32;
  wxFloat64 float64;
  wxFloat64* float64p = &float64;

  file.seekg(0, std::ios::beg);
  file.read((char*) integer32p, 4); // from byte 0
  header.file_code = myINT_SWAP_ON_LE(integer32);

  file.seekg(24, std::ios::beg);
  file.read((char*) integer32p, 4); // from byte 24
  header.file_length = myINT_SWAP_ON_LE(integer32);

  file.read((char*) integer32p, 4); // from byte 28
  header.version = myINT_SWAP_ON_BE(integer32);

  file.read((char*) integer32p, 4); // from byte 32
  header.shape_type = myINT_SWAP_ON_BE(integer32);

  file.read((char*) float64p, 8); // from byte 36
  header.bbox_x_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 44
  header.bbox_y_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 52
  header.bbox_x_max = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 60
  header.bbox_y_max = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 68
  header.bbox_z_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 76
  header.bbox_z_max = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 84
  header.bbox_m_min = myDOUBLE_SWAP_ON_BE(float64);

  file.read((char*) float64p, 8); // from byte 92
  header.bbox_m_max = myDOUBLE_SWAP_ON_BE(float64);

  file.close();

  return true;
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

bool Shapefile::writePolygonMainFile(const wxString& fname,
									 const Main& main, wxString& err_msg)
{
	if (main.header.shape_type != Shapefile::POLYGON) {
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
		
		PolygonContents* pc = (PolygonContents*) main.records[i].contents_p;
		
		integer32 = pc->shape_type;
		integer32 = myINT_SWAP_ON_BE(integer32);
		out_file.write((char*) integer32p, 4);
		
		float64 = pc->box[0];
		float64 = myDOUBLE_SWAP_ON_BE(float64);
		out_file.write((char*) float64p, 8);
		
		float64 = pc->box[1];
		float64 = myDOUBLE_SWAP_ON_BE(float64);
		out_file.write((char*) float64p, 8);
		
		float64 = pc->box[2];
		float64 = myDOUBLE_SWAP_ON_BE(float64);
		out_file.write((char*) float64p, 8);
		
		float64 = pc->box[3];
		float64 = myDOUBLE_SWAP_ON_BE(float64);
		out_file.write((char*) float64p, 8);
		
		integer32 = pc->num_parts;
		integer32 = myINT_SWAP_ON_BE(integer32);
		out_file.write((char*) integer32p, 4);
		
		integer32 = pc->num_points;
		integer32 = myINT_SWAP_ON_BE(integer32);
		out_file.write((char*) integer32p, 4);
		
		for (int j=0; j<pc->num_parts; j++) {
			integer32 = pc->parts[j];
			integer32 = myINT_SWAP_ON_BE(integer32);
			out_file.write((char*) integer32p, 4);	
		}
		
		for (int j=0; j<pc->num_points; j++) {
			float64 = pc->points[j].x;
			float64 = myDOUBLE_SWAP_ON_BE(float64);
			out_file.write((char*) float64p, 8);
			
			float64 = pc->points[j].y;
			float64 = myDOUBLE_SWAP_ON_BE(float64);
			out_file.write((char*) float64p, 8);
		}
	}
	
	out_file.close();
	return true;
}


std::string Shapefile::shapeTypeToString(int st)
{
  using namespace Shapefile;
  switch (st) {
  case NULL_SHAPE:
    return "NULL_SHAPE";
  case POINT_TYP:
    return "POINT_TYP";
  case POLY_LINE:
    return "POLY_LINE";
  case POLYGON:
    return "POLYGON";
  case MULTI_POINT:
    return "MULTI_POINT";
  case POINT_Z:
    return "POINT_Z";
  case POLY_LINE_Z:
    return "POLY_LINE_Z";
  case POLYGON_Z:
    return "POLYGON_Z";
  case MULTI_POINT_Z:
    return "MULTI_POINT_Z";
  case POINT_M:
    return "POINT_M";
  case POLY_LINE_M:
    return "POLY_LINE_M";
  case POLYGON_M:
    return "POLYGON_M";
  case MULTI_POINT_M:
    return "MULTI_POINT_M";
  case MULTI_PATCH:
    return "MULTI_PATCH";
  default :
    return "";
  }
}

std::string Shapefile::getIndentString(int indent)
{
  std::string a("");
  std::string b("");
  for (int i=0; i<spaces_per_indent; i++) a += " ";
  for (int i=0; i<indent; i++) b += a;
  return b;
}

void Shapefile::printHeader(const Shapefile::Header& header, std::ostream& s,
			    int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "file_code: " << header.file_code << std::endl;
  s << pre << "file_length: " << header.file_length
    << " (16-bit words)" << std::endl;
  s << pre << "file_length: " << header.file_length/2
    << " (32-bit words)" << std::endl;
  s << pre << "version: " << header.version << std::endl;
  s << pre << "shape_type: "
    << Shapefile::shapeTypeToString(header.shape_type) << std::endl;
  s << pre << "bbox_x_min: " << header.bbox_x_min << std::endl;
  s << pre << "bbox_y_min: " << header.bbox_y_min << std::endl;
  s << pre << "bbox_x_max: " << header.bbox_x_max << std::endl;
  s << pre << "bbox_y_max: " << header.bbox_y_max << std::endl;
  s << pre << "bbox_z_min: " << header.bbox_z_min << std::endl;
  s << pre << "bbox_z_max: " << header.bbox_z_max << std::endl;
  s << pre << "bbox_m_min: " << header.bbox_m_min << std::endl;
  s << pre << "bbox_m_max: " << header.bbox_m_max << std::endl;
}

void Shapefile::printIndexRecord(const Shapefile::IndexRecord& ir,
				 std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "offset: " << ir.offset << std::endl;
  s << pre << "content_length: " << ir.content_length << std::endl;
}

void Shapefile::printMainRecord(const Shapefile::MainRecord& mr,
								std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "header:" << std::endl;
  printMainRecordHeader(mr.header, s, indent+1);
  s << pre << "contents_p:" << std::endl;
  printRecordContents(mr.contents_p, s, indent+1);
}

void Shapefile::printMainRecordHeader(const Shapefile::MainRecordHeader& mrh,
				      std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "record_number: " << mrh.record_number << std::endl;
  s << pre << "content_length: " << mrh.content_length << std::endl;
}

/** read just the shapefile type from the header */
Shapefile::ShapeType Shapefile::readShapeType(const wxString& fname)
{
	Header hdr;
	bool success = populateHeader(fname, hdr);
	ShapeType st = static_cast<ShapeType>(hdr.shape_type);
	return st;
}

void Shapefile::printRecordContents(const Shapefile::RecordContents* rc,
				    std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  if (const PointContents* p = dynamic_cast<const PointContents*>(rc)) {
    printPointContents(*p, s, indent);
  } else if (const PolyLineContents* p =
			 dynamic_cast<const PolyLineContents*>(rc)) {
    printPolyLineContents(*p, s, indent);
  } else if (const PolygonContents* p =
			 dynamic_cast<const PolygonContents*>(rc)) {
    printPolygonContents(*p, s, indent);
  }
}

void Shapefile::printPolygonContents(const Shapefile::PolygonContents& pc,
				     std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "shape_type: " << shapeTypeToString(pc.shape_type) << std::endl;
  s << pre << "box: " << boxToString(pc.box) << std::endl;
  s << pre << "num_parts: " << pc.num_parts << std::endl;
  s << pre << "num_points: " << pc.num_points << std::endl;
  s << pre << "parts(" << pc.num_parts << "): " << std::endl;
  pre = getIndentString(indent+1);
  for (int i=0; i < pc.num_parts; i++) {
    s << pre << "parts[" << i << "]: " << pc.parts.at(i) << std::endl;
  }
  pre = getIndentString(indent);
  s << pre << "points(" << pc.num_points << "): " << std::endl;
  pre = getIndentString(indent+1);
  for (int i=0; i < pc.num_points; i++) {
    s << pre << "points[" << i << "]: "
      << pointToString(pc.points.at(i)) << std::endl;
  }
}

void Shapefile::printPolyLineContents(const Shapefile::PolyLineContents& pc,
				      std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "shape_type: " << shapeTypeToString(pc.shape_type) << std::endl;
}

void Shapefile::printPointContents(const Shapefile::PointContents& pc,
				   std::ostream& s, int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "shape_type: " << shapeTypeToString(pc.shape_type) << std::endl;
  s << pre << "(x,y): " << pc.x << ", " << pc.y << ")" << std::endl;
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

void Shapefile::printIndex(const Shapefile::Index& index_s, std::ostream& s,
			   int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "Header:" << std::endl;
  Shapefile::printHeader(index_s.header, s, indent+1);
  s << pre << "IndexRecord(" << index_s.records.size() << "):" << std::endl;
  for ( int i=0, iend=index_s.records.size(); i<iend; i++) {
    s << getIndentString(1+indent) << "records[" << i << "]:" << std::endl;
    Shapefile::printIndexRecord(index_s.records[i], s, indent+2);
  }
}

void Shapefile::printMain(const Shapefile::Main& main_s, std::ostream& s,
			  int indent)
{
  std::string pre = getIndentString(indent);
  s << pre << "Header:" << std::endl;
  Shapefile::printHeader(main_s.header, s, indent+1);
  s << pre << "MainRecord(" << main_s.records.size() << "):" << std::endl;
  for ( int i=0, iend=main_s.records.size(); i<iend; i++) {
    s << getIndentString(1+indent) << "records[" << i << "]:" << std::endl;
    Shapefile::printMainRecord(main_s.records[i], s, indent+2);
  }
}

bool Shapefile::populateMain(const Index& index_s, const wxString& fname,
							 Main& main_s)
{
	using namespace Shapefile;
	bool success = populateHeader(fname, main_s.header);
	if (!success) return false;
	
	std::ifstream file;
	file.open(GET_ENCODED_FILENAME(fname),std::ios::in | std::ios::binary);
	
	if (!(file.is_open() && file.good())) {
		return false;
	}
	
	bool skip_m = false;
	bool skip_z = false;
	
	if (main_s.header.shape_type == POLYGON_Z) {
		skip_m = true;
		skip_z = true;
		main_s.header.shape_type = POLYGON;
	} else if (main_s.header.shape_type == POLYGON_M) {
		skip_m = true;
		main_s.header.shape_type = POLYGON;
	} else if (main_s.header.shape_type == POINT_Z) {
		skip_m = true;
		skip_z = true;
		main_s.header.shape_type = POINT_TYP;
	} else if (main_s.header.shape_type == POINT_M ||
			   main_s.header.shape_type == MULTI_POINT) {
		skip_m = true;
		main_s.header.shape_type = POINT_TYP;
	} else if (main_s.header.shape_type == POLY_LINE_Z) {
		skip_m = true;
		skip_z = true;
		main_s.header.shape_type = POLY_LINE;
	} else if (main_s.header.shape_type == POLY_LINE_M) {
		skip_m = true;
		main_s.header.shape_type = POLY_LINE;
	}
	
	if ( main_s.header.shape_type == POINT_TYP ||
		main_s.header.shape_type == POLY_LINE ||
		main_s.header.shape_type == POLYGON ) {
		
		// Allocate memory as needed and put all records in their proper sorted
		// order.
		main_s.records.resize(index_s.records.size());
		if (main_s.header.shape_type == POINT_TYP) {
			for (int i=0, iend=main_s.records.size(); i<iend; i++) {
				main_s.records[i].contents_p = new PointContents();
			}
			populatePointMainRecords(main_s.records, file, skip_m, skip_z);
		} else if ( main_s.header.shape_type == POLY_LINE ) {
			for (int i=0, iend=main_s.records.size(); i<iend; i++) {
				main_s.records[i].contents_p = new PolyLineContents();
			}
			populatePolyLineMainRecords(main_s.records, file, skip_m, skip_z);      
		} else if ( main_s.header.shape_type == POLYGON ) {
			for (int i=0, iend=main_s.records.size(); i<iend; i++) {
				main_s.records[i].contents_p = new PolygonContents();
			}
			populatePolygonMainRecords(main_s.records, file, skip_m, skip_z);
		}
		
	} else {
		success = false;
	}
	
	file.close();
	return success;
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
