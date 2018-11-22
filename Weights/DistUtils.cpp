//
//  DistUtils.c
//  GeoDa
//
//  Created by Xun Li on 11/21/18.
//

#include <cfloat>
#include <cmath>
#include "DistUtils.h"

using namespace GeoDa;

DistUtils::DistUtils(const std::vector<std::vector<double> >& input_data)
{
    eps = 0.0;
    
    n_cols = input_data.size();
    if (n_cols > 0) {
        n_rows = input_data[0].size();
    }
    data = new double*[n_rows];
    for (size_t i=0; i<n_rows; i++) {
        data[i] = new double[n_cols];
        for (size_t j=0; j<n_cols; j++) {
            data[i][j] = input_data[j][i];
        }
    }
    // create a kdtree
    kdTree = new ANNkd_tree(data, n_rows, n_cols /*dim*/);
}

DistUtils::~DistUtils()
{
    for (size_t i=0; i<n_rows; i++) {
        delete[] data[i];
    }
    delete[] data;
    
    if (kdTree) {
        delete kdTree;
    }
}
double DistUtils::GetMinThreshold()
{
    double max_1nn_dist = 0;
    
    int k = 2; // the first one is alway the query point itself
    ANNidxArray nnIdx = new ANNidx[k];
    ANNdistArray dists = new ANNdist[k];
    for (size_t i=0; i<n_rows; i++) {
        kdTree->annkSearch(data[i], k, nnIdx, dists);
        if (dists[1] > max_1nn_dist) {
            max_1nn_dist = dists[1];
        }
    }
    delete[] nnIdx;
    delete[] dists;
    
    return sqrt(max_1nn_dist);
}

/*
 The classic 2-approximation algorithm for this problem, with running time O(nd), is to choose an arbitrary point and then return the maximum distance to another point. The diameter is no smaller than this value and no larger than twice this value.
 An optional extension: Pick random point x. Pick point y furthest from x. Pick point z furthest from y.
 More: http://www-sop.inria.fr/members/Gregoire.Malandain/diameter/
 */
double DistUtils::GetMaxThreshold()
{
    srand(0);
    int k = n_rows;
    int x_idx;
    int y_idx;
    double dist_cand = 0;
    
    int n_iter = 10;
    
    ANNidxArray nnIdx = new ANNidx[k];
    ANNdistArray dists = new ANNdist[k];
    for (size_t i=0; i<n_iter; i++) {
        x_idx = rand() % n_rows;
        kdTree->annkSearch(data[i], k, nnIdx, dists);
        y_idx = nnIdx[k-1];
        kdTree->annkSearch(data[y_idx], k, nnIdx, dists);
        if (dists[k-1] > dist_cand) {
            dist_cand = dists[k-1];
        }
    }
    delete[] nnIdx;
    delete[] dists;
    
    return sqrt(dist_cand);
}

void DistUtils::CreateDistBandWeights(double band, bool is_inverse, int power)
{
    double radius = ANN_POW(band);
    for (size_t i=0; i<n_rows; i++) {
        int k = kdTree->annkFRSearch(data[i], radius, 0, NULL, NULL);
        ANNidxArray nnIdx = new ANNidx[k];
        ANNdistArray dists = new ANNdist[k];
        kdTree->annkFRSearch(data[i], radius, k, nnIdx, dists);
        for (size_t j=0; j<k; j++) {
            // iter each neighbor
        }
        delete[] nnIdx;
        delete[] dists;
    }
}
