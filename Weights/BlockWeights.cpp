//
//  BlockWeights.cpp
//  GeoDa
//
//  Created by Xun Li on 4/7/21.
//
#include <set>
#include <stack>

#include "../ShapeOperations/GeodaWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/WeightUtils.h"
#include "BlockWeights.h"


BlockWeights::BlockWeights() { 
    
}

BlockWeights::BlockWeights(const std::vector<std::vector<wxInt64> >& cat_values,
                           const GeoDaWeight *cont_weights)
{
    num_vars = (int)cat_values.size();
    num_obs = (int)cat_values[0].size();

    std::vector<GeoDaWeight*> ws;
    boost::unordered_map<wxInt64, std::set<int> >::iterator it;
    std::set<int>::iterator it1, it2;

    for (int i=0; i<num_vars; i++) {
        // create a dict for groups
        boost::unordered_map<wxInt64, std::set<int> > cat_dict;
        const std::vector<wxInt64>& vals = cat_values[i];
        for (int j=0; j< (int)vals.size(); ++j) {
            if (vals[j] >= 0) { // avoid ignored observations value=-1
                cat_dict[ vals[j] ].insert(j);
            }
        }

        // create block weights for this variable
        GalElement* gal = new GalElement[num_obs];
        for (it = cat_dict.begin(); it != cat_dict.end(); ++it) {
            int c = it->first;
            std::set<int>& ids = it->second;
            int nbr_sz = (int)ids.size() - 1;
            // ids will be neighbors of each other
            for (it1 = ids.begin(); it1 != ids.end(); ++it1) {
                int cnt = 0;
                gal[*it1].SetSizeNbrs(nbr_sz);
                for (it2 = ids.begin(); it2 != ids.end(); ++it2) {
                    if (*it1 != *it2) {
                        gal[*it1].SetNbr(cnt++, *it2);
                    }
                }
            }
        }
        GalWeight* w = new GalWeight();
        w->num_obs = num_obs;
        w->gal = gal;
        ws.push_back(w);
    }

    GalWeight* new_w = WeightUtils::WeightsIntersection(ws);
    for (int i=0; i< (int)ws.size(); ++i) {
        delete ws[i];
    }

    // create cluster map from new weights
    GalElement* gal = new_w->gal;
    int c = 1;
    cluster_ids.resize(num_obs, 0);

    for (int i=0; i<num_obs; i++) {
        int nn = (int)gal[i].Size();
        if (cluster.find(i) == cluster.end()) {
            if (nn == 0) {
                cluster[i] = 0;
            } else {
                // check contiguity, and separate islands
                std::vector<long> nbrs = gal[i].GetNbrs();
                cluster[i] = c;
                cluster_ids[i] = c;
                for (int j=0; j<nn; ++j) {
                    cluster[ nbrs[j] ] = c;
                    cluster_ids[ nbrs[j] ] = c;
                }
                c += 1;
                /*
                std::vector<std::vector<int> > groups = CheckContiguity(i, nbrs, cont_weights);
                for (int j=0; j<(int)groups.size(); ++j) {
                    std::vector<int> group = groups[j];
                    for (int k=0; k < (int)group.size(); ++k) {
                        cluster[ group[k] ] = c;
                        cluster_ids[ group[k] ] = c;
                    }
                    c += 1;
                }
                 */
            }
        }
    }

    delete new_w;
}

std::vector<std::vector<int> > BlockWeights::CheckContiguity(int start, std::vector<long> nbrs, const GeoDaWeight* w)
{
    // check contiguity, and separate islands into groups
    std::vector<std::vector<int> > groups;

    boost::unordered_map<int, bool> cluster_dict;
    for (int i=0; i < (int)nbrs.size(); ++i) {
        cluster_dict[(int)nbrs[i]] = true;
    }
    cluster_dict[start] = true;

    while (cluster_dict.empty() == false) {
        int start = cluster_dict.begin()->first;
        cluster_dict.erase(start);

        std::stack<int> processed_ids;
        processed_ids.push(start);

        std::vector<int> group;

        while (processed_ids.empty() == false) {
            int fid = processed_ids.top();
            processed_ids.pop();
            group.push_back(fid);

            std::vector<long> nn = w->GetNeighbors(fid);
            for (int i=0; i<(int)nn.size(); ++i) {
                int nid = (int)nn[i];
                if (cluster_dict.find(nid) != cluster_dict.end()) {
                    processed_ids.push(nid);
                    cluster_dict.erase(nid);
                }
            }
        }

        groups.push_back(group);
    }
    return groups;
}

BlockWeights::~BlockWeights() {

}

std::vector<wxInt64> BlockWeights::GetClusters() {
    return cluster_ids;
}



