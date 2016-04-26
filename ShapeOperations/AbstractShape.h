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

#ifndef __GEODA_CENTER_ABSTRACT_SHAPE_H__
#define __GEODA_CENTER_ABSTRACT_SHAPE_H__

#include <iostream>
#include <vector>
#include "BasePoint.h"
#include "Box.h"
#include "DBF.h"
#include "ShapeFile.h"
#include "../GdaConst.h"

using namespace std;

/*
 AbstractShape
 An abstarct class for shapes. Has only Id, the most common
 feature of all shapes.
  */
class AbstractShape {
protected:
	char Id[ GdaConst::ShpObjIdLen ];
	void Assign(char* nme);
	void Identify(const long d);
	AbstractShape(char* name) { Assign(name); }
	AbstractShape(long id= 0) { Identify(id); }
	void WriteID(std::ostream &s, const long pts= 1) const;
public:
	virtual ~AbstractShape()  {};
	bool IsEmpty() { return !strcmp(Id, "0"); }
	bool IsEqual(AbstractShape& a) { return !strcmp(Id, a.Id); }
	void ReadID(std::istream& s);
	void ReadDbf(iDBF& s) {
		char st [GdaConst::ShpObjIdLen+1];
		s.Read(st, GdaConst::ShpObjIdLen);
		Assign(st);
	}
	void WriteDbf(oDBF& s) const { s.Write(Id); }
	
	virtual Box ShapeBox() const = 0;
	virtual BasePoint Centroid() const = 0;
	virtual BasePoint MeanCenter() const = 0;
	virtual void AssignPointData(double x, double y) = 0;
	virtual long ContentsLength() const = 0;
	virtual std::ostream& WriteShape(std::ostream& s) const = 0;
	virtual std::istream& ReadShape(std::istream& s) = 0;
	virtual Box SetData(int nParts, int* Part, int nPoints,
						const vector<BasePoint>& P) = 0;
	virtual oShapeFile& WriteShape(oShapeFile& s) const = 0;
	virtual iShapeFile& ReadShape(iShapeFile& s) = 0;
};

#endif
