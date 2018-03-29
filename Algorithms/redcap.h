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
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////
class RedCapNode
{
public:
    RedCapNode(int id, const vector<double>& value);
    
    RedCapNode(RedCapNode* node);
   
    RedCapNode(RedCapNode* node, RedCapNode* exclude_neighb_node);
    
    ~RedCapNode();
    
    void AddNeighbor(RedCapNode* node);
    
    void RemoveNeighbor(RedCapNode* node);
    
    void RemoveNeighbor(int node_id);
    
    int id; // mapping to record id
    
    const vector<double>& value;
    
    std::set<RedCapNode*> neighbors;
    
    std::set<RedCapNode*>::iterator it;
};

/////////////////////////////////////////////////////////////////////////
//
// RedCapEdge
//
/////////////////////////////////////////////////////////////////////////
class RedCapEdge
{
public:
    RedCapEdge(RedCapNode* a, RedCapNode* b, double length, double weight=1.0);
    ~RedCapEdge();
    
    RedCapNode* a;
    RedCapNode* b;
    double length; // legnth of the edge |a.val - b.val|
    
    bool Has(int id1) {
        return a->id == id1 || b->id == id1;
    }
protected:
    double weight;
    
};

////////////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
////////////////////////////////////////////////////////////////////////////////
class RedCapCluster {
public:
    RedCapCluster(RedCapEdge* edge);
    
    RedCapCluster(RedCapNode* node);
    
    ~RedCapCluster();
    
    int size();
   
    bool IsConnectWith(RedCapCluster* c);
    
    RedCapNode* GetNode(int i);
    
    bool Has(RedCapNode* node);
    
    bool Has(int id);
    
    void AddNode(RedCapNode* node);
    
    void Merge(RedCapCluster* cluster);
    
    RedCapNode* root;
    
    unordered_map<int, bool> node_id_dict;
    
    unordered_map<RedCapNode*, bool> node_dict;
    
    unordered_map<RedCapEdge*, bool> edge_dict;
};

////////////////////////////////////////////////////////////////////////////////
//
// RedCapClusterManager
//
////////////////////////////////////////////////////////////////////////////////
class RedCapClusterManager {

public:
    RedCapClusterManager();
    ~RedCapClusterManager();
   
    unordered_map<RedCapCluster*, bool> clusters_dict;
    unordered_map<RedCapCluster*, bool>::iterator it;
    
    bool HasCluster(RedCapCluster* cluster);
    
    RedCapCluster* UpdateByAdd(RedCapEdge* edge);
    

    bool CheckConnectivity(RedCapEdge* edge,
                           RedCapCluster** c1,
                           RedCapCluster** c2);
    
    RedCapCluster* getCluster(RedCapNode* node);
    
    RedCapCluster* createCluster(RedCapNode* node);
    
    RedCapCluster* mergeToCluster(RedCapNode* node, RedCapCluster* cluster);
    
    RedCapCluster* mergeClusters(RedCapCluster* cluster1,
                                 RedCapCluster* cluster2);
    
};

/////////////////////////////////////////////////////////////////////////
//
// SpanningTree
//
/////////////////////////////////////////////////////////////////////////
typedef std::pair<int, int> E;

static unordered_map<std::set<int>, double> ssd_dict; // <start, end>: ssd value

class SpanningTree
{
public:
    vector<RedCapEdge*> edges;
    
    unordered_map<E, bool> edge_dict;
    
    unordered_map<int, bool> node_dict;
    
    boost::mutex mutex;
    
    double ssd;
    
    RedCapNode* root;
    
    const vector<vector<double> >& data;
    
    const vector<bool>& undefs;
    
    double* controls;
    
    double control_thres;
    
    SpanningTree* left_child;
    
    SpanningTree* right_child;
    
    map<RedCapEdge*, double> cand_trees;
    
    unordered_map<int, RedCapNode*> new_nodes;
    
    vector<RedCapEdge*> new_edges;

public:
    SpanningTree(const vector<RedCapNode*>& all_nodes,
                 const vector<vector<double> >& _data,
                 const vector<bool>& _undefs,
                 double* controls = NULL,
                 double control_thres = 0);
    
    SpanningTree(RedCapNode* node,
                 RedCapNode* exclude_node,
                 const vector<vector<double> >& _data,
                 const vector<bool>& _undefs,
                 double* controls = NULL,
                 double control_thres = 0);

    ~SpanningTree();
   
    
    bool AddEdge(RedCapEdge* edge);
  
    bool IsFullyCovered();
    
    void Split();
    
    void subSplit(int start, int end);
    
    SpanningTree* GetLeftChild();
    
    SpanningTree* GetRightChild();
    
    double GetSSD();
    
    std::set<int> getSubTree(RedCapNode* a, RedCapNode* exclude_node);

    bool quickCheck(RedCapNode* node, RedCapNode* exclude_node);
    
    bool checkEdge(RedCapEdge* edge);
    
    void save();
    
protected:
    
    double computeSSD(std::set<int>& ids);
    
    bool checkControl(std::set<int>& ids);
    // check if all odes are included in the graph
    
    RedCapNode* getNewNode(RedCapNode* old_node, bool copy_nbrs=false);
    
    void addNewEdge(RedCapNode* a, RedCapNode* b);
    
};

////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
////////////////////////////////////////////////////////////////////////////////
/*! A REDCAP class */

struct CompareCluster
{
public:
    bool operator() (const SpanningTree* lhs, const SpanningTree* rhs) const
    {
        return lhs->ssd < rhs->ssd;
    }
};

typedef heap::priority_queue<SpanningTree*, heap::compare<CompareCluster> > PriorityQueue;


class AbstractRedcap
{
public:
    
    AbstractRedcap(const vector<vector<double> >& distances,
                   const vector<vector<double> >& data,
                   const vector<bool>& undefs,
                   GalElement * w);
    
    //! A Deconstructor
    /*!
     Details.
     */
    virtual ~AbstractRedcap();

    void init();
    
    // check if mst is a spanning tree
    bool CheckSpanningTree();
   
    void Partitioning(int k);
    
    virtual void Clustering()=0;
    
    vector<vector<int> >& GetRegions();
    
    void createFullOrderEdges(vector<RedCapEdge*>& e);
    
    void createFirstOrderEdges(vector<RedCapEdge*>& e);
    
    bool checkFirstOrderEdge(int node_i, int node_j);
    
protected:
    
    int num_obs;
    
    int num_vars;
    
    GalElement* w;
    
    vector<vector<bool> > first_order_dict;
    
    const vector<vector<double> >& distances;
    
    const vector<vector<double> >& data;
    
    const vector<bool>& undefs; // undef = any one item is undef in all variables
    
    double* controls;
    
    double control_thres;

    vector<RedCapNode*> all_nodes;
    
    vector<RedCapEdge*> first_order_edges;
    
    SpanningTree* mstree;
    
    vector<SpanningTree*> regions;
    
    vector<vector<int> > cluster_ids;
    
    RedCapClusterManager cm; // manage to create a MST
    
};

////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FirstOrderSLKRedCap : public AbstractRedcap
{
public:
    FirstOrderSLKRedCap(const vector<vector<double> >& _distances,
                        const vector<vector<double> >& data,
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
    FirstOrderALKRedCap(const vector<vector<double> >& _distances,
                        const vector<vector<double> >& data,
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
    FirstOrderCLKRedCap(const vector<vector<double> >& _distances,
                        const vector<vector<double> >& data,
                        const vector<bool>& undefs,
                        GalElement * w,
                        double* controls,
                        double control_thres);
    
    virtual ~FirstOrderCLKRedCap();
    
    virtual void Clustering();
    
protected:
    vector<vector<double> > maxDist;

    double getMaxDist(RedCapCluster* l, RedCapCluster* m);
    
};


////////////////////////////////////////////////////////////////////////////////
//
// 4 FullOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
class FullOrderSLKRedCap : public AbstractRedcap
{
public:
    FullOrderSLKRedCap(const vector<vector<double> >& _distances,
                       const vector<vector<double> >& data,
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
    FullOrderALKRedCap(const vector<vector<double> >& _distances,
                       const vector<vector<double> >& data,
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
    FullOrderCLKRedCap(const vector<vector<double> >& _distances,
                       const vector<vector<double> >& data,
                       const vector<bool>& undefs,
                       GalElement * w,
                       double* controls,
                       double control_thres);
    
    virtual ~FullOrderCLKRedCap();
    
    virtual void Clustering();
   
protected:
    vector<vector<double> > maxDist;
};


#endif
