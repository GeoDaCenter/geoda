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

#include <algorithm>
#include <vector>
#include <limits>
#include <boost/unordered_map.hpp>

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
    std::vector<double> getDataAverage(const std::set<int>& areaList);

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

    boost::unordered_map<std::set<int>, std::vector<double> > cache_cent;
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
    
    virtual void LocalImproving() = 0;
    
    virtual std::vector<int> GetResults() = 0;

    virtual double GetInitObjectiveFunction() = 0;

    virtual double GetFinalObjectiveFunction() = 0;

protected:
    // Return the areas of a region
    std::set<int> returnRegion2Area(int regionID);

    // return regions created
    std::vector<int> returnRegions();

    // calculate the objective function
    void calcObj();

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
    
    // Re-calculate the value of the objective function
    double recalcObj(boost::unordered_map<int, std::set<int> >& region2AreaDict, bool use_cache=true);
    double recalcObj(boost::unordered_map<int, std::set<int> >& region2AreaDict,
                     std::pair<int, int>& modifiedRegions);
    // Return the value of the objective function from regions to area dictionary
    double getObjective(boost::unordered_map<int, std::set<int> >& region2AreaDict);
    // Return the value of the objective function from regions to area dictionary
    double getObjectiveFast(boost::unordered_map<int, std::set<int> >& region2AreaDict,
                            std::pair<int, int>& modifiedRegions);

    // Returns bordering areas of a region
    std::set<int> returnBorderingAreas(int regionID);
    
    // Check feasibility from a change region (remove an area from a region)
    int checkFeasibility(int regionID, int areaID,
                         boost::unordered_map<int, std::set<int> >& region2AreaDict);
    
    // Removed an area from a region and appended it to another one
    void swapArea(int area, int newRegion,
                  boost::unordered_map<int, std::set<int> >& region2AreaDict,
                  boost::unordered_map<int, int>& area2RegionDict);
    
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

    boost::unordered_map<int, bool> unassignedAreas;

    boost::unordered_map<int, bool> assignedAreas;
    
    boost::unordered_map<int, bool> areaNoNeighbor;

    boost::unordered_map<int, int> area2Region;

    boost::unordered_map<int, std::set<int> > region2Area;

    // area that could be assigned to which regions
    boost::unordered_map<int, std::set<int> > potentialRegions4Area;

    // area on border, could belongs to which regions
    boost::unordered_map<int, std::set<int> > intraBorderingAreas;

    // area --(assign to)--region : distance
    boost::unordered_map<std::pair<int, int>, double> candidateInfo;

    // mark which region has just been changed
    int changedRegion;

    // mark which area has just been added
    int addedArea;

    boost::unordered_map<int, bool> externalNeighs;

    boost::unordered_map<int, bool> oldExternal;

    boost::unordered_map<int, bool> newExternal;

    // object function value
    double objInfo;

    // cache object value
    boost::unordered_map<int, double> objDict;

    std::vector<std::set<int> > nbrSet;

    Xoroshiro128Random rng;
};

////////////////////////////////////////////////////////////////////////////////
////// AZP
////////////////////////////////////////////////////////////////////////////////
class AZP : public RegionMaker
{
    std::vector<int> final_solution;

    double initial_objectivefunction;

    double final_objectivefunction;

public:
    AZP(int p, GalElement* const w,
        double** data, // row-wise
        RawDistMatrix* dist_matrix,
        int n, int m)
    : RegionMaker(p,w,data,dist_matrix,n,m)
    {
        initial_objectivefunction = this->objInfo;
        this->LocalImproving();
        this->calcObj();
        final_solution = this->returnRegions();
        final_objectivefunction = this->objInfo;
    }

    virtual ~AZP() {}

    virtual void LocalImproving();
    
    virtual std::vector<int> GetResults() {
        return final_solution;
    }

    virtual double GetInitObjectiveFunction() {
        return initial_objectivefunction;
    }

    virtual double GetFinalObjectiveFunction() {
        return final_objectivefunction;
    }
};

////////////////////////////////////////////////////////////////////////////////
////// AZP Simulated Annealing
////////////////////////////////////////////////////////////////////////////////

// Keeps the minimum amount of information about a given solution. It keeps the
// Objective function value (self.objInfo) and the region each area has been assigned to
// (self.regions)
class BasicMemory
{
public:
    BasicMemory() {
        objInfo = std::numeric_limits<double>::max();
    }
    virtual ~BasicMemory() {}
    
    void updateBasicMemory(double val, const std::vector<int>& rgn) {
        // Updates BasicMemory when a solution is modified.
        objInfo = val;
        regions = rgn;
    }
    
    double objInfo;
    std::vector<int> regions;
};

class AZPSA : public RegionMaker
{
    std::vector<int> final_solution;
    double initial_objectivefunction;
    double final_objectivefunction;
public:
    AZPSA(int p, GalElement* const w,
        double** data, // row-wise
        RawDistMatrix* dist_matrix,
        int n, int m, double _alpha = 0.85, int _max_iter= 1)
    : RegionMaker(p,w,data,dist_matrix,n,m), temperature(1.0), alpha(_alpha), max_iter(_max_iter)
    {
        std::vector<int> init_sol = this->returnRegions();
        initial_objectivefunction = this->objInfo;

        // local search
        BasicMemory basicMemory, localBasicMemory;
        
        // step a
        double T = temperature;
        int k = 0;
        while (k < 3) {
            int improved = 0;
            for (int i=0; i<max_iter; ++i) {
                localBasicMemory.updateBasicMemory(this->objInfo, this->returnRegions());
                // step b: modified step 5
                this->LocalImproving();
                
                if (this->objInfo < localBasicMemory.objInfo) {
                    improved = 1;
                }
                if (this->objInfo < basicMemory.objInfo) {
                    // print "Best solution so far: ", rm.returnRegions()
                    //  print "Best O.F. so far: ", rm.objInfo
                    basicMemory.updateBasicMemory(this->objInfo, this->returnRegions());
                }
            }
            // step c
            T *= alpha;
            if (improved == 1) {
                k = 0;
            } else {
                k += 1;
            }
            // step d: repeat b and c
        }
        
        
        final_solution = basicMemory.regions;
        final_objectivefunction = basicMemory.objInfo;
    }

    virtual ~AZPSA() {}

    virtual void LocalImproving();
    
    virtual std::vector<int> GetResults() {
        return final_solution;
    }

    virtual double GetInitObjectiveFunction() {
        return initial_objectivefunction;
    }

    virtual double GetFinalObjectiveFunction() {
        return final_objectivefunction;
    }

protected:
    double temperature;
    
    double alpha;
    
    int max_iter;
};

////////////////////////////////////////////////////////////////////////////////
////// AZP Tabu
////////////////////////////////////////////////////////////////////////////////
class AZPTabu : public RegionMaker
{
    std::vector<int> final_solution;
    double initial_objectivefunction;
    double final_objectivefunction;

public:
    AZPTabu(int p, GalElement* const w,
        double** data, // row-wise
        RawDistMatrix* dist_matrix,
        int n, int m, int tabu_length=10, int _convTabu=0)
    : RegionMaker(p,w,data,dist_matrix,n,m), tabuLength(tabu_length), convTabu(_convTabu)
    {
        if (tabuLength <= 0) {
            tabuLength = 10;
        }
        if (convTabu <= 0) {
            convTabu = 10;
        }
        initial_objectivefunction = this->objInfo;
        std::vector<int> init_sol = this->returnRegions();

        this->LocalImproving();

        this->calcObj();
        final_solution = this->returnRegions();
        final_objectivefunction = this->objInfo;
    }

    virtual ~AZPTabu() {}

    virtual void LocalImproving();

    virtual std::vector<int> GetResults() {
        return final_solution;
    }

    virtual double GetInitObjectiveFunction() {
        return initial_objectivefunction;
    }

    virtual double GetFinalObjectiveFunction() {
        return final_objectivefunction;
    }
    
    // Select neighboring solutions.
    void allCandidates();

    // constructs a dictionary with the objective function per region
    boost::unordered_map<int, double> makeObjDict();

    // Add a new value to the tabu list.
    std::vector<std::pair<int, int> > updateTabuList(int area, int region, std::vector<std::pair<int, int> >& aList,
                   int endInd);

    std::pair<int, int> find_notabu_move(const boost::unordered_map<std::pair<int, int>, double>& s1,
                                         const std::vector<std::pair<int, int> >& s2)
    {
        std::pair<int, int> move(-1, -1);
        double minval;
        int cnt = 0;
        boost::unordered_map<std::pair<int, int>, double> s = s1;
        for (int i=0; i < s2.size(); ++i) {
            s.erase(s2[i]);
        }
        boost::unordered_map<std::pair<int, int>, double>::iterator it;
        for (it = s.begin(); it != s.end(); ++it) {
            if (cnt == 0 || it->second < minval) {
                minval = it->second;
                move = it->first;
            }
            cnt += 1;
        }
        return move;
    }

    std::pair<int, int> find_tabu_move(const boost::unordered_map<std::pair<int, int>, double>& s1,
                                  const std::vector<std::pair<int, int> >& s2)
    {
        std::pair<int, int> move(-1, -1);
        double minval;
        int cnt = 0;
        boost::unordered_map<std::pair<int, int>, double> s = s1;
        for (int i=0; i < s2.size(); ++i) {
            if (s.find(s2[i]) != s.end()) {
                if (cnt == 0 || s[s2[i]] < minval) {
                    minval = s[s2[i]];
                    move = s2[i];
                }
                cnt += 1;
            }
        }
        return move;
    }
protected:
    int tabuLength; //5

    int convTabu; // 5:  230*numpy.sqrt(pRegions)

    boost::unordered_map<std::pair<int, int>, double> neighSolutions;

    std::vector<int> regions;


};
#endif
