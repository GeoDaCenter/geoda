/**
 * Source code from: https://cran.r-project.org/web/packages/dbscan/
 *
 * Created and Modified: 5/30/2017 lixun910@gmail.com
 */

#ifndef __GEODA_CENTER_HDBSCAN_H__
#define __GEODA_CENTER_HDBSCAN_H__

#include <boost/unordered_map.hpp>
#include <algorithm>
#include <vector>

#include "../kNN/ANN/ANN.h"

class RawDistMatrix;

namespace Gda {
    struct IdxCompare
    {
        const std::vector<int>& target;
        
        IdxCompare(const std::vector<int>& target): target(target) {}
        
        bool operator()(int a, int b) const { return target[a] < target[b]; }
    };
    
    class SimpleEdge
    {
    public:
        int orig;
        int dest;
        double length;
        
        SimpleEdge(int o, int d, double l) {
            orig = o;
            dest = d;
            length = l;
        }
        ~SimpleEdge(){}
    };
    
    class UnionFind
    {
    public:
        int* parent;
        int* size;
        int next_label;

    public:
        UnionFind(int N) {
            parent = new int[2*N-1];
            for (int i=0; i<2*N-1; i++) {
                parent[i] = -1;
            }
            next_label = N;
            size = new int[2*N -1];
            for (int i=0; i<2*N -1; i++) {
                if (i < N) {
                    size[i] = 1;
                } else {
                    size[i] = 0;
                }
            }
            
        }
        ~UnionFind() {
            delete[] parent;
            delete[] size;
        }
        
        int fast_find(int n) {
            int p = n;
            while (parent[n] != -1) {
                n = parent[n];
            }
            // label up to the root
            while (parent[p] != n) {
                parent[p] = n;
                p = parent[p];
            }
            return n;
        }
       
        void Union(int m, int n) {
            size[next_label] = size[m] + size[n];
            parent[m] = next_label;
            parent[n] = next_label;
            size[next_label] = size[m] + size[n];
            next_label += 1;
        }
    };
    
    class TreeUnionFind
    {
    public:
        std::vector<bool> is_component;
        std::vector<std::pair<int, int> > _data;
        
        TreeUnionFind(int size) {
            _data.resize(size);
            is_component.resize(size);
            for (int i=0; i<size; i++) {
                is_component[i] = true;
                _data[i] = std::make_pair(i, 0);
            }
        }
        
        void union_(int x, int y) {
            int x_root = find(x);
            int y_root = find(y);
            
            if (_data[x_root].second < _data[y_root].second) {
                _data[x_root].first = y_root;
            } else if (_data[x_root].second > _data[y_root].second) {
                _data[y_root].first = x_root;
            } else {
                _data[y_root].first = x_root;
                _data[x_root].second += 1;
            }
        }
        
        int find(int x) {
            if (_data[x].first != x) {
                _data[x].first = find(_data[x].first);
                is_component[x] = false;
            }
            return _data[x].first;
        }
        
        std::vector<int> components() {
            std::vector<int> c;
            for (int i=0; i<is_component.size(); i++) {
                if (is_component[i]) {
                    c.push_back(i);
                }
            }
            return c;
        }
    };
    
    class CondensedTree
    {
    public:
        CondensedTree() {
            parent = 0;
            child = 0;
            lambda_val = 0;
            child_size = 0;
        }
        CondensedTree(int p, int c, double l, int cs) {
            parent = p;
            child = c;
            lambda_val = l;
            child_size = cs;
        }
        CondensedTree(const CondensedTree& t) {
            parent = t.parent;
            child = t.child;
            lambda_val = t.lambda_val;
            child_size = t.child_size;
        }
        ~CondensedTree(){}
        
        int parent;
        int child;
        double lambda_val;
        int child_size;
    };
    
    
    /////////////////////////////////////////////////////////////////////////
    //
    // HDBSCAN
    //
    /////////////////////////////////////////////////////////////////////////
    class HDBScan
    {
    public:
        int rows;
        int cols;
        
        double** single_linkage_tree;
        std::vector<SimpleEdge*> mst_edges;
        std::vector<CondensedTree*> condensed_tree;
        std::vector<double> core_dist;
        std::vector<int> labels;
        std::vector<double> probabilities;
        std::vector<double> stabilities;
        std::vector<double> outliers;
        std::set<int> clusters;
        boost::unordered_map<int, int> cluster_map, reverse_cluster_map;
        
        HDBScan(int min_points,
                int min_samples,
                double alpha,
                int cluster_selection_method,
                bool allow_single_cluster,
                int rows, int cols,
                RawDistMatrix* raw_dist,
                std::vector<double> core_dist,
                const std::vector<bool>& undefs
                //GalElement * w,
                //double* controls,
                //double control_thres
        );
        virtual ~HDBScan();

        static std::vector<double> ComputeCoreDistance(double** input_data, int n_pts,
                                              int n_dim, int min_samples,
                                              char dist);
        static std::vector<SimpleEdge*> mst_linkage_core_vector(int num_features,
                                                    std::vector<double>& core_distances,
                                                    RawDistMatrix* dist_metric,
                                                    double alpha);
        
        void Run();
        
        std::vector<std::vector<int> > GetRegions();
        
        std::vector<double> outlier_scores(std::vector<CondensedTree*>& tree);
        
        boost::unordered_map<int, double> compute_stability(
                                        std::vector<CondensedTree*>& condensed_tree);
        
        void condense_tree(double** hierarchy, int N, int min_cluster_size=10);
        
        std::vector<double> max_lambdas(std::vector<CondensedTree*>& tree);
        
        std::vector<int> do_labelling(std::vector<CondensedTree*>& tree,
                            std::set<int>& clusters,
                            boost::unordered_map<int, int>& cluster_label_map,
                            bool allow_single_cluster = false,
                            bool match_reference_implementation = false);
        
        std::vector<double> get_probabilities(std::vector<CondensedTree*>& tree,
                            boost::unordered_map<int, int>& reverse_cluster_map,
                            std::vector<int>& labels);
        
        std::vector<double> get_stability_scores(std::vector<int>& labels,
                                std::set<int>& clusters,
                                boost::unordered_map<int, double>& stability,
                                double max_lambda);
        
        void get_clusters(std::vector<CondensedTree*>& tree,
                          boost::unordered_map<int, double>& stability,
                          std::vector<int>& out_labels,
                          std::vector<double>& out_probs,
                          std::vector<double>& out_stabilities,
                          int cluster_selection_method=0,
                          bool allow_single_cluster= false,
                          bool match_reference_implementation=false);
        
        std::vector<int> get_cluster_tree_leaves(std::vector<CondensedTree*>& cluster_tree);
        
        std::vector<int> recurse_leaf_dfs(std::vector<CondensedTree*>& cluster_tree,
                                     int current_node);
        
        std::vector<int> bfs_from_hierarchy(double** hierarchy, int dim, int bfs_root)
        {
            int max_node = 2* dim;
            int num_points = max_node - dim + 1;
            
            std::vector<int> to_process;
            to_process.push_back(bfs_root);
            
            std::vector<int> result;
            while (!to_process.empty()) {
                for (int i=0; i<to_process.size(); i++) {
                    result.push_back(to_process[i]);
                }
                std::vector<int> tmp;
                for (int i=0; i<to_process.size(); i++) {
                    if (to_process[i] >= num_points) {
                        int x = to_process[i] - num_points;
                        tmp.push_back(x);
                    }
                }
                to_process.clear();
                if (!tmp.empty()) {
                    for (int i=0; i<tmp.size(); i++) {
                        to_process.push_back(hierarchy[ tmp[i] ][0]);
                        to_process.push_back(hierarchy[ tmp[i] ][1]);
                    }
                }
            }
            return result;
        }
        
        std::vector<int> bfs_from_cluster_tree(std::vector<CondensedTree*>& tree, int bfs_root)
        {
            std::vector<int> result;
            std::set<int> to_process;
            std::set<int>::iterator it;
            
            to_process.insert(bfs_root);
            
            while (!to_process.empty()) {
                for (it = to_process.begin(); it != to_process.end(); it++) {
                    result.push_back(*it);
                }
                std::set<int> tmp;
                for (int i=0; i<tree.size(); i++) {
                    if (to_process.find( tree[i]->parent) != to_process.end() ) {
                        tmp.insert( tree[i]->child);
                    }
                }
                to_process = tmp;
            }
            return result;
        }
    };
    
}
#endif
