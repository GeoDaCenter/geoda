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
#include <boost/heap/priority_queue.hpp>

//#include <tr1/type_traits>

#include "../ShapeOperations/GalWeight.h"
#include "rng.h"
#include "DataUtils.h"

typedef boost::unordered_map<int, boost::unordered_map<int, bool> > REGION_AREAS;

////////////////////////////////////////////////////////////////////////////////
////// ZoneControl
////////////////////////////////////////////////////////////////////////////////
class ZoneControl {
public:
    // A ZoneControl is for one variable (e.g. population), and it contains
    // the values of the variable
    ZoneControl(const std::vector<double>& in_data);
    ZoneControl(int n, double* in_data) {
        for (int i=0; i<n; ++i) {
            data.push_back(in_data[i]);
        }
    }

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
    bool SatisfyLowerBound(boost::unordered_map<int, bool>& candidates);

    bool CheckBound(boost::unordered_map<int, bool>& candidates);

    bool CheckRemove(int area, boost::unordered_map<int, bool>& candidates);

    // Check if a candidate zone satisfies upper bound when adding a area
    bool CheckAdd(int area, boost::unordered_map<int, bool>& candidates);

    double getZoneValue(int i, boost::unordered_map<int, bool>& candidates);

protected:
    std::vector<double> data;

    std::vector<Operation> operations;

    std::vector<Comparator> comparators;

    std::vector<double> comp_values;
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
    double getDistance2Region(int area, int region, REGION_AREAS& regions);

    // update the centroid if region changed (remove/add area)
    void updateRegionCentroids(int region, REGION_AREAS& regions);

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

    // cache the cetnroids for regions, any change of the region should
    // call updateRegionCentroids()
    std::map<int, std::vector<double> > region_centroids;

};

////////////////////////////////////////////////////////////////////////////////
/// ObjectiveFunction
////////////////////////////////////////////////////////////////////////////////
// ObjectiveFunction: the target of the AZP is to minimize the objective function
// of clustering results. E.g. sum of squares
class ObjectiveFunction
{
public:
    ObjectiveFunction(int _n, int _m, double** _data, GalElement* _w, REGION_AREAS& _regions)
    : n(_n), m(_m), data(_data), w(_w), regions(_regions) {}
    virtual ~ObjectiveFunction() {}

    virtual double GetValue() {
        // Calculate the value of the objective function
        double ss = 0; // e.g. sum of squares
        REGION_AREAS::iterator it;
        for (it = regions.begin(); it != regions.end(); ++it) {
            int region = it->first;
            if (region_of.find(region) == region_of.end()) {
                // objective function of region needs to be computed
                double obj = getObjectiveValue(regions[region]);
                region_of[region] = obj;
            }
            ss += region_of[region];
        }
        return ss;
    }
    
    virtual double GetRawValue() {
        // Calculate the value of the objective function
        double ss = 0; // e.g. sum of squares
        REGION_AREAS::iterator it;
        for (it = regions.begin(); it != regions.end(); ++it) {
            int region = it->first;
            // objective function of region needs to be computed
            double obj = getObjectiveValue(regions[region]);
            ss += obj;
        }
        return ss;
    }
    

    virtual void UpdateRegions() {
        // region changes, update it's
        REGION_AREAS::iterator it;
        for (it = regions.begin(); it != regions.end(); ++it) {
            int region = it->first;
            double obj = getObjectiveValue(regions[region]);
            region_of[region] = obj;
        }
    }

    virtual void UpdateRegion(int region) {
        // region changes, update it's
        REGION_AREAS::iterator it;
        for (it = regions.begin(); it != regions.end(); ++it) {
            int r = it->first;
            if (r == region) {
                double obj = getObjectiveValue(regions[region]);
                region_of[region] = obj;
            }
        }
    }

    virtual double getObjectiveValue(boost::unordered_map<int, bool>& areas) {
        // compute centroid of this set of areas
        std::vector<double> dataAvg(m, 0);
        boost::unordered_map<int, bool>::iterator sit;
        for (sit = areas.begin(); sit != areas.end(); ++sit) {
            int idx = sit->first;
            for (int j=0; j<m; ++j) {
                dataAvg[j] += data[idx][j];
            }
        }
        double n_areas = (double)areas.size();
        for (int j=0; j<m; ++j) {
            dataAvg[j] /= n_areas;
        }
        // distance from each area in this region to centroid
        double obj = 0;

        for (sit = areas.begin(); sit != areas.end(); ++sit) {
            int idx = sit->first;
            double dist = DataUtils::EuclideanDistance(data[idx], dataAvg);
            obj += dist;
        }
        return obj;
    }

    virtual double TabuSwap(int area, int from_region, int to_region) {
        // try to swap area to region, compute the value of objective function
        // no phyical swap happens
        boost::unordered_map<int, bool> from_areas = regions[from_region];
        boost::unordered_map<int, bool> to_areas = regions[to_region];
        from_areas.erase(area);
        to_areas[area] = false;

        double ss_from = getObjectiveValue(from_areas);
        double ss_to = getObjectiveValue(to_areas);

        double ss = GetValue();
        double delta = ss_from + ss_to - region_of[from_region] - region_of[to_region];
        double new_ss = ss + delta;

        return new_ss;
    }

    virtual std::pair<double, bool> TrySwap(int area, int from_region, int to_region) {
        // try to swap area to region, compute the value of objective function
        // phyical swap could happen if contiguity check is passed
        boost::unordered_map<int, bool> from_areas = regions[from_region];
        boost::unordered_map<int, bool> to_areas = regions[to_region];
        from_areas.erase(area);
        to_areas[area] = false;

        double ss_from = getObjectiveValue(from_areas);
        double ss_to = getObjectiveValue(to_areas);

        double delta = ss_from + ss_to - region_of[from_region] - region_of[to_region];
        if (delta <= 0) {
            // improved
            if (checkFeasibility(from_region, area)) {
                region_of[from_region] = ss_from;
                region_of[to_region] = ss_to;
                // confirm swap, lock
                // update values for two changed regions
                regions[from_region] = from_areas;
                regions[to_region] = to_areas;
                return std::make_pair(delta, true);
            }
        }
        return std::make_pair(delta, false);
    }

    virtual std::pair<double, bool> TrySwapSA(int area, int from_region, int to_region, double best_of) {
        // try to swap area to region, compute the value of objective function
        // phyical swap could happen if contiguity check is passed
        boost::unordered_map<int, bool> from_areas = regions[from_region];
        boost::unordered_map<int, bool> to_areas = regions[to_region];
        from_areas.erase(area);
        to_areas[area] = false;
        
        double ss_from = getObjectiveValue(from_areas);
        double ss_to = getObjectiveValue(to_areas);

        double ss = GetValue();
        double delta = ss_from + ss_to - region_of[from_region] - region_of[to_region];
        double new_ss = ss + delta;

        if (new_ss <= best_of) {
            // improved
            if (checkFeasibility(from_region, area)) {
                region_of[from_region] = ss_from;
                region_of[to_region] = ss_to;
                // confirm swap, lock
                // update values for two changed regions
                regions[from_region] = from_areas;
                regions[to_region] = to_areas;
                return std::make_pair(new_ss, true);
            }
        }
        return std::make_pair(new_ss, false);
    }

    virtual double MakeMove(int area, int from_region, int to_region) {
        // use reference here to make actual change
        boost::unordered_map<int, bool>& from_areas = regions[from_region];
        boost::unordered_map<int, bool>& to_areas = regions[to_region];
        
        if (from_areas.size() <=1) {
            // has to make sure each region has at least one area
            return 0;
        }
        from_areas.erase(area);
        to_areas[area] = false;

        double ss_from = getObjectiveValue(from_areas);
        double ss_to = getObjectiveValue(to_areas);

        region_of[from_region] = ss_from;
        region_of[to_region] = ss_to;
        // update values for two changed regions
        regions[from_region] = from_areas;
        regions[to_region] = to_areas;

        return GetValue();
    }

    bool checkFeasibility(int regionID, int areaID, bool is_remove = true)
    {
        // Check feasibility from a change region
        boost::unordered_map<int, bool> areas2Eval = regions[regionID];

        if (is_remove) {
            // removing an area from a region
            areas2Eval.erase(areaID);
        } else {
            // adding an area from a region)
            areas2Eval[areaID] = true;
        }
        
        if (areas2Eval.empty()) {
            return false;
        }
        
        // remove the area first
        int seedArea = areas2Eval.begin()->first;

        // then, start from 1st object, do BFS
        std::stack<int> processed_ids;
        processed_ids.push(seedArea);
        while (processed_ids.empty() == false) {
            int fid = processed_ids.top();
            processed_ids.pop();
            areas2Eval.erase(fid); // remove area from current group as processed
            const std::vector<long>& nbrs = w[fid].GetNbrs();
            for (int i=0; i<nbrs.size(); i++ ) {
                int nid = nbrs[i];
                if (areas2Eval.find(nid) != areas2Eval.end()) {
                    // only processed the neighbor in current group
                    processed_ids.push(nid);
                }
            }
        }
        // all should be removed if all connected
        return areas2Eval.empty();
    }
    
protected:
    // n: number of observations
    int n;

    // m is the dimension of the variable space
    int m;

    // w defines the weights structure, who's who's neighbor
    GalElement* w;

    // original row-wise data
    double** data;

    // cache the cetnroids for regions, any change of the region should
    // call updateRegionCentroids()
    std::map<int, double > region_of;

    // a reference to region data: region2Area
    REGION_AREAS& regions;
};

////////////////////////////////////////////////////////////////////////////////
////// RegionMaker
////////////////////////////////////////////////////////////////////////////////
class RegionMaker
{
public:
    // for p-region problem
    RegionMaker(int p, GalElement* const w,
                double** data, // row-wise
                RawDistMatrix* dist_matrix,
                int n, int m, const std::vector<ZoneControl>& c,
                const std::vector<int>& init_regions=std::vector<int>(),
                long long seed=123456789);

    virtual ~RegionMaker();

    // Sets the initial seeds for clustering
    void setSeeds(std::vector<int> seeds);
    
    virtual void LocalImproving()  {}
    
    virtual std::vector<int> GetResults() { return std::vector<int>();}

    virtual double GetInitObjectiveFunction() { return 0;}

    virtual double GetFinalObjectiveFunction() { return 0;}

    // Check is_control_satisfied
    bool IsSatisfyControls();

    int GetPRegions() { return region2Area.size();}

    
    void Copy(RegionMaker& rm);
    
protected:
    // Return the areas of a region
    //boost::unordered_map<int, bool> returnRegion2Area(int regionID);

    // return regions created
    std::vector<int> returnRegions();

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
    bool assignArea(int areaID, int regionID);

    std::set<int> getBufferingAreas(boost::unordered_map<int, bool>& areas);

    // Get bordering areas of a region
    void getBorderingAreas(int regionID);

    // Get possible move of an area (should be a bordering area)
    std::set<int> getPossibleMove(int area);

    bool growRegion();

    void InitFromRegion(std::vector<int>& init_regions);
    
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

    ObjectiveFunction* objective_function;

    std::vector<ZoneControl> controls;

    Xoroshiro128Random rng;
    
    bool is_control_satisfied;
    
public:
    // for copy
    std::vector<int> init_regions;

    boost::unordered_map<int, bool> unassignedAreas;

    boost::unordered_map<int, bool> assignedAreas;

    // area without neighbor (islands)
    boost::unordered_map<int, bool> areaNoNeighbor;

    boost::unordered_map<int, int> area2Region;

    // REGION region2Area
    boost::unordered_map<int, boost::unordered_map<int, bool> > region2Area;

    // For initial regions
    // area that could be assigned to which regions
    std::map<int, std::set<int> > potentialRegions4Area;
    
    // area --(assign to)--region : distance
    std::map<std::pair<int, int>, double> candidateInfo;


    // object function value
    double objInfo;
};

////////////////////////////////////////////////////////////////////////////////
////// MaxpRegionMaker
////////////////////////////////////////////////////////////////////////////////
class MaxpRegionMaker : public RegionMaker
{
public:
    // for max-p problem
    // the init_regions is different than RegionMaker's init_regions
    // e.g. it could be LISA's core objects, and they are used to
    // grow the potential regions
    MaxpRegionMaker(GalElement* const w,
                double** data, // row-wise
                RawDistMatrix* dist_matrix,
                int n, int m, const std::vector<ZoneControl>& c,
                const std::vector<int>& init_areas=std::vector<int>(),
                long long seed=123456789);

    virtual ~MaxpRegionMaker() {
        if (objective_function) {
            delete objective_function;
            objective_function = 0;
        }
    }

    virtual void LocalImproving() {}

    virtual std::vector<int> GetResults() {
        return this->returnRegions();
    }

    virtual double GetInitObjectiveFunction() {
         return this->objInfo;
    }

    virtual double GetFinalObjectiveFunction() {
        return this->objInfo;
    }

protected:
    std::vector<int> returnRegions();
    
    void InitSolution();

protected:
    std::vector<int> init_areas;
};


////////////////////////////////////////////////////////////////////////////////
////// MaxpRegion
////////////////////////////////////////////////////////////////////////////////
class MaxpRegion : public RegionMaker
{
    std::vector<int> final_solution;

    double initial_objectivefunction;

    double final_objectivefunction;

public:
    MaxpRegion(int max_iter, GalElement* const w,
               double** data, // row-wise
               RawDistMatrix* dist_matrix,
               int n, int m, const std::vector<ZoneControl>& c, int inits=0,
               const std::vector<int>& init_areas=std::vector<int>(),
               long long seed=123456789);

    virtual ~MaxpRegion() {}

    virtual void LocalImproving() {}

    virtual std::vector<int> GetResults() {
        return final_solution;
    }

    virtual double GetInitObjectiveFunction() {
        return initial_objectivefunction;
    }

    virtual double GetFinalObjectiveFunction() {
        return final_objectivefunction;
    }

    void RunAZP(std::vector<int>& solution, long long seed, int i);
    
    void RunConstruction(long long seed);
    
protected:
    std::vector<int> init_areas;

    int max_iter;
    
    std::map<double, std::vector<int> > candidates;
    
    int largest_p;
    
    double best_of;
    
    std::vector<int> best_result;
    
    boost::mutex mutex;
};

class MaxpSA : public RegionMaker
{
    std::vector<int> final_solution;

    double initial_objectivefunction;

    double final_objectivefunction;

public:
    MaxpSA(int max_iter, GalElement* const w,
               double** data, // row-wise
               RawDistMatrix* dist_matrix,
               int n, int m, const std::vector<ZoneControl>& c,
               double alpha = 0.85, int sa_iter= 1, int inits=0,
               const std::vector<int>& init_regions=std::vector<int>(),
               long long seed=123456789);

    virtual ~MaxpSA() {}

    virtual void LocalImproving() {}

    virtual std::vector<int> GetResults() {
        return final_solution;
    }

    virtual double GetInitObjectiveFunction() {
        return initial_objectivefunction;
    }

    virtual double GetFinalObjectiveFunction() {
        return final_objectivefunction;
    }
    
    void RunAZP(std::vector<int>& solution, long long seed, int i);
    
    void RunConstruction(long long seed);

protected:
    std::vector<int> init_areas;

    int max_iter;
    
    double temperature;
    
    double alpha;
    
    int sa_iter;
    
    int largest_p;
    
    double best_of;
    
    std::vector<int> best_result;
    
    std::map<double, std::vector<int> > candidates;
    
    boost::mutex mutex;
};

class MaxpTabu : public RegionMaker
{
    std::vector<int> final_solution;

    double initial_objectivefunction;

    double final_objectivefunction;

public:
    MaxpTabu(int max_iter, GalElement* const w,
               double** data, // row-wise
               RawDistMatrix* dist_matrix,
               int n, int m, const std::vector<ZoneControl>& c,
                int tabu_length=10, int inits=0,
               const std::vector<int>& init_areas=std::vector<int>(),
               long long seed=123456789);

    virtual ~MaxpTabu() {}

    virtual void LocalImproving() {}

    virtual std::vector<int> GetResults() {
        return final_solution;
    }

    virtual double GetInitObjectiveFunction() {
        return initial_objectivefunction;
    }

    virtual double GetFinalObjectiveFunction() {
        return final_objectivefunction;
    }

    void RunAZP(std::vector<int>& solution, long long seed, int i);
    
    void RunConstruction(long long seed);
    
protected:
    std::vector<int> init_areas;

    int max_iter;
    
    int tabuLength; //5
    
    int convTabu;
    
    int largest_p;
    
    double best_of;
    
    std::vector<int> best_result;
    
    std::map<double, std::vector<int> > candidates;
    
    boost::mutex mutex;
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
        int n, int m, const std::vector<ZoneControl>& c, int inits=0,
        const std::vector<int>& init_regions=std::vector<int>(),
        long long seed=123456789)
    : RegionMaker(p,w,data,dist_matrix,n,m,c,init_regions, seed)
    {
        if (inits > 0) {
            // ARiSeL
            for (int i=0; i<inits-1; ++i) {
                RegionMaker rm(p,w,data,dist_matrix,n,m,c,init_regions, seed + i);
                if (rm.objInfo < this->objInfo)  {
                    // better initial solution
                    this->Copy(rm);
                }
            }
        }
        initial_objectivefunction = this->objInfo;
        double best_score = this->objInfo;
        bool improvement = true;
        while (improvement) {
            this->LocalImproving();
            improvement = this->objInfo < best_score;
            best_score = this->objInfo;
        }

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
          int n, int m, const std::vector<ZoneControl>& c,
          double _alpha = 0.85, int _max_iter= 1, int inits=0,
          const std::vector<int>& init_regions=std::vector<int>(),
          long long seed=123456789)
    : RegionMaker(p,w,data,dist_matrix,n,m,c,init_regions,seed), temperature(1.0),
    alpha(_alpha), max_iter(_max_iter)
    {
        if (inits > 0) {
            // ARiSeL
            for (int i=0; i<inits-1; ++i) {
                RegionMaker rm(p,w,data,dist_matrix,n,m,c,init_regions, seed + i);
                if (rm.objInfo < this->objInfo)  {
                    // better initial solution
                    this->Copy(rm);
                }
            }
        }
        
        std::vector<int> init_sol = this->returnRegions();
        initial_objectivefunction = this->objInfo;

        // local search
        BasicMemory basicMemory, localBasicMemory;
        basicMemory.updateBasicMemory(this->objInfo, this->returnRegions());
        
        // step a
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
            //std::cout << basicMemory.objInfo << std::endl;
            // step c
            temperature *= alpha; // annealing
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
struct CompareTabuMove
{
public:
    bool operator() (const double& lhs,
                     const double& rhs) const
    {
        return lhs > rhs;
    }
};

class AZPTabu : public RegionMaker
{
    std::vector<int> final_solution;
    double initial_objectivefunction;
    double final_objectivefunction;

public:
    AZPTabu(int p, GalElement* const w,
            double** data, // row-wise
            RawDistMatrix* dist_matrix,
            int n, int m, const std::vector<ZoneControl>& c,
            int tabu_length=10, int _convTabu=0,  int inits = 0,
            const std::vector<int>& init_regions=std::vector<int>(),
            long long seed=123456789)
    : RegionMaker(p,w,data,dist_matrix,n,m,c,init_regions, seed),
    tabuLength(tabu_length), convTabu(_convTabu)
    {
        if (inits > 0) {
            // ARiSeL
            for (int i=0; i<inits-1; ++i) {
                RegionMaker rm(p,w,data,dist_matrix,n,m,c,init_regions, seed + i);
                if (rm.objInfo < this->objInfo)  {
                    // better initial solution
                    this->Copy(rm);
                }
            }
        }
        
        if (tabuLength <= 0) {
            tabuLength = 10;
        }
        if (convTabu <= 0) {
            convTabu = 10;
        }
        initial_objectivefunction = this->objInfo;
        std::vector<int> init_sol = this->returnRegions();

        this->LocalImproving();

        final_solution = this->regions;
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

    void updateNeighSolution(int area, int from, int to);

protected:
    int tabuLength; //5

    int convTabu; // 5:  230*numpy.sqrt(pRegions)?

    boost::unordered_map<std::pair<int, int>, double> neighSolutions;

    boost::heap::priority_queue<double, boost::heap::compare<CompareTabuMove> > neighSolObjs;

    std::vector<int> regions;


};
#endif
