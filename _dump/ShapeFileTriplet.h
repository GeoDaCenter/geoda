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

#ifndef __GEODA_CENTER_SHAPE_FILE_TRIPLET_H__
#define __GEODA_CENTER_SHAPE_FILE_TRIPLET_H__

#include "AbstractShape.h"
#include "Box.h"
#include "DBF.h"
#include "ShapeFile.h"
#include "ShapeFileTypes.h"
#include "ShapeFileHdr.h"

/*
 oShapeFileTriplet
 This corresponds to a concept of Shapefile (SHP,SHX,DBF files
 with the same names), which is being created.
 This is the best way to create a Shapefile (to make sure that all its
 all of its components correspond to each other).
 On the opening the "empty" headers are created, then add shapes
 to the Shapefile using << (as many as needed)
 and on the closing, the headers are updated according to
 actual number of shapes placed in the files, actual sizes of the
 files, their bounding boxes and so on.
  */
class oShapeFileTriplet {
private:
	wxString fn;
public:
	long record, offset;
	oDBF dbf;
	oShapeFile shp, shx;
	ShapeFileHdr head;
	oShapeFileTriplet(const wxString& fname, const Box& xoBox,
					  const wxString& nme,
					  const ShapeFileTypes::ShapeType s=ShapeFileTypes::SPOINT);
	oShapeFileTriplet(const wxString& fname,
					  const Box& xoBox, DBF_descr *dbfdesc, int numfld,
					  const ShapeFileTypes::ShapeType s=ShapeFileTypes::SPOINT);
	virtual ~oShapeFileTriplet();
	friend oShapeFileTriplet& operator<<(oShapeFileTriplet& ot,
										 const AbstractShape& a);
	friend oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, double dt);
	friend oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, long dt);
	friend oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, wxInt32 dt);
	friend oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, char* dt);
	friend oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, wxString dt);
	void SetFileBox(const Box& fBox);
	void CloseTriplet();
};

#endif
