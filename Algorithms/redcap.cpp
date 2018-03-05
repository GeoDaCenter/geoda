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

typedef map<pair<SpanningTree*, SpanningTree*>, double> SubTrees;

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
    neighbors = node->neighbors;
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

void RedCapNode::RemoveNeighbor(int node_id)
{
    for (it=neighbors.begin(); it!=neighbors.end(); it++) {
        if ( (*it)->id == node_id ) {
            neighbors.erase(it);
            break;
        }
    }
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
// SpanningTree
//
/////////////////////////////////////////////////////////////////////////
SpanningTree::SpanningTree(const vector<RedCapNode*>& all_nodes, const vector<vector<double> >& _data, const vector<bool>& _undefs, double* _controls, double _control_thres)
: data(_data), undefs(_undefs), controls(_controls), control_thres(_control_thres)
{
    left_child = NULL;
    right_child = NULL;
    root = NULL;
    ssd = -1;
    
    for (int i=0; i<all_nodes.size(); i++) {
        node_dict[all_nodes[i]->id] = false;
    }
}

SpanningTree::SpanningTree(RedCapNode* node, RedCapNode* exclude_node, const vector<vector<double> >& _data, const vector<bool>& _undefs, double* _controls, double _control_thres)
: data(_data), undefs(_undefs),controls(_controls), control_thres(_control_thres)
{
    left_child = NULL;
    right_child = NULL;
    root = NULL;
    ssd = -1;
    
    // construct spanning tree from root
    unordered_map<int, bool> visited;
    list<RedCapNode*> stack;
    stack.push_back(node);
    while (!stack.empty()) {
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        node_dict[tmp->id] = false;
        
        RedCapNode* new_node = getNewNode(tmp);
        if (root==NULL) root = new_node;
        
        set<RedCapNode*>& nbrs = tmp->neighbors;
        set<RedCapNode*>::iterator it;
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            RedCapNode* nn = *it;
            if (nn->id == exclude_node->id)
                continue;
            
            if (visited.find(nn->id) == visited.end()) {
                stack.push_back(nn);
            }
            visited[nn->id] = true;
            
            // local copy node,
            RedCapNode* new_nn = getNewNode(nn);
            // local copy edges
            addNewEdge(new_node, new_nn);
            // add neighbor info
            new_node->AddNeighbor(new_nn);
        }
    }
}

SpanningTree::~SpanningTree()
{
    if (left_child)
        delete left_child;
    if (right_child)
        delete right_child;
    for (int i=0; i<new_edges.size(); i++) {
        delete new_edges[i];
    }
    unordered_map<int, RedCapNode*>::iterator it;
    for (it=new_nodes.begin(); it != new_nodes.end(); it++) {
        delete it->second;
    }
}

bool SpanningTree::checkEdge(RedCapEdge *edge)
{
    RedCapNode* a = edge->a;
    RedCapNode* b = edge->b;
    
    if ( edge_dict.find(make_pair(a->id, b->id)) != edge_dict.end() ||
         edge_dict.find(make_pair(b->id, a->id)) != edge_dict.end() )
        return true;
    return false;
}

void SpanningTree::addNewEdge(RedCapNode* a, RedCapNode* b)
{
    if ( edge_dict.find(make_pair(a->id, b->id)) == edge_dict.end() &&
        edge_dict.find(make_pair(b->id, a->id)) == edge_dict.end() )
    {
        RedCapEdge* e = new RedCapEdge(a, b);
        edge_dict[ make_pair(a->id, b->id)] = true;
        edges.push_back(e);
        new_edges.push_back(e); // clean memeory
    }
}

RedCapNode* SpanningTree::getNewNode(RedCapNode* old_node, bool copy_nbrs)
{
    if (new_nodes.find(old_node->id) == new_nodes.end()) {
        RedCapNode* new_node = new RedCapNode(old_node->id, old_node->value);
        if (copy_nbrs) {
            std::set<RedCapNode*>::iterator it;
            std::set<RedCapNode*>::iterator begin = old_node->neighbors.begin();
            std::set<RedCapNode*>::iterator end = old_node->neighbors.end();
            for (it= begin; it!= end; it++) {
                RedCapNode* nbr_node = getNewNode(*it);
                new_node->AddNeighbor(nbr_node);
            }
        }
        new_nodes[ old_node->id ] = new_node;
        return new_node;
    }
    return new_nodes[ old_node->id ];
}

void SpanningTree::AddEdge(RedCapEdge *e)
{
    if (!checkEdge(e)) {
        // add edge
        cout << e->a->id << "--" << e->b->id << endl;
        edge_dict[ make_pair(e->a->id, e->b->id) ] = true;
        
        RedCapNode* a = getNewNode(e->a);
        RedCapNode* b = getNewNode(e->b);

        node_dict[a->id] = true;
        node_dict[b->id] = true;
        
        if (root == NULL) {
            root = a;
        }
        
        // local nodes will reconstruct their neigbhors
        a->AddNeighbor(b);
        b->AddNeighbor(a);

        e->a = a; // replace with local node
        e->b = b;
        edges.push_back(e);
    }
}

bool SpanningTree::IsFullyCovered()
{
    // check if fully covered
    bool is_fully_covered = true;
    unordered_map<int, bool>::iterator it;
    for (it = node_dict.begin(); it != node_dict.end(); it++) {
        if (it->second == false) {
            is_fully_covered = false;
            break;
        }
    }
    return is_fully_covered;
}

void SpanningTree::Split()
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
        boost::thread* worker = new boost::thread(boost::bind(&SpanningTree::subSplit, this, a, b));
        threadPool.add_thread(worker);
    }
    threadPool.join_all();
    
    // merge results
    bool is_first = true;
    double best_hg;
    RedCapEdge* best_e;
    
    map<RedCapEdge*, double>::iterator it;
    for (it = cand_trees.begin(); it != cand_trees.end(); it++) {
        RedCapEdge* e = it->first;
        double hg = it->second;

        if (is_first || (best_hg >0 && best_hg < hg) ) {
            best_hg = hg;
            is_first = false;
            best_e = e;
        }
    }
    left_child = new SpanningTree(best_e->a, best_e->b, data, undefs, controls, control_thres);
    right_child = new SpanningTree(best_e->b, best_e->a, data, undefs, controls, control_thres);
}

bool SpanningTree::quickCheck(RedCapNode* node, RedCapNode* exclude_node)
{
    if (controls == NULL)
        return false;
    
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
            {
                stack.push_back(nn);
                visited[nn->id] = true;
            }
        }
    }
    
    return check_val >  control_thres;
}

set<int> SpanningTree::getSubTree(RedCapNode* a, RedCapNode* exclude_node)
{
    set<int> visited;
    list<RedCapNode*> stack;
    stack.push_back(a);
    while (!stack.empty()) {
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        visited.insert(tmp->id);
        set<RedCapNode*>& nbrs = tmp->neighbors;
        set<RedCapNode*>::iterator it;
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            RedCapNode* nn = *it;
            if (nn->id != exclude_node->id &&
                node_dict.find(nn->id) != node_dict.end() &&
                visited.find(nn->id) == visited.end())
            {
                stack.push_back(nn);
                visited.insert(nn->id);
            }
        }
    }
    return visited;
}

void SpanningTree::subSplit(int start, int end)
{
    // search best cut
    double hg = 0;
    RedCapEdge* best_e = NULL;
    SpanningTree* left_best=NULL;
    SpanningTree* right_best=NULL;
    
    bool is_first = true;
    //for (int i=0; i<edges.size(); i++) {
    for (int i=start; i<=end; i++) {
        //if (i >= edges.size()) continue;
        RedCapEdge* e = edges[i];
        // remove i-th edge, there should be two sub-trees created (left, right)
        set<int> left_ids = getSubTree(e->a, e->b);
        set<int> right_ids = getSubTree(e->b, e->a);

        double left_ssd = computeSSD(left_ids);
        double right_ssd = computeSSD(right_ids);

        if (left_ssd == 0 || right_ssd == 0) {
            // can't be split-ted: e.g. only one node, or doesn't check control
            continue;
        }
        
        double hg_sub = left_ssd + right_ssd;
        if (is_first || (hg > 0 && hg_sub < hg) ) {
            hg = hg_sub;
            best_e = e;
            is_first = false;
        }
    }
    if (best_e && hg > 0) {
        if ( cand_trees.find(best_e) == cand_trees.end() ) {
            cand_trees[best_e] = hg;
            
        } else {
            if ( cand_trees[best_e] > hg) {
                cand_trees[best_e] = hg;
            }
        }
    }
}

bool SpanningTree::checkControl(set<int>& ids)
{
    if (controls == NULL) return true;
   
    double val = 0;
    set<int>::iterator it;
    for (it=ids.begin(); it!=ids.end(); it++) {
        val += controls[*it];
    }
    
    return val > control_thres;
}

SpanningTree* SpanningTree::GetLeftChild()
{
    return left_child;
}

SpanningTree* SpanningTree::GetRightChild()
{
    return right_child;
}

double SpanningTree::computeSSD(set<int>& ids)
{
    if (ids.size() <= 1) {
        return 0;
    }
    
    // sum of squared deviations (ssd)
    if (ssd_dict.find(ids) != ssd_dict.end())
        return ssd_dict[ids];
    
    double sum_ssd = 0;
    if (checkControl(ids) == false)
        return 0; // doesn't satisfy control variable
   
    set<int>::iterator it;
    int n_vars = data[0].size();
    for (int i=0; i<n_vars; i++) {
        vector<double> tmp_data;
        int n =0;
        for (it=ids.begin(); it!=ids.end(); it++) {
            int id = *it;
            if (undefs[id]) continue;
            tmp_data.push_back(data[id][i]);
            n += 1;
        }
        double ssd = GenUtils::GetVariance(tmp_data);
        sum_ssd += ssd * n;
    }
   
    boost::mutex::scoped_lock scoped_lock(mutex);
    ssd_dict[ids]  = sum_ssd;
    
    return sum_ssd; // * n
}

double SpanningTree::GetSSD()
{
    if (ssd < 0) {
        set<int> ids;
        unordered_map<int, bool>::iterator it;
        for (it = node_dict.begin(); it != node_dict.end(); it++) {
            ids.insert(it->first);
        }
        ssd =  computeSSD(ids);
    }
    return ssd;
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
  
    // create first_order_edges
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) continue;
        const vector<long>& nbrs = w[i].GetNbrs();
        const vector<double>& nbrs_w = w[i].GetNbrWeights();
        for (int j=0; j<nbrs.size(); j++) {
            int nbr = nbrs[j];
            if (undefs[nbr] || i == nbr)
                continue;
            if (checkFirstOrderEdge(i, nbr) == false) {
                double w = nbrs_w[j];
                RedCapNode* a = all_nodes[i];
                RedCapNode* b = all_nodes[nbr];
                RedCapEdge* e = new RedCapEdge(a, b, w);
                first_order_edges.push_back(e);
                fo_edge_dict[make_pair(i,nbr)] = e->length;
            }
        }
    }
    
    // init SpanningTree
    mstree = new SpanningTree(all_nodes, data, undefs, controls, control_thres);
    
    Clustering();
}

bool AbstractRedcap::checkFirstOrderEdge(int node_i, int node_j)
{
    if (fo_edge_dict.find(make_pair(node_i, node_j)) != fo_edge_dict.end() ||
        fo_edge_dict.find(make_pair(node_j, node_i)) != fo_edge_dict.end() )
        return true;
    
    return false;
}

void AbstractRedcap::createFirstOrderEdges(vector<RedCapEdge*>& edges)
{
    
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

bool AbstractRedcap::CheckSpanningTree()
{
    return true;
}

vector<vector<int> >& AbstractRedcap::GetRegions()
{
    return cluster_ids;
}

bool RedCapTreeLess(SpanningTree* a, SpanningTree* b)
{
    //return a->all_nodes_dict.size() < b->all_nodes_dict.size();
    return a->ssd < b->ssd;
}

void AbstractRedcap::Partitioning(int k)
{
    PriorityQueue sub_trees;
    sub_trees.push(mstree);
   
    vector<SpanningTree*> not_split_trees;
    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        SpanningTree* tmp_tree = sub_trees.top();
        sub_trees.pop();
        tmp_tree->Split();
       
        SpanningTree* left_tree = tmp_tree->GetLeftChild();
        SpanningTree* right_tree = tmp_tree->GetRightChild();
    
        if (left_tree== NULL && right_tree ==NULL) {
            not_split_trees.push_back(tmp_tree);
            k = k -1;
            continue;
        }
        
        if (left_tree) {
            left_tree->GetSSD();
            sub_trees.push(left_tree);
        }
        if (right_tree) {
            right_tree->GetSSD();
            sub_trees.push(right_tree);
        }
    }
  
    cluster_ids.clear();
    
    for (int i = 0; i< not_split_trees.size(); i++)
        sub_trees.push(not_split_trees[i]);
    
    PriorityQueue::iterator begin = sub_trees.begin();
    PriorityQueue::iterator end = sub_trees.end();
    unordered_map<int, bool>::iterator node_it;
   
    for (PriorityQueue::iterator it = begin; it != end; ++it) {
        SpanningTree* _tree = *it;
        vector<int> cluster;
        for (node_it=_tree->node_dict.begin();
             node_it!=_tree->node_dict.end(); node_it++)
        {
            cluster.push_back(node_it->first);
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

bool RedCapCluster::IsConnectWith(RedCapCluster* c)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    for (it=node_dict.begin(); it!=node_dict.end(); it++) {
        if (c->Has(it->first))
            return true;
    }
    return false;
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
   
    if (l->size()==1 && m->size() == 1)
        return true;
    
    return !l->IsConnectWith(m);
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
    std::sort(first_order_edges.begin(), first_order_edges.end(), RedCapEdgeLess);
    
    // step 2.
    for (int i=0; i<first_order_edges.size(); i++) {
        RedCapEdge* edge = first_order_edges[i];
        
        // if an edge connects two different clusters, it is added to T,
        // and the two clusters are merged;
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, w, l, m) ) {
            continue;
        }
        cm.UpdateByAdd(edge);
        
        mstree->AddEdge(edge);
        bool b_all_node_covered = mstree->IsFullyCovered();
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
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    
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
        mstree->AddEdge(edge);
        bool b_all_node_covered = mstree->IsFullyCovered();
        
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
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    
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
        mstree->AddEdge(edge);
        bool b_all_node_covered = mstree->IsFullyCovered();
        
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
    // make a copy of first_first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
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
                if (tmp_e->length < tmp_e->length)
                    e = tmp_e;
            }
        }
        
        // (2) add e'to T
        mstree->AddEdge(edge);
        bool b_all_node_covered = mstree->IsFullyCovered();
        
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
    vector<RedCapEdge*> E = first_order_edges;

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
                if (tmp_e->length < tmp_e->length)
                    e = tmp_e;
            }
        }
        
        // (2) add e'to T
        mstree->AddEdge(edge);
        bool b_all_node_covered = mstree->IsFullyCovered();
        
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
    vector<RedCapEdge*> E = first_order_edges;
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
            mstree->AddEdge(edge);
            bool b_all_node_covered = mstree->IsFullyCovered();
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
