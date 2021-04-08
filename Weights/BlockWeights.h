//
//  BlockWeights.hpp
//  GeoDa
//
//  Created by Xun Li on 4/7/21.
//

#ifndef BlockWeights_hpp
#define BlockWeights_hpp

#include <vector>
#include <wx/wx.h>

class GeoDaWeight;

class BlockWeights
{
public:
    BlockWeights();

    BlockWeights(const std::vector<std::vector<wxInt64> >& cat_values,
                 const GeoDaWeight* cont_weights);
    ~BlockWeights();

    std::vector<wxInt64> GetClusters();

    void CheckContiguity(int start, std::vector<long> nbrs, GalElement* gal);

protected:
    int num_obs;

    int num_vars;

    std::vector<wxInt64> cluster_ids;

    std::unordered_map<int, int> cluster;
};

#endif /* BlockWeights_hpp */
