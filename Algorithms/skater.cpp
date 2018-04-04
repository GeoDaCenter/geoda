
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


void SRegion::findBestCut()
{
    if (not_splitable) {
        return;
    }
    
    int n_edges = edges.size();
    
    set<int> ids;
    for (int i=0; i<edges.size(); i++) {
        ids.insert(edges[i].first);
        ids.insert(edges[i].second);
    }
    ssd = compute_ssd(ids, data, num_vars);
    
    if (n_edges <= 1) {
        sub_ssd = 0;
        int node1 = edges[0].first;
        int node2 = edges[0].second;
        left = new SRegion(node1, data, num_vars, check_floor, floor, floor_variable);
        right = new SRegion(node2, data, num_vars, check_floor, floor, floor_variable);
        return;
    }

    int tree_size = edges.size();
    vector<double> scores(tree_size);
    vector<vector<set<int> > > cids(tree_size);
    vector<ClusterPair> candidates(tree_size);
    
    //prunecost(data, num_vars, edges, 0, tree_size-1, scores, cids, candidates, check_floor, floor_variable, floor);
    //run_threads(edges, scores, cids, candidates);
    
    sub_ssd = scores[0];
    best_cut = 0;
    
    for (int i=1; i<scores.size(); i++) {
        if (scores[i] < sub_ssd) {
            sub_ssd = scores[i];
            best_cut = i;
        }
    }
    if (cids[best_cut][0].size() == 1) {
        int node = *cids[best_cut][0].begin();
        left = new SRegion(node, data, num_vars, check_floor, floor, floor_variable);
    } else {
        left =new SRegion(candidates[best_cut][0], data, num_vars, check_floor, floor, floor_variable);
    }
    
    if (cids[best_cut][1].size() == 1) {
        int node = *cids[best_cut][1].begin();
        right = new SRegion(node, data, num_vars, check_floor, floor, floor_variable);
    } else {
        right =new SRegion(candidates[best_cut][1], data, num_vars, check_floor, floor, floor_variable);
    }
}


Skater::Skater(int _num_obs, int _num_vars, int _num_clusters, double** _data, vector<vector<double> >& dist_matrix, bool _check_floor, double _floor, double* _floor_variable)
: num_obs(_num_obs), num_vars(_num_vars), num_clusters(_num_clusters),data(_data), check_floor(_check_floor), floor(_floor), floor_variable(_floor_variable)
{
    //ssd_dict.clear();
    
    Graph g(num_obs);
    for (int i=0; i<num_obs; i++) {
        for (int j=i; j<num_obs; j++) {
            // note: this distance matrix is created using weights
            if (dist_matrix[i][j] >= 0) {
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

void Skater::run_threads(vector<E> tree, vector<double>& scores, vector<vector<set<int> > >& cids, vector<ClusterPair>& candidates)
{
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
       
        //prunecost(data, num_vars, tree, a, b, scores, cids, candidates, check_floor, floor_variable, floor);
        boost::thread* worker = new boost::thread(boost::bind(&Skater::prune, this, tree, a, b, boost::ref(scores), boost::ref(cids), boost::ref(candidates)));
        threadPool.add_thread(worker);
    }
    
    threadPool.join_all();
}

vector<vector<int> > Skater::GetRegions()
{
    vector<vector<int> > regions;
    PriorityQueue::iterator begin = solution.begin();
    PriorityQueue::iterator end = solution.end();
   
    set<int>::iterator set_it;
    for (PriorityQueue::iterator it = begin; it != end; ++it) {
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

vector<vector<int> > Skater::GetRegions1()
{
    vector<vector<int> > regions;

    set<int>::iterator set_it;
    for (int i=0; i<sorted_solution.size(); i++) {
        set<int> ids = sorted_solution[i]->GetIds();
        vector<int> reg;
        for (set_it=ids.begin(); set_it!=ids.end(); set_it++) {
            reg.push_back(*set_it);
        }
        regions.push_back(reg);
    }
    return regions;
}


void Skater::run1()
{

    SRegion* root = new SRegion(mst_edges, data, num_vars, check_floor, floor, floor_variable);
    root->findBestCut();
    root->UpdateTotalSSD(root->ssd);
   
    sorted_solution.clear();
    sorted_solution.push_back(root);
    
    int nclst = 1;
    while (nclst < num_clusters) {
        SRegion* region = sorted_solution.back();
        
        SRegion* left = region->left;
        SRegion* right = region->right;
        
        sorted_solution.pop_back();
        delete region;
        nclst--;
        
            left->findBestCut();
            right->findBestCut();
        
        sorted_solution.push_back(left);
        nclst++;
        sorted_solution.push_back(right);
        nclst++;
        
        double total_ssd = 0;
        for (int i=0; i<sorted_solution.size(); i++) {
            total_ssd += sorted_solution[i]->ssd;
        }
        // update total_ssd for every regions
        for (int i=0; i<sorted_solution.size(); i++) {
            sorted_solution[i]->UpdateTotalSSD(total_ssd);
        }
        
        std::sort(sorted_solution.begin(), sorted_solution.end(), RegionLess);
    }
}

void Skater::run()
{
    ClusterEl c(0, mst_edges);
    
    solution.push(c);
    
    while (solution.size() < num_clusters) {
        const ClusterEl& cluster = solution.top();
        vector<E> tree = cluster.second;
       
        double sswt = compute_ssd(tree, data, num_vars);
        // check where to split
        int tree_size = tree.size();
        vector<double> scores(tree_size);
        vector<vector<set<int> > > cids(tree_size);
        vector<ClusterPair> candidates(tree_size);
        
        //prunecost(tree, 0, tree_size-1, scores, cids, candidates);
        run_threads(tree, scores, cids, candidates);
       
        for (int i=0; i<scores.size(); i++) scores[i] = sswt - scores[i];
        
        // check where to split
        double best_score = scores[0];
        vector<set<int> > best_cids = cids[0];
        ClusterPair best_pair = candidates[0];
        
        for (int i=1; i<scores.size(); i++) {
            if (scores[i] > best_score && !candidates[i].empty()) {
                best_score = scores[i];
                best_cids = cids[i];
                best_pair = candidates[i];
            }
        }
        
        if (best_pair.empty())
            break;
        
        solution.pop();
        
        // subtree 1
        int t1_size = best_pair[0].size();
        vector<double> scores1(t1_size);
        vector<vector<set<int> > > cids1(t1_size);
        vector<ClusterPair> cand1(t1_size);
        
        run_threads(best_pair[0], scores1, cids1, cand1);
        
        double best_score_1 = DBL_MAX;
        for (int i=0; i<scores1.size(); i++) {
            if (scores1[i] < best_score_1 && !cand1[i].empty()) {
                best_score_1 = scores1[i];
            }
        }
        best_score_1 = compute_ssd(best_pair[0], data, num_vars) - best_score_1;
        if (t1_size == 0) {
            set<int>& tmp_set = best_cids[0];
            int tmp_id = *tmp_set.begin();
            E tmp_e(tmp_id, tmp_id);
            best_pair[0].push_back(tmp_e);
        }
        solution.push( ClusterEl(best_score_1, best_pair[0]));
        
        // subtree 2
        int t2_size = best_pair[1].size();
        vector<double> scores2(t2_size);
        vector<vector<set<int> > > cids2(t2_size);
        vector<ClusterPair> cand2(t2_size);
        
        run_threads(best_pair[1], scores2, cids2, cand2);
        
        double best_score_2 = DBL_MAX;
        for (int i=0; i<scores2.size(); i++) {
            if (scores2[i] < best_score_2 && !cand2[i].empty()) {
                best_score_2 = scores2[i];
            }
        }
        best_score_2 = compute_ssd(best_pair[1], data, num_vars) - best_score_2;
        if (t2_size == 0) {
            set<int>& tmp_set = best_cids[1];
			if (!tmp_set.empty()) {
                int tmp_id = *tmp_set.begin();
                E tmp_e(tmp_id, tmp_id);
                best_pair[1].push_back(tmp_e);
			}
        }
        solution.push( ClusterEl(best_score_2, best_pair[1]));
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

void Skater::prune(vector<E> tree, int start, int end, vector<double>& scores, vector<vector<set<int> > >& cids, vector<ClusterPair>& candidates)
{
    prunecost(data, num_vars, tree, start, end, scores, cids, candidates, check_floor, floor_variable, floor);
}

void prunecost(double** data, int num_vars, vector<E> tree, int start, int end, vector<double>& scores, vector<vector<set<int> > >& cids, vector<ClusterPair>& candidates, bool check_floor, double* floor_variable, double floor)
{
    //prune mst by removing one edge and get the best cut
    
    for (int i=start; i<=end; i++) {
        // move i-th edge to top: used by prunemst()
        E e_i = tree[i];
        tree.erase(tree.begin() + i);
        tree.insert(tree.begin(), e_i);
        
        // prune tree to get two groups
        set<int> vex1, vex2;
        vector<E> part1, part2;
        prunemst(tree, vex1, vex2, part1, part2);
        
        // check by bound
        bool valid = true;
        if (check_floor && valid) {
            if (bound_check(vex1, floor_variable, floor) == false || bound_check(vex2, floor_variable, floor) == false) {
                valid = false;
            }
        }
        
        // compute objective function
        double ssw1 = compute_ssd(vex1, data, num_vars);
        double ssw2 = compute_ssd(vex2, data, num_vars);
        scores[i] = ssw1 + ssw2;
        
        if (valid) {
            vector<set<int> > pts;
            pts.push_back(vex1);
            pts.push_back(vex2);
            cids[i] = pts;
            
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

double compute_ssd(vector<E>& cluster, double** data, int num_vars)
{
    set<int> ids;
    for (int i=0; i<cluster.size(); i++) {
        ids.insert(cluster[i].first);
        ids.insert(cluster[i].second);
    }
    return compute_ssd(ids, data, num_vars);
}

double compute_ssd(set<int>& ids, double** data, int num_vars)
{
    // This function computes the sum of dissimilarity between each
    // observation and the mean (scalar of vector) of the observations.
    // sum((x_i - x_min)^2)
    
    if (ids.empty()) {
        return 0;
    }
    
    if (ssd_dict.find(ids) != ssd_dict.end()) {
        return ssd_dict[ids];
    }
    
    double n = ids.size();
    vector<double> means(num_vars);
    set<int>::iterator it;
    
    for (int c=0; c<num_vars; c++) {
        double sum = 0;
        for (it=ids.begin(); it!=ids.end(); it++) {
            int i = *it;
            sum += data[i][c];
        }
        double mean = n > 0 ? sum / n : 0;
        means[c] = mean;
    }

    double ssw_val = 0;
    
    for (it=ids.begin(); it!=ids.end(); it++) {
        double sum = 0;
        int i = *it;
        for (int c=0; c<num_vars; c++) {
            double tmp = data[i][c] - means[c];
            sum += tmp * tmp;
        }
        
        ssw_val += sqrt(sum);
    }
        
    boost::mutex::scoped_lock scoped_lock(mutex);
    ssd_dict[ids]  = ssw_val;
    
    return ssw_val;
}

bool bound_check(set<int>& cluster, double* floor_variable, double floor)
{
    set<int>::iterator it;
    double sum=0;
    for (it=cluster.begin(); it!=cluster.end(); it++) {
        int i = *it;
        sum += floor_variable[i];
    }
    return sum >= floor;
}

// This function deletes a first edge and makes two subsets of edges. Each
// subset is a Minimun Spanning Treee.
void prunemst(vector<E>& edges, set<int>& set1, set<int>& set2, vector<E>& part1, vector<E>& part2)
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
    set1.insert(edges[0].first);
    for (int i=0; i<gr.size(); i++) {
        if (gr[i] == 1) {
            set1.insert(edges[i].first);
            set1.insert(edges[i].second);
        }
    }
    for (int i=0; i<gr.size(); i++) {
        if (set1.find(edges[i].first) == set1.end()) {
            set2.insert(edges[i].first);
        }
        if (set1.find(edges[i].second) == set1.end()) {
            set2.insert(edges[i].second);
        }
    }
    
    for (int i=1; i<num_edges; i++) {
        if (gr[i] == 1) part1.push_back(edges[i]);
        else part2.push_back(edges[i]);
    }
}
