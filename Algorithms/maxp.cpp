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
 * Created 5/30/2017 lixun910@gmail.com
 */

#include <algorithm>
#include <vector>
#include <map>
#include <list>
#include <cstdlib>

#include "../ShapeOperations/GalWeight.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "cluster.h"
#include "maxp.h"

using namespace std;

Maxp::Maxp(const GalElement* _w,  const vector<vector<double> >& _z, int floor, vector<vector<int> > floor_variable, int initial, vector<int> seed)
: w(_w), z(_z), LARGE(1000000), MAX_ATTEMPTS(100)
{
    num_obs = floor_variable.size();
    
    if (random_state<0) {
        unsigned int initseed = (unsigned int) time(0);
        srand(initseed);
    } else {
        srand(random_state);
    }

    
    // init solution
    init_solution();
    if (p != 0)
        feasible = false;
    else {
        feasible = true;
        double best_val = objective_function();
        // deep copy
        vector<vector<int> > current_regions = regions;
        map<int, int> current_area2region = area2region;
        vector<double> initial_wss;
        int attemps = 0;
        
        for (int i=0; i<initial; i++) {
            init_solution();
            if (p > 0) {
                double val = objective_function();
                initial_wss.push_back(val);
                
                wxString str;
                str << "initial solution";
                str << i;
                str << val;
                str << best_val;
                LOG_MSG(str.ToStdString());
                
                if (val < best_val) {
                    current_regions = regions;
                    current_area2region = area2region;
                    best_val = val;
                }
                attemps += 1;
            }
        }
        
        regions = current_regions;
        p = regions.size();
        area2region = current_area2region;
        
        swap();
    }
}

Maxp::~Maxp()
{
    
}

double Maxp::objective_function()
{
    double wss = 0;
    // for every variable, calc the variance using selected neighbors
    vector<vector<double> > selected_z;
    
    //for (int n=0; n<)
    for (int n=0; n<z.size(); n++) {
        vector<double> val;
        for (int i=0; i<regions.size(); i++ ) {
            for (int j=0; j<regions[i].size(); j++) {
                val.push_back( z[n][ regions[i][j] ]);
            }
        }
    }
    
    for (int n=0; n<z.size(); n++) {
        double var = GenUtils::StandardizeData(selected_z[n]);
        
    }
    return 0;
}

bool Maxp::check_floor(const vector<int>& region)
{
    double cv = 0;
    for (size_t i=0; i<region.size(); i++) {
        vector<int>& f_v = floor_variable[ region[i] ];
        for (size_t j=0; j<f_v.size(); j++) {
            cv += f_v[j];
        }
    }
    if (cv >= floor)
        return true;
    else
        return false;
}

void Maxp::init_solution()
{
    p = 0;
    bool solving = true;
    int attempts = 0;
    
    while (solving && attempts <= MAX_ATTEMPTS) {
        vector<vector<int> > regions;
        list<int> enclaves;
        list<int> candidates;
        if (seed.empty()) {
            vector<int> _candidates;
            for (int i=0; i<num_obs;i++) _candidates.push_back(i);
            random_shuffle (_candidates.begin(), _candidates.end());
            for (int i=0; i<num_obs;i++) candidates.push_back(_candidates[i]);
        } else {
            // candidates.extend(nonseeds)
        }
        
        list<int>::iterator iter;
        vector<int>::iterator vector_iter;
        
        while (!candidates.empty()) {
            int seed = candidates.front();
            candidates.pop_front();
            
            // try to grow it till threshold constraint is satisfied
            vector<int> region;
            region.push_back(seed);

            
            bool building_region = true;
            while (building_region) {
                // check if floor is satisfied
                if (check_floor(region)) {
                    regions.push_back(region);
                    building_region = false;
                } else {
                    vector<int> potential;
                    for (int area=0; area<region.size(); area++) {
                        for ( int n=0; n<w[area].Size(); n++) {
                            int nbr = w[area][n];
                            
                            iter = find(candidates.begin(), candidates.end(), nbr);
                            if (iter == candidates.end()) continue;
                            
                            vector_iter = find(region.begin(), region.end(), nbr);
                            if (vector_iter != region.end()) continue;
                            
                            vector_iter = find(potential.begin(), potential.end(), nbr);
                            if (vector_iter != potential.end()) continue;
                            
                            potential.push_back(nbr);
                        }
                    }
                    if (!potential.empty()) {
                        // add a random neighbor
                        int neigID = (int) (uniform() * potential.size());
                        int neigAdd = potential[neigID];
                        potential.erase(potential.begin() + neigID);
                        region.push_back(neigAdd);
                        // remove it from candidate
                        candidates.remove(neigAdd);
                    } else {
                        for (int i=0; i<region.size(); i++) {
                            enclaves.push_back(region[i]);
                        }
                        building_region = false;
                    }
                }
            }
        }
        // check to see if any regions were made before going to enclave stage
        bool feasible =false;
        if (!regions.empty())
            feasible = true;
        else {
            attempts += 1;
            break;
        }
        // self.enclaves = enclaves[:]
        map<int, int> a2r;
        for (int i=0; i<regions.size(); i++) {
            for (int j=0; j<regions[i].size(); j++) {
                a2r[j] = i;
            }
        }
        int encCount = enclaves.size();
        int encAttempts = 0;
        
        while (encCount > 0 && encAttempts != encCount) {
            int enclave = enclaves.front();
            enclaves.pop_front();
            for ( int n=0; n<w[enclave].Size(); n++) {
                int nbr = w[enclave][n];
                
                iter = find(enclaves.begin(), enclaves.end(), nbr);
                if (iter != enclaves.end()) continue;
                
                int region = a2r[nbr];
                
                iter = find(candidates.begin(), candidates.end(), region);
                if (iter != candidates.end()) continue;

                candidates.push_back(region);
            }
            
            if (!candidates.empty()) {/*
                // add enclave to random region
                int neigID = (int) (uniform() * candidates.size());
                iter = candidates.begin();
                std::advance(iter, neigID);
                int rid = *iter;
                regions[rid].push_back(enclave);
                a2r[enclave] = rid;
                // structure to loop over enclaves until no more joining is possible
                encCount = enclaves.size();
                encAttempts = 0;
                feasible = true;
                                        */
            } else {
                // put back on que, no contiguous regions yet
                enclaves.push_back(enclave);
                encAttempts += 1;
                feasible = false;
            }
            
            if (feasible) {
                solving = false;
                p = regions.size();
            } else {
                if (attempts == MAX_ATTEMPTS) {
                    LOG_MSG("No initial solution found");
                    p = 0;
                }
                attempts += 1;
            }
        }

    }
}

void Maxp::swap()
{
    bool swapping = true;
    int swap_iteration = 0;
    int total_moves = 0;
    int k = regions.size();
    
    vector<int>::iterator iter;
    vector<int> changed_regions(k, 1);
    while (swapping) {
        int moves_made = 0;
        vector<int> regionIds;
        for (int r=0; r<k; r++) {
            if (changed_regions[r] >0) {
                regionIds.push_back(r);
            }
        }
        random_shuffle(regionIds.begin(), regionIds.end());
        for (int r=0; r<k; r++) changed_regions[r] = 0;
        swap_iteration += 1;
        for (int i=0; i<regionIds.size(); i++) {
            int seed = regionIds[i];
            bool local_swapping = true;
            int local_attempts = 0;
            while (local_swapping) {
                int local_moves = 0;
                // get neighbors
                vector<int> neighbors;
                vector<int>& members = regions[seed];
                for (int j=0; j<members.size(); j++) {
                    int member = members[j];
                    for (int k=0; k<w[member].Size(); k++) {
                        int candidate = w[member][k];
                        iter = find(members.begin(), members.end(), candidate);
                        if (iter != members.end()) continue;// not in members
                        iter = find(neighbors.begin(), neighbors.end(), candidate);
                        if (iter != neighbors.end()) continue; // not in neighbors
                        neighbors.push_back(candidate);
                    }
                }
                for (int j=0; j<neighbors.size(); j++) {
                    int nbr = neighbors[j];
                    vector<int> block = regions[ area2region[ nbr ] ]; // deep copy
                    //if (check_contiguity(block, neighbor)) {
                    //    block.erase(neighbor);
                    //}
                }
            }
        }
    }
}

bool Maxp::is_component(const GalElement *w, const vector<int> &ids)
{
    int components = 0;
    map<int, int> masks;
    for (int i=0; i<ids.size(); i++) masks[ids[i]] = 0;
    
    vector<int> q;
    for (int i=0; i<ids.size(); i++)
    {
        int node = ids[i];
    }
    return false;
}
