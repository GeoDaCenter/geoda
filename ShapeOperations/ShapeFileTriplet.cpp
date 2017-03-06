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

#include "ShapeFileTriplet.h"
#include "../GenUtils.h"

#include <wx/string.h>

// Use this when we would like to have
// the dbf file as defined in dbfdesc
oShapeFileTriplet::oShapeFileTriplet(const wxString& fname,
									 const Box& fBox,
									 DBF_descr *dbfdesc, int numfld,
									 const ShapeFileTypes::ShapeType s)
	: fn(fname),
	shp(fname, "shp"),
	shx(fname, "shx"),
	record(0),
	offset(GdaConst::ShpHeaderSize),
	dbf(fname.c_str(), dbfdesc,1,numfld), head(s)  
{

	head.SetFileBox(fBox);

	shp << head;
	shx << head;
}

// Use this when we would like to have a very simple dbf file
// it contains only POLYID in the dbf file
oShapeFileTriplet::oShapeFileTriplet(const wxString& fname,
									 const Box& fBox,
									 const wxString& nme,
									 const ShapeFileTypes::ShapeType s)
	: fn(fname),
	shp(fname, "shp"),
	shx(fname, "shx"),
	record(0),
	offset(GdaConst::ShpHeaderSize),
	dbf(fname, DBF::InitField(nme, 1),1,1), head(s)  
{
	
	head.SetFileBox(fBox);

	shp << head;
	shx << head;
}

/* This is only for creating Record Header */
oShapeFileTriplet& operator<<(oShapeFileTriplet &ot, const AbstractShape &a)  
{
	wxInt32  Contents= a.ContentsLength();
#ifdef WORDS_BIGENDIAN
	ot.shp << ++ot.record << Contents << ot.head.FileShape();
#else
	ot.shp << GenUtils::Reverse(++ot.record) << GenUtils::Reverse(Contents);
	ot.shp << ot.head.FileShape();
#endif
	a.WriteShape(ot.shp);
	
	//  create a record in the shx file
	if (ot.head.FileShape()==1)
#ifdef WORDS_BIGENDIAN
	ot.shx << ot.offset << 10L;
#else
	ot.shx << GenUtils::Reverse(ot.offset) << GenUtils::Reverse(10);
#endif
	else
#ifdef WORDS_BIGENDIAN
	ot.shx << ot.offset << Contents;
#else
	ot.shx << GenUtils::Reverse(ot.offset) << GenUtils::Reverse(Contents);
#endif
	
	//  create a record in the dbf file
	//  a.WriteDbf(ot.dbf);
	//  ot.dbf.Write(ot.record);
	
	// update internal variables
	if (ot.head.FileShape()==1) 
		ot.offset += 14;
	else
		ot.offset += Contents + 4;
	ot.head << a;
	return ot;
}

oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, char* dt)  
{
	ot.dbf.Write(dt);
	return ot;
}

oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, wxString dt)
{
	ot.dbf.Write(dt);
	return ot;
}

oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, double dt)  
{
	ot.dbf.Write(dt);
	return ot;
}

oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, long dt)  
{
	ot.dbf.Write(dt);
	return ot;
}

oShapeFileTriplet& operator<<(oShapeFileTriplet& ot, wxInt32 dt)  
{
	ot.dbf.Write(dt);
	return ot;
}

void oShapeFileTriplet::SetFileBox(const Box& fBox)
{
	head.SetFileBox(fBox);
}

void oShapeFileTriplet::CloseTriplet()
{
	shp.flush();
    shp.close();
    shx.close();
	dbf.close();
	wxString xx=fn;
	xx.Trim(false);
	xx.Trim(false);
    head.Replace(xx, record);
}

oShapeFileTriplet::~oShapeFileTriplet()  
{
}
