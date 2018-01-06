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
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/mds.h"
#include "../Explore/ScatterNewPlotView.h"
#include "SaveToTableDlg.h"
#include "MDSDlg.h"

BEGIN_EVENT_TABLE( MDSDlg, wxDialog )
EVT_CLOSE( MDSDlg::OnClose )
END_EVENT_TABLE()

MDSDlg::MDSDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("MDS Settings"))
{
    wxLogMessage("Open MDSDlg.");
   
    CreateControls();
}

MDSDlg::~MDSDlg()
{
}

void MDSDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(420,540), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    AddSimpleInputCtrls(panel, &combo_var, vbox);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,10,0);
   
    // power iteration option approximation
    wxStaticText* st15 = new wxStaticText(panel, wxID_ANY, _("Use Power Iteration:"), wxDefaultPosition, wxSize(134,-1));
    wxBoxSizer *hbox15 = new wxBoxSizer(wxHORIZONTAL);
    chk_poweriteration = new wxCheckBox(panel, wxID_ANY, "");
    lbl_poweriteration = new wxStaticText(panel, wxID_ANY, _("# Max Iteration:"));
    txt_poweriteration = new wxTextCtrl(panel, wxID_ANY, "100",wxDefaultPosition, wxSize(70,-1));
    txt_poweriteration->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
    chk_poweriteration->Bind(wxEVT_CHECKBOX, &MDSDlg::OnCheckPowerIteration, this);
    if (project->GetNumRecords() < 150) {
        lbl_poweriteration->Disable();
        txt_poweriteration->Disable();
    } else {
        chk_poweriteration->SetValue(true);
    }
    hbox15->Add(chk_poweriteration);
    hbox15->Add(lbl_poweriteration);
    hbox15->Add(txt_poweriteration);
    gbox->Add(st15, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox15, 1, wxEXPAND);
    
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(134,-1));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
    panel->SetSizer(container);
   
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);
    
    scrl->SetSizer(panelSizer);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(scrl, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);

    Centre();
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &MDSDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &MDSDlg::OnCloseClick, this);
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

void MDSDlg::OnCheckPowerIteration(wxCommandEvent& event)
{
    if (chk_poweriteration->IsChecked()) {
        txt_poweriteration->Enable();
        lbl_poweriteration->Enable();
    } else {
        txt_poweriteration->Disable();
        lbl_poweriteration->Disable();
    }
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

void MDSDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox HClusterDlg.");
    
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
}

wxString MDSDlg::_printConfiguration()
{
    return "";
}

void MDSDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click MDSDlg::OnOK");
   
    int transform = combo_tranform->GetSelection();
   
    if (!GetInputData(transform, 2))
        return;

    double* weight = GetWeights(columns);

    int transpose = 0; // row wise
    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];
  
    int new_col = 2;
    vector<vector<double> > results;
    
    if (chk_poweriteration->IsChecked()) {
        double** ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
        
        vector<vector<double> > distances = DataUtils::copyRaggedMatrix(ragged_distances, rows, rows);
        for (int i = 1; i < rows; i++) free(ragged_distances[i]);
        free(ragged_distances);
        
        if (dist == 'b') {
            for (int i=0; i<distances.size(); i++) {
                for (int j=0; j<distances.size(); j++) {
                    distances[i][j] = distances[i][j]*distances[i][j];
                    distances[i][j] = distances[i][j]*distances[i][j];
                }
            }
        }
       
        wxString str_iterations;
        str_iterations = txt_poweriteration->GetValue();
        long l_iterations = 0;
        str_iterations.ToLong(&l_iterations);
        FastMDS mds(distances, 2, (int)l_iterations);
        results = mds.GetResult();
        
    } else {
        results.resize(new_col);
        for (int i=0; i<new_col; i++) results[i].resize(rows);
        double** rst = mds(rows, columns, input_data,  mask, weight, transpose, dist,  NULL, 2);
        for (int i=0; i<new_col; i++) {
            for (int j = 0; j < rows; ++j) {
                results[i][j] = rst[j][i];
            }

        }
        for (int j = 0; j < rows; ++j) delete[] rst[j];
        delete[] rst;
    }
   
    if (!results.empty()) {
        
        std::vector<SaveToTableEntry> new_data(new_col);
        std::vector<std::vector<double> > vals(new_col);
        std::vector<std::vector<bool> > undefs(new_col);
        
        for (int j = 0; j < new_col; ++j) {
            vals[j].resize(rows);
            undefs[j].resize(rows);
            for (int i = 0; i < rows; ++i) {
                vals[j][i] = double(results[j][i]);
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
        if (dlg.ShowModal() == wxID_OK) {
            // show in a scatter plot
            std::vector<int>& new_col_ids = dlg.new_col_ids;
            std::vector<wxString>& new_col_names = dlg.new_col_names;
            
            std::vector<GdaVarTools::VarInfo> new_var_info;
            new_var_info.resize(2);
            
            new_var_info[0].time = 0;
            // Set Primary GdaVarTools::VarInfo attributes
            new_var_info[0].name = new_col_names[0];
            new_var_info[0].is_time_variant = table_int->IsColTimeVariant(new_col_ids[0]);
            table_int->GetMinMaxVals(new_col_ids[0], new_var_info[0].min, new_var_info[0].max);
            new_var_info[0].sync_with_global_time = new_var_info[0].is_time_variant;
            new_var_info[0].fixed_scale = true;
            
            new_var_info[1].time = 0;
            // Set Primary GdaVarTools::VarInfo attributes
            new_var_info[1].name = new_col_names[1];
            new_var_info[1].is_time_variant = table_int->IsColTimeVariant(new_col_ids[1]);
            table_int->GetMinMaxVals(new_col_ids[1], new_var_info[1].min, new_var_info[1].max);
            new_var_info[1].sync_with_global_time = new_var_info[1].is_time_variant;
            new_var_info[1].fixed_scale = true;
            
            wxString title = _("MDS Plot - ") + new_col_names[0] + ", " + new_col_names[1];
            
            MDSPlotFrame* subframe =
            new MDSPlotFrame(parent, project,
                                    new_var_info, new_col_ids,
                                    false, title, wxDefaultPosition,
                                    GdaConst::scatterplot_default_size,
                                    wxDEFAULT_FRAME_STYLE);
            wxCommandEvent ev;
            subframe->OnViewLinearSmoother(ev);
            subframe->OnDisplayStatistics(ev);
        }

    }
}
