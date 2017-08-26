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

#include <set>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/dialog.h>
#include <wx/xrc/xmlres.h>
#include <wx/tokenzr.h>

#include "../FramesManager.h"
#include "../DataViewer/TableInterface.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "SaveToTableDlg.h"
#include "MDSDlg.h"

BEGIN_EVENT_TABLE( MDSDlg, wxDialog )
EVT_CLOSE( MDSDlg::OnClose )
END_EVENT_TABLE()

MDSDlg::MDSDlg(wxFrame *parent, Project* project_s)
: wxDialog(parent, -1, _("MDS Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
frames_manager(project_s->GetFramesManager())
{
    wxLogMessage("Open MDSDlg.");
    
    SetMinSize(wxSize(360,750));
    
    project = project_s;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
    frames_manager->registerObserver(this);
}

MDSDlg::~MDSDlg()
{
    frames_manager->removeObserver(this);
}

void MDSDlg::update(FramesManager* o)
{
}

bool MDSDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void MDSDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // input
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables"),
                                         wxDefaultPosition, wxDefaultSize);
    
    wxListBox* box = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(250,250), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, "Input:");
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(box, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,10,0);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxString choices13[] = {"Distance", "--Euclidean", "--City-block", "Correlation", "--Pearson","--Absolute Pearson", "Cosine", "--Signed", "--Un-signed", "Rank", "--Spearman", "--Kendal"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 12, choices13);
    m_distance->SetSelection(1);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    
    
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Transformation:"),
                                          wxDefaultPosition, wxSize(120,-1));
    const wxString _transform[3] = {"Raw", "Demean", "Standardize"};
    wxChoice* box01 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(120,-1), 3, _transform);
    
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box01, 1, wxEXPAND);
    
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Components:"),
                                         wxDefaultPosition, wxSize(140,-1));
    wxChoice* box1 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(120,-1), 0, NULL);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    hbox1->Add(st1, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box1, 1, wxALIGN_CENTER_VERTICAL);
    
    
    
    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    saveButton = new wxButton(panel, wxID_SAVE, wxT("Save"), wxDefaultPosition,
                              wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(saveButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 1, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
    panel->SetSizer(container);
    
    Centre();
    
    // Content
    InitVariableCombobox(box);
    
    saveButton->Enable(false);
    combo_var = box;
    combo_n = box1;
    
    combo_transform = box01;
    combo_transform->SetSelection(2);
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &MDSDlg::OnOK, this);
    saveButton->Bind(wxEVT_BUTTON, &MDSDlg::OnSave, this);
    closeButton->Bind(wxEVT_BUTTON, &MDSDlg::OnCloseClick, this);
    m_distance->Bind(wxEVT_CHOICE, &MDSDlg::OnDistanceChoice, this);
}

void MDSDlg::OnDistanceChoice(wxCommandEvent& event)
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

void MDSDlg::InitVariableCombobox(wxListBox* var_box)
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
                name_to_nm[nm] = name;// table_int->GetColName(id, t);
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

void MDSDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close HClusterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void MDSDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close MDSDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void MDSDlg::OnSave(wxCommandEvent& event )
{
    wxLogMessage("OnSave MDSDlg.");
    
    if (scores.size()==0)
        return;
    
    // save to table
    int new_col = combo_n->GetSelection() + 1;
    
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
        new_data[j].label = wxString::Format("V%d", j+1);
        new_data[j].field_default = wxString::Format("V%d", j+1);
        new_data[j].type = GdaConst::double_type;
        new_data[j].undefined = &undefs[j];
    }
    
    SaveToTableDlg dlg(project, this, new_data,
                       "Save Results: MDS",
                       wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
    
    event.Skip();
    
}

void MDSDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click MDSDlg::OnOK");
    
    wxArrayString sel_names;
    int max_sel_name_len = 0;
    
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = selections.size();
    if (num_var < 2) {
        // show message box
        wxString err_msg = _("Please select at least 2 variables.");
        wxMessageDialog dlg(NULL, err_msg, "Info", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    int rows = project->GetNumRecords();
    int columns =  num_var;
    
    col_ids.resize(num_var);
    var_info.resize(num_var);
    
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString nm = name_to_nm[combo_var->GetString(idx)];
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
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
   
    std::vector<d_array_type> data; // data[variable][time][obs]
    data.resize(col_ids.size());
    for (int i=0; i<var_info.size(); i++) {
        table_int->GetColData(col_ids[i], data[i]);
    }
    
    int transpose = 0; // row wise
    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','e','b','c','c','a','u','u','x','s','s','k'};
    dist = dist_choices[dist_sel];
  
    double* weight = new double[columns];
    for (int i=0; i<columns; i++ ) weight[i] = 1;
    
    int transform = combo_transform->GetSelection();
    
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
  
    
    double** results = mds(rows, columns, input_data,  mask, weight, transpose, dist,  NULL, 2);
    
    combo_n->Clear();
    for (int i=0; i<rows; i++){
        combo_n->Append(wxString::Format("%d", i+1));
    }
    combo_n->SetSelection(1);
    
    saveButton->Enable(true);
    
    for (int i=0; i<rows; i++) {
        delete[] input_data[i];
        delete[] mask[i];
    }
    delete[] input_data;
    delete[] weight;
}
