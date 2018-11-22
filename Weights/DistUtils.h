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
    
    class DistUtils
    {
    protected:
        ANNkd_tree* kdTree;
        double** data;
        double eps;
        unsigned long n_cols;
        unsigned long n_rows;
        
    public:
        DistUtils(const std::vector<std::vector<double> >& input_data);
        ~DistUtils();
        
        // The minimum threshold distance guarantees that every observation has
        // at least one neighbor if creating a weights
        double GetMinThreshold();
        
        // The maximu, threshold distance should be the diameter of the space
        // represents by all data points
        double GetMaxThreshold();
        
        void CreateDistBandWeights(double band, bool is_inverse, int power);
    };
}
#endif /* DistUtils_h */
