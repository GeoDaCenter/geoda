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
 *
 * Created: 5/30/2017 lixun910@gmail.com
 */

#ifndef __GEODA_CENTER_HDBSCAN_H__
#define __GEODA_CENTER_HDBSCAN_H__

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef boost::geometry::model::point
<
double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>
> pt_lonlat; // spherical point
typedef bg::model::point<double, 2, bg::cs::cartesian> pt_2d;
typedef bg::model::point<double, 3, bg::cs::cartesian> pt_3d;
typedef bg::model::box<pt_2d> box_2d;
typedef bg::model::box<pt_3d> box_3d;
typedef std::pair<box_2d, unsigned> box_2d_val;
typedef std::pair<pt_2d, unsigned> pt_2d_val;
typedef std::pair<pt_3d, unsigned> pt_3d_val;
typedef std::pair<pt_lonlat, unsigned> pt_lonlat_val;

typedef boost::geometry::index::rtree< box_2d_val, bgi::quadratic<16> > rtree;

typedef bgi::rtree< pt_2d_val, bgi::quadratic<16> > rtree_pt_2d_t;
typedef bgi::rtree< pt_3d_val, bgi::quadratic<16> > rtree_pt_3d_t;
typedef bgi::rtree< pt_lonlat_val, bgi::quadratic<16> > rtree_pt_lonlat_t;


#include "redcap.h"

using namespace SpanningTreeClustering;

namespace GeoDaClustering {
    /////////////////////////////////////////////////////////////////////////
    //
    // HDBSCAN
    //
    /////////////////////////////////////////////////////////////////////////
    class HDBScan : public FirstOrderSLKRedCap
    {
        vector<double> cores;
        vector<int> designations;
        
    public:
        HDBScan(int rows, int cols,
                double** _distances,
                double** data,
                const vector<bool>& undefs,
                GalElement * w,
                double* controls,
                double control_thres);
        virtual ~HDBScan();
        
    };
    
}
#endif
