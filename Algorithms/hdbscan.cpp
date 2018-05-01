#include <deque>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include "hdbscan.h"


using namespace GeoDaClustering;


bool EdgeLess1(Edge* a, Edge* b)
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
////////////////////////////////////////////////////////////////////////////////
//
// HDBSCAN
//
////////////////////////////////////////////////////////////////////////////////
HDBScan::HDBScan(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
{
    all_nodes.resize(rows);
    nbr_dict.resize(rows);
    
    for (int i=0; i<rows; i++) {
        all_nodes[i] = new Node(i);
    }
    
    // compute core distances
    vector<double> core_dist(rows);
    
    int k = 10;
    int m_clSize = 1;
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
    
    // compute mutual reachability
    double** mutual_dist = new double*[rows];
    for (int i=0; i<rows; i++) {
        mutual_dist[i] = new double[rows];
    }
    for (int i=0; i<rows; i++) {
        for (int j=0; j<i; j++) {
            if (i==j) {
                mutual_dist[i][j] = core_dist[i];
            } else {
                mutual_dist[i][j] = _distances[i][j];
                if (mutual_dist[i][j] < core_dist[i]) {
                    mutual_dist[i][j] = core_dist[i];
                }
                if (mutual_dist[i][j] < core_dist[j]) {
                    mutual_dist[i][j] = core_dist[j];
                }
                mutual_dist[j][i] = mutual_dist[i][j];
            }
        }
    }
    
    // MST
    mst_edges.resize(rows-1);
    Graph g(rows);
    boost::unordered_map<pair<int, int>, bool> access_dict;
    for (int i=0; i<rows; i++) {
        for (int j=0; j<i; j++) {
            if (i!=j) {
                boost::add_edge(i, j, mutual_dist[i][j], g);
            }
        }
    }
    std::vector<int> p(num_vertices(g));
    boost::prim_minimum_spanning_tree(g, p.data());
    
    double sum_length=0;
    int cnt = 0;
    for (int source = 0; source < p.size(); ++source) {
        int target = p[source];
        if (source != target) {
            double length = mutual_dist[source][target];
            mst_edges[cnt++] = new Edge(all_nodes[source],all_nodes[target], length);
        }
    }
    
    // Extend the MST to obtain MSText, by adding for each vertex a “self edge"
    for (int i=0; i<rows; i++) {
        mst_edges.push_back(new Edge(all_nodes[i], all_nodes[i], core_dist[i]));
    }
    
    // Extract the HDBSCAN hierarchy as a dendrogram from mst
    vector<UnionFind*> ufs(rows);
    for (int i=0; i<rows; i++) {
        ufs[i] = new UnionFind(i);
    }
    
    //sort edges from smallest weight to largest
    std::sort(mst_edges.begin(), mst_edges.end(), EdgeLess1);
    
    //everyone starts in their own cluster!
    vector<vector<int> > currentGroups(rows);
    for (int i=0; i<rows; i++) {
        currentGroups[i].push_back(i);
    }
    
    int next_cluster_label = 0;
    // List of all the cluster options we have found
    vector<vector<int> > cluster_options;
    
    // Stores a list for each cluster. Each value in the sub list is the
    // weight at which that data point was added to the cluster
    vector<vector<double> > entry_size;
    vector<double> birthSize, deathSize;
    vector<pair<int, int> > children;
    vector<int> map_to_cluster_label(rows, -1);
    
    for (int i=0; i<mst_edges.size(); i++) {
        Edge* e = mst_edges[i];
        int from = e->orig->id;
        int to = e->dest->id;
        double weight = e->length;
        
        if (to == from) {
            continue;
        }
        
        UnionFind* union_from = ufs[from];
        UnionFind* union_to = ufs[to];
        
        int clust_A = union_from->find()->getItem();
        int clust_B = union_to->find()->getItem();
        
        UnionFind* clust_A_tmp = union_from->find();
        UnionFind* clust_B_tmp = union_to->find();
        
        union_from->Union(union_to);
        
        int a_size = currentGroups[clust_A].size();
        int b_size = currentGroups[clust_B].size();
        int new_size = a_size+b_size;
        
        int mergedClust;
        int otherClust;
        if(union_from->find()->getItem() == clust_A) {
            mergedClust = clust_A;
            otherClust = clust_B;
        } else { //other way around
            mergedClust = clust_B;
            otherClust = clust_A;
        }
        
        if ( new_size >= m_clSize && a_size < m_clSize && b_size < m_clSize) {
            //birth of a new cluster!
            cluster_options.push_back(currentGroups[mergedClust]);
            
            vector<double> dl(new_size, weight);
            entry_size.push_back(dl);
            
            children.push_back(make_pair(-1,-1)); //we have no children!
            birthSize.push_back(weight);
            deathSize.push_back(DBL_MAX);//we don't know yet
            map_to_cluster_label[mergedClust] = next_cluster_label;
            next_cluster_label++;
            
        } else if (new_size >= m_clSize && a_size >= m_clSize && b_size >= m_clSize) {
            //birth of a new cluster from the death of two others!
            //record the weight that the other two died at
            deathSize[ map_to_cluster_label[mergedClust] ] = weight;
            deathSize[ map_to_cluster_label[otherClust] ] = weight;
            
            //replace with new object so that old references
            // in cluster_options are not altered further
            vector<int> copy_group = currentGroups[mergedClust];
            currentGroups[mergedClust] = copy_group;
            
            cluster_options.push_back(currentGroups[mergedClust]);
            vector<double> dl(new_size, weight);
            entry_size.push_back(dl);
            
            children.push_back(make_pair(map_to_cluster_label[mergedClust], map_to_cluster_label[otherClust]));
            birthSize.push_back(weight);
            deathSize.push_back(DBL_MAX);//we don't know yet
            map_to_cluster_label[mergedClust] = next_cluster_label;
            next_cluster_label++;
            
        }  else if( new_size >= m_clSize ) {
            // existing cluster has grown in size,
            // so add the points and record their weight for use later
            // index may change, so book keeping update
            if (map_to_cluster_label[mergedClust] == -1) {
                //the people being added are the new owners
                //set to avoid index out of boudns bellow
                int c = map_to_cluster_label[mergedClust] = map_to_cluster_label[otherClust];
                //make sure we keep track of the correct list
                cluster_options[c] = currentGroups[mergedClust];
                map_to_cluster_label[otherClust] = -1;
            }
            
            
            for(int  j=0; j< currentGroups[otherClust].size(); j++) {
                entry_size[ map_to_cluster_label[mergedClust] ].push_back(weight);
            }
        }
        
        for (int j=0; j<currentGroups[otherClust].size(); j++) {
            currentGroups[mergedClust].push_back(currentGroups[otherClust][j]);
        }
        currentGroups[otherClust].clear();
    }
    
    //Remove the last "cluster" because its the dumb one of everything
    cluster_options.erase(cluster_options.begin() + cluster_options.size()-1);
    entry_size.erase(entry_size.begin() + entry_size.size()-1);
    birthSize.erase(birthSize.begin() + birthSize.size()-1);
    deathSize.erase(deathSize.begin() + deathSize.size()-1);
    children.erase(children.begin() + children.size()-1);
    
    // See equation (3) in paper
    vector<double> S(cluster_options.size());
    for(int c = 0; c < S.size(); c++) {
        double lambda_min = birthSize[c];
        double lambda_max = deathSize[c];
        double s = 0;
        for(int j=0; j< entry_size[c].size(); j++) {
            double f_x = entry_size[c][j];
            s += min(f_x, lambda_max) - lambda_min;
        }
        S[c] = s;
    }
    
    vector<bool> toKeep(S.size(), true);
    vector<double> S_hat(cluster_options.size());
    
    //Queue<Integer> notKeeping = new ArrayDeque<>();
    deque<int> notKeeping;
    for(int i = 0; i < S.size(); i++)
    {
        pair<int, int>& child = children[i];
        if(child.first == -1) {
            //I'm a leaf!
            //for all leaf nodes, set ˆS(C_h)= S(C_h)
            S_hat[i] = S[i];
            continue;
        }
        int il = child.first;
        int ir = child.second;
        //If S(C_i) < ˆS(C_il)+ ˆ S(C_ir ), set ˆS(C_i)= ˆS(C_il)+ ˆS(C_ir )and set δi =0.
        if (S[i] < S_hat[il] + S_hat[ir]) {
            S_hat[i] = S_hat[il] + S_hat[ir];
            toKeep[i] = false;
        } else {
            // set ˆS(C_i)= S(C_i)and set δ(·) = 0 for all clusters in C_i’s subtrees.
            S_hat[i] = S[i];
            //place children in q to process and set all sub children as not keeping
            notKeeping.push_back(il);
            notKeeping.push_back(ir);
            while(!notKeeping.empty())
            {
                int c = notKeeping.front();
                notKeeping.pop_front();
                toKeep[c] = false;
                pair<int, int>& c_children = children[c];
                if(c_children.first == -1) {
                    continue;
                }
                notKeeping.push_back(c_children.first);
                notKeeping.push_back(c_children.second);
            }
        }
    }
    
    //initially fill with -1 indicating it was noise
    for (int i=0; i<rows; i++) {
        designations[i] = -1;
    }
    
    int clusters = 0;
    for (int c = 0; c < toKeep.size(); c++) {
        if (toKeep[c]) {
            for(int j=0; j< cluster_options[c].size(); j++) {
                int indx = cluster_options[c][j];
                designations[indx] = clusters;
            }
            clusters++;
        }
    }
    
    //return designations;
}

HDBScan::~HDBScan()
{
    for (int i=0; i<rows; i++) {
        delete all_nodes[i];
    }
}
