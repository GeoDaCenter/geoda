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
 * Created: 8/22/2017 lixun910@gmail.com
 */

#ifndef __GEODA_CENTER_AZP_H__
#define __GEODA_CENTER_AZP_H__

#include <vector>

//#include <tr1/type_traits>

#include "../ShapeOperations/GalWeight.h"
#include "rng.h"
#include "DataUtils.h"

////////////////////////////////////////////////////////////////////////////////
////// ZoneControl
////////////////////////////////////////////////////////////////////////////////
class ZoneControl {
public:
    // A ZoneControl is for one variable (e.g. population), and it contains
    // the values of the variable
    ZoneControl(const std::vector<double>& in_data);
    virtual ~ZoneControl();

    enum Operation {SUM, MEAN, MAX, MIN};
    enum Comparator {LESS_THAN, MORE_THAN};

    // A ZoneControl can have more than one control, which defines the
    // Operation (Sum/mean/max/min), Comparator (less/more) and a restrict
    // value. For example: the following two AddControl() function calls
    // define two restrictions when building a zone
    // zc.AddControl(Operation::SUM, Comparator::MORE_THAN, 10000);
    // zc.AddControl(Operation::SUM, Comparator::LESS_THAN, 50000);
    // SUM of population is MORE than value 10,000
    // SUM of population is LESS than value 50,000
    void AddControl(Operation op, Comparator cmp, const double& val);

    // Check if a candidate zone satisfies the restrictions
    bool CheckZone(const std::vector<int>& candidates);

protected:
    std::vector<double> data;

    std::vector<Operation> operations;

    std::vector<Comparator> comparators;

    std::vector<double> comp_values;
};

////////////////////////////////////////////////////////////////////////////////
////// Area
////////////////////////////////////////////////////////////////////////////////
// Area Class for Regional Clustering.
class Area
{
    int id;
    std::vector<double> data;
    DistMatrix* dist_matrix;
    GalElement*  const w;
public:
    Area(int _id, GalElement* const _w, const std::vector<double>& _data,
         DistMatrix* const _dist_matrix)
    : id(_id), w(_w), data(_data), dist_matrix(_dist_matrix) {}

    virtual ~Area() {}

    // Return the distance between the area and other area
    double returnDistance2Area(Area& otherArea) {
        return dist_matrix->getDistance(id, otherArea.GetId());
    }

    int GetId() { return id;}
};

////////////////////////////////////////////////////////////////////////////////
/// AreaManager
////////////////////////////////////////////////////////////////////////////////

// This class contains operations at areal level, including the generation of
// instances of areas, a wide range of area2area and area2region distance
// functions.
class AreaManager
{
public:
    AreaManager(int n, int m, GalElement* const w, double** data, DistMatrix* const dist_matrix);

    virtual ~AreaManager() {}
    
    // Returns the distance between two areas
    double returnDistance2Area(int i, int j);

    // Returns the attribute centroid of a set of areas
    std::vector<double> getDataAverage(const std::vector<int>& areaList);

    // Returns the distance from an area to a region (centroid)
    double getDistance2Region(int area, const std::set<int>& areaList);

protected:
    // n: number of observations
    int n;

    // m is the dimension of the variable space
    int m;

    // w defines the weights structure, who's who's neighbor
    GalElement* w;

    // distance matrix between obs i and j: getDistance(i, j)
    DistMatrix* dist_matrix;

    double** data;
};

////////////////////////////////////////////////////////////////////////////////
////// RegionMaker
////////////////////////////////////////////////////////////////////////////////
class RegionMaker
{
public:
    RegionMaker(int p, GalElement* const w,
                double** data, // row-wise
                RawDistMatrix* dist_matrix,
                int n, int m);

    virtual ~RegionMaker() {}

    // Sets the initial seeds for clustering
    void setSeeds(const std::vector<int>& seeds);

    // Return the areas of a region
    std::set<int> returnRegion2Area(int regionID);

    // calculate objective function
    double calcObj();

    // return regions created
    std::vector<int> returnRegions();

    //
    virtual void LocalImproving() = 0;
    
protected:
    // Assign to the region "-1" for the areas without neighbours
    void AssignAreasNoNeighs();

    // Initial p starting observations using K-Means
    std::vector<int> kmeansInit();

    // Assign an area to a region and updates potential regions for the neighs
    // Parameters
    void assignSeeds(int areaID, int regionID);

    // Assgin an area to a region
    void assignAreaStep1(int areaID, int regionID);

    // Construct potential regions per area
    void constructRegions();

    // Assign an area to a region and updates potential regions for neighs
    void assignArea(int areaID, int regionID);

    //  Gets the intrabordering areas
    void getIntraBorderingAreas();

    // Return the value of the objective function
    double getObj();
    double calcObj(std::map<int, std::set<int> >& region2Area);

    // Returns bordering areas of a region
    std::set<int> returnBorderingAreas(int regionID);
    
    // Check feasibility from a change region (remove an area from a region)
    int checkFeasibility(int regionID, int areaID,
                        std::map<int, std::set<int> > region2AreaDict);
    
    // Get neighbors in a set
    std::set<int> getNeighbors(int areaID);
    
    // Removed an area from a region and appended it to another one
    void swapArea(int area, int newRegion,
                  std::map<int, std::set<int> >& region2AreaDict,
                  std::map<int, int>& area2RegionDict);
    
    // Move an area to a region
    void moveArea(int area, int move);
    
    // help functions for std::set<>
    std::set<int> unions(const std::set<int>& s1, const std::set<int>& s2);
    std::set<int> intersects(const std::set<int>& s1, const std::set<int>& s2);
    std::set<int> removes(const std::set<int>& s1, const std::set<int>& s2);

protected:
    double** data;

    // n is the number of observations
    int n;

    // m is the dimension of the variable space
    int m;

    // p is the number of zones/regions to construct
    int p;

    // w defines the weights structure, who's who's neighbor
    GalElement* w;

    // pairwise distance between obs i and j
    RawDistMatrix* dist_matrix;

    AreaManager am;

    std::vector<int> seeds;

    std::map<int, bool> unassignedAreas;

    std::map<int, bool> assignedAreas;
    
    std::map<int, bool> areaNoNeighbor;

    std::map<int, int> area2Region;

    std::map<int, std::set<int> > region2Area;

    std::map<int, std::set<int> > potentialRegions4Area;

    std::map<int, std::set<int> > intraBorderingAreas;

    std::map<std::pair<int, int>, double> candidateInfo;

    // mark which region has been changed
    int changedRegion;

    // mark which area has just been added
    int addedArea;

    std::map<int, bool> externalNeighs;

    std::map<int, bool> oldExternal;

    std::map<int, bool> newExternal;

    // object function value
    double objInfo;

    Xoroshiro128Random rng;
};

////////////////////////////////////////////////////////////////////////////////
////// AZP
////////////////////////////////////////////////////////////////////////////////
class AZP : public RegionMaker
{
public:
    AZP(int p, GalElement* const w,
        double** data, // row-wise
        RawDistMatrix* dist_matrix,
        int n, int m)
    : RegionMaker(p,w,data,dist_matrix,n,m)
    {

    }

    virtual ~AZP() {}

    virtual void LocalImproving();
};

////////////////////////////////////////////////////////////////////////////////
////// AZP Simulated Annealing
////////////////////////////////////////////////////////////////////////////////
class AZPSA : public RegionMaker
{
public:
    AZPSA(int p, GalElement* const w,
        double** data, // row-wise
        RawDistMatrix* dist_matrix,
        int n, int m)
    : RegionMaker(p,w,data,dist_matrix,n,m)
    {

    }

    virtual ~AZPSA() {}

    virtual void LocalImproving();
};

////////////////////////////////////////////////////////////////////////////////
////// AZP Tabu
////////////////////////////////////////////////////////////////////////////////
class AZPTabu : public RegionMaker
{
public:
    AZPTabu(int p, GalElement* const w,
        double** data, // row-wise
        RawDistMatrix* dist_matrix,
        int n, int m)
    : RegionMaker(p,w,data,dist_matrix,n,m)
    {

    }

    virtual ~AZPTabu() {}

    virtual void LocalImproving();
};
#endif
