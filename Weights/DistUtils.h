//
//  DistUtils.h
//  GeoDa
//
//  Created by Xun Li on 11/21/18.
//

#ifndef DistUtils_h
#define DistUtils_h

#include <vector>
#include <stdio.h>

#include "../kNN/ANN/ANN.h"

namespace GeoDa {
    typedef std::vector<std::vector<std::pair<int, double> > > Weights;
    
    class DistUtils
    {
    protected:
        ANNkd_tree* kdTree;
        double** data;
        double eps;
        unsigned long n_cols;
        unsigned long n_rows;
        
    public:
        DistUtils(const std::vector<std::vector<double> >& input_data,
                  int distance_metric = 2);
        ~DistUtils();
        
        // The minimum threshold distance guarantees that every observation has
        // at least one neighbor if creating a weights
        double GetMinThreshold();
        
        // The maximu, threshold distance should be the diameter of the space
        // represents by all data points
        double GetMaxThreshold();
        
        GeoDa::Weights CreateDistBandWeights(double band, bool is_inverse, int power);
        
        GeoDa::Weights CreateKNNWeights(int k, bool is_inverse, int power);
        
        // is_adaptive_bandwidth: true: use distance of k n-neighbors as bandwidth
        //                        false: use max KNN as bandwidth
        // apply_kernel_to_diag:  true: apply kernel to diagnal weights
        //                        false: diagonal weights = 1
        GeoDa::Weights CreateAdaptiveKernelWeights(int kernel_type, int k,
            bool is_adaptive_bandwidth = true,
            bool apply_kernel_to_diag = false);
        
        GeoDa::Weights CreateAdaptiveKernelWeights(int kernel_type, double band,
            bool apply_kernel_to_diag = false);
        
        void ApplyKernel(GeoDa::Weights& w, int kernel_type, bool apply_kernel_to_diag);
    };
}
#endif /* DistUtils_h */
