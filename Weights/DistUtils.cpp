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

DistUtils::DistUtils(const std::vector<std::vector<double> >& input_data, int distance_metric)
{
    eps = 0.0;
    
    ANN_DIST_TYPE = distance_metric;
    
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
        kdTree->annkSearch(data[x_idx], k, nnIdx, dists);
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

GeoDa::Weights DistUtils::CreateDistBandWeights(double band, bool is_inverse, int power)
{
    GeoDa::Weights weights;
    
    double radius = ANN_POW(band);
    double w;
    
    for (size_t i=0; i<n_rows; i++) {
        int k = kdTree->annkFRSearch(data[i], radius, 0, NULL, NULL);
        ANNidxArray nnIdx = new ANNidx[k];
        ANNdistArray dists = new ANNdist[k];
        kdTree->annkFRSearch(data[i], radius, k, nnIdx, dists);
        std::vector<std::pair<int, double> > nbrs;
        for (size_t j=0; j<k; j++) {
            // iter each neighbor
            if (nnIdx[j] != i) {
                w = ANN_ROOT(dists[j]);
                if (is_inverse) {
                    w = pow(w, power);
                }
                nbrs.push_back(std::make_pair(nnIdx[j],w));
            }
        }
        weights.push_back(nbrs);
        delete[] nnIdx;
        delete[] dists;
    }
    
    return weights;
}

GeoDa::Weights DistUtils::CreateKNNWeights(int k, bool is_inverse, int power)
{
    GeoDa::Weights weights;
    
    double w;
    ANNidxArray nnIdx = new ANNidx[k+1];
    ANNdistArray dists = new ANNdist[k+1];
    for (size_t i=0; i<n_rows; i++) {
        // k+1, because data[i] will be always returned
        kdTree->annkSearch(data[i], k+1, nnIdx, dists);
        std::vector<std::pair<int, double> > nbrs;
        for (size_t j=0; j<k+1; j++) {
            // iter each neighbor
            if (nnIdx[j] != i) {
                w = ANN_ROOT(dists[j]);
                if (is_inverse) {
                    w = pow(w, power);
                }
                nbrs.push_back(std::make_pair(nnIdx[j],w));
            }
        }
        weights.push_back(nbrs);
    }
    delete[] nnIdx;
    delete[] dists;
    
    return weights;
}

GeoDa::Weights DistUtils::CreateAdaptiveKernelWeights(int kernel_type, int k,
                                           bool is_adaptive_bandwidth,
                                           bool apply_kernel_to_diag)
{
    GeoDa::Weights weights;
    double w;
    double max_knn_bandwidth = 0;
    ANNidxArray nnIdx = new ANNidx[k+1];
    ANNdistArray dists = new ANNdist[k+1];
    
    if (is_adaptive_bandwidth) {
        for (size_t i=0; i<n_rows; i++) {
            // k+1, because data[i] will be always returned
            kdTree->annkSearch(data[i], k+1, nnIdx, dists);
            std::vector<std::pair<int, double> > nbrs;
            double local_band = 0;
            for (size_t j=0; j<k+1; j++) {
                // iter each neighbor, include itself
                if (dists[j] > local_band) {
                    local_band = dists[j];
                }
            }
            local_band = ANN_ROOT(local_band);
            for (size_t j=0; j<k+1; j++) {
                // iter each neighbor
                w = ANN_ROOT(dists[j]);
                w = local_band > 0 ? w / local_band : 0;
                nbrs.push_back(std::make_pair(nnIdx[j], w));
            }
            weights.push_back(nbrs);
        }
    } else {
        // use max knn distance as bandwidth
        for (size_t i=0; i<n_rows; i++) {
            // k+1, because data[i] will be always returned
            kdTree->annkSearch(data[i], k+1, nnIdx, dists);
            std::vector<std::pair<int, double> > nbrs;
            for (size_t j=0; j<k+1; j++) {
                // iter each neighbor
                w = ANN_ROOT(dists[j]);
                nbrs.push_back(std::make_pair(nnIdx[j], w));
                if (w > max_knn_bandwidth) {
                    max_knn_bandwidth = w;
                }
            }
            weights.push_back(nbrs);
        }
        for (size_t i=0; i<n_rows; i++) {
            for (size_t j=0; j<weights[i].size(); j++) {
                weights[i][j].second /= max_knn_bandwidth;
            }
        }
    }
    
    delete[] nnIdx;
    delete[] dists;
    
    ApplyKernel(weights, kernel_type, apply_kernel_to_diag);
    
    return weights;
}

GeoDa::Weights DistUtils::CreateAdaptiveKernelWeights(int kernel_type, double band,
                                           bool apply_kernel_to_diag)
{
    GeoDa::Weights weights;
    double radius = ANN_POW(band);
    double w;
    
    for (size_t i=0; i<n_rows; i++) {
        int k = kdTree->annkFRSearch(data[i], radius, 0, NULL, NULL);
        ANNidxArray nnIdx = new ANNidx[k];
        ANNdistArray dists = new ANNdist[k];
        kdTree->annkFRSearch(data[i], radius, k, nnIdx, dists);
        std::vector<std::pair<int, double> > nbrs;
        for (size_t j=0; j<k; j++) {
            // iter each neighbor
            w = ANN_ROOT(dists[j]) / band;
            nbrs.push_back(std::make_pair(nnIdx[j],w));
        }
        weights.push_back(nbrs);
        delete[] nnIdx;
        delete[] dists;
    }
    ApplyKernel(weights, kernel_type, apply_kernel_to_diag);
    return weights;
}

void DistUtils::ApplyKernel(GeoDa::Weights& w, int kernel_type, bool apply_kernel_to_diag)
{
    double gaussian_const = pow(M_PI * 2.0, -0.5);
    
    for (size_t i=0; i<w.size(); i++) {

        for (size_t j=0; j<w[i].size(); j++) {
            if (!apply_kernel_to_diag && i==w[i][j].first) {
                // diagonal weights = 1
                w[i][j].second = 1.0;
                continue;
            }
            // functions follow Anselin and Rey (2010) table 5.4
            if (kernel_type == 0) {
                // uniform
                w[i][j].second = 0.5;
            } if (kernel_type == 1) {
                // triagular
                w[i][j].second = 1 - w[i][j].second;
            } else if (kernel_type == 2) {
                // epanechnikov
                w[i][j].second = (3.0 / 4.0) * (1.0 - pow(w[i][j].second,2.0));
            } else if (kernel_type == 3) {
                // quartic
                w[i][j].second = (15.0 / 16.0) * pow((1.0 - pow(w[i][j].second,2.0)), 2.0);
            } else if (kernel_type == 4) {
                // gaussian
                w[i][j].second = gaussian_const * exp( -pow(w[i][j].second, 2.0) / 2.0 );
            }
        }
    }
}
