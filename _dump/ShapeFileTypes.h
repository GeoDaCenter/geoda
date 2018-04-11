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

#ifndef __GEODA_CENTER_SHAPE_FILE_TYPES_H__
#define __GEODA_CENTER_SHAPE_FILE_TYPES_H__

namespace ShapeFileTypes {
	enum ShapeType { // constants describing types of the Shapefiles
		NULL_SHAPE = 0,
		SPOINT      = 1, // POINT
		ARC         = 3, // POLY_LINE
		POLYGON     = 5,
		MULTIPOINT  = 8, // MULTI_POINT
		POINT_Z = 11,
		ARC_Z = 13, // POLY_LINE_Z
		POLYGON_Z = 15,
		MULTIPOINT_Z = 18, // MULTI_POINT_Z
		POINT_M = 21,
		ARC_M = 23, // POLY_LINE_M
		POLYGON_M = 25,
		MULTIPOINT_M = 28, // MULTI_POINT_MÏ
		MULTI_PATCH = 31
	};
}

#endif
