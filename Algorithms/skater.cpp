
#include <iostream>
#include <cfloat>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include <boost/graph/connected_components.hpp>


#include "../GenUtils.h"
#include "skater.h"

Skater::Skater(int _num_obs, int _num_vars, int _num_clusters, double** _data, vector<vector<double> >& dist_matrix, bool _check_floor, double _floor, double* _floor_variable)
: num_obs(_num_obs), num_vars(_num_vars), num_clusters(_num_clusters),data(_data), check_floor(_check_floor), floor(_floor), floor_variable(_floor_variable)
{
    Graph g(num_obs);
    for (int i=0; i<num_obs; i++) {
        for (int j=i; j<num_obs; j++) {
            // note: this distance matrix is created using weights
            if (dist_matrix[i][j] > 0) {
                boost::add_edge(i, j, dist_matrix[i][j], g);
            }
        }
    }
    get_MST(g);
    
    run();

}

Skater::~Skater()
{
    
}

void Skater::run_threads(vector<E> tree, vector<double>& scores, vector<ClusterPair>& candidates)
{
    // There are num_obs - 1 edges in the MST
    // To create K clusters, you need to remove K-1 edges from MST
    // There are C(K-1, numobs-1) ways to remove K-1 edges.
    //
    // each thread will prun the original MST by removing
    // num_clusters - 1 edges
    int n_jobs = tree.size();
    
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
        boost::thread* worker = new boost::thread(boost::bind(&Skater::prunecost, this, tree, a, b, boost::ref(scores), boost::ref(candidates)));
        threadPool.add_thread(worker);
    }
    
    threadPool.join_all();
}

vector<vector<int> > Skater::GetRegions()
{
    vector<vector<int> > regions;
    typename PriorityQueue::iterator begin = solution.begin();
    typename PriorityQueue::iterator end = solution.end();
   
    set<int>::iterator set_it;
    for (typename PriorityQueue::iterator it = begin; it != end; ++it) {
        const vector<E>& c = (*it).second;
        set<int> ids;
        for (int i=0; i< c.size(); i++) {
            ids.insert(c[i].first);
            ids.insert(c[i].second);
        }
        vector<int> reg;
        for (set_it=ids.begin(); set_it!=ids.end(); set_it++) {
            reg.push_back(*set_it);
        }
        regions.push_back(reg);
    }
    return regions;
}

void Skater::run()
{
    ClusterEl c(0, mst_edges);
    
    solution.push(c);
    
    while (solution.size() < num_clusters) {
        const ClusterEl& cluster = solution.top();
        vector<E> tree = cluster.second;
        
        // check where to split
        int tree_size = tree.size();
        vector<double> scores(tree_size);
        vector<ClusterPair> candidates(tree_size);
        
        //prunecost(tree, 0, tree_size, scores, candidates);
        run_threads(tree, scores, candidates);
        
        // check where to split
        double best_score = DBL_MAX;
        ClusterPair best_pair;
        
        for (int i=0; i<scores.size(); i++) {
            if (scores[i] > 0 && scores[i] < best_score && !candidates[i][0].empty() && !candidates[i][1].empty()) {
                best_score = scores[i];
                best_pair = candidates[i];
            }
        }
        //double score_p1 = ssw(best_pair[0]);
        //double score_p2 = ssw(best_pair[1]);
       
        if (best_pair.empty())
            break;
        
        solution.pop();
        
        if (!best_pair[0].empty())
            solution.push( ClusterEl(best_pair[0].size(), best_pair[0]));
        if (!best_pair[1].empty())
            solution.push( ClusterEl(best_pair[1].size(), best_pair[1]));
    }
}

void Skater::prunecost(vector<E> tree, int start, int end, vector<double>& scores, vector<ClusterPair>& candidates)
{
    //prune mst by removing one edge and get the best cut
    
    for (int i=start; i<=end; i++) {
        // move i-th edge to top: used by prunemst()
        E e_i = tree[i];
        tree.erase(tree.begin() + i);
        tree.insert(tree.begin(), e_i);
        
        // prune tree to get two groups
        vector<int> vex1, vex2;
        vector<E> part1, part2;
        prunemst(tree, vex1, vex2, part1, part2);
        
        // check by bound
        bool valid = true;
        if (check_floor) {
            if (bound_check(vex1) == false || bound_check(vex2) == false) {
                scores[i] = -1;
                valid = false;
            }
        }
       
        if (valid) {
            // compute objective function
            double ssw_sum = 0;
            ssw_sum += ssw(vex1);
            ssw_sum += ssw(vex2);
        
            scores[i] = ssw_sum;
            ClusterPair c;
            c.push_back(part1);
            c.push_back(part2);
            candidates[i] = c;
        }
        // restore tree
        tree.erase(tree.begin());
        tree.insert(tree.begin() + i, e_i);
    }
}

void Skater::get_MST(const Graph &in)
{
    //https://github.com/vinecopulib/vinecopulib/issues/22
    std::vector<int> p(num_vertices(in));
    
    prim_minimum_spanning_tree(in, p.data());
    
    for (int source = 0; source < p.size(); ++source)
    {
        int target = p[source];
        if (source != target) {
            //boost::add_edge(source, p[source], mst);
            mst_edges.push_back(E(source, p[source]));
        }
    }
}

double Skater::ssw(vector<E>& cluster)
{
    set<int> ids;
    for (int i=0; i<cluster.size(); i++) {
        ids.insert(cluster[i].first);
        ids.insert(cluster[i].second);
    }
    //return ssw(ids);
    return 0;
}

double Skater::ssw(vector<int>& ids)
{
    // This function computes the sum of dissimilarity between each
    // observation and the mean (scalar of vector) of the observations.
    // sum((x_i - x_min)^2)
   
    vector<int>::iterator it;
    double ssw_val = 0;
    for (int c=0; c<num_vars; c++) {
        double sum = 0;
        double n = ids.size();
        for (it=ids.begin(); it!=ids.end(); it++) {
            int i = *it;
            sum += data[i][c];
        }
        double mean = n > 0 ? sum / n : 0;
       
        sum = 0;
        for (it=ids.begin(); it!=ids.end(); it++) {
            int i = *it;
            double tmp = data[i][c] - mean;
            sum += tmp * tmp;
        }
        
        ssw_val += sum;
    }
    return ssw_val;
}

bool Skater::bound_check(vector<int>& cluster)
{
    vector<int>::iterator it;
    double sum=0;
    for (it=cluster.begin(); it!=cluster.end(); it++) {
        int i = *it;
        sum += floor_variable[i];
    }
    return sum >= floor;
}

// This function deletes a first edge and makes two subsets of edges. Each
// subset is a Minimun Spanning Treee.
void Skater::prunemst(vector<E>& edges, vector<int>& vex1, vector<int>& vex2, vector<E>& part1, vector<E>& part2)
{
    // first edge is going to be removed
    int num_edges = edges.size();
    int i, j, n1=1, li=0, ls=1;
    
    vector<int> no1(num_edges);
    vector<int> gr(num_edges);
    
    //no1[0] = e1[0];
    no1[0] = edges[0].first;
    
    for (i=0; i<num_edges; i++) gr[i] = 0;
    
    do {
        for (i=li; i<ls; i++) {
            for (j=1; j<num_edges; j++) {
                if (gr[j] == 0) {
                    if (no1[i]==edges[j].first) {
                        gr[j] = 1;
                        no1[n1++] = edges[j].second; //e2[j];
                    }
                    if (no1[i]==edges[j].second) {
                        gr[j] = 1;
                        no1[n1++] = edges[j].first;
                    }
                }
            }
        }
        li = ls;
        ls = n1;
    } while(li < ls);
  
    // first edge will be removed
    vex1.push_back(edges[0].first);
    for (int i=0; i<num_edges; i++) {
        if (gr[i] == 1) vex1.push_back(edges[i].first);
        else vex2.push_back(edges[i].first);
    }
    for (int i=1; i<num_edges; i++) {
        if (gr[i] == 0) part1.push_back(edges[i]);
        else part2.push_back(edges[i]);
    }
}
