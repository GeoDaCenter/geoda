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
#include <stack>

#include <wx/stopwatch.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>

#include "../ShapeOperations/GalWeight.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "cluster.h"
#include "redcap.h"

using namespace std;
using namespace boost;
using namespace SpanningTreeClustering;

bool EdgeLess(Edge* a, Edge* b)
{
    if (a->length < b->length) {
        return true;
    } else if (a->length > b->length ) {
        return false;
    } else if (a->orig->id < b->orig->id) {
        return true;
    } else if (a->orig->id > b->orig->id) {
        return false;
    } else if (a->dest->id < b->dest->id) {
        return true;
    } else if (a->dest->id > b->dest->id) {
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
//
// SSDUtils
//
/////////////////////////////////////////////////////////////////////////
void SSDUtils::MeasureSplit(double ssd, vector<int> &ids, int split_position,  Measure& result)
{
    int start1 = 0;
    int end1 = split_position;
    int start2 = split_position;
    int end2 = (int)ids.size();
    
    double ssd1 = ComputeSSD(ids, start1, end1);
    double ssd2 = ComputeSSD(ids, start2, end2);

    
    result.measure_reduction = ssd - ssd1 - ssd2;
    result.ssd = ssd;
    result.ssd_part1 = ssd1;
    result.ssd_part2 = ssd2;
}

double SSDUtils::ComputeSSD(vector<int> &visited_ids, int start, int end)
{
    int size = end - start;
    double sum_squared = 0.0;
    double val;
    for (int i = 0; i < col; ++i) {
        double sqsum = 0.0;
        double sum = 0.0;
        for (int j = start; j < end; ++j) {
            val = raw_data[visited_ids[j]][i];
            sum += val;
            sqsum += val * val;
        }
        double mean = sum / size;
        sum_squared += sqsum -  size * mean * mean;
    }
    return sum_squared / col;
}

/////////////////////////////////////////////////////////////////////////
//
// Node
//
/////////////////////////////////////////////////////////////////////////
Node::Node(int _id)
{
    id = _id;
}

DisjoinSet::DisjoinSet()
{
    
}

DisjoinSet::DisjoinSet(int id)
{
    MakeSet(id);
}

Node* DisjoinSet::FindSet(Node* node)
{
    Node* parent = node->parent;
    if (parent == node) {
        return parent;
    }
    node->parent = FindSet(node->parent);
    return node->parent;
}

Node* DisjoinSet::MakeSet(int id)
{
    Node* node = new Node(id);
    node->parent = node;
    node->rank = 0;
    map[id] = node;
    
    return node;
}

void DisjoinSet::Union(Node* n1, Node* n2)
{
    Node* parent1 = FindSet(n1);
    Node* parent2 = FindSet(n2);
    
    if (parent1 == parent2) {
        return;
    }
    
    if (parent1->rank >= parent2->rank) {
        parent1->rank = (parent1->rank == parent2->rank) ? parent1->rank +1 : parent1->rank;
        parent2->parent = parent1;
    } else {
        parent1->parent = parent2;
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Edge
//
/////////////////////////////////////////////////////////////////////////
Edge::Edge(Node* a, Node* b, double _length)
{
    orig = a;
    dest = b;
    length = _length;
}


/////////////////////////////////////////////////////////////////////////
//
// Tree
//
/////////////////////////////////////////////////////////////////////////
Tree::Tree(vector<int> _ordered_ids, vector<Edge*> _edges, AbstractClusterFactory* _cluster)
: ordered_ids(_ordered_ids), edges(_edges), cluster(_cluster)
{
    ssd_reduce = 0;
    split_pos = 0;
    ssd_utils = cluster->ssd_utils;
    controls = cluster->controls;
    control_thres = cluster->control_thres;
    
    int size = (int)ordered_ids.size();
    int edge_size = (int)edges.size();
    this->ssd = 0;
    this->ssd_reduce = 0;
    
    if (ordered_ids.size() > 1) {
        this->ssd = ssd_utils->ComputeSSD(ordered_ids, 0, size);
        max_id = -1;
        for (int i=0; i<size; i++) {
            if (ordered_ids[i] > max_id) {
                max_id = ordered_ids[i];
            }
        }
        
        // use edges and ordered_ids to create nbr_dict and od_array
        boost::unordered_map<int, vector<int> > nbr_dict;
        od_array.resize(edge_size);
        
        int o_id, d_id;
        for (int i=0; i<edge_size; i++) {
            o_id = edges[i]->orig->id;
            d_id = edges[i]->dest->id;
            
            od_array[i].first = o_id;
            od_array[i].second = d_id;
            
            nbr_dict[o_id].push_back(d_id);
            nbr_dict[d_id].push_back(o_id);
        }
        
        if (size < 1000) {
            Partition(0, (int)od_array.size()-1, ordered_ids, od_array, nbr_dict);
        } else {
            run_threads(ordered_ids, od_array, nbr_dict);
        }
        if (!split_cands.empty()) {
            SplitSolution& ss = split_cands[0];
            this->split_ids = ss.split_ids;
            this->split_pos = ss.split_pos;
            this->ssd = ss.ssd;
            this->ssd_reduce = ss.ssd_reduce;
            
            for (int j=1; j<split_cands.size(); j++) {
                SplitSolution& tmp_ss = split_cands[j];
                if (tmp_ss.ssd_reduce > this->ssd_reduce) {
                    this->split_ids = tmp_ss.split_ids;
                    this->split_pos = tmp_ss.split_pos;
                    this->ssd = tmp_ss.ssd;
                    this->ssd_reduce = tmp_ss.ssd_reduce;
                }
            }
        }
    }
}

Tree::~Tree()
{
}

void Tree::run_threads(vector<int>& ids,
                       vector<pair<int, int> >& od_array,
                       boost::unordered_map<int, vector<int> >& nbr_dict)
{
    int n_jobs = (int)od_array.size();
    
    int nCPUs = boost::thread::hardware_concurrency();;
    int quotient = n_jobs / nCPUs;
    int remainder = n_jobs % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
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
        
        //prunecost(tree, a, b, scores, candidates);
        boost::thread* worker = new boost::thread(boost::bind(&Tree::Partition, this, a, b, boost::ref(ids), boost::ref(od_array), boost::ref(nbr_dict)));
        threadPool.add_thread(worker);
    }
    
    threadPool.join_all();
}

void Tree::Partition(int start, int end, vector<int>& ids,
                           vector<pair<int, int> >& od_array,
                           boost::unordered_map<int, vector<int> >& nbr_dict)
{
    int size = (int)nbr_dict.size();
    int orig_id, dest_id;
    int i;
    
    //int best_edge = -1;
    int evaluated = 0;
    int best_pos = -1;
    double tmp_ssd_reduce = 0, tmp_ssd=0;
    
    vector<int> visited_ids(size), best_ids(size);
    
    // cut edge one by one
    for ( i=start; i<=end; i++) {
        orig_id = od_array[i].first;
        dest_id = od_array[i].second;
        
        int idx = 0;
        vector<int> cand_ids(max_id+1, -1);
        Split(orig_id, dest_id, nbr_dict, cand_ids);
        
        for (int j=0; j<ids.size(); j++) {
            if (cand_ids[ ids[j] ] == 1) {
                visited_ids[idx] = ids[j];
                idx++;
            }
        }
        
        int tmp_split_pos = idx;
        
        if (checkControl(cand_ids, ids, 1)) {
            evaluated++;
            for (int j=0; j<ids.size(); j++) {
                if (cand_ids[ ids[j] ] == -1) {
                    visited_ids[idx] = ids[j];
                    idx++;
                }
            }
            if (checkControl(cand_ids, ids, -1)) {
                Measure result;
                ssd_utils->MeasureSplit(ssd, visited_ids, tmp_split_pos, result);
                if (result.measure_reduction > tmp_ssd_reduce) {
                    tmp_ssd_reduce = result.measure_reduction;
                    tmp_ssd = result.ssd;
                    best_pos = tmp_split_pos;
                    best_ids = visited_ids;
                }
            }
        }
    }
    
    if (split_pos != -1) {
        SplitSolution ss;
        ss.split_pos =  best_pos;
        ss.split_ids = best_ids;
        ss.ssd = tmp_ssd;
        ss.ssd_reduce = tmp_ssd_reduce;
        mutex.lock();
        split_cands.push_back(ss);
        mutex.unlock();
    }
}

void Tree::Split(int orig, int dest, boost::unordered_map<int, vector<int> >& nbr_dict, vector<int>& cand_ids)
{
    stack<int> visited_ids;
    int cur_id, i, nbr_size, nbr;
    
    visited_ids.push(orig);
    while (!visited_ids.empty()) {
        cur_id = visited_ids.top();
        visited_ids.pop();
        cand_ids[cur_id] = 1;
        vector<int>& nbrs = nbr_dict[cur_id];
        nbr_size = (int)nbrs.size();
        for (i=0; i<nbr_size; i++) {
            nbr = nbrs[i];
            if (nbr != dest && cand_ids[nbr] == -1) {
                visited_ids.push(nbr);
            }
        }
    }
}

bool Tree::checkControl(vector<int>& cand_ids, vector<int>& ids, int flag)
{
    if (controls == NULL) {
        return true;
    }
    
    double val = 0;
    for (int j=0; j<ids.size(); j++) {
        if (cand_ids[ ids[j] ] == flag) {
            val += controls[ ids[j] ];
        }
    }
    
    return val >= control_thres;
}

pair<Tree*, Tree*> Tree::GetSubTrees()
{
    if (split_ids.empty()) {
        return this->subtrees;
    }
    int size = (int)this->split_ids.size();
    vector<int> part1_ids(this->split_pos);
    vector<int> part2_ids(size -this->split_pos);
    
    int max_id = -1;
    for (int i=0; i<size; i++) {
        if (i <split_pos) {
            part1_ids[i] = split_ids[i];
        } else {
            part2_ids[i-split_pos] = split_ids[i];
        }
        if (split_ids[i] > max_id) {
            max_id = split_ids[i];
        }
    }
    
    vector<Edge*> part1_edges(part1_ids.size()-1);
    vector<Edge*> part2_edges(part2_ids.size()-1);
    
    vector<int> part_index(max_id+1, 0);
    for (int i=0; i< part1_ids.size(); i++) {
        part_index[ part1_ids[i] ] = -1;
    }
    for (int i=0; i< part2_ids.size(); i++) {
        part_index[ part2_ids[i] ] = 1;
    }
    int o_id, d_id;
    int cnt1=0, cnt2=0, cnt=0;
    for (int i=0; i<this->edges.size(); i++) {
        o_id = this->edges[i]->orig->id;
        d_id = this->edges[i]->dest->id;
        
        if (part_index[o_id] == -1 && part_index[d_id] == -1) {
            part1_edges[cnt1++] = this->edges[i];
        } else if (part_index[o_id] == 1 && part_index[d_id] == 1) {
            part2_edges[cnt2++] =  this->edges[i];
        } else {
            cnt++;
        }
    }

    
    Tree* left_tree = new Tree(part1_ids, part1_edges, cluster);
    Tree* right_tree = new Tree(part2_ids, part2_edges, cluster);
    subtrees.first = left_tree;
    subtrees.second = right_tree;
    return subtrees;
}

////////////////////////////////////////////////////////////////////////////////
//
// AbstractClusterFactory
//
////////////////////////////////////////////////////////////////////////////////
AbstractClusterFactory::AbstractClusterFactory(int row, int col,  double** _distances, double** _data, const vector<bool>& _undefs, GalElement * _w)
: rows(row), cols(col), dist_matrix(_distances), raw_data(_data), undefs(_undefs), w(_w)
{
}

AbstractClusterFactory::~AbstractClusterFactory()
{
    if (ssd_utils) {
        delete ssd_utils;
    }

    for (int i=0; i<edges.size(); i++) {
        delete edges[i];
    }
    for (int i=0; i<nodes.size(); i++) {
        delete nodes[i];
    }
}

void AbstractClusterFactory::init()
{
    ssd_utils = new SSDUtils(raw_data, rows, cols);
    
    // create nodes and edges
    nodes.resize(rows);
    for (int i=0; i<rows; i++) {
        Node* node = djset.MakeSet(i);
        nodes[i] = node;
    }
    
    this->dist_dict.resize(rows);
    
    Node* orig;
    Node* dest;
    double length;
    boost::unordered_map<pair<int, int>, bool> access_dict;
    
    for (int i=0; i<rows; i++) {
        orig = nodes[i];
        const vector<long>& nbrs = w[i].GetNbrs();
        for (int j=0; j<w[i].Size(); j++) {
            int nbr = (int)nbrs[j];
            dest = nodes[nbr];
            length = dist_matrix[orig->id][dest->id];
            
            if (access_dict.find(make_pair(i, nbr)) == access_dict.end()) {
                edges.push_back(new Edge(orig, dest, length));
                access_dict[make_pair(i, nbr)] = true;
                access_dict[make_pair(nbr, i)] = true;
            }
            this->dist_dict[i][nbr] = length;
        }
    }
    
    Clustering();
    
}

vector<vector<int> >& AbstractClusterFactory::GetRegions()
{
    return cluster_ids;
}

void AbstractClusterFactory::Partitioning(int k)
{
    wxStopWatch sw;
    
    vector<Tree*> not_split_trees;
    Tree* current_tree = new Tree(ordered_ids, ordered_edges, this);
    PriorityQueue sub_trees;
    sub_trees.push(current_tree);

    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        Tree* tmp_tree = sub_trees.top();
        //cout << tmp_tree->ssd_reduce << endl;
        sub_trees.pop();
        
        if (tmp_tree->ssd == 0) {
            not_split_trees.push_back(tmp_tree);
            k = k -1;
            continue;
        }
        
        pair<Tree*, Tree*> children = tmp_tree->GetSubTrees();
       
        Tree* left_tree = children.first;
        Tree* right_tree = children.second;
    
        if (left_tree== NULL && right_tree ==NULL) {
            not_split_trees.push_back(tmp_tree);
            k = k -1;
            continue;
        }
        
        if (left_tree) {
            sub_trees.push(left_tree);
        }
        if (right_tree) {
            sub_trees.push(right_tree);
        }
        
        //delete tmp_tree;
    }
  
    cluster_ids.clear();
    
    for (int i = 0; i< not_split_trees.size(); i++) {
        sub_trees.push(not_split_trees[i]);
    }
    
    PriorityQueue::iterator begin = sub_trees.begin();
    PriorityQueue::iterator end = sub_trees.end();
    boost::unordered_map<int, bool>::iterator node_it;
   
    for (PriorityQueue::iterator it = begin; it != end; ++it) {
        Tree* tmp_tree = *it;
        cluster_ids.push_back(tmp_tree->ordered_ids);
    }
    
    for (int i = 0; i< not_split_trees.size(); i++) {
        delete not_split_trees[i];
    }
    wxString time = wxString::Format("The long running function took %ldms to execute", sw.Time());
}

////////////////////////////////////////////////////////////////////////////////
//
// Skater
//
////////////////////////////////////////////////////////////////////////////////
Skater::Skater(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
: AbstractClusterFactory(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

Skater::~Skater()
{
    
}

void Skater::Clustering()
{
    Graph g(rows);
    boost::unordered_map<pair<int, int>, bool> access_dict;
    for (int i=0; i<rows; i++) {
        for (int j=0; j<w[i].Size(); j++) {
            if (access_dict.find(make_pair(i, w[i][j])) == access_dict.end()) {
                boost::add_edge(i, w[i][j], dist_matrix[i][ w[i][j] ], g);
                access_dict[make_pair(i, w[i][j])] = true;
                access_dict[make_pair(w[i][j], i)] = true;
            }
        }
    }
    
    boost::unordered_map<int, bool> id_dict;
    boost::unordered_map<pair<int, int>, Edge*> edge_dict;
    for (int i=0; i<edges.size(); i++) {
        Edge* e = edges[i];
        edge_dict[make_pair(e->orig->id, e->dest->id)] = e;
        edge_dict[make_pair(e->dest->id, e->orig->id)] = e;
    }
    
    //https://github.com/vinecopulib/vinecopulib/issues/22
    std::vector<int> p(num_vertices(g));
    boost::prim_minimum_spanning_tree(g, p.data());
  
    double sum_length=0;
    
    for (int source = 0; source < p.size(); ++source) {
        int target = p[source];
        if (source != target) {
            //boost::add_edge(source, p[source], mst);
            //ordered_edges.push_back(new Edge(source, p[source]));
            pair<int,int> o_d(source, target);
            pair<int,int> d_o(target, source);
            if (edge_dict.find(o_d) != edge_dict.end() &&
                edge_dict.find(d_o) != edge_dict.end())
            {
                sum_length += edge_dict[ o_d]->length;
                ordered_edges.push_back( edge_dict[ o_d]);
            }
        }
    }
    
    //cout << "Skater mst sum length:" << sum_length << endl;
    
    for (int i=0; i<ordered_edges.size(); i++) {
        int source = ordered_edges[i]->orig->id;
        int target = ordered_edges[i]->dest->id;
        
        if (id_dict.find(source)==id_dict.end()) {
            ordered_ids.push_back(source);
            id_dict[source] = true;
        }
        if (id_dict.find(target)==id_dict.end()) {
            ordered_ids.push_back(target);
            id_dict[target] = true;
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderSLKRedCap::FirstOrderSLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
: AbstractClusterFactory(rows, cols, _distances, _data, _undefs, w)
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
    std::sort(edges.begin(), edges.end(), EdgeLess);
    
    int num_nodes = (int)nodes.size();

    ordered_edges.resize(num_nodes-1);
    int cnt = 0;
    //double sum_length = 0;
    
    for (int i=0; i<edges.size(); i++) {
        Edge* edge = edges[i];
        
        Node* orig = edge->orig;
        Node* dest = edge->dest;
        
        Node* root1 = djset.FindSet(orig);
        Node* root2 = djset.FindSet(dest);
        
        if (root1 == root2) {
            continue;
        } else {
            ordered_edges[cnt] = edge;
            djset.Union(orig, dest);
        }
        if ( ++cnt == num_nodes - 1) {
            break;
        }
    }
    //cout << "FO-SLK mst sum length:" << sum_length << endl;
    
    boost::unordered_map<int, bool> id_dict;
    for (int i=0; i<ordered_edges.size(); i++) {
        Edge* edge = ordered_edges[i];
        Node* orig = edge->orig;
        Node* dest = edge->dest;
        
        if (id_dict.find(orig->id)==id_dict.end()) {
            ordered_ids.push_back(orig->id);
            id_dict[orig->id] = true;
        }
        if (id_dict.find(dest->id)==id_dict.end()) {
            ordered_ids.push_back(dest->id);
            id_dict[dest->id] = true;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
// The First-Order-ALK method also starts with the spatially contiguous graph G*. However, after each merge, the distance between the new cluster and every other cluster is recalculated. Therefore, edges that connect the new cluster and every other cluster are updated with new length values. Edges in G* are then re-sorted and re-evaluated from the beginning. The procedure stops when all objects are in one cluster. The algorithm is shown in figure 3. The complexity is O(n2log n) due to the sorting after each merge.
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderALKRedCap::FirstOrderALKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractClusterFactory(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderALKRedCap::~FirstOrderALKRedCap()
{
    
}


void FirstOrderALKRedCap::Clustering()
{
    
}

////////////////////////////////////////////////////////////////////////////////
//
// 3 FirstOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderCLKRedCap::FirstOrderCLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractClusterFactory(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderCLKRedCap::~FirstOrderCLKRedCap()
{
    
}

void FirstOrderCLKRedCap::Clustering()
{
    
}

////////////////////////////////////////////////////////////////////////////////
//
// 4 FullOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderSLKRedCap::FullOrderSLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: FullOrderALKRedCap(rows, cols, _distances, _data, _undefs, w, _controls, _control_thres, false)
{
    init();
}

FullOrderSLKRedCap::~FullOrderSLKRedCap()
{
    
}

double FullOrderSLKRedCap::UpdateClusterDist(int cur_id, int o_id, int d_id, bool conn_c_o, bool conn_c_d, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum)
{
    double new_dist = 0;
    if (conn_c_o && conn_c_d) {
        double d_c_o = dist_dict[cur_id][o_id];
        double d_c_d = dist_dict[cur_id][d_id];
        if ((new_dist = d_c_o) > d_c_d) {
            new_dist = d_c_d;
        }
        
    } else if (conn_c_o || conn_c_d) {
        if (conn_c_d) {
            int tmp_id = o_id;
            o_id = d_id;
            d_id = tmp_id;
        }
        new_dist = dist_dict[cur_id][o_id];
        int c_endpos = clst_startpos[cur_id] + clst_nodenum[cur_id];
        int d_endpos = clst_startpos[d_id] + clst_nodenum[d_id];
        for (int i=clst_startpos[cur_id]; i<c_endpos; i++) {
            for (int j=clst_startpos[d_id]; j<d_endpos; j++) {
                if (dist_dict[clst_ids[i]] [clst_ids[j]] < new_dist) {
                    new_dist = dist_dict[clst_ids[i]] [clst_ids[j]];
                }
            }
        }
    }
    return new_dist;
}

////////////////////////////////////////////////////////////////////////////////
//
// 5 FullOrderALKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderALKRedCap::FullOrderALKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs,  GalElement * w, double* _controls, double _control_thres, bool init_flag)
: AbstractClusterFactory(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    if (init_flag) {
        init();
    }
}

FullOrderALKRedCap::~FullOrderALKRedCap()
{
    
}

void FullOrderALKRedCap::Clustering()
{
    int num_nodes = (int)nodes.size();
    vector<Node*> ordered_nodes(num_nodes);
    
    for (int i=0; i< this->edges.size(); i++) {
        Edge* edge = this->edges[i];
        Node* orig = edge->orig;
        Node* dest = edge->dest;
        ordered_nodes[ orig->id ] = orig;
        ordered_nodes[ dest->id ] = dest;
    }

    std::sort(edges.begin(), edges.end(), EdgeLess);
    int num_edges = (int)edges.size();
    vector<Edge*> edges_copy(num_edges);
    for (int i=0; i<num_edges; i++) {
        edges_copy[i] = edges[i];
    }
    
    //cout << "# edges:" << num_edges << endl;
    
    this->ordered_edges.resize(num_nodes-1);
    
    vector<int> ids(num_nodes);
    // number of nodes in a cluster
    // the start position of a cluster
    // the cluster id of each nodes
    vector<int> cluster_nodenum(num_nodes);
    vector<int> cluster_ids(num_nodes);
    vector<int> cluster_startpos(num_nodes);
    for (int i=0; i<num_nodes; ++i) {
        ids[i] = i;
        cluster_ids[i] = i;
        cluster_startpos[i] = i;
        cluster_nodenum[i] = 1;
    }
    
    int index = 0;
    int cnt = 0;
    
    vector<bool> access_flag(num_nodes, false);
    vector<int> counts(num_nodes);
    vector<Edge*> new_edges;
    
    Edge* cur_edge = edges_copy[0];
    for (int k=0; k<num_edges; k++) {
        ++cnt;
        Node* orig = cur_edge->orig;
        Node* dest = cur_edge->dest;
        int orig_id = ids[orig->id];
        int dest_id = ids[dest->id];
        
        Node* root1 = djset.FindSet(orig);
        Node* root2 = djset.FindSet(dest);
        
        if (root1 != root2) {
            // find the shortest e in edges
            for (int i=0; i<this->edges.size(); i++) {
                ++cnt;
                Edge* tmp_edge = this->edges[i];
                int tmp_o_id = ids[tmp_edge->orig->id];
                int tmp_d_id = ids[tmp_edge->dest->id];
                if ((tmp_o_id==orig_id && tmp_d_id == dest_id) ||
                    (tmp_d_id==orig_id && tmp_o_id == dest_id))
                {
                    // add edge
                    this->ordered_edges[index++] = tmp_edge;
                    break;
                }
            }
            // add e to cluster
            djset.Union(orig, dest);
            
            if (index == num_nodes -1) {
                break;
            }
            
            // remove edges(c,l) and edges(c, m) from copy
            int i=0;
            while (i < num_edges) {
                ++cnt;
                int tmp_o_id = ids[edges_copy[i]->orig->id];
                int tmp_d_id = ids[edges_copy[i]->dest->id];
                if (tmp_o_id==orig_id || tmp_o_id==dest_id ||
                    tmp_d_id==orig_id || tmp_d_id==dest_id)
                {
                    edges_copy[i] = edges_copy[--num_edges];
                } else {
                    ++i;
                }
            }
            
            for (i=0; i<num_nodes; i++) {
                access_flag[i] = false;
            }
            
            // update distance to (o,d) cluster
            for (i=0; i<num_nodes; ++i) {
                ++cnt;
                int tmp_id = ids[i];
                if (!access_flag[tmp_id] && tmp_id != dest_id && tmp_id != orig_id) {
                    bool d_is_nbr = dist_dict[tmp_id].find(dest_id) != dist_dict[tmp_id].end();
                    bool o_is_nbr = dist_dict[tmp_id].find(orig_id) != dist_dict[tmp_id].end();
                    if (d_is_nbr || o_is_nbr) {
                        double update_dist = UpdateClusterDist(tmp_id, orig_id, dest_id, o_is_nbr, d_is_nbr, cluster_ids, cluster_startpos, cluster_nodenum);
                        
                        Edge* new_e = new Edge(ordered_nodes[tmp_id], ordered_nodes[orig_id], update_dist);
                        
                        edges_copy[num_edges++] = new_e;
                        new_edges.push_back(new_e);
                        
                        dist_dict[tmp_id].erase(dest_id);
                        dist_dict[dest_id].erase(tmp_id);
                        dist_dict[tmp_id][orig_id] = update_dist;
                        dist_dict[orig_id][tmp_id] = update_dist;
                    }
                    access_flag[tmp_id] = true;
                } else if (tmp_id == dest_id || tmp_id == orig_id) {
                    ids[i] = orig_id;
                 }
            }
            
            cluster_nodenum[orig_id] += cluster_nodenum[dest_id];
            cluster_nodenum[dest_id] = 0;
            
            cluster_startpos[0] = 0;
            counts[0] = 0;
            for (i=1; i<num_nodes; ++i) {
                cluster_startpos[i] = cluster_startpos[i-1] + cluster_nodenum[i-1];
                counts[i] = 0;
             }
            
            for (i=0; i<num_nodes; i++) {
                int j = ids[i];
                cluster_ids[cluster_startpos[j] + counts[j]] = i;
                ++counts[j];
            }
            
            
            cur_edge = GetShortestEdge(edges_copy, 0, num_edges);
            k = -1;
        }
    }
    
    //cout << "cnt: " << cnt << endl;
    
    boost::unordered_map<int, bool> id_dict;

    for (int i=0; i<ordered_edges.size();i++) {
        Edge* e = ordered_edges[i];
        Node* orig = e->orig;
        Node* dest = e->dest;
        
        if (id_dict.find(orig->id)==id_dict.end()) {
            ordered_ids.push_back(orig->id);
            id_dict[orig->id] = true;
        }
        if (id_dict.find(dest->id)==id_dict.end()) {
            ordered_ids.push_back(dest->id);
            id_dict[dest->id] = true;
        }
    }
    
    for (int i=0; i<new_edges.size(); i++) {
        delete new_edges[i];
    }
}

double FullOrderALKRedCap::UpdateClusterDist(int cur_id, int o_id, int d_id, bool conn_c_o, bool conn_c_d, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum)
{
    double new_dist = 0;
    if (conn_c_o && conn_c_d) {
        double d_c_o = dist_dict[cur_id][o_id];
        double d_c_d = dist_dict[cur_id][d_id];
        
        // (avg_d(c,o) * numEdges(c,o)  + avg_d(c,d)*numEdges(c,d)) /
        // (numEdges(c, o) + numEdges(c, d))
        new_dist = (d_c_o * clst_nodenum[o_id] * clst_nodenum[cur_id] + d_c_d * clst_nodenum[d_id] * clst_nodenum[cur_id]) / ((clst_nodenum[o_id] + clst_nodenum[d_id]) * clst_nodenum[cur_id]);
        
        
    } else if (conn_c_o || conn_c_d) {
        if (conn_c_d) {
            int tmp_id = o_id;
            o_id = d_id;
            d_id = tmp_id;
        }
        double d_c_o = dist_dict[cur_id][o_id];
        double sumval_c_d = 0;
        int c_endpos = clst_startpos[cur_id] + clst_nodenum[cur_id];
        int d_endpos = clst_startpos[d_id] + clst_nodenum[d_id];
        
        for (int i=clst_startpos[cur_id]; i<c_endpos; i++) {
            for (int j=clst_startpos[d_id]; j<d_endpos; j++) {
                sumval_c_d += dist_matrix[clst_ids[i]] [clst_ids[j]];
            }
        }
        
        new_dist = (d_c_o * clst_nodenum[o_id] * clst_nodenum[cur_id] + sumval_c_d) / ((clst_nodenum[o_id] + clst_nodenum[d_id]) * clst_nodenum[cur_id]);
        
    }
    return new_dist;
}

Edge* FullOrderALKRedCap::GetShortestEdge(vector<Edge*>& _edges, int start, int end)
{
    double len = DBL_MAX;
    Edge* short_e = NULL;
    for (int i=start; i<end; i++) {
        if (_edges[i]->length < len) {
            len = _edges[i]->length;
            short_e = _edges[i];
        }
    }
    return short_e;
}
////////////////////////////////////////////////////////////////////////////////
//
// 6 FullOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderCLKRedCap::FullOrderCLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: FullOrderALKRedCap(rows, cols, _distances, _data, _undefs, w, _controls, _control_thres, false)
{
    init();
}

FullOrderCLKRedCap::~FullOrderCLKRedCap()
{
    
}

double FullOrderCLKRedCap::UpdateClusterDist(int cur_id, int o_id, int d_id, bool conn_c_o, bool conn_c_d, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum)
{
    double new_dist = 0.0;
    if (conn_c_o && conn_c_d) {
        double d_c_o = dist_dict[cur_id][o_id];
        double d_c_d = dist_dict[cur_id][d_id];
        if ((new_dist = d_c_o) < d_c_d) {
            new_dist = d_c_d;
        }
        
    } else if (conn_c_o || conn_c_d) {
        if (conn_c_d) {
            int tmp_id = o_id;
            o_id = d_id;
            d_id = tmp_id;
        }
        new_dist = dist_dict[cur_id][o_id];
        int c_endpos = clst_startpos[cur_id] + clst_nodenum[cur_id];
        int d_endpos = clst_startpos[d_id] + clst_nodenum[d_id];
        for (int i=clst_startpos[cur_id]; i<c_endpos; i++) {
            for (int j=clst_startpos[d_id]; j<d_endpos; j++) {
                if (dist_dict[clst_ids[i]] [clst_ids[j]] > new_dist) {
                    new_dist = dist_dict[clst_ids[i]] [clst_ids[j]];
                }
            }
        }
    }
    return new_dist;
}

////////////////////////////////////////////////////////////////////////////////
FullOrderWardRedCap::FullOrderWardRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs,  GalElement * w, double* _controls, double _control_thres)
: AbstractClusterFactory(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderWardRedCap::~FullOrderWardRedCap()
{
    
}

void FullOrderWardRedCap::Clustering()
{
    int num_nodes = (int)nodes.size();
    vector<Node*> ordered_nodes(num_nodes);
    
    for (int i=0; i< this->edges.size(); i++) {
        Edge* edge = this->edges[i];
        Node* orig = edge->orig;
        Node* dest = edge->dest;
        ordered_nodes[ orig->id ] = orig;
        ordered_nodes[ dest->id ] = dest;
    }
    
    std::sort(edges.begin(), edges.end(), EdgeLess);
    int num_edges = (int)edges.size();
    vector<Edge*> edges_copy(num_edges);
    for (int i=0; i<num_edges; i++) {
        edges_copy[i] = edges[i];
    }
    
    //cout << "# edges:" << num_edges << endl;
    
    this->ordered_edges.resize(num_nodes-1);
}
