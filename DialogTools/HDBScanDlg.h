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
#include "../GenColor.h"

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

    int margin_left, margin_right, margin_top, margin_bottom;
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

    int max_cluster_size;
    std::map<int, int> cluster_left, cluster_right, cluster_sz;
    std::map<int, double> cluster_birth;
    std::map<int, bool> cluster_isleft;
    std::vector<int> cluster_ids;
    std::map<int, std::vector<int> > cluster_children;
    std::set<int> clusters;
    std::map<int, double> cluster_death;

    std::vector<ColorSpace::Rgb> colors;
    std::map<int, wxBrush*> cluster_brush;
    bool setup;

public:
    wxCondensedTree(wxWindow* parent,
                 wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                 const wxSize &size=wxDefaultSize)
    : wxHTree(parent, id, pos, size), setup(false) {
        margin_top = 10;
        margin_bottom = 10;
        margin_left = 100;
        margin_right = 100;
    }
    virtual ~wxCondensedTree() {}

    ColorSpace::Rgb GetColor(int cluster_size) {
        int idx = 100 * cluster_size / (double) max_cluster_size;
        return colors[idx];
    }

    // wxCondensedTree will be drawn only after Setup()
    void Setup(std::vector<Gda::CondensedTree*>& _tree,
               std::set<int> clusters) {
        this->clusters = clusters;

        // color for number of clusters
        ColorSpace::Rgb a(8,29,88), b(253,227,32); // BluYl
        colors = ColorSpace::ColorSpectrumHSV(a, b, 101); // 0-100

        // copy tree
        tree.resize(_tree.size());
        for (int i=0; i<tree.size(); ++i) {
            tree[i].parent = _tree[i]->parent;
            tree[i].child = _tree[i]->child;
            tree[i].lambda_val = _tree[i]->lambda_val;
            tree[i].child_size = _tree[i]->child_size;
            if (tree[i].lambda_val == DBL_MAX) {
                tree[i].lambda_val = 0;
            }
        }

        // get the data range of the tree: width, height, where to draw clusters
        // and how to draw clusters
        tree_t = 0;
        tree_b = 0;

        int n = 0;
        int root = tree[0].parent;

        // get max lambda value; and size of each cluster (number of items)
        for (int i=0; i<tree.size(); ++i) {
            int parent = tree[i].parent;
            int child = tree[i].child;
            int child_size = tree[i].child_size;
            double lambda = tree[i].lambda_val;
            if (tree_b < lambda ) {
                tree_b = lambda;
            }

            cluster_sz[parent] += child_size;

            if (child < parent){
                n += 1; // count how many items
            } else {
                cluster_children[parent].push_back(child); // record children
            }
            // max lambda of each cluster
            if (cluster_death.find(parent) == cluster_death.end()) {
                cluster_death[parent] = 0;;
            }
            if (cluster_death[parent] < lambda) {
                cluster_death[parent] = lambda;
            }
        }

        // process as a binary tree for clusters
        // get position for each cluster [left, right]
        cluster_left[root] = 0;
        cluster_right[root] = n;
        cluster_birth[root] = 0;
        cluster_isleft[root] = true;
        tree_left = 0;
        tree_right = n;

        for (int i=0; i<tree.size(); ++i) {
            int parent = tree[i].parent;
            int child = tree[i].child;
            double lambda = tree[i].lambda_val;

            if (child > parent) {
                // sub clusters
                if (cluster_children[parent][0] == child) {
                    // left child
                    cluster_left[child] = cluster_left[parent] - cluster_sz[parent];
                    cluster_right[child] = cluster_left[child] + cluster_sz[child];
                    if (cluster_left[child] < tree_left) {
                        tree_left = cluster_left[child];
                    }
                    if (cluster_birth[child] < lambda) {
                        cluster_birth[child] = lambda;
                    }
                    cluster_isleft[child] = true;
                } else {
                    // right child
                    cluster_right[child] = cluster_right[parent] + cluster_sz[parent];
                    cluster_left[child] = cluster_right[parent] - cluster_sz[child];
                    if (cluster_right[child] > tree_right) {
                        tree_right = cluster_right[child];
                    }
                    if (cluster_birth[child] < lambda) {
                        cluster_birth[child] = lambda;
                    }
                    cluster_isleft[child] = false;
                }
            }
        }

        tree_w = tree_right - tree_left;
        tree_h = tree_b - tree_t;

        // setup brush color
        max_cluster_size = 0;
        std::map<int, int>::iterator it;
        for (it = cluster_sz.begin(); it != cluster_sz.end(); ++it) {
            int sz = it->second;
            if (max_cluster_size < sz) {
                max_cluster_size = sz;
            }
            ColorSpace::Rgb clr = GetColor(sz);
            cluster_brush[it->first] = new wxBrush(wxColour(clr.r, clr.g, clr.b));
        }
        setup = true;
    }

    virtual void SetupProjection() {
        wxSize sz = this->GetClientSize();
        screen_w = sz.GetWidth();
        screen_h = sz.GetHeight();
        if (screen_w <= margin_left + margin_right ||
            screen_h <= margin_top + margin_bottom) {
            return;
        }
        screen_w = screen_w - margin_left - margin_right;
        screen_h = screen_h - margin_top - margin_bottom;

        ratio_w = screen_w / tree_w;
        ratio_h = screen_h / tree_h;
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

        // draw each cluster
        double w = 1;

        std::map<int, double> cluster_startx;

        for (int i=0; i<tree.size(); ++i) {
            int parent = tree[i].parent;
            int child = tree[i].child;
            int child_size = tree[i].child_size;
            double lambda = tree[i].lambda_val;

            if (cluster_startx.find(parent) == cluster_startx.end()) {
                cluster_startx[parent] = cluster_isleft[parent] ? cluster_left[parent] : cluster_right[parent];
            }

            // draw a rectangle
            double end_y = lambda;

            if (child_size == 1) {
                //continue;
            }
            if (cluster_isleft[parent]) {
                // left branch (draw item from left to right)
                double start_x = cluster_startx[parent];
                double end_x = start_x + w * child_size;
                double start_y = cluster_birth[parent];
                DrawRect(dc, parent, start_x, start_y, end_x, end_y);
                cluster_startx[parent] = end_x;
            } else {
                // right branch (draw item from right to left)
                double start_x = cluster_startx[parent];
                double end_x = start_x - w * child_size;
                double start_y = cluster_birth[parent];
                DrawRect(dc, parent, start_x, start_y, end_x, end_y);
                cluster_startx[parent] = end_x;
            }
        }

        // draw horizontal line for each cluster
        std::map<int, double>::iterator it;
        for (it = cluster_birth.begin(); it != cluster_birth.end(); ++it) {
            int c = it->first;
            double x1 = cluster_left[c] - cluster_sz[c];
            double x2 = cluster_right[c] + cluster_sz[c];

            if (!cluster_children[c].empty()) {
                // it's children's birth
                double y2 = cluster_birth[cluster_children[c][0]];
                // draw horizontal line
                DrawHLine(dc, x1, x2, y2);
            }
            // draw selected ellipse
            if (clusters.find(c) != clusters.end()) {
                DrawSelectCluster(dc, c);
            }
        }

        // draw legend;
        DrawLegend(dc);
        DrawAxis(dc);

        // wxbitmap is ready to paint on screen
        dc.SelectObject(wxNullBitmap);
        isLayerValid = true;
        Refresh();
    }

    virtual void DrawSelectCluster(wxDC&dc, int c) {
        double w = cluster_sz[c];
        double x = cluster_isleft[c] ? cluster_left[c] : cluster_right[c] - w;
        double y = cluster_birth[c];
        double h = std::abs(cluster_death[c] - cluster_birth[c]);

        double w1 = w / 1.1 * 2;
        double h1 = h / 1.1 * 2;

        if (h1 == 0) h1 = tree_h / 30.0;

        x = x - (w1 - w)/2.0;
        y = y - (h1 - h)/2.0;

        int xx = (x - tree_left) * ratio_w + margin_left;
        int yy = (y - tree_t) * ratio_h + margin_top;
        int ww = w1 * ratio_w;
        int hh = h1 * ratio_h;
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawEllipse(xx, yy, ww, hh);
    }

    virtual void DrawRect(wxDC& dc, int c, double x1, double y1, double x2, double y2) {
        ColorSpace::Rgb rgb = GetColor(c);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(*(cluster_brush[c]));

        int xx1 = (x1 - tree_left) * ratio_w + margin_left;
        int xx2 = (x2 - tree_left) * ratio_w + margin_left;
        int yy1 = (y1 - tree_t) * ratio_h + margin_top;
        int yy2 = (y2 - tree_t) * ratio_h + margin_top;

        int start_x = xx1 < xx2 ? xx1 : xx2;
        int start_y = yy1 < yy2 ? yy1 : yy2;

        int w = std::abs(xx2 - xx1);
        int h = std::abs(yy2 - yy1);

        dc.DrawRectangle(start_x, start_y, w, h);
    }

    virtual void DrawHLine(wxDC& dc, double x1, double x2, double y) {
        int xx1 = (x1 - tree_left) * ratio_w + margin_left;
        int xx2 = (x2 - tree_left) * ratio_w + margin_left;
        int yy = (y - tree_t) * ratio_h + margin_top;
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(xx1, yy, xx2, yy);
    }

    virtual void DrawLegend(wxDC& dc) {
        wxSize sz = this->GetClientSize();
        int sch = sz.GetHeight() - margin_top*4 - margin_bottom*4;
        int scw = sz.GetWidth();

        // 0-100 legend
        double legend_h = 101;
        double ratio_h = 101 / (double)sch;
        int w = 20; // legend width

        for (int i=0; i<101; ++i) {
            ColorSpace::Rgb rgb = colors[100-i];
            int x = scw - margin_right + 30;
            int y = i / ratio_h + margin_top*4;
            int h = 1 / ratio_h + 2;

            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(wxColour(rgb.r, rgb.g, rgb.b)));
            dc.DrawRectangle(x, y, w, h);

            if (i % 20 == 0) {
                int sz = max_cluster_size * (100 - i) / 100.0;
                wxString sz_lbl;
                sz_lbl << sz;
                dc.SetPen(*wxBLACK_PEN);
                dc.DrawText(sz_lbl, x + w + 5, y);
            }
        }
        wxString legend_lbl = _("Number of Points");
        wxSize extent(dc.GetTextExtent(legend_lbl));
        double x = extent.GetWidth();
        double y = extent.GetHeight();
        dc.DrawRotatedText(legend_lbl, scw - 10, (sch + x) / 2.0, 90.0);
    }

    virtual void DrawAxis(wxDC& dc) {
        wxSize sz = this->GetClientSize();
        int sch = sz.GetHeight();

        // draw label
        wxString legend_lbl = _("lambda value");
        wxSize extent(dc.GetTextExtent(legend_lbl));
        double lbl_w = extent.GetWidth();
        double lbl_h = extent.GetHeight();
        dc.DrawRotatedText(legend_lbl, 5, (sch + lbl_w) / 2.0, 90.0);

        // draw a verticle line
        int xpad = 40;
        int x = margin_left - xpad;
        int y0 = margin_top;
        int y1 = sch - margin_bottom;

        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(x, y0, x, y1);

        // from tree_top to tree_bottom
        double range = tree_b - tree_t;
        double itv = range / 5.0;

        for (int i=0; i<5; i++) {
            double lambda = itv * i;

            wxString str_lambda;
            str_lambda << lambda;

            wxSize extent(dc.GetTextExtent(str_lambda));
            int str_w = extent.GetWidth();

            int x1 = margin_left - xpad - str_w - 10;
            int y1 = (lambda - tree_t) * ratio_h + margin_top;

            dc.DrawText(str_lambda, x1, y1);
            dc.DrawLine(margin_left -xpad -5, y1, margin_left-xpad, y1);
        }

        // last one
        wxString str_lambda;
        str_lambda << tree_b;
        wxSize extent1(dc.GetTextExtent(str_lambda));
        int str_w = extent1.GetWidth();
        int x1 = margin_left - xpad - str_w - 10;
        y1 = (tree_b - tree_t) * ratio_h + margin_top;

        dc.DrawText(str_lambda, x1, y1);
        dc.DrawLine(margin_left -xpad -5, y1, margin_left-xpad, y1);
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
