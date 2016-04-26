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

#ifndef __GEODA_CENTER_BASE_POINT_H__
#define __GEODA_CENTER_BASE_POINT_H__

#include <iostream>
#include "ShapeFile.h"

/*
 BasePoint
 Class to define a point in two dimensional space. Has only two
 attributes: X and Y (coordinates). Supplied functionality:
 comparision (operator ==, operator !=),
 min, max,
 input (operator >> for text file and for Shapefile),
 output (operator << for text file and for ShapeFile).
  */

class BasePoint  {
public:
	double x,y;
	BasePoint(double ix= 0, double iy= 0) : x(ix), y(iy) {};
	BasePoint(const BasePoint& a) : x(a.x), y(a.y) {};
	BasePoint operator=(const BasePoint& a) { x = a.x; y = a.y;	return *this; };
	friend bool operator==(const BasePoint &a, const BasePoint &b);
	friend bool operator!=(const BasePoint &a, const BasePoint &b);
	friend BasePoint pmin(const BasePoint &a, const BasePoint &b);
	friend BasePoint pmax(const BasePoint &a, const BasePoint &b);
	friend BasePoint& operator+= (BasePoint &a, const BasePoint &b);
	friend std::istream& operator>>(std::istream &s, BasePoint &p);
	friend std::ostream& operator<<(std::ostream &s, const BasePoint &p);
	friend iShapeFile& operator>>(iShapeFile &s, BasePoint &p);
	friend oShapeFile& operator<<(oShapeFile &s, const BasePoint &p);
	void setXY(double a, double b) { x=a; y=b;};
};

#endif
