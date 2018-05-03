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
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dcbuffer.h>


#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../Algorithms/DataUtils.h"


#include "SaveToTableDlg.h"
#include "HDBScanDlg.h"

BEGIN_EVENT_TABLE( HDBScanDlg, wxDialog )
EVT_CLOSE( HDBScanDlg::OnClose )
END_EVENT_TABLE()


HDBScanDlg::HDBScanDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s,  _("HDBScan Clustering Settings"))
{
    wxLogMessage("Open HDBScanDlg.");
    hdb = NULL;
    parent = parent_s;
    project = project_s;
    
    highlight_state = project->GetHighlightState();
                    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
    
    highlight_state->registerObserver(this);
}

HDBScanDlg::~HDBScanDlg()
{
    highlight_state->removeObserver(this);
    if (hdb != NULL) {
        delete hdb;
        hdb = NULL;
    }
}

void HDBScanDlg::Highlight(int id)
{
    vector<bool>& hs = highlight_state->GetHighlight();
    
    for (int i=0; i<hs.size(); i++) hs[i] = false;
    hs[id] = true;
    
    highlight_state->SetEventType(HLStateInt::delta);
    highlight_state->notifyObservers(this);
}

void HDBScanDlg::Highlight(vector<int>& ids)
{
    vector<bool>& hs = highlight_state->GetHighlight();
    
    for (int i=0; i<hs.size(); i++) hs[i] = false;
    for (int i=0; i<ids.size(); i++) hs[ids[i]] = true;
    
    highlight_state->SetEventType(HLStateInt::delta);
    highlight_state->notifyObservers(this);
}

bool HDBScanDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    num_obs = project->GetNumRecords();
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void HDBScanDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(880,780), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
   
    wxPanel *panel = new wxPanel(scrl);
    
    // Input
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    AddInputCtrls(panel, vbox);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,5,0);

    wxStaticText* st2 = new wxStaticText(panel, wxID_ANY, _("Min cluster size:"),
                                         wxDefaultPosition, wxDefaultSize);
    wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
    wxArrayString list;
    wxString valid_chars("0123456789");
    size_t len = valid_chars.Length();
    for (size_t i=0; i<len; i++) {
        list.Add(wxString(valid_chars.GetChar(i)));
    }
    validator.SetIncludes(list);
    m_minpts = new wxTextCtrl(panel, wxID_ANY, "5", wxDefaultPosition, wxSize(120, -1),0,validator);
    gbox->Add(st2, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_minpts, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(120,-1));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 2, choices13);
    box13->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box13, 1, wxEXPAND);

    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxFlexGridSizer* gbox1 = new wxFlexGridSizer(5,2,5,0);

    /*
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"),
                                         wxDefaultPosition, wxDefaultSize);
    max_n_clusters = num_obs < 60 ? num_obs : 60;
    m_cluster = new wxTextCtrl(panel, wxID_ANY, "5", wxDefaultPosition, wxSize(120, -1),0,validator);
    
    gbox1->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(m_cluster, 1, wxEXPAND);
     */
    
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"), wxDefaultPosition, wxDefaultSize);
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, wxT("CL"), wxDefaultPosition, wxSize(120,-1));
    gbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    hbox1->Add(gbox1, 1, wxEXPAND);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition, wxSize(70, 30));
    saveButton = new wxButton(panel, wxID_SAVE, _("Save"), wxDefaultPosition, wxDefaultSize);
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(saveButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);

	// Summary control 
    notebook = new wxNotebook( panel, wxID_ANY);
    //m_panel = new DendrogramPanel(max_n_clusters, notebook, wxID_ANY);
    //notebook->AddPage(m_panel, _("Dendrogram"));
    m_reportbox = new SimpleReportTextCtrl(notebook, wxID_ANY, "");
    notebook->AddPage(m_reportbox, _("Summary"));
    notebook->Connect(wxEVT_NOTEBOOK_PAGE_CHANGING, wxBookCtrlEventHandler(HDBScanDlg::OnNotebookChange), NULL, this);

    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    container->Add(notebook,1, wxEXPAND | wxALL);
    
    panel->SetSizerAndFit(container);
    
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);
    
    scrl->SetSizer(panelSizer);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(scrl, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
    
    // Content
    m_textbox = box3;
    m_distance = box13;
    
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &HDBScanDlg::OnOKClick, this);
    saveButton->Bind(wxEVT_BUTTON, &HDBScanDlg::OnSave, this);
    closeButton->Bind(wxEVT_BUTTON, &HDBScanDlg::OnClickClose, this);
    //m_cluster->Connect(wxEVT_TEXT, wxCommandEventHandler(HDBScanDlg::OnClusterChoice), NULL, this);
    
    saveButton->Disable();
    //combo_n->Disable();
    //m_cluster->Disable();
}

void HDBScanDlg::OnNotebookChange(wxBookCtrlEvent& event)
{
    int tab_idx = event.GetOldSelection();
    //m_panel->SetActive(tab_idx == 1);
}

void HDBScanDlg::OnSave(wxCommandEvent& event )
{
    // save to table
    int new_col = 3;
    
    std::vector<SaveToTableEntry> new_data(new_col);

    vector<bool> undefs(rows, false);

    
    new_data[0].d_val = &hdb->core_dist;
    new_data[0].label = "Core Dist";
    new_data[0].field_default = "HDB_CORE";
    new_data[0].type = GdaConst::double_type;
    new_data[0].undefined = &undefs;
    
    new_data[1].d_val = &hdb->probabilities;
    new_data[1].label = "Probabilities";
    new_data[1].field_default = "HDB_PVAL";
    new_data[1].type = GdaConst::double_type;
    new_data[1].undefined = &undefs;
    
    new_data[2].d_val = &hdb->stabilities;
    new_data[2].label = "Stabilities";
    new_data[2].field_default = "HDB_STAB";
    new_data[2].type = GdaConst::double_type;
    new_data[2].undefined = &undefs;
    
    SaveToTableDlg dlg(project, this, new_data,
                       "Save Results: HDBScan (Core Distances/Probabilities/Stabilities)",
                       wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
    
    event.Skip();
}

void HDBScanDlg::OnDistanceChoice(wxCommandEvent& event)
{
    
    if (m_distance->GetSelection() == 0) {
        m_distance->SetSelection(1);
    } else if (m_distance->GetSelection() == 3) {
        m_distance->SetSelection(4);
    } else if (m_distance->GetSelection() == 6) {
        m_distance->SetSelection(7);
    } else if (m_distance->GetSelection() == 9) {
        m_distance->SetSelection(10);
    }
}

void HDBScanDlg::OnClusterChoice(wxCommandEvent& event)
{
    //int sel_ncluster = combo_n->GetSelection() + 2;
    wxString tmp_val = m_cluster->GetValue();
    tmp_val.Trim(false);
    tmp_val.Trim(true);
    long sel_ncluster;
    bool is_valid = tmp_val.ToLong(&sel_ncluster);
    if (is_valid) {
        //sel_ncluster += 2;
        // update dendrogram
        //m_panel->UpdateCluster(sel_ncluster, clusters);
    }
}

void HDBScanDlg::UpdateClusterChoice(int n, std::vector<wxInt64>& _clusters)
{
    //int sel = n - 2;
    //combo_n->SetSelection(sel);
    wxString str_n;
    str_n << n;
    m_cluster->SetValue(str_n);
    for (int i=0; i<clusters.size(); i++){
        clusters[i] = _clusters[i];
    }
}

void HDBScanDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox HDBScanDlg.");

    wxArrayString items;
    
    std::vector<int> col_id_map;
    table_int->FillNumericColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = name;
                name_to_tm_id[nm] = t;
                items.Add(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            items.Add(name);
        }
    }
    if (!items.IsEmpty())
        var_box->InsertItems(items,0);
    
    for (int i=0; i<select_vars.size(); i++) {
        var_box->SetStringSelection(select_vars[i], true);
    }
}

void HDBScanDlg::update(HLStateInt* o)
{
}

void HDBScanDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose HDBScanDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void HDBScanDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close HDBScanDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString HDBScanDlg::_printConfiguration()
{
    wxString txt;
    txt << "Minimum cluster size:\t" << m_minpts->GetValue() << "\n";
    txt << "Number of clusters:\t" << cluster_ids.size() << "\n";
    txt << "Transformation:\t" << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
    txt << "Distance function:\t" << m_distance->GetString(m_distance->GetSelection()) << "\n";
    
    return txt;
}

void HDBScanDlg::OnOKClick(wxCommandEvent& event )
{
    wxLogMessage("Click HDBScanDlg::OnOK");
    
    long minPts = 0;
    m_minpts->GetValue().ToLong(&minPts);
    if (minPts<1) {
        wxString err_msg = _("Minimum cluster size should be greater than one.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    int transform = combo_tranform->GetSelection();
    bool success = GetInputData(transform,1);
    if (!success) {
        return;
    }
    
    char method = 's';
    char dist = 'e';
    
    int transpose = 0; // row wise
    
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];
    
    clusters.clear();
    clusters_undef.clear();
   
    double** ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
    double** distances = DataUtils::fullRaggedMatrix(ragged_distances, rows, rows);
    for (int i = 1; i < rows; i++) free(ragged_distances[i]);
    free(ragged_distances);
    
    if (hdb != NULL) {
        delete hdb;
        hdb = NULL;
    }
    hdb = new GeoDaClustering::HDBScan(minPts, rows, columns, distances, input_data, undefs);
    
    cluster_ids = hdb->GetRegions();
    
    int ncluster = cluster_ids.size();
    
    vector<wxInt64> clusters(rows, 0);
    vector<bool> clusters_undef(rows, false);
    
    // sort result
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < ncluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    
    // in case not clustered
    int not_clustered =0;
    for (int i=0; i<clusters.size(); i++) {
        if (clusters[i] == 0) {
            clusters[i] = ncluster + 1;
            not_clustered ++;
        }
    }
    
    // summary
    CreateSummary(clusters);
    
    // save to table
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    int time=0;
    int col = table_int->FindColId(field_name);
    if ( col == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_long_len;
        int m_decimals_val = 0;
        
        col = table_int->InsertCol(GdaConst::long64_type, field_name, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    } else {
        // detect if column is integer field, if not raise a warning
        if (table_int->GetColType(col) != GdaConst::long64_type ) {
            wxString msg = _("This field name already exists (non-integer type). Please input a unique name.");
            wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }
    }
    
    if (col > 0) {
        table_int->SetColData(col, time, clusters);
        table_int->SetColUndefined(col, time, clusters_undef);
    }
    
    // free memory
    for (int i = 1; i < rows; i++) free(distances[i]);
    free(distances);
    
    //delete[] bound_vals;
    //bound_vals = NULL;
    
    // show a cluster map
    if (project->IsTableOnlyProject()) {
        return;
    }
    std::vector<GdaVarTools::VarInfo> new_var_info;
    std::vector<int> new_col_ids;
    new_col_ids.resize(1);
    new_var_info.resize(1);
    new_col_ids[0] = col;
    new_var_info[0].time = 0;
    // Set Primary GdaVarTools::VarInfo attributes
    new_var_info[0].name = field_name;
    new_var_info[0].is_time_variant = table_int->IsColTimeVariant(col);
    table_int->GetMinMaxVals(new_col_ids[0], new_var_info[0].min, new_var_info[0].max);
    new_var_info[0].sync_with_global_time = new_var_info[0].is_time_variant;
    new_var_info[0].fixed_scale = true;
    
    
    MapFrame* nf = new MapFrame(parent, project,
                                new_var_info, new_col_ids,
                                CatClassification::unique_values,
                                MapCanvas::no_smoothing, 4,
                                boost::uuids::nil_uuid(),
                                wxDefaultPosition,
                                GdaConst::map_default_size);
    wxString ttl;
    ttl << "HDBScan Cluster Map (";
    ttl << clusters.size();
    ttl << " clusters)";
    nf->SetTitle(ttl);
    
    if (not_clustered>0) {
        nf->SetLegendLabel(ncluster, "Not Clustered");
    }
    
    saveButton->Enable();
    
    // draw dendrogram
    // GdaNode* _root, int _nelements, int _nclusters, std::vector<wxInt64>& _clusters, double _cutoff
    /*double** sltree =hdb.single_linkage_tree;
    GdaNode* htree = new GdaNode[rows-1];
    
    for (int i=0; i<rows-1; i++) {
        htree[i].left = edges[i].orig;
        htree[i].right = edges[i].dest;
        htree[i].distance = edges[i].length;
    }
    
    m_panel->Setup(htree, rows,  not_clustered > 0 ? ncluster +1 : ncluster, clusters, 0);
    // free(htree); should be freed in m_panel since drawing still needs it's instance
    */
    
    saveButton->Enable();
    //m_cluster->Enable();
}
