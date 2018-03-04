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
#include <iterator>
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

typedef map<pair<SpatialContiguousTree*, SpatialContiguousTree*>, double> SubTrees;

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
    // node will be released at it's container
}

/////////////////////////////////////////////////////////////////////////
//
// SpatialContiguousTree
//
/////////////////////////////////////////////////////////////////////////
SpatialContiguousTree::SpatialContiguousTree(const vector<RedCapNode*>& all_nodes, const vector<vector<double> >& _data, const vector<bool>& _undefs, double* _controls, double _control_thres)
: data(_data), undefs(_undefs),controls(_controls), control_thres(_control_thres)
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

SpatialContiguousTree::SpatialContiguousTree(RedCapNode* graph, RedCapNode* exclude_node, unordered_map<int, RedCapNode*> parent_ids_dict, const vector<vector<double> >& _data, const vector<bool>& _undefs, double* _controls, double _control_thres)
: data(_data), undefs(_undefs), controls(_controls), control_thres(_control_thres)
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
    
    // this is wrong:
    // all nodes in this tree should be the same, except the "root" and the "exclude_node"
    // so, let's make a copy of "root" and "exclude_node",
    // then make a tree starts from "root"
    // NOTE: what you need is a DEEP-copy when create sub-tree
    
    while(!stack.empty()){
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        
        std::set<RedCapNode*>& nbrs = tmp->neighbors;
        
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            nn = *it;
            if (nn->id != exclude_node->id &&
                ids_dict.find(nn->id) == ids_dict.end())// && // not add yet
               // parent_ids_dict.find(nn->id) != parent_ids_dict.end()) // in parent
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
    int n_tasks = edges.size();

    int nCPUs = boost::thread::hardware_concurrency();
    int quotient = n_tasks / nCPUs;
    int remainder = n_tasks % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
    cand_trees.clear();
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
        boost::thread* worker = new boost::thread(boost::bind(&SpatialContiguousTree::subSplit, this, a, b));
        threadPool.add_thread(worker);
    }
    threadPool.join_all();
    
    // merge results
    bool is_first = true;
    double best_hg;
    
    SubTrees::iterator it;
    for (it=cand_trees.begin(); it!=cand_trees.end(); it++) {
        pair<SpatialContiguousTree*, SpatialContiguousTree*> left_right = it->first;
        SpatialContiguousTree* left = left_right.first;
        SpatialContiguousTree* right = left_right.second;
        double hg = it->second;
        
        if (is_first || hg < best_hg) {
            best_hg = hg;
            is_first = false;
            if (left_child) delete left_child;
            if (right_child) delete right_child;
            left_child = left;
            right_child = right;
        } else {
            if (left) delete left;
            if (right) delete right;
        }
    }
}

bool SpatialContiguousTree::quickCheck(RedCapNode* node, RedCapNode* exclude_node)
{
    if (controls == NULL) return false;
    
    double check_val = 0;
    unordered_map<int, bool> visited;
    
    list<RedCapNode*> stack;
    stack.push_back(node);
    while(!stack.empty()){
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        visited[tmp->id] = true;
        check_val += controls[tmp->id];
        
        std::set<RedCapNode*>& nbrs = tmp->neighbors;
        std::set<RedCapNode*>::iterator it;
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            RedCapNode* nn = *it;
            if (nn->id != exclude_node->id &&
                visited.find(nn->id) == visited.end())// && // not add yet
                // parent_ids_dict.find(nn->id) != parent_ids_dict.end()) // in parent
            {
                stack.push_back(nn);
                visited[nn->id] = true;
            }
        }
    }
    
    return check_val >  control_thres;
}

void SpatialContiguousTree::subSplit(int start, int end)
{
    // search best cut
    double hg = 0;
    RedCapEdge* e = NULL;
    SpatialContiguousTree* left_best=NULL;
    SpatialContiguousTree* right_best=NULL;
    
    bool is_first = true;
    //for (int i=0; i<edges.size(); i++) {
    for (int i=start; i<=end; i++) {
        if (i >= edges.size()) continue;
        RedCapEdge* out_edge = edges[i];
        RedCapNode* a = out_edge->a;
        RedCapNode* b = out_edge->b;
      
        // do a quick check, if not satisfy control, then continue
        if (controls && (!quickCheck(a, b) || !quickCheck(b,a))) {
            continue;
        }
        
        SpatialContiguousTree* left = new SpatialContiguousTree(a, b, ids_dict, data, undefs, controls, control_thres);
        SpatialContiguousTree* right = new SpatialContiguousTree(b, a, ids_dict, data, undefs, controls, control_thres);
 
        if (left == NULL && right == NULL) {
            // can't be split-ted
            continue;
        }
        
        if (controls) {
            bool good = (left && left->checkControl()) &&  (right && right->checkControl());
            //printf("left->count:%d, right->count:%d, good:%d\n", left->all_nodes_dict.size(), right->all_nodes_dict.size(), good);
            if (!good) {
                if (left)  { delete left; left = NULL; }
                if (right) { delete right; right = NULL;}
                continue;
            }
        }
        
        double hg_sub = left->heterogeneity + right->heterogeneity;
        
        if (is_first || hg_sub < hg) {
            is_first = false;
            if (left_best) delete left_best;
            if (right_best) delete right_best;
            hg = hg_sub;
            e = out_edge;
            left_best = left;
            right_best = right;
        } else {
            if (left) delete left;
            if (right) delete right;
        }
    }
    if (left_best != NULL && right_best != NULL)
        cand_trees[make_pair(left_best, right_best)] = hg;
}

bool SpatialContiguousTree::checkControl()
{
    if (controls == NULL) return true;
   
    double val = 0;
    unordered_map<int, RedCapNode*>::iterator it;
    for (it=ids_dict.begin(); it!=ids_dict.end(); it++) {
        int idx = it->first;
        val += controls[idx];
    }
    
    return val > control_thres;
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
    int n_vars = data[0].size();
    for (int i=0; i<n_vars; i++) {
        vector<double> tmp_data;
        unordered_map<RedCapNode*, bool>::iterator it;
        for (it=all_nodes_dict.begin(); it!=all_nodes_dict.end(); it++) {
            RedCapNode* node = it->first;
            int id = node->id;
            if (undefs[id]) continue;
            tmp_data.push_back(data[id][i]);
        }
        double ssd = GenUtils::GetVariance(tmp_data);
        sum_ssd += ssd;
    }
    return sum_ssd;
}

////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
////////////////////////////////////////////////////////////////////////////////
AbstractRedcap::AbstractRedcap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * _w)
: data(_data), undefs(_undefs), w(_w)
{
    
}

AbstractRedcap::~AbstractRedcap()
{
    delete mstree;
    for (int i=0; i<order_edges.size(); i++) {
        delete order_edges[i];
    }
    for (int i=0; i<all_nodes.size(); i++) {
        delete all_nodes[i];
    }
}

bool RedCapEdgeLess(RedCapEdge* a, RedCapEdge* b)
{
    return a->length < b->length;
}

bool RedCapEdgeLarger(RedCapEdge* a, RedCapEdge* b)
{
    return a->length > b->length;
}

void AbstractRedcap::init()
{
    num_obs = data.size();
    num_vars = data[0].size();
    
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) continue;
        RedCapNode* node = new RedCapNode(i, data[i]);
        all_nodes.push_back(node);
    }
  
    // create order_edges
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) continue;
        const vector<long>& nbrs = w[i].GetNbrs();
        const vector<double>& nbrs_w = w[i].GetNbrWeights();
        for (int j=0; j<nbrs.size(); j++) {
            int nbr = nbrs[j];
            if (undefs[nbr] || i == nbr)
                continue;
            if (fo_edge_dict.find(make_pair(i,nbr)) == fo_edge_dict.end()) {
                double w = nbrs_w[j];
                RedCapNode* a = all_nodes[i];
                RedCapNode* b = all_nodes[nbr];
                RedCapEdge* e = new RedCapEdge(a, b, w);
                order_edges.push_back(e);
                fo_edge_dict[make_pair(i,nbr)] = e->length;
                fo_edge_dict[make_pair(nbr,i)] = e->length;
            }
        }
    }
    
    // init spatialContiguousTree
    mstree = new SpatialContiguousTree(all_nodes, data, undefs, controls, control_thres);
    
    Clustering();
}

void AbstractRedcap::createFullOrderEdges(vector<RedCapEdge*>& edges)
{
    for (int i=0; i<num_obs; i++) {
        if (undefs[i])
            continue;
        for (int j=0; j<num_obs; j++) {
            if (i == j)
                continue;
            RedCapNode* a = all_nodes[i];
            RedCapNode* b = all_nodes[j];
            RedCapEdge* e = new RedCapEdge(a, b, 1);
            edges.push_back(e);
        }
    }
}

bool AbstractRedcap::checkFirstOrderEdges()
{
    return true;
}

vector<vector<int> >& AbstractRedcap::GetRegions()
{
    return cluster_ids;
}

bool RedCapTreeLess(SpatialContiguousTree* a, SpatialContiguousTree* b)
{
    //return a->all_nodes_dict.size() < b->all_nodes_dict.size();
    return a->heterogeneity < b->heterogeneity;
}

void AbstractRedcap::Partitioning(int k)
{
    vector<SpatialContiguousTree*> sub_trees;
    sub_trees.push_back(mstree);
    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        SpatialContiguousTree* tmp_tree = sub_trees.back();
        sub_trees.pop_back();
        tmp_tree->Split();
       
        SpatialContiguousTree* left_tree = tmp_tree->GetLeftChild();
        SpatialContiguousTree* right_tree = tmp_tree->GetRightChild();
       
        if (left_tree== NULL && right_tree ==NULL) {
            break;
            //if (tmp_tree->all_nodes_dict.size()== 1)
                // only one item, push it back
                //sub_trees.push_back(tmp_tree);
        }
        
        if (left_tree)
            sub_trees.push_back(left_tree);
        if (right_tree)
            sub_trees.push_back(right_tree);
        
        // sort by number of items
        std::sort(sub_trees.begin(), sub_trees.end(), RedCapTreeLess);
    }
  
    cluster_ids.clear();
    vector<SpatialContiguousTree*>::iterator it;
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

////////////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
////////////////////////////////////////////////////////////////////////////////
RedCapCluster::RedCapCluster(RedCapEdge* edge)
{
    node_dict[edge->a] = true;
    node_dict[edge->b] = true;
}

RedCapCluster::RedCapCluster(RedCapNode* node)
{
    // single node cluster
    node_dict[node] = true;
}

RedCapCluster::~RedCapCluster()
{
}

int RedCapCluster::size()
{
    return node_dict.size();
}

RedCapNode* RedCapCluster::GetNode(int i)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    it = node_dict.begin();
    std::advance(it, i);
    return it->first;
}

bool RedCapCluster::Has(RedCapNode* node)
{
    return node_dict.find(node) != node_dict.end();
}

void RedCapCluster::AddNode(RedCapNode* node)
{
    node_dict[node] = true;
}

void RedCapCluster::AddEdge(RedCapEdge* e)
{
    edge_dict[e] = true;
}

void RedCapCluster::Merge(RedCapCluster* cluster)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    for (it=cluster->node_dict.begin(); it!=cluster->node_dict.end(); it++) {
        node_dict[it->first] = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// RedCapClusterManager
//
////////////////////////////////////////////////////////////////////////////////
RedCapClusterManager::RedCapClusterManager()
{
}

RedCapClusterManager::~RedCapClusterManager()
{
    for (it=clusters_dict.begin(); it!=clusters_dict.end(); it++) {
        delete it->first;
    }
}

RedCapCluster* RedCapClusterManager::UpdateByAdd(RedCapEdge* edge)
{
    // updte clusters by adding edge
    RedCapCluster* c1 = getCluster(edge->a);
    RedCapCluster* c2 = getCluster(edge->b);
    
    if (c1 == c2)
        return c1;
    
    return mergeClusters(c1, c2);
}

bool RedCapClusterManager::HasCluster(RedCapCluster* c)
{
    return clusters_dict.find(c) != clusters_dict.end();
}

bool RedCapClusterManager::CheckContiguity(RedCapCluster* l, RedCapCluster* m, GalElement* w)
{
    unordered_map<int, bool> l_nbrs;
    for (int i=0; i< l->size(); i++) {
        RedCapNode* node = l->GetNode(i);
        vector<long> nbrs = w[node->id].GetNbrs();
        for (int j=0; j< nbrs.size(); j++) {
            l_nbrs[ nbrs[j] ] = true;
        }
    }
    
    // check if any match between l_nbrs and m
    for (int i=0; i< m->size(); i++) {
        RedCapNode* node = m->GetNode(i);
        if (l_nbrs.find( node->id ) != l_nbrs.end()) {
            return true;
        }
    }
    return false;
}

bool RedCapClusterManager::CheckConnectivity(RedCapEdge* edge, GalElement* w, RedCapCluster* l, RedCapCluster* m)
{
    // check if edge connects two clusters, if yes, then return l and m
    l = getCluster(edge->a);
    m = getCluster(edge->b);
    
    bool b_connect = CheckContiguity(l, m, w);

    return b_connect;
}

RedCapCluster* RedCapClusterManager::getCluster(RedCapNode* node)
{
    for (it=clusters_dict.begin(); it!=clusters_dict.end(); it++) {
        RedCapCluster* cluster = it->first;
        if (cluster->Has(node)) {
            return cluster;
        }
    }
    return createCluster(node); // only one node
}

RedCapCluster* RedCapClusterManager::createCluster(RedCapNode *node)
{
    RedCapCluster* new_cluster = new RedCapCluster(node);
    clusters_dict[new_cluster] = true;
    return new_cluster;
}

RedCapCluster* RedCapClusterManager::createCluster(RedCapEdge *edge)
{
    RedCapCluster* new_cluster = new RedCapCluster(edge);
    clusters_dict[new_cluster] = true;
    return new_cluster;
}

RedCapCluster* RedCapClusterManager::mergeToCluster(RedCapNode* node, RedCapCluster* cluster)
{
    cluster->AddNode(node);
    return cluster;
}

RedCapCluster* RedCapClusterManager::mergeClusters(RedCapCluster* cluster1, RedCapCluster* cluster2)
{
    cluster1->Merge(cluster2);
  
    clusters_dict.erase(clusters_dict.find(cluster2));
    delete cluster2;
    
    return cluster1;
}

bool RedCapClusterManager::GetAvgEdgeLength(RedCapCluster* c1, RedCapCluster* c2, double* length, unordered_map<pair<int, int>, double>& e_dict)
{
    double nn = 0;
    double e_length = 0;
    unordered_map<RedCapNode*, bool>::iterator it1;
    unordered_map<RedCapNode*, bool>::iterator it2;
    for (it1=c1->node_dict.begin(); it1!=c1->node_dict.end(); it1++) {
        for (it2=c2->node_dict.begin(); it2!=c2->node_dict.end(); it2++) {
            pair<int, int> p = make_pair((it1->first)->id, (it2->first)->id);
            if (e_dict.find(p) != e_dict.end()) {
                nn += 1;
                e_length += e_dict[p];
            }
        }
    }
    if (nn > 0) {
        *length = e_length / nn;
        return true;
    }
    return false;
}

bool RedCapClusterManager::GetMaxEdgeLength(RedCapCluster* c1, RedCapCluster* c2, double* length, unordered_map<pair<int, int>, double>& e_dict)
{
    double nn = 0;
    double max_length = 0;
    unordered_map<RedCapNode*, bool>::iterator it1;
    unordered_map<RedCapNode*, bool>::iterator it2;
    for (it1=c1->node_dict.begin(); it1!=c1->node_dict.end(); it1++) {
        for (it2=c2->node_dict.begin(); it2!=c2->node_dict.end(); it2++) {
            pair<int, int> p = make_pair((it1->first)->id, (it2->first)->id);
            if (e_dict.find(p) != e_dict.end()) {
                if (e_dict[p] > max_length)
                    nn += 1;
                    max_length = e_dict[p];
            }
        }
    }
    if (nn > 0) {
        *length = max_length;
        return true;
    }
    return false;
}


void RedCapClusterManager::UpdateEdgeLength(RedCapCluster* c1, RedCapCluster* c2, double new_length, vector<RedCapEdge*>& edges)
{
    for (int i=0; i<edges.size(); i++) {
        RedCapNode* a = edges[i]->a;
        RedCapNode* b = edges[i]->b;
        if ( (c1->Has(a) && c2->Has(b)) || (c2->Has(a) && c1->Has(b)) ) {
            edges[i]->length = new_length;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderSLKRedCap::FirstOrderSLKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
: AbstractRedcap(_data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderSLKRedCap::~FirstOrderSLKRedCap()
{
    
}

void FirstOrderSLKRedCap::Clustering()
{
    // step 1. sort edges based on length
    std::sort(order_edges.begin(), order_edges.end(), RedCapEdgeLess);
    
    // step 2.
    for (int i=0; i<order_edges.size(); i++) {
        RedCapEdge* edge = order_edges[i];
        
        // if an edge connects two different clusters, it is added to T,
        // and the two clusters are merged;
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, w, l, m) ) {
            continue;
        }
        
        cm.UpdateByAdd(edge);
        
        bool b_all_node_covered = mstree->AddEdge(edge);
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
// The First-Order-ALK method also starts with the spatially contiguous graph G*. However, after each merge, the distance between the new cluster and every other cluster is recalculated. Therefore, edges that connect the new cluster and every other cluster are updated with new length values. Edges in G* are then re-sorted and re-evaluated from the beginning. The procedure stops when all objects are in one cluster. The algorithm is shown in figure 3. The complexity is O(n2log n) due to the sorting after each merge.
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderALKRedCap::FirstOrderALKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderALKRedCap::~FirstOrderALKRedCap()
{
    
}

double FirstOrderALKRedCap::getALKDistance(RedCapCluster* l, RedCapCluster* m)
{
    if (avgDist.find(make_pair(l, m)) == avgDist.end())
        return 0;
    return avgDist[make_pair(l, m)];
}

bool FirstOrderALKRedCap::updateALKDistanceToCluster(RedCapCluster* l, vector<RedCapEdge*>& E)
{
    bool dist_changed = false;
    unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
    
    for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
        RedCapCluster* c = it->first;
        if (c != l) {
            // avgDist(c, l) = average length of edges in E that connects c and l
            double avg_length=0;
            bool is_connect = cm.GetAvgEdgeLength(c, l, &avg_length, fo_edge_dict);
            if (is_connect == false)
                continue;
            avgDist[make_pair(c, l)] = avg_length;
            avgDist[make_pair(l, c)] = avg_length;
            // update the legnth of edge(c,l) in E with avgDist(c, l)
            cm.UpdateEdgeLength(c, l, avg_length, E);
            dist_changed = true;
        }
    }
    return dist_changed;
}

/**
 *
 */
void FirstOrderALKRedCap::Clustering()
{
    // make a copy of order_edges
    vector<RedCapEdge*> E = order_edges;
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);
    
    // 2. construct an nxn matrix avgDist to store distances between clusters
    // avgDist
    
    // 3. For each edge e in the sorted list (shortest first)
    while (!E.empty()) {
        // get shortest edge
        RedCapEdge* edge = E.back();
        E.pop_back();
        
        // If e connects two different clusters l, m, and e.length >= avgDist(l,m)
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, w, l, m) ) {
            continue;
        }
        if ( edge->length < getALKDistance(l, m) ) {
            continue;
        }
        
        // (1) find shortest edge e' in E that connnect cluster l and m
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, w, l, m)) {
                if (tmp_e->length < edge->length)
                    edge = tmp_e;
            }
        }
        
        // (2) add e'to T ane merge m to l (l is now th new cluster)
        l = cm.UpdateByAdd(edge);
        bool b_all_node_covered = mstree->AddEdge(edge);
        
        // (3) for each cluster c that is not l
        //      update avgDist(c, l) in E that connects c and l
        bool dist_changed = updateALKDistanceToCluster(l, E);
        
        // (4) sort all edges in E and set e = shortest one in the list
        if (dist_changed)
            sort(E.begin(), E.end(), RedCapEdgeLarger);
        
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 3 FirstOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderCLKRedCap::FirstOrderCLKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderCLKRedCap::~FirstOrderCLKRedCap()
{
    
}

double FirstOrderCLKRedCap::getCLKDistance(RedCapCluster* l, RedCapCluster* m)
{
    if (maxDist.find(make_pair(l, m)) == maxDist.end())
        return 0;
    return maxDist[make_pair(l, m)];
}

bool FirstOrderCLKRedCap::updateCLKDistanceToCluster(RedCapCluster* l, vector<RedCapEdge*>& E)
{
    bool dist_changed = false;
    unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
    
    for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
        RedCapCluster* c = it->first;
        if (c != l) {
            // maxDist(c, l) = max length of edges in E that connects c and l
            double max_length=0;
            bool is_connect = cm.GetMaxEdgeLength(c, l, &max_length, fo_edge_dict);
            if (is_connect == false)
                continue;
            maxDist[make_pair(c, l)] = max_length;
            maxDist[make_pair(l, c)] = max_length;
            // update the legnth of edge(c,l) in E with maxDist(c, l)
            cm.UpdateEdgeLength(c, l, max_length, E);
            dist_changed = true;
        }
    }
    return dist_changed;
}

void FirstOrderCLKRedCap::Clustering()
{
    // make a copy of order_edges
    vector<RedCapEdge*> E = order_edges;
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);
    
    // 2. construct an nxn matrix maxDist to store distances between clusters
    // maxDist
    
    // 3. For each edge e in the sorted list (shortest first)
    while (!E.empty()) {
        // get shortest edge
        RedCapEdge* edge = E.back();
        E.pop_back();
        
        // If e connects two different clusters l, m, and e.length >= maxDist(l,m)
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, w, l, m) ) {
            continue;
        }
        if ( edge->length < getCLKDistance(l, m) ) {
            continue;
        }
       
        // (1) find shortest edge e' in E that connnect cluster l and m
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, w, l, m)) {
                if (tmp_e->length < edge->length)
                    edge = tmp_e;
            }
        }
        // (2) add e'to T ane merge m to l (l is now th new cluster)
        l = cm.UpdateByAdd(edge);
        bool b_all_node_covered = mstree->AddEdge(edge);
        
        // (3) for each cluster c that is not l
        //      update maxDist(c, l) in E that connects c and l
        updateCLKDistanceToCluster(l, E);
        
        
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 4 FullOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderSLKRedCap::FullOrderSLKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderSLKRedCap::~FullOrderSLKRedCap()
{
    
}

void FullOrderSLKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = order_edges;
    // create a full_order_deges
    vector<RedCapEdge*> FE;
    createFullOrderEdges(FE);
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);
    std::sort(FE.begin(), FE.begin(), RedCapEdgeLarger);
    
    // 2 Repeat until all nodes in MSTree
    int i = 0;
    bool b_all_node_covered = false;
    while (b_all_node_covered == false) {
        // e is the i-th edge in E
        RedCapEdge* edge = FE[i];

        // If e connects two different clusters l, m, and C(l,m) = contiguity
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, w, l, m) ) {
            i += 1;
            continue;
        }

        // (1) find shortest edge e' in E that connnect cluster l and m
        RedCapEdge* e = NULL;
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, w, l, m)) {
                if (tmp_e->length < e->length)
                    e = tmp_e;
            }
        }
        
        // (2) add e'to T
        bool b_all_node_covered = mstree->AddEdge(edge);
        
        // (3) for each existing cluster c, c <> l , c <> m
        // Set C(c,l) = 1 if C(c,l) == 1 or C(c,m) == 1
        
        // (4) Merge cluster m to cluster l (l is now th new cluster)
        l = cm.UpdateByAdd(edge);

        // (5) reset i = 0
        i = 0;
        
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
    
    // no need to clean up full_order_edges
}

////////////////////////////////////////////////////////////////////////////////
//
// 5 FullOrderALKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderALKRedCap::FullOrderALKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs,  GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderALKRedCap::~FullOrderALKRedCap()
{
    
}

void FullOrderALKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = order_edges;

    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);

    // 2 Repeat until all nodes in MSTree
    int i = 0;
    bool b_all_node_covered = false;
    while (b_all_node_covered == false) {
        // e is the i-th edge in E
        RedCapEdge* edge = E[i];
        
        // If e connects two different clusters l, m, and C(l,m) = contiguity
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, w, l, m) ) {
            i += 1;
            continue;
        }
        
        // (1) find shortest edge e' in E that connnect cluster l and m
        RedCapEdge* e = NULL;
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, w, l, m)) {
                if (tmp_e->length < e->length)
                    e = tmp_e;
            }
        }
        
        // (2) add e'to T
        bool b_all_node_covered = mstree->AddEdge(edge);
        
        // (3) for each existing cluster c, c <> l
        //  update AvgDist(c, l)

        // (4) Merge cluster m to cluster l (l is now th new cluster)
        l = cm.UpdateByAdd(edge);
        
        // (5) reset i = 0
        i = 0;
        
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 6 FullOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderCLKRedCap::FullOrderCLKRedCap(const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderCLKRedCap::~FullOrderCLKRedCap()
{
    
}

double FullOrderCLKRedCap::getCLKDistance(RedCapCluster* l, RedCapCluster* m)
{
    if (maxDist.find(make_pair(l, m)) == maxDist.end())
        return 0;
    return maxDist[make_pair(l, m)];
}

bool FullOrderCLKRedCap::updateCLKDistanceToCluster(RedCapCluster* l, vector<RedCapEdge*>& E)
{
    bool dist_changed = false;
    unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
    
    for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
        RedCapCluster* c = it->first;
        if (c != l) {
            double max_length=0;
            bool is_connect = cm.GetMaxEdgeLength(c, l, &max_length, fo_edge_dict);
            if (is_connect == false)
                continue;
            maxDist[make_pair(c, l)] = max_length;
            maxDist[make_pair(l, c)] = max_length;
            // update the legnth of edge(c,l) in E with maxDist(c, l)
            cm.UpdateEdgeLength(c, l, max_length, E);
            dist_changed = true;
        }
    }
    return dist_changed;
}

void FullOrderCLKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = order_edges;
    // create a full_order_deges
    vector<RedCapEdge*> FE;
    createFullOrderEdges(FE);
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);
    std::sort(FE.begin(), FE.begin(), RedCapEdgeLarger);
    // maxDist(u, v) = 0
    
    // 2 Repeat until all nodes in MSTree
    int i = 0;
    bool b_all_node_covered = false;
    while (b_all_node_covered == false) {
        // e is the i-th edge in E
        RedCapEdge* edge = FE[i];
        
        // If e connects two different clusters l, m, and C(l,m) = contiguity
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if ( cm.CheckConnectivity(edge, w, l, m) && edge->length >= getCLKDistance(l,m) )
        {
            // (1) find shortest edge e' in E that connnect cluster l and m
            RedCapEdge* e = NULL;
            for (int j=0; j<E.size(); j++) {
                RedCapEdge* tmp_e = E[j];
                if (cm.CheckConnectivity(tmp_e, w, l, m)) {
                    if (tmp_e->length < e->length)
                        e = tmp_e;
                }
            }
            
            // (2) add e'to T and Merge cluster m to cluster l
            bool b_all_node_covered = mstree->AddEdge(e);
            l = cm.UpdateByAdd(e);
            
            // (3) for each existing cluster c, c <> l , c <> m
            // Set maxDist(c, l) = max( maxDist(c, l), maxDist(c, m) )
        }
        
        i += 1;
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}
