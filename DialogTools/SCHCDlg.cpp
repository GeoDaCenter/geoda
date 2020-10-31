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
: HClusterDlg(parent_s, project_s, false /*dont show centroids control*/)
{
    wxLogMessage("Open SCHCDlg.");
    SetTitle(_("Spatial Constrained Hierarchical Clustering Settings"));

    // disable number of cluster control
    combo_n->Disable();
    
    // bind new event
    saveButton->Bind(wxEVT_BUTTON, &SCHCDlg::OnSave, this);
}

SCHCDlg::~SCHCDlg()
{
}

void SCHCDlg::OnSave(wxCommandEvent& event )
{
    long user_select_n;
    combo_n->GetValue().ToLong(&user_select_n);
    if (user_select_n < cutoff_n_cluster) {
        wxString msg = _("The selected number of clusters is %d. It is less than the minimum number of clusters (%d) that guarantees spatially constrained results.\n\nDo you want to continue?");
        wxMessageDialog dlg(NULL, wxString::Format(msg, (int)user_select_n, cutoff_n_cluster),
                            _("Warning"), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
        if (dlg.ShowModal() == wxID_NO) {
            return;
        }
    }

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
    double* pwdist = NULL;
    if (dist == 'e') {
        pwdist = DataUtils::getPairWiseDistance(input_data, weight, rows,
                                                columns,
                                                DataUtils::EuclideanDistance);
    } else {
        pwdist = DataUtils::getPairWiseDistance(input_data, weight, rows,
                                                columns,
                                                DataUtils::ManhattanDistance);
    }

    fastcluster::auto_array_ptr<t_index> members;
    if (htree != NULL) {
        delete[] htree;
        htree = NULL;
    }
    htree = new GdaNode[rows-1];
    fastcluster::cluster_result Z2(rows-1);

    if (method == 's') {
        fastcluster::NN_chain_core_w<fastcluster::METHOD_METR_SINGLE, t_index>(gw->gal, rows, pwdist, NULL, Z2);
    } else if (method == 'w') {
        members.init(rows, 1);
        fastcluster::NN_chain_core_w<fastcluster::METHOD_METR_WARD, t_index>(gw->gal, rows, pwdist, members, Z2);
    } else if (method == 'm') {
        fastcluster::NN_chain_core_w<fastcluster::METHOD_METR_COMPLETE, t_index>(gw->gal, rows, pwdist, NULL, Z2);
    } else if (method == 'a') {
        members.init(rows, 1);
        fastcluster::NN_chain_core_w<fastcluster::METHOD_METR_AVERAGE, t_index>(gw->gal, rows, pwdist, members, Z2);
    }

    delete[] pwdist;

    //std::stable_sort(Z2[0], Z2[rows-1]);
    t_index node1, node2;
    int i=0, clst_cnt=0;
    fastcluster::union_find nodes(rows);

    n_cluster = 0;
    for (fastcluster::node const * NN=Z2[0]; NN!=Z2[rows-1]; ++NN, ++i) {
        if (NN) {
            // Find the cluster identifiers for these points.
            node1 = nodes.Find(NN->node1);
            node2 = nodes.Find(NN->node2);
            // Merge the nodes in the union-find data structure by making them
            // children of a new node.
            nodes.Union(node1, node2);
            
            node2 = node2 < rows ? node2 : rows-node2-1;
            node1 = node1 < rows ? node1 : rows-node1-1;
            
            cout << i<< ":" << node2 <<", " <<  node1 << ", " << Z2[i]->dist <<endl;
            htree[i].left = node1;
            htree[i].right = node2;

            clst_cnt += 1;
            htree[i].distance = clst_cnt;
        }
    }

    if (n_cluster == 0) n_cluster = 2;
    CutTree(rows, htree, n_cluster, clusters);

    // check if additional cluster/split is needed
    if (CheckContiguity(gw->gal, clusters)  == false) {
        n_cluster += 1;
        CutTree(rows, htree, n_cluster, clusters);
    }

    cutoff_n_cluster = n_cluster;

    combo_n->SetValue(wxString::Format("%d", n_cluster));
    combo_n->Enable();

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
