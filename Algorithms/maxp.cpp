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
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "../ShapeOperations/GalWeight.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "maxp.h"

using namespace boost;
using namespace std;

Maxp::Maxp(const GalElement* _w,  const vector<vector<double> >& _z, double _floor, double* _floor_variable, int _initial, vector<wxInt64> _seeds, int _method, int _tabu_length, double _cool_rate,int _rnd_seed, char _dist,  bool _test )
: w(_w), z(_z), floor(_floor), floor_variable(_floor_variable), initial(_initial),  LARGE(1000000), MAX_ATTEMPTS(100), rnd_seed(_rnd_seed), test(_test), initial_wss(_initial), regions_group(_initial), area2region_group(_initial), p_group(_initial), dist(_dist), best_ss(DBL_MAX), method(_method), tabu_length(_tabu_length), cooling_rate(_cool_rate)
{
    num_obs = z.size();
    num_vars = z[0].size();

    if (test) {
        initial = 2;
        floor = 5;
    }

    // setup random number
    if (rnd_seed<0) {
        unsigned int initseed = (unsigned int) time(0);
        srand(initseed);
    } else {
        srand(rnd_seed);
    }
    seed_start = rand();
    seed_increment = MAX_ATTEMPTS * num_obs * 10000;
    
    // init solution
    if (_seeds.empty()) {
        init_solution(-1);
    } else {
        map<int, vector<int> > region_dict;
        for (int i=0; i< _seeds.size(); i++) {
            int rgn = _seeds[i];
            this->area2region[i] = rgn;
            if (region_dict.find(rgn) == region_dict.end()) {
                vector<int> ids;
                ids.push_back(i);
                region_dict[rgn] = ids;
            } else {
                region_dict[rgn].push_back(i);
            }
        }
        map<int, vector<int> >::iterator it;
        for (it = region_dict.begin(); it!= region_dict.end(); it++) {
            this->regions.push_back(it->second);
        }
        this->p = this->regions.size();
        
        GenUtils::sort(_seeds, _seeds, seeds);
    }
    
    if (p == 0)
        feasible = false;
    else {
        feasible = true;
        
        best_ss = objective_function();
        vector<vector<int> > best_regions;
        unordered_map<int, int> best_area2region;

        int attemps = 0;
        
        // parallize following block, comparing the objective_function() return values
        //for (int i=0; i<initial; i++)  init_solution(i);
        run_threaded();
        
        for (int i=0; i<initial; i++) {
            vector<vector<int> >& current_regions = regions_group[i];
            unordered_map<int, int>& current_area2region = area2region_group[i];
            
            //print_regions(current_regions);
            LOG_MSG(initial_wss[i]);
            
            if (p_group[i] > 0) {
                double val = initial_wss[i];
                
                if (val < best_ss) {
                    best_regions = current_regions;
                    best_area2region = current_area2region;
                    best_ss = val;
                }
                attemps += 1;
            }
        }
        
        if (!best_regions.empty()) {
            regions = best_regions;
            p = regions.size();
            area2region = best_area2region;
        }
    }
}

Maxp::~Maxp()
{
    
}

wxString Maxp::print_regions(vector<vector<int> >& _regions)
{
    wxString txt;
    txt << "regions:\n";
    for (int i=0; i<_regions.size(); i++) {
        txt << "(" << i+1 << "):";
        for (int j=0; j< _regions[i].size(); j++) {
            txt << _regions[i][j] << ", ";
        }
        txt << "\n";
    }
    return txt;
}

void Maxp::run(int a, int b)
{
    for (int i=a; i<=b; i++) {
        init_solution(i);
    }
}

void Maxp::run_threaded()
{
    int nCPUs = boost::thread::hardware_concurrency();;
    int quotient = initial / nCPUs;
    int remainder = initial % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
    boost::thread_group threadPool;
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
        boost::thread* worker = new boost::thread(boost::bind(&Maxp::run,this,a,b));
        threadPool.add_thread(worker);
    }
    
    threadPool.join_all();
}

vector<vector<int> >& Maxp::GetRegions()
{
    return regions;
}

void Maxp::init_solution(int solution_idx)
{
    uint64_t seed_local = seed_start + (solution_idx+1) * seed_increment;
    int p = 0;
    bool solving = true;
    int attempts = 0;
    
    vector<vector<int> > _regions;
    unordered_map<int, int> _area2region;
    
    while (solving && attempts <= MAX_ATTEMPTS) {
        vector<vector<int> > regn;
        list<int> enclaves;
        list<int> candidates;
        unordered_map<int, bool> candidates_dict;
        
        if (seeds.empty()) {
            vector<int> _candidates(num_obs);
            for (int i=0; i<num_obs;i++) _candidates[i] = i;
            
            //random_shuffle (_candidates.begin(), _candidates.end());
            for (int i=num_obs-1; i>=1; --i) {
                int k = Gda::ThomasWangHashDouble(seed_local++) * (i+1);
                while (k>=i) k = Gda::ThomasWangHashDouble(seed_local++) * (i+1);
                if (k != i) std::iter_swap(_candidates.begin() + k, _candidates.begin()+i);
            }
            for (int i=0; i<num_obs;i++) {
                candidates.push_back( _candidates[i] );
                candidates_dict[ _candidates[i] ] = true;
            }
        } else {
            //nonseeds = [i for i in self.w.id_order if i not in seeds]
            // candidates.extend(nonseeds)
            unordered_map<int, bool> cand_dict;
            unordered_map<int, bool>::iterator it;
            for (int i=0; i<seeds.size(); i++) {
                cand_dict[ seeds[i] ] = true;
            }
            for (int i=0; i<num_obs; i++) {
                cand_dict[ i ] = true;
            }
            for (it = cand_dict.begin(); it != cand_dict.end(); it++) {
                candidates.push_back(it->first);
                candidates_dict[ it->first ] = true;
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
            unordered_map<int, bool> region_dict;
            region_dict[seed] = true;
            
            // check floor and enclave
            bool is_enclave = true;
            double cv = floor_variable[ seed ];
           
            while (is_enclave && cv < floor && !region.empty()) {
                int area = region.back();
                region.pop_back();
               
                for ( int n=0; n<w[area].Size(); n++) {
                    int nbr = w[area][n];
                    if (region_dict[nbr] != true && candidates_dict[nbr] == true) {
                        region.push_back(nbr);
                        region_dict[nbr] = true;
                        candidates.remove(nbr);
                        candidates_dict[nbr] = false;
                        cv += floor_variable[ nbr];
                        if (cv >= floor) {
                            is_enclave = false;
                            break;
                        }
                    }
                }
            }
            
            unordered_map<int, bool>::iterator rit;
            if (is_enclave) {
                for (rit=region_dict.begin(); rit!=region_dict.end();rit++) {
                    if (rit->second) enclaves.push_back(rit->first);
                }
            } else {
                vector<int> _region;
                for (rit=region_dict.begin(); rit!=region_dict.end();rit++) {
                    if (rit->second) _region.push_back(rit->first);
                }
                regn.push_back(_region);
            }
        }
        // check to see if any regions were made before going to enclave stage
        bool feasible =false;
        if (!regn.empty())
            feasible = true;
        else {
            attempts += 1;
            break;
        }
        // self.enclaves = enclaves[:]
        unordered_map<int, int> a2r;
        for (int i=0; i<regn.size(); i++) {
            for (int j=0; j<regn[i].size(); j++) {
                a2r[ regn[i][j] ] = i;
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
                int regID = Gda::ThomasWangHashDouble(seed_local++) * candidates.size();
                
                int rid = candidates[regID];
                
                regn[rid].push_back(enclave);
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
            double ss = objective_function(regn);
            if (ss < best_ss) { // just need to be better than first initial solution
                solving = false;
                p = regn.size();
                _regions = regn;
                _area2region = a2r;
            }
        } else {
            if (attempts == MAX_ATTEMPTS) {
                LOG_MSG("No initial solution found");
                p = 0;
            }
        }
        attempts += 1;
    }
    
    if (solution_idx >=0) {
        if (_regions.empty()) {
            p_group[solution_idx] = 0;
            initial_wss[solution_idx] = 0;
        } else {
            // apply local search
            if (method == 0) {
                swap(_regions, _area2region, seed_local);
            } else if (method == 1) {
                tabu_search(_regions, _area2region, tabu_length, seed_local);
            } else {
                double temperature = 1.0;
                simulated_annealing(_regions, _area2region, cooling_rate, temperature, seed_local);
            }
            
            regions_group[solution_idx] = _regions;
            area2region_group[solution_idx] = _area2region;
            p_group[solution_idx] = p;
            initial_wss[solution_idx] = objective_function(_regions);
        }
    } else {
        if (this->regions.empty()) {
            this->regions = _regions;
            this->area2region = _area2region;
            this->p = p;
        } else {
            best_ss = objective_function();
            if (objective_function(_regions) < best_ss) {
                this->regions = _regions;
                this->area2region = _area2region;
                this->p = p;
            }
        }
        
    }
}

void Maxp::shuffle(vector<int>& arry, uint64_t& seed)
{
    //random_shuffle
    for (int i=arry.size()-1; i>=1; --i) {
        int k = Gda::ThomasWangHashDouble(seed++) * (i+1);
        while (k>=i) k = Gda::ThomasWangHashDouble(seed++) * (i+1);
        if (k != i) std::iter_swap(arry.begin() + k, arry.begin()+i);
    }
}


void Maxp::simulated_annealing(vector<vector<int> >& init_regions, unordered_map<int, int>& init_area2region, double alpha, double temperature, uint64_t seed_local)
{
    vector<vector<int> > local_best_solution;
    unordered_map<int, int> local_best_area2region;
    double local_best_ssd = 1;
    
    int nr = init_regions.size();
    vector<int> changed_regions(nr, 1);
   
    bool use_sa = false;
    double T = 1; // temperature
    // Openshaw's Simulated Annealing for AZP algorithm
    int maxit = 0;
    
    while ( T > 0.1 || maxit < 3 ) {
    //while ( maxit < 3 ) {
        int improved = 0;
       
        bool swapping = true;
        int total_move = 0;
        int nr = init_regions.size();
        vector<int>::iterator iter;
        vector<int> changed_regions(nr, 1);
        while (swapping) {
            int moves_made = 0;
            vector<int> regionIds;
            for (int r=0; r<nr; r++) {
                //if (changed_regions[r] >0) {
                regionIds.push_back(r);
                //}
            }
            shuffle(regionIds, seed_local);
            for (int r=0; r<nr; r++) changed_regions[r] = 0;
            for (int i=0; i<regionIds.size(); i++) {
                int seed = regionIds[i];
                unordered_map<int, bool>::iterator m_it, n_it;
                unordered_map<int, bool> member_dict, neighbors_dict;
                for (int j=0; j<init_regions[seed].size();j++) {
                    int member = init_regions[seed][j];
                    member_dict[member]=true;
                }
                for (int j=0; j<init_regions[seed].size();j++) {
                    int member = init_regions[seed][j];
                    for (int k=0; k<w[member].Size(); k++) {
                        int cand = w[member][k];
                        if (member_dict.find(cand) == member_dict.end())
                            neighbors_dict[cand] = true;
                    }
                }
                int m_size = member_dict.size();
                int n_size = neighbors_dict.size();
                
                vector<int> candidates;
                for (n_it=neighbors_dict.begin(); n_it!=neighbors_dict.end(); n_it++) {
                    int nbr = n_it->first;
                    vector<int>& block = init_regions[ init_area2region[ nbr ] ];
                    if (check_floor(block, nbr)) {
                        if (check_contiguity(w, block, nbr)) {
                            candidates.push_back(nbr);
                        }
                    }
                }
                // find the best local move
                if (use_sa) {
                    // use Simulated Annealing
                    double cv = 0.0;
                    int best = 0;
                    bool best_found = false;
                    for (int j=0; j<candidates.size() && best_found == false; j++) {
                        int area = candidates[j];
                        vector<int>& current_internal = init_regions[seed];
                        vector<int>& current_outter = init_regions[init_area2region[area]];
                        double change = objective_function_change(area, current_internal, current_outter);
                        change = -change / (local_best_ssd * T);
                        if (exp(change) > Gda::ThomasWangHashDouble(seed_local++)) {
                            best = area;
                            cv = change;
                            best_found = true;
                        }
                    }
                    if (best_found) {
                        // make the move
                        int area = best;
                        int old_region = init_area2region[area];
                        vector<int>& rgn = init_regions[old_region];
                        rgn.erase(remove(rgn.begin(),rgn.end(), area), rgn.end());
                        
                        init_area2region[area] = seed;
                        init_regions[seed].push_back(area);
                      
                        moves_made += 1;
                        changed_regions[seed] = 1;
                        changed_regions[old_region] = 1;
                    }
                } else {
                    while (!candidates.empty()) {
                        double cv = 0.0;
                        int best = 0;
                        bool best_found = false;
                        for (int j=0; j<candidates.size(); j++) {
                            int area = candidates[j];
                            vector<int>& current_internal = init_regions[seed];
                            vector<int>& current_outter = init_regions[init_area2region[area]];
                            double change = objective_function_change(area, current_internal, current_outter);
                            if (change <= cv) {
                                best = area;
                                cv = change;
                                best_found = true;
                            }
                        }
                        candidates.clear();
                        if (best_found) {
                            // make the move
                            int area = best;
                            int old_region = init_area2region[area];
                            vector<int>& rgn = init_regions[old_region];
                            rgn.erase(remove(rgn.begin(),rgn.end(), area), rgn.end());
                            
                            init_area2region[area] = seed;
                            init_regions[seed].push_back(area);
                            
                            moves_made += 1;
                            changed_regions[seed] = 1;
                            changed_regions[old_region] = 1;
                            
                            // update candidates list after move in
                            member_dict[area] = true;
                            neighbors_dict[area] = false;
                            for (int k=0; k<w[area].Size(); k++) {
                                int nbr = w[area][k];
                                if (member_dict[nbr] || neighbors_dict[nbr]) continue;
                                vector<int>& block = init_regions[ init_area2region[ nbr ] ];
                                if (check_floor(block, nbr)) {
                                    if (check_contiguity(w, block, nbr)) {
                                        candidates.push_back(nbr);
                                        neighbors_dict[nbr] = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            total_move += moves_made;
            if (moves_made == 0) {
                swapping = false;
                total_moves = total_move;
            } else {
                if (use_sa) use_sa = false; // a random move is made using SA
            }
        }
       
        if (local_best_solution.empty()) {
            improved = 1;
            local_best_solution = init_regions;
            local_best_area2region = init_area2region;
            local_best_ssd = objective_function(init_regions);
        } else {
            double current_ssd = objective_function(init_regions);
            if ( current_ssd < local_best_ssd) {
                improved = 1;
                local_best_solution = init_regions;
                local_best_area2region = init_area2region;
                local_best_ssd = current_ssd;
            }
        }

        if (improved == 1) {
            use_sa = false;
            T = 1;
            maxit = 0;
        } else {
            maxit += 1;
            T *= alpha;
            use_sa = true;
        }
    }
    // make sure tabu result is no worse than greedy research
    double search_best_ssd = objective_function(init_regions);
    if (local_best_ssd < search_best_ssd) {
        init_regions = local_best_solution;
        init_area2region = local_best_area2region;
    }
}

void Maxp::tabu_search(vector<vector<int> >& init_regions, unordered_map<int, int>& init_area2region, int tabuLength, uint64_t seed_local)
{
    vector<vector<int> > local_best_solution;
    unordered_map<int, int> local_best_area2region;
    double local_best_ssd;
    
    int nr = init_regions.size();
    
    vector<int> changed_regions(nr, 1);
    // tabuLength: Number of times a reverse move is prohibited. Default value tabuLength = 85.
    int convTabu = 230 * sqrt((double)nr);
    // convTabu=230*numpy.sqrt(maxP)
    vector<TabuMove> tabuList;
    
    bool use_tabu = false;
    int c = 0;
    
    while ( c<convTabu ) {
        int num_move = 0;
        vector<int> regionIds;
        for (int r=0; r<nr; r++) {
            //if (changed_regions[r] >0 || use_tabu) {
                regionIds.push_back(r);
            //}
        }
        shuffle(regionIds, seed_local);
        for (int r=0; r<nr; r++) changed_regions[r] = 0;
        for (int i=0; i<regionIds.size(); i++) {
            int seed = regionIds[i];
            
            // get neighbors of current region
            unordered_map<int, bool>::iterator m_it, n_it;
            unordered_map<int, bool> member_dict, neighbors_dict;
            
            for (int j=0; j<init_regions[seed].size();j++) {
                int member = init_regions[seed][j];
                member_dict[member]=true;
            }
            for (int j=0; j<init_regions[seed].size();j++) {
                int member = init_regions[seed][j];
                for (int k=0; k<w[member].Size(); k++) {
                    int cand = w[member][k];
                    if (member_dict.find(cand) == member_dict.end())
                        neighbors_dict[cand] = true;
                }
            }
            vector<int> candidates;
            for (n_it=neighbors_dict.begin(); n_it!=neighbors_dict.end(); n_it++) {
                int nbr = n_it->first;
                vector<int>& block = init_regions[ init_area2region[ nbr ] ];
                if (check_floor(block, nbr)) {
                    if (check_contiguity(w, block, nbr)) {
                        candidates.push_back(nbr);
                    }
                }
            }
            // find the best local move to improve current region
            if (use_tabu == false) {
                double cv = 0.0;
                int best = -1;
                bool best_found = false;
                for (int j=0; j<candidates.size(); j++) {
                    int area = candidates[j];
                    vector<int>& current_internal = init_regions[seed];
                    vector<int>& current_outter = init_regions[init_area2region[area]];
                    if (!tabuList.empty()) {
                        TabuMove tabu(area, init_area2region[area], seed);
                        if ( find(tabuList.begin(), tabuList.end(), tabu) != tabuList.end() )
                            continue;
                    }
                    double change = objective_function_change(area, current_internal, current_outter);
                    if (change <= cv) {
                        best = area;
                        cv = change;
                        best_found = true;
                    }
                }
                
                if (best_found) {
                    int area = best;
                    if (init_area2region.find(area) != init_area2region.end()) {
                        int old_region = init_area2region[area];
                        // make the move
                        move(area, old_region, seed, init_regions, init_area2region, tabuList,tabuLength);
                        num_move ++;
                        changed_regions[seed] = 1;
                        changed_regions[old_region] = 1;
                    }
                }
            } else {
                double cv = 0.0;
                int best = -1;
                bool best_found = false;
                for (int j=0; j<candidates.size(); j++) {
                    int area = candidates[j];
                    vector<int>& current_internal = init_regions[seed];
                    vector<int>& current_outter = init_regions[init_area2region[area]];
                    // prohibit tabu
                    TabuMove tabu(area, init_area2region[area], seed);
                    if ( find(tabuList.begin(), tabuList.end(), tabu) != tabuList.end() )
                        continue;
                    double change = objective_function_change(area, current_internal, current_outter);
                    if (j ==0 || change <= cv) {
                        best = area;
                        cv = change;
                        best_found = true;
                    }
                }
                
                if (best_found) {
                    int area = best;
                    if (init_area2region.find(area) != init_area2region.end()) {
                        int old_region = init_area2region[area];
                        // make the move
                        move(area, old_region, seed, init_regions, init_area2region, tabuList,tabuLength);
                        num_move ++;
                        changed_regions[seed] = 1;
                        changed_regions[old_region] = 1;
                    }
                }
                c++;
            }
        }
        
        // all regions are checked with possible moves
        if (num_move ==0) {
            // if no improving move can be made, then see if a tabu move can be made (relaxing its basic rule) which improves on the current local best (termed an aspiration move)
            use_tabu = true;
           
            if (local_best_solution.empty()) {
                local_best_solution = init_regions;
                local_best_area2region = init_area2region;
                local_best_ssd = objective_function(init_regions);
            } else {
                double current_ssd = objective_function(init_regions);
                if ( current_ssd < local_best_ssd ) {
                    local_best_solution = init_regions;
                    local_best_area2region = init_area2region;
                    local_best_ssd = current_ssd;
                }
            }
            
        } else {
            // some moves just made
            if (use_tabu == true)
                use_tabu = false; // switch from tabu to regular move
            else
                c = 0; // always reset tabu since a move is just made
        }
    }
    // make sure tabu result is no worse than greedy research
    double search_best_ssd = objective_function(init_regions);
    if (local_best_ssd < search_best_ssd) {
        init_regions = local_best_solution;
        init_area2region = local_best_area2region;
    }
}


void Maxp::move(int area, int from_region, int to_region, vector<vector<int> >& _regions, unordered_map<int, int>& _area2region)
{
    vector<int>& rgn = _regions[from_region];
    rgn.erase(remove(rgn.begin(),rgn.end(), area), rgn.end());
    
    _area2region[area] = to_region;
    _regions[to_region].push_back(area);
}

void Maxp::move(int area, int from_region, int to_region, vector<vector<int> >& _regions, unordered_map<int, int>& _area2region, vector<TabuMove>& tabu_list, int max_labu_length)
{
    vector<int>& rgn = _regions[from_region];
    rgn.erase(remove(rgn.begin(),rgn.end(), area), rgn.end());
    
    _area2region[area] = to_region;
    _regions[to_region].push_back(area);
    
    TabuMove tabu(area, from_region, to_region);
    
    if ( find(tabu_list.begin(), tabu_list.end(), tabu) == tabu_list.end() ) {
        if (tabu_list.size() >= max_labu_length) {
            tabu_list.pop_back();
        }
        tabu_list.insert(tabu_list.begin(), tabu);
    }
}

void Maxp::swap(vector<vector<int> >& init_regions, unordered_map<int, int>& init_area2region, uint64_t seed_local)
{
    // local search AZP
    
    bool swapping = true;
    int swap_iteration = 0;
    int total_move = 0;
    int nr = init_regions.size();
    
    vector<int>::iterator iter;
    vector<int> changed_regions(nr, 1);
    
    // nr = range(k)
    while (swapping) {
        int moves_made = 0;
        
        //selects a neighbouring solution at random
        // regionIds = [r for r in nr if changed_regions[r]]
        
        vector<int> regionIds;
        for (int r=0; r<nr; r++) {
            //if (changed_regions[r] >0) {
                regionIds.push_back(r);
            //}
        }
        //random_shuffle(regionIds.begin(), regionIds.end());
        for (int i=regionIds.size()-1; i>=1; --i) {
            int k = Gda::ThomasWangHashDouble(seed_local++) * (i+1);
            while (k>=i) k = Gda::ThomasWangHashDouble(seed_local++) * (i+1);
            if (k != i) std::iter_swap(regionIds.begin() + k, regionIds.begin()+i);
        }
        
        for (int r=0; r<nr; r++) changed_regions[r] = 0;
        
        swap_iteration += 1;
        for (int i=0; i<regionIds.size(); i++) {
            int seed = regionIds[i];
            // get neighbors
            
            unordered_map<int, bool>::iterator m_it, n_it;
            unordered_map<int, bool> member_dict, neighbors_dict;
           
            for (int j=0; j<init_regions[seed].size();j++) {
                int member = init_regions[seed][j];
                member_dict[member]=true;
            }
            for (int j=0; j<init_regions[seed].size();j++) {
                int member = init_regions[seed][j];
                for (int k=0; k<w[member].Size(); k++) {
                    int cand = w[member][k];
                    if (member_dict.find(cand) == member_dict.end())
                        neighbors_dict[cand] = true;
                }
            }
            int m_size = member_dict.size();
            int n_size = neighbors_dict.size();
            
            vector<int> candidates;
            for (n_it=neighbors_dict.begin(); n_it!=neighbors_dict.end(); n_it++) {
                int nbr = n_it->first;
                vector<int>& block = init_regions[ init_area2region[ nbr ] ];
                if (check_floor(block, nbr)) {
                    if (check_contiguity(w, block, nbr)) {
                        candidates.push_back(nbr);
                    }
                }
            }
            // find the best local move
            while (!candidates.empty()) {
                double cv = 0.0;
                int best = 0;
                bool best_found = false;
                for (int j=0; j<candidates.size(); j++) {
                    int area = candidates[j];
                    vector<int>& current_internal = init_regions[seed];
                    vector<int>& current_outter = init_regions[init_area2region[area]];
                    double change = objective_function_change(area, current_internal, current_outter);
                    if (change <= cv) {
                        best = area;
                        cv = change;
                        best_found = true;
                    }
                }
                candidates.clear();
                if (best_found) {
                    // make the move
                    int area = best;
                    int old_region = init_area2region[area];
                    vector<int>& rgn = init_regions[old_region];
                    rgn.erase(remove(rgn.begin(),rgn.end(), area), rgn.end());
                    
                    init_area2region[area] = seed;
                    init_regions[seed].push_back(area);
                    
                    moves_made += 1;
                    changed_regions[seed] = 1;
                    changed_regions[old_region] = 1;
                   
                    // update candidates list after move in
                    
                    member_dict[area] = true;
                    neighbors_dict[area] = false;
                    for (int k=0; k<w[area].Size(); k++) {
                        int nbr = w[area][k];
                        if (member_dict[nbr] || neighbors_dict[nbr]) continue;
                        vector<int>& block = init_regions[ init_area2region[ nbr ] ];
                        if (check_floor(block, nbr)) {
                            if (check_contiguity(w, block, nbr)) {
                                candidates.push_back(nbr);
                                neighbors_dict[nbr] = true;
                            }
                        }
                    }
                }
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

bool Maxp::check_floor(const vector<int>& region, int leaver)
{
    // selectionIDs = [self.w.id_order.index(i) for i in region]
    double cv = 0;
    for (size_t i=0; i<region.size(); i++) {
        int selectionID = region[i];
        if (selectionID == leaver) continue;
        cv += floor_variable[ selectionID ];
    }
    if (cv >= floor)
        return true;
    else
        return false;
}

bool Maxp::check_floor(const vector<int>& region)
{
    // selectionIDs = [self.w.id_order.index(i) for i in region]
    double cv = 0;
    for (size_t i=0; i<region.size(); i++) {
        int selectionID = region[i];
        double f_v = floor_variable[ selectionID ];
        cv += f_v;
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

double Maxp::objective_function(vector<int>& solution)
{
    //if (objval_dict.find(solution) != objval_dict.end()) {
    //    return objval_dict[solution];
    //}
    
    // solution is a list of region ids [1,7,2]
    double wss = 0;
    
    int n_size = solution.size();
    
    // for every variable, calc the variance using selected neighbors
    for (int m=0; m<num_vars; m++) {
        
        vector<double> selected_z(n_size);
        
        for (int i=0; i<solution.size(); i++ ) {
            int selectionID = solution[i];
            selected_z[i] =  z[selectionID][m];
        }
        
        double ssd = GenUtils::SumOfSquares(selected_z);
        wss += ssd;
    }
    
    //objval_dict[solution] = wss;
    return wss;
}

double Maxp::objective_function(vector<int>& region1, int leaver, vector<int>& region2, int comer )
{
    // solution is a list of region ids [1,7,2]
    double wss = 0;
    int j=0;
    int n_size = region1.size();
    // for every variable, calc the variance using selected neighbors
    for (int m=0; m<num_vars; m++) {
        vector<double> selected_z(n_size-1);
        j = 0;
        for (int i=0; i<n_size; i++ ) {
            if (region1[i] == leaver) continue;
            int selectionID = region1[i];
            selected_z[j++] =  z[selectionID][m];
        }
        double ssd = GenUtils::SumOfSquares(selected_z);
        wss += ssd;
    }
    
    n_size = region2.size();
    // for every variable, calc the variance using selected neighbors
    for (int m=0; m<num_vars; m++) {
        vector<double> selected_z(n_size+1);
        for (int i=0; i<n_size; i++ ) {
            int selectionID = region2[i];
            selected_z[i] =  z[selectionID][m];
        }
        selected_z[n_size] = z[comer][m];
        double ssd = GenUtils::SumOfSquares(selected_z);
        wss += ssd;
    }
    return wss;
}

double Maxp::objective_function(vector<vector<int> >& solution)
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
            double var = GenUtils::SumOfSquares(selected_z[n]);
            sum += var;
        }
        wss += sum ;
    }
    
    return wss;
}

double Maxp::objective_function(vector<int>& current_internal, vector<int>& current_outter)
{
    vector<vector<int> > composed_region;
    composed_region.push_back(current_internal);
    composed_region.push_back(current_outter);
    
    return objective_function(composed_region);
}


double Maxp::objective_function_change(int area, vector<int>& current_internal, vector<int>& current_outter)
{
    double current = objective_function(current_internal) + objective_function(current_outter);
    double new_val = objective_function(current_outter, area, current_internal, area);
    double change = new_val - current;
    return change;
}

bool Maxp::is_component(const GalElement *w, const vector<int> &ids)
{
    //Check if the set of ids form a single connected component
    int components = 0;
    unordered_map<int, int> marks;
    for (int i=0; i<ids.size(); i++) marks[ids[i]] = 0;
    
    list<int> q;
    list<int>::iterator iter;
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

bool Maxp::check_contiguity(const GalElement* w, vector<int>& ids, int leaver)
{
    //vector<int> ids = neighbors;
    //ids.erase(remove(ids.begin(),ids.end(), leaver), ids.end());
    //return is_component(w, ids);
    list<int> q;
    unordered_map<int, bool> marks;
    for (int i=0; i<ids.size(); i++) {
        if (ids[i]!=leaver) {
            marks[ids[i]] = false;
            if (q.empty()) {
                q.push_back(ids[i]);
                marks[ids[i]] = true;
            }
        }
    }
    
    int nbr, node;
    while (!q.empty()) {
        node = q.front();
        q.pop_front();
        
        marks[node] = true;
        for (int n=0; n<w[node].Size(); n++) {
            nbr = w[node][n];
            if (marks.find(nbr) != marks.end()) {
                if (marks[nbr] == false) {
                    marks[nbr] = true;
                    q.push_back(nbr);
                }
            }
        }
    }
    unordered_map<int, bool>::iterator it;
    for (it=marks.begin(); it!=marks.end(); it++) {
        if (it->second == false)
            return false;
    }
    return true;
}
