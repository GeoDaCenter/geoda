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

#ifndef __GEODA_CENTER_HDBSCAN_DLG_H___
#define __GEODA_CENTER_HDBSCAN_DLG_H___

#include <vector>
#include <map>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "../logger.h"
#include "HClusterDlg.h"
#include "AbstractClusterDlg.h"
#include "../Algorithms/hdbscan.h"

class Project;
class TableInterface;

struct TreeNode{
    int id;
    int left;
    int right;
    double distance;
};

// wxWidgets control of Condensed Tree in HDBScan
class wxHTree : public wxPanel
{
protected:
    bool isLeftDown;
    bool isLeftMove;
    bool isMovingSelectBox;
    bool isLayerValid;
    bool isWindowActive;
    bool isResize;
    wxPoint startPos;
    std::vector<int> hl_ids;
    std::vector<bool> hs;
    wxRect* select_box;
    wxBitmap* layer_bm;

public:
    wxHTree(wxWindow* parent,
            wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
            const wxSize &size=wxDefaultSize);
    virtual ~wxHTree();

    virtual void OnIdle(wxIdleEvent& event);
    virtual void OnEvent(wxMouseEvent& event );
    virtual void OnSize(wxSizeEvent& event);
    virtual void OnPaint(wxPaintEvent& event);

    virtual void InitCanvas();

    virtual void DoDraw() = 0;

    DECLARE_ABSTRACT_CLASS(wxHTree)
    DECLARE_EVENT_TABLE()
};

class wxCondensedTree : public wxHTree
{
    std::vector<Gda::CondensedTree> tree;

    int margin;
    double ratio_w;
    double ratio_h;
    int screen_w;
    int screen_h;

    double tree_left;
    double tree_right;
    double tree_h;
    double tree_w;
    double tree_t;
    double tree_b;
    std::map<int, int> cluster_left, cluster_right, cluster_sz;

    bool setup;
public:
    wxCondensedTree(wxWindow* parent,
                 wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                 const wxSize &size=wxDefaultSize)
    : wxHTree(parent, id, pos, size), margin(10), setup(false) {}
    virtual ~wxCondensedTree() {}

    // wxCondensedTree will be drawn only after Setup()
    void Setup(std::vector<Gda::CondensedTree*>& _tree) {
        // copy tree
        tree.resize(_tree.size());
        for (int i=0; i<tree.size(); ++i) {
            tree[i].parent = _tree[i]->parent;
            tree[i].child = _tree[i]->child;
            tree[i].lambda_val = _tree[i]->lambda_val;
            tree[i].child_size = _tree[i]->child_size;
        }

        // get the data range of the tree: width, height, where to draw clusters
        // and how to draw clusters
        tree_t = 0;
        tree_b = 0;


        int n = 0;
        int root = tree[0].parent;

        // get max lambda value; and size of each cluster (number of items)
        for (int i=0; i<tree.size(); ++i) {
            if (tree_b < tree[i].lambda_val) {
                tree_b = tree[i].lambda_val;
            }
            int parent = tree[i].parent;
            int child = tree[i].child;
            int child_size = tree[i].child_size;
            if (parent >= root) {
                cluster_sz[parent] += child_size;
            }
            if (child < parent){
                n += 1; // count how many items
            }
        }

        // process as a binary tree for clusters
        // get position for each cluster [left, right]
        cluster_left[root] = 0;
        cluster_right[root] = n;
        tree_left = 0;
        tree_right = n;

        int num_child = 0;
        for (int i=0; i<tree.size(); ++i) {
            int parent = tree[i].parent;
            int child = tree[i].child;

            if (child > parent) {
                // sub clusters
                if (num_child == 0) {
                    // left child
                    cluster_left[child] = cluster_left[parent] - cluster_sz[parent];
                    cluster_right[child] = cluster_left[child] + cluster_sz[child];
                    if (cluster_left[child] < tree_left) {
                        tree_left = cluster_left[child];
                    }
                    num_child += 1;
                } else if (num_child == 1) {
                    // right child
                    cluster_right[child] = cluster_right[parent] + cluster_sz[parent];
                    cluster_left[child] = cluster_right[parent] - cluster_sz[child];
                    if (cluster_right[child] > tree_right) {
                        tree_right = cluster_right[child];
                    }
                    num_child = 0;
                }
            }
        }

        tree_w = tree_right - tree_left;
        tree_h = tree_b - tree_t;
    }

    virtual void DoDraw() {
        if (!setup) return;

        // draw on layer_bm
        InitCanvas();

        // project data points to screen
        //SetupProjection();

        // draw everything on a wxbitmap
        wxMemoryDC dc;
        dc.SelectObject(*layer_bm);
        dc.Clear();
        dc.SetFont(*GdaConst::extra_small_font);

        double start_y = 0;
        int root = tree[0].parent;

        for (int i=0; i<tree.size(); ++i) {
            int parent = tree[i].parent;
            int child = tree[i].child;
            int child_size = tree[i].child_size;
            double lambda = tree[i].lambda_val;

            // draw a rectangle
            int start_x = cluster_left[parent];
            double w = 1;
            double h = lambda;
            //dc
        }

        // wxbitmap is ready to paint on screen
        dc.SelectObject(wxNullBitmap);
        isLayerValid = true;
        Refresh();
    }
};

class wxDendrogram : public wxHTree
{
    std::vector<TreeNode> htree;
    std::map<int, double> node_elevation;
    std::map<int, double> node_position;
    std::vector<int> leaves;

    double position_min;
    double position_max;
    double elevation_min;
    double elevation_max;

    int margin;
    double ratio_w;
    double ratio_h;
    int screen_w;
    int screen_h;

    bool setup;
public:
    wxDendrogram(wxWindow* parent,
                 wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                 const wxSize &size=wxDefaultSize)
    : wxHTree(parent, id, pos, size), margin(10), setup(false) {}
    virtual ~wxDendrogram() {}

    virtual void SetupProjection() {
        wxSize sz = this->GetClientSize();
        screen_w = sz.GetWidth();
        screen_h = sz.GetHeight();
        if (screen_w <= 2*margin || screen_h <= 2*margin) {
            return;
        }
        screen_w = screen_w - 2 * margin;
        screen_h = screen_h - 2 * margin;

        ratio_w = screen_w / elevation_max;
        ratio_h = screen_h / position_max;
    }

    virtual void DoDraw() {
        if (!setup) return;

        // draw on layer_bm
        InitCanvas();

        // project data points to screen
        SetupProjection();

        // draw everything on a wxbitmap
        wxMemoryDC dc;
        dc.SelectObject(*layer_bm);
        dc.Clear();
        dc.SetFont(*GdaConst::extra_small_font);

        for (int i=0; i<htree.size(); ++i) {
            int left = htree[i].left;
            int right = htree[i].right;
            double d = htree[i].distance;
            DrawBranch(dc, left, right, d);
        }

        for (int i=0; i<leaves.size(); ++i) {
            DrawNode(dc, leaves[i]);
        }

        // wxbitmap is ready to paint on screen
        dc.SelectObject(wxNullBitmap);
        isLayerValid = true;
        Refresh();
    }

    // wxDendrogram will be drawn only after Setup()
    void Setup(const std::vector<TreeNode>& tree) {
        htree = tree;

        int nn = htree.size();
        int n_nodes = nn + 1;

        leaves = bfs_htree();

        // get position for all nodes (project nodes to same level)
        for (int i=0; i<n_nodes; ++i) {
            node_position[ leaves[i] ] = i;
        }
        position_min = 0;
        position_max = nn;

        // get elevation for all nodes in tree
        for (int i=0; i<n_nodes; ++i) {
            node_elevation[i] = 0;
        }
        elevation_min = 0;
        elevation_max = 0;

        int parent_node = n_nodes;
        for (int i=0; i<nn; ++i) {
            int left = htree[i].left;
            int right = htree[i].right;
            double d = htree[i].distance;
            // parent node: set elevation
            node_elevation[parent_node] = d;
            node_position[parent_node] = (node_position[left] + node_position[right]) / 2.0;
            parent_node += 1;
            // update elevation_max
            if (d > elevation_max) {
                elevation_max = d;
            }
        }
        setup = true;
    }

    void DrawLine(wxDC &dc, double x0, double y0, double x1, double y1) {
        int xx0 = margin + screen_w - x0 * ratio_w;
        int xx1 = margin + screen_w - x1 * ratio_w;
        int yy0 = margin + y0 * ratio_h;
        int yy1 = margin + y1 * ratio_h;
        dc.DrawLine(xx0, yy0, xx1, yy1);
    }

    void DrawBranch(wxDC &dc, int a, int b, double dist) {
        //   ----------- b (node_position[b])
        //  |
        //  |
        //   ---- a        (node_position[a])
        //
        //  |<---dist------------
        //               |<---elevation[b]
        //        |<---elevation[a]
        double x0 = dist, x1 = node_elevation[b], x2 = node_elevation[a];
        double y0 = node_position[b], y1 = node_position[a];
        DrawLine(dc, x0, y0, x1, y0);
        DrawLine(dc, x0, y1, x2, y1);
        DrawLine(dc, dist, y0, dist, y1);
    }

    void DrawNode(wxDC &dc, int node) {
        double elevation = node_elevation[node];
        if (elevation > 0) {
            return;
        }
        double position = node_position[node];
        // rectangle
        int xx = margin + screen_w - elevation * ratio_w;
        int yy = margin + position * ratio_h;
        dc.DrawRectangle(wxRect(xx-4, yy-2, 8, 4));
    }

    std::vector<int> bfs_htree() {
        std::vector<int> leaves; // return
        int nn = htree.size();
        int n_nodes = nn + 1;
        std::stack<int> nodes;
        // process top node
        int top = nn - 1;
        nodes.push(htree[top].left);
        nodes.push(htree[top].right);

        while (!nodes.empty()) {
            int cur = nodes.top();
            nodes.pop();

            if (cur < n_nodes) {
                // leaf node
                leaves.push_back(cur);
            } else {
                // process left and right nodes
                int next = cur - n_nodes;
                int left = htree[next].left;
                int right = htree[next].right;
                nodes.push(left);
                nodes.push(right);
            }
        }
        return leaves;
    }
};



// HDBScan Dialog
class HDBScanDlg : public AbstractClusterDlg, public HighlightStateObserver
{
    char     dist;
    int      m_min_pts;
    int      m_min_samples;
    double   m_alpha;
    int      m_cluster_selection_method;
    bool     m_allow_single_cluster;

    vector<double> core_dist;
    vector<double> probabilities;
    vector<double> outliers;
    vector<vector<int> > cluster_ids;

    int max_n_clusters;

    double cutoffDistance;
    vector<wxInt64> clusters;
    GdaNode* htree;

    wxButton *saveButton;
    wxChoice* combo_n;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxChoice* m_distance;
    wxCondensedTree* m_condensedtree;
    wxDendrogram* m_dendrogram;
    wxTextCtrl* m_minpts;
    wxTextCtrl* m_minsamples;
    wxTextCtrl* m_ctl_alpha;
    wxTextCtrl* m_cluster;
    wxNotebook* notebook;
    wxChoice* m_select_method;
    wxCheckBox* chk_allowsinglecluster;

public:
    HDBScanDlg(wxFrame *parent, Project* project);
    virtual ~HDBScanDlg();
    
    void CreateControls();
    virtual bool Init();
    
    void OnSave(wxCommandEvent& event );
    void OnOKClick( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnClusterChoice(wxCommandEvent& event);
    void OnNotebookChange(wxBookCtrlEvent& event);
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual void update(HLStateInt* o);
    
    virtual wxString _printConfiguration();
    
    HLStateInt* highlight_state;
    
    void UpdateClusterChoice(int n, std::vector<wxInt64>& clusters);
    void Highlight(int id);
    void Highlight(vector<int>& ids);

protected:
    virtual bool Run(vector<wxInt64>& clusters);
    virtual bool CheckAllInputs();

    DECLARE_EVENT_TABLE()
};

#endif
