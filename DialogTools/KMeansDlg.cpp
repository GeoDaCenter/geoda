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
#include <algorithm>
#include <limits>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "KMeansDlg.h"


BEGIN_EVENT_TABLE( KMeansDlg, wxDialog )
EVT_CLOSE( KMeansDlg::OnClose )
END_EVENT_TABLE()

KMeansDlg::KMeansDlg(wxFrame* parent_s, Project* project_s)
: frames_manager(project_s->GetFramesManager()),
wxDialog(NULL, -1, _("K-Means Settings"), wxDefaultPosition, wxDefaultSize,
         wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open KMeanDlg.");
    
	SetMinSize(wxSize(360,750));

    parent = parent_s;
    project = project_s;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
    frames_manager->registerObserver(this);
}

KMeansDlg::~KMeansDlg()
{
    frames_manager->removeObserver(this);
}

bool KMeansDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void KMeansDlg::update(FramesManager* o)
{
    
}

void KMeansDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables"),
                                         wxDefaultPosition, wxDefaultSize);
    
    wxListBox* box = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(250,250), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    wxCheckBox* cbox = new wxCheckBox(panel, wxID_ANY, _("Use Geometric Centroids"));
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, "Input:");
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(box, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox0->Add(cbox, 0, wxLEFT | wxRIGHT, 10);
    
    if (project->IsTableOnlyProject()) {
        cbox->Disable();
    }
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);

    
    wxString choices[] = {"2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20"};
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"),
                                         wxDefaultPosition, wxSize(128,-1));
    wxChoice* box1 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                      wxSize(200,-1), 9, choices);
    box1->SetSelection(3);
    gbox->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box1, 1, wxEXPAND);
    
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Transformation:"),
                                          wxDefaultPosition, wxSize(120,-1));
    const wxString _transform[3] = {"Raw", "Demean", "Standardize"};
    wxChoice* box01 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(120,-1), 3, _transform);
    box01->SetSelection(2);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box01, 1, wxEXPAND);
    
    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY, _("Initialization Method:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxString choices16[] = {"KMeans++", "Random"};
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(200,-1), 2, choices16);
    combo_method->SetSelection(0);

    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);

    
    wxStaticText* st10 = new wxStaticText(panel, wxID_ANY, _("Initialization Re-runs:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxTextCtrl  *box10 = new wxTextCtrl(panel, wxID_ANY, wxT("50"), wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st10, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box10, 1, wxEXPAND);
    
    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Use specified seed:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
    chk_seed = new wxCheckBox(panel, wxID_ANY, "");
    seedButton = new wxButton(panel, wxID_OK, wxT("Change Seed"));
    
    hbox17->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
    hbox17->Add(seedButton,0,wxALIGN_CENTER_VERTICAL);
    seedButton->Disable();
    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox17, 1, wxEXPAND);
    
    if (GdaConst::use_gda_user_seed) {
        setrandomstate(GdaConst::gda_user_seed);
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    
    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("Maximal Iterations:"),
                                         wxDefaultPosition, wxSize(128,-1));
    wxTextCtrl  *box11 = new wxTextCtrl(panel, wxID_ANY, wxT("300"), wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box11, 1, wxEXPAND);
    
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Method:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxString choices12[] = {"Arithmetic Mean", "Arithmetic Median"};
    wxChoice* box12 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                       wxSize(200,-1), 2, choices12);
	box12->SetSelection(0);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box12, 1, wxEXPAND);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxString choices13[] = {"Distance", "--Euclidean", "--City-block", "Correlation", "--Pearson","--Absolute Pearson", "Cosine", "--Signed", "--Un-signed", "Rank", "--Spearman", "--Kendal"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 12, choices13);
    box13->SetSelection(1);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box13, 1, wxEXPAND);

    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"),
                                         wxDefaultPosition, wxDefaultSize);
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL);
    
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    //wxButton *saveButton = new wxButton(panel, wxID_SAVE, wxT("Save"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    //hbox2->Add(saveButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);

    
    Centre();

    
    // Content
    InitVariableCombobox(box);
    combo_n = box1;
    m_textbox = box3;
    combo_var = box;
    m_use_centroids = cbox;
    m_iterations = box11;
    m_pass = box10;
    m_method = box12;
    m_distance = box13;
    combo_tranform = box01;
    
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &KMeansDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &KMeansDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &KMeansDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &KMeansDlg::OnChangeSeed, this);
    
    m_distance->Connect(wxEVT_CHOICE,
                        wxCommandEventHandler(KMeansDlg::OnDistanceChoice),
                        NULL, this);
}

void KMeansDlg::OnSeedCheck(wxCommandEvent& event)
{
    bool use_user_seed = chk_seed->GetValue();
    
    if (use_user_seed) {
        seedButton->Enable();
        if (GdaConst::use_gda_user_seed == false && GdaConst::gda_user_seed == 0) {
            OnChangeSeed(event);
            return;
        }
        GdaConst::use_gda_user_seed = true;
        setrandomstate(GdaConst::gda_user_seed);
        
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "1");
    } else {
        seedButton->Disable();
    }
}

void KMeansDlg::OnChangeSeed(wxCommandEvent& event)
{
    // prompt user to enter user seed (used globally)
    wxString m;
    m << "Enter a seed value for random number generator:";
    
    long long unsigned int val;
    wxString dlg_val;
    wxString cur_val;
    cur_val << GdaConst::gda_user_seed;
    
    wxTextEntryDialog dlg(NULL, m, "Enter a seed value", cur_val);
    if (dlg.ShowModal() != wxID_OK) return;
    dlg_val = dlg.GetValue();
    dlg_val.Trim(true);
    dlg_val.Trim(false);
    if (dlg_val.IsEmpty()) return;
    if (dlg_val.ToULongLong(&val)) {
        uint64_t new_seed_val = val;
        GdaConst::gda_user_seed = new_seed_val;
        GdaConst::use_gda_user_seed = true;
        setrandomstate(GdaConst::gda_user_seed);
        
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        wxString str_gda_user_seed;
        str_gda_user_seed << GdaConst::gda_user_seed;
        ogr_adapt.AddEntry("gda_user_seed", str_gda_user_seed.ToStdString());
        ogr_adapt.AddEntry("use_gda_user_seed", "1");
    } else {
        wxString m;
        m << "\"" << dlg_val << "\" is not a valid seed. Seed unchanged.";
        wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        GdaConst::use_gda_user_seed = false;
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "0");
    }
}

void KMeansDlg::OnDistanceChoice(wxCommandEvent& event)
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

void KMeansDlg::InitVariableCombobox(wxListBox* var_box)
{
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
    
    var_box->InsertItems(items,0);
}

void KMeansDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose KMeansDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void KMeansDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close HClusterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void KMeansDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click KMeansDlg::OnOK");
    
    int ncluster = combo_n->GetSelection() + 2;
    
    bool use_centroids = m_use_centroids->GetValue();
    
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = selections.size();
    if (num_var < 2 && !use_centroids) {
        // show message box
        wxString err_msg = _("Please select at least 2 variables.");
        wxMessageDialog dlg(NULL, err_msg, "Info", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    col_ids.resize(num_var);
    var_info.resize(num_var);
    
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString nm = name_to_nm[combo_var->GetString(idx)];
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen the Regression Dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        int tm = name_to_tm_id[combo_var->GetString(idx)];
        
        col_ids[i] = col;
        var_info[i].time = tm;
        
        // Set Primary GdaVarTools::VarInfo attributes
        var_info[i].name = nm;
        var_info[i].is_time_variant = table_int->IsColTimeVariant(idx);
        
        // var_info[i].time already set above
        table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
        var_info[i].sync_with_global_time = var_info[i].is_time_variant;
        var_info[i].fixed_scale = true;
    }
    
    // Call function to set all Secondary Attributes based on Primary Attributes
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
    
    int rows = project->GetNumRecords();
    int columns =  0;
    
    std::vector<d_array_type> data; // data[variable][time][obs]
    data.resize(col_ids.size());
    for (int i=0; i<var_info.size(); i++) {
        table_int->GetColData(col_ids[i], data[i]);
    }
    // get columns (if time variables show)
    for (int i=0; i<data.size(); i++ ){
        for (int j=0; j<data[i].size(); j++) {
            columns += 1;
        }
    }
    
    // if use centroids
    if (use_centroids) {
        columns += 2;
    }
    
    int transform = combo_tranform->GetSelection();
    char method = 'a'; // mean, 'm' median
    char dist = 'e'; // euclidean
    int npass = 10;
    int n_maxiter = 300; // max iteration of EM
    int transpose = 0; // row wise
    int* clusterid = new int[rows];
    double* weight = new double[columns];
    for (int j=0; j<columns; j++){ weight[j] = 1;}
    
    wxString iterations = m_iterations->GetValue();
    long value;
    if(iterations.ToLong(&value)) {
        n_maxiter = value;
    }
    
    if (combo_method->GetSelection() == 0) method = 'b'; // mean with kmeans++
    
    wxString str_pass = m_pass->GetValue();
    long value_pass;
    if(str_pass.ToLong(&value_pass)) {
        npass = value_pass;
    }
    
    int method_sel = m_method->GetSelection();
    if (method_sel == 1) method = 'm';
    
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','e','b','c','c','a','u','u','x','s','s','k'};
    dist = dist_choices[dist_sel];
    
    // init input_data[rows][cols]
    double** input_data = new double*[rows];
    int** mask = new int*[rows];
    for (int i=0; i<rows; i++) {
        input_data[i] = new double[columns];
        mask[i] = new int[columns];
        for (int j=0; j<columns; j++){
            mask[i][j] = 1;
        }
    }
    
    // assign value
    int col_ii = 0;
    for (int i=0; i<data.size(); i++ ){ // col
        
        for (int j=0; j<data[i].size(); j++) { // time
            
            std::vector<double> vals;
            
            for (int k=0; k< rows;k++) { // row
                vals.push_back(data[i][j][k]);
            }

            if (transform == 2) {
                GenUtils::StandardizeData(vals);
            } else if (transform == 1 ) {
                GenUtils::DeviationFromMean(vals);
            }
            
            for (int k=0; k< rows;k++) { // row
                input_data[k][col_ii] = vals[k];
            }
            col_ii += 1;
        }
    }
    if (use_centroids) {
        std::vector<GdaPoint*> cents = project->GetCentroids();
        std::vector<double> cent_xs;
        std::vector<double> cent_ys;
        
        for (int i=0; i< rows; i++) {
            cent_xs.push_back(cents[i]->GetX());
            cent_ys.push_back(cents[i]->GetY());
        }
        
        if (transform == 2) {
            GenUtils::StandardizeData(cent_xs );
            GenUtils::StandardizeData(cent_ys );
        } else if (transform == 1 ) {
            GenUtils::DeviationFromMean(cent_xs );
            GenUtils::DeviationFromMean(cent_ys );
        }
        
        for (int i=0; i< rows; i++) {
            input_data[i][col_ii + 0] = cent_xs[i];
            input_data[i][col_ii + 1] = cent_ys[i];
        }
    }
    
    double error;
    int ifound;
    kcluster(ncluster, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound);
    
    vector<wxInt64> clusters;
    vector<bool> clusters_undef;
    
    // clean memory
    for (int i=0; i<rows; i++) {
        delete[] input_data[i];
        delete[] mask[i];
        clusters.push_back(clusterid[i] + 1);
        clusters_undef.push_back(ifound == -1);
    }
    delete[] input_data;
    delete[] weight;
    delete[] clusterid;
    
    // sort result
    std::vector<std::vector<int> > cluster_ids(ncluster);
    
    for (int i=0; i < clusters.size(); i++) {
        cluster_ids[ clusters[i] - 1 ].push_back(i);
    }

    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < ncluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    
    // save to table
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
            wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }
    }
    
    if (col > 0) {
        table_int->SetColData(col, time, clusters);
        table_int->SetColUndefined(col, time, clusters_undef);
    }
    
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
}
