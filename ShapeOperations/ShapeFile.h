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

#ifndef __GEODA_CENTER_SHAPE_FILE_H__
#define __GEODA_CENTER_SHAPE_FILE_H__

#include <fstream>
#include <wx/string.h>

/*
 iShapeFile
 Class to define Shapefile objects (*.SHP and *.SHX) that are
 availbable for input only.
 Recl should be called before reading every shape from *.SHP.
  */
class iShapeFile : public std::ifstream {
private:
	long record;                         // contains record number
public:
	iShapeFile(const wxString& fname, const wxString& ext);
	virtual ~iShapeFile();
	long Record() const { return record; }
	long Recl(const long& shape);    // returns length of the record
	iShapeFile& operator>>(long &v)  // reads 4 byte integer // MMM: big assumption!
    { read((char*) &v, 4); return *this; }
	iShapeFile& operator>>(wxInt32 &v)  // reads 4 byte integer // MMM: big assumption!
    { read((char*) &v, 4); return *this; }
	iShapeFile& operator>>(double &v)    // reads 8 byte float point
    { read((char*) &v, 8); return *this; }
};

/*
 oShapeFile
 Class to define ShapeFile objects (*.SHP and *.SHX) that are
 available for output only.
  */
class oShapeFile : public std::ofstream {
public:
	oShapeFile(const wxString& fname, const wxString& ext);
	virtual ~oShapeFile();
	oShapeFile& operator<<(const wxInt32& v)   // write 4 byte integer
    { write((char*) &v, 4); return *this; }
	oShapeFile& operator<<(const long& v)   // write 4 byte integer
    { write((char*) &v, 4); return *this; }
	oShapeFile& operator<<(const double& v) // writes 8 byte double
    { write((char*) &v, 8); return *this; }
};

#endif

