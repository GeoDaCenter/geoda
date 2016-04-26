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

#ifndef __GEODA_CENTER_SIMPLE_POINT_H__
#define __GEODA_CENTER_SIMPLE_POINT_H__

#include <iostream>
#include <vector>
#include "AbstractShape.h"

/**
 SimplePoint
 Corresponds to BasePoint shapes in the Shapefile.
 */
class SimplePoint :  public virtual AbstractShape  
{
	private :
	BasePoint p;
	public :
	SimplePoint()  {};
	SimplePoint(char *name) : AbstractShape(name)  {};
	virtual ~SimplePoint()  {};
	virtual Box ShapeBox() const  {  return Box(p);  };
	virtual Box SetData(int nParts, int* Part, int nPoints,
											const vector<BasePoint>& P) 
	{ Box b; return b;}
	virtual BasePoint Centroid() const { return p; }
	virtual BasePoint MeanCenter() const { return p; }
	virtual BasePoint* GetPoints() const { return NULL; }
	virtual long ContentsLength() const  { return 20; }
	virtual ostream& WriteShape(ostream &s) const
	{  WriteID(s);  return s << p << endl; }
	virtual istream& ReadShape(istream &s);
	virtual oShapeFile& WriteShape(oShapeFile &s) const;
	virtual iShapeFile& ReadShape(iShapeFile &s);
	virtual void AssignPointData(double x, double y) { p.setXY(x,y); }
};

#endif
