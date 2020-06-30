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
#include "../Algorithms/dbscan.h"

struct GdaNode;
class Project;
class TableInterface;


class DBScanDlg : public AbstractClusterDlg, public HighlightStateObserver
{
public:
    DBScanDlg(wxFrame *parent, Project* project);
    virtual ~DBScanDlg();
    
    void CreateControls();
    virtual bool Init();
    
    void OnOKClick( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnClusterChoice(wxCommandEvent& event);
    void OnSelectVars(wxCommandEvent& event);
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
    DendrogramPanel* m_panel;
    wxTextCtrl* m_eps;
    wxTextCtrl* m_minsamples;
    wxTextCtrl* m_cluster;
    wxNotebook* notebook;

    DECLARE_EVENT_TABLE()
};

#endif
