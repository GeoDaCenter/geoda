
#include <cstddef> // for std::ptrdiff_t
#include <limits> // for std::numeric_limits<...>::infinity()
#include <algorithm> // for std::fill_n
#include <stdexcept> // for std::runtime_error
#include <string> // for std::string

#include "fastcluster.h"

using namespace fastcluster;

void fastcluster::MST_linkage_core(const t_index N, const t_float * const D,
                                   cluster_result & Z2) {
/*
    N: integer, number of data points
    D: condensed distance matrix N*(N-1)/2
    Z2: output data structure

    The basis of this algorithm is an algorithm by Rohlf:

    F. James Rohlf, Hierarchical clustering using the minimum spanning tree,
    The Computer Journal, vol. 16, 1973, p. 93–95.
*/
  t_index i;
  t_index idx2;
  doubly_linked_list active_nodes(N);
  auto_array_ptr<t_float> d(N);

  t_index prev_node;
  t_float min;

  // first iteration
  idx2 = 1;
  min = std::numeric_limits<t_float>::infinity();
  for (i=1; i<N; ++i) {
    d[i] = D[i-1];
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    if (d[i] < min) {
      min = d[i];
      idx2 = i;
    }
    else if (fc_isnan(d[i]))
      throw (nan_error());
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif
  }
  Z2.append(0, idx2, min);

  for (t_index j=1; j<N-1; ++j) {
    prev_node = idx2;
    active_nodes.remove(prev_node);

    idx2 = active_nodes.succ[0];
    min = d[idx2];
    for (i=idx2; i<prev_node; i=active_nodes.succ[i]) {
      t_float tmp = D_(i, prev_node);
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
      if (tmp < d[i])
        d[i] = tmp;
      else if (fc_isnan(tmp))
        throw (nan_error());
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif
      if (d[i] < min) {
        min = d[i];
        idx2 = i;
      }
    }
    for (; i<N; i=active_nodes.succ[i]) {
      t_float tmp = D_(prev_node, i);
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
      if (d[i] > tmp)
        d[i] = tmp;
      else if (fc_isnan(tmp))
        throw (nan_error());
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif
      if (d[i] < min) {
        min = d[i];
        idx2 = i;
      }
    }
    Z2.append(prev_node, idx2, min);
  }
}

bool fastcluster::CheckContiguity(GalElement* w, t_index idx1, t_index idx2, std::map<std::pair<t_index, t_index>, bool>  & conn_dict)
{
    // check if idx1 and idx2 are contiguity
    if (w[idx1].Check(idx2))  {
        return true;
    }
    
    // check if idx1 is a neighbor of (idx2 group)
    std::map<t_index, bool> visited;
    std::stack<t_index> vids;
    vids.push(idx2);
    
    while (vids.empty() ==  false) {
        t_index fid = vids.top();
        vids.pop();
        visited[fid] = true;
        
        const std::vector<long>& nbrs = w[fid].GetNbrs();
        for (int i=0; i<nbrs.size(); i++ ) {
            t_index nid = nbrs[i];
            if (nid != idx1 && nid != idx2  && visited[nid] == false) {
                if (conn_dict[std::make_pair(nid, idx2)]) {
                    if (w[idx1].Check(nid)) {
                        return true;
                    }
                    vids.push(nid);
                }
            }
        }
    }
    
    // check if idx2 is a neighbor of (idx1 group)
    vids.push(idx1);
    
    while (vids.empty() ==  false) {
        t_index fid = vids.top();
        vids.pop();
        visited[fid] = true;
        
        const std::vector<long>& nbrs = w[fid].GetNbrs();
        for (int i=0; i<nbrs.size(); i++ ) {
            t_index nid = nbrs[i];
            if (nid != idx2 && nid != idx1  && visited[nid] == false) {
                if (conn_dict[std::make_pair(nid, idx1)]) {
                    if (w[idx2].Check(nid)) {
                        return true;
                    }
                    vids.push(nid);
                }
            }
        }
    }
    return false;
}
