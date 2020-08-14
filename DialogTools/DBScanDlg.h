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

#ifndef __GEODA_CENTER_DBSCAN_DLG_H___
#define __GEODA_CENTER_DBSCAN_DLG_H___

#include <vector>
#include <map>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "../logger.h"
#include "HClusterDlg.h"
#include "AbstractClusterDlg.h"
#include "HDBScanDlg.h"
#include "../Algorithms/dbscan.h"

struct GdaNode;
class Project;
class TableInterface;
class wxDBScanDendrogram;

class DBScanDlg : public AbstractClusterDlg, public HighlightStateObserver
{
public:
    DBScanDlg(wxFrame *parent, Project* project);
    virtual ~DBScanDlg();
    
    void CreateControls();
    virtual bool Init();
    
    void OnOKClick( wxCommandEvent& event );
    void OnSaveClick( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnEpsInput(wxCommandEvent& ev);
    void OnClusterChoice(wxCommandEvent& event);
    void OnSelectVars(wxCommandEvent& event);
    void OnNotebookChange(wxBookCtrlEvent& event);
    void InitVariableCombobox(wxListBox* var_box);
    void OnDBscanCheck(wxCommandEvent& event);
    void OnDBscanStarCheck(wxCommandEvent& event);
    void UpdateFromDendrogram(double cutoff, std::vector<wxInt64>& cluster_labels,
                              std::vector<std::vector<int> >& cluster_groups);

    virtual void update(HLStateInt* o);
    
    virtual wxString _printConfiguration();
    
    HLStateInt* highlight_state;
    
    void UpdateClusterChoice(int n, std::vector<wxInt64>& clusters);
    void Highlight(int id);
    void Highlight(vector<int>& ids);

protected:
    virtual bool Run(vector<wxInt64>& clusters);
    virtual bool RunStar();
    virtual bool CheckAllInputs();
    virtual void GetClusterFromDendrogram(vector<wxInt64>& clusters);
    
protected:
    char     dist;
    int      m_min_samples;
    double   eps;
    
    vector<vector<int> > cluster_ids;
        
    double cutoffDistance;
    vector<wxInt64> clusters;
    
    wxChoice* combo_n;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxChoice* m_distance;
    wxTextCtrl* m_eps;
    wxTextCtrl* m_minsamples;
    wxTextCtrl* m_cluster;
    wxNotebook* notebook;
    wxCheckBox* chk_dbscanstar;
    wxCheckBox* chk_dbscan;
    wxDBScanDendrogram* m_dendrogram;
    wxButton* saveButton;
    
    DECLARE_EVENT_TABLE()
};


class wxDBScanDendrogram : public wxDendrogram
{
public:
    wxDBScanDendrogram(wxWindow* parent,
                       wxWindowID id=wxID_ANY, const wxPoint &pos=wxDefaultPosition,
                       const wxSize &size=wxDefaultSize)
    : wxDendrogram(parent, id, pos, size) {

    }
    virtual ~wxDBScanDendrogram() {}
    virtual void NotifySelection(double cutoff, std::vector<wxInt64>& cluster_labels)
    {
        wxWindow* parent = GetParent();
        while (parent) {
            wxWindow* w = parent;
            DBScanDlg* dlg = dynamic_cast<DBScanDlg*>(w);
            if (dlg) {
                dlg->UpdateFromDendrogram(cutoff, cluster_labels, clusters);
                break;
            }
            parent = w->GetParent();
        }
    }
    virtual void UpdateHighlight()
    {
        wxWindow* parent = GetParent();
        while (parent) {
            wxWindow* w = parent;
            DBScanDlg* dlg = dynamic_cast<DBScanDlg*>(w);
            if (dlg) {
                dlg->Highlight(hl_ids);
                break;
            }
            parent = w->GetParent();
        }
    }
};
#endif
