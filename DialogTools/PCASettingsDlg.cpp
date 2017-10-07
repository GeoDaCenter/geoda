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



#include <fstream>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/panel.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/notebook.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/hyperlink.h>

#include "../logger.h"
#include "../FramesManager.h"
#include "../Project.h"
#include "../Algorithms/pca.h"

#include "SaveToTableDlg.h"
#include "PCASettingsDlg.h"

BEGIN_EVENT_TABLE( PCASettingsDlg, wxDialog )
EVT_CLOSE( PCASettingsDlg::OnClose )
END_EVENT_TABLE()

PCASettingsDlg::PCASettingsDlg(Project* project_s)
: wxDialog(NULL, -1, _("PCA Settings"), wxDefaultPosition, wxSize(860, 600), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
frames_manager(project_s->GetFramesManager())
{
    wxLogMessage("Open PCASettingsDlg.");
    
    project = project_s;
   
    if (project_s->GetTableInt()->GetNumberCols() == 0) {
        wxString err_msg = _("No numeric variables found in table.");
        wxMessageDialog dlg(NULL, err_msg, "Warning", wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }

    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }

    frames_manager->registerObserver(this);
}

PCASettingsDlg::~PCASettingsDlg()
{
    frames_manager->removeObserver(this);
}

void PCASettingsDlg::update(FramesManager* o)
{
}

bool PCASettingsDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void PCASettingsDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(820,620), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
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
    
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Method:"),
                                          wxDefaultPosition, wxSize(120,-1));
    const wxString _methods[2] = {"SVD", "Eigen"};
    wxChoice* box0 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(120,-1), 2, _methods);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box0, 1, wxEXPAND);
    
    
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
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Components:"));
    wxChoice* box1 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(120,-1), 0, NULL);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    hbox1->Add(st1, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box1, 1, wxEXPAND);
    
    
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
    vbox->Add(hbox1, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    m_textbox = new SimpleReportTextCtrl(panel, XRCID("ID_TEXTCTRL"), "", wxDefaultPosition, wxSize(320,430));
    
    if (GeneralWxUtils::isWindows()) {
        wxFont font(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
    } else {
        wxFont font(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
        
    }
    vbox1->Add(m_textbox, 1, wxEXPAND|wxALL,20);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    container->Add(vbox1,1, wxEXPAND | wxALL);
    
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
    
    // Content
    InitVariableCombobox(box);
    
    saveButton->Enable(false);
    combo_var = box;
    combo_n = box1;
    
    combo_method = box0;
    combo_transform = box01;
    
    combo_method->SetSelection(0);
    combo_transform->SetSelection(2);
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &PCASettingsDlg::OnOK, this);
    saveButton->Bind(wxEVT_BUTTON, &PCASettingsDlg::OnSave, this);
    closeButton->Bind(wxEVT_BUTTON, &PCASettingsDlg::OnCloseClick, this);
    
    combo_method->Connect(wxEVT_CHOICE,
                          wxCommandEventHandler(PCASettingsDlg::OnMethodChoice),
                          NULL, this);
    
}

void PCASettingsDlg::OnMethodChoice(wxCommandEvent& event)
{
    /*
     if (combo_method->GetSelection() == 0) {
     combo_transform->Enable();
     } else if (combo_method->GetSelection() == 1) {
     combo_transform->SetSelection(2);
     combo_transform->Disable();
     }
     */
}

void PCASettingsDlg::InitVariableCombobox(wxListBox* var_box)
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
    if (!items.IsEmpty())
        var_box->InsertItems(items,0);
}

void PCASettingsDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close HClusterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void PCASettingsDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close PCASettingsDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void PCASettingsDlg::OnSave(wxCommandEvent& event )
{
    wxLogMessage("OnSave PCASettingsDlg.");
    
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

void PCASettingsDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click PCASettingsDlg::OnOK");
    
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
    std::vector<std::vector<double> > data;
    data.resize(num_var);
    
    var_info.resize(num_var);
    
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString sel_name = combo_var->GetString(idx);
        
        sel_names.Add(sel_name);
        if (sel_name.length() > max_sel_name_len) {
            max_sel_name_len = sel_name.length();
        }
        
        wxString nm = name_to_nm[sel_name];
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm); wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        int tm = name_to_tm_id[sel_name];
        
        data[i].resize(rows);
        
        table_int->GetColData(col, tm, data[i]);
    }
    
    // Call function to set all Secondary Attributes based on Primary Attributes
    //GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
    
    
    vector<float> vec;
    
    for (int k=0; k< rows;k++) {
        for (int i=0; i<data.size(); i++ ){
            vec.push_back(data[i][k]);
        }
    }
    
    Pca pca;
    
    bool is_corr = combo_method->GetSelection() == 1;
    
    bool is_center = false;
    bool is_scale = false;
    
    if (combo_transform->GetSelection() == 1) {
        is_center = true;
        is_scale = false;
        
    } else if (combo_transform->GetSelection() == 2) {
        is_center = true;
        is_scale = true;
    }
    
    if (rows < columns && is_corr == true) {
        wxString msg = _("SVD will be automatically used for PCA since the number of rows is less than the number of columns.");
        wxMessageDialog dlg(NULL, msg, "Information", wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        combo_method->SetSelection(0);
        is_corr = false;
    }
    
    int init_result = pca.Calculate(vec, rows, columns, is_corr, is_center, is_scale);
    if (0 != init_result) {
        wxString msg = _("There is an error during PCA calculation. Please check if the data is valid.");
        wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    vector<float> sd = pca.sd();
    vector<float> prop_of_var = pca.prop_of_var();
    vector<float> cum_prop = pca.cum_prop();
    scores = pca.scores();
    
    vector<unsigned int> el_cols = pca.eliminated_columns();
    
    float kaiser = pca.kaiser();
    thresh95 = pca.thresh95();
    
    unsigned int ncols = pca.ncols();
    unsigned int nrows = pca.nrows();
    
    wxString method = pca.method();
    
    wxString pca_log;
    //pca_log << "\n\nPCA method: " << method;
    
    pca_log << "\n\nStandard deviation:\n";
    for (int i=0; i<sd.size();i++) pca_log << sd[i] << " ";
    
    pca_log << "\n\nProportion of variance:\n";
    for (int i=0; i<prop_of_var.size();i++) pca_log << prop_of_var[i] << " ";
    
    pca_log << "\n\nCumulative proportion:\n";
    for (int i=0; i<cum_prop.size();i++) pca_log << cum_prop[i] << " ";
    
    pca_log << "\n\nKaiser criterion: " << kaiser;
    pca_log << "\n\n95% threshold criterion: " << thresh95;
    
    // Add the correlation matrix between the original variables and the principal components
    
    pca_log << "\n\nEigenvalues:\n";
    std::stringstream ss;
    ss << pca.eigen_values;
    pca_log << ss.str();
    
    //pca_log << pca.eigen_values;
    pca_log << "\n\nEigenvectors/Variable Loadings:\n";
    // Loadings=Eigenvectors⋅ Square root of (Absolute Eigen values)
    
    std::stringstream ss1;
    ss1 << pca.eigen_vectors;
    wxString loadings =  ss1.str();
    wxStringTokenizer tokenizer(loadings, "\n");
    wxArrayString items;
    bool header = false;
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        // process token here
        items.Add(token);
        
        if (header == false) {
            pca_log << wxString::Format("%-*s", max_sel_name_len+4, "");
            int n_len = token.length();
            int pos = 0;
            bool start = false;
            int  sub_len = 0;
            int pc_idx = 1;
            
            while (pos < n_len){
                if ( start && sub_len > 0 && (token[pos] == ' ' || pos == n_len-1) ) {
                    // end of a number
                    pca_log << wxString::Format("%*s%d", sub_len-1, "PC", pc_idx++);
                    sub_len = 1;
                    start = false;
                } else {
                    if (!start && token[pos] != ' ') {
                        start = true;
                    }
                    sub_len += 1;
                }
                pos += 1;
            }
            header = true;
            pca_log << "\n";
        }
    }
    
    for (int k=0; k<items.size();k++) {
        pca_log << wxString::Format("%-*s", max_sel_name_len+4, sel_names[k]) << items[k] << "\n";
    }
    
    
    
    if (scores.size() != nrows * ncols) {
        row_lim = (nrows < ncols)? nrows : ncols,
        col_lim = (ncols < nrows)? ncols : nrows;
    } else {
        row_lim = nrows;
        col_lim = ncols;
    }
    
    //https://stats.stackexchange.com/questions/143905/loadings-vs-eigenvectors-in-pca-when-to-use-one-or-another
    
    
    pca_log << "\n\nRotated data: \n";
    for (unsigned int i = 0; i < row_lim; ++i) {
        for (unsigned int j = 0; j < col_lim; ++j) {
            pca_log << scores[j + col_lim*i] << "\t";
        }
        pca_log << "\n";
    }
    
    m_textbox->SetValue(pca_log);
    
    combo_n->Clear();
    for (int i=0; i<col_lim; i++){
        combo_n->Append(wxString::Format("%d", i+1));
    }
    combo_n->SetSelection((int)thresh95 -1);
    
    saveButton->Enable(true);
}
