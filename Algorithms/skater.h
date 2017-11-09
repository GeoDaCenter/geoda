
#ifndef __GEODA_SKATER_H_
#define __GEODA_SKATER_H_

#include <vector>
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
    int id;
    MSTNode* left;
    MSTNode* right;
};

class MSTree {
public:
    MSTNode* root;
    
    void addEdge(E& e);
    
    vector<MSTree*> split(int e1, int e2);
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
    
    void run_threads();
   
    void prunecost(vector<E> tree, int start, int end, vector<double>& scores, vector<ClusterPair>& candidates);
    
    void prunemst(vector<E>& edges, vector<int>& vex1, vector<int>& vex2, vector<E>& part1, vector<E>& part2);
   
    double ssw(vector<E>& cluster);
    
    double ssw(vector<int>& ids);
    
    bool bound_check(vector<int>& cluster);
};

#endif
