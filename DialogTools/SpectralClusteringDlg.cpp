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

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/spectral.h"
#include "../Algorithms/DataUtils.h"
#include "../kNN/ANN/ANN.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "SpectralClusteringDlg.h"


BEGIN_EVENT_TABLE( SpectralClusteringDlg, wxDialog )
EVT_CLOSE( SpectralClusteringDlg::OnClose )
END_EVENT_TABLE()

SpectralClusteringDlg::SpectralClusteringDlg(wxFrame* parent_s,
                                             Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Spectral Clustering Settings"))
{
    wxLogMessage("Open SpectralClusteringDlg.");
    
    parent = parent_s;
    project = project_s;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
}

SpectralClusteringDlg::~SpectralClusteringDlg()
{
}

bool SpectralClusteringDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void SpectralClusteringDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY,
                                                  wxDefaultPosition,
                                                  wxSize(820,880),
                                                  wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    bool show_auto_button = true;
    AddInputCtrls(panel, vbox, show_auto_button);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(14,2,5,0);

    // NumberOfCluster Control
    AddNumberOfClusterCtrl(panel, gbox);
    
    // Affinity heading
    gbox->Add(new wxStaticText(panel, wxID_ANY,"Affinity"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(new wxStaticText(panel, wxID_ANY,""), 1, wxEXPAND);
    
    // Spectral controls: KNN
    int suggest_k = ceil(log10((double)num_obs));
    wxString str_k;
    str_k << suggest_k;
    lbl_knn = new wxStaticText(panel, wxID_ANY, _("        K-NN:"));
    wxBoxSizer* hbox19 = new wxBoxSizer(wxHORIZONTAL);
    chk_knn = new wxCheckBox(panel, wxID_ANY, "");
    lbl_neighbors = new wxStaticText(panel, wxID_ANY, _("# Neighors:"));
    m_knn = new wxComboBox(panel, wxID_ANY, str_k, wxDefaultPosition,
                             wxSize(80,-1), 0, NULL);
    m_knn->Append(wxString::Format("%d", suggest_k));
    // ln(n)
    m_knn->Append(wxString::Format("%d", (int)(ceil(log((double)num_obs)))));

    hbox19->Add(chk_knn);
    hbox19->Add(lbl_neighbors);
    hbox19->Add(m_knn);
    gbox->Add(lbl_knn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox19, 1, wxEXPAND);
    
    // Mutual KNN
    wxStaticText *lbl_mknn = new wxStaticText(panel, wxID_ANY, _("        Mutual K-NN:"));
    wxBoxSizer* hbox20 = new wxBoxSizer(wxHORIZONTAL);
    chk_mknn = new wxCheckBox(panel, wxID_ANY, "");
    lbl_m_neighbors = new wxStaticText(panel, wxID_ANY, _("# Neighors:"));
    m_mknn = new wxComboBox(panel, wxID_ANY, str_k, wxDefaultPosition,
                           wxSize(80,-1), 0, NULL);
    // ln(n)
    m_mknn->Append(wxString::Format("%d", suggest_k));
    m_mknn->Append(wxString::Format("%d", (int)(ceil(log((double)num_obs)))));
    hbox20->Add(chk_mknn);
    hbox20->Add(lbl_m_neighbors);
    hbox20->Add(m_mknn);
    gbox->Add(lbl_mknn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox20, 1, wxEXPAND);
    m_mknn->Enable(false);
    
	// Spectral Controls: Kernel
    lbl_kernel = new wxStaticText(panel, wxID_ANY, _("        Gaussian:"));
    wxBoxSizer* hbox18 = new wxBoxSizer(wxHORIZONTAL);
    chk_kernel = new wxCheckBox(panel, wxID_ANY, "");
    lbl_sigma = new wxStaticText(panel, wxID_ANY, _(" Sigma:"));
    m_sigma = new wxComboBox(panel, wxID_ANY, "", wxDefaultPosition,
                            wxSize(80,-1), 0, NULL);

    hbox18->Add(chk_kernel);
    hbox18->Add(lbl_sigma);
    hbox18->Add(m_sigma);
    gbox->Add(lbl_kernel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox18, 1, wxEXPAND);
    wxCommandEvent ev;
    UpdateGaussian(ev);

    // Weights (not enabled)
    lbl_weights = new wxStaticText(panel, wxID_ANY, _("Use Weights:"));
    wxBoxSizer *hbox22 = new wxBoxSizer(wxHORIZONTAL);
    chk_weights = new wxCheckBox(panel, wxID_ANY, "");
    combo_weights = new wxChoice(panel, wxID_ANY);
    chk_weights->SetValue(true);
    hbox22->Add(chk_weights);
    hbox22->Add(combo_weights);
    gbox->Add(lbl_weights, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox22, 1, wxEXPAND);

    // Transformation
    AddTransformation(panel, gbox);
    
    wxStaticText* st20 = new wxStaticText(panel, wxID_ANY, "(K-Means)");
    wxStaticText* st21 = new wxStaticText(panel, wxID_ANY, "",
                                          wxDefaultPosition, wxSize(0,-1));
    gbox->Add(st20, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(st21, 1, wxEXPAND);
    
    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY,
                                          _("Initialization Method:"));
    wxString choices16[] = {"KMeans++", "Random"};
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(160,-1), 2, choices16);
    combo_method->SetSelection(0);

    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);

    
    wxStaticText* st10 = new wxStaticText(panel, wxID_ANY,
                                          _("Initialization Re-runs:"));
    wxTextCtrl  *box10 = new wxTextCtrl(panel, wxID_ANY, "150", wxDefaultPosition, wxSize(160,-1));
    gbox->Add(st10, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box10, 1, wxEXPAND);
    
    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Use Specified Seed:"));
    wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
    chk_seed = new wxCheckBox(panel, wxID_ANY, "");
    seedButton = new wxButton(panel, wxID_OK, _("Change Seed"));
    
    hbox17->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
    hbox17->Add(seedButton,0,wxALIGN_CENTER_VERTICAL);
    seedButton->Disable();
    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox17, 1, wxEXPAND);
    
    if (GdaConst::use_gda_user_seed) {
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    
    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY,
                                          _("Maximum Iterations:"));
    wxTextCtrl  *box11 = new wxTextCtrl(panel, wxID_ANY, "300");
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box11, 1, wxEXPAND);

    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY,
                                          _("Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(160,-1), 2, choices13);
    box13->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box13, 1, wxEXPAND);

    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel,
                                                  _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);
    
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY,
                                          _("Save Cluster in Field:"));
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, "CL", wxDefaultPosition,
                                       wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel,
                                                   _("Output:"));
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    //vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
	// Summary control 
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
	wxNotebook* notebook = AddSimpleReportCtrls(panel);
	vbox1->Add(notebook, 1, wxEXPAND|wxALL,20);
    
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
    //InitVariableCombobox(box);
    
    // temp solution:
    chk_kernel->SetValue(false);
    //lbl_kernel->Disable();
    lbl_sigma->Disable();
    m_sigma->Disable();
    
    
    chk_knn->SetValue(true);
    //lbl_knn ->Enable();
    lbl_neighbors->Enable();
    m_knn->Enable();
    
    lbl_m_neighbors->Disable();
    chk_weights->SetValue(false);
    chk_weights->Disable();
    lbl_weights->Disable();
    combo_weights->Disable();
    chk_weights->Hide();
    lbl_weights->Hide();
    combo_weights->Hide();
    
    m_textbox = box3;
    m_iterations = box11;
    m_pass = box10;
    //m_method = box12;
    m_distance = box13;
    
    // Events
    chk_kernel->Bind(wxEVT_CHECKBOX, &SpectralClusteringDlg::OnKernelCheck, this);
    chk_knn->Bind(wxEVT_CHECKBOX, &SpectralClusteringDlg::OnKNNCheck, this);
    chk_mknn->Bind(wxEVT_CHECKBOX, &SpectralClusteringDlg::OnMutualKNNCheck, this);
    chk_weights->Bind(wxEVT_CHECKBOX, &SpectralClusteringDlg::OnWeightsCheck, this);
    
    okButton->Bind(wxEVT_BUTTON, &SpectralClusteringDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &SpectralClusteringDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &SpectralClusteringDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &SpectralClusteringDlg::OnChangeSeed, this);
    combo_var->Bind(wxEVT_LISTBOX, &SpectralClusteringDlg::UpdateGaussian, this);
}

void SpectralClusteringDlg::OnWeightsCheck(wxCommandEvent& event)
{
    
}

void SpectralClusteringDlg::UpdateGaussian(wxCommandEvent& event)
{
    if (m_sigma) {
        m_sigma->Clear();
        wxArrayInt selections;
        combo_var->GetSelections(selections);
        if (selections.size() > 0) {
            // sklearn: gamma = 1.0 / NV,  gamma = 1/(2sigma^2) => sigma = sqrt(1/gamma/ 2.0);
            // suggest_sigma = sqrt(NV/2.0);
            m_sigma->Append(wxString::Format("%f", sqrt(1/(double)selections.size())));
        }
        m_sigma->Append(wxString::Format("%f", log10((double)num_obs)+1));
        m_sigma->Append(wxString::Format("%f", log((double)num_obs)+1));
        m_sigma->SetSelection(0);
    }
}
void SpectralClusteringDlg::OnKernelCheck(wxCommandEvent& event)
{
    bool flag = chk_kernel->IsChecked();
    lbl_sigma->Enable(flag);
    m_sigma->Enable(flag);
    if (flag) {
        wxCommandEvent ev;
        UpdateGaussian(ev);
    }

    chk_knn->SetValue(!flag);
    lbl_neighbors->Enable(!flag);
    m_knn->Enable(!flag);

    if (flag) {
        chk_mknn->SetValue(!flag);
        lbl_m_neighbors->Enable(!flag);
        m_mknn->Enable(!flag);
    }
}

void SpectralClusteringDlg::OnKNNCheck(wxCommandEvent& event)
{
    bool flag = chk_knn->IsChecked();
    lbl_neighbors->Enable(flag);
    m_knn->Enable(flag);

    chk_mknn->SetValue(!flag);
    lbl_m_neighbors->Enable(!flag);
    m_mknn->Enable(!flag);

    if (flag) {
        chk_kernel->SetValue(!flag);
        lbl_sigma->Enable(!flag);
        m_sigma->Enable(!flag);
    }
}

void SpectralClusteringDlg::OnMutualKNNCheck(wxCommandEvent& event)
{
    bool flag = chk_mknn->IsChecked();
    lbl_m_neighbors->Enable(flag);
    m_mknn->Enable(flag);
    
    chk_knn->SetValue(!flag);
    lbl_neighbors->Enable(!flag);
    m_knn->Enable(!flag);

    if (flag) {
        chk_kernel->SetValue(!flag);
        lbl_sigma->Enable(!flag);
        m_sigma->Enable(!flag);
    }
}

void SpectralClusteringDlg::OnSeedCheck(wxCommandEvent& event)
{
    bool use_user_seed = chk_seed->GetValue();
    
    if (use_user_seed) {
        seedButton->Enable();
        if (GdaConst::use_gda_user_seed == false && GdaConst::gda_user_seed == 0) {
            OnChangeSeed(event);
            return;
        }
        GdaConst::use_gda_user_seed = true;
        
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "1");
    } else {
        seedButton->Disable();
    }
}

void SpectralClusteringDlg::OnChangeSeed(wxCommandEvent& event)
{
    // prompt user to enter user seed (used globally)
    wxString m;
    m << _("Enter a seed value for random number generator:");
    
    long long unsigned int val;
    wxString dlg_val;
    wxString cur_val;
    cur_val << GdaConst::gda_user_seed;
    
    wxTextEntryDialog dlg(NULL, m, _("Enter a seed value"), cur_val);
    if (dlg.ShowModal() != wxID_OK) return;
    dlg_val = dlg.GetValue();
    dlg_val.Trim(true);
    dlg_val.Trim(false);
    if (dlg_val.IsEmpty()) return;
    if (dlg_val.ToULongLong(&val)) {
        uint64_t new_seed_val = val;
        GdaConst::gda_user_seed = new_seed_val;
        GdaConst::use_gda_user_seed = true;
        
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        wxString str_gda_user_seed;
        str_gda_user_seed << GdaConst::gda_user_seed;
        ogr_adapt.AddEntry("gda_user_seed", str_gda_user_seed.ToStdString());
        ogr_adapt.AddEntry("use_gda_user_seed", "1");
    } else {
        wxString m = _("\"%s\" is not a valid seed. Seed unchanged.");
        m = wxString::Format(m, dlg_val);
        wxMessageDialog dlg(NULL, m, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        GdaConst::use_gda_user_seed = false;
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "0");
    }
}

void SpectralClusteringDlg::InitVariableCombobox(wxListBox* var_box)
{
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
}

void SpectralClusteringDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose SpectralClusteringDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void SpectralClusteringDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close SpectralClusteringDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString SpectralClusteringDlg::_printConfiguration()
{
    int ncluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        ncluster = (int)value_ncluster;
    }
    
    wxString txt;
    txt << _("Number of clusters:\t") << ncluster << "\n";
   
    if (chk_kernel->IsChecked())  {
        txt << _("Affinity with Gaussian Kernel:\tSigma=") << m_sigma->GetValue() << "\n";
    } else if (chk_knn->IsChecked()) {
        txt << _("Affinity with K-Nearest Neighbors:\tK=") << m_knn->GetValue() << "\n";
    } else if (chk_mknn->IsChecked()) {
        txt << _("Affinity with Mutual K-Nearest Neighbors:\tK=") << m_mknn->GetValue() << "\n";
    }
    txt << _("Transformation:\t") << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
    
    txt << _("Distance function:\t") << m_distance->GetString(m_distance->GetSelection()) << "\n";
    
    txt << "(K-Means) " << _("Initialization method:\t") << combo_method->GetString(combo_method->GetSelection()) << "\n";
    txt << "(K-Means) " << _("Initialization re-runs:\t") << m_pass->GetValue() << "\n";
    txt << "(K-Means) " << _("Maximum iterations:\t") << m_iterations->GetValue() << "\n";
    //txt << "(K-Means) Method:\t" << m_method->GetString(m_method->GetSelection()) << "\n";
    return txt;
}

bool SpectralClusteringDlg::CheckAllInputs()
{
    // get input: variables and data, and auto weights
    transform = combo_tranform->GetSelection();
    if( GetInputData(transform, 1) == false) return false;

    // get input: number of cluster
    n_cluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        n_cluster = (int)value_ncluster;
    }
    if (n_cluster < 2 || n_cluster > num_obs) {
        wxString err_msg = _("Please enter a valid number of cluster.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }

    // get input: sigma
    value_sigma = 0.018;
    double d_value_sigma;
    wxString str_sigma = m_sigma->GetValue();
    if(str_sigma.ToDouble(&d_value_sigma)) {
        value_sigma = d_value_sigma;
    }

    // get input: knn
    knn = 4;
    wxString str_knn = m_knn->GetValue();
    long value_knn;
    if(str_knn.ToLong(&value_knn)) {
        knn = (int)value_knn;
    }

    if (chk_knn->GetValue() && (knn >= num_obs || knn < 1)) {
        wxString err_msg = _("Please enter a valid number of KNN neighbors.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }

    // get input: mutual knn
    mutual_knn = 4;
    wxString str_mknn = m_mknn->GetValue();
    long value_mknn;
    if(str_mknn.ToLong(&value_mknn)) {
        mutual_knn = (int)value_mknn;
    }

    if (chk_mknn->GetValue() && (mutual_knn >= num_obs || mutual_knn < 1)) {
        wxString err_msg = _("Please enter a valid number of mutual KNN neighbors.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }

    // get input: kmeans init
    method = 'a'; // mean, 'm' median
    if (combo_method->GetSelection() == 0) method = 'b'; // mean with kmeans++

    // get input: kmeans reruns
    npass = 10;
    wxString str_pass = m_pass->GetValue();
    long value_pass;
    if(str_pass.ToLong(&value_pass)) {
        npass = (int)value_pass;
    }

    // get input: kmeans max iteration
    n_maxiter = 300; // max iteration of EM
    wxString iterations = m_iterations->GetValue();
    long l_maxiter;
    if(iterations.ToLong(&l_maxiter)) {
        n_maxiter = (int)l_maxiter;
    }

    // get input: distance
    dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];

    // get input: affinity
    affinity_type = chk_kernel->IsChecked() ? 0 : 1;

    return true;
}

bool SpectralClusteringDlg::Run(vector<wxInt64>& clusters)
{
    if (GdaConst::use_gda_user_seed) {
        setrandomstate((int)GdaConst::gda_user_seed);
        resetrandom();
    } else {
        setrandomstate(-1);
        resetrandom();
    }

    // NOTE input_data should be retrieved first!! (see CheckAllInput())
    // get input: weights (auto)
    // this function has to be called when use auto-weighting
    weight = GetWeights(columns);

    // add weight to input_data
    double** data = new double*[rows];
    for (int i=0; i<rows; i++) {
        data[i] = new double[columns];
        for (int j=0; j<columns; j++) {
            data[i][j] = input_data[i][j] * weight[j];
        }
    }

    Spectral spectral;
    spectral.set_data(data, rows, columns);
    spectral.set_centers(n_cluster);
    if (affinity_type == 0) {
        spectral.set_kernel(0);
        spectral.set_sigma(value_sigma);
    } else {
        bool is_mutual = chk_mknn->GetValue();
        int k = is_mutual ? mutual_knn : knn;
        spectral.set_knn(k, is_mutual);
    }
    spectral.set_kmeans_dist(dist);
    spectral.set_kmeans_method(method);
    spectral.set_kmeans_npass(npass);
    spectral.set_kmeans_maxiter(n_maxiter);
    spectral.cluster(affinity_type);
    clusters = spectral.get_assignments();

    for (int i=0; i<rows; i++) {
        if (data[i]) delete[] data[i];
    }
    if (data) delete[] data;

    // sort result
    std::vector<std::vector<int> > cluster_ids(n_cluster);

    for (int i=0; i < clusters.size(); i++) {
        cluster_ids[ clusters[i] - 1 ].push_back(i);
    }

    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);

    for (int i=0; i < n_cluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    return true;
}

void SpectralClusteringDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click SpectralClusteringDlg::OnOK");

    // get input: save to field name
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    if (CheckAllInputs() == false) return;

    if (Run(clusters) == false) return;
    
    // summary
    CreateSummary(clusters);
    
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

    wxString tmp = _("Spectral Clustering Map (%d clusters)");
    wxString ttl = wxString::Format(tmp, n_cluster);
    nf->SetTitle(ttl);
}
