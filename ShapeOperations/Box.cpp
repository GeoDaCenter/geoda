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

#include "Box.h"
#include "../GenGeomAlgs.h"

double Box::range_x(int method) const
{
	if (method ==2) {
		double range = GenGeomAlgs::ComputeArcDistMi(Bmin.x,Bmin.y,Bmax.x,Bmin.y);
		return range;
	} else {
		return Bmax.x - Bmin.x;
	}
}
double Box::range_y(int method) const
{  
	if (method ==2) {
		double range = GenGeomAlgs::ComputeArcDistMi(Bmin.x,Bmin.y,Bmin.x,Bmax.y);
		return range;
	} else {
		return Bmax.y - Bmin.y;
	}
}

/*
 Operators on Box
  */

std::istream& operator>>(std::istream &s, Box &b)
{ return s >> b.Bmin >> b.Bmax; }

std::ostream& operator<<(std::ostream &s, const Box &b)
{ return s << b.Bmin << ' ' << b.Bmax;  }

iShapeFile& operator>>(iShapeFile &s, Box &b)
{ return s >> b.Bmin >> b.Bmax; }

oShapeFile& operator<<(oShapeFile &s, const Box &b)
{  return s << b.Bmin << b.Bmax; }

