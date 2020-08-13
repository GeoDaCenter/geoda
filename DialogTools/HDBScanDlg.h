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
#include "../Algorithms/hdbscan.h"
#include "../GenColor.h"
#include "HClusterDlg.h"
#include "AbstractClusterDlg.h"

class Project;
class TableInterface;

struct TreeNode{
    int parent;
    int left;
    int right;
    double distance;
};

// base wxWidgets control of Hierachical Tree
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

    virtual bool DoDraw() = 0;
    virtual void AfterDraw(wxDC& dc) {}

    virtual void SetActive(bool flag) {
        isWindowActive = flag;
    }
    virtual void SetBlank(bool flag) {
        isLayerValid = !flag;
        Refresh();
    }

    DECLARE_ABSTRACT_CLASS(wxHTree)
    DECLARE_EVENT_TABLE()
};

// wxWidgets control of Condensed Tree in HDBScan
class wxCondensedTree : public wxHTree
{
protected:
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

    int n_nodes;
    int max_cluster_size;
    std::map<int, double> node_lambda;
    std::map<int, int> cluster_left, cluster_right, cluster_sz;
    std::map<int, double> cluster_birth;
    std::map<int, bool> cluster_isleft;
    //std::vector<int> cluster_ids;
    std::map<int, std::vector<int> > cluster_children;
    std::set<int> select_treeclusters;
    std::map<int, double> cluster_death;

    std::vector<ColorSpace::Rgb> colors;
    std::map<int, wxBrush*> cluster_brush;

    std::map<int, wxColour> cluster_colors;

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
        n_nodes = 0;
    }
    virtual ~wxCondensedTree() {}

    virtual std::set<int> GetSelectInBox() {
        if (select_box == NULL || select_box->GetWidth() <=0 || select_box->GetHeight() == 0)
            return std::set<int>();

        int x1 = select_box->GetLeft();
        int x2 = select_box->GetRight();
        int y1 = select_box->GetTop();
        int y2 = select_box->GetBottom();

        double x_left = (x1 - margin_left) / ratio_w + tree_left;
        double x_right = (x2 - margin_left) / ratio_w + tree_left;
        double y_top = (y1 - margin_top) / ratio_h + tree_t;
        double y_bottom = (y2 - margin_top) / ratio_h + tree_t;

        // check which clusters are selected
        std::vector<int> target_clusters;
        std::map<int, int>::iterator it;
        for (it = cluster_left.begin(); it != cluster_left.end(); ++it) {
            int c = it->first;
            int c_left = it->second;
            int c_right = cluster_right[c];
            double c_top = cluster_birth[c];
            double c_bottom = cluster_death[c];

            if (x_right < c_left || x_left > c_right ||
                y_top > c_bottom || y_bottom < c_top) {
                // not intersect with this cluster
                continue;
            }
            target_clusters.push_back(c);
        }

        std::set<int> select_nodes;
        // check what leaf nodes in the selecte cluster
        for (int i=0; i<target_clusters.size(); ++i) {
            int c = target_clusters[i];
            int c_left = cluster_left[c];
            int c_right = cluster_right[c];
            std::vector<int> leaves = GetLeaves(c);
            if (cluster_isleft[c]) {
                for (int j=c_left, k=0; j<c_right && k<leaves.size(); ++j, ++k) {
                    int node = leaves[k];
                    double lambda = node_lambda[node];
                    if ( j > x_left && j < x_right && lambda > y_top ) {
                        select_nodes.insert(node);
                    }
                }
            } else {
                for (int j=c_right, k=0; j> c_left && k<leaves.size(); --j, ++k) {
                    int node = leaves[k];
                    double lambda = node_lambda[node];
                    if ( j > x_left && j < x_right && lambda > y_top ) {
                        select_nodes.insert(node);
                    }
                }
            }
        }

        return select_nodes;
    }

    virtual std::vector<int> GetLeaves(int c) {
        // get subclusters of c
        std::map<int, bool> all_clusters;
        std::stack<int> clusters;
        clusters.push(c);
        while (!clusters.empty()) {
            int c = clusters.top();
            all_clusters[c] = true;
            clusters.pop();
            std::vector<int>& children = cluster_children[c];
            for (int i=0; i<children.size(); ++i) {
                clusters.push(children[i]);
            }
        }
        std::vector<int> leaves;
        // get leaf nodes
        for (int i=0; i<tree.size(); ++i) {
            int c = tree[i].parent;
            if (all_clusters.find(c) != all_clusters.end()) {
                if (tree[i].child < c) {
                    leaves.push_back(tree[i].child);
                }
            }
        }
        return leaves;
    }

    virtual ColorSpace::Rgb GetColor(int cluster_size) {
        int idx = 100 * cluster_size / (double) max_cluster_size;
        return colors[idx];
    }

    virtual void UpdateColor(bool has_noise)
    {
        int nclusters = (int)select_treeclusters.size() + has_noise;
        std::vector<wxColour> colors;
        CatClassification::PickColorSet(colors, nclusters);
        
        int i = 0;
        std::set<int>::iterator it;
        for (it = select_treeclusters.begin(); it != select_treeclusters.end(); ++it) {
            cluster_colors[*it] = colors[i + has_noise];
            i += 1;
        }
        
        setup = true; // trigger to draw
    }

    // wxCondensedTree will be drawn only after Setup()
    // the treeclusters<> are the id of clusters in hdbscan's condensed tree
    virtual void Setup(std::vector<Gda::CondensedTree*>& _tree,
               std::set<int> treeclusters)
    {
        this->select_treeclusters = treeclusters;

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
                node_lambda[child] = lambda; // leaf:lambda pair
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

        n_nodes = n;
        // setup highlight
        hs.clear();
        hl_ids.clear();
        hs.resize(n, false);
        for (int i=0; i<n; ++i) {
            hl_ids.push_back(i);
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

    virtual void SetHighlight(const std::vector<int>& ids)
    {
        hl_ids = ids;
        for (size_t i=0; i<hs.size(); ++i) hs[i] = false;
        for (size_t i=0; i<hl_ids.size(); ++i) hs[ hl_ids[i] ] = true;

        if (select_box) {
            // clean up existing select box
            delete select_box;
            select_box = 0;
        }
        DoDraw();
    }

    virtual bool DoDraw() {
        if (!setup) return false;

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
                DrawRect(dc, parent, child, start_x, start_y, end_x, end_y);
                cluster_startx[parent] = end_x;
            } else {
                // right branch (draw item from right to left)
                double start_x = cluster_startx[parent];
                double end_x = start_x - w * child_size;
                double start_y = cluster_birth[parent];
                DrawRect(dc, parent, child, start_x, start_y, end_x, end_y);
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
            if (select_treeclusters.find(c) != select_treeclusters.end()) {
                DrawSelectCluster(dc, c);
            }
        }

        // draw legend;
        DrawLegend(dc);
        DrawAxis(dc);

        // wxbitmap is ready to paint on screen
        dc.SelectObject(wxNullBitmap);
        Refresh();

        return true; // set isLayerValid
    }

    virtual void DrawSelectCluster(wxDC&dc, int c) {
        double w = cluster_sz[c];
        double x = cluster_isleft[c] ? cluster_left[c] : cluster_right[c] - w;
        double y = cluster_birth[c];
        double h = std::abs(cluster_death[c] - cluster_birth[c]);

        double w1 = w / 1.2 * 2;
        double h1 = h / 1.2 * 2;

        if (h1 == 0) h1 = tree_h / 30.0;

        x = x - (w1 - w)/2.0;
        y = y - (h1 - h)/2.0;

        int xx = (x - tree_left) * ratio_w + margin_left;
        int yy = (y - tree_t) * ratio_h + margin_top;
        int ww = w1 * ratio_w;
        int hh = h1 * ratio_h;
        
        if (cluster_colors.find(c) != cluster_colors.end()) {
            wxColour color = cluster_colors[c];
            dc.SetPen(wxPen(color));
        }
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawEllipse(xx, yy, ww, hh);
    }

    virtual void DrawRect(wxDC& dc, int c, int idx, double x1, double y1, double x2, double y2) {
        ColorSpace::Rgb rgb = GetColor(c);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(*(cluster_brush[c]));

        if (idx < n_nodes && hs[idx]) {
            dc.SetBrush(*wxRED_BRUSH);
        }

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
        dc.DrawRotatedText(legend_lbl, scw - 10, (sch + x) / 2.0, 90.0);
    }

    virtual void DrawAxis(wxDC& dc) {
        wxSize sz = this->GetClientSize();
        int sch = sz.GetHeight();

        // draw label
        wxString legend_lbl = _("lambda value");
        wxSize extent(dc.GetTextExtent(legend_lbl));
        double lbl_w = extent.GetWidth();
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

    virtual void UpdateHighlight()
    {
        // should be implemented by inherited classes
    }

    virtual void OnEvent(wxMouseEvent& event)
    {
        if (event.LeftDown()) {
            isLeftDown = true;

            startPos = event.GetPosition();
            if (select_box == 0) {
                select_box = new wxRect(startPos.x, startPos.y, 0, 0);
            } else {
                if (select_box->Contains(startPos))  {
                    // drag&move select box
                    isMovingSelectBox = true;
                } else {
                    if ( !event.ShiftDown() && !event.CmdDown() ) {
                        hl_ids.clear();
                        for (size_t i=0; i<hs.size(); ++i) hs[i] = false;
                    }
                    isMovingSelectBox = false;
                    select_box->SetPosition(startPos);
                    select_box->SetWidth(0);
                    select_box->SetHeight(0);
                    UpdateHighlight();
                }
            }
        } else if (event.Dragging()) {
            if (isLeftDown) {
                isLeftMove = true;
                // if using select box
                if (select_box != 0) {
                    if ( !event.ShiftDown() && !event.CmdDown() ) {
                        hl_ids.clear();
                        for (size_t i=0; i<hs.size(); ++i) hs[i] = false;
                    }

                    if (isMovingSelectBox) {
                        select_box->Offset(event.GetPosition() - startPos);
                    } else {
                        select_box->SetBottomRight(event.GetPosition());
                    }
                    UpdateHighlight();
                    Refresh();
                }
                startPos = event.GetPosition();
            }
        } else if (event.LeftUp()) {
            if (isLeftMove) {
                isLeftMove = false;
                // stop move
            } else {
                // only left click
                if (select_box) {
                    delete select_box;
                    select_box = 0;
                    Refresh();
                }
            }
            isMovingSelectBox = false;
            isLeftDown = false;
        }
    }
};

// wxWidgets control of Dendrogram
class wxDendrogram : public wxHTree
{
protected:
    std::vector<TreeNode> htree;
    std::map<int, double> node_elevation;
    std::map<int, double> node_position;
    std::vector<int> leaves;

    double position_min;
    double position_max;
    double elevation_min;
    double elevation_max;

    int margin_left, margin_right, margin_top, margin_bottom;
    double ratio_w;
    double ratio_h;
    int screen_w;
    int screen_h;
    
    double cutoff;
    DendroSplitLine* split_line;
    std::vector<RectNode*> end_nodes;

    bool setup;
    bool isMovingSplitLine;
    bool isDrawSplitLine;
    std::vector<wxColour> colors;
    std::vector<wxInt64> cluster_ids;
    
    std::vector<std::vector<int> > clusters;

    // for highlight
    std::vector<int> hl_ids;
    std::vector<bool> hs;

public:
    wxDendrogram(wxWindow* parent,
                 wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                 const wxSize &size=wxDefaultSize)
    : wxHTree(parent, id, pos, size), margin_left(10),
    margin_right(10), margin_top(10), margin_bottom(50),
    setup(false), cutoff(0) {
        split_line = NULL;
        isMovingSplitLine = false;
        isDrawSplitLine = false;
    }
    virtual ~wxDendrogram() {
        if (split_line) {
            delete split_line;
            split_line = NULL;
        }
        for (int i=0; i<end_nodes.size(); i++) {
            delete end_nodes[i];
        }
        end_nodes.clear();
    }
    virtual double GetCutoff() { return cutoff; }

    virtual std::vector<std::vector<int> > GetClusters()  {
        if (isDrawSplitLine) {
            int split_pos = split_line == NULL ? margin_left + 10 : split_line->getX();
            OnSplitLineChange(split_pos);
        }
        return clusters;
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

        ratio_w = screen_w / elevation_max;
        ratio_h = screen_h / position_max;
    }

    virtual void UpdateColor(std::vector<wxInt64>& _cluster_ids, int nclusters) {
        isLayerValid = false;

        this->cluster_ids = _cluster_ids;
        colors.clear();
        CatClassification::PickColorSet(colors, nclusters);

        // update color for parent nodes
        for (int i=0; i<htree.size(); ++i) {
            int left = htree[i].left;
            int right = htree[i].right;
            //int parent = htree[i].parent;
            if (cluster_ids[left] == cluster_ids[right]) {
                cluster_ids.push_back(cluster_ids[left]);
            } else {
                cluster_ids.push_back(-1);
            }
        }

        setup = true;
        isLayerValid = DoDraw();
        Refresh();
    }

    virtual bool DoDraw() {
        if (!setup) return false;

        // draw on layer_bm
        InitCanvas();

        // project data points to screen
        SetupProjection();

        // end nodes
        for (int i=0; i<end_nodes.size(); i++) {
            delete end_nodes[i];
        }
        end_nodes.clear();

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
        
        // draw axis
        DrawAxis(dc);

        // split line
        if (isDrawSplitLine) {
            DrawSplitLine();
        }
        
        // wxbitmap is ready to paint on screen
        dc.SelectObject(wxNullBitmap);
        Refresh();

        return true;
    }

    // wxDendrogram will be drawn only after Setup()
    virtual void Setup(const std::vector<TreeNode>& tree, bool use_spllit_line=false) {
        htree = tree;
        isDrawSplitLine = use_spllit_line;
        int nn = (int)htree.size();
        int n_nodes = nn + 1;

        // setup highlight
        hs.clear();
        hl_ids.clear();
        hs.resize(n_nodes, false);
        for (int i=0; i<n_nodes; ++i) {
            hl_ids.push_back(i);
        }

        // get all leaves
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
            htree[i].parent = parent_node;
            // parent node: set elevation
            node_elevation[parent_node] = d;
            node_position[parent_node] = (node_position[left] + node_position[right]) / 2.0;
            parent_node += 1;
            // update elevation_max
            if (d > elevation_max) {
                elevation_max = d;
            }
        }

        cutoff = elevation_max;
    }

    virtual void DrawLine(wxDC &dc, double x0, double y0, double x1, double y1) {
        int xx0 = margin_left + screen_w - x0 * ratio_w;
        int xx1 = margin_left + screen_w - x1 * ratio_w;
        int yy0 = margin_top + y0 * ratio_h;
        int yy1 = margin_top + y1 * ratio_h;
        dc.DrawLine(xx0, yy0, xx1, yy1);
    }

    virtual void DrawBranch(wxDC &dc, int a, int b, double dist) {
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
        
        if (!cluster_ids.empty()) {
            // draw with color (defined for each cluster)
            dc.SetPen(*wxBLACK_PEN);
            int cluster_id = (int)cluster_ids[a];
            if (cluster_id >= 0) {
                wxColor c = colors[cluster_id];
                dc.SetPen(wxPen(c));
            }
            DrawLine(dc, x0, y1, x2, y1);
            
            dc.SetPen(*wxBLACK_PEN);
            cluster_id = (int)cluster_ids[b];
            if (cluster_id >= 0) {
                wxColor c = colors[cluster_ids[b]];
                dc.SetPen(wxPen(c));
            }
            DrawLine(dc, x0, y0, x1, y0);
            
            if (cluster_ids[a] != cluster_ids[b]) {
                dc.SetPen(*wxBLACK_PEN);
            }
            DrawLine(dc, dist, y0, dist, y1);
            
        } else {
        
            dc.SetPen(*wxBLACK_PEN);
            DrawLine(dc, x0, y0, x1, y0);
            DrawLine(dc, x0, y1, x2, y1);
            DrawLine(dc, dist, y0, dist, y1);
        }
    }

    virtual void DrawNode(wxDC &dc, int node) {
        double elevation = node_elevation[node];
        if (elevation > 0) {
            return;
        }
        double position = node_position[node];
        // rectangle
        int xx = margin_left + screen_w - elevation * ratio_w;
        int yy = margin_left + position * ratio_h;

        wxColor c = cluster_ids.empty() ? *wxBLACK : colors[cluster_ids[node]];
        RectNode* end = new RectNode(node, xx - 8, yy, c);
        end->draw(dc, hs[node]);
        end_nodes.push_back(end);

        if (!cluster_ids.empty()) {
            wxColor c = colors[cluster_ids[node]];
            dc.SetBrush(wxBrush(c));
            dc.SetPen(*wxTRANSPARENT_PEN);
        }
        //dc.DrawRectangle(wxRect(xx-4, yy-2, 8, 4));
    }

    virtual void DrawAxis(wxDC& dc) {
        // elevation_max elevation_min
        int x1 = margin_left + screen_w - elevation_max * ratio_w;
        int x2 = margin_left + screen_w - elevation_min * ratio_w;
        int y = screen_h + margin_top + 10;
        
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(x1, y, x2, y);
        
        double range = elevation_max - elevation_min;
        double itv = range / 5.0;
        
        for (int i=0; i< 5; ++i) {
            double val = elevation_min + (i * itv);
            int x = margin_left + screen_w - val * ratio_w;
            int y1 = y + 10;
            dc.DrawLine(x, y1, x, y);
            wxString lbl;
            lbl << val;
            dc.DrawText(lbl, x, y1);
        }
        dc.DrawLine(x1, y+5, x1, y);
        wxString lbl;
        lbl << elevation_max;
        dc.DrawText(lbl, x1  - 2, y + 10);
        // draw label
        wxString ttl = _("Mutual Reachability Distance");
        if (clusters.size() > 0 ) {
            ttl << " (" << clusters.size() << " clusters)";
        }
        wxSize extent(dc.GetTextExtent(ttl));
        double ttl_w = extent.GetWidth();
        dc.DrawText(ttl, (screen_w -ttl_w) / 2.0 + margin_left, y + 23);
    }
        
    virtual void DrawSplitLine() {
        // split between elevation_max elevation_min
        wxSize sz = this->GetClientSize();
        int x = (elevation_max - cutoff) / elevation_max * screen_w + margin_left;
        wxPoint v_p0(x, 0);
        wxPoint v_p1(x, sz.GetHeight());
        if (split_line == NULL) {
            split_line = new DendroSplitLine(v_p0, v_p1);
        } else {
            split_line->update(v_p0, v_p1);
        }
    }
    
    virtual void NotifySelection(double cutoff, std::vector<wxInt64>& cluster_labels)
    {
        // should be implemented by inherited classes
    }

    virtual void UpdateHighlight()
    {
        // should be implemented by inherited classes
    }
    virtual void OnSplitLineChange(int x)
    {
        if (screen_w <= 0) return;

        cutoff = elevation_max * (1 - (x - margin_left) / (double)screen_w);

        if (cutoff >= elevation_max || cutoff <= 0) {
            return;
        }
        
        // get clusters at cutoff elevation
        clusters.clear();
        std::map<int, bool> visited_nodes;
        
        // process from top node
        int n = (int)htree.size();
        int n_nodes = n + 1;
        for (int i= n-1; i>=0; --i) {
            if (cutoff < htree[i].distance) {
                continue;
            }
            if (visited_nodes.find(htree[i].left) != visited_nodes.end() &&
                visited_nodes.find(htree[i].right) != visited_nodes.end()) {
                continue;
            }
            // start extracting clusters
            // get all children
            std::vector<int> cluster;
            std::stack<int> nodes;
            nodes.push(htree[i].left);
            nodes.push(htree[i].right);
            while (!nodes.empty()) {
                int cur = nodes.top();
                visited_nodes[cur] = true;
                nodes.pop();
                if (cur < n_nodes) {
                    cluster.push_back(cur);
                } else {
                    // process left and right nodes
                    int next = cur - n_nodes;
                    int left = htree[next].left;
                    int right = htree[next].right;
                    nodes.push(left);
                    nodes.push(right);
                }
            }
            clusters.push_back(cluster);
        }
        std::vector<wxInt64> cluster_labels(n_nodes, 0);
        for (int i=0; i<clusters.size(); ++i) {
            for (int j=0; j<clusters[i].size(); ++j) {
                cluster_labels[ clusters[i][j] ] = i+1;
            }
        }
        UpdateColor(cluster_labels, clusters.size() + 1);
        NotifySelection(cutoff, cluster_labels);
    }
    
    virtual void AfterDraw(wxDC& dc) {
        if (split_line) {
            split_line->draw(dc);
        }
    }
    
    virtual void OnEvent(wxMouseEvent& event)
    {
        if (event.LeftDown()) {
            isLeftDown = true;
            
            startPos = event.GetPosition();
            // test SplitLine
            if (split_line) {
                if (split_line->contains(startPos)) {
                    isMovingSplitLine = true;
                }
            }
            if (!isMovingSplitLine) {
                // test end_nodes
                if (select_box == 0) {
                    select_box = new wxRect(startPos.x, startPos.y, 0, 0);
                } else {
                    if (select_box->Contains(startPos))  {
                        // drag&move select box
                        isMovingSelectBox = true;
                    } else {
                        if ( !event.ShiftDown() && !event.CmdDown() ) {
                            hl_ids.clear();
                            for (size_t i=0; i<hs.size(); ++i) hs[i] = false;
                        }
                        isMovingSelectBox = false;
                        select_box->SetPosition(startPos);
                        select_box->SetWidth(0);
                        select_box->SetHeight(0);
                        // highlight objects
                        for (int i=0;i<end_nodes.size();i++) {
                            if (end_nodes[i]->contains(startPos)) {
                                hl_ids.push_back(end_nodes[i]->idx);
                                hs[end_nodes[i]->idx] = true;
                            }
                        }
                        UpdateHighlight();
                    }
                }
            }
        } else if (event.Dragging()) {
            if (isLeftDown) {
                isLeftMove = true;
                // moving split line
                if (isMovingSplitLine && split_line) {
                    wxPoint pt = event.GetPosition();
                    wxSize sz = GetClientSize();
                    
                    if (sz.GetWidth()> 0 && pt.x > sz.GetWidth() - 10)
                        pt.x = sz.GetWidth() - 10;
                    
                    split_line->move(pt, startPos);
                    int x = split_line->getX();
                    Refresh();
                    OnSplitLineChange(x);
                    startPos = pt;
                } else {
                    // if using select box
                    if (select_box != 0) {
                        if ( !event.ShiftDown() && !event.CmdDown() ) {
                            hl_ids.clear();
                            for (size_t i=0; i<hs.size(); ++i) hs[i] = false;
                        }
                        
                        if (isMovingSelectBox) {
                            select_box->Offset(event.GetPosition() - startPos);
                        } else {
                            select_box->SetBottomRight(event.GetPosition());
                        }
                        // highlight object
                        for (int i=0;i<end_nodes.size();i++) {
                            if (end_nodes[i]->intersects(*select_box)) {
                                hl_ids.push_back(end_nodes[i]->idx);
                                hs[end_nodes[i]->idx] = true;
                            }
                        }
                        UpdateHighlight();
                        Refresh();
                    }
                    startPos = event.GetPosition();
                }
                
            }
        } else if (event.LeftUp()) {
            if (isLeftMove) {
                isLeftMove = false;
                // stop move
                isMovingSplitLine = false;
            } else {
                // only left click
                if (select_box) {
                    delete select_box;
                    select_box = 0;
                    Refresh();
                }
            }
            isMovingSelectBox = false;
            isLeftDown = false;
        }
    }

    void SetHighlight(const std::vector<int>& ids)
    {
        hl_ids = ids;
        for (size_t i=0; i<hs.size(); ++i) hs[i] = false;
        for (size_t i=0; i<hl_ids.size(); ++i) hs[ hl_ids[i] ] = true;

        if (select_box) {
            // clean up existing select box
            delete select_box;
            select_box = 0;
        }
        DoDraw();
    }

    virtual std::vector<int> bfs_htree() {
        std::vector<int> leaves; // return
        int nn = (int)htree.size();
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


class wxHDBScanDendrogram;
class wxHDBScanCondensedTree;

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
    vector<int> hdbscan_clusters;
    vector<vector<int> > cluster_ids;

    int max_n_clusters;

    double cutoffDistance;
    vector<wxInt64> clusters;

    wxButton *saveButton;
    wxChoice* combo_n;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxChoice* m_distance;
    wxHDBScanCondensedTree* m_condensedtree;
    wxHDBScanDendrogram* m_dendrogram;
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


class wxHDBScanDendrogram : public wxDendrogram
{
public:
    wxHDBScanDendrogram(wxWindow* parent,
                       wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                       const wxSize &size=wxDefaultSize)
    : wxDendrogram(parent, id, pos, size) {

    }
    virtual ~wxHDBScanDendrogram() {}

    virtual void UpdateHighlight()
    {
        wxWindow* parent = GetParent();
        while (parent) {
            wxWindow* w = parent;
            HDBScanDlg* dlg = dynamic_cast<HDBScanDlg*>(w);
            if (dlg) {
                dlg->Highlight(hl_ids);
                break;
            }
            parent = w->GetParent();
        }
    }
};

class wxHDBScanCondensedTree : public wxCondensedTree
{
public:
    wxHDBScanCondensedTree(wxWindow* parent,
                        wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                        const wxSize &size=wxDefaultSize)
    : wxCondensedTree(parent, id, pos, size) {

    }
    virtual ~wxHDBScanCondensedTree() {}

    virtual void UpdateHighlight()
    {
        // should be implemented by inherited classes
        std::set<int> select_ids = GetSelectInBox();
        std::set<int>::iterator it;
        for (it = select_ids.begin(); it != select_ids.end(); ++it) {
            int sel_id = *it;
            hl_ids.push_back(sel_id);
            hs[sel_id] = true;
        }

        wxWindow* parent = GetParent();
        while (parent) {
            wxWindow* w = parent;
            HDBScanDlg* dlg = dynamic_cast<HDBScanDlg*>(w);
            if (dlg) {
                dlg->Highlight(hl_ids);
                break;
            }
            parent = w->GetParent();
        }
    }
};

#endif
