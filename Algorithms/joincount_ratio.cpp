#include <boost/unordered_map.hpp>

#include "joincount_ratio.h"

using namespace boost;

JoinCountRatio sub_joincount_ratio(std::string cluster, unordered_map<int, bool>& selected, GeoDaWeight* w)
{
    int total_neighbors = 0;
    int total_joincount = 0;
    
    unordered_map<int, bool>::iterator it;
    for (it = selected.begin(); it != selected.end(); ++it) {
        int idx = it->first;
        const std::vector<long>& nbrs = w->GetNeighbors(idx);
        int nn = (int)nbrs.size();
        total_neighbors += nn;
        // get join count for obs[idx] = 1
        for (int j=0; j < nn; ++j) {
            int nn = (int)nbrs[j];
            if (selected.find(nn) != selected.end()) {
                total_joincount += 1;
            }
        }
    }
    
    double ratio = total_neighbors > 0 ? total_joincount / (double)total_neighbors : 0;
    
    JoinCountRatio result;
    result.cluster = cluster;
    result.n = (int)selected.size();
    result.totalNeighbors = total_neighbors;
    result.totalJoinCount = total_joincount;
    result.ratio = ratio;
    
    return result;
}

std::vector<JoinCountRatio> joincount_ratio(const std::vector<wxString>& clusters, GeoDaWeight* w)
{
    std::vector<JoinCountRatio> result;
    
    if (w == 0) return result;
    
    int num_obs = w->GetNumObs();
    
    if (num_obs != clusters.size()) return result;
    
    // Turn each regime into a dummy variable
    // Compute local join counts for that variable (jc)
    // Compute number of neighbors for each observation (nn)
    // Take ratio of sum to total sum of neighbors
    std::map<std::string, unordered_map<int, bool> > regimes;
    
    int nClusters = (int)clusters.size();
    
    for (int i=0; i < nClusters; ++i) {
        std::string c = std::string(clusters[i].mb_str());
        regimes[c][i] = true;
    }
    
    std::map<std::string, unordered_map<int, bool> >::iterator it;
    for (it = regimes.begin(); it != regimes.end(); ++it) {
        std::string cluster = it->first;
        JoinCountRatio jcr = sub_joincount_ratio(cluster, it->second, w);
        result.push_back(jcr);
    }
    
	return result;
}

JoinCountRatio all_joincount_ratio(const std::vector<JoinCountRatio>& items)
{
    JoinCountRatio result;
    
    int nItems = (int)items.size();
    for (int i=0; i<nItems; ++i) {
        result.n += items[i].n;
        result.totalNeighbors += items[i].totalNeighbors;
        result.totalJoinCount += items[i].totalJoinCount;
    }
    
    result.ratio = result.totalNeighbors > 0 ? result.totalJoinCount / (double)result.totalNeighbors : 0;
    
    return result;
}
