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

#include <algorithm>
#include <vector>
#include <limits>
#include <math.h>
#include "GenGeomAlgs.h"


double GenGeomAlgs::DegToRad(double deg)
{
	return deg * pi_ovr_180;
}

double GenGeomAlgs::RadToDeg(double rad)
{
	return rad * d180_ovr_pi;
}

double GenGeomAlgs::NormLonDeg(double lon)
{
	bool neg = lon < 0;
	if (neg) lon = -lon;
	lon += 180.0;
	lon = fmod(lon,360.0);
	lon -= 180.0;
	if (neg) lon = -lon;
	return lon;
}

double GenGeomAlgs::NormLonRad(double lon)
{
	bool neg = lon < 0;
	if (neg) lon = -lon;
	lon += pi;
	lon = fmod(lon,pi_x_2);
	lon -= pi;
	if (neg) lon = -lon;
	return lon;
}

double GenGeomAlgs::NormLatDeg(double lat)
{
	bool neg = lat < 0;
	if (neg) lat = -lat;
	if (lat > 90 && lat <= 270) {
		lat = 180.0 - lat;
	} else if (lat > 270) {
		lat -= 360.0;
	}
	if (neg) lat = -lat;
	return lat;
}

double GenGeomAlgs::NormLatRad(double lat)
{
	bool neg = lat < 0;
	if (neg) lat = -lat;
	if (lat > pi_ovr_2 && lat <= pi_x_1_5) {
		lat = pi - lat;
	} else if (lat > pi_x_1_5) {
		lat -= pi_x_2;
	}
	if (neg) lat = -lat;
	return lat;
}

void GenGeomAlgs::LongLatDegToUnit(const double& lon, const double& lat,
								   double& x, double& y, double& z)
{
	// unit sphere -> radius is 1
	double lat_r = DegToRad(lat);
	double lon_r = DegToRad(lon);
	double c_lat = cos(lat_r);
	x = c_lat * cos(lon_r);
	y = c_lat * sin(lon_r);
	z = sin(lat_r);
}

void GenGeomAlgs::LongLatRadToUnit(const double& lon, const double& lat,
								   double& x, double& y, double& z)
{
	// unit sphere -> radius is 1
	double c_lat = cos(lat);
	x = c_lat * cos(lon);
	y = c_lat * sin(lon);
	z = sin(lat);
}

void GenGeomAlgs::UnitToLongLatDeg(const double& x, const double& y,
								   const double& z,
								   double& lon, double& lat)
{
	// unit sphere -> radius is 1
	lat = RadToDeg(NormLatRad(asin(z)));
	lon = RadToDeg(NormLonRad(atan2(y,x)));
}

void GenGeomAlgs::UnitToLongLatRad(const double& x, const double& y,
								   const double& z,
								   double& lon, double& lat)
{
	// unit sphere -> radius is 1
	lat = NormLatRad(asin(z));
	lon = NormLonRad(atan2(y,x));
}

double GenGeomAlgs::UnitDistToRad(double d)
{
	if (d<0) d = -d;
	if (d >= 2) return pi;
	return acos((2.0-d*d)/2.0);
}

double GenGeomAlgs::UnitDistToDeg(double d)
{
	return RadToDeg(UnitDistToRad(d));
}

double GenGeomAlgs::RadToUnitDist(double r)
{
	if (r<0) r = -r;
	r = ShortestRad(r);
	if (r >= pi) return 2;
	double t = (2.0-2.0*cos(r));
	if (t <= 0) return 0;
	return sqrt(t);
}

double GenGeomAlgs::DegToUnitDist(double r)
{
	return RadToUnitDist(DegToRad(r));
}

double GenGeomAlgs::ShortestRad(double r)
{
	if (r < 0) r = -r;
	if (r <= pi) return r;
	r = fmod(r, 2.0*pi);
	if (r <= pi) return r;
	return 2.0*pi - r;
}

double GenGeomAlgs::ShortestDeg(double d)
{
	if (d < 0) d = -d;
	if (d <= 180.0) return d;
	d = fmod(d, 360.0);
	if (d <= 180.0) return d; 
	return 360.0 - d;
}

double GenGeomAlgs::ComputeEucDist(double x1, double y1, double x2, double y2) 
{
	return sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}

/*
 Notes on ComputeArcDistMi:
 In the equation below, the 69.11 factor is actually the distance,
 in miles, between each degree of latitude for the WGS
 84 sphere. Remember that this equation is just an approximation
 because the earth is actually an ellipsoid. Because of
 this, the distance between latitudes will increase as the latitude
 increases. The distance at 0∞ on the WGS 84 ellipsoid is
 actually 68.71 miles while it is 69.40 miles at 90 deg.
 */

double GenGeomAlgs::ComputeArcDistMi(double lon1, double lat1, double lon2, double lat2)
{
	return ComputeArcDistRad(lon1, lat1, lon2, lat2) * earth_radius_mi;
}

double GenGeomAlgs::ComputeArcDistKm(double lon1, double lat1, double lon2, double lat2)
{
    return ComputeArcDistRad(lon1, lat1, lon2, lat2) * earth_radius_km;
}

double GenGeomAlgs::ComputeArcDistRad(double lon1, double lat1, double lon2, double lat2)
{
	return LonLatRadDistRad(DegToRad(lon1), DegToRad(lat1), DegToRad(lon2), DegToRad(lat2));
}

double GenGeomAlgs::LonLatRadDistRad(double lon1, double lat1, double lon2, double lat2)
{
	// this is the haversine formula which is particularly well-conditioned
	double d_lat_ovr_2 = (lat2-lat1)/2.0;
	double sin_sq_d_lat_ovr_2 = sin(d_lat_ovr_2);
	sin_sq_d_lat_ovr_2 *= sin_sq_d_lat_ovr_2;
	double d_lon_ovr_2 = (lon2-lon1)/2.0;
	double sin_sq_d_lon_ovr_2 = sin(d_lon_ovr_2);
	sin_sq_d_lon_ovr_2 *= sin_sq_d_lon_ovr_2;

	double a = sin_sq_d_lat_ovr_2 +
		cos(lat1)*cos(lat2) * sin_sq_d_lon_ovr_2;
	return 2.0* atan2(sqrt(a),sqrt(1.0-a));
}

double GenGeomAlgs::ComputeArcDistDeg(double lon1, double lat1, double lon2, double lat2)
{
	return RadToDeg(ComputeArcDistRad(lon1, lat1, lon2, lat2));
}

double GenGeomAlgs::EarthRadToKm(double radians)
{
	return radians * earth_radius_km;
}

double GenGeomAlgs::EarthRadToMi(double radians)
{
	return radians * earth_radius_mi;
}

double GenGeomAlgs::EarthKmToRad(double d)
{
	return d/earth_radius_km;
}

double GenGeomAlgs::EarthMiToRad(double d)
{
	return d/earth_radius_mi;
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
	for (int i=0;i<n+2; i++) z[i] = 0.0;
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
	for (int i=0; i < n-1; i++) {
		Peri += GenGeomAlgs::ComputeEucDist(x[i],y[i],x[i+1],y[i+1]);
	}
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
										 const double& xmax, const double& ymax)
	{
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

/** convert input rectangle corners s1 and s2 into screen-coordinate corners */
void GenGeomAlgs::StandardizeRect(const wxPoint& s1, const wxPoint& s2,
																	wxPoint& lower_left, wxPoint& upper_right)
{
	lower_left = s1;
	upper_right = s2;
	if (lower_left.x > upper_right.x) {
		// swap
		int t = lower_left.x;
		lower_left.x = upper_right.x;
		upper_right.x = t;
	}
	if (lower_left.y < upper_right.y) {
		// swap
		int t = lower_left.y;
		lower_left.y = upper_right.y;
		upper_right.y = t;
	}
}

/** assumes input corners are all screen-coordinate correct for
 lower left and upper right corners */
bool GenGeomAlgs::RectsIntersect(const wxPoint& r1_lower_left,
                                 const wxPoint& r1_upper_right,
                                 const wxPoint& r2_lower_left,
                                 const wxPoint& r2_upper_right)
{
	// return negation of all situations where rectangles
	// do not intersect.
	return (!((r1_lower_left.x > r2_upper_right.x) ||
						(r1_upper_right.x < r2_lower_left.x) ||
						(r1_lower_left.y < r2_upper_right.y) ||
						(r1_upper_right.y > r2_lower_left.y)));
}

bool GenGeomAlgs::CounterClockwise(const wxPoint& p1, const wxPoint& p2,
																const wxPoint& p3)
{
	return ((p2.y-p1.y)*(p3.x-p2.x) < (p3.y-p2.y)*(p2.x-p1.x));
}

bool GenGeomAlgs::LineSegsIntersect(const wxPoint& l1_p1, const wxPoint& l1_p2,
																		const wxPoint& l2_p1, const wxPoint& l2_p2)
{
	return ((CounterClockwise(l2_p1, l2_p2, l1_p1) !=
					 CounterClockwise(l2_p1, l2_p2, l1_p2)) &&
					(CounterClockwise(l1_p1, l1_p2, l2_p1) !=
					 CounterClockwise(l1_p1, l1_p2, l2_p2)));
}

/** A ray is defined by two points (x0, y0) and (x1, y1) that are within the
 given bounding box (BB) extent.  The ray is anchored at (x0, y0) and extends
 in the direction of (x1, y1).  If (x0, y0) or (x1, y1) are outside the BB,
 false is returned.  Otherwise, the ray is extended to it's intersection
 point with the BB and this point is retruned as (x2, y2). True is returned
 on success. */
bool GenGeomAlgs::ExtendRayToBB(double x0, double y0, double x1, double y1,
																double& x2, double& y2,
																const double xmin, const double ymin,
																const double xmax, const double ymax)
{
	// check if points are nearly identical
	if (nearlyEqual(x0,x1) && nearlyEqual(y0,y1)) return false;
	// check if any points outisde of BB
	if (x0<xmin || y0<ymin || x1<xmin || y1<ymin ||
			x0>xmax || y0>ymax || x1>xmax || y1>ymax) return false;
	// check for easy special cases of nearly horizontal or nearly vertical lines
	if (nearlyEqual(x0, x1)) {
		// simply extend y coordinate to BB limits
		x2 = x0;
		y2 = (y0 < y1) ? ymax : ymin;
		return true;
	}
	if (nearlyEqual(y0, y1)) {
		// simply extend x coordinate to BB limits
		x2 = (x0 < x1) ? xmax : xmin;
		y2 = y0;
		return true;
	}
	
	// At this point we are not dealing with a special case.  In particular,
	// we shouldn't have to worry about dividing by zero when calculating
	// line slopes.
	
	// Let y = s*x + t be equation for line through original points.
	// Or, solving for x we get: x = (y-t)/s
	// Calculate slope a and y-intercept b as follows:
	double s = (y1-y0)/(x1-x0);
	double t = y0 - s*x0;
	
	double b0x, b0y, b1x, b1y;
	if (x0 < x1) {
		// ray travels to the right and intersects xmax
		b0x = xmax;
		b0y = s*xmax+t;
		if (y0 < y1) {
			// ray travels up and intersects ymax
			b1y = ymax;
		} else {
			// ray travels down and intersects ymin
			b1y = ymin;
		}
		b1x = (b1y-t)/s;
		
		if (b0x < b1x) {
			x2 = b0x;
			y2 = b0y;
		} else {
			x2 = b1x;
			y2 = b1y;
		}
	} else {
		// ray travels to the left and intersects xmin
		b0x = xmin;
		b0y = s*xmin+t;
		if (y0 < y1) {
			// ray travels up and intersects ymax
			b1y = ymax;
		} else {
			// ray travels down and intersects ymin
			b1y = ymin;
		}
		b1x = (b1y-t)/s;
		
		if (b0x > b1x) {
			x2 = b0x;
			y2 = b0y;
		} else {
			x2 = b1x;
			y2 = b1y;
		}
	}
	return true;
}

bool GenGeomAlgs::nearlyEqual(double x, double y)
{
	double max1xy = std::max(fabs(x) ,fabs(y));
	max1xy = std::max(max1xy, 1.0);
	// have added in an extra factor of 8 in the following.  Not needed,
	// but want to add in a margin of safety
	return fabs(x-y) <= 8*std::numeric_limits<double>::epsilon()*max1xy;
}


