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

#ifndef __GEODA_CENTER_REDCAP_H__
#define __GEODA_CENTER_REDCAP_H__

#include <vector>
#include <set>
#include <float.h>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/heap/priority_queue.hpp>

#include "../ShapeOperations/GalWeight.h"


using namespace std;
using namespace boost;

template <class T>
class SortedList
{
public:
    SortedList();
    ~SortedList();
    
    void push_back(T* node);
   
    T* pop(); // pop with largest node->items
};

typedef std::pair<int, int> E;

/////////////////////////////////////////////////////////////////////////
//
// SSDUtils
//
/////////////////////////////////////////////////////////////////////////
struct Measure
{
    double ssd;
    double ssd_part1;
    double ssd_part2;
    double measure_reduction;
};

class SSDUtils
{
    double** raw_data;
    int row;
    int col;
    
public:
    SSDUtils(double** data, int _row, int _col) {
        raw_data = data;
        row = _row;
        col = _col;
    }
    ~SSDUtils() {}
    
    double ComputeSSD(vector<int>& visited_ids, int start, int end);
    void MeasureSplit(double ssd, vector<int>& visited_ids, int split_position, Measure& result);
    
};

class RedCapCluster;
class RedCapNode;
class RedCapEdge;
/////////////////////////////////////////////////////////////////////////
//
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////

struct NeighborInfo
{
    RedCapNode* p;
    RedCapNode* n1;
    RedCapNode* n2;
    RedCapEdge* e1;
    RedCapEdge* e2;
    
    void SetDefault(RedCapNode* parent) {
        p = parent;
        n1 = NULL;
        n2 = NULL;
        e1 = NULL;
        e2 = NULL;
    }
    
    void AddNeighbor(RedCapNode* nbr, RedCapEdge* e) {
        if (n1 == NULL) {
            n1 = nbr;
            e1 = e;
        } else if (n2 == NULL) {
            n2 = nbr;
            e2 = e;
        } else {
            cout << "AddNeighbor() > 2" << endl;
        }
    }
};

class RedCapNode
{
public:
    RedCapNode(int id);
    ~RedCapNode() {}
    
    void SetCluster(RedCapCluster* cluster);
    void AddNeighbor(RedCapNode* nbr, RedCapEdge* e);
    
    int id; // mapping to record id
    RedCapCluster* container;
    NeighborInfo nbr_info;
};

/////////////////////////////////////////////////////////////////////////
//
// RedCapEdge
//
/////////////////////////////////////////////////////////////////////////
class RedCapEdge
{
public:
    RedCapEdge(RedCapNode* a, RedCapNode* b, double length);
    ~RedCapEdge() {}
    
    RedCapNode* orig;
    RedCapNode* dest;
    double length; // legnth of the edge |a.val - b.val|
};

////////////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
////////////////////////////////////////////////////////////////////////////////
class RedCapCluster {
public:
    static RedCapCluster* GetRoot(RedCapNode* node);
    static void GetOrderedNodes(RedCapCluster* root, vector<RedCapNode*>& ordered_nodes);

    RedCapCluster(RedCapNode* node);
    RedCapCluster(RedCapCluster* c1, RedCapCluster* c2, RedCapEdge* e, double** dist_matrix);
    ~RedCapCluster() {
        if (child1) {
            delete child1;
        }
        if (child2) {
            delete child2;
        }
    }

    
    double** dist_matrix;
    
    RedCapNode* p1;
    RedCapNode* p2;
    RedCapCluster* parent;
    RedCapCluster* child1;
    RedCapCluster* child2;
};



/////////////////////////////////////////////////////////////////////////
//
// RedCapTree
//
/////////////////////////////////////////////////////////////////////////
class AbstractRedcap;

class RedCapTree
{
public:
    RedCapTree(vector<int> ordered_ids,
               vector<RedCapEdge*> _edges,
               AbstractRedcap* redcap);

    ~RedCapTree();
   
    void Partition(vector<int>& ids,
                   vector<pair<int, int> >& od_array,
                   unordered_map<int, vector<int> >& nbr_dict);
    void Split(int orig, int dest,
               unordered_map<int, vector<int> >& nbr_dict,
               vector<int>& cand_ids);
    bool checkControl(vector<int>& cand_ids);
    pair<RedCapTree*, RedCapTree*> GetSubTrees();
    
    double ssd_reduce;
    double ssd;
    
    vector<pair<int, int> > od_array;
    AbstractRedcap* redcap;
    pair<RedCapTree*, RedCapTree*> subtrees;
    int max_id;
    int split_pos;
    vector<int> split_ids;
    vector<RedCapEdge*> edges;
    vector<int> ordered_ids;
    SSDUtils* ssd_utils;
    
    
    double* controls;
    double control_thres;
};

////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
////////////////////////////////////////////////////////////////////////////////
struct CompareTree
{
public:
    bool operator() (const RedCapTree* lhs, const RedCapTree* rhs) const
    {
        return lhs->ssd_reduce < rhs->ssd_reduce;
    }
};

typedef heap::priority_queue<RedCapTree*, heap::compare<CompareTree> > PriorityQueue;


class AbstractRedcap
{
public:
    int rows;
    int cols;
    GalElement* w;
    double** dist_matrix;
    double** raw_data;
    const vector<bool>& undefs; // undef = any one item is undef in all variables
    double* controls;
    double control_thres;
    SSDUtils* ssd_utils;
    
    RedCapCluster* cluster;
    
    vector<RedCapNode*> nodes;
    vector<RedCapEdge*> edges;
    
    vector<int> ordered_ids;
    vector<RedCapEdge*> ordered_edges;
    
    vector<vector<int> > cluster_ids;

    AbstractRedcap(int row, int col,
                   double** distances,
                   double** data,
                   const vector<bool>& undefs,
                   GalElement * w);
    virtual ~AbstractRedcap();

    virtual void Clustering()=0;
    
    void init();
    void Partitioning(int k);
    vector<vector<int> >& GetRegions();
};

////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FirstOrderSLKRedCap : public AbstractRedcap
{
public:
    FirstOrderSLKRedCap(int rows, int cols,
                        double** _distances,
                        double** data,
                        const vector<bool>& undefs,
                        GalElement * w,
                        double* controls,
                        double control_thres);
    virtual ~FirstOrderSLKRedCap();
    
    virtual void Clustering();
};

////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FirstOrderALKRedCap : public AbstractRedcap
{
public:
    FirstOrderALKRedCap(int rows, int cols,
                        double** _distances,
                        double** data,
                        const vector<bool>& undefs,
                        GalElement * w,
                        double* controls,
                        double control_thres);
    
    virtual ~FirstOrderALKRedCap();
    
    virtual void Clustering();
    
};

////////////////////////////////////////////////////////////////////////////////
//
// 3 FirstOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FirstOrderCLKRedCap : public AbstractRedcap
{
public:
    FirstOrderCLKRedCap(int rows, int cols,
                        double** _distances,
                        double** data,
                        const vector<bool>& undefs,
                        GalElement * w,
                        double* controls,
                        double control_thres);
    
    virtual ~FirstOrderCLKRedCap();
    
    virtual void Clustering();
    
};


////////////////////////////////////////////////////////////////////////////////
//
// 4 FullOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FullOrderSLKRedCap : public AbstractRedcap
{
public:
    FullOrderSLKRedCap(int rows, int cols,
                       double** _distances,
                       double** data,
                       const vector<bool>& undefs,
                       GalElement * w,
                       double* controls,
                       double control_thres);
    virtual ~FullOrderSLKRedCap();
    
    virtual void Clustering();
};

////////////////////////////////////////////////////////////////////////////////
//
// 5 FullOrderALKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FullOrderALKRedCap : public AbstractRedcap
{
public:
    FullOrderALKRedCap(int rows, int cols,
                       double** _distances,
                       double** data,
                       const vector<bool>& undefs,
                       GalElement * w,
                       double* controls,
                       double control_thres);
    
    virtual ~FullOrderALKRedCap();
    
    virtual void Clustering();
};

////////////////////////////////////////////////////////////////////////////////
//
// 6 FullOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FullOrderCLKRedCap : public AbstractRedcap
{
public:
    FullOrderCLKRedCap(int rows, int cols,
                       double** _distances,
                       double** data,
                       const vector<bool>& undefs,
                       GalElement * w,
                       double* controls,
                       double control_thres);
    
    virtual ~FullOrderCLKRedCap();
    
    virtual void Clustering();
   
};


#endif
