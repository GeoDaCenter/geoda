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

#include "../ShapeOperations/GalWeight.h"


using namespace std;

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
    map<int, int> mdict;
    vector<int>::iterator v_iter;
    map<int, int>::iterator m_iter;
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
    Maxp(const GalElement* w, const vector<vector<double> >& z, int floor, vector<vector<int> > floor_variable, int initial, vector<int> seed);
    
    
    //! A Deconstructor
    /*!
     Details.
     */
    ~Maxp();
    
    
protected:
    //! A const spatial weights reference.
    /*!
     Details.
     */
    const GalElement* w;
    
    
    
    bool feasible;
    
    //! A integer number of regions.
    /*!
     Details.
     */
    int num_obs;
    
    //! A vector of vector<int> list of ids to form initial seeds.
    /*!
     Details. seed list ids of observations to form initial seeds.
     */
    vector<vector<int> > seed;
    
    //! A n*1 vector of observations on variable for the floor
    /*!
     Details.
     */
    vector<vector<int> > floor_variable;
    
    //! A n*m array of observations on m attributes across n areas.
    /*!
     Details. This is used to calculate intra-regional
     */
    const vector<vector<double> > z;
    
    //! A map variable mapping of areas to region.
    /*!
     Details. key is area id, value is region id.
     */
    map<int, int> area2region;
    
    //! A vector of vector<int> list of lists of regions.
    /*!
     Details. each list has the ids of areas in that region.
     */
    vector<vector<int> > regions;
    
    //! A integer number of regions.
    /*!
     Details.
     */
    int p;
    
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
    int floor;
    
    //! A const integer number of largest regions (=10 ** 6).
    /*!
     Details.
     */
    const int LARGE;
    
    //! A const integer number of max attemps (=100).
    /*!
     Details.
     */
    const int MAX_ATTEMPTS;
    
    //! A protected member function: init_solution(void).
    /*!
     Details.
     */
    void init_solution();
    
    //! A protected member function: init_solution(void). return
    /*!
     \param region a const vector of unsigned int.
     \return boolean true if meet floor else return false
     */
    bool check_floor(const vector<int>& region);
    
    double objective_function();

    void swap();
    
    bool is_component(const GalElement* w, const vector<int>& ids);
};

#endif
