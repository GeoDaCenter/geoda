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

#include <math.h>
#include "GenGeomAlgs.h"

double GenGeomAlgs::ComputeEucDist(double x1, double y1, double x2, double y2) 
{
	return sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}

/*
 Notes on ComputeArcDist:
 In the equation below, the 69.11 factor is actually the distance,
 in miles, between each degree of latitude for the WGS
 84 sphere. Remember that this equation is just an approximation
 because the earth is actually an ellipsoid. Because of
 this, the distance between latitudes will increase as the latitude
 increases. The distance at 0∞ on the WGS 84 ellipsoid is
 actually 68.71 miles while
 it is 69.40 miles at 90∞.
 */

double GenGeomAlgs::ComputeArcDist(double long1, double lat1,
								   double long2, double lat2)
{
	//const double pi = 3.141592653589;
	const double R_earth_mi = 6371.0 / 1.609344;
	const double rad = 0.017453292519938; // rad = pi/180.0
	double rlat1 = (90.0 - lat1) *rad;
	double rlat2 = (90.0 - lat2) *rad;
	double rlong = (long2-long1) *rad;
	double drad = cos(rlong)*sin(rlat1)*sin(rlat2) + cos(rlat1)*cos(rlat2);
	// 6371.0 / 1.609344 = approx radius of earth in miles
	// 6371 is Earth radius in km and the conversion to (Statue) miles is
	// given by 1 mi = 1.609344 km
	double dist = acos(drad) * R_earth_mi;
	return dist;  // in miles.
}

/*
 * Fhe following four functions: findArea, ComputeArea2D,
 *     and ComputePerimeter2D are borrowed from FastArea.c++
 *
 * From the paper:
 *
 *      Daniel Sunday
 *      "Fast Polygon Area and Newell Normal Computation"
 *      journal of graphics tools, 7(2):9-13, 2002
 *
 */

// assume vertex coordinates are in arrays x[], y[], and z[]
// with room to duplicate the first two vertices at the end

// return the signed area of a 2D polygon
double GenGeomAlgs::findArea(int n, double *x, double *y) // 2D polygon
{
	// guarantee the first two vertices are also at array end
	x[n] = x[0];
	y[n] = y[0];
	x[n+1] = x[1];
	y[n+1] = y[1];
	
	double sum = 0.0;
	double *xptr = x+1, *ylow = y, *yhigh = y+2;
	for (int i=1; i <= n; i++) {
		sum += (*xptr++) * ( (*yhigh++) - (*ylow++) );
	}
	return (sum / 2.0);
}

// output unit normal
double GenGeomAlgs::ComputeArea2D(int n, double *x, double *y)
{
	// get the Newell normal
	double *z = new double [n+2];
	for (int i=0;i<n+2; i++)
		z[i] = 0.0;
	double nwx = GenGeomAlgs::findArea(n, y, z);
	double nwy = GenGeomAlgs::findArea(n, z, x);
	double nwz = GenGeomAlgs::findArea(n, x, y);
	
	// get length of the Newell normal
	double nlen = sqrt( nwx*nwx + nwy*nwy + nwz*nwz );
	return nlen;    // area of polygon = length of Newell normal
}

double GenGeomAlgs::ComputePerimeter2D(int n, double *x, double *y) 
{
	double Peri = GenGeomAlgs::ComputeEucDist(x[0],y[0],x[n-1],y[n-1]);
	for (int i=0; i < n-1; i++)
		Peri += GenGeomAlgs::ComputeEucDist(x[i],y[i],x[i+1],y[i+1]);
	return Peri;
}

namespace GenGeomAlgs {
	// Based on http://en.wikipedia.org/wiki/Cohen-Sutherland_algorithm
	const int INSIDE = 0; // 0000
	const int LEFT = 1;   // 0001
	const int RIGHT = 2;  // 0010
	const int BOTTOM = 4; // 0100
	const int TOP = 8;    // 1000
	int ComputeOutCode(const double& x, const double& y,
					   const double& xmin, const double& ymin,
					   const double& xmax, const double& ymax) {
        int code = INSIDE;       // initialised as being inside of clip window
		
        if (x < xmin)           // to the left of clip window
			code |= LEFT;
        else if (x > xmax)      // to the right of clip window
			code |= RIGHT;
        if (y < ymin)           // below the clip window
			code |= BOTTOM;
        else if (y > ymax)      // above the clip window
			code |= TOP;
		
        return code;
	}
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
// diagonal from (xmin, ymin) to (xmax, ymax).
// Based on http://en.wikipedia.org/wiki/Cohen-Sutherland_algorithm
// return false if line segment outside of bounding box
bool GenGeomAlgs::ClipToBB(double& x0, double& y0, double& x1, double& y1,
						   const double& xmin, const double& ymin,
						   const double& xmax, const double& ymax)
{
	// compute outcodes for P0, P1,
	// and whatever point lies outside the clip rectangle
	int outcode0 = ComputeOutCode(x0, y0, xmin, ymin, xmax, ymax);
	int outcode1 = ComputeOutCode(x1, y1, xmin, ymin, xmax, ymax);
	bool accept = false;
	
	while (true) {
		if (!(outcode0 | outcode1)) {
			// Bitwise OR is 0. Trivially accept and get out of loop
			accept = true;
			break;
		} else if (outcode0 & outcode1) {
			// Bitwise AND is not 0. Trivially reject and get out of loop
			break;
		} else {
			// failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge
			double x, y;
			
			// At least one endpoint is outside the clip rectangle; pick it.
			int outcodeOut = outcode0 ? outcode0 : outcode1;
			
			// Now find the intersection point;
			// use formulas y = y0 + slope * (x - x0),
			//   x = x0 + (1 / slope) * (y - y0)
			if (outcodeOut & TOP) {
				// point is above the clip rectangle
				x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
				y = ymax;
			} else if (outcodeOut & BOTTOM) {
				// point is below the clip rectangle
				x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
				y = ymin;
			} else if (outcodeOut & RIGHT) {
				// point is to the right of clip rectangle
				y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
				x = xmax;
			} else if (outcodeOut & LEFT) {
				// point is to the left of clip rectangle
				y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
				x = xmin;
			}
			
			// Now we move outside point to intersection point to clip
			// and get ready for next pass.
			if (outcodeOut == outcode0) {
				x0 = x;
				y0 = y;
				outcode0 = ComputeOutCode(x0, y0, xmin, ymin, xmax, ymax);
			} else {
				x1 = x;
				y1 = y;
				outcode1 = ComputeOutCode(x1, y1, xmin, ymin, xmax, ymax);
			}
		}
	}
	return accept;
}

