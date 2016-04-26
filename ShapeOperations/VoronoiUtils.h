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

#ifndef __GEODA_CENTER_VORONOI_UTILS_H__
#define __GEODA_CENTER_VORONOI_UTILS_H__

#include <list>
#include <set>
#include <vector>

class GdaPolygon;
class GdaShape;
class GalElement;

namespace Gda {
	namespace VoronoiUtils {
		
		void FindPointDuplicates(const std::vector<double>& x,
								 const std::vector<double>& y,
								 std::list<std::list<int> >& duplicates);
		bool MakePolygons(const std::vector<double>& x,
						  const std::vector<double>& y,
						  std::vector<GdaShape*> &polys,
						  double& voronoi_bb_xmin, double& voronoi_bb_ymin,
						  double& voronoi_bb_xmax, double& voronoi_bb_ymax);
		bool PointsToContiguity(const std::vector<double>& x,
								const std::vector<double>& y,
								bool queen, // if false, then rook only
								std::vector<std::set<int> >& nbr_map);
		GalElement* NeighborMapToGal(std::vector<std::set<int> >& nbr_map);
	}
}

#endif
