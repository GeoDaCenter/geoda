
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


using namespace boost;

typedef std::pair<int, int> E;

struct CompareCluster
{
public:
    bool operator() (const std::pair<double, std::vector<E> > & lhs, const std::pair<double, std::vector<E> > & rhs) const
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
    > BGraph;

typedef std::vector<std::vector<E> > ClusterPair;
typedef std::pair<double, std::vector<E> > ClusterEl;
typedef heap::priority_queue<ClusterEl, heap::compare<CompareCluster> > PriorityQueue;

class Skater {
public:
    Skater(int num_obs, int num_vars, int num_clusters, double** _data,
           std::vector<std::vector<double> >& dist_matrix,
           bool check_floor, double floor, double* floor_variable);
    ~Skater();
    
    std::vector<std::vector<int> > GetRegions();
    
protected:
    int num_obs;
    int num_clusters;
    int num_vars;
    double** data;
    bool check_floor;
    double floor;
    double* floor_variable;
    std::vector<E> mst_edges;
    
    heap::priority_queue<ClusterEl, heap::compare<CompareCluster> > solution;
    
    void get_MST(const BGraph &in);
    
    void run();
    
    void run_threads(std::vector<E> tree, std::vector<double>& scores, std::vector<std::vector<std::set<int> > >& cids, std::vector<ClusterPair>& candidates);
   
    void prunecost(std::vector<E> tree, int start, int end, std::vector<double>& scores, std::vector<std::vector<std::set<int> > >& cids, std::vector<ClusterPair>& candidates);
    
    void prunemst(std::vector<E>& edges, std::set<int>& vex1, std::set<int>& vex2, std::vector<E>& part1, std::vector<E>& part2);
   
    double ssw(std::vector<E>& cluster);
    
    double ssw(std::set<int>& ids);
    
    bool bound_check(std::set<int>& cluster);
};

#endif
