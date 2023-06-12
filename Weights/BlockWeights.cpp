//
//  BlockWeights.cpp
//  GeoDa
//
//  Created by Xun Li on 4/7/21.
//
#include <set>
#include <stack>

#include "../GenUtils.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/GeodaWeight.h"
#include "../ShapeOperations/WeightUtils.h"
#include "BlockWeights.h"

BlockWeights::BlockWeights() {}

// cat_values are categorical values of multiple variables. Each variable
// represents how observations are grouped into regions, e.g. 1,1,2,2,3,4,4
// means the first two observations belong to group 1, the 3rd and 4th
// observations belong to group 2 etc.
//
// Each variable can be used to create a contiguity weights.
// The final block weights are intersection of the contiguity weights.
BlockWeights::BlockWeights(const std::vector<std::vector<wxInt64>> &cat_values,
                           const GeoDaWeight *cont_weights) {
  num_vars = (int)cat_values.size();
  num_obs = (int)cat_values[0].size();

  std::vector<GeoDaWeight *> ws;
  boost::unordered_map<wxInt64, std::set<int>>::iterator it;
  std::set<int>::iterator it1, it2;

  for (int i = 0; i < num_vars; i++) {
    // create a dict for groups
    boost::unordered_map<wxInt64, std::set<int>> cat_dict;
    const std::vector<wxInt64> &vals = cat_values[i];
    for (int j = 0; j < (int)vals.size(); ++j) {
      if (vals[j] >= 0) { // avoid ignored observations value=-1
        cat_dict[vals[j]].insert(j);
      }
    }

    // create block weights for this variable
    GalElement *gal = new GalElement[num_obs];
    for (it = cat_dict.begin(); it != cat_dict.end(); ++it) {
      std::set<int> &ids = it->second;
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
    GalWeight *w = new GalWeight();
    w->num_obs = num_obs;
    w->gal = gal;
    ws.push_back(w);
  }

  GalWeight *new_w = WeightUtils::WeightsIntersection(ws);
  for (int i = 0; i < (int)ws.size(); ++i) {
    delete ws[i];
  }

  // create cluster map from new weights
  GalElement *gal = new_w->gal;
  int c = 1;
  cluster_ids.resize(num_obs, 0);

  for (int i = 0; i < num_obs; i++) {
    int nn = (int)gal[i].Size();
    if (cluster.find(i) == cluster.end()) {
      if (nn == 0) {
        cluster[i] = 0;
      } else {
        // check contiguity, and separate islands
        std::vector<long> nbrs = gal[i].GetNbrs();
        cluster[i] = c;
        cluster_ids[i] = c;
        for (int j = 0; j < nn; ++j) {
          cluster[(int)nbrs[j]] = c;
          cluster_ids[nbrs[j]] = c;
        }
        c += 1;
      }
    }
  }

  delete new_w;
}

std::vector<std::vector<int>>
BlockWeights::CheckContiguity(int start, std::vector<long> nbrs,
                              const GeoDaWeight *w) {
  // check contiguity, and separate islands into groups
  std::vector<std::vector<int>> groups;

  boost::unordered_map<int, bool> cluster_dict;
  for (int i = 0; i < (int)nbrs.size(); ++i) {
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
      for (int i = 0; i < (int)nn.size(); ++i) {
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

BlockWeights::~BlockWeights() {}

std::vector<wxInt64> BlockWeights::GetClusters(int min_cluster_size) {
  // filter by min_cluster_size
  std::vector<std::vector<int>> clusters;
  std::map<wxInt64, std::vector<int>> solution;
  for (int i = 0; i < (int)cluster_ids.size(); ++i) {
    solution[cluster_ids[i]].push_back(i);
  }
  std::map<wxInt64, std::vector<int>>::iterator it;
  for (it = solution.begin(); it != solution.end(); ++it) {
    clusters.push_back(it->second);
  }

  // sort the clusters
  std::sort(clusters.begin(), clusters.end(), GenUtils::less_vectors);

  int nclusters = (int)clusters.size();
  for (int i = 0; i < nclusters; i++) {
    int cluster_sz = (int)clusters[i].size();
    for (int j = 0; j < cluster_sz; j++) {
      int idx = clusters[i][j];
      cluster_ids[idx] = cluster_sz >= min_cluster_size ? i : 0;
    }
  }

  return cluster_ids;
}

// Make unique values map for the classifications that overlap but use the
// color from the original cluster map of the target cluster
std::vector<wxInt64> BlockWeights::GetClustersBasedOnCategories(
    const std::vector<wxInt64> &cat_values) {
  // result array intialized with 0 (not clustered)
  std::vector<wxInt64> result(cat_values.size(), 0);

  std::map<wxInt64, std::vector<size_t>> origin_clusters;
  std::map<wxInt64, std::vector<size_t>>::iterator it;

  for (size_t i = 0; i < cat_values.size(); ++i) {
    origin_clusters[cat_values[i]].push_back(i);
  }

  // for each group from the original cluster map, iterate items in this group
  // check the overlaps and keep the largest overlapped area as the final
  // intersection (common cluster) with the target cluster map
  for (it = origin_clusters.begin(); it != origin_clusters.end(); ++it) {
    wxInt64 origin_cid = it->first;
    const std::vector<size_t> &row_ids = it->second;
    // find largest overlapped area and keep it
    std::map<wxInt64, size_t> overlapped_area;
    for (size_t i = 0; i < row_ids.size(); ++i) {
      size_t row = row_ids[i];
      wxInt64 c = cluster_ids[row];
      overlapped_area[c] += 1;
    }
    std::map<wxInt64, size_t>::iterator largest
              = std::max_element(overlapped_area.begin(),overlapped_area.end(),[] (const std::pair<wxInt64, size_t>& a, const std::pair<wxInt64, size_t>& b)->bool{ return a.second < b.second; } );
    wxInt64 c = largest->first;
    for (size_t i = 0; i < row_ids.size(); ++i) {
        size_t row = row_ids[i];
      if (c == cluster_ids[row]) {
        result[row] = origin_cid;
      }
    }
  }
  return result;
}
