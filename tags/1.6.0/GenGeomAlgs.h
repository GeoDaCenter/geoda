/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

/*
 * Fhe following functions: findArea and ComputeArea2D
 *     are borrowed from FastArea.c++
 *
 * From the paper:
 *
 *      Daniel Sunday
 *      "Fast Polygon Area and Newell Normal Computation"
 *      journal of graphics tools, 7(2):9-13, 2002
 *
 */

#ifndef __GEODA_CENTER_GEN_GEOM_ALGS_H__
#define __GEODA_CENTER_GEN_GEOM_ALGS_H__

namespace GenGeomAlgs {
	double ComputeEucDist(double x1, double y1, double x2, double y2);
	double ComputeArcDist(double lat1, double long1, double lat2, double long2);
	double ComputePerimeter2D(int n, double *x, double *y);
	double ComputeArea2D(int n, double *x, double *y);    // output unit normal
	double findArea(int n, double *x, double *y);         // 2D polygon
	bool ClipToBB(double& x0, double& y0, double& x1, double& y1,
				  const double& xmin, const double& ymin,
				  const double& xmax, const double& ymax);
}

#endif
