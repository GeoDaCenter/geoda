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
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "../FramesManager.h"
#include "../DataViewer/TableInterface.h"
#include "../Project.h"
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/mds.h"
#include "../Algorithms/smacof.h"
#include "../Explore/ScatterNewPlotView.h"
#include "../Explore/3DPlotView.h"
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
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(420,700), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    AddSimpleInputCtrls(panel, vbox);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(7,2,10,0);

    // method
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Method:"));
    const wxString _methods[2] = {"classic metric", "smacof"};
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(120,-1), 2, _methods);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);

    // power iteration option approximation
    txt_usepower= new wxStaticText(panel, wxID_ANY, _("Use Power Iteration:"));
    wxBoxSizer *hbox15 = new wxBoxSizer(wxHORIZONTAL);
    chk_poweriteration = new wxCheckBox(panel, wxID_ANY, "");

    chk_poweriteration->Bind(wxEVT_CHECKBOX, &MDSDlg::OnCheckPowerIteration, this);

    hbox15->Add(chk_poweriteration);
    gbox->Add(txt_usepower, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox15, 1, wxEXPAND);
    
    // smacof
    txt_maxit = new wxStaticText(panel, wxID_ANY, _("Maximum # of Iterations:"));
    m_iterations = new wxTextCtrl(panel, wxID_ANY, "1000", wxDefaultPosition, wxSize(200,-1));
    m_iterations->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
    gbox->Add(txt_maxit, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_iterations, 1, wxEXPAND);

    if (project->GetNumRecords() < 150) {
        txt_maxit->Disable();
        m_iterations->Disable();
    } else {
        chk_poweriteration->SetValue(true);
    }

    txt_eps = new wxStaticText(panel, wxID_ANY, _("Convergence Criterion:"));
    m_eps = new wxTextCtrl(panel, wxID_ANY, "0.000001", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(txt_eps, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_eps, 1, wxEXPAND);

    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);

    // Output
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("# of Dimensions:"), wxDefaultPosition, wxSize(160,-1));
    const wxString dims[2] = {"2", "3"};
    combo_n = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 2, dims);

    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    hbox1->Add(st1, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(combo_n, 1, wxEXPAND);

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
    vbox->Add(hbox1, 0, wxEXPAND | wxALL, 10);
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
    combo_method->Bind(wxEVT_CHOICE, &MDSDlg::OnMethodChoice, this);

    wxCommandEvent evt;
    OnMethodChoice(evt);
}

void MDSDlg::OnMethodChoice(wxCommandEvent &event)
{
    bool flag = combo_method->GetSelection() == 0 ? false : true;

    m_iterations->Enable(flag);
    m_eps->Enable(flag);
    m_distance->Enable(flag);
    txt_maxit->Enable(flag);
    m_iterations->Enable(flag);
    txt_eps->Enable(flag);
    if (flag) chk_poweriteration->SetValue(false);

    chk_poweriteration->Enable(!flag);
    txt_usepower->Enable(!flag);
    if (!flag) m_distance->SetSelection(0);
}

void MDSDlg::OnCheckPowerIteration(wxCommandEvent& event)
{
    if (chk_poweriteration->IsChecked()) {
        m_iterations->Enable();
        txt_maxit->Enable();
    } else {
        m_iterations->Disable();
        txt_maxit->Disable();
    }
}

void MDSDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close MDSDlg");
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
    wxLogMessage("InitVariableCombobox MDSDlg.");
    
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

double MDSDlg::_calculateRankCorr(char dist, int rows, double **ragged_distances,
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

double MDSDlg::_calculateStress(char dist, int rows, double **ragged_distances, const std::vector<std::vector<double> >& result)
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

    wxString str_iterations = m_iterations->GetValue();
    long n_iter;
    if (!str_iterations.ToLong(&n_iter)) {
        wxString err_msg = _("Please enter a valid number for maximum number of iterations.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    wxString str_eps = m_eps->GetValue();
    double eps;
    if (!str_eps.ToDouble(&eps)) {
        wxString err_msg = _("Please enter a valid value for convergence criterion.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    int new_col = combo_n->GetSelection() == 0 ? 2 : 3;
    vector<vector<double> > results;
    double stress = 0;
    int itel = 0;
    std::vector<std::pair<wxString, double> > output_vals;

    double **ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);

    if (combo_method->GetSelection() == 1) {
        // column-wise lower-triangle matrix for SMACOF
        size_t idx = 0;
        double *delta = new double[rows * (rows-1)/2];
        for (size_t i=0; i< rows-1; ++i) { // col idx
            for (size_t j=1+i; j < rows; ++j) { // row idx
                delta[idx] = ragged_distances[j][i];
                idx += 1;
            }
        }
        int m = (int)idx;

        // init random xold for smacof
        boost::mt19937 rng((const unsigned  int)GdaConst::gda_user_seed);
        boost::uniform_01<boost::mt19937> X(rng);
        double *xold = new double[m * new_col];
        for (size_t i=0; i< m * new_col; ++i) {
            xold[i] =  X();
        }

        double *xnew;
        stress = runSmacof(delta, m, new_col, (int)n_iter, eps, xold, &itel, &xnew);
        delete[] delta;

        results.resize(new_col);
        for (size_t i=0; i<new_col; ++i) {
            for (size_t j=0; j<rows; ++j) {
                results[i].push_back(xnew[j + i*rows]);
            }
        }
        for (size_t i=0; i<new_col; ++i) {
            GenUtils::StandardizeData(results[i]);
        }
        free(xnew);

        output_vals.push_back(std::make_pair("iterations", itel));
        output_vals.push_back(std::make_pair("/", n_iter));
    } else {
        if (chk_poweriteration->IsChecked()) {
            // classical MDS with power iteration and full matrix
            vector<vector<double> > distances = DataUtils::copyRaggedMatrix(ragged_distances, rows, rows);
            if (dist == 'b') {
                for (size_t i=0; i<distances.size(); i++) {
                    for (int j=0; j<distances.size(); j++) {
                        distances[i][j] = distances[i][j]*distances[i][j];
                        distances[i][j] = distances[i][j]*distances[i][j];
                    }
                }
            }
            wxString str_iterations;
            str_iterations = m_iterations->GetValue();
            long l_iterations = 0;
            str_iterations.ToLong(&l_iterations);
            FastMDS mds(distances, new_col, (int)l_iterations);
            results = mds.GetResult();

        } else {
            // classical MDS
            results.resize(new_col);
            for (size_t i=0; i<new_col; i++) results[i].resize(rows);
            double **rst = mds(rows, columns, input_data,  mask, weight, transpose, dist,  ragged_distances, new_col);
            for (size_t i=0; i<new_col; i++) {
                for (size_t j = 0; j < rows; ++j) {
                    results[i][j] = rst[j][i];
                }
            }
            for (size_t j = 0; j < rows; ++j) delete[] rst[j];
            delete[] rst;
        }
    }

    stress = _calculateStress(dist, rows, ragged_distances, results);
    double r = _calculateRankCorr(dist, rows, ragged_distances, results);

    output_vals.insert(output_vals.begin(), std::make_pair("rank correlation", r));
    output_vals.insert(output_vals.begin(), std::make_pair("stress value", stress));

    // clean distance matrix
    for (size_t i=1; i< rows; ++i) free(ragged_distances[i]);
    free(ragged_distances);

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
                           _("Save Results: MDS"),
                           wxDefaultPosition, wxSize(400,400));
        wxString method_str = combo_method->GetStringSelection();
        std::vector<wxString> info_str;
        for (size_t k=0; k<col_names.size(); k++) {
            info_str.push_back(col_names[k]);
        }
        
        if (dlg.ShowModal() == wxID_OK) {
            // show in a scatter plot
            std::vector<int>& new_col_ids = dlg.new_col_ids;
            std::vector<wxString>& new_col_names = dlg.new_col_names;

            // at least 2 variables
            if (new_col_ids.size() < 2) return;

            size_t num_new_vars = new_col_ids.size();

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
                wxString title = _("MDS Plot (%s) - %s, %s");
                title = wxString::Format(title, method_str, new_col_names[0], new_col_names[1]);

                MDSPlotFrame* subframe =
                new MDSPlotFrame(parent, project, info_str, output_vals,
                                    new_var_info, new_col_ids,
                                    false, title, wxDefaultPosition,
                                    GdaConst::scatterplot_default_size,
                                    wxDEFAULT_FRAME_STYLE);

            } else if (num_new_vars == 3) {

                new_var_info[2].time = 0;
                // Set Primary GdaVarTools::VarInfo attributes
                new_var_info[2].name = new_col_names[2];
                new_var_info[2].is_time_variant = table_int->IsColTimeVariant(new_col_ids[2]);
                table_int->GetMinMaxVals(new_col_ids[2], new_var_info[2].min, new_var_info[2].max);
                new_var_info[2].sync_with_global_time = new_var_info[2].is_time_variant;
                new_var_info[2].fixed_scale = true;

                wxString title = _("MDS 3D Plot (%s) - %s, %s, %s");
                title = wxString::Format(title, method_str, new_col_names[0], new_col_names[1], new_col_names[2]);

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
