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

#ifndef __GEODA_CENTER_HCLUSTER_DLG_H___
#define __GEODA_CENTER_HCLUSTER_DLG_H___

#include <vector>
#include <map>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "../logger.h"
#include "AbstractClusterDlg.h"

struct GdaNode;
class Project;
class TableInterface;

class RectNode
{
public:
    RectNode(int _idx, int _x, int _y, const wxColour& _color) {
        idx = _idx;
        x = _x;
        y = _y;
        
        w = 20;
        h = 4;
        
        rec = wxRect(x + 8, y - 2,w,h);
        color = _color;
    }
    ~RectNode() {
        
    }
    
    bool contains(const wxPoint& cur_pos) {
        
        return rec.Contains(cur_pos);
    }
    
    bool intersects(const wxRect& rect) {
        int x_left = rect.width >= 0 ? rect.x : rect.x + rect.width;
        int x_right = rect.width >= 0 ? rect.x + rect.width : rect.x;
        int y_top = rect.height >= 0 ? rect.y : rect.y + rect.height;
        int y_bottom = rect.height >= 0 ? rect.y + rect.height : rect.y;
        
        return !(rec.x + rec.width < x_left || rec.x > x_right ||
                 rec.y > y_bottom || rec.y + rec.height < y_top);
        //return rec.Intersects(rect);
    }
    
    void draw(wxDC& dc, bool is_hl=false) {
        wxBrush brush(color);
        wxPen pen(color);
        dc.SetBrush(brush);
        dc.SetPen(pen);
        //dc.DrawRectangle(wxRect(x, y-2, 4, 4));
                
        if (is_hl)
            dc.SetPen(*wxRED);
        dc.DrawRectangle(rec);
    }
    
    int idx;
private:
    
    int x;
    int y;
    int w;
    int h;
    
    wxRect rec;
    wxColour color;
};

class DendroSplitLine
{
public:
    DendroSplitLine(const wxPoint& _p0, const wxPoint& _p1){
        pt0 = _p0;
        pt1 = _p1;
        color = *wxRED;
        
        //int padding = 5;
        
        rec = wxRect(pt0.x -5, pt0.y, 10, pt1.y - pt0.y);
    }
    ~DendroSplitLine(){
        
    }
    
    void update(const wxPoint& _p0, const wxPoint& _p1){
        pt0 = _p0;
        pt1 = _p1;
        
        rec = wxRect(pt0.x -5, pt0.y, 10, pt1.y - pt0.y);
    }
    
    bool contains(const wxPoint& cur_pos) {
        
        return rec.Contains(cur_pos);
    }
    
    void move(const wxPoint& cur_pos, const wxPoint& old_pos) {
        int offset_x = cur_pos.x - old_pos.x;

        pt0.x += offset_x;
        pt1.x += offset_x;
        
        if (pt0.x < 5) pt0.x = 5;
        if (pt1.x < 5) pt1.x = 5;
        
        rec = wxRect(pt0.x -5, pt0.y, 10, pt1.y - pt0.y);
    }
    
    void draw(wxDC& dc) {
        dc.SetPen(wxPen(color, 2, wxPENSTYLE_SHORT_DASH));
        dc.DrawLine(pt0, pt1);
    }
    
    int getX() {
        return pt0.x;
    }
    
    wxPoint pt0;
    wxPoint pt1;
    wxColour color;
    wxRect rec;
};

class DendroColorPoint
{
public:
    DendroColorPoint() {
        color = *wxBLACK;
        is_valid = false;
    }
    DendroColorPoint(const wxPoint& _pt, const wxColour& _clr){
        pt = _pt;
        color = _clr;
        is_valid = true;
    }
    ~DendroColorPoint() {
        
    }

    bool IsValid() {
        return is_valid;
    }
    
    int level;
    wxPoint pt;
    wxColour color;
    bool is_valid;
};

class DendrogramPanel : public wxPanel
{
public:
    //DendrogramPanel();
    DendrogramPanel(int max_n_clusters, wxWindow* parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize);
    virtual ~DendrogramPanel();
    
    void OnIdle(wxIdleEvent& event);
    void OnEvent( wxMouseEvent& event );
    void OnSize(  wxSizeEvent& event);
    virtual void OnPaint( wxPaintEvent& event );
    void Setup(GdaNode* _root, int _nelements, int _nclusters, std::vector<wxInt64>& _clusters, double _cutoff);
    void UpdateCluster(int _nclusters, std::vector<wxInt64>& _clusters);
    void OnSplitLineChange(int x);
    
    void SetActive(bool flag);
    void SetHighlight(const std::vector<int>& ids);
    
private:
    bool isWindowActive;
    bool isMovingSelectBox;
    int leaves;
    int levels;
    int nelements;
    int nclusters;
   
    int max_n_clusters;
    
    double margin;
    double currentY;
    double heightPerLeaf;
    double widthPerLevel;
    double maxDistance;
    double minDistance;
    double cutoffDistance;
    
    bool isResize;
    wxBitmap* layer_bm;
    
    GdaNode* root;
    
    bool isLayerValid;
    bool isLeftDown;
    bool isLeftMove;
    bool isMovingSplitLine;
    wxPoint startPos;
    std::vector<int> hl_ids;
    std::vector<bool> hs;
    
    std::map<int, int> accessed_node;
    std::map<int, double> level_node;
    std::vector<RectNode*> end_nodes;
    std::vector<wxInt64> clusters;
    std::vector<wxColour> color_vec;
    DendroSplitLine* split_line;
    wxRect* select_box;
    
    int countLeaves(GdaNode* node);
    
    int countLevels(GdaNode* node);
    
    int countLeaves(int node_idx);
    int countLevels(int node_idx, int lvl);
    
    DendroColorPoint doDraw(wxDC &dc, int node_idx, int y);
    void init();
    void NotifySelection();
    
    DECLARE_ABSTRACT_CLASS(DendrogramPanel)
    DECLARE_EVENT_TABLE()
};

class HClusterDlg : public AbstractClusterDlg, public HighlightStateObserver
{
public:
    HClusterDlg(wxFrame *parent, Project* project, bool show_centroids=true, bool show_weights=false);
    virtual ~HClusterDlg();
    
    void CreateControls();
    virtual bool Init();
    
    void OnSave(wxCommandEvent& event );
    void OnOKClick( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnClusterChoice(wxCommandEvent& event);
    void OnNotebookChange(wxBookCtrlEvent& event);
    void InitVariableCombobox(wxListBox* var_box);
    void OnMethodChoice(wxCommandEvent& event);
    
    virtual void update(HLStateInt* o);
    
    virtual wxString _printConfiguration();
    
    HLStateInt* highlight_state;
    
    void UpdateClusterChoice(int n, std::vector<wxInt64>& clusters);
    void Highlight(int id);
    void Highlight(std::vector<int>& ids);
    
protected:
    virtual bool Run(std::vector<wxInt64>& clusters);
    virtual bool CheckAllInputs();

    GdaNode* htree;
    int n_cluster;
    char dist;
    char method;

    double cutoffDistance;
    std::vector<wxInt64> clusters;
    bool show_centroids;
    bool show_weights;
    
    wxButton *saveButton;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxChoice* m_method;
    wxChoice* m_distance;
    DendrogramPanel* m_panel;
    wxNotebook* notebook;
    wxStaticText* m_sctxt;
    
    DECLARE_EVENT_TABLE()
};

#endif
