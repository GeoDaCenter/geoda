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

class GdaNode;
class Project;
class TableInterface;

class DendrogramPanel : public wxPanel
{
public:
    //DendrogramPanel();
    DendrogramPanel(wxWindow* parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize);
    
    void OnMouse( wxMouseEvent& event );
    void OnSize(  wxSizeEvent& event);
    void OnPaint( wxPaintEvent& event );
    void Paint(wxDC *dc);
    void Draw(wxDC* dc);
    void Setup(GdaNode* _root, int _nelements);
    
    
private:
    int leaves;
    int levels;
    int nelements;
    
    double margin;
    double currentY;
    double heightPerLeaf;
    double widthPerLevel;
    
    wxBitmap* layer_bm;
    
    GdaNode* root;
    
    int countLeaves(GdaNode* node);
    
    int countLevels(GdaNode* node);
    
    int countLeaves(int node_idx);
    int countLevels(int node_idx);
    
    wxPoint doDraw(wxMemoryDC &dc, int node_idx, int y);
};

class HClusterDlg : public wxDialog, public FramesManagerObserver
{
public:
    HClusterDlg(wxFrame *parent, Project* project);
    virtual ~HClusterDlg();
    
    void CreateControls();
    bool Init();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnDistanceChoice(wxCommandEvent& event);
    
    void InitVariableCombobox(wxListBox* var_box);
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
private:
    wxFrame *parent;
    Project* project;
    TableInterface* table_int;
    std::vector<wxString> tm_strs;
    
    FramesManager* frames_manager;
    
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
