#include <deque>
#include <algorithm>

#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include "hdbscan.h"


using namespace GeoDaClustering;


bool EdgeLess1(const SimpleEdge& a, const SimpleEdge& b)
{
    return a.length <= b.length;
}

bool CondessLess(const CondensedTree& a, const CondensedTree& b)
{
    return a.child <= b.child;
}

bool CondessParentLess(const CondensedTree& a, const CondensedTree& b)
{
    return a.parent <= b.parent;
}

////////////////////////////////////////////////////////////////////////////////
//
// HDBSCAN
//
////////////////////////////////////////////////////////////////////////////////
HDBScan::HDBScan(int k, int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs
                 //,GalElement* w, double* _controls, double _control_thres
                 )
{
    int cluster_selection_method = 0;
    bool allow_single_cluster = false;
    bool match_reference_implementation = false;
    
    // compute core distances
    core_dist.resize(rows);
    
    int min_cluster_size = k;
    int dim = cols;
    double eps = 0; // error bound
    int nPts = rows;
    
    ANNkd_tree* kdTree = new ANNkd_tree(_data, nPts, dim);

    ANNidxArray nnIdx = new ANNidx[k];
    ANNdistArray dists = new ANNdist[k];
    for (int i=0; i<nPts; i++) {
        kdTree->annkSearch(_data[i], k, nnIdx, dists, eps);
        core_dist[i] = sqrt(dists[k-1]);
    }
    delete[] nnIdx;
    delete[] dists;
    delete kdTree;
    
    // MST
    mst_edges = mst_linkage_core_vector(dim, core_dist, _distances, 1.0);
    std::sort(mst_edges.begin(), mst_edges.end(), EdgeLess1);
    
    // Extract the HDBSCAN hierarchy as a dendrogram from mst
    int N = rows;
    UnionFind U(N);
    single_linkage_tree = new double*[N-1];
    for (int i=0; i<mst_edges.size(); i++) {
        SimpleEdge& e = mst_edges[i];
        int a = e.orig;
        int b = e.dest;
        double delta = e.length;
        
        int aa = U.fast_find(a);
        int bb = U.fast_find(b);
        
        single_linkage_tree[i] = new double[4];
        single_linkage_tree[i][0] = aa;
        single_linkage_tree[i][1] = bb;
        single_linkage_tree[i][2] = delta;
        single_linkage_tree[i][3] = U.size[aa] + U.size[bb];
        
        //cout << a << " " << b << " " << delta << endl;
        cout << aa << " " << bb << " " << delta << " " <<single_linkage_tree[i][3] << endl;
        U.Union(aa, bb);
    }
    
    // following: _tree_to_labels()
    
    // condensed_tree = condense_tree(single_linkage_tree, min_cluster_size)
    condensed_tree = condense_tree(single_linkage_tree, N, min_cluster_size);
    
    // stability_dict = compute_stability(condensed_tree)
    boost::unordered_map<int, double> stability_dict = compute_stability(condensed_tree);
    
    // labels, probabilities, stabilities = get_clusters(condensed_tree,
    get_clusters(condensed_tree, stability_dict, labels, probabilities, stabilities, cluster_selection_method, allow_single_cluster, match_reference_implementation);
    
    for (int i=0; i<N-1; i++) {
        delete[] single_linkage_tree[i];
    }
    delete[] single_linkage_tree;
}

HDBScan::~HDBScan()
{
}

vector<vector<int> > HDBScan::GetRegions()
{
    int min_cid = labels[0];
    int max_cid = labels[0];
    for (int i=0; i<labels.size(); i++) {
        if (labels[i] > max_cid) {
            max_cid = labels[i];
        }
        if (labels[i] < min_cid) {
            min_cid = labels[i];
        }
    }
    vector<vector<int> > regions(max_cid + 1);
    
    int cid = 0;
    for (int i=0; i<labels.size(); i++) {
        if (labels[i] >=0) {
            int cid = labels[i];
            regions[cid].push_back(i);
        }
    }
    
    return regions;
}

vector<CondensedTree> HDBScan::condense_tree(double** hierarchy, int N, int min_cluster_size)
{
    int root = 2 * (N-1);
    int num_points = root /2 + 1;
    int next_label = num_points + 1;
    
    vector<int> node_list = bfs_from_hierarchy(hierarchy, N-1, root);
    
    vector<int> relabel(root+1);
    relabel[root] = num_points;
    vector<CondensedTree> result_list;
    vector<bool> ignore(node_list.size(),false);
    
    double lambda_value;
    int left_count, right_count;
    
    for (int i=0; i<node_list.size(); i++) {
        int node = node_list[i];
        if (ignore[node] || node < num_points) {
            continue;
        }
        
        double* children = hierarchy[node-num_points];
        int left = children[0];
        int right = children[1];
        
        if (children[2] > 0.0) {
            lambda_value = 1.0 / children[2];
        } else {
            lambda_value = DBL_MAX;
        }
        
        if (left >= num_points) {
            left_count = hierarchy[left - num_points][3];
        } else {
            left_count = 1;
        }
        
        if (right >= num_points) {
            right_count = hierarchy[right - num_points][3];
        } else {
            right_count = 1;
        }
        
        if (left_count >= min_cluster_size && right_count >= min_cluster_size) {
            relabel[left] = next_label;
            next_label += 1;
            CondensedTree node1(relabel[node], relabel[left], lambda_value, left_count);
            result_list.push_back(node1);
            
            relabel[right] = next_label;
            next_label += 1;
            CondensedTree node2(relabel[node], relabel[right], lambda_value, right_count);
            result_list.push_back(node2);
            
        } else if (left_count < min_cluster_size && right_count < min_cluster_size) {
            vector<int> sub_nodes = bfs_from_hierarchy(hierarchy, N-1, left);
            for (int j=0; j<sub_nodes.size(); j++) {
                int sub_node = sub_nodes[j];
                if (sub_node < num_points) {
                    CondensedTree node1(relabel[node], sub_node, lambda_value, 1);
                    result_list.push_back(node1);
                }
                ignore[sub_node] = true;
            }
            sub_nodes.clear();
            sub_nodes = bfs_from_hierarchy(hierarchy, N-1, right);
            for (int j=0; j<sub_nodes.size(); j++) {
                int sub_node = sub_nodes[j];
                if (sub_node < num_points) {
                    CondensedTree node1(relabel[node], sub_node, lambda_value, 1);
                    result_list.push_back(node1);
                }
                ignore[sub_node] = true;
            }
            
        } else if (left_count < min_cluster_size) {
            relabel[right] = relabel[node];
            vector<int> sub_nodes = bfs_from_hierarchy(hierarchy, N-1, left);
            for (int j=0; j<sub_nodes.size(); j++) {
                int sub_node = sub_nodes[j];
                if (sub_node < num_points) {
                    CondensedTree node1(relabel[node], sub_node, lambda_value, 1);
                    result_list.push_back(node1);
                }
                ignore[sub_node] = true;
            }
        } else {
            relabel[left] = relabel[node];
            vector<int> sub_nodes = bfs_from_hierarchy(hierarchy, N-1, right);
            for (int j=0; j<sub_nodes.size(); j++) {
                int sub_node = sub_nodes[j];
                if (sub_node < num_points) {
                    CondensedTree node1(relabel[node], sub_node, lambda_value, 1);
                    result_list.push_back(node1);
                }
                ignore[sub_node] = true;
            }
        }
    }
    return result_list;
}

boost::unordered_map<int, double> HDBScan::compute_stability(vector<CondensedTree>& condensed_tree)
{
    int largest_child = condensed_tree[0].child;
    int smallest_cluster = condensed_tree[0].parent;
    int largest_cluster = condensed_tree[0].parent;
    for (int i=1; i<condensed_tree.size(); i++) {
        if (condensed_tree[i].child > largest_child) {
            largest_child = condensed_tree[i].child;
        }
        if (condensed_tree[i].parent < smallest_cluster) {
            smallest_cluster = condensed_tree[i].parent;
        }
        if (condensed_tree[i].parent > largest_cluster) {
            largest_cluster = condensed_tree[i].parent;
        }
    }
    int num_clusters = largest_cluster - smallest_cluster + 1;
    
    if (largest_child < smallest_cluster) {
        largest_child = smallest_cluster;
    }
    
    vector<CondensedTree> sorted_child_data;
    sorted_child_data = condensed_tree;
    std::sort(sorted_child_data.begin(), sorted_child_data.end(), CondessLess);
    
    vector<double> births(largest_child + 1, NAN);
    
    int current_child = -1;
    double min_lambda = 0;
    
    int child;
    double lambda_;
    
    for (int row=0; row < sorted_child_data.size(); row++) {
        child = sorted_child_data[row].child;
        lambda_ = sorted_child_data[row].lambda_val;
        
        if (child == current_child) {
            min_lambda = min(min_lambda, lambda_);
        } else if ( current_child != -1) {
            births[current_child] = min_lambda;
            current_child = child;
            min_lambda = lambda_;
        } else {
            // Initialize
            current_child = child;
            min_lambda = lambda_;
        }
    }
    
    if (current_child != -1) {
        births[current_child] = min_lambda;
        births[smallest_cluster] = 0.0;
    }
    
    vector<double> result_arr(num_clusters, 0);
    
    for (int i=0; i<condensed_tree.size(); i++) {
        int parent = condensed_tree[i].parent;
        double lambda_ = condensed_tree[i].lambda_val;
        int child_size = condensed_tree[i].child_size;
        int result_index = parent - smallest_cluster;
        result_arr[result_index] += (lambda_ - births[parent]) * child_size;
    }
    
    vector<int> node_list;
    boost::unordered_map<int, double> stability;
    for (int i=smallest_cluster, cnt=0; i<largest_cluster+1; i++, cnt++) {
        stability[i] = result_arr[cnt];
        node_list.push_back(i);
    }
    return stability;
}

vector<int> HDBScan::do_labelling(vector<CondensedTree>& tree, set<int>& clusters,
                                  boost::unordered_map<int, int>& cluster_label_map,
                                  bool allow_single_cluster,
                                  bool match_reference_implementation)
{
    int root_cluster = tree[0].parent;
    int parent_array_max = tree[0].parent;
    for (int i=1; i<tree.size(); i++) {
        if (tree[i].parent < root_cluster) {
            root_cluster = tree[i].parent;
        }
        if (tree[i].parent > parent_array_max) {
            parent_array_max = tree[i].parent;
        }
    }
    vector<int> result(root_cluster);
    
    TreeUnionFind union_find(parent_array_max + 1);
    
    for (int n=0; n<tree.size(); n++) {
        int child = tree[n].child;
        int parent = tree[n].parent;
        if (clusters.find(child) ==clusters.end() ) {
            union_find.union_(parent, child);
        }
    }
    
    for (int n=0; n<root_cluster; n++) {
        int cluster = union_find.find(n);
        if ( cluster < root_cluster) {
            result[n] = -1;
            
        } else if (cluster == root_cluster) {
            result[n] = -1;
            if (clusters.size()==1 && allow_single_cluster) {
                double c_lambda = -1;
                double p_lambda = -1;
                for (int j=0; j<tree.size(); j++) {
                    if (tree[j].child == n) {
                        c_lambda = tree[j].lambda_val;
                    }
                    if (tree[j].parent == cluster) {
                        if (tree[j].lambda_val > p_lambda) {
                            p_lambda = tree[j].lambda_val;
                        }
                    }
                }
                if (c_lambda >= p_lambda && p_lambda > -1) {
                    result[n] = cluster_label_map[cluster];
                }
            }
            
        } else {
            if (match_reference_implementation) {
                double point_lambda=-1, cluster_lambda=-1;
                for (int j=0; j<tree.size(); j++) {
                    if (tree[j].child == n) {
                        point_lambda = tree[j].lambda_val;
                        break;
                    }
                }
                for (int j=0; j<tree.size(); j++) {
                    if (tree[j].child == cluster) {
                        cluster_lambda = tree[j].lambda_val;
                        break;
                    }
                }
                if (point_lambda > cluster_lambda && cluster_lambda > -1) {
                    result[n] = cluster_label_map[cluster];
                } else {
                    result[n] = -1;
                }
            } else {
                result[n] = cluster_label_map[cluster];
            }
        }
    }
    return result;
}

vector<double> HDBScan::get_probabilities(vector<CondensedTree>& tree,
                                 boost::unordered_map<int, int>& cluster_map,
                                 vector<int>& labels)
{
    vector<double> result(labels.size(), 0);

    vector<double> deaths = max_lambdas(tree);
    int root_cluster = tree[0].parent;
    for (int i=0; i<tree.size(); i++) {
        if (tree[i].parent < root_cluster) {
            root_cluster = tree[i].parent;
        }
    }
    
    int cluster_num;
    int cluster;
    double max_lambda;
    double lambda_;
    
    for (int n=0; n<tree.size(); n++) {
        int point = tree[n].child;
        if (point >= root_cluster) {
            continue;
        }
        
        cluster_num = labels[point];
        
        if (cluster_num == -1) {
            continue;
        }
            
        cluster = cluster_map[cluster_num];
        max_lambda = deaths[cluster];
        
        if (max_lambda == 0.0 || tree[n].lambda_val == DBL_MAX) {
            result[point] = 1.0;
        } else {
            lambda_ = min(tree[n].lambda_val, max_lambda);
            result[point] = lambda_ / max_lambda;
        }
    }
    return result;
}

vector<double> HDBScan::get_stability_scores(vector<int>& labels, set<int>& clusters,
                                    boost::unordered_map<int, double>& stability,
                                    double max_lambda)
{
    vector<double> result(clusters.size());
    
    vector<int> clusters_;
    set<int>::iterator it;
    for (it=clusters.begin(); it!= clusters.end(); it++) {
        clusters_.push_back(*it);
    }
    sort(clusters_.begin(), clusters_.end());
    
    for (int n=0; n<clusters_.size(); n++) {
        int c = clusters_[n];
        int cluster_size = 0;
        for (int i=0; i<labels.size(); i++) {
            if (labels[i] == n) {
                cluster_size += 1;
            }
        }
        if (max_lambda == DBL_MAX || max_lambda == 0 || cluster_size == 0) {
            result[n] = 1.0;
        } else {
            result[n] = stability[c] / (cluster_size * max_lambda);
        }
    }
    return result;
}

vector<double> HDBScan::max_lambdas(vector<CondensedTree>& tree)
{
    int largest_parent = tree[0].parent;
    
    for (int i=1; i<tree.size(); i++) {
        if (tree[i].parent > largest_parent) {
            largest_parent= tree[i].parent;
        }
    }
    
    vector<CondensedTree> sorted_parent_data;
    sorted_parent_data = tree;
    sort(sorted_parent_data.begin(), sorted_parent_data.end(), CondessParentLess);
    
    vector<double> deaths(largest_parent + 1, 0);
    
    int current_parent = -1;
    double max_lambda = 0;
    
    for (int row=0; row<sorted_parent_data.size(); row++) {
        int parent = sorted_parent_data[row].parent;
        double lambda_ = sorted_parent_data[row].lambda_val;
        
        if (parent == current_parent){
            max_lambda = max(max_lambda, lambda_);
        } else if (current_parent != -1) {
            deaths[current_parent] = max_lambda;
            current_parent = parent;
            max_lambda = lambda_;
        } else {
            // Initialize
            current_parent = parent;
            max_lambda = lambda_;
        }
    }
    
    return deaths;
}

void HDBScan::get_clusters(vector<CondensedTree>& tree,
                           boost::unordered_map<int, double>& stability,
                           vector<int>& out_labels,
                           vector<double>& out_probs,
                           vector<double>& out_stabilities,
                           int cluster_selection_method,
                           bool allow_single_cluster,
                           bool match_reference_implementation)
{
    vector<int> node_list;
    boost::unordered_map<int, double>::iterator sit;
    for (sit = stability.begin(); sit!=stability.end(); sit++) {
        node_list.push_back(sit->first);
    }
    std::sort(node_list.begin(), node_list.end(), std::greater<int>());
    if (!allow_single_cluster) {
        // exclude root
        node_list.pop_back();
    }
    
    vector<CondensedTree> cluster_tree;
    for (int i=0; i<tree.size(); i++) {
        if (tree[i].child_size > 1) {
            cluster_tree.push_back(tree[i]);
        }
    }
    
    boost::unordered_map<int, bool> is_cluster;
    for (int i=0; i<node_list.size(); i++) {
        is_cluster[node_list[i]] = true;
    }
    
    int num_points = DBL_MIN;
    for (int i=0; i<tree.size(); i++) {
        if (tree[i].child_size == 1) {
            if (tree[i].child > num_points) {
                num_points = tree[i].child;
            }
        }
    }
    num_points += 1;
    
    double max_lambda = tree[0].lambda_val;
    for (int i=1; i<tree.size(); i++) {
        if (tree[i].lambda_val  > max_lambda) {
            max_lambda = tree[i].lambda_val;
        }
    }
    
    if (cluster_selection_method == 0) {
        // eom
        for (int i=0; i<node_list.size(); i++) {
            int node = node_list[i];
            vector<int> child_selection;
            for (int j=0; j<cluster_tree.size(); j++) {
                if (cluster_tree[j].parent == node) {
                    child_selection.push_back(node);
                }
            }
            double subtree_stability = 0;
            for (int j=0; j<child_selection.size(); j++) {
                if (child_selection[j] < cluster_tree.size()) {
                    int idx = cluster_tree[ child_selection[j] ].child;
                    subtree_stability += stability[idx];
                }
            }
            
            if (subtree_stability > stability[node]) {
                is_cluster[node] = false;
                stability[node] = subtree_stability;
            } else {
                vector<int> sub_nodes = bfs_from_cluster_tree(cluster_tree, node);
                for (int j=0; j<sub_nodes.size(); j++) {
                    int sub_node = sub_nodes[j];
                    if (sub_node != node) {
                        is_cluster[sub_node] = false;
                    }
                }
            }
        }
    } else { // leaf
        
    }
    
    set<int> clusters;
    boost::unordered_map<int, bool>::iterator it;
    for (it = is_cluster.begin(); it!= is_cluster.end(); it++) {
        int c = it->first;
        if (it->second) {
            clusters.insert(c);
        }
    }
    vector<int> _clusters;
    set<int>::iterator _it;
    for (_it =clusters.begin(); _it != clusters.end(); _it++) {
        _clusters.push_back(*_it);
    }
    
    std::sort(_clusters.begin(), _clusters.end());
    
    boost::unordered_map<int, int> cluster_map, reverse_cluster_map;
    for (int i=0; i<_clusters.size(); i++) {
        cluster_map[_clusters[i]] = i;
        reverse_cluster_map[i] = _clusters[i];
    }
    
    // do labeling (tree, clusters, cluster_map, False, False
    out_labels = do_labelling(tree, clusters, cluster_map, allow_single_cluster, match_reference_implementation);
    
    //  probs = get_probabilities(tree, reverse_cluster_map, labels)
    out_probs = get_probabilities(tree, reverse_cluster_map, out_labels);
    
    // stabilities = get_stability_scores(labels, clusters, stability, max_lambda)
    out_stabilities = get_stability_scores(out_labels, clusters, stability, max_lambda);
}

vector<SimpleEdge> HDBScan::mst_linkage_core_vector(int num_features, vector<double>& core_distances, double** dist_metric, double alpha)
{
    int dim = core_distances.size();
    vector<SimpleEdge> result;
    
    double current_node_core_distance;
    vector<int> in_tree(dim,0);
    int current_node = 0;
    vector<double> current_distances(dim, DBL_MAX);
    
    for (int i=1; i<dim; i++) {
        in_tree[current_node] = 1;
        current_node_core_distance = core_distances[current_node];
        
        double new_distance = DBL_MAX;
        int new_node = 0;
        
        for (int j=0; j<dim; j++) {
            if (in_tree[j]) {
                continue;
            }
            double right_value = current_distances[j];
            double left_value = dist_metric[current_node][j];
            
            if (alpha!=1.0) {
                left_value /= alpha;
            }
            
            double core_value = core_distances[j];
            if (current_node_core_distance > right_value || core_value > right_value ||  left_value > right_value) {
                if (right_value < new_distance) {
                    new_distance = right_value;
                    new_node = j;
                }
                continue;
            }
            
            if (core_value > current_node_core_distance) {
                if (core_value > left_value) {
                    left_value = core_value;
                }
            } else {
                if (current_node_core_distance > left_value) {
                    left_value = current_node_core_distance;
                }
            }
            
            if (left_value < right_value) {
                current_distances[j] = left_value;
                if (left_value < new_distance) {
                    new_distance = left_value;
                    new_node = j;
                }
            } else {
                if (right_value < new_distance) {
                    new_distance = right_value;
                    new_node = j;
                }
            }
        }
        SimpleEdge e(current_node, new_node, new_distance);
        result.push_back(e);
        
        current_node = new_node;
    }
    return result;
}
