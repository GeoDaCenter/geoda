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

#ifndef __GEODA_CENTER_POINT_SET_ALGS_H__
#define __GEODA_CENTER_POINT_SET_ALGS_H__

#include <vector>
#include <wx/gdicmn.h> // for wxRealPoint

namespace PointSetAlgs {
	void GetMinMax(const std::vector<wxRealPoint>& pts, double& min_x,
								 double& min_y, double& max_x, double& max_y);
	/** Returns the maximum distance between any pair of points in
	  pts set. pt1 and pt2 return the pair of points that realized
	  that difference.  For long/lat points the returned points pt1
	  and pt2 might have some rounding error.  For long/lat points,
	  results are in degrees. */
	double EstDiameter(const std::vector<double>& x,
										 const std::vector<double>& y,
										 bool is_arc,
										 wxRealPoint& pt1, wxRealPoint& pt2);
	double EstDiameter(const std::vector<wxRealPoint>& pts, bool is_arc,
										 wxRealPoint& pt1, wxRealPoint& pt2);
	double EstDistMedian(const std::vector<wxRealPoint>& pts, bool is_arc,
						 int trials=-1);
}

#endif
