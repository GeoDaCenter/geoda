//
//  BlockWeights.cpp
//  GeoDa
//
//  Created by Xun Li on 4/7/21.
//
#include <unordered_map>
#include <set>

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
    std::unordered_map<wxInt64, std::set<int> >::iterator it;
    std::set<int>::iterator it1, it2;

    for (int i=0; i<num_vars; i++) {
        // create a dict for groups
        std::unordered_map<wxInt64, std::set<int> > cat_dict;
        const std::vector<wxInt64>& vals = cat_values[i];
        for (int j=0; j< (int)vals.size(); ++j) {
            cat_dict[ vals[j] ].insert(j);
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
        int nn = gal[i].Size();
        if (cluster.find(i) == cluster.end()) {
            if (nn == 0) {
                cluster[i] = 0;
            } else {
                // check contiguity, and separate island
                cluster[i] = c;
                cluster_ids[i] = c;
                std::vector<long> nbrs = gal[i].GetNbrs();
                for (int j=0; j<nn; ++j) {
                    cluster[ nbrs[j] ] = c;
                    cluster_ids[ nbrs[j] ] = c;
                }
            }
            c += 1;
        }
    }

    delete new_w;
}

void BlockWeights::CheckContiguity(int start, std::vector<long> nbrs, GalElement* gal)
{
    std::vector<int> group;

    while (nbrs.empty() == false) {
        int nn = (int)nbrs.size();
        for (int j=0; j<nn; ++j) {
            int nb = nbrs[j];
            if (gal[start].Check(nb) ) {

            }
        }
    }

}

BlockWeights::~BlockWeights() {

}

std::vector<wxInt64> BlockWeights::GetClusters() {
    return cluster_ids;
}



