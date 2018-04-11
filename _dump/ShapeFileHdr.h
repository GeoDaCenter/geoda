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

#ifndef __GEODA_CENTER_SHAPE_FILE_HDR_H__
#define __GEODA_CENTER_SHAPE_FILE_HDR_H__

#include "AbstractShape.h"
#include "Box.h"
#include "ShapeFile.h"
#include "ShapeFileTypes.h"

/*
 ShapeFileHdr
 Header Data structure that corresponds to the first record of
 all *.SHP files.
  */
class ShapeFileHdr  {
private:
    wxInt32 FileCode, Version, fShape, FileLength;
    Box FileBox;
    typedef struct {
        wxInt32    f[9];
        Box     b;
        wxInt32    s[8];
    } HdrRecord;
    typedef struct {
        int f[9];
        Box b;
        int s[8];
    } HdrRecord64;
public:
	enum  HdrEnum  {
		kVersion = 1000,
		kFileCode = 9994
	};
	ShapeFileHdr(const ShapeFileTypes::ShapeType FileShape=ShapeFileTypes::SPOINT);
	ShapeFileHdr(const char* s);
	wxInt32 FileShape() const { return fShape; }
	wxInt32 Length() const { return FileLength; }
	Box BoundingBox() const { return FileBox; }
	void MakeBuffer(char *s) const;
	void SetFileBox(const Box& fBox);
	void SetFileLength(wxInt32 flength);
	friend ShapeFileHdr& operator<<(ShapeFileHdr& hd, const AbstractShape& s);
	friend oShapeFile& operator<<(oShapeFile& s, const ShapeFileHdr& hd);
	void Replace(const wxString& fname, const wxInt32& recs);
	wxInt32 getFileCode() const { return FileCode; }
	wxInt32 getVersion() const { return Version; }
};

#endif
