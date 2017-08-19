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
#include <boost/unordered_map.hpp>

#include "../ShapeOperations/GalWeight.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "cluster.h"
#include "redcap.h"

using namespace std;
using namespace boost;


/////////////////////////////////////////////////////////////////////////
//
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////
RedCapNode::RedCapNode(int _id, const vector<double>& _value)
: value(_value)
{
    id = _id;
}

RedCapNode::RedCapNode(RedCapNode* node)
: value(node->value)
{
    id = node->id;
    
    std::set<RedCapNode*>::iterator it;
    for( it=node->neighbors.begin(); it!=node->neighbors.end(); it++) {
        neighbors.insert(*it);
    }
}

RedCapNode::~RedCapNode()
{
}

void RedCapNode::AddNeighbor(RedCapNode* node)
{
    neighbors.insert(node);
}

void RedCapNode::RemoveNeighbor(RedCapNode* node)
{
    neighbors.erase(neighbors.find(node));
}

/////////////////////////////////////////////////////////////////////////
//
// RedCapEdge
//
/////////////////////////////////////////////////////////////////////////
RedCapEdge::RedCapEdge(RedCapNode* _a, RedCapNode* _b, double _w)
{
    a = _a;
    b = _b;
    weight = _w;
    length = 0;
    
    // no smooth
    int n_vars = a->value.size();
    
    for (int i=0; i<n_vars; i++) {
        double diff = a->value[i] - b->value[i];
        length += diff * diff;
    }
}

RedCapEdge::~RedCapEdge()
{
}

/////////////////////////////////////////////////////////////////////////
//
// SpatialContiguousTree
//
/////////////////////////////////////////////////////////////////////////
SpatialContiguousTree::SpatialContiguousTree(const vector<RedCapNode*>& all_nodes, const vector<vector<double> >& _data, const vector<bool>& _undefs)
: data(_data), undefs(_undefs)
{
    left_child = NULL;
    right_child = NULL;
    root = NULL;
    
    for (int i=0; i<all_nodes.size(); i++) {
        RedCapNode* node = all_nodes[i];
        all_nodes_dict[node] = false; // false means not adding into tree yet
    }
    heterogeneity = calcHeterogeneity();
}

SpatialContiguousTree::SpatialContiguousTree(RedCapNode* graph, RedCapNode* exclude_node, unordered_map<int, RedCapNode*> parent_ids_dict, const vector<vector<double> >& _data, const vector<bool>& _undefs)
: data(_data), undefs(_undefs)
{
    // create a tree when remove an edge from its parent
    // from a given node, dont go to branch = exclue_node
    // also make sure all nodes are valid (within parent_ids)
    left_child = NULL;
    right_child = NULL;
    root = NULL;
   
    list<RedCapNode*> stack;
    stack.push_back(graph);
    int num_nodes = 1;
    std::set<RedCapNode*>::iterator it;
    RedCapNode* nn = NULL;
    
    while(!stack.empty()){
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        
        std::set<RedCapNode*>& nbrs = tmp->neighbors;
        
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            nn = *it;
            if (nn->id != exclude_node->id &&
                ids_dict.find(nn->id) == ids_dict.end() && // not add yet
                parent_ids_dict.find(nn->id) != parent_ids_dict.end()) // in parent
            {
                stack.push_back(nn);
                AddEdgeDirectly(tmp, nn); // create a copy of nodes
                num_nodes +=1;
            }
        }
    }
    if (num_nodes == 1) {
        // take care of one node case
        all_nodes_dict[graph] = true;
        root = graph;
        ids_dict[graph->id] = graph;
    }
    heterogeneity = calcHeterogeneity();
}

SpatialContiguousTree::~SpatialContiguousTree()
{
    if (left_child)
        delete left_child;
    if (right_child)
        delete right_child;
    for (int i=0; i<new_nodes.size();i++) {
        delete new_nodes[i];
    }
}

bool SpatialContiguousTree::AddEdge(RedCapEdge *edge)
{
    bool all_covered = true;
    // check if all nodes are covered by this tree
    unordered_map<RedCapNode*, bool>::iterator it;
    for (it=all_nodes_dict.begin(); it!=all_nodes_dict.end(); it++) {
        bool is_node_covered = it->second;
        if (is_node_covered == false) {
            all_covered = false;
            break;
        }
    }
    if (all_covered == false) {
        RedCapNode* a = edge->a;
        RedCapNode* b = edge->b;
        
        all_nodes_dict[edge->a] = true;
        all_nodes_dict[edge->b] = true;
        edges.push_back(edge);
        
        a->AddNeighbor(b);
        b->AddNeighbor(a);
        
        ids_dict[a->id] = a;
        ids_dict[b->id] = b;
        
        if (root == NULL)
            root = a;
    }
    return all_covered;
}

void SpatialContiguousTree::AddEdgeDirectly(RedCapNode* _a, RedCapNode* _b)
{
    // if Node not exists, create a new one, store it
    
    int aid = _a->id;
    int bid = _b->id;
    
    RedCapNode* a;
    if (ids_dict.find(aid) == ids_dict.end()) {
        a = new RedCapNode(_a);
        new_nodes.push_back(a);
    } else {
        a = ids_dict[aid];
    }
    
    RedCapNode* b;
    if (ids_dict.find(bid) == ids_dict.end()) {
        b = new RedCapNode(_b);
        new_nodes.push_back(b);
    } else {
        b = ids_dict[bid];
    }
    
    all_nodes_dict[a] = true;
    all_nodes_dict[b] = true;
    
    ids_dict[a->id] = a;
    ids_dict[b->id] = b;
    
    edges.push_back(new RedCapEdge(a, b));
    
    a->AddNeighbor(b);
    b->AddNeighbor(a);
    
    if (root == NULL)
        root = a;
}

void SpatialContiguousTree::Split()
{
    // search best cut
    double hg = 0;
    RedCapEdge* e = NULL;
    
    for (int i=0; i<edges.size(); i++) {
        RedCapEdge* out_edge = edges[i];
        RedCapNode* a = out_edge->a;
        RedCapNode* b = out_edge->b;
       
        SpatialContiguousTree* left = new SpatialContiguousTree(a, b, ids_dict, data, undefs);
        SpatialContiguousTree* right = new SpatialContiguousTree(b, a, ids_dict, data, undefs);
   
        double hg_sub = heterogeneity - left->heterogeneity - right->heterogeneity;
        
        if (hg_sub > hg) {
            if (left_child) delete left_child;
            if (right_child) delete right_child;
            hg = hg_sub;
            e = out_edge;
            left_child = left;
            right_child = right;
        } else {
            if (left) delete left;
            if (right) delete right;
        }
    }
}

SpatialContiguousTree* SpatialContiguousTree::GetLeftChild()
{
    return left_child;
}

SpatialContiguousTree* SpatialContiguousTree::GetRightChild()
{
    return right_child;
}

double SpatialContiguousTree::calcHeterogeneity()
{
    // sum of squared deviations
    double sum_ssd = 0;
    int n_vars = data.size();
    for (int i=0; i<n_vars; i++) {
        vector<double> tmp_data;
        unordered_map<RedCapNode*, bool>::iterator it;
        for (it=all_nodes_dict.begin(); it!=all_nodes_dict.end(); it++) {
            RedCapNode* node = it->first;
            int id = node->id;
            if (undefs[id]) continue;
            tmp_data.push_back(data[i][id]);
        }
        double ssd = GenUtils::GetVariance(tmp_data);
        sum_ssd += ssd;
    }
    return sum_ssd;
}

//////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
//////////////////////////////////////////////////////////////////////////////////
AbstractRedcap::AbstractRedcap(const vector<vector<double> >& _data, const vector<bool>& _undefs)
: data(_data), undefs(_undefs)
{
    
}

AbstractRedcap::~AbstractRedcap()
{
    delete tree;
    for (int i=0; i<first_order_edges.size(); i++) {
        delete first_order_edges[i];
    }
    for (int i=0; i<all_nodes.size(); i++) {
        delete all_nodes[i];
    }
}

bool RedCapEdgeLess(RedCapEdge* a, RedCapEdge* b)
{
    return a->length < b->length;
}

void AbstractRedcap::init( GalElement * w)
{
    num_obs = data.size();
    num_vars = data[0].size();
    cluster_ids.resize(num_obs);
    
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) continue;
        RedCapNode* node = new RedCapNode(i, data[i]);
        all_nodes.push_back(node);
    }
  
    // create first_order_edges
    unordered_map<pair<int, int>, bool> pair_dict;
    
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) continue;
        const vector<long>& nbrs = w[i].GetNbrs();
        const vector<double>& nbrs_w = w[i].GetNbrWeights();
        for (int j=0; j<nbrs.size(); j++) {
            int nbr = nbrs[j];
            if (undefs[nbr] || i == nbr) continue;
            if (pair_dict.find(make_pair(i,nbr)) == pair_dict.end()) {
                double w = nbrs_w[nbr];
                RedCapNode* a = all_nodes[i];
                RedCapNode* b = all_nodes[nbr];
                RedCapEdge* e = new RedCapEdge(a, b, w);
                first_order_edges.push_back(e);
                pair_dict[make_pair(i,nbr)] = true;
                pair_dict[make_pair(nbr,i)] = true;
            }
        }
    }
    
    // init spatialContiguousTree
    tree = new SpatialContiguousTree(all_nodes, data, undefs);
    
    Clustering();
}

bool AbstractRedcap::checkFirstOrderEdges()
{
    return true;
}

vector<vector<int> >& AbstractRedcap::GetRegions()
{
    return cluster_ids;
}

void AbstractRedcap::Partitioning(int k)
{
    list<SpatialContiguousTree*> sub_trees;
    sub_trees.push_back(tree);
    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        SpatialContiguousTree* tmp_tree = sub_trees.front();
        sub_trees.pop_front();
        tmp_tree->Split();
       
        SpatialContiguousTree* left_tree = tmp_tree->GetLeftChild();
        SpatialContiguousTree* right_tree = tmp_tree->GetRightChild();
       
        if (left_tree)
            sub_trees.push_back(left_tree);
        if (right_tree)
            sub_trees.push_back(right_tree);
        
        if (left_tree== NULL && right_tree ==NULL) {
            // only one item, push it back
            sub_trees.push_back(tmp_tree);
        }
    }
   
    list<SpatialContiguousTree*>::iterator it;
    unordered_map<RedCapNode*, bool>::iterator node_it;
    
    for (it=sub_trees.begin(); it!=sub_trees.end(); it++) {
        SpatialContiguousTree* _tree = *it;
        vector<int> cluster;
        for (node_it=_tree->all_nodes_dict.begin();
             node_it!=_tree->all_nodes_dict.end(); node_it++)
        {
            RedCapNode* node = node_it->first;
            cluster.push_back(node->id);
        }
        cluster_ids.push_back(cluster);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
//////////////////////////////////////////////////////////////////////////////////
RedCapCluster::RedCapCluster(RedCapEdge* edge)
{
    node_dict[edge->a] = true;
    node_dict[edge->b] = true;
}

RedCapCluster::RedCapCluster()
{
}

RedCapCluster::~RedCapCluster()
{
}

bool RedCapCluster::Has(RedCapNode* node)
{
    return node_dict.find(node) != node_dict.end();
}

void RedCapCluster::AddNode(RedCapNode* node)
{
    node_dict[node] = true;
}

void RedCapCluster::Merge(RedCapCluster* cluster)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    for (it=cluster->node_dict.begin(); it!=cluster->node_dict.end(); it++) {
        node_dict[it->first] = true;
    }
}
//////////////////////////////////////////////////////////////////////////////////
//
// RedCapClusterManager
//
//////////////////////////////////////////////////////////////////////////////////
RedCapClusterManager::RedCapClusterManager()
{
}

RedCapClusterManager::~RedCapClusterManager()
{
    for (int i=0; i<clusters.size(); i++) {
        delete clusters[i];
    }
}

bool RedCapClusterManager::Update(RedCapEdge* edge)
{
    bool b_connect_clusters = true;
    RedCapCluster* c1 = getCluster(edge->a);
    RedCapCluster* c2 = getCluster(edge->b);
    
    if (c1 == NULL && c2 == NULL) {
        createCluster(edge);
    } else if (c1 == NULL && c2) {
        mergeToCluster(edge->a, c2);
    } else if (c1 && c2 == NULL) {
        mergeToCluster(edge->b, c1);
    } else {
        if (c1 != c2)
            mergeClusters(c1, c2);
        else
            b_connect_clusters = false;
    }
    return b_connect_clusters;
}

RedCapCluster* RedCapClusterManager::getCluster(RedCapNode* node)
{
    for (int i=0; i<clusters.size(); i++) {
        RedCapCluster* cluster = clusters[i];
        if (cluster->Has(node)) {
            return cluster;
        }
    }
    return NULL;
}

void RedCapClusterManager::createCluster(RedCapEdge *edge)
{
    RedCapCluster* new_cluster = new RedCapCluster(edge);
    clusters.push_back(new_cluster);
}

void RedCapClusterManager::mergeToCluster(RedCapNode* node, RedCapCluster* cluster)
{
    cluster->AddNode(node);
}

void RedCapClusterManager::mergeClusters(RedCapCluster* cluster1, RedCapCluster* cluster2)
{
    cluster1->Merge(cluster2);
  
    int idx_to_remove = -1;
    for (int i=0; i<clusters.size(); i++) {
        if (clusters[i] == cluster2) {
            idx_to_remove = i;
        }
    }
    delete cluster2;
    if (idx_to_remove>=0) {
        clusters.erase(clusters.begin() + idx_to_remove);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
//////////////////////////////////////////////////////////////////////////////////
FirstOrderSLKRedCap::FirstOrderSLKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w)
: AbstractRedcap(_data, _undefs)
{
    init(w);
}

FirstOrderSLKRedCap::~FirstOrderSLKRedCap()
{
    
}

void FirstOrderSLKRedCap::Clustering()
{
    RedCapClusterManager cm;
    
    // sort edges based on length
    std::sort(first_order_edges.begin(), first_order_edges.end(), RedCapEdgeLess);
    for (int i=0; i<first_order_edges.size(); i++) {
        RedCapEdge* edge = first_order_edges[i];
        // if an edge connects two different clusters, it is added to T,
        // and the two clusters are merged;
        bool b_connect_clusters = cm.Update(edge);
        if (!b_connect_clusters)
            continue;
        bool b_all_node_covered = tree->AddEdge(edge);
        if (b_all_node_covered)
            // stop when all nodes are covered by this tree
            break;
    }
}

//////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
//
//////////////////////////////////////////////////////////////////////////////////
FirstOrderALKRedCap::FirstOrderALKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs)
: AbstractRedcap(_data, _undefs)
{
    
}

FirstOrderALKRedCap::~FirstOrderALKRedCap()
{
    
}

void FirstOrderALKRedCap::Clustering()
{
    
}
