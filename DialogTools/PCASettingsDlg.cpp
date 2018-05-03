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

PCASettingsDlg::PCASettingsDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("PCA Settings"))
{
    wxLogMessage("Open PCASettingsDlg.");
    
    CreateControls();
}

PCASettingsDlg::~PCASettingsDlg()
{
}

void PCASettingsDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(820,620), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // input
    AddSimpleInputCtrls(panel, vbox);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,10,0);
    
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Method:"),
                                          wxDefaultPosition, wxSize(120,-1));
    const wxString _methods[2] = {"SVD", "Eigen"};
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(120,-1), 2, _methods);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Components:"));
    combo_n = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 0, NULL);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    hbox1->Add(st1, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(combo_n, 1, wxEXPAND);
    
    
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
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    m_textbox = new SimpleReportTextCtrl(panel, XRCID("ID_TEXTCTRL"), "");
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
    
    saveButton->Enable(false);

    combo_method->SetSelection(0);
    combo_tranform->SetSelection(2);
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &PCASettingsDlg::OnOK, this);
    saveButton->Bind(wxEVT_BUTTON, &PCASettingsDlg::OnSave, this);
    closeButton->Bind(wxEVT_BUTTON, &PCASettingsDlg::OnCloseClick, this);
}

void PCASettingsDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close PCASettingsDlg");
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
    
    for (int j = 0; j < new_col; ++j) {
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

wxString PCASettingsDlg::_printConfiguration()
{
    return wxEmptyString;
}

void PCASettingsDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click PCASettingsDlg::OnOK");
    
    int transform = combo_tranform->GetSelection();
    
    if (!GetInputData(transform,1))
        return;
   
    int max_sel_name_len = 0;
    for (int i=0; i<col_names.size(); i++) {
        if (col_names[i].length() > max_sel_name_len) {
            max_sel_name_len = col_names[i].length();
        }
    }
    

    if (rows < columns) {
        wxString msg = _("SVD will be automatically used for PCA since the number of rows is less than the number of columns.");
        wxMessageDialog dlg(NULL, msg, "Information", wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        combo_method->SetSelection(0);
    }
    
    int pca_method = combo_method->GetSelection();
    
    Pca pca(input_data, rows, columns);
    int init_result = 0;
    
    if (pca_method == 0)
        init_result = pca.CalculateSVD();
    else
        init_result = pca.Calculate();
    
    if (0 != init_result) {
        wxString msg = _("There is an error during PCA calculation. Please check if the data is valid.");
        wxMessageDialog dlg(NULL, msg, _("Error"), wxOK | wxICON_ERROR);
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
    pca_log << "---\n\nPCA method: " << method;
    
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
        pca_log << wxString::Format("%-*s", max_sel_name_len+4, col_names[k]) << items[k] << "\n";
    }
    
    if (scores.size() != nrows * ncols) {
        row_lim = (nrows < ncols)? nrows : ncols,
        col_lim = (ncols < nrows)? ncols : nrows;
    } else {
        row_lim = nrows;
        col_lim = ncols;
    }
    
    //https://stats.stackexchange.com/questions/143905/loadings-vs-eigenvectors-in-pca-when-to-use-one-or-another
    /*
    pca_log << "\n\nRotated data: \n";
    for (unsigned int i = 0; i < row_lim; ++i) {
        for (unsigned int j = 0; j < col_lim; ++j) {
            pca_log << scores[j + col_lim*i] << "\t";
        }
        pca_log << "\n";
    }
    */
    // squared correlations for PCA
    pca_log << "\n\nSquared correlations:\n";
    
    int num_pc = col_lim;
    vector<vector<double> > pc_data(num_pc);
    for (int i=0; i<col_lim; i++ ) {
        pc_data[i].resize(row_lim);
        for (int j=0; j<row_lim; j++ ) {
            pc_data[i][j] = scores[i + col_lim*j];
        }
    }
    
    vector<int> col_size(num_pc, 0);
    vector<vector<wxString> > corr_matrix(columns);
    double corr, corr_sqr;
    for (int i=0; i<columns; i++) {
        vector<double> col_data;
        for (int j=0; j<rows; j++) col_data.push_back(input_data[j][i]);
        corr_matrix[i].resize(num_pc);
        for (int j=0; j<num_pc; j++) {
            corr = GenUtils::Correlation(col_data, pc_data[j]);
            corr_sqr = corr * corr;
            wxString tmp;
            tmp << corr_sqr;
            if (tmp.length() > col_size[j]) {
                col_size[j] = tmp.length();
            }
            corr_matrix[i][j] << corr_sqr;
        }
    }
    
    pca_log << wxString::Format("%-*s", max_sel_name_len+4, "");
    for (int j=0; j<num_pc; j++) {
        wxString ttl;
        ttl << "PC" << j+1;
        pca_log << wxString::Format("%*s", col_size[j]+4, ttl);
    }
    pca_log << "\n";
    
    for (int i=0; i<columns; i++) {
        pca_log << wxString::Format("%-*s", max_sel_name_len+4, col_names[i]);
        for (int j=0; j<num_pc; j++) {
            pca_log <<   wxString::Format("%*s", col_size[j]+4, corr_matrix[i][j]);
        }
        pca_log << "\n";
    }
    pca_log << "\n";
    
    pca_log << m_textbox->GetValue();
    m_textbox->SetValue(pca_log);
    
    combo_n->Clear();
    for (int i=0; i<col_lim; i++){
        combo_n->Append(wxString::Format("%d", i+1));
    }
    combo_n->SetSelection((int)thresh95 -1);
    
    saveButton->Enable(true);
}
