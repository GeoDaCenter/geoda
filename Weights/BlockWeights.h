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
#include <boost/unordered_map.hpp>
class GeoDaWeight;

class BlockWeights
{
public:
    BlockWeights();

    BlockWeights(const std::vector<std::vector<wxInt64> >& cat_values,
                 const GeoDaWeight* cont_weights);
    ~BlockWeights();

    std::vector<wxInt64> GetClusters();

    std::vector<std::vector<int> > CheckContiguity(int start, std::vector<long> nbrs, const GeoDaWeight* w);

protected:
    int num_obs;

    int num_vars;

    std::vector<wxInt64> cluster_ids;

    boost::unordered_map<int, int> cluster;
};

#endif /* BlockWeights_hpp */
