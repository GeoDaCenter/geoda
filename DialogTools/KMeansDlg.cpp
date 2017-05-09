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


#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "KMeansDlg.h"


BEGIN_EVENT_TABLE( KMeansDlg, wxDialog )
EVT_CLOSE( KMeansDlg::OnClose )
END_EVENT_TABLE()

KMeansDlg::KMeansDlg(wxFrame* parent_s, Project* project_s)
: frames_manager(project_s->GetFramesManager()),
wxDialog(NULL, -1, _("K-Means Settings"), wxDefaultPosition, wxSize(360, 640), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open KMeanDlg.");
    
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
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,0,0);

    wxString choices[] = {"2","3","4","5","6","7","8","9","10"};
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"),
                                         wxDefaultPosition, wxSize(122,-1));
    wxComboBox* box1 = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                      wxSize(200,-1), 9, choices, wxCB_READONLY);
    gbox->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box1, 1, wxEXPAND);
    
    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("# of Iterations (EM):"),
                                         wxDefaultPosition, wxSize(122,-1));
    wxTextCtrl  *box11 = new wxTextCtrl(panel, wxID_ANY, wxT("5"), wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box11, 1, wxEXPAND);
    
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Method:"),
                                          wxDefaultPosition, wxSize(122,-1));
    wxString choices12[] = {"Arithmetic Mean","Arithmetic Median"};
    wxComboBox* box12 = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                       wxSize(200,-1), 2, choices12, wxCB_READONLY);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box12, 1, wxEXPAND);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(122,-1));
    wxString choices13[] = {"Euclidean distance", "Pearson correlation","Absolute Pearson correlation","Uncentered correlation","Absolute uncentered correlation","Spearman rank correlation","Tau","City-block distance"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                      wxSize(2000,-1), 8, choices13);
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
    vbox->Add(hbox1, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 1, wxALIGN_CENTER | wxALL, 10);
    
    
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    /*
    m_textbox = new wxTextCtrl(panel, XRCID("ID_TEXTCTRL"), "", wxDefaultPosition, wxSize(320,830), wxTE_MULTILINE | wxTE_READONLY);
    
    if (GeneralWxUtils::isWindows()) {
        wxFont font(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
    } else {
        wxFont font(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
        
    }
    vbox1->Add(m_textbox, 1, wxEXPAND|wxALL,20);
    */
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    container->Add(vbox1,1, wxEXPAND | wxALL);
    
    panel->SetSizer(container);
    
    Centre();
    
    // Content
    InitVariableCombobox(box);
    combo_n = box1;
    m_textbox = box3;
    combo_var = box;
    m_use_centroids = cbox;
    m_iterations = box11;
    m_method = box12;
    m_distance = box13;
    
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &KMeansDlg::OnOK, this);
    //saveButton->Bind(wxEVT_BUTTON, &KMeansDlg::OnSave, this);
    closeButton->Bind(wxEVT_BUTTON, &KMeansDlg::OnClickClose, this);
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
}

void KMeansDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close HClusterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void KMeansDlg::OnSave(wxCommandEvent& event )
{
    wxLogMessage("OnSave KMeansDlg.");
    
    if (scores.size()==0)
        return;
    
    // save to table
    int new_col = int(thresh95);
    int user_sel = combo_n->GetSelection();
    if (user_sel >=0 && user_sel < new_col){
        new_col = user_sel;
    }
    std::vector<SaveToTableEntry> new_data(new_col);
    std::vector<std::vector<double> > vals(new_col);
    std::vector<std::vector<bool> > undefs(new_col);
    
    for (unsigned int j = 0; j < new_col; ++j) {
        vals[j].resize(row_lim);
        undefs[j].resize(row_lim);
        for (unsigned int i = 0; i < row_lim; ++i) {
            vals[j][i] = double(scores[j + col_lim*i]);
            undefs[j][i] = false;
        }
        new_data[j].d_val = &vals[j];
        new_data[j].label = wxString::Format("PC%d", j+1);
        new_data[j].field_default = wxString::Format("PC%d", j+1);
        new_data[j].type = GdaConst::double_type;
        new_data[j].undefined = &undefs[j];
    }
    
    SaveToTableDlg dlg(project, this, new_data,
                       "Save Results: PCA",
                       wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
    
    event.Skip();
    
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
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen the Regression Dialog to synchronize with Table data."), nm); wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
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
    
    bool standardize = true;
    int n_init = 20;
    
    char method = 'a'; // mean, 'm' median
    char dist = 'e'; // euclidean
    int npass = 300;
    int transpose = 0; // row wise
    int* clusterid = new int[rows];
    double* weight = new double[columns];
    for (int j=0; j<columns; j++){ weight[j] = 1;}
    
    wxString iterations = m_iterations->GetValue();
    long value;
    if(iterations.ToLong(&value)) {
        npass = value;
    }
    
    int method_sel = m_method->GetSelection();
    if (method_sel == 1) method = 'm';
    
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','c','a','u','x','s','k','b'};
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
            
            if (standardize) {
                //GenUtils::StandardizeData(rows, data[i][j]);
            }
            
            for (int k=0; k< rows;k++) { // row
                input_data[k][col_ii] = data[i][j][k];
            }
            col_ii += 1;
        }
    }
    if (use_centroids) {
        std::vector<GdaPoint*> cents = project->GetCentroids();
        for (int i=0; i< rows; i++) {
            input_data[i][col_ii + 0] = cents[i]->GetX();
            input_data[i][col_ii + 1] = cents[i]->GetY();
        }
    }
    
    // standardize
    
    
    // get init centroids using n_init
    double* errors = new double[n_init];
    vector<wxInt64> best_clusters;
    
    double error;
    double best_error;
    
    for (int r=0; r<n_init; r++) {
        // run kmeans with init centroids
        for (int i=0; i<rows; i++) {   clusterid[i] = -1; }
        int ifound;
        kcluster(ncluster, rows, columns, input_data, mask, weight, transpose, npass, method, dist, clusterid, &error, &ifound);
        
        vector<wxInt64> clusters;
        vector<bool> clusters_undef;
        
        for (int i=0; i<rows; i++) {
            clusters.push_back(clusterid[i] + 1);
            clusters_undef.push_back(ifound == -1);
        }
        
        if (r == 0) {
            best_error = error;
            best_clusters = clusters;
        } else {
            if (error < best_error) {
                best_clusters = clusters;
            }
        }
    }
    
    for (int i=0; i<rows; i++) {
        // clean memory
        delete[] input_data[i];
        delete[] mask[i];
    }
    delete[] input_data;
    delete[] weight;
    delete[] clusterid;

    
    // save to table
    int time=0;
    int col = table_int->FindColId(field_name);
    if ( col == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_long_len;
        int m_decimals_val = 0;
        
        col = table_int->InsertCol(GdaConst::long64_type, field_name, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    }
    
    if (col > 0) {
        table_int->SetColData(col, time, best_clusters);
        //table_int->SetColUndefined(col, time, clusters_undef);
    }
    
    // show a cluster map
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
