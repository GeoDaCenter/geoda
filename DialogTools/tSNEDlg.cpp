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

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../FramesManager.h"
#include "../DataViewer/TableInterface.h"
#include "../Project.h"
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/mds.h"
#include "../Algorithms/vptree.h"
#include "../Algorithms/splittree.h"
#include "../Algorithms/tsne.h"
#include "../Explore/ScatterNewPlotView.h"
#include "../Explore/3DPlotView.h"
#include "SaveToTableDlg.h"
#include "tSNEDlg.h"

BEGIN_EVENT_TABLE( TSNEDlg, wxDialog )
EVT_CLOSE( TSNEDlg::OnClose )
END_EVENT_TABLE()

TSNEDlg::TSNEDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("t-SNE Settings"))
{
    wxLogMessage("Open tSNE Dialog.");
   
    CreateControls();
}

TSNEDlg::~TSNEDlg()
{
}

void TSNEDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(480,820), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    AddSimpleInputCtrls(panel, vbox);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(15,2,10,0);

    // perplexity
    size_t num_obs = project->GetNumRecords();
    wxString str_perplexity;
    if ((int)((num_obs - 1) /  3)  < 30) str_perplexity << (int)((num_obs - 1) /  3);
    else str_perplexity << 30;

    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Perplexity:"));
    txt_perplexity = new wxTextCtrl(panel, wxID_ANY, str_perplexity,wxDefaultPosition, wxSize(70,-1));
    txt_perplexity->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_perplexity, 1, wxEXPAND);

    // theta
    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY, _("Theta:"));
    txt_theta = new wxTextCtrl(panel, wxID_ANY, "0.5",wxDefaultPosition, wxSize(70,-1));
    txt_theta->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_theta, 1, wxEXPAND);

    // max iteration
    wxStaticText* st15 = new wxStaticText(panel, wxID_ANY, _("Max Iteration:"));
    txt_iteration = new wxTextCtrl(panel, wxID_ANY, "1000",wxDefaultPosition, wxSize(70,-1));
    txt_iteration->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st15, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_iteration, 1, wxEXPAND);

    // lr
    wxStaticText* st21 = new wxStaticText(panel, wxID_ANY, _("Learning Rate:"));
    txt_learningrate = new wxTextCtrl(panel, wxID_ANY, "200",wxDefaultPosition, wxSize(70,-1));
    txt_learningrate->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st21, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_learningrate, 1, wxEXPAND);

    // momentum
    wxStaticText* st18 = new wxStaticText(panel, wxID_ANY, _("Momentum:"));
    txt_momentum = new wxTextCtrl(panel, wxID_ANY, "0.5",wxDefaultPosition, wxSize(70,-1));
    txt_momentum->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st18, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_momentum, 1, wxEXPAND);

    // final momentum
    wxStaticText* st19 = new wxStaticText(panel, wxID_ANY, _("Final Momentum:"));
    txt_finalmomentum = new wxTextCtrl(panel, wxID_ANY, "0.8",wxDefaultPosition, wxSize(70,-1));
    txt_finalmomentum->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st19, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_finalmomentum, 1, wxEXPAND);

    // mom_switch_iter
    wxStaticText* st20 = new wxStaticText(panel, wxID_ANY, _("# Iteration Switch Momentum:"));
    txt_mom_switch_iter = new wxTextCtrl(panel, wxID_ANY, "250",wxDefaultPosition, wxSize(70,-1));
    txt_mom_switch_iter->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st20, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_mom_switch_iter, 1, wxEXPAND);
    
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);

    // seed
    wxStaticText* st27 = new wxStaticText(panel, wxID_ANY, _("Use specified seed:"));
    wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
    chk_seed = new wxCheckBox(panel, wxID_ANY, "");
    seedButton = new wxButton(panel, wxID_OK, _("Change Seed"));

    hbox17->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
    hbox17->Add(seedButton,0,wxALIGN_CENTER_VERTICAL);
    seedButton->Disable();
    gbox->Add(st27, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox17, 1, wxEXPAND);

    if (GdaConst::use_gda_user_seed) {
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);

    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("# of Dimensions:"));
    const wxString dims[2] = {"2", "3"};
    combo_n = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 2, dims);

    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(combo_n, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    
    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxALL |wxEXPAND, 10);
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
    okButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnCloseClick, this);
}


void TSNEDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close TSNEDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void TSNEDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close TSNEDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void TSNEDlg::OnSeedCheck(wxCommandEvent& event)
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
        GdaConst::use_gda_user_seed = false;
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "0");

        seedButton->Disable();
    }
}

void TSNEDlg::OnChangeSeed(wxCommandEvent& event)
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

void TSNEDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox TSNEDlg.");
    
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

wxString TSNEDlg::_printConfiguration()
{
    return "";
}

double TSNEDlg::_calculateRankCorr(char dist, int rows, double **ragged_distances,
                                  const std::vector<std::vector<double> >& result)
{
    double d;
    std::vector<double> x, y;
    for (size_t r=1; r<rows; ++r) {
        for (size_t c=0; c<r; ++c) {
            x.push_back(ragged_distances[r][c]);
            if (dist == 'b') {
                d = DataUtils::ManhattanDistance(result, r, c);
            } else {
                d = DataUtils::EuclideanDistance(result, r, c);
            }
            y.push_back(d);
        }
    }
    double r = GenUtils::RankCorrelation(x, y);
    return r;
}

double TSNEDlg::_calculateStress(char dist, int rows, double **ragged_distances, const std::vector<std::vector<double> >& result)
{
    double d, tmp;
    double sum_dist = 0;
    double sum_diff = 0;
    double stress = 0;
    for (size_t r=1; r<rows; ++r) {
        for (size_t c=0; c<r; ++c) {
            if (dist == 'b') {
                d = DataUtils::ManhattanDistance(result, r, c);
                tmp = ragged_distances[r][c] - d;
                sum_dist += ragged_distances[r][c] * ragged_distances[r][c];
            } else {
                d = DataUtils::EuclideanDistance(result, r, c);
                tmp = sqrt(ragged_distances[r][c]) - sqrt(d);
                sum_dist += ragged_distances[r][c];
            }
            tmp = tmp * tmp;
            sum_diff += tmp;
        }
    }
    stress = sum_dist == 0 ? 0 : sqrt( sum_diff/ sum_dist);
    return stress;
}
void TSNEDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click TSNEDlg::OnOK");
   
    int transform = combo_tranform->GetSelection();
   
    if (!GetInputData(transform, 1))
        return;

    double* weight = GetWeights(columns);

    double perplexity = 0;
    int suggest_perp = (int)((project->GetNumRecords() - 1) /  3);
    wxString val = txt_perplexity->GetValue();
    if (!val.ToDouble(&perplexity)) {
        wxString err_msg = _("Please input a valid numeric value for perplexity.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    if (perplexity > suggest_perp) {
        wxString err_msg = _("Perplexity parameter should not be larger than %d.");
        wxMessageDialog dlg(NULL, wxString::Format(err_msg, suggest_perp), _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double theta;
    val = txt_theta->GetValue();
    if (!val.ToDouble(&theta)) {
        wxString err_msg = _("Please input a valid numeric value for theta.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double momentum;
    val = txt_momentum->GetValue();
    if (!val.ToDouble(&momentum)) {
        wxString err_msg = _("Please input a valid numeric value for momentum.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double finalmomentum;
    val = txt_finalmomentum->GetValue();
    if (!val.ToDouble(&finalmomentum)) {
        wxString err_msg = _("Please input a valid numeric value for final momentum.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double learningrate;
    val = txt_learningrate->GetValue();
    if (!val.ToDouble(&learningrate)) {
        wxString err_msg = _("Please input a valid numeric value for learning rate.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    long max_iteration;
    val = txt_iteration->GetValue();
    if (!val.ToLong(&max_iteration)) {
        wxString err_msg = _("Please input a valid numeric value for max iterations.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    long mom_switch_iter;
    val = txt_mom_switch_iter->GetValue();
    if (!val.ToLong(&mom_switch_iter)) {
        wxString err_msg = _("Please input a valid numeric value for iterations for momentum switch.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    long out_dim = combo_n->GetSelection() == 0 ? 2 : 3;

    int transpose = 0; // row wise
    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];
  
    int new_col = out_dim;
    vector<vector<double> > results;

    double *data = new double[rows * columns];
    for (size_t i=0; i<rows; ++i) {
        for (size_t j=0; j<columns; ++j) {
            data[i*columns + j] = input_data[i][j];
        }
    }
    double* Y = new double[rows * new_col];

    int num_threads = 1;
    int verbose = 0;
#ifdef DEBUG
    verbose = 1;
#endif
    double early_exaggeration = 12;

    double final_cost;
    TSNE tsne;
    tsne.run(data, rows, columns, Y, new_col, perplexity, theta, num_threads,
             max_iteration, (int)mom_switch_iter,
            (int)GdaConst::gda_user_seed, GdaConst::use_gda_user_seed,
            verbose, early_exaggeration, learningrate, &final_cost);

    results.resize(new_col);
    for (int i=0; i<new_col; i++) {
        results[i].resize(rows);
        for (int j = 0; j < rows; ++j) {
            results[i][j] = Y[j*new_col +i];
        }
    }

    // compute rank correlation
    double **ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
    double r = _calculateRankCorr(dist, rows, ragged_distances, results);
    double stress = _calculateStress(dist, rows, ragged_distances, results);
    for (size_t i=1; i< rows; ++i) free(ragged_distances[i]);
    free(ragged_distances);

    std::vector<std::pair<wxString, double> > output_vals;
    output_vals.insert(output_vals.begin(), std::make_pair("rank correlation", r));
    output_vals.insert(output_vals.begin(), std::make_pair("final cost", final_cost));

    delete[] Y;
    delete[] data;
   
    if (!results.empty()) {

        wxString info_str;
        info_str << "t-SNE (";
        for (size_t k=0; k<col_names.size(); k++) {
            info_str << col_names[k];
            if (k < col_names.size()-1 ) info_str << ", ";
        }
        info_str << ")";
        
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
                           _("Save Results: t-SNE"),
                           wxDefaultPosition, wxSize(400,400));
        if (dlg.ShowModal() == wxID_OK) {
            // show in a scatter plot
            std::vector<int>& new_col_ids = dlg.new_col_ids;
            std::vector<wxString>& new_col_names = dlg.new_col_names;

            // at least 2 variables
            if (new_col_ids.size() < 2) return;

            int num_new_vars = new_col_ids.size();

            std::vector<GdaVarTools::VarInfo> new_var_info;
            new_var_info.resize(num_new_vars);
            
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

            if (num_new_vars == 2) {
                wxString title = _("t-SNE Plot - ") + new_col_names[0] + ", " + new_col_names[1];
            
                MDSPlotFrame* subframe =
                new MDSPlotFrame(parent, project, info_str, output_vals,
                                    new_var_info, new_col_ids,
                                    false, title, wxDefaultPosition,
                                    GdaConst::scatterplot_default_size,
                                    wxDEFAULT_FRAME_STYLE);
                wxCommandEvent ev;
                subframe->OnViewLinearSmoother(ev);
                subframe->OnDisplayStatistics(ev);

            } else if (num_new_vars == 3) {

                new_var_info[2].time = 0;
                // Set Primary GdaVarTools::VarInfo attributes
                new_var_info[2].name = new_col_names[2];
                new_var_info[2].is_time_variant = table_int->IsColTimeVariant(new_col_ids[2]);
                table_int->GetMinMaxVals(new_col_ids[2], new_var_info[2].min, new_var_info[2].max);
                new_var_info[2].sync_with_global_time = new_var_info[2].is_time_variant;
                new_var_info[2].fixed_scale = true;

                wxString title = _("t-SNE 3D Plot - ") + new_col_names[0] + ", " + new_col_names[1] + ", " + new_col_names[2];
                wxString addition_text = wxString::Format("stress: %.3f, rank correlation: %.3f", stress, r);

                C3DPlotFrame *subframe =
                new C3DPlotFrame(parent, project,
                                 new_var_info, new_col_ids,
                                 title, info_str, output_vals, wxDefaultPosition,
                                 GdaConst::three_d_default_size,
                                 wxDEFAULT_FRAME_STYLE);
            }
        }

    }
}
