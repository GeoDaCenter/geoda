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

#include <wx/stopwatch.h>
#include "GenGeomAlgs.h"
#include "PointSetAlgs.h"
#include "libgdiam/gdiam.hpp"
#include "logger.h"

void PointSetAlgs::GetMinMax(const std::vector<wxRealPoint>& pts,
							 double& min_x, double& min_y,
							 double& max_x, double& max_y)
{
	if (pts.size() == 0) return;
	min_x = pts[0].x;
	max_x = min_x;
	min_y = pts[0].y;
	max_y = min_y;
	for (size_t i=0, sz=pts.size(); i<sz; ++i) {
		if (pts[i].x < min_x) {
			min_x = pts[i].x;
		} else if (pts[i].x > max_x) {
			max_x = pts[i].x;
		}
		if (pts[i].y < min_y) {
			min_y = pts[i].y;
		} else if (pts[i].y > max_y) {
			max_y = pts[i].y;
		}
	}
}

double PointSetAlgs::EstDiameter(const std::vector<double>& x,
                                 const std::vector<double>& y,
                                 bool is_arc,
																 wxRealPoint& pt1, wxRealPoint& pt2)
{
	LOG_MSG("Entering PointSetAlgs::EstDiameter");
	using namespace GenGeomAlgs;
	wxStopWatch sw;
	size_t num = x.size();
	gdiam_real* points;
	points = (gdiam_point)malloc( sizeof( gdiam_point_t ) * num );
	if (points == NULL) return -1;
	
	wxString msg;
	msg << "Computing the diameter for " << num << " points ";
	
	if (is_arc) {
		// convert all long/lat points to points on unit sphere
		double xt, yt, zt;
		for (size_t i=0; i<num; ++i) {
			LongLatDegToUnit(x[i], y[i], xt, yt, zt);
			points[i*3+0] = xt;
			points[i*3+1] = yt;
			points[i*3+2] = zt;
		}
	} else {
		for (size_t i=0; i<num; ++i) {
			points[i*3+0] = x[i];
			points[i*3+1] = y[i];
			points[i*3+2] = 0;
		}
	}
	
	GPointPair pair;
	pair = gdiam_approx_diam_pair( (gdiam_real *)points, num, 0.0 );
	msg = "";
	double dist;
	if (is_arc) {
		double p_lat, p_lon, q_lat, q_lon;
		UnitToLongLatDeg(pair.p[0], pair.p[1], pair.p[2], p_lon, p_lat);
		UnitToLongLatDeg(pair.q[0], pair.q[1], pair.q[2], q_lon, q_lat);
		pt1.x = p_lon;
		pt1.y = p_lat;
		pt2.x = q_lon;
		pt2.y = q_lat;
		
		dist = ComputeArcDistDeg(p_lon, p_lat, q_lon, q_lat);
		
		msg << "\n";
		msg << "Diameter distance (deg): " << dist << "\n";
		msg << "Diameter distance (km): " << EarthRadToKm(DegToRad(dist)) << "\n";
		msg << "Diameter distance (mi): " << EarthRadToMi(DegToRad(dist)) << "\n";
		msg << "Points realizing the diameter (long, lat):\n";
		msg << "  (" << p_lon << ", " << p_lat << ")  ";
		msg << "(" << q_lon << ", " << q_lat << ")\n";
	} else {
		dist = pair.distance;
		pt1.x = pair.p[0];
		pt1.y = pair.p[1];
		pt2.x = pair.q[0];
		pt2.y = pair.q[1];
		msg << "Diameter distance: " << dist << "\n";
		msg << "Points realizing the diameter:\n";
		msg << "  (" << pair.p[0] << ", " << pair.p[1] << ", " << pair.p[2] << ")  ";
		msg << "(" << pair.q[0] << ", " << pair.q[1] << ", " << pair.q[2] << ")\n";
	}
	msg << "Compute time in ms: " << sw.Time();
	
	free(points);
	
	LOG_MSG("Exiting PointSetAlgs::EstDiameter");
	return dist;  // if arc, then value returned in degrees
}


double PointSetAlgs::EstDiameter(const std::vector<wxRealPoint>& pts,
																 bool is_arc,
																 wxRealPoint& pt1, wxRealPoint& pt2)
{
	size_t nobs = pts.size();
	std::vector<double> x(nobs);
	std::vector<double> y(nobs);
	for (size_t i=0; i<nobs; ++i) {
		x[i] = pts[i].x;
		y[i] = pts[i].y;
	}
	return EstDiameter(x, y, is_arc, pt1, pt2);
}

double PointSetAlgs::EstDistMedian(const std::vector<wxRealPoint>& pts,
																	 bool is_arc, int trials)
{
	return -1;
}

