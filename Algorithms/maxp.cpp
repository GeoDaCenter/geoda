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

#include <vector>
#include <map>
#include <list>

#include "../ShapeOperations/GalWeight.h"

#include "maxp.h"


using namespace std;

Maxp::Maxp(const GalElement* _w, vector<double> z, int floor, vector<int> floor_variable, int initial, vector<int> seed)
: w(_w), LARGE(1000000), MAX_ATTEMPTS(100)
{
    num_obs = floor_variable.size();
    // init solution
    
}

Maxp::~Maxp()
{
    
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
        vector<int> enclaves;
        list<int> candidates;
        if (seed.empty()) {
            // candidats = w.id_order
        } else {
            // candidates.extend(nonseeds)
        }
        
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
                            list<int>::iterator iter;
                            iter = find(candidates.begin(), candidates.end(), nbr);
                            bool in_cand = iter != candidates.end();
                            
                            //iter = find(region.begin(), region.end(), nbr);
                            //bool in_region = iter != region.end();
                            
                            
                        }
                    }
                }
            }
        }
    }
}
