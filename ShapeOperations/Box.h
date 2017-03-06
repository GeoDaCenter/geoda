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

#ifndef __GEODA_CENTER_BOX_H__
#define __GEODA_CENTER_BOX_H__

#include <iostream>
#include "ShapeFile.h"
#include "BasePoint.h"

/*
 Box
 Class to define bounding box in n-dimensional space. The dimension
 of space is determined by dimension of BasePoint.
  */
class Box {
public:
	BasePoint Bmin, Bmax;
	Box (const BasePoint &minP, const BasePoint &maxP)
		: Bmin(minP), Bmax(maxP) {}
	Box (const BasePoint p= 0) : Bmin(p), Bmax(p) {}
	Box (const Box& a) { Bmin = a.Bmin; Bmax = a.Bmax; }
	friend std::istream& operator>>(std::istream& s, Box& b);
	friend std::ostream& operator<<(std::ostream& s, const Box& b);
	friend iShapeFile& operator>>(iShapeFile& s, Box& b);
	friend oShapeFile& operator<<(oShapeFile& s, const Box& b);
	Box operator=(const Box& a) { Bmin= a.Bmin;	Bmax= a.Bmax; return *this; }
	Box operator+=(const Box& a) {
		Bmin= pmin(Bmin, a.Bmin);
		Bmax= pmax(Bmax, a.Bmax);
		return *this;
	}
	Box operator+=(const BasePoint& p) {
		Bmin= pmin(Bmin, p);
		Bmax= pmax(Bmax, p);
		return *this;
	}
	BasePoint _min() const { return Bmin; }
	BasePoint _max() const { return Bmax; }
	double range_x(int method) const;
	double range_y(int method) const;
};

#endif
