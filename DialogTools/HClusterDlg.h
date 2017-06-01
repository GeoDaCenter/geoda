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
        h = 8;
        
        rec = wxRect(x + 8, y - 5,w,h);
        color = _color;
    }
    ~RectNode() {
        
    }
    
    bool contains(const wxPoint& cur_pos) {
        
        return rec.Contains(cur_pos);
    }
    
    void draw(wxDC& dc) {
        wxBrush brush(color);
        wxPen pen(color);
        dc.SetBrush(brush);
        dc.SetPen(pen);
        dc.DrawRectangle(wxRect(x, y-2, 4, 4));
        
        dc.DrawRectangle(rec);
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
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
    DendroColorPoint(){
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
    DendrogramPanel(wxWindow* parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize);
    virtual ~DendrogramPanel();
    
    void OnIdle(wxIdleEvent& event);
    void OnEvent( wxMouseEvent& event );
    void OnSize(  wxSizeEvent& event);
    virtual void OnPaint( wxPaintEvent& event );
    void Setup(GdaNode* _root, int _nelements, int _nclusters, std::vector<wxInt64>& _clusters, double _cutoff);
    void UpdateCluster(int _nclusters, std::vector<wxInt64>& _clusters);
    void OnSplitLineChange(int x);
    
private:
    int leaves;
    int levels;
    int nelements;
    int nclusters;
    
    double margin;
    double currentY;
    double heightPerLeaf;
    double widthPerLevel;
    double maxDistance;
    double cutoffDistance;
    
    bool isResize;
    wxBitmap* layer_bm;
    
    GdaNode* root;
    
    bool isLayerValid;
    bool isLeftDown;
    bool isLeftMove;
    bool isMovingSplitLine;
    wxPoint startPos;
    
    std::map<int, int> accessed_node;
    std::map<int, double> level_node;
    std::vector<RectNode*> end_nodes;
    std::vector<wxInt64> clusters;
    std::vector<wxColour> color_vec;
    DendroSplitLine* split_line;
    
    int countLeaves(GdaNode* node);
    
    int countLevels(GdaNode* node);
    
    int countLeaves(int node_idx);
    int countLevels(int node_idx, int lvl);
    
    DendroColorPoint doDraw(wxDC &dc, int node_idx, int y);
    void init();
    
    DECLARE_ABSTRACT_CLASS(DendrogramPanel)
    DECLARE_EVENT_TABLE()
};

class HClusterDlg : public wxDialog, public FramesManagerObserver, public HighlightStateObserver
{
public:
    HClusterDlg(wxFrame *parent, Project* project);
    virtual ~HClusterDlg();
    
    void CreateControls();
    bool Init();
    
    void OnSave(wxCommandEvent& event );
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnDistanceChoice(wxCommandEvent& event);
    void OnClusterChoice(wxCommandEvent& event);
    
    void InitVariableCombobox(wxListBox* var_box);
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
    
    virtual void update(HLStateInt* o);
    
    HLStateInt*           highlight_state;
    
    void UpdateClusterChoice(int n, std::vector<wxInt64>& clusters);
    void Highlight(int id);
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
private:
    wxFrame *parent;
    Project* project;
    TableInterface* table_int;
    std::vector<wxString> tm_strs;
    
    FramesManager* frames_manager;
    
    double cutoffDistance;
    vector<wxInt64> clusters;
    vector<bool> clusters_undef;
    
    wxButton *saveButton;
    wxListBox* combo_var;
    wxChoice* combo_n;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxCheckBox* m_use_centroids;
    wxChoice* m_method;
    wxChoice* m_distance;
    DendrogramPanel* m_panel;
    wxChoice* combo_tranform;
    
    std::map<wxString, wxString> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    
    unsigned int row_lim;
    unsigned int col_lim;
    std::vector<float> scores;
    double thresh95;
    
    DECLARE_EVENT_TABLE()
};

#endif
