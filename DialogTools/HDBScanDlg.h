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

struct GdaNode;
class Project;
class TableInterface;
class CondensedTree;

// wxWidgets control of Condensed Tree in HDBScan
class wxCondensedTree : public wxPanel
{
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

    std::vector<CondensedTree*> condensed_tree;

public:
    wxCondensedTree(wxWindow* parent,
                    //const std::vector<CondensedTree*>& condensed_tree,
                    wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                    const wxSize &size=wxDefaultSize);

    void OnIdle(wxIdleEvent& event);
    void OnEvent(wxMouseEvent& event );
    void OnSize(wxSizeEvent& event);
    virtual void OnPaint(wxPaintEvent& event);

    DECLARE_ABSTRACT_CLASS(wxCondensedTree)
    DECLARE_EVENT_TABLE()
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
    wxCondensedTree* m_panel;
    DendrogramPanel* m_dendrogram;
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
