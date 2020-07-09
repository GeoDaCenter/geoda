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
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../Algorithms/dbscan.h"
#include "../Weights/DistUtils.h"
#include "SaveToTableDlg.h"
#include "DBScanDlg.h"

BEGIN_EVENT_TABLE( DBScanDlg, wxDialog )
EVT_CLOSE( DBScanDlg::OnClose )
END_EVENT_TABLE()


DBScanDlg::DBScanDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s,  _("DBScan Clustering Settings"))
{
    wxLogMessage("Open DBScanDlg.");
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

DBScanDlg::~DBScanDlg()
{
    highlight_state->removeObserver(this);
}

void DBScanDlg::Highlight(int id)
{
    vector<bool>& hs = highlight_state->GetHighlight();
    
    for (int i=0; i<hs.size(); i++) hs[i] = false;
    hs[id] = true;
    
    highlight_state->SetEventType(HLStateInt::delta);
    highlight_state->notifyObservers(this);
}

void DBScanDlg::Highlight(vector<int>& ids)
{
    vector<bool>& hs = highlight_state->GetHighlight();
    
    for (int i=0; i<hs.size(); i++) hs[i] = false;
    for (int i=0; i<ids.size(); i++) hs[ids[i]] = true;
    
    highlight_state->SetEventType(HLStateInt::delta);
    highlight_state->notifyObservers(this);
}

bool DBScanDlg::Init()
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

void DBScanDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(880,780), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
   
    wxPanel *panel = new wxPanel(scrl);
    
    // Input
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    bool show_auto_ctrl = true;
    AddInputCtrls(panel, vbox, show_auto_ctrl);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(10,2,5,0);

    wxStaticText* st2 = new wxStaticText(panel, wxID_ANY, _("Distance Threshold (epsilon):"));
    wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
    wxArrayString list;
    wxString valid_chars(".0123456789");
    size_t len = valid_chars.Length();
    for (size_t i=0; i<len; i++) {
        list.Add(wxString(valid_chars.GetChar(i)));
    }
    validator.SetIncludes(list);
    m_eps = new wxTextCtrl(panel, wxID_ANY, "0.5", wxDefaultPosition, wxSize(120, -1),0,validator);
    gbox->Add(st2, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_eps, 1, wxEXPAND);
    
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Min Samples:"));
    m_minsamples = new wxTextCtrl(panel, wxID_ANY, "5", wxDefaultPosition, wxSize(120, -1),0,validator);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_minsamples, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 2, choices13);
    box13->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box13, 1, wxEXPAND);

    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxFlexGridSizer* gbox1 = new wxFlexGridSizer(5,2,5,0);

    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"));
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, "CL", wxDefaultPosition, wxSize(120,-1));
    gbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    hbox1->Add(gbox1, 1, wxEXPAND);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 0, wxALIGN_CENTER | wxALL, 5);
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
    notebook->Connect(wxEVT_NOTEBOOK_PAGE_CHANGING, wxBookCtrlEventHandler(DBScanDlg::OnNotebookChange), NULL, this);

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
    okButton->Bind(wxEVT_BUTTON, &DBScanDlg::OnOKClick, this);
    closeButton->Bind(wxEVT_BUTTON, &DBScanDlg::OnClickClose, this);
    combo_var->Bind(wxEVT_LISTBOX, &DBScanDlg::OnSelectVars, this);
    m_distance->Bind(wxEVT_CHOICE, &DBScanDlg::OnSelectVars, this);
    combo_tranform->Bind(wxEVT_CHOICE, &DBScanDlg::OnSelectVars, this);
}

void DBScanDlg::OnNotebookChange(wxBookCtrlEvent& event)
{
    //int tab_idx = event.GetOldSelection();
    //m_panel->SetActive(tab_idx == 1);
}

void DBScanDlg::OnSelectVars(wxCommandEvent& event)
{
    // when user select variables, update the eps value
    int transform = combo_tranform->GetSelection();
    if ( GetInputData(transform,1) == false) return;
    weight = GetWeights(columns);
    double** data = input_data;
    
    if (weight) {
        // add weight to input_data
        data  = new double*[rows];
        for (int i=0; i<rows; i++) {
            data[i] = new double[columns];
            for (int j=0; j<columns; j++) {
                data[i][j] = input_data[i][j] * weight[j];
            }
        }
    }
    
    dist = 'e';
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];
    int metric = dist == 'e' ? ANNuse_euclidean_dist : ANNuse_manhattan_dist;
    Gda::DistUtils dist_util(data, rows, columns, metric);
    double m_thres = dist_util.GetMinThreshold();
    m_eps->SetValue(wxString::Format("%f", m_thres));
    if (weight) {
        for (int i=0; i<rows; i++) delete[] data[i];
        delete[] data;
    }
}

void DBScanDlg::OnClusterChoice(wxCommandEvent& event)
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

void DBScanDlg::UpdateClusterChoice(int n, std::vector<wxInt64>& _clusters)
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

void DBScanDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox DBScanDlg.");

    wxArrayString items;
    
    std::vector<int> col_id_map;
    table_int->FillNumericColIdMap(col_id_map);
    for (int i=0; i<col_id_map.size(); i++) {
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

void DBScanDlg::update(HLStateInt* o)
{
}

void DBScanDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose DBScanDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void DBScanDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close DBScanDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString DBScanDlg::_printConfiguration()
{
    wxString txt;
    txt << "Distance Threshold (epsilon):\t" << m_eps->GetValue() << "\n";
    txt << "Min Samples:\t" << m_minsamples->GetValue() << "\n";
    txt << "Transformation:\t" << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
    txt << "Distance function:\t" << m_distance->GetString(m_distance->GetSelection()) << "\n";
    txt << "Number of clusters (output):\t" << cluster_ids.size() << "\n";
    
    return txt;
}

bool DBScanDlg::CheckAllInputs()
{
    if (m_eps->GetValue().ToDouble(&eps) == false) {
        wxString err_msg = _("Please input a valid numeric number for epsilon.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return false;
    }

    m_min_samples = 1;
    long l_min_samples;
    if (m_minsamples->GetValue().ToLong(&l_min_samples)) {
        m_min_samples = (int)l_min_samples;
    }
    if (m_min_samples < 1 || m_min_samples > num_obs) {
        wxString err_msg = _("Min samples (self included) should be greater than 1 and less than N.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return false;
    }

    int transform = combo_tranform->GetSelection();
    if ( GetInputData(transform,1) == false) return false;

    dist = 'e';
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];

    return true;
}

bool DBScanDlg::Run(vector<wxInt64>& clusters)
{
    cluster_ids.clear();
    clusters.clear();
    clusters.resize(rows, 0);

    // NOTE input_data should be retrieved first!!
    // get input: weights (auto)
    weight = GetWeights(columns);
    // add weight to input_data
    double** data = input_data;
    if (weight) {
        data = new double*[rows];
        for (int i=0; i<rows; i++) {
            data[i] = new double[columns];
            for (int j=0; j<columns; j++) {
                data[i][j] = input_data[i][j] * weight[j];
            }
        }
    }
    
    int metric = dist == 'e' ? ANNuse_euclidean_dist : ANNuse_manhattan_dist;

    DBSCAN dbscan((unsigned int)m_min_samples, (float)eps,
                  (const double**)data, (unsigned int)rows,
                  (unsigned int)columns, (int)metric);

    double averagen = dbscan.getAverageNN();
    // check if epsilon may be too small and large
    if (averagen < 1 + 0.1 * (m_min_samples - 1)) {
        wxString err_msg = _("There are very few neighbors found. Epsilon may be too small.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
    }
    if (averagen > 100 * m_min_samples) {
        wxString err_msg = _("There are very many neighbors found. Epsilon may be too large.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
    }

    std::vector<int> labels = dbscan.getResults();

    // process results  and  sort the clusters
    int n_cluster = 0;
    for (int i=0; i<labels.size();  ++i) {
        if (labels[i] > n_cluster) {
            //  label = -1 means noise
            n_cluster = labels[i];
        }
    }
    n_cluster +=  1;
    cluster_ids.resize(n_cluster);
    // group into clusters
    for (int i=0; i < labels.size(); i++) {
        if (labels[i] >= 0) {
            cluster_ids[ labels[i] ].push_back(i);
        }
    }
    // sort clusters
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    // reassign labels
    clusters.resize(rows, 0); // 0 as not clustered
    for (int i=0; i < n_cluster; i++) {
        int c = i+1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }

    if (weight) {
        for (int i=0; i<rows; i++) delete[] data[i];
        delete[] data;
    }
    
    return true;
}

void DBScanDlg::OnOKClick(wxCommandEvent& event )
{
    wxLogMessage("Click DBScanDlg::OnOK");

    // save to table
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    if (CheckAllInputs() == false) return;

    if (Run(clusters) == false) return;
    
    // in case not clustered
    int not_clustered =0;
    for (int i=0; i<clusters.size(); i++) {
        if (clusters[i] == 0) {
            not_clustered ++;
        }
    }
    
    if (not_clustered == rows) {
        wxString err_msg = _("No clusters can be found using current parameters.");
        wxMessageDialog dlg(NULL, err_msg, _("Information"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    
    // summary
    CreateSummary(clusters);
    
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
        vector<bool> clusters_undef(rows, false);
        table_int->SetColData(col, time, clusters);
        table_int->SetColUndefined(col, time, clusters_undef);
    }
    
    // show a cluster map
    if (project->IsTableOnlyProject()) return;

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
    wxString tmp = _("DBScan Cluster Map (%d clusters)");
    wxString ttl = wxString::Format(tmp, (int)cluster_ids.size() - 1);
    nf->SetTitle(ttl);
    if (not_clustered >0) nf->SetLegendLabel(0, _("Not Clustered"));
}
