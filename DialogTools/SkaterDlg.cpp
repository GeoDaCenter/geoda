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
#include "../Algorithms/maxp.h"
#include "../Algorithms/skater.h"

#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "SkaterDlg.h"


BEGIN_EVENT_TABLE( SkaterDlg, wxDialog )
EVT_CLOSE( SkaterDlg::OnClose )
END_EVENT_TABLE()

SkaterDlg::SkaterDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Skater Settings"))
{
    wxLogMessage("Open Skater dialog.");
    CreateControls();
}

SkaterDlg::~SkaterDlg()
{
    wxLogMessage("On SkaterDlg::~SkaterDlg");
}

void SkaterDlg::CreateControls()
{
    wxLogMessage("On SkaterDlg::CreateControls");
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(800,780), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables (for intra-regional homogeneity)"), wxDefaultPosition, wxDefaultSize);
    
    combo_var = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(250,250), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, "Input:");
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(combo_var, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);

    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"),
                                          wxDefaultPosition, wxSize(128,-1));
    m_max_region = new wxTextCtrl(panel, wxID_ANY, wxT("4"), wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_max_region, 1, wxEXPAND);
    
	// Weights Control
    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY, _("Weights:"), wxDefaultPosition, wxSize(128,-1));
    combo_weights = new wxChoice(panel, wxID_ANY, wxDefaultPosition,  wxSize(200,-1));
    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_weights, 1, wxEXPAND);
   
	// Minimum Bound Control
    AddMinBound(panel, gbox);

    // Min regions
    st_minregions = new wxStaticText(panel, wxID_ANY, _("Min Region Size:"), wxDefaultPosition, wxSize(128,-1));
    txt_minregions = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(200,-1));
    txt_minregions->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    gbox->Add(st_minregions, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_minregions, 1, wxEXPAND);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);

    AddTransformation(panel, gbox);
    
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
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    st17->Hide();
    chk_seed->Hide();
    seedButton->Hide();
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"),
                                         wxDefaultPosition, wxDefaultSize);
    m_textbox = new wxTextCtrl(panel, wxID_ANY, wxT("CL"), wxDefaultPosition, wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(m_textbox, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
	// Summary control 
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
	wxNotebook* notebook = AddSimpleReportCtrls(panel);
	vbox1->Add(notebook, 1, wxEXPAND|wxALL,20);

    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    container->Add(vbox1, 1, wxEXPAND | wxALL);
    
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
    InitVariableCombobox(combo_var);
  
    // init weights
    vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    
    size_t sel_pos=0;
    for (size_t i=0; i<weights_ids.size(); ++i) {
        combo_weights->Append(w_man_int->GetShortDispName(weights_ids[i]));
        if (w_man_int->GetDefault() == weights_ids[i])
            sel_pos = i;
    }
    if (weights_ids.size() > 0) combo_weights->SetSelection(sel_pos);
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &SkaterDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &SkaterDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &SkaterDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &SkaterDlg::OnChangeSeed, this);

}

void SkaterDlg::OnCheckMinBound(wxCommandEvent& event)
{
    wxLogMessage("On SkaterDlg::OnLISACheck");
    AbstractClusterDlg::OnCheckMinBound(event);
   
    if (chk_floor->IsChecked()) {
        st_minregions->Disable();
        txt_minregions->Disable();
    } else {
        st_minregions->Enable();
        txt_minregions->Enable();
    }
}

void SkaterDlg::OnSeedCheck(wxCommandEvent& event)
{
    wxLogMessage("On SkaterDlg::OnSeedCheck");
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

void SkaterDlg::OnChangeSeed(wxCommandEvent& event)
{
    wxLogMessage("On SkaterDlg::OnChangeSeed");
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

void SkaterDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("On SkaterDlg::InitVariableCombobox");
    wxArrayString items;
   
    int cnt_floor = 0;
    std::vector<int> col_id_map;
    table_int->FillNumericColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        GdaConst::FieldType ftype = table_int->GetColType(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = name;
                name_to_tm_id[nm] = t;
                items.Add(nm);
                combo_floor->Insert(nm, cnt_floor++);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            items.Add(name);
            combo_floor->Insert(name, cnt_floor++);
        }
    }
    
    if (!items.IsEmpty())
        var_box->InsertItems(items,0);
    
    combo_floor->SetSelection(-1);
    txt_floor->SetValue("");
}

void SkaterDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose SkaterDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void SkaterDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close SkaterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString SkaterDlg::_printConfiguration()
{
    wxString txt;
    
    txt << "Number of clusters:\t" << m_max_region->GetValue() << "\n";
    
    txt << "Weights:\t" << combo_weights->GetString(combo_weights->GetSelection()) << "\n";
    
    if (chk_floor && chk_floor->IsChecked()) {
        int idx = combo_floor->GetSelection();
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        txt << "Minimum bound:\t" << txt_floor->GetValue() << "(" << nm << ")" << "\n";
    } else {
        txt << "Minimum region size:\t" << txt_minregions->GetValue() << "\n";
    }
    
    txt << "Distance function:\t" << m_distance->GetString(m_distance->GetSelection()) << "\n";
    
    txt << "Transformation:\t" << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
    
    return txt;
}

void SkaterDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click SkaterDlg::OnOK");
   
    if (GdaConst::use_gda_user_seed) {
        setrandomstate(GdaConst::gda_user_seed);
        resetrandom();
    } else {
        setrandomstate(-1);
        resetrandom();
    }
    
    // Get input data
    int transform = combo_tranform->GetSelection();
	bool success = GetInputData(transform, 1);
    if (!success) {
        return;
    }
    
    wxString str_initial = m_max_region->GetValue();
    if (str_initial.IsEmpty()) {
        wxString err_msg = _("Please enter number of regions");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
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
    
	// Get Distance Selection
    int transpose = 0; // row wise
    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];
    
    // Weights selection
    vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    int sel = combo_weights->GetSelection();
    if (sel < 0) sel = 0;
    if (sel >= weights_ids.size()) sel = weights_ids.size()-1;
    boost::uuids::uuid w_id = weights_ids[sel];
    GalWeight* gw = w_man_int->GetGal(w_id);
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    // Check connectivity
    if (!CheckConnectivity(gw)) {
        wxString msg = _("The connectivity of selected spatial weights is incomplete, please adjust the spatial weights.");
        wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING );
        dlg.ShowModal();
    }
    
	// Get Bounds
    bool check_floor = false;
    double min_bound = GetMinBound();
    if (chk_floor->IsChecked()) {
        wxString str_floor = txt_floor->GetValue();
        if (str_floor.IsEmpty()) {
            wxString err_msg = _("Please enter minimum bound value");
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        check_floor = true;
    }
    
    double* bound_vals = GetBoundVals();
    if (bound_vals == NULL) {
        wxString str_min_regions = txt_minregions->GetValue();
        long val_min_regions;
        if (str_min_regions.ToLong(&val_min_regions)) {
            min_bound = val_min_regions;
            check_floor = true;
        }
        bound_vals = new double[rows];
        for (int i=0; i<rows; i++)
            bound_vals[i] = 1;
    }
    
	// Get region numbers
    int initial = 0;
    long value_initial;
    if(str_initial.ToLong(&value_initial)) {
        initial = value_initial;
    }
    
	// Get random seed
    int rnd_seed = -1;
    if (chk_seed->GetValue()) rnd_seed = GdaConst::gda_user_seed;
    
    // Create distance matrix using weights
    vector<vector<double> > distances(rows);
    double** ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
    for (int i = 0; i < rows; i++) {
        distances[i].resize(rows);
        for (int j = 0; j < rows; j++)
            distances[i][j] = -1;
    }
    for (int i=0; i< rows; i++) {
        for (int j=0; j< gw->gal[i].Size(); j++) {
            int k = gw->gal[i][j];
            distances[i][k] = 0;
        }
    }
    for (int i = 1; i < rows; i++) {
        for (int j = 0; j < i; j++) {
            if (distances[i][j] == 0)
                distances[i][j] = sqrt(ragged_distances[i][j]);
            if (distances[j][i] == 0)
                distances[j][i] = sqrt(ragged_distances[i][j]);
        }
    }
    for (int i = 1; i < rows; i++) free(ragged_distances[i]);
    free(ragged_distances);
    
	// Run Skater
    Skater skater(rows, columns, initial, input_data, distances, check_floor, min_bound, bound_vals);
    
	delete[] bound_vals;

    vector<vector<int> > cluster_ids = skater.GetRegions();
    int ncluster = cluster_ids.size();
    
    vector<wxInt64> clusters(rows, 0);
    vector<bool> clusters_undef(rows, false);

    // sort result
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    for (int i=0; i < ncluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
   
    // check not clustered
    int n_island = 0;
    for (int i=0; i<clusters.size(); i++) {
        if (clusters[i] == 0) {
            n_island++;
        }
    }
    
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
    wxString ttl;
    ttl << "Skater Cluster Map (";
    ttl << ncluster;
    ttl << " clusters)";
    nf->SetTitle(ttl);
    
    if (n_island>0) {
        nf->SetLegendLabel(0, "Not Clustered");
    }
}
