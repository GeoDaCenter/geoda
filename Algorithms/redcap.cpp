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
#include "redcap.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////
//
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////
RedCapNode::RedCapNode(int _id, double _val)
{
    id = _id;
    value = _val;
}

RedCapNode::~RedCapNode()
{
    
}

void RedCapNode::AddNeighbor(RedCapNode* node)
{
    neighbors.insert(node);
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
    
    // no smooth
    length = abs(a->value - b->value);
}

RedCapEdge::~RedCapEdge()
{
}

/////////////////////////////////////////////////////////////////////////
//
// SpatialContiguousTree
//
/////////////////////////////////////////////////////////////////////////
SpatialContiguousTree::SpatialContiguousTree(const vector<RedCapNode*>& all_nodes, const vector<double>& _data, const vector<bool>& _undefs)
{
    left_child = NULL;
    right_child = NULL;
    
    data = _data;
    undefs = _undefs;
    
    for (int i=0; i<all_nodes.size(); i++) {
        RedCapNode* node = all_nodes[i];
        all_nodes_dict[node] = false; // false means not adding into tree yet
    }
    
    heterogeneity = calc_heterogeneity();
}

SpatialContiguousTree::SpatialContiguousTree(RedCapCluster* cluster, vector<RedCapEdge*> _edges, const vector<double>& _data, const vector<bool>& _undefs)
{
    left_child = NULL;
    right_child = NULL;
    
    data = _data;
    undefs = _undefs;
   
    map<RedCapNode*, bool>::iterator it;
    for (it=cluster->node_dict.begin(); it!=cluster->node_dict.end(); it++) {
        RedCapNode* node = it->first;
        all_nodes_dict[node] = false; // false means not adding into tree yet
    }
    
    // create edges
    for (int i=0; i<_edges.size(); i++) {
        RedCapEdge* e = _edges[i];
        if (cluster->Has(e->a) && cluster->Has(e->b)) {
            AddEdge(e);
        }
    }
    
    heterogeneity = calc_heterogeneity();
}


SpatialContiguousTree::~SpatialContiguousTree()
{
    if (left_child)
        delete left_child;
    if (right_child)
        delete right_child;
}

bool SpatialContiguousTree::AddEdge(RedCapEdge *edge)
{
    bool all_covered = true;
    // check if all nodes are covered by this tree
    map<RedCapNode*, bool>::iterator it;
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
    }
    return all_covered;
}

void SpatialContiguousTree::Split()
{
    // search best cut
    int hg = 0;
    RedCapEdge* e = NULL;
    
    for (int i=0; i<edges.size(); i++) {
        RedCapEdge* out_edge = edges[i];
        RedCapNode* a = out_edge->a;
        RedCapNode* b = out_edge->b;
       
        SpatialContiguousTree* left = findSubTree(a, b);
        SpatialContiguousTree* right = findSubTree(b, a);
   
        int hg_sub = heterogeneity - left->heterogeneity - right->heterogeneity;
        
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


SpatialContiguousTree* SpatialContiguousTree::findSubTree(RedCapNode* node, RedCapNode* exclude_node)
{
    RedCapCluster cluster;
    list<RedCapNode*> container;
    container.push_back(node);
   
    
    while (!container.empty()) {
        RedCapNode* tmp = container.front();
        container.pop_front();
        
        if (tmp->id != exclude_node->id) {
            if (cluster.Has(tmp))
                continue;
            cluster.AddNode(tmp);
        }
        
        set<RedCapNode*>::iterator it;
        for (it=tmp->neighbors.begin(); it!=tmp->neighbors.end(); it++) {
            if ((*it)->id != exclude_node->id) {
                container.push_back(*it);
            }
        }
    }
    
    SpatialContiguousTree* sub_tree = new SpatialContiguousTree(&cluster, edges, data, undefs);
    return sub_tree;
}

double SpatialContiguousTree::calc_heterogeneity()
{
    // sum of squared deviations
    vector<double> tmp_data;
   
    map<RedCapNode*, bool>::iterator it;
    for (it=all_nodes_dict.begin(); it!=all_nodes_dict.end(); it++) {
        RedCapNode* node = it->first;
        int i = node->id;
        if (undefs[i]) continue;
        tmp_data.push_back(data[i]);
    }
    double ssd = GenUtils::GetVariance(tmp_data);
    return ssd;
}


//////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
//////////////////////////////////////////////////////////////////////////////////

bool RedCapEdgeLess(RedCapEdge* a, RedCapEdge* b)
{
    return a->length < b->length;
}

AbstractRedcap::AbstractRedcap()
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

void AbstractRedcap::init(vector<double> _data, vector<bool> _undefs, GalElement * w)
{
    data = _data;
    undefs = _undefs;
    
    num_obs = data.size();
    cluster_ids.resize(num_obs);
    
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) continue;
        RedCapNode* node = new RedCapNode(i, data[i]);
        all_nodes.push_back(node);
    }
  
    // create first_order_edges
    map<pair<int, int>, bool> pair_dict;
    
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

void AbstractRedcap::Partitioning(int k)
{
    list<SpatialContiguousTree*> sub_trees;
    sub_trees.push_back(tree);
    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        SpatialContiguousTree* tmp_tree = sub_trees.front();
        sub_trees.pop_front();
        tmp_tree->Split();
        sub_trees.push_back(tmp_tree->GetLeftChild());
        sub_trees.push_back(tmp_tree->GetRightChild());
    }
   
    int cid = 1;
    list<SpatialContiguousTree*>::iterator it;
    map<RedCapNode*, bool>::iterator node_it;
    
    for (it=sub_trees.begin(); it!=sub_trees.end(); it++) {
        SpatialContiguousTree* _tree = *it;
        for (node_it=_tree->all_nodes_dict.begin();
             node_it!=_tree->all_nodes_dict.end(); node_it++)
        {
            RedCapNode* node = node_it->first;
            cluster_ids[node->id] = cid;
        }
        cid += 1;
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
    map<RedCapNode*, bool>::iterator it;
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
FirstOrderSLKRedCap::FirstOrderSLKRedCap(vector<double> data, vector<bool> undefs, GalElement * w)
{
    init(data, undefs, w);
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
FirstOrderALKRedCap::FirstOrderALKRedCap()
{
    
}

FirstOrderALKRedCap::~FirstOrderALKRedCap()
{
    
}

void FirstOrderALKRedCap::Clustering()
{
    
}
