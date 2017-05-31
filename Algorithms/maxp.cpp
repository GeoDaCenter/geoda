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

Maxp::Maxp(const GalElement* _w,  const vector<vector<double> >& _z, int floor, vector<int> floor_variable, int initial, vector<int> seed)
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
    for (int i=0; i<regions.size(); i++ ) {
        for (int j=0; j<regions[i].size(); j++) {
            regions[i][j] ;
        }
    }
    for (int i=0; i<selected_z.size(); i++) {
        double var = GenUtils::StandardizeData(selected_z[i]);
        
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
            for (int i=0; i<num_obs;i++) candidates.push_back(i);
            //random_shuffle (candidates.begin(), candidates.end());
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
