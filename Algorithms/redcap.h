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
#include <map>

#include "../ShapeOperations/GalWeight.h"


using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////
class RedCapNode
{
public:
    RedCapNode(int id, double val);
    ~RedCapNode();
    
    void AddNeighbor(RedCapNode* node);
    
    int id; // mapping to record id
    double value; // value of node itself
    
    std::set<RedCapNode*> neighbors;
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
    
    map<RedCapNode*, bool> node_dict;
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
    SpatialContiguousTree(const vector<double>& _data, const vector<bool>& _undefs);
    
    SpatialContiguousTree(const vector<RedCapNode*>& all_nodes, const vector<double>& _data, const vector<bool>& _undefs);
    
    SpatialContiguousTree(RedCapCluster* cluster, vector<RedCapEdge*> _edges, const vector<double>& _data, const vector<bool>& _undefs);
    
    ~SpatialContiguousTree();
    
    // all nodes info
    map<RedCapNode*, bool> all_nodes_dict;
    
    // should be a set of edges
    vector<RedCapEdge*> edges;
    
    bool AddEdge(RedCapEdge* edge);
    
    void Split();
    
    SpatialContiguousTree* GetLeftChild();
    
    SpatialContiguousTree* GetRightChild();
    
    double heterogeneity;
   
protected:
    
    vector<double> data;
    
    vector<bool> undefs;
    
    SpatialContiguousTree* left_child;
    
    SpatialContiguousTree* right_child;
    
    double calc_heterogeneity();
    
    // check if all odes are included in the graph
    SpatialContiguousTree* findSubTree(RedCapNode* node, RedCapNode* exclude_node);
    
};

//////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
//////////////////////////////////////////////////////////////////////////////////
/*! A REDCAP class */

class AbstractRedcap
{
public:
    
    AbstractRedcap();
    
    //! A Deconstructor
    /*!
     Details.
     */
    virtual ~AbstractRedcap();

    void init(vector<double> data, vector<bool> undefs, GalElement* w);
    
    // check if complete graph, no islands
    bool checkFirstOrderEdges();
   
    void Partitioning(int k);
    
    virtual void Clustering()=0;
    
    vector<int> cluster_ids;
    
protected:
    
    int num_obs;
    
    vector<double> data;
    
    vector<bool> undefs;
    
    vector<RedCapNode*> all_nodes;
    
    vector<RedCapEdge*> first_order_edges;
    
    //vector<RedCapEdge*> full_order_edges;
    
    SpatialContiguousTree* tree;
    
    vector<SpatialContiguousTree*> regions;
    
    
};

class FirstOrderSLKRedCap : public AbstractRedcap
{
public:
    FirstOrderSLKRedCap();
    FirstOrderSLKRedCap(vector<double> data, vector<bool> undefs, GalElement * w);
    virtual ~FirstOrderSLKRedCap();
    
    virtual void Clustering();
};

class FirstOrderALKRedCap : public AbstractRedcap
{
public:
    FirstOrderALKRedCap();
    virtual ~FirstOrderALKRedCap();
    
    virtual void Clustering();
    
     double** distance_matrix;
};

#endif
