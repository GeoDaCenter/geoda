#include "../kNN/ANN/ANN.h"
#include "dbscan.h"

DBSCAN::DBSCAN(unsigned int min_samples, float eps, const double** input_data,
               unsigned int num_rows, unsigned int num_cols, int distance_metric)
: eps(eps), min_samples(min_samples), num_rows(num_rows), num_cols(num_cols),
averagen(0)
{
    ANN_DIST_TYPE = distance_metric;
    // create a kdtree
    kd_tree = new ANNkd_tree((ANNpointArray)input_data, num_rows, num_cols /*dim*/);

    createNearestNeighbors(input_data);
    
    run();
}

DBSCAN::~DBSCAN()
{
    if (kd_tree) delete kd_tree;

    // reset this global variable, so it won't impact using ANN in other code
    ANN_DIST_TYPE = ANNuse_euclidean_dist;
}

double DBSCAN::getAverageNN()
{
    return averagen;
}

std::vector<int> DBSCAN::getResults()
{
    return labels;
}

void DBSCAN::run()
{
    // Initially, all samples are noise.
    labels.resize(num_rows, -1);

    // A list of all core samples found.
    is_core.resize(num_rows, false);

    for (size_t i=0; i<num_rows; ++i) {
        if (nn[i].size() >= min_samples) {
            is_core[i] = true;
        }
    }

    // Fast inner loop for DBSCAN
    // see: scikit-learn/sklearn/cluster/_dbscan_inner.pyx
    int label_num = 0, v;
    std::vector<int> stack;

    for (size_t i=0; i<num_rows; ++i) {
        if (labels[i] != -1 || is_core[i] == false) {
            continue;
        }

        // Depth-first search starting from i, ending at the non-core points.
        // This is very similar to the classic algorithm for computing connected
        // components, the difference being that we label non-core points as
        // part of a cluster (component), but don't expand their neighborhoods.
        int k = i;
        while (true) {
            if (labels[k] ==  -1) {
                labels[k] = label_num;
                if ( is_core[k] ) {
                    std::vector<std::pair<int, double> >& neighb = nn[k];
                    for (size_t j=0; j<neighb.size(); ++j) {
                        v = neighb[j].first;
                        if (labels[v] == -1) {
                            // push v to stack
                            stack.push_back(v);
                        }
                    }
                }
            }
            if ( stack.size() == 0) {
                break;
            }
            k = stack.back();
            stack.pop_back();
        }
        label_num += 1;
    }
}

void DBSCAN::createNearestNeighbors(const double** input_data)
{
    int total_nn = 0;
    // This has worst case O(n^2) memory complexity
    double radius = ANN_POW(eps), w;
    nn.resize(num_rows);
    for (size_t i=0; i<num_rows; i++) {
        std::vector<std::pair<int, double> > nbrs;
        int k = kd_tree->annkFRSearch((ANNpoint)input_data[i], radius, 0, NULL, NULL);
        ANNidxArray nnIdx = new ANNidx[k];
        ANNdistArray dists = new ANNdist[k];
        total_nn += k;
        kd_tree->annkFRSearch((ANNpoint)input_data[i], radius, k, nnIdx, dists);
        for (size_t j=0; j<k; j++) {
            // iter each neighbor
            int nbr_id = nnIdx[j];
            //if (nbr_id != i) {
                w = ANN_ROOT(dists[j]);
                nbrs.push_back(std::make_pair(nbr_id,w));
            //}
        }
        delete[] nnIdx;
        delete[] dists;
        nn[i] = nbrs;
    }
    averagen = total_nn / (double) num_rows;
}
