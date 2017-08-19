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

#include <boost/unordered_map.hpp>

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
    RedCapEdge(RedCapNode* a, RedCapNode* b, double weight=1.0);
    ~RedCapEdge();
    
    RedCapNode* a;
    RedCapNode* b;
    double length; // legnth of the edge |a.val - b.val|
    
protected:
    double weight;
    
};

//////////////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
//////////////////////////////////////////////////////////////////////////////////
class RedCapCluster {
public:
    RedCapCluster(RedCapEdge* edge);
    RedCapCluster();
    ~RedCapCluster();
    
    bool Has(RedCapNode* node);
    
    void AddNode(RedCapNode* node);
    
    void Merge(RedCapCluster* cluster);
    
    unordered_map<RedCapNode*, bool> node_dict;
};

//////////////////////////////////////////////////////////////////////////////////
//
// RedCapClusterManager
//
//////////////////////////////////////////////////////////////////////////////////
class RedCapClusterManager {

public:
    RedCapClusterManager();
    ~RedCapClusterManager();
   
    bool Update(RedCapEdge* edge);
    
protected:
    vector<RedCapCluster*> clusters;
    
    RedCapCluster* getCluster(RedCapNode* node);
    
    void createCluster(RedCapEdge* edge);
    
    void mergeToCluster(RedCapNode* node, RedCapCluster* cluster);
    
    void mergeClusters(RedCapCluster* cluster1, RedCapCluster* cluster2);
};

/////////////////////////////////////////////////////////////////////////
//
// SpatialContiguousTree
//
/////////////////////////////////////////////////////////////////////////

class SpatialContiguousTree
{
public:
    SpatialContiguousTree(const vector<RedCapNode*>& all_nodes,
                          const vector<vector<double> >& _data,
                          const vector<bool>& _undefs,
                          double* controls = NULL,
                          double control_thres = 0);
    
    SpatialContiguousTree(RedCapNode* graph,
                          RedCapNode* exclude_node,
                          unordered_map<int, RedCapNode*> ids_dict,
                          const vector<vector<double> >& _data,
                          const vector<bool>& _undefs,
                          double* controls = NULL,
                          double control_thres = 0);
    
    ~SpatialContiguousTree();
    
    // all nodes info
    unordered_map<RedCapNode*, bool> all_nodes_dict;
    
    unordered_map<RedCapNode*, bool>::iterator it_all_nodes;
    
    unordered_map<int, RedCapNode*> ids_dict;
    
    double heterogeneity;
    
    void AddNode(RedCapNode* node);
    
    // should be a set of edges
    vector<RedCapEdge*> edges;
    
    bool AddEdge(RedCapEdge* edge);
   
    void AddEdgeDirectly(RedCapNode* _a, RedCapNode* _b);
   
    void Split();
    
    void subSplit(int start, int end);
    
    SpatialContiguousTree* GetLeftChild();
    
    SpatialContiguousTree* GetRightChild();
    
    bool quickCheck(RedCapNode* node, RedCapNode* exclude_node);
    
protected:
    
    RedCapNode* root;
    
    const vector<vector<double> >& data;
    
    const vector<bool>& undefs;
   
    double* controls;
    
    double control_thres;
    
    SpatialContiguousTree* left_child;
    
    SpatialContiguousTree* right_child;
    
    map<pair<SpatialContiguousTree*, SpatialContiguousTree*>, double> cand_trees;
    
    vector<RedCapNode*> new_nodes;
    vector<RedCapEdge*> new_edges;
    
    double calcHeterogeneity();
    
    bool checkControl();
    // check if all odes are included in the graph
};

//////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
//////////////////////////////////////////////////////////////////////////////////
/*! A REDCAP class */

class AbstractRedcap
{
public:
    
    AbstractRedcap(const vector<vector<double> >& data, const vector<bool>& undefs);
    
    //! A Deconstructor
    /*!
     Details.
     */
    virtual ~AbstractRedcap();

    void init(GalElement* w);
    
    // check if complete graph, no islands
    bool checkFirstOrderEdges();
   
    void Partitioning(int k);
    
    virtual void Clustering()=0;
    
    vector<vector<int> >& GetRegions();
    
protected:
    
    int num_obs;
    
    int num_vars;
    
    const vector<vector<double> >& data;
    
    const vector<bool>& undefs; // undef = any one item is undef in all variables
    
    double* controls;
    
    double control_thres;

    vector<RedCapNode*> all_nodes;
    
    vector<RedCapEdge*> first_order_edges;
    
    //vector<RedCapEdge*> full_order_edges;
    
    SpatialContiguousTree* tree;
    
    vector<SpatialContiguousTree*> regions;
    
    vector<vector<int> > cluster_ids;
};

//////////////////////////////////////////////////////////////////////////////////
//
// FirstOrderSLKRedCap
//
//////////////////////////////////////////////////////////////////////////////////
class FirstOrderSLKRedCap : public AbstractRedcap
{
public:
    FirstOrderSLKRedCap();
    FirstOrderSLKRedCap(const vector<vector<double> >& data, const vector<bool>& undefs, GalElement * w, double* controls, double control_thres);
    virtual ~FirstOrderSLKRedCap();
    
    virtual void Clustering();
};

//////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
//
//////////////////////////////////////////////////////////////////////////////////
class FirstOrderALKRedCap : public AbstractRedcap
{
public:
    FirstOrderALKRedCap(const vector<vector<double> >& data, const vector<bool>& undefs);
    
    virtual ~FirstOrderALKRedCap();
    
    virtual void Clustering();
    
     double** distance_matrix;
};

#endif
