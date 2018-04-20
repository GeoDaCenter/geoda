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

#include <wx/textfile.h>
#include <wx/stopwatch.h>
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
// SSDUtils
//
/////////////////////////////////////////////////////////////////////////
void SSDUtils::MeasureSplit(double ssd, vector<int> &ids, int split_position,  Measure& result)
{
    int start1 = 0;
    int end1 = split_position;
    int start2 = split_position;
    int end2 = ids.size();
    
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
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////
RedCapNode::RedCapNode(int _id)
{
    id = _id;
    nbr_info.SetDefault(this);
}

void RedCapNode::SetCluster(RedCapCluster* cluster)
{
    container = cluster;
}

void RedCapNode::AddNeighbor(RedCapNode* nbr, RedCapEdge* e)
{
    nbr_info.AddNeighbor(nbr, e);
}
/////////////////////////////////////////////////////////////////////////
//
// RedCapEdge
//
/////////////////////////////////////////////////////////////////////////
RedCapEdge::RedCapEdge(RedCapNode* a, RedCapNode* b, double _length)
{
    orig = a;
    dest = b;
    length = _length;
}

bool RedCapEdgeLess(RedCapEdge* a, RedCapEdge* b)
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

bool RedCapEdgeLarger(RedCapEdge* a, RedCapEdge* b)
{
    return a->length > b->length;
}
/////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
/////////////////////////////////////////////////////////////////////////
RedCapCluster::RedCapCluster(RedCapNode* node)
{
    p1 = node;
    p2 = node;
    parent = NULL;
    child1 = NULL;
    child2 = NULL;
    
    node->SetCluster(this);
}

RedCapCluster::RedCapCluster(RedCapCluster* c1, RedCapCluster* c2, RedCapEdge* e, double** _dist_matrix)
{
    dist_matrix = _dist_matrix;
    c1->parent = this;
    c2->parent = this;
    child1 = c1;
    child2 = c2;
    parent = NULL;
    
    double d11 = dist_matrix[c1->p1->id][c2->p1->id];
    double d12 = dist_matrix[c1->p1->id][c2->p2->id];
    double d21 = dist_matrix[c1->p2->id][c2->p1->id];
    double d22 = dist_matrix[c1->p2->id][c2->p2->id];
    
    if ((d11 < d12) && (d11 < d21) && (d11 < d22))
    {
        p1 = c1->p2;
        p2 = c2->p2;
        c1->p1->AddNeighbor(c2->p1, e);
        c2->p1->AddNeighbor(c1->p1, e);
        
    } else if ((d12 < d21) && (d12 < d22)) {
        p1 = c1->p2;
        p2 = c2->p1;
        c1->p1->AddNeighbor(c2->p2, e);
        c2->p2->AddNeighbor(c1->p1, e);
        
    } else if (d21 < d22) {
        p1 = c1->p1;
        p2 = c2->p2;
        c1->p2->AddNeighbor(c2->p1, e);
        c2->p1->AddNeighbor(c1->p2, e);
        
    } else {
        p1 = c1->p1;
        p2 = c2->p1;
        c1->p2->AddNeighbor(c2->p2, e);
        c2->p2->AddNeighbor(c1->p2, e);
    }
}

RedCapCluster* RedCapCluster::GetRoot(RedCapNode* node)
{
    RedCapCluster* root = NULL;
    RedCapCluster* cur_cluster = node->container;
    while (cur_cluster != NULL) {
        root = cur_cluster;
        cur_cluster = cur_cluster->parent;
    }
    return root;
}

void RedCapCluster::GetOrderedNodes(RedCapCluster* root, vector<RedCapNode*>& ordered_nodes)
{
    RedCapNode* pre_node = root->p1;
    RedCapNode* cur_node = NULL;
    
    RedCapNode* n1 = pre_node->nbr_info.n1;
    RedCapNode* n2 = pre_node->nbr_info.n2;
    
    RedCapNode* next_node = NULL;
    
    if (n1 != NULL) {
        next_node = n1;
    } else if (n2 != NULL) {
        next_node = n2;
    } else {
        cout << "no neighbor point." << endl;
    }
    
    int count = 0;
    pre_node->container = NULL;
    
    ordered_nodes[0] = pre_node;
    RedCapEdge* e;
    
    while (next_node != NULL) {
        RedCapEdge* e1 = next_node->nbr_info.e1;
        RedCapEdge* e2 = next_node->nbr_info.e1;
        RedCapNode* n1 = next_node->nbr_info.n1;
        RedCapNode* n2 = next_node->nbr_info.n2;
        if (n1 == pre_node) {
            next_node = n2;
            e = e1;
        } else if (n2 == pre_node) {
            next_node = n1;
            e = e2;
        } else {
            cout << "wrong neibhro" << endl;
        }
        if (next_node->container == NULL) {
            cout << "visited before" << endl;
        }
        count ++;
        ordered_nodes[count] = next_node;
        next_node->container = NULL;

        pre_node = next_node;
    }
}

/////////////////////////////////////////////////////////////////////////
//
// RedCapTree
//
/////////////////////////////////////////////////////////////////////////
RedCapTree::RedCapTree(vector<int> _ordered_ids, vector<RedCapEdge*> _edges, AbstractRedcap* _redcap)
: ordered_ids(_ordered_ids), edges(_edges), redcap(_redcap)
{
    ssd_reduce = 0;
    ssd_utils = redcap->ssd_utils;
    controls = redcap->controls;
    control_thres = redcap->control_thres;
    
    int size = ordered_ids.size();
    int edge_size = edges.size();
    
    if (ordered_ids.size() > 1) {
        this->ssd = ssd_utils->ComputeSSD(ordered_ids, 0, size);
        max_id = -1;
        for (int i=0; i<size; i++) {
            if (ordered_ids[i] > max_id) {
                max_id = ordered_ids[i];
            }
        }
        
        // use edges and ordered_ids to create nbr_dict and od_array
        unordered_map<int, vector<int> > nbr_dict;
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
        
        // testing
        for (int i=0; i<ordered_ids.size(); i++) {
            if (nbr_dict.find( ordered_ids[i] ) == nbr_dict.end()){
                cout << "testing wrong" <<endl;
            }
        }
        
        Partition(ordered_ids, od_array, nbr_dict);
    } else {
        this->ssd = 0;
        this->ssd_reduce = 0;
    }
}

RedCapTree::~RedCapTree()
{
}

void RedCapTree::Partition(vector<int>& ids,
                           vector<pair<int, int> >& od_array,
                           unordered_map<int, vector<int> >& nbr_dict)
{
    int od_size = od_array.size();
    int size = nbr_dict.size();
    int id, orig_id, dest_id;
    int i, e_idx, k = 1, cnt=0;
    
    /*
    vector<vector<int> > id_edge_dict(size);
    for (i=0; i<nbr_dict.size(); i++) {
        id_edge_dict[i].resize(nbr_dict[i].size());
    }
    vector<int> nbr_count(size,0);
    for (i=od_size-1; i>=0; --i) {
        orig_id = od_array[i].first;
        dest_id = od_array[i].second;
        
        id_edge_dict[orig_id][ nbr_count[orig_id]++ ] = i;
        id_edge_dict[dest_id][ nbr_count[dest_id]++ ] = i;
    }
    
    vector<int> access_order(od_size);
    access_order[0] = od_size - 1;
    
    vector<bool> od_flags(od_size, false);
    od_flags[od_size - 1] = true;
    
    while ( k < od_size ) {
        int idx = access_order[cnt];
        orig_id = od_array[idx].first;
        dest_id = od_array[idx].second;
        
        for (i=0; i<nbr_dict[orig_id].size(); i++) {
            e_idx = id_edge_dict[orig_id][i];
            if ( !od_flags[e_idx] ) {
                od_flags[e_idx] = true;
                access_order[k++] = e_idx;
            }
        }
        
        for (i=0; i<nbr_dict[dest_id].size(); i++) {
            e_idx = id_edge_dict[dest_id][i];
            if ( !od_flags[e_idx] ) {
                od_flags[e_idx] = true;
                access_order[k++] = e_idx;
            }
        }
        cnt++;
    }
    */
    int best_edge = -1;
    int evaluated = 0;
    int split_pos = -1;
    
    
    vector<int> visited_ids(size);
    
    // cut edge one by one
    for ( i=0; i<od_array.size(); i++) {
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
        
        if (checkControl(cand_ids)) {
            evaluated++;
            for (int j=0; j<ids.size(); j++) {
                if (cand_ids[ ids[j] ] == -1) {
                    visited_ids[idx] = ids[j];
                    idx++;
                }
            }
            Measure result;
            ssd_utils->MeasureSplit(ssd, visited_ids, tmp_split_pos, result);
            if (result.measure_reduction > ssd_reduce) {
                ssd_reduce = result.measure_reduction;
                ssd = result.ssd;
                split_pos = tmp_split_pos;
                split_ids = visited_ids;
            }
        }
    }
    
    if (split_pos != -1) {
        this->split_pos = split_pos;
    }
}

void RedCapTree::Split(int orig, int dest, unordered_map<int, vector<int> >& nbr_dict, vector<int>& cand_ids)
{
    stack<int> visited_ids;
    int cur_id, i, nbr_size, nbr;
    
    visited_ids.push(orig);
    while (!visited_ids.empty()) {
        cur_id = visited_ids.top();
        visited_ids.pop();
        cand_ids[cur_id] = 1;
        vector<int>& nbrs = nbr_dict[cur_id];
        nbr_size = nbrs.size();
        for (i=0; i<nbr_size; i++) {
            nbr = nbrs[i];
            if (nbr != dest && cand_ids[nbr] == -1) {
                visited_ids.push(nbr);
            }
        }
    }
}

bool RedCapTree::checkControl(vector<int>& cand_ids)
{
    if (controls == NULL) {
        return true;
    }
    
    double val = 0;
    for (int i=0;i<cand_ids.size(); i++) {
        if (cand_ids[i] == 1) {
            val += controls[ cand_ids[i] ];
        }
    }
    
    return val > control_thres;
}

pair<RedCapTree*, RedCapTree*> RedCapTree::GetSubTrees()
{
    if (split_ids.empty()) {
        return this->subtrees;
    }
    int size = this->split_ids.size();
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
    
    vector<RedCapEdge*> part1_edges(part1_ids.size()-1);
    vector<RedCapEdge*> part2_edges(part2_ids.size()-1);
    
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
    if (cnt == 0) {
        // no cut
    } else if (cnt >1) {
        // more than 1 cut
        cout << "more than 1 cut" << endl;
    }
    
    RedCapTree* left_tree = new RedCapTree(part1_ids, part1_edges, redcap);
    RedCapTree* right_tree = new RedCapTree(part2_ids, part2_edges, redcap);
    subtrees.first = left_tree;
    subtrees.second = right_tree;
    return subtrees;
}

////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
////////////////////////////////////////////////////////////////////////////////
AbstractRedcap::AbstractRedcap(int row, int col,
                               double** _distances,
                               double** _data,
                               const vector<bool>& _undefs,
                               GalElement * _w)
: rows(row), cols(col), dist_matrix(_distances), raw_data(_data), undefs(_undefs), w(_w)
{
}

AbstractRedcap::~AbstractRedcap()
{
    if (ssd_utils) {
        delete ssd_utils;
    }
    // delete from this->cluster
    delete this->cluster;
    for (int i=0; i<edges.size(); i++) {
        delete edges[i];
    }
    for (int i=0; i<ordered_edges.size(); i++) {
        //delete ordered_edges[i];
    }
    for (int i=0; i<nodes.size(); i++) {
        delete nodes[i];
    }
    
}


void AbstractRedcap::init()
{
    ssd_utils = new SSDUtils(raw_data, rows, cols);
    
    // create nodes and edges
    nodes.resize(rows);
    for (int i=0; i<rows; i++) {
        RedCapNode* node = new RedCapNode(i);
        RedCapCluster* cluster = new RedCapCluster(node); // NOTE should be released somewhere
        nodes[i] = node;
    }
    
    RedCapNode* orig;
    RedCapNode* dest;
    double length;
    for (int i=0; i<rows; i++) {
        orig = nodes[i];
        const vector<long>& nbrs = w[i].GetNbrs();
        for (int j=0; j<w[i].Size(); j++) {
            int nbr = nbrs[j];
            dest = nodes[nbr];
            length = dist_matrix[orig->id][dest->id];
            edges.push_back(new RedCapEdge(orig, dest, length));
        }
    }
    
    Clustering();
    
    wxTextFile file("/Users/xun/Desktop/frequence.gwt");
    file.Create("/Users/xun/Desktop/frequence.gwt");
    file.Open("/Users/xun/Desktop/frequence.gwt");
    file.Clear();
    file.AddLine("0 88 ohlung record_id");
    
    for (int i=0; i<ordered_edges.size(); i++) {
        wxString line;
        line << ordered_edges[i]->orig->id+1<< " " << ordered_edges[i]->dest->id +1<< " 1" ;
        file.AddLine(line);
    }
    file.Write();
    file.Close();
}

vector<vector<int> >& AbstractRedcap::GetRegions()
{
    return cluster_ids;
}

void AbstractRedcap::Partitioning(int k)
{
    wxStopWatch sw;
    
    vector<RedCapTree*> not_split_trees;
    RedCapTree* current_tree = new RedCapTree(ordered_ids, ordered_edges, this);
    PriorityQueue sub_trees;
    sub_trees.push(current_tree);

    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        RedCapTree* tmp_tree = sub_trees.top();
        cout << tmp_tree->ssd_reduce << endl;
        sub_trees.pop();
        
        pair<RedCapTree*, RedCapTree*> children = tmp_tree->GetSubTrees();
       
        RedCapTree* left_tree = children.first;
        RedCapTree* right_tree = children.second;
    
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
    unordered_map<int, bool>::iterator node_it;
   
    for (PriorityQueue::iterator it = begin; it != end; ++it) {
        RedCapTree* tmp_tree = *it;
        cluster_ids.push_back(tmp_tree->ordered_ids);
    }
    
    for (int i = 0; i< not_split_trees.size(); i++) {
        delete not_split_trees[i];
    }
    wxString time = wxString::Format("The long running function took %ldms to execute", sw.Time());
}


////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderSLKRedCap::FirstOrderSLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
: AbstractRedcap(rows, cols, _distances, _data, _undefs, w)
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
    std::sort(edges.begin(), edges.end(), RedCapEdgeLess);
    
    int num_nodes = nodes.size();
    unordered_map<int, bool> id_dict;
    
    for (int i=0; i<edges.size(); i++) {
        RedCapEdge* edge = edges[i];
        
        RedCapNode* orig = edge->orig;
        RedCapNode* dest = edge->dest;
        
        RedCapCluster* root1 = RedCapCluster::GetRoot(orig);
        RedCapCluster* root2 = RedCapCluster::GetRoot(dest);
        
        if (root1 != root2) {
            this->cluster = new RedCapCluster(root1, root2, edge, dist_matrix);
            ordered_edges.push_back(edge);
            if (id_dict.find(orig->id)==id_dict.end()) {
                ordered_ids.push_back(orig->id);
                id_dict[orig->id] = true;
            }
            if (id_dict.find(dest->id)==id_dict.end()) {
                ordered_ids.push_back(dest->id);
                id_dict[dest->id] = true;
            }
            if (ordered_edges.size() == num_nodes - 1) {
                break;
            }
        }
    }
    
    //vector<RedCapNode*> ordered_nodes(num_nodes);
    //RedCapCluster::GetOrderedNodes(this->cluster, ordered_nodes);
    
    //ordered_ids.resize(num_nodes);
    //for (int i=0; i<num_nodes; i++ ) {
    //    ordered_ids[i] = ordered_nodes[i]->id;
    //}
}

////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
// The First-Order-ALK method also starts with the spatially contiguous graph G*. However, after each merge, the distance between the new cluster and every other cluster is recalculated. Therefore, edges that connect the new cluster and every other cluster are updated with new length values. Edges in G* are then re-sorted and re-evaluated from the beginning. The procedure stops when all objects are in one cluster. The algorithm is shown in figure 3. The complexity is O(n2log n) due to the sorting after each merge.
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderALKRedCap::FirstOrderALKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderALKRedCap::~FirstOrderALKRedCap()
{
    
}

/**
 *
 */
void FirstOrderALKRedCap::Clustering()
{
    
}

////////////////////////////////////////////////////////////////////////////////
//
// 3 FirstOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderCLKRedCap::FirstOrderCLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(rows, cols, _distances, _data, _undefs, w)
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
: AbstractRedcap(rows, cols, _distances, _data, _undefs, w)
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
}

////////////////////////////////////////////////////////////////////////////////
//
// 5 FullOrderALKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderALKRedCap::FullOrderALKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs,  GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(rows, cols, _distances, _data, _undefs, w)
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

}

////////////////////////////////////////////////////////////////////////////////
//
// 6 FullOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderCLKRedCap::FullOrderCLKRedCap(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(rows, cols, _distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderCLKRedCap::~FullOrderCLKRedCap()
{
    
}


void FullOrderCLKRedCap::Clustering()
{

}
