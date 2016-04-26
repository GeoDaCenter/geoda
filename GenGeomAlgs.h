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

#include <wx/gdicmn.h> // for wxPoint / wxRealPoint

namespace GenGeomAlgs {
	const double pi = 3.141592653589793238463;
	const double pi_x_2 = pi*2.0;
	const double pi_x_1_5 = pi*1.5;
	const double pi_ovr_2 = pi/2.0;
	const double d180_ovr_pi = 180.0/pi;
	const double pi_ovr_180 = pi/180.0;
	const double one_mi_in_km = 1.609344; // 1 Statute mi in km
	const double earth_radius_km = 6371.0;
	const double earth_radius_mi = earth_radius_km / one_mi_in_km;
	double DegToRad(double deg);
	double RadToDeg(double rad);
	double NormLonDeg(double lon);
	double NormLonRad(double lon);
	double NormLatDeg(double lat);
	double NormLatRad(double lat);
	void LongLatDegToUnit(const double& lon, const double& lat,
						  double& x, double& y, double& z);
	void LongLatRadToUnit(const double& lon, const double& lat,
						  double& x, double& y, double& z);
	void UnitToLongLatDeg(const double& x, const double& y, const double& z,
						  double& lon, double& lat);
	void UnitToLongLatRad(const double& x, const double& y, const double& z,
						  double& lon, double& lat);
	/** Convert unit sphere euclidean distance to radians. */
	double UnitDistToRad(double d);
	double UnitDistToDeg(double d);
	/** Convert radians to straight-line euclidean distance on unit sphere. */
	double RadToUnitDist(double r);
	double DegToUnitDist(double r);
	/** Limit angle to between 0 and pi radians (shortest distance
	 * around circle). */
	double ShortestRad(double r);
	/** Limit angle to between 0 and 180 radians (shortest distance
	 * around circle). */
	double ShortestDeg(double d);
	double ComputeEucDist(double x1, double y1, double x2, double y2);
	double ComputeArcDistMi(double lon1, double lat1, double lon2, double lat2);
	double ComputeArcDistKm(double lon1, double lat1, double lon2, double lat2);
	double ComputeArcDistRad(double lon1, double lat1, double lon2, double lat2);
	double LonLatRadDistRad(double lon1, double lat1, double lon2, double lat2);
	double ComputeArcDistDeg(double lon1, double lat1, double lon2, double lat2);
	double EarthRadToKm(double radians);
	double EarthRadToMi(double radians);
	double EarthKmToRad(double d);
	double EarthMiToRad(double d);
	double ComputePerimeter2D(int n, double *x, double *y);
	double ComputeArea2D(int n, double *x, double *y);    // output unit normal
	double findArea(int n, double *x, double *y);         // 2D polygon
	bool ClipToBB(double& x0, double& y0, double& x1, double& y1,
				  const double& xmin, const double& ymin,
				  const double& xmax, const double& ymax);
	
	void StandardizeRect(const wxPoint& s1, const wxPoint& s2,
											 wxPoint& lower_left, wxPoint& upper_right);
	bool RectsIntersect(const wxPoint& r1_lower_left,
											const wxPoint& r1_upper_right,
											const wxPoint& r2_lower_left,
											const wxPoint& r2_upper_right);
	bool CounterClockwise(const wxPoint& p1, const wxPoint& p2,
												const wxPoint& p3);
	bool LineSegsIntersect(const wxPoint& l1_p1, const wxPoint& l1_p2,
												 const wxPoint& l2_p1, const wxPoint& l2_p2);
	
	bool ExtendRayToBB(double x0, double y0, double x1, double y1,
										 double& x2, double& y2,
										 const double xmin, const double ymin,
										 const double xmax, const double ymax);
	
	bool nearlyEqual(double x, double y);
}

#endif
