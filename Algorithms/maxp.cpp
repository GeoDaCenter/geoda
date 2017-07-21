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

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "../ShapeOperations/GalWeight.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "cluster.h"
#include "maxp.h"

using namespace std;

Maxp::Maxp(const GalElement* _w,  const vector<vector<double> >& _z, int _floor, vector<vector<int> > _floor_variable, int _initial, vector<size_t> _seeds, int _rnd_seed, bool _test)
: w(_w), z(_z), floor(_floor), floor_variable(_floor_variable), initial(_initial), seeds(_seeds), LARGE(1000000), MAX_ATTEMPTS(100), rnd_seed(_rnd_seed), test(_test), initial_wss(_initial), regions_group(_initial), area2region_group(_initial), p_group(_initial)
{
    num_obs = z.size();
    num_vars = z[0].size();
    
    if (test) {
        initial = 2;
        floor = 5;
        init_test();
    }
    
    if (rnd_seed<0) {
        unsigned int initseed = (unsigned int) time(0);
        srand(initseed);
    } else {
        srand(rnd_seed);
    }
    
    // init solution
    init_solution();
    
    if (p == 0)
        feasible = false;
    else {
        feasible = true;
        
        double best_val = objective_function();
        vector<vector<int> > best_regions;
        map<int, int> best_area2region;

        int attemps = 0;
        
        // parallize following block, comparing the objective_function() return values
        //for (int i=0; i<initial; i++)  init_solution(i);
        run_threaded();
        
        for (int i=0; i<initial; i++) {
            vector<vector<int> >& current_regions = regions_group[i];
            map<int, int>& current_area2region = area2region_group[i];
            
            if (p_group[i] > 0) {
                double val = initial_wss[i];
                
                wxString str;
                str << "initial solution";
                str << i;
                str << val;
                str << best_val;
                LOG_MSG(str.ToStdString());
                
                if (val < best_val) {
                    best_regions = current_regions;
                    best_area2region = current_area2region;
                    best_val = val;
                }
                attemps += 1;
            }
        }
        
        regions = best_regions;
        p = regions.size();
        area2region = best_area2region;
        
        swap();
    }
}

Maxp::~Maxp()
{
    
}

void Maxp::run(int a, int b, uint64_t seed_start)
{
    //wxString msg = wxString::Format("Maxp:run(%d, %d)", a, b);
    //LOG_MSG(msg.mb_str());
    
    for (int i=a; i<b; i++) {
        init_solution(i, seed_start);
    }
}

void Maxp::run_threaded()
{
    int nCPUs = wxThread::GetCPUCount();
    int quotient = initial / nCPUs;
    int remainder = initial % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
    boost::thread_group threadPool;
    uint64_t seed_start = rnd_seed;
    for (int i=0; i<tot_threads; i++) {
        int a=0;
        int b=0;
        if (i < remainder) {
            a = i*(quotient+1);
            b = a+quotient;
        } else {
            a = remainder*(quotient+1) + (i-remainder)*quotient;
            b = a+quotient-1;
        }
        
        seed_start = seed_start + MAX_ATTEMPTS * (num_obs *3);
        boost::thread* worker = new boost::thread(boost::bind(&Maxp::run, this, a, b, seed_start));
        threadPool.add_thread(worker);
    }
    
    threadPool.join_all();
}

vector<vector<int> >& Maxp::GetRegions()
{
    return regions;
}

void Maxp::init_solution(int solution_idx, uint64_t seed_start)
{
    int p = 0;
    bool solving = true;
    int attempts = 0;
    double objective_val = 0;
    
    vector<vector<int> > _regions;
    map<int, int> _area2region;
    
    if (seed_start > 0) srand(seed_start);
    
    while (solving && attempts <= MAX_ATTEMPTS) {
        vector<vector<int> > regions;
        list<int> enclaves;
        list<int> candidates;
        if (seeds.empty()) {
            vector<int> _candidates;
            for (int i=0; i<num_obs;i++) _candidates.push_back(i);
            random_shuffle (_candidates.begin(), _candidates.end());
    
            if (test) _candidates = test_get_random();
            
            for (int i=0; i<num_obs;i++) candidates.push_back(_candidates[i]);
        } else {
            //nonseeds = [i for i in self.w.id_order if i not in seeds]
            // candidates.extend(nonseeds)
            map<int, bool> cand_dict;
            map<int, bool>::iterator it;
            for (int i=0; i<seeds.size(); i++) {
                cand_dict[ seeds[i] ] = true;
            }
            for (int i=0; i<num_obs; i++) {
                cand_dict[ i ] = true;
            }
            for (it = cand_dict.begin(); it != cand_dict.end(); it++) {
                candidates.push_back(it->first);
            }
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
                    for (int i=0; i<region.size(); i++) {
                        int area = region[i];
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
                        int neigID = rand() % potential.size();//(int) (uniform() * potential.size()); //0
                        if (test ) {
                            neigID = test_random_numbers.front();
                            test_random_numbers.pop_front();
                        }
                        
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
                a2r[ regions[i][j] ] = i;
            }
        }
        int encCount = enclaves.size();
        int encAttempts = 0;
        
        while (encCount > 0 && encAttempts != encCount) {
            int enclave = enclaves.front();
            enclaves.pop_front();
            
            vector<int> candidates;
            
            for ( int n=0; n<w[enclave].Size(); n++) {
                int nbr = w[enclave][n];
                
                iter = find(enclaves.begin(), enclaves.end(), nbr);
                if (iter != enclaves.end()) continue;
                
                int region = a2r[nbr];
                
                vector_iter = find(candidates.begin(), candidates.end(), region);
                if (vector_iter != candidates.end()) continue;

                candidates.push_back(region);
            }
            
            if (!candidates.empty()) {
                // add enclave to random region
                int regID = (int) (uniform() * candidates.size());
                if (test)   {
                    regID = enclave_random_number.front();
                    enclave_random_number.pop_front();
                }
                
                int rid = candidates[regID];
                
                regions[rid].push_back(enclave);
                a2r[enclave] = rid;
                
                // structure to loop over enclaves until no more joining is possible
                encCount = enclaves.size();
                encAttempts = 0;
                feasible = true;
                                       
            } else {
                // put back on que, no contiguous regions yet
                enclaves.push_back(enclave);
                encAttempts += 1;
                feasible = false;
            }
        }
        
        if (feasible) {
            solving = false;
            p = regions.size();
            _regions = regions;
            _area2region = a2r;
            objective_val = objective_function(_regions);
        } else {
            if (attempts == MAX_ATTEMPTS) {
                LOG_MSG("No initial solution found");
                p = 0;
            }
            attempts += 1;
        }
    }
    
    if (solution_idx >=0 ) {
        regions_group[solution_idx] = _regions;
        area2region_group[solution_idx] = _area2region;
        p_group[solution_idx] = p;
        initial_wss[solution_idx] = objective_val;
    } else {
        this->regions = _regions;
        this->area2region = _area2region;
        this->p = p;
    }
}

void Maxp::swap()
{
    bool swapping = true;
    int swap_iteration = 0;
    
    LOG_MSG("Initial solution, objective function:");
    
    int total_move = 0;
    int k = regions.size();
    
    vector<int>::iterator iter;
    vector<int> changed_regions(k, 1);
    
    // nr = range(k)
    
    while (swapping) {
        int moves_made = 0;
        // regionIds = [r for r in nr if changed_regions[r]]
        vector<int> regionIds;
        for (int r=0; r<k; r++) {
            if (changed_regions[r] >0) {
                regionIds.push_back(r);
            }
        }
        //random_shuffle(regionIds.begin(), regionIds.end());
        
        for (int r=0; r<k; r++) changed_regions[r] = 0;
        swap_iteration += 1;
        for (int i=0; i<regionIds.size(); i++) {
            int seed = regionIds[i];
            bool local_swapping = true;
            int local_attempts = 0;
            while (local_swapping) {
                int local_moves = 0;
                // get neighbors
                vector<int>& members = regions[seed];
                vector<int> neighbors;
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
                vector<int> candidates;
                for (int j=0; j<neighbors.size(); j++) {
                    int nbr = neighbors[j];
                    vector<int> block = regions[ area2region[ nbr ] ]; // deep copy
                    if (check_contiguity(w, block, nbr)) {
                        block.erase(remove(block.begin(),block.end(),nbr), block.end()); // remove frm vector
                        if (check_floor(block)) {
                            candidates.push_back(nbr);
                        }
                    }
                }
                // find the best local move
                if (candidates.empty()) {
                    local_swapping = false;
                } else {
                    int nc = candidates.size();
                    double cv = 0.0;
                    int best = 0;
                    bool best_found = false;
                    
                    for (int j=0; j<candidates.size(); j++) {
                        int area = candidates[j];
                        vector<int>& current_internal = regions[seed];
                        vector<int>& current_outter = regions[area2region[area]];
                        double change = objective_function_change(area, current_internal, current_outter);
                        if (change < cv) {
                            best = area;
                            cv = change;
                            best_found = true;
                        }
                    }
                    
                    if (best_found) {
                        // make the move
                        int area = best;
                        int old_region = area2region[area];
                        vector<int>& rgn = regions[old_region];
                        rgn.erase(remove(rgn.begin(),rgn.end(), area), rgn.end());
                        
                        area2region[area] = seed;
                        regions[seed].push_back(area);
                        moves_made += 1;
                        changed_regions[seed] = 1;
                        changed_regions[old_region] = 1;
                    } else {
                        // no move improves the solution
                        local_swapping = false;
                    }
                }
                local_attempts += 1;
            
            }
        }
        total_move += moves_made;
        if (moves_made == 0) {
            swapping = false;
            swap_iterations = swap_iteration;
            total_moves = total_move;
        }
    }
}

bool Maxp::check_floor(const vector<int>& region)
{
    // selectionIDs = [self.w.id_order.index(i) for i in region]
    double cv = 0;
    for (size_t i=0; i<region.size(); i++) {
        int selectionID = region[i];
        vector<int>& f_v = floor_variable[ selectionID ];
        for (size_t j=0; j<f_v.size(); j++) {
            cv += f_v[j];
        }
    }
    if (cv >= floor)
        return true;
    else
        return false;
}

double Maxp::objective_function()
{
    return objective_function(regions);
}

double Maxp::objective_function(const vector<vector<int> >& solution)
{
    // solution is a list of lists of region ids [[1,7,2],[0,4,3],...] such
    // that the first region has areas 1,7,2 the second region 0,4,3 and so
    // on. solution does not have to be exhaustive
    
    double wss = 0;
    
    // for every variable, calc the variance using selected neighbors
    
    
    for (int i=0; i<solution.size(); i++ ) {
        vector<vector<double> > selected_z(num_vars);
        for (int j=0; j<solution[i].size(); j++) {
            int selectionID = solution[i][j];
            for (int m=0; m<num_vars; m++) {
                selected_z[m].push_back( z[selectionID][m] );
            }
        }
        double sum = 0.0;
        for (int n=0; n<num_vars; n++) {
            double var = GenUtils::GetVariance(selected_z[n]);
            sum += var;
        }
        wss += sum * solution[i].size();
    }
    
    return wss;
}

double Maxp::objective_function(const vector<int>& current_internal, const vector<int>& current_outter)
{
    vector<vector<int> > composed_region;
    composed_region.push_back(current_internal);
    composed_region.push_back(current_outter);
    
    return objective_function(composed_region);
}


double Maxp::objective_function_change(int area, const vector<int>& current_internal, const vector<int>& current_outter)
{
    double current = objective_function(current_internal, current_outter);
    vector<int> new_internal = current_internal;
    vector<int> new_outter = current_outter;
    new_internal.push_back(area);
    new_outter.erase(remove(new_outter.begin(),new_outter.end(), area), new_outter.end());
    double new_val = objective_function(new_internal, new_outter);
    double change = new_val - current;
    return change;
}

bool Maxp::is_component(const GalElement *w, const vector<int> &ids)
{
    //Check if the set of ids form a single connected component
    int components = 0;
    map<int, int> marks;
    for (int i=0; i<ids.size(); i++) marks[ids[i]] = 0;
    
    list<int> q;
    list<int>::iterator iter;
    vector<int>::iterator vector_iter;
    for (int i=0; i<ids.size(); i++)
    {
        int node = ids[i];
        if (marks[node] == 0) {
            components += 1;
            q.push_back(node);
            if (components > 1)
                return false;
        }
        while (!q.empty()) {
            node = q.back();
            q.pop_back();
            marks[node] = components;
            for (int n=0; n<w[node].Size(); n++) {
                int nbr = w[node][n];
                if (marks.find(nbr) != marks.end()) {
                    if (marks[nbr] == 0 ) {
                        iter = find(q.begin(), q.end(), nbr);
                        if (iter == q.end()) {
                            q.push_back(nbr);
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool Maxp::check_contiguity(const GalElement* w, vector<int>& neighbors, int leaver)
{
    vector<int> ids = neighbors;
    ids.erase(remove(ids.begin(),ids.end(), leaver), ids.end());
    
    return is_component(w, ids);
}

void Maxp::init_test()
{
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(4);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(5);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(4);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(1);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(3);
    enclave_random_number.push_back(2);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(6);
    test_random_numbers.push_back(5);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(4);
    test_random_numbers.push_back(4);
    test_random_numbers.push_back(5);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(7);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    enclave_random_number.push_back(1);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(6);
    test_random_numbers.push_back(3);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(2);
    test_random_numbers.push_back(1);
    test_random_numbers.push_back(0);
    test_random_numbers.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(1);
    enclave_random_number.push_back(1);
    enclave_random_number.push_back(1);
    enclave_random_number.push_back(1);
    enclave_random_number.push_back(0);
    enclave_random_number.push_back(2);
    
    int test[48] = {6, 28, 32, 26, 41, 47, 5, 40, 18, 31, 0, 11, 36, 17, 43, 35, 33, 7, 45, 20, 25, 19, 21, 13, 1, 4, 12, 38, 22, 29, 9, 27, 16, 42, 46, 37, 14, 44, 2, 34, 30, 10, 15, 23, 39, 3, 24, 8};
    vector<int> cand;
    for (int i=0; i<num_obs;i++) cand.push_back(test[i]);
    test_random_cand.push_back(cand);
    
    int test1[48] = {37, 8, 27, 14, 17, 1, 35, 13, 28, 40, 32, 11, 45, 15, 29, 10, 24, 42, 6, 44, 16, 9, 46, 25, 18, 4, 21, 19, 2, 47, 26, 0, 39, 33, 12, 31, 20, 34, 30, 22, 38, 7, 5, 36, 43, 41, 3, 23};
    vector<int> cand1;
    for (int i=0; i<num_obs;i++) cand1.push_back(test1[i]);
    test_random_cand.push_back(cand1);
    
    int test2[48] = {44, 9, 8, 16, 27, 31, 45, 35, 19, 4, 42, 22, 11, 41, 12, 15, 32, 39, 25, 34, 6, 36, 5, 3, 21, 10, 0, 38, 26, 47, 17, 23, 43, 2, 30, 7, 37, 18, 28, 29, 24, 40, 46, 33, 14, 1, 20, 13};
    vector<int> cand2;
    for (int i=0; i<num_obs;i++) cand2.push_back(test2[i]);
    test_random_cand.push_back(cand2);
}

vector<int> Maxp::test_get_random()
{
    vector<int> val = test_random_cand.front();
    test_random_cand.pop_front();
    return val;
}
