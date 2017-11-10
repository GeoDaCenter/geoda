
#ifndef __GEODA_SKATER_H_
#define __GEODA_SKATER_H_

#include <vector>
#include <boost/unordered_map.hpp>
#include <set>
#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include <boost/thread/thread.hpp>
#include <boost/heap/priority_queue.hpp>


using namespace std;
using namespace boost;

typedef std::pair<int, int> E;

struct CompareCluster
{
public:
    bool operator() (const pair<double, vector<E> > & lhs, const pair<double, vector<E> > & rhs) const
    {
        return lhs.first < rhs.first;
    }
};

typedef adjacency_list <
    vecS,
    vecS,
    undirectedS,
    boost::no_property,         //VertexProperties
    property < edge_weight_t, double>   //EdgeProperties
    > Graph;

typedef std::vector<std::vector<E> > ClusterPair;
typedef std::pair<double, std::vector<E> > ClusterEl;
typedef heap::priority_queue<ClusterEl, heap::compare<CompareCluster> > PriorityQueue;

class MSTNode {
public:
    MSTNode(int _id) {
        id = _id;
    }
    void AddNode(MSTNode* nbr) {
        nbrs.push_back(nbr);
    }
    void Break(int nid) {
        for (int i=0; i<nbrs.size(); i++) {
            if (nbrs[i]->id == nid) {
                nbrs.erase(nbrs.begin() + i);
                return;
            }
        }
    }
    
    int id;
    vector<MSTNode*> nbrs;
};

class MSTree {
public:
    MSTree(vector<E>& e) {
        for (int i=0; i<e.size(); i++) {
            MSTNode* n1 = new MSTNode(e[i].first);
            MSTNode* n2 = new MSTNode(e[i].second);
            n1->AddNode(n2);
            n2->AddNode(n1);
            node_dict[e[i].first] = n1;
            node_dict[e[i].second] = n2;
        }
    }
    MSTree(MSTNode* node) {
        vector<MSTNode*> stack;
        stack.push_back(node);
        while (!stack.empty()) {
            MSTNode* tmp = stack.back();
            stack.pop_back();
            for (int i=0; i<tmp->nbrs.size(); i++) {
                MSTNode* nn = tmp->nbrs[i];
                stack.push_back(nn);
            }
        }
    }
    ~MSTree() {
        unordered_map<int, MSTNode*>::iterator it;
        for (it=node_dict.begin(); it!=node_dict.end(); it++) {
            MSTNode* nd = it->second;
            delete nd;
        }
        node_dict.clear();
    }
    
    vector<MSTNode*> split(E e) {
        // return two trees
        int n1 = e.first;
        int n2 = e.second;
        vector<MSTNode*> trees;
        
        if (node_dict.find(n1) != node_dict.end()) {
            MSTNode* node1 = node_dict[n1];
            node1->Break(n2);
            trees.push_back(node1);
        }
        if (node_dict.find(n2) != node_dict.end()) {
            MSTNode* node2 = node_dict[n2];
            node2->Break(n1);
            trees.push_back(node2);
        }
        return trees;
    }
    
    void recovery(E e) {
        int n1 = e.first;
        int n2 = e.second;
        MSTNode* node1 = node_dict[n1];
        MSTNode* node2 = node_dict[n2];
        node1->AddNode(node2);
        node2->AddNode(node1);
    }
    
protected:
    MSTNode* root;
    unordered_map<int, MSTNode*> node_dict;
};

class Skater {
public:
    Skater(int num_obs, int num_vars, int num_clusters, double** _data,
           vector<vector<double> >& dist_matrix,
           bool check_floor, double floor, double* floor_variable);
    ~Skater();
    
    vector<vector<int> > GetRegions();
    
protected:
    int num_obs;
    int num_clusters;
    int num_vars;
    double** data;
    bool check_floor;
    double floor;
    double* floor_variable;
    vector<E> mst_edges;
    
    heap::priority_queue<ClusterEl, heap::compare<CompareCluster> > solution;
    
    void get_MST(const Graph &in);
    
    void run();
    
    void run_threads(vector<E> tree, vector<double>& scores, vector<ClusterPair>& candidates);
   
    void prunecost(vector<E> tree, int start, int end, vector<double>& scores, vector<ClusterPair>& candidates);
    
    void prunemst(vector<E>& edges, vector<int>& vex1, vector<int>& vex2, vector<E>& part1, vector<E>& part2);
   
    double ssw(vector<E>& cluster);
    
    double ssw(vector<int>& ids);
    
    bool bound_check(vector<int>& cluster);
};

#endif
