/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <map>

#include <wx/wx.h>
#include <boost/unordered_map.hpp>

#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/fastcluster.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightUtils.h"

#include "../Algorithms/redcap.h"
#include "SaveToTableDlg.h"
#include "SCHCDlg.h"

BEGIN_EVENT_TABLE( SCHCDlg, wxDialog )
EVT_CLOSE( SCHCDlg::OnClose )
END_EVENT_TABLE()

SCHCDlg::SCHCDlg(wxFrame* parent_s, Project* project_s)
: HClusterDlg(parent_s, project_s, false /*dont show centroids control*/), redcap(0)
{
    wxLogMessage("Open SCHCDlg.");
    SetTitle(_("Spatial Constrained Hierarchical Clustering Settings"));
    
    // bind new event
    saveButton->Bind(wxEVT_BUTTON, &SCHCDlg::OnSave, this);
}

SCHCDlg::~SCHCDlg()
{
    wxLogMessage("On SCHCDlg::~SCHCDlg");
    //frames_manager->removeObserver(this);
    if (redcap) {
        delete redcap;
        redcap = NULL;
    }
}

void SCHCDlg::OnSave(wxCommandEvent& event )
{
    HClusterDlg::OnSave(event);

    // check cluster connectivity
    GalWeight* gw = CheckSpatialWeights();
    if (gw == NULL) return ;

    if (CheckContiguity(gw->gal, clusters)  == false) {
        wxString err_msg = _("The clustering result is not spatially constrained. Please adjust the number of clusters.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }
}

bool SCHCDlg::Run(vector<wxInt64>& clusters)
{
    // NOTE input_data should be retrieved first!!
    // get input: weights (auto)
    weight = GetWeights(columns);

    // Check weights
    GalWeight* gw = CheckSpatialWeights();
    if (gw == NULL) {
        return false;
    }
    // Check connectivity
    if (!CheckConnectivity(gw)) {
        wxString msg = _("The connectivity of selected spatial weights is incomplete, please adjust the spatial weights.");
        wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
        dlg.ShowModal();
        return false;
    }

    // get pairwise distance
    int transpose = 0; // row wise
    double** ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
    double** distances = DataUtils::fullRaggedMatrix(ragged_distances, rows, rows);
    for (int i = 1; i < rows; i++) free(ragged_distances[i]);
    free(ragged_distances);

    // run RedCap
      std::vector<bool> undefs(rows, false);
    
      if (redcap != NULL) {
          delete redcap;
          redcap = NULL;
      }
    double* bound_vals = 0;
    double min_bound = 0;
      if (method == 's') {
          redcap = new SpanningTreeClustering::FullOrderSLKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
      } else if (method == 'w') {
          redcap = new SpanningTreeClustering::FullOrderWardRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
      } else if (method == 'm') {
          redcap = new SpanningTreeClustering::FullOrderCLKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
      } else if (method == 'a') {
          redcap = new SpanningTreeClustering::FullOrderALKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
      }
     
      if (redcap==NULL) {
          for (int i = 1; i < rows; i++) delete[] distances[i];
          delete[] distances;
          return false;
      }
    
    redcap->ordered_edges;
    
    if (htree != NULL) {
        delete[] htree;
        htree = NULL;
    }
    htree = new GdaNode[rows-1];

    //std::stable_sort(Z2[0], Z2[rows-1]);
    t_index node1, node2;
    fastcluster::union_find nodes(rows);
    int cluster_idx = 1;
    
    for (int i=0; i<redcap->ordered_edges.size(); ++i) {
        SpanningTreeClustering::Edge* e = redcap->ordered_edges[i];
        if (e) {
            // Find the cluster identifiers for these points.
            node1 = nodes.Find(e->orig->id);
            node2 = nodes.Find(e->dest->id);
                        
            // Merge the nodes in the union-find data structure by making them
            // children of a new node.
            nodes.Union(node1, node2);
            
            node2 = node2 < rows ? node2 : rows-node2-1;
            node1 = node1 < rows ? node1 : rows-node1-1;
            
            htree[i].left = node1;
            htree[i].right = node2;
            htree[i].distance = cluster_idx;
            cluster_idx += 1;
        }
    }
    
    clusters.clear();
    int* clusterid = new int[rows];
    cutoffDistance = cuttree (rows, htree, n_cluster, clusterid);
    for (int i=0; i<rows; i++) {
        clusters.push_back(clusterid[i]+1);
    }
    delete[] clusterid;
    clusterid = NULL;

    // sort result
    std::vector<std::vector<int> > cluster_ids(n_cluster);
    
    for (int i=0; i < clusters.size(); i++) {
        cluster_ids[ clusters[i] - 1 ].push_back(i);
    }
    
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < n_cluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    
    return true;
}

void SCHCDlg::CutTree(int rows, GdaNode* htree, int n_cluster, std::vector<wxInt64>& clusters)
{
    clusters.clear();
    int* clusterid = new int[rows];
    cutoffDistance = cuttree (rows, htree, n_cluster, clusterid);
    for (int i=0; i<rows; i++) {
        clusters.push_back(clusterid[i]+1);
    }
    delete[] clusterid;
}
