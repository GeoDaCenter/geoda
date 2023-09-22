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
#include <wx/textfile.h>

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "RedcapDlg.h"

using namespace SpanningTreeClustering;

BEGIN_EVENT_TABLE( RedcapDlg, wxDialog )
EVT_CLOSE( RedcapDlg::OnClose )
END_EVENT_TABLE()

RedcapDlg::RedcapDlg(wxFrame* parent_s, Project* project_s)
: redcap(NULL), AbstractClusterDlg(parent_s, project_s,  _("REDCAP Settings"))
{
    wxLogMessage("Open REDCAP dialog.");
    
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

RedcapDlg::~RedcapDlg()
{
    wxLogMessage("On RedcapDlg::~RedcapDlg");
    frames_manager->removeObserver(this);
    if (redcap) {
        delete redcap;
        redcap = NULL;
    }
}

bool RedcapDlg::Init()
{
    wxLogMessage("On RedcapDlg::Init");
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void RedcapDlg::CreateControls()
{
    wxLogMessage("On RedcapDlg::CreateControls");
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(820,780), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    AddSimpleInputCtrls(panel, vbox, false, true/*show spatial weights control*/);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(11,2,5,0);

    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("Number of Regions:"));
    m_max_region = new wxTextCtrl(panel, wxID_ANY, "5", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_max_region, 1, wxEXPAND);

    wxStaticText* st20 = new wxStaticText(panel, wxID_ANY, _("Method:"));
    wxString choices20[] = {"FirstOrder-SingleLinkage", "FullOrder-WardLinkage",  "FullOrder-AverageLinkage", "FullOrder-CompleteLinkage",  "FullOrder-SingleLinkage" };
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 5, choices20);
    combo_method->SetSelection(1);
    
    gbox->Add(st20, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);
    
    // Minimum Bound Control
    AddMinBound(panel, gbox);

    // Min regions
    st_minregions = new wxStaticText(panel, wxID_ANY, _("Min Region Size:"));
    txt_minregions = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(200,-1));
    txt_minregions->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    gbox->Add(st_minregions, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_minregions, 1, wxEXPAND);

    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {_("Euclidean"), _("Manhattan")};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);

    // Transformation
    AddTransformation(panel, gbox);

    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Use Specified Seed:"));
    wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
    chk_seed = new wxCheckBox(panel, wxID_ANY, "");
    seedButton = new wxButton(panel, wxID_OK, _("Change Seed"));
    st17->Hide();
    chk_seed->Hide();
    seedButton->Hide();
    hbox17->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
    hbox17->Add(seedButton,0,wxALIGN_CENTER_VERTICAL);
    seedButton->Disable();
    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox17, 1, wxEXPAND);
    
    if (GdaConst::use_gda_user_seed) {
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"));
    m_textbox = new wxTextCtrl(panel, wxID_ANY, "CL", wxDefaultPosition, wxSize(158,-1));
    chk_save_mst = new wxCheckBox(panel, wxID_ANY, "Save Complete Spanning Tree");

    wxFlexGridSizer* gbox_out = new wxFlexGridSizer(2,2,5,0);
    gbox_out->Add(st3, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox_out->Add(m_textbox, 1, wxEXPAND);
    gbox_out->Add(new wxStaticText(panel, wxID_ANY, _("(Optional)")),
                  0, wxALIGN_RIGHT | wxRIGHT | wxLEFT, 10);
    gbox_out->Add(chk_save_mst, 1, wxEXPAND);

    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    hbox1->Add(gbox_out, 1, wxEXPAND);

    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    saveButton = new wxButton(panel, wxID_OK, _("Save Spanning Tree"), wxDefaultPosition,
                                      wxSize(140, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(saveButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 5);
    saveButton->Disable();
    
    // Container
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

    // Events
    okButton->Bind(wxEVT_BUTTON, &RedcapDlg::OnOK, this);
    saveButton->Bind(wxEVT_BUTTON, &RedcapDlg::OnSaveTree, this);
    closeButton->Bind(wxEVT_BUTTON, &RedcapDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &RedcapDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &RedcapDlg::OnChangeSeed, this);
}

void RedcapDlg::OnCheckMinBound(wxCommandEvent& event)
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

void RedcapDlg::OnSeedCheck(wxCommandEvent& event)
{
    wxLogMessage("On RedcapDlg::OnSeedCheck");
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

void RedcapDlg::OnChangeSeed(wxCommandEvent& event)
{
    wxLogMessage("On RedcapDlg::OnChangeSeed");
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

void RedcapDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("On RedcapDlg::InitVariableCombobox");
    wxArrayString items;
    
	int cnt_floor = 0;
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
}

void RedcapDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose RedcapDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void RedcapDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close RedcapDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString RedcapDlg::_printConfiguration()
{
    wxString txt;
    txt << _("Weights:") << "\t" << m_spatial_weights->GetString(m_spatial_weights->GetSelection()) << "\n";
   
    txt << _("Method:\t") << combo_method->GetString(combo_method->GetSelection()) << "\n";
    
    if (chk_floor && chk_floor->IsChecked()) {
        int idx = combo_floor->GetSelection();
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        txt << _("Minimum bound:\t") << txt_floor->GetValue() << "(" << nm << ")" << "\n";
    } else {
        txt << "Minimum region size:\t" << txt_minregions->GetValue() << "\n";
    }
    
    txt << _("Save cluster in field:\t") << m_textbox->GetValue() << "\n";
    
    txt << _("Transformation:") << "\t" << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
    
    txt << _("Distance function:\t") << m_distance->GetString(m_distance->GetSelection()) << "\n";
    return txt;
}

void RedcapDlg::OnSaveTree(wxCommandEvent& event )
{
    if (redcap) {
        wxString filter = "GWT|*.gwt";
        wxFileDialog dialog(NULL, _("Save Spanning Tree to a Weights File"),
                            wxEmptyString,
                            wxEmptyString, filter,
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }

        // get info from input weights
        GalWeight* gw = GetInputSpatialWeights();
        GeoDaWeight* gdw = (GeoDaWeight*)gw;
        wxString id = gdw->GetIDName();
        int col = table_int->FindColId(id);
        if (col < 0) {
            return;
        }
        std::vector<wxString> ids;
        table_int->GetColData(col, 0, ids);
        
        wxFileName fname = wxFileName(dialog.GetPath());
        wxString new_main_dir = fname.GetPathWithSep();
        wxString new_main_name = fname.GetName();
        wxString new_txt = new_main_dir + new_main_name+".gwt";
        wxTextFile file(new_txt);
        file.Create(new_txt);
        file.Open(new_txt);
        file.Clear();

        wxString header;
        header << "0 " << project->GetNumRecords() << " ";
        header << "\"" << project->GetProjectTitle() << "\" ";
        header << id;
		file.AddLine(header);

        std::vector<std::vector<int> > cluster_ids = redcap->GetRegions();
        std::map<int, int> nid_cid; // node id -> cluster id
        for (int c=0; c<cluster_ids.size(); ++c) {
            for (int i=0; i<cluster_ids[c].size(); ++i) {
                nid_cid[ cluster_ids[c][i] ] = c;
            }
        }

        for (int i=0; i<redcap->ordered_edges.size(); i++) {
            wxString line;
            int from_idx = redcap->ordered_edges[i]->orig->id;
            int to_idx = redcap->ordered_edges[i]->dest->id;

            if (chk_save_mst->GetValue() == false) {
                if (nid_cid[from_idx] != nid_cid[to_idx])
                    continue;
            }
            
            double cost = redcap->ordered_edges[i]->length;
            wxString line1;
            line1 << ids[from_idx] << " " << ids[to_idx] << " " <<  cost;
            file.AddLine(line1);
            wxString line2;
            line2 << ids[to_idx] << " " << ids[from_idx] << " " <<  cost;
            file.AddLine(line2);
        }
        file.Write();
        file.Close();
        
        // Load the weights file into Weights Manager
        WeightsManInterface* w_man_int = project->GetWManInt();
        WeightUtils::LoadGwtInMan(w_man_int, new_txt, table_int, id,
                                  WeightsMetaInfo::WT_tree);
    }
}

void RedcapDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click RedcapDlg::OnOK");
   
    if (GdaConst::use_gda_user_seed) {
        setrandomstate(GdaConst::gda_user_seed);
        resetrandom();
    }
    
    // Get input data
    int transform = combo_tranform->GetSelection();
	bool success = GetInputData(transform, 1);
    if (!success) return;
    // check if X-Centroids selected but not projected
    if ((has_x_cent || has_y_cent) && check_spatial_ref) {
        bool cont_process = project->CheckSpatialProjection(check_spatial_ref);
        if (cont_process == false) {
            return;
        }
    }
    bool check_floor = false;
    if (chk_floor->IsChecked()) {
        wxString str_floor = txt_floor->GetValue();
        if (str_floor.IsEmpty() || combo_floor->GetSelection() < 0) {
            wxString err_msg = _("Please enter minimum bound value");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        check_floor = true;
    }

    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	// Get Weights Selection
    GalWeight* gw = CheckSpatialWeights();
    if (gw == NULL) {
        return;
    }

    // Check connectivity
    if (!CheckConnectivity(gw)) {
        wxString msg = _("The connectivity of selected spatial weights is incomplete, please adjust the spatial weights.");
        wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
        dlg.ShowModal();
        //return;
    }

    wxString str_max_region = m_max_region->GetValue();
    if (str_max_region.IsEmpty()) {
        if (txt_minregions->GetValue().IsEmpty() && check_floor == false) {
            wxString err_msg = _("Please enter number of regions, or minimum bound value, or minimum region size.");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
    }

    // Get Bounds
    double min_bound = GetMinBound();
    if (chk_floor->IsChecked()) {
        wxString str_floor = txt_floor->GetValue();
        if (str_floor.IsEmpty()) {
            wxString err_msg = _("Please enter minimum bound value");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
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

	// Get Distance Selection
    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];

    // Get number of regions
    int n_regions = std::numeric_limits<int>::max();
    long value_n_region;
    if(str_max_region.ToLong(&value_n_region)) {
        n_regions = value_n_region;
    }
    
    // Get user specified seed
    int rnd_seed = -1;
    if (chk_seed->GetValue()) rnd_seed = GdaConst::gda_user_seed;
 
    int method_idx = combo_method->GetSelection();
    
    int transpose = 0; // row wise
    // todo should be replaced
    double** ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
    bool isSqrt = method_idx == 2 ? true : false;
    double** distances = DataUtils::fullRaggedMatrix(ragged_distances, rows, rows, isSqrt);
    for (int i = 1; i < rows; i++) free(ragged_distances[i]);
    free(ragged_distances);
    
    // run RedCap
    std::vector<bool> undefs(rows, false);
  
    if (redcap != NULL) {
        delete redcap;
        redcap = NULL;
    }
    
    if (method_idx == 0) {
        redcap = new FirstOrderSLKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
        
    } else if (method_idx == 1) {
        redcap = new FullOrderWardRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
        
    } else if (method_idx == 2) {
        redcap = new FullOrderALKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
        
    } else if (method_idx == 3) {
        redcap = new FullOrderCLKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
        
    } else if (method_idx == 4) {
        redcap = new FullOrderSLKRedCap(rows, columns, distances, input_data, undefs, gw->gal, bound_vals, min_bound);
        
    }

    if (redcap==NULL) {
        if (distances) {
            for (int i = 1; i < rows; i++) delete[] distances[i];
            delete[] distances;
        }
        if (bound_vals) {
            delete[] bound_vals;
            bound_vals = NULL;
        }
        return;
    }
    
    redcap->Partitioning(n_regions);
    
    std::vector<std::vector<int> > cluster_ids = redcap->GetRegions();
    
    int ncluster = cluster_ids.size();
    
    if (n_regions != std::numeric_limits<int>::max() && ncluster < n_regions) {
        // show message dialog to user
        wxString warn_str = _("The number of identified clusters is less than ");
        warn_str << n_regions;
        wxMessageDialog dlg(NULL, warn_str, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
    }
    std::vector<wxInt64> clusters(rows, 0);
    std::vector<bool> clusters_undef(rows, false);

    // sort result
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < ncluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    
    // in case c == 0
    for (int i=0; i<clusters.size(); i++) {
        if (clusters[i] == 0) {
            clusters[i] = ncluster + 1;
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
            wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }
    }
    
    if (col > 0) {
        table_int->SetColData(col, time, clusters);
        table_int->SetColUndefined(col, time, clusters_undef);
    }
    
    // free memory
    for (int i = 1; i < rows; i++) delete[] distances[i];
    delete[] distances;
    
	delete[] bound_vals;
	bound_vals = NULL;
    
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
    ttl << "REDCAP " << _("Cluster Map ") << "(";
    ttl << ncluster;
    ttl << " clusters)";
    nf->SetTitle(ttl);
    
    saveButton->Enable();
}
