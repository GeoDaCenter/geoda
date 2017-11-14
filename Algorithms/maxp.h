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

#ifndef __GEODA_CENTER_MAX_P_H__
#define __GEODA_CENTER_MAX_P_H__

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>

#include "../ShapeOperations/GalWeight.h"

using namespace std;
using namespace boost;

class qvector
{
public:
    qvector();
    ~qvector();
    
    bool find(int val);
    void remove(int val);
    void add(int val);
    
protected:
    vector<int> vdata;
    unordered_map<int, int> mdict;
    vector<int>::iterator v_iter;
    unordered_map<int, int>::iterator m_iter;
};

/*! A Max-p class */

class Maxp
{
public:
    //! A Constructor
    /*!
     \param w spatial weights object
     \param z array n*m array of observations on m attributes across n areas. This is used to calculate intra-regional
     \param floor int a minimum bound for a variable that has to be obtained in each region homogeneity
     \param floor_variable array n*1 vector of observations on variable for the floor
     \param initial int number of initial solutions to generate
     \param seed list ids of observations to form initial seeds. If len(ids) is less than the number of observations, the complementary ids are added to the end of seeds. Thus the specified seeds get priority in the solution
     */
    Maxp(const GalElement* w, const vector<vector<double> >& z, double floor, double* floor_variable, int initial, vector<wxInt64> seeds, int rnd_seed=-1, char dist='e', bool test=false);
    
    
    //! A Deconstructor
    /*!
     Details.
     */
    ~Maxp();
    
    
    //! xxx
    /* !
     \param block
     \param neighbor
     \return boolean
     */
    vector<vector<int> >& GetRegions();
    
protected:
    //! A const spatial weights reference.
    /*!
     Details.
     */
    const GalElement* w;
    
    char dist;
    
    int rnd_seed;
    
    bool feasible;
    
    //! A integer number of regions.
    /*!
     Details.
     */
    int num_obs;
    
    //! A integer number of variables.
    /*!
     Details.
     */
    int num_vars;
    
    //! A vector of vector<int> list of ids to form initial seeds.
    /*!
     Details. seed list ids of observations to form initial seeds.
     */
    vector<size_t> seeds;
    
    //! A n*1 vector of observations on variable for the floor
    /*!
     Details.
     */
    double* floor_variable;
    
    //! A n*m array of observations on m attributes across n areas.
    /*!
     Details. This is used to calculate intra-regional
     */
    const vector<vector<double> > z;
    
    //! A map variable mapping of areas to region.
    /*!
     Details. key is area id, value is region id.
     */
    unordered_map<int, int> area2region;
    
    vector<unordered_map<int, int> > area2region_group;
    
    //! A vector of vector<int> list of lists of regions.
    /*!
     Details. each list has the ids of areas in that region.
     */
    vector<vector<int> > regions;
    
    vector<vector<vector<int> > > regions_group;
   
    double best_ss;
    
    //! A integer number of regions.
    /*!
     Details.
     */
    int p;
    
    vector<int> p_group;
    
    //! A integer number of initializations.
    /*!
     Details.
     */
    int initial;
    
    //! A integer number of swap iterations.
    /*!
     Details.
     */
    int swap_iterations;
    
    //! A integer number of moves into internal regions.
    /*!
     Details.
     */
    int total_moves;
    
    //! A integer number of moves into internal regions.
    /*!
     Details.
     */
    double floor;
    
    //! A const integer number of largest regions (=10 ** 6).
    /*!
     Details.
     */
    const int LARGE;
    
    //! A const integer number of max attemps (=100).
    /*!
     Details.
     */
    int MAX_ATTEMPTS;
   
    uint64_t seed_start;
    
    uint64_t seed_increment;
    
    vector<double> initial_wss;
    
    //! A protected member function: init_solution(void).
    /*!
     Details.
     */
    void init_solution(int solution_idx=-1);
    
    void run(int a, int b);
    
    void run_threaded();
    
    //! A protected member function: init_solution(void).
    /*!
     Details.
     */
    void swap(vector<vector<int> >& init_regions, unordered_map<int, int>& area2region);
    
    //! A protected member function: init_solution(void). return
    /*!
     \param region a const vector of unsigned int.
     \return boolean true if meet floor else return false
     */
    bool check_floor(const vector<int>& region);
    
    double objective_function();
    
    double objective_function(const vector<vector<int> >& solution);
    
    double objective_function(const vector<int>& current_internal, const vector<int>& current_outter);
    
    double objective_function_change(int area, const vector<int>& current_internal, const vector<int>& current_outter);
   
    wxString print_regions(vector<vector<int> >& _regions);
    //! xxx
    /* !
     \param block
     \param neighbor
     \return boolean
     */
    bool check_contiguity(const GalElement* w, vector<int>& block, int neighbor);
    
    bool is_component(const GalElement* w, const vector<int>& ids);
    
    bool test;
    list<int> test_random_numbers;
    list<int> enclave_random_number;
    list<vector<int> > test_random_cand;
    vector<int> test_get_random();
    void init_test();
};

#endif
