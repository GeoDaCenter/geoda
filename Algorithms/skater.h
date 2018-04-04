
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

static unordered_map<std::set<int>, double> ssd_dict; // <start, end>: ssd value
double compute_ssd(vector<E>& cluster, double** data, int num_vars);
double compute_ssd(set<int>& ids, double** data, int num_vars);

void prunemst(vector<E>& edges, set<int>& vex1, set<int>& vex2, vector<E>& part1, vector<E>& part2);
void prunecost(double** data, int num_vars, vector<E> tree, int start, int end, vector<double>& scores, vector<vector<set<int> > >& cids, vector<ClusterPair>& candidates, bool check_floor, double* floor_variable, double floor);
bool bound_check(set<int>& cluster, double* floor_variable, double floor);



class SRegion {
public:
    SRegion(vector<E>& _edges, double** _data, int _num_vars,bool _check_floor, double _floor, double* _floor_variable) {
        edges = _edges;
        data = _data;
        num_vars = _num_vars;
        check_floor = _check_floor;
        floor = _floor;
        floor_variable = _floor_variable;
        
        not_splitable = false;
    }
    SRegion(int _node, double** _data, int _num_vars,bool _check_floor, double _floor, double* _floor_variable) {

        node = _node;
        data = _data;
        num_vars = _num_vars;
        check_floor = _check_floor;
        floor = _floor;
        floor_variable = _floor_variable;
        
        not_splitable = true;
        score = DBL_MAX;
    }
    
    ~SRegion(){}

    void UpdateTotalSSD(double _total_ssd) {
        if (!not_splitable) {
            total_ssd = _total_ssd;
            score = total_ssd - ssd + sub_ssd;
        }
    }
    
    set<int> GetIds()
    {
        set<int> ids;
        if (not_splitable) {
            ids.insert(node);
        } else {
            for (int i=0; i< edges.size(); i++) {
                ids.insert(edges[i].first);
                ids.insert(edges[i].second);
            }
        }
        return ids;
    }
    
    void findBestCut();
    
    bool not_splitable;
    double** data;
    int num_vars;
    bool check_floor;
    double floor;
    double* floor_variable;
    
    int node;
    vector<E> edges;
    SRegion* left;
    SRegion* right;
    ClusterPair children;
    
    double total_ssd;
    double ssd;
    double sub_ssd;
    int best_cut; //idx
    
    double score;
};

struct {
    bool operator()(SRegion* a, SRegion* b) const
    {
        return a->score > b->score;
    }
} RegionLess;

class Skater {
public:
    Skater(int num_obs, int num_vars, int num_clusters, double** _data,
           vector<vector<double> >& dist_matrix,
           bool check_floor, double floor, double* floor_variable);
    ~Skater();
    
    vector<vector<int> > GetRegions();
    vector<vector<int> > GetRegions1();
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
    vector<SRegion*> sorted_solution;
    
    void get_MST(const Graph &in);
    
    void run();
    
    void run1();
    
    void run_threads(vector<E> tree, vector<double>& scores, vector<vector<set<int> > >& cids, vector<ClusterPair>& candidates);
    
    void prune(vector<E> tree, int start, int end, vector<double>& scores, vector<vector<set<int> > >& cids, vector<ClusterPair>& candidates);
    
};

#endif
