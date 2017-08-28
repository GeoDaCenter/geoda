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

#include <boost/thread.hpp>
#include <boost/bind.hpp>

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

#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "AbstractClusterDlg.h"


AbstractClusterDlg::AbstractClusterDlg(wxFrame* parent_s, Project* project_s, wxString title)
: frames_manager(project_s->GetFramesManager()),
wxDialog(NULL, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
validator(wxFILTER_INCLUDE_CHAR_LIST)
{
    wxLogMessage("Open AbstractClusterDlg.");
   
    wxArrayString list;
    wxString valid_chars(wxT(".0123456789"));
    size_t len = valid_chars.Length();
    for (size_t i=0; i<len; i++)
        list.Add(wxString(valid_chars.GetChar(i)));
    validator.SetIncludes(list);
    
	SetMinSize(wxSize(360,750));

    parent = parent_s;
    project = project_s;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    }
    frames_manager->registerObserver(this);
}

AbstractClusterDlg::~AbstractClusterDlg()
{
    frames_manager->removeObserver(this);
}

bool AbstractClusterDlg::Init()
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

void AbstractClusterDlg::update(FramesManager* o)
{
    
}

void AbstractClusterDlg::AddInputCtrls(wxPanel *panel, wxListBox** combo_var, wxCheckBox** m_use_centroids,wxSlider** m_weight_centroids, wxTextCtrl** m_wc_txt, wxBoxSizer* vbox)
{
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables"),
                                         wxDefaultPosition, wxDefaultSize);
    
    *combo_var = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(250,250), 0, NULL,
                               wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    InitVariableCombobox(*combo_var);
    
    (*m_use_centroids) = new wxCheckBox(panel, wxID_ANY, _("Use geometric centroids"));
    wxStaticText* st_wc = new wxStaticText (panel, wxID_ANY, _("Weighting:"), wxDefaultPosition, wxDefaultSize);
    wxStaticText* st_w0 = new wxStaticText (panel, wxID_ANY, _("0"));
    wxStaticText* st_w1 = new wxStaticText (panel, wxID_ANY, _("1"));
    (*m_weight_centroids) = new wxSlider(panel, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    (*m_wc_txt) = new wxTextCtrl(panel, wxID_ANY, wxT("1"), wxDefaultPosition, wxSize(40,-1), 0, validator);
    wxBoxSizer *hbox_w = new wxBoxSizer(wxHORIZONTAL);
    hbox_w->Add(st_wc, 0, wxLEFT, 20);
    hbox_w->Add(st_w0, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    hbox_w->Add(*m_weight_centroids, 0, wxEXPAND);
    hbox_w->Add(st_w1, 0, wxALIGN_CENTER_VERTICAL);
    hbox_w->Add(*m_wc_txt, 0, wxALIGN_TOP|wxLEFT, 5);
    
    
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, "Input:");
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(*combo_var, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox0->Add(*m_use_centroids, 0, wxLEFT | wxRIGHT, 10);
    hbox0->Add(hbox_w, 0, wxLEFT | wxRIGHT, 10);
    
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    
    if (project->IsTableOnlyProject()) {
        (*m_use_centroids)->Disable();
    }
    (*m_weight_centroids)->Disable();
    (*m_wc_txt)->Disable();
    (*m_use_centroids)->Bind(wxEVT_CHECKBOX, &AbstractClusterDlg::OnUseCentroids, this);
	(*m_weight_centroids)->Bind(wxEVT_SLIDER, &AbstractClusterDlg::OnSlideWeight, this);
}
void AbstractClusterDlg::OnSlideWeight(wxCommandEvent& ev)
{
    int val = m_weight_centroids->GetValue();
    wxString t_val = wxString::Format("%.2f", val/100.0);
    m_wc_txt->SetValue(t_val);
}
void AbstractClusterDlg::OnUseCentroids(wxCommandEvent& event)
{
    if (m_use_centroids->IsChecked()) {
        m_weight_centroids->Enable();
        m_weight_centroids->SetValue(100);
    } else {
        m_weight_centroids->SetValue(false);
        m_weight_centroids->Disable();
    }
}


void AbstractClusterDlg::AddMinBound(wxPanel *panel, wxCheckBox** chk_floor, wxChoice** combo_floor, wxTextCtrl** txt_floor, wxSlider** slider_floor, wxTextCtrl** txt_floor_pct, wxFlexGridSizer* gbox)
{
    wxStaticText* st = new wxStaticText(panel, wxID_ANY, _("Minimum Bound:"),
                                          wxDefaultPosition, wxSize(128,-1));
    
    wxBoxSizer *hbox0 = new wxBoxSizer(wxHORIZONTAL);
    *chk_floor = new wxCheckBox(panel, wxID_ANY, "");
    *combo_floor = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(128,-1), var_items);
    *txt_floor = new wxTextCtrl(panel, wxID_ANY, wxT("1"), wxDefaultPosition, wxSize(70,-1), 0, validator);
    hbox0->Add(*chk_floor);
    hbox0->Add(*combo_floor);
    hbox0->Add(*txt_floor);
   
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    (*slider_floor) = new wxSlider(panel, wxID_ANY, 10, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    (*txt_floor_pct) = new wxTextCtrl(panel, wxID_ANY, wxT("10%"), wxDefaultPosition, wxSize(70,-1), 0, validator);
    hbox1->Add(*slider_floor);
    hbox1->Add(*txt_floor_pct);
    
    wxBoxSizer *hbox = new wxBoxSizer(wxVERTICAL);
    hbox->Add(hbox0);
    hbox->Add(hbox1);
    
    gbox->Add(st, 0, wxALIGN_TOP| wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox, 1, wxEXPAND);
    
    (*chk_floor)->Bind(wxEVT_CHECKBOX, &AbstractClusterDlg::OnCheckMinBound, this);
    (*combo_floor)->Bind(wxEVT_CHOICE, &AbstractClusterDlg::OnSelMinBound, this);
    (*txt_floor)->Bind(wxEVT_TEXT, &AbstractClusterDlg::OnTypeMinBound, this);
	(*slider_floor)->Bind(wxEVT_SLIDER, &AbstractClusterDlg::OnSlideMinBound, this);
    (*combo_floor)->Disable();
    (*txt_floor)->Disable();
    (*slider_floor)->Disable();
    (*txt_floor_pct)->Disable();
    
}
void AbstractClusterDlg::OnSlideMinBound(wxCommandEvent& event)
{
    int idx = combo_floor->GetSelection();
    if (idx >= 0) {
        int val = slider_floor->GetValue();
        wxString t_val = wxString::Format("%d%%", val);
        txt_floor_pct->SetValue(t_val);
       
        if (idx_sum.find(idx) != idx_sum.end()) {
            double slide_val = (val / 100.0) * idx_sum[idx];
            wxString str_val;
            str_val << slide_val;
            txt_floor->SetValue(str_val);
        }
    }
}
void AbstractClusterDlg::OnCheckMinBound(wxCommandEvent& event)
{
    if (chk_floor->IsChecked() ) {
        combo_floor->Enable();
        txt_floor->Enable();
        combo_floor->SetSelection(-1);
        txt_floor->SetValue("");
    } else {
        combo_floor->Disable();
        txt_floor->Disable();
        slider_floor->Disable();
        txt_floor_pct->Disable();
        combo_floor->SetSelection(-1);
        txt_floor->SetValue("");
    }
}
void AbstractClusterDlg::OnSelMinBound(wxCommandEvent& event)
{
    int rows = project->GetNumRecords();
    int idx = combo_floor->GetSelection();
    if (idx >= 0) {
        slider_floor->Enable();
        txt_floor_pct->Enable();
        
        vector<double> floor_variable(rows, 1);
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        int tm = name_to_tm_id[combo_floor->GetString(idx)];
        table_int->GetColData(col, tm, floor_variable);
        // 10% as default to txt_floor
        double sum = 0;
        for (int i=0; i<rows; i++) {
            sum += floor_variable[i];
        }
        idx_sum[idx] = sum;
        double suggest = sum * 0.1;
        wxString str_suggest;
        str_suggest << suggest;
        txt_floor->SetValue(str_suggest);
        slider_floor->SetValue(10);
        txt_floor_pct->SetValue("10%");
    } else {
        slider_floor->Disable();
        txt_floor_pct->Disable();
        slider_floor->SetValue(10);
        txt_floor_pct->SetValue("10%");
    }
}
void AbstractClusterDlg::OnTypeMinBound(wxCommandEvent& event)
{
    wxString tmp_val = txt_floor->GetValue();
    tmp_val.Trim(false);
    tmp_val.Trim(true);
    long input_min_k;
    bool is_valid = tmp_val.ToLong(&input_min_k);
    if (is_valid) {
    }
}

void AbstractClusterDlg::InitVariableCombobox(wxListBox* var_box)
{
    var_items.Clear();
    
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
                var_items.Add(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            var_items.Add(name);
        }
    }
    
    var_box->InsertItems(var_items,0);
}

bool AbstractClusterDlg::GetInputData(int transform, int min_num_var)
{
    bool use_centroids = m_use_centroids->GetValue();
    
    if (use_centroids && m_weight_centroids->GetValue() == 0) {
        use_centroids = false;
    }
    
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = selections.size();
    if (num_var < min_num_var && !use_centroids) {
        // show message box
        wxString err_msg = wxString::Format(_("Please select at least %d variables."), min_num_var);
        wxMessageDialog dlg(NULL, err_msg, "Info", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }
    
    if ((!use_centroids && num_var>0) || (use_centroids && m_weight_centroids->GetValue() != 1)) {
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
                return false;
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
        
        rows = project->GetNumRecords();
        columns =  0;
        
        std::vector<d_array_type> data; // data[variable][time][obs]
        data.resize(col_ids.size());
        for (int i=0; i<var_info.size(); i++) {
            table_int->GetColData(col_ids[i], data[i]);
        }
        
        // if use centroids
        if (use_centroids) {
            columns += 2;
        }
        
        // get columns (if time variables show)
        for (int i=0; i<data.size(); i++ ){
            for (int j=0; j<data[i].size(); j++) {
                columns += 1;
            }
        }
        
        weight = GetWeights(columns);
        
        // init input_data[rows][cols]
        input_data = new double*[rows];
        mask = new int*[rows];
        for (int i=0; i<rows; i++) {
            input_data[i] = new double[columns];
            mask[i] = new int[columns];
            for (int j=0; j<columns; j++){
                mask[i][j] = 1;
            }
        }
        
        // assign value
        int col_ii = 0;
        
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
            col_ii = 2;
        }
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
        return true;
    }
    return false;
}

double* AbstractClusterDlg::GetWeights(int columns)
{
    double* weight = new double[columns];
    double wc = 1;
    int sel_wc = m_weight_centroids->GetValue();
    if ( sel_wc > 0 && m_use_centroids->IsChecked() ) {
        wc = sel_wc / 100.0;
        double n_var_cols = (double)(columns - 2);
        for (int j=0; j<columns; j++){
            if (j==0 || j==1)
                weight[j] = wc * 0.5;
            else {
                weight[j] = (1 - wc) / n_var_cols;
            }
        }
    } else {
        for (int j=0; j<columns; j++){
            weight[j] = 1;
        }
    }
    return weight;
}

double AbstractClusterDlg::GetMinBound()
{
    double bound = 0;
    if (chk_floor->IsChecked()) {
        wxString tmp_val = txt_floor->GetValue();
        tmp_val.ToDouble(&bound);
    }
    return bound;
}

double* AbstractClusterDlg::GetBoundVals()
{
    int rows = project->GetNumRecords();
    int idx = combo_floor->GetSelection();
    double* vals = new double[rows];
    if (idx >= 0) {
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col != wxNOT_FOUND) {
            vector<double> floor_variable(rows, 1);
            int tm = name_to_tm_id[combo_floor->GetString(idx)];
            table_int->GetColData(col, tm, floor_variable);
            for (int i=0; i<rows; i++) {
                vals[i] = floor_variable[i];
            }
        }
    }
    return vals;
}