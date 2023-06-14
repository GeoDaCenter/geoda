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

#include <limits>
#include <vector>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include <wx/statbmp.h>
#include <wx/checklst.h>
#include <wx/settings.h>
#include <boost/unordered_map.hpp>
#include <wx/treectrl.h>

#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../DialogTools/RandomizationDlg.h"
#include "../DialogTools/AbstractClusterDlg.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../ShapeOperations/PolysToContigWeights.h"
#include "../Weights/BlockWeights.h"
#include "ConditionalClusterMapView.h"
#include "MapNewView.h"
#include "ClusterMatchMapView.h"

using namespace std;

BEGIN_EVENT_TABLE( ClusterMatchSelectDlg, wxDialog )
EVT_CLOSE( ClusterMatchSelectDlg::OnClose )
END_EVENT_TABLE()

ClusterMatchSelectDlg::ClusterMatchSelectDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Cluster Match Map Settings"))
{
    wxLogMessage("Open ClusterMatchSelectDlg.");
    
    base_choice_id = XRCID("CLUSTER_CHOICE_BTN");
    select_variable_lbl = _("Select Clusters to Compare");
    CreateControls();
}

ClusterMatchSelectDlg::~ClusterMatchSelectDlg()
{
    
}

void ClusterMatchSelectDlg::update(TableState* o)
{
    if (o->GetEventType() != TableState::time_ids_add_remove &&
        o->GetEventType() != TableState::time_ids_rename &&
        o->GetEventType() != TableState::time_ids_swap &&
        o->GetEventType() != TableState::cols_delta) return;

    // init variable without combobox, integer only, no centroids
    InitVariableCombobox(NULL, true, false);
    wxCommandEvent ev;
    OnTargetSelect(ev);
}

void ClusterMatchSelectDlg::CreateControls()
{
    wxScrolledWindow* all_scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(450,780), wxHSCROLL|wxVSCROLL );
    all_scrl->SetScrollRate( 5, 5 );
    
    panel = new wxPanel(all_scrl);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    //AddSimpleInputCtrls(panel, vbox, true, false, false);
    wxStaticText* st_orig = new wxStaticText (panel, wxID_ANY,
                                              _("Select Origin Cluster Indicator"));
    list_var = new wxComboBox(panel, wxID_ANY, "", wxDefaultPosition,
                              wxSize(250, -1), 0, NULL, wxCB_READONLY);
    
    bool integer_only = true;
    bool add_centroids = false;
    wxStaticText* st = new wxStaticText (panel, wxID_ANY,
                                         _("Select Target Cluster Indicator"));
    
    
    // init variable without combobox, integer only, no centroids
    InitVariableCombobox(NULL, integer_only, add_centroids);

    target_var = new wxComboBox(panel, wxID_ANY, "", wxDefaultPosition,
                                wxSize(250, -1), 0, NULL, wxCB_READONLY);

    if (!var_items.IsEmpty()) {
        list_var->Append(var_items);
        target_var->Append(var_items);
    }
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxVERTICAL, panel, _("Input:"));
    hbox->Add(st_orig, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox->Add(list_var, 0,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox->Add(target_var, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    vbox->Add(hbox, 1,  wxEXPAND | wxTOP | wxLEFT, 10);
    
    
    // Parameter
    wxBoxSizer *vvbox = new wxBoxSizer(wxVERTICAL);
    scrl = new wxScrolledWindow(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(400,130), wxHSCROLL|wxVSCROLL );
    if (!wxSystemSettings::GetAppearance().IsDark()) {
        scrl->SetBackgroundColour(*wxWHITE);
    }
    scrl->SetScrollRate( 5, 5 );
    vvbox->Add(scrl);
    
    gbox = new wxFlexGridSizer(0,1,5,5);
    wxBoxSizer *scrl_box = new wxBoxSizer(wxVERTICAL);
    scrl_box->Add(gbox, 1, wxEXPAND|wxLEFT|wxRIGHT, 20);
    scrl->SetSizer(scrl_box);
    
    m_cluster_lbl = new wxStaticText(panel, wxID_ANY, select_variable_lbl);
    vbox->Add(m_cluster_lbl, 0, wxALIGN_CENTER_HORIZONTAL);
    vbox->Add(vvbox, 0, wxLEFT|wxRIGHT|wxBOTTOM, 20);
    
    // Minimum size of cluster
    wxStaticText* st2 = new wxStaticText (panel, wxID_ANY, _("Minimum Size of Common Cluster:"));
    m_min_size = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(108,-1));
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox0->Add(st2, 0, wxALIGN_CENTER_VERTICAL);
    hbox0->Add(m_min_size, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    // hide controls in #2437
    st2->Hide();
    m_min_size->Hide();
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Common Cluster in Field:"));
    m_textbox = new wxTextCtrl(panel, wxID_ANY, "CL_COM", wxDefaultPosition, wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(m_textbox, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("OK"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
   
    panel->SetAutoLayout(true);
    panel->SetSizer(container);
    
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);
    
    all_scrl->SetSizer(panelSizer);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(all_scrl, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
    
    // Events
    list_var->Bind(wxEVT_COMBOBOX, &ClusterMatchSelectDlg::OnOriginSelect, this);
    target_var->Bind(wxEVT_COMBOBOX, &ClusterMatchSelectDlg::OnTargetSelect, this);
    okButton->Bind(wxEVT_BUTTON, &ClusterMatchSelectDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &ClusterMatchSelectDlg::OnClickClose, this);
}

void ClusterMatchSelectDlg::OnOriginSelect(wxCommandEvent& event)
{
    int idx = list_var->GetSelection();
    wxString sel_str = list_var->GetString(idx);
    selected_target = name_to_nm[sel_str];
    
    int col = table_int->FindColId(selected_target);
    if (col == wxNOT_FOUND) {
        wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), selected_target);
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    // check if selected target is valid, and list unique values in "select clusters to compare" box
    int tm = name_to_tm_id[sel_str];
    std::vector<wxInt64> cat_vals;
    table_int->GetColData(col, tm, cat_vals);
    if (!ShowOptionsOfVariable(selected_target, cat_vals)) {
        list_var->SetSelection(-1);
        return;
    }
    
    // recreate target variable list
    target_var->Clear();
    var_items.Clear();
    
    std::vector<int> col_id_map;
    table_int->FillIntegerColIdMap(col_id_map);
    for (int i=0, iend=(int)col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                if (nm != selected_target) var_items.Add(nm);
            }
        } else {
            if (name != selected_target) var_items.Add(name);
        }
    }

    if (!var_items.IsEmpty()) {
        target_var->Append(var_items);
    }
    
    target_var->SetSelection(-1);
}

void ClusterMatchSelectDlg::OnTargetSelect( wxCommandEvent& event)
{
    selected_variable = "";
    select_vars.clear();
    
    std::vector<int> tms;
    
    int idx = target_var->GetSelection();
    if (idx >= 0) {
        // check selected variables for any category values, and add them to choice box
        wxString sel_str = target_var->GetString(idx);
        select_vars.push_back(sel_str);
        
        wxString nm = name_to_nm[sel_str];
        int tm = name_to_tm_id[target_var->GetString(idx)];
        tms.push_back(tm);
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        col_ids.clear();
        col_ids.push_back(col);
        
        // get values
        std::vector<wxInt64> cat_vals;
        table_int->GetColData(col, tm, cat_vals);
        if (!CheckCategorical(sel_str, cat_vals)){
            target_var->SetSelection(-1);
            return;
        }
    }
    
}

bool ClusterMatchSelectDlg::CheckCategorical(const wxString& var_name,
                                             std::vector<wxInt64>& cat_vals)
{
    int num_obs = (int)cat_vals.size();
    boost::unordered_map<wxInt64, std::set<int> > cat_dict;
    for (int j=0; j<num_obs; ++j) {
        cat_dict[ cat_vals[j] ].insert(j);
    }
    if ((int)cat_dict.size() == num_obs) {
        wxMessageDialog dlg(this,
                            wxString::Format(_("The selected variable %s is not categorical."), var_name),
                            _("Warning"), wxOK_DEFAULT | wxICON_WARNING );
        dlg.ShowModal();
        return false;
    }
    return true;
}

bool ClusterMatchSelectDlg::ShowOptionsOfVariable(const wxString& var_name, std::vector<wxInt64> cat_vals)
{
    // clean controls
    for (int i=0; i<(int)chk_list.size(); ++i) {
        chk_list[i]->Destroy();
        chk_list[i] = NULL;
    }
    chk_list.clear();
    gbox->SetRows(0);
    
    // check if categorical values
    int num_obs = (int)cat_vals.size();
    std::map<wxInt64, std::set<int> > cat_dict;
    for (int j=0; j<num_obs; ++j) {
        cat_dict[ cat_vals[j] ].insert(j);
    }
    if ((int)cat_dict.size() == num_obs) {
        wxMessageDialog dlg(this,
                            wxString::Format(_("The selected variable %s is not categorical."), var_name),
                            _("Warning"), wxOK_DEFAULT | wxICON_WARNING );
        dlg.ShowModal();
        
        return false;
    }
    
    // create controls for this variable
    wxString update_lbl = select_variable_lbl + " (" + selected_target + ")";
    m_cluster_lbl->SetLabel(update_lbl);
    
    int n_rows = (int)cat_dict.size();
    gbox->SetRows(n_rows);
    int cnt = 0;
    std::map<wxInt64, bool> select_values;
    std::map<wxInt64, std::set<int> >::iterator co_it;
    for (co_it = cat_dict.begin(); co_it!=cat_dict.end(); co_it++) {
        wxString tmp;
        tmp << co_it->first;
        select_values[co_it->first] = false;
        wxCheckBox* chk = new wxCheckBox(scrl, base_choice_id+cnt, tmp);
        chk->SetValue(false);
        if (input_conf.find(var_name) != input_conf.end()) {
            long v = 0;
            tmp.ToLong(&v);
            bool sel = input_conf[var_name].find(v) != input_conf[var_name].end();
            chk->SetValue(sel);
        }
        chk_list.push_back(chk);
        gbox->Add(chk, 1, wxEXPAND);
        chk->Bind(wxEVT_CHECKBOX, &ClusterMatchSelectDlg::OnCheckBoxChange, this);
        cnt ++;
    }
    
    if (input_conf.find(var_name) == input_conf.end()) {
        // init input_conf[var_name], if not existed
        input_conf[selected_target] = select_values;
    }

    container->Layout();
    return true;
}

void ClusterMatchSelectDlg::OnCheckBoxChange(wxCommandEvent& event)
{
    // update configuration dict
    int n_all_values = (int)chk_list.size();
    std::map<wxInt64, bool> select_values;
    // Get checked values for selected variable
    for (int i=0; i < n_all_values; ++i) {
        wxString lbl = chk_list[i]->GetLabel();
        long val = 0;
        if (lbl.ToLong(&val)) {
            select_values[(wxInt64)val] = chk_list[i]->IsChecked();
        }
    }
    input_conf[selected_target] = select_values;
}

wxString ClusterMatchSelectDlg::_printConfiguration()
{
    return "";
}

GeoDaWeight* ClusterMatchSelectDlg::CreateQueenWeights()
{
    int num_obs = project->GetNumRecords();

    GalWeight* poW = new GalWeight;
    poW->num_obs = num_obs;
    poW->is_symmetric = true;
    poW->symmetry_checked = true;
    bool is_queen = true;

    if (project->GetShapefileType() == Shapefile::POINT_TYP) {
        std::vector<std::set<int> > nbr_map;
        const std::vector<GdaPoint*>& centroids = project->GetCentroids();
        std::vector<double> x(num_obs), y(num_obs);
        for (int i=0; i<num_obs; ++i) {
            x[i] = centroids[i]->GetX();
            y[i] = centroids[i]->GetY();
        }
        Gda::VoronoiUtils::PointsToContiguity(x, y, is_queen, nbr_map);
        poW->gal = Gda::VoronoiUtils::NeighborMapToGal(nbr_map);

    } else if (project->GetShapefileType() == Shapefile::POLYGON) {
        poW->gal = PolysToContigWeights(project->main_data, is_queen, 0);
    }

    return (GeoDaWeight*)poW;
}

void ClusterMatchSelectDlg::OnOK( wxCommandEvent& event)
{
    const int NOT_CLUSTERED_IDENTITY = -1;
    int num_obs = project->GetNumRecords();
    std::vector<std::vector<wxInt64> > cat_values;
    
    if (list_var->GetSelection() < 0) {
        wxMessageDialog dlg(this, _("Please select an origin variable."),
                            _("Warning"), wxOK_DEFAULT | wxICON_WARNING );
        dlg.ShowModal();
        return;
    }
    
    // origin variable
    int col = table_int->FindColId(selected_target);
    wxString origin_name = list_var->GetStringSelection();
    int tm = name_to_tm_id[origin_name];
    std::vector<wxInt64> cat_vals;
    std::vector<wxInt64> cat_vals_filtered(num_obs, NOT_CLUSTERED_IDENTITY);
    table_int->GetColData(col, tm, cat_vals);
    // apply user fileter
    boost::unordered_map<wxInt64, bool > obs_dict;
    for (int j=0; j<num_obs; ++j) {
        wxInt64 v = cat_vals[j];
        if (input_conf[selected_target].find(v) != input_conf[selected_target].end() &&
            input_conf[selected_target][v]) {
            cat_vals_filtered[j] = v;
            obs_dict[j] = true;
        }
    }
    
    if (obs_dict.empty()) {
        wxMessageDialog dlg(this, _("Please select at least one value for origin variable."),
                            _("Warning"), wxOK_DEFAULT | wxICON_WARNING );
        dlg.ShowModal();
        return;
    }
    
    // target variable
    int num_vars = (int)select_vars.size();
    if (num_vars < 1) {
        wxMessageDialog dlg(this, _("Please select one target variable."),
                            _("Warning"), wxOK_DEFAULT | wxICON_WARNING );
        dlg.ShowModal();
        return;
    }
    for (int i=0; i<num_vars; ++i) {
        int col_id = col_ids[i];
        wxString col_name = select_vars[i];
        int col_t = name_to_tm_id[col_name];
        std::vector<wxInt64> cat_vals;
        std::vector<wxInt64> cat_vals_filtered(num_obs, NOT_CLUSTERED_IDENTITY);
        table_int->GetColData(col_id, col_t, cat_vals);

        num_obs = (int)cat_vals.size();
        for (int j=0; j<num_obs; ++j) {
            wxInt64 v = cat_vals[j];
            if (obs_dict.find(j) != obs_dict.end()) {
                cat_vals_filtered[j] = v;
            }
        }

        // filter data
        cat_values.push_back(cat_vals_filtered);
    }

    wxString str_min_size = m_min_size->GetValue();
    long l_min_size = 0;
    str_min_size.ToLong(&l_min_size);
    
    int valid_clusters = 0;
    std::vector<wxInt64> clusters = *cat_values.begin();
    std::vector<bool> clusters_undef(num_obs, false);
    for (int i=0; i<(int)clusters.size(); ++i) {
        if (clusters[i] == NOT_CLUSTERED_IDENTITY) {
            clusters_undef[i] = true;
        } else {
            valid_clusters += 1;
        }
    }
    
    if (valid_clusters == 0) {
        wxString err_msg = _("No common cluster can be found. Please choose other variables or clusters to compare.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }

    // field name
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }

    // save to table
    int time=0;
    col = table_int->FindColId(field_name);
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

    GdaConst::map_undefined_colour = wxColour(255,255,255);
    
    MapFrame* nf = new MapFrame(parent, project,
                                new_var_info, new_col_ids,
                                CatClassification::unique_values,
                                MapCanvas::no_smoothing, 4,
                                boost::uuids::nil_uuid(),
                                wxDefaultPosition,
                                GdaConst::map_default_size);
    GdaConst::map_undefined_colour = wxColour(70, 70, 70);
    
    wxString ttl = _("Cluster Match Map");
    ttl << ": " << list_var->GetStringSelection() << " - ";
    for (int i=0; i<select_vars.size(); i++) {
        ttl << select_vars[i];
        if (i < select_vars.size() -1) {
            ttl << "/";
        }
    }
    nf->SetTitle(ttl);
}

void ClusterMatchSelectDlg::OnClose( wxCloseEvent& event)
{
    wxLogMessage("ClusterMatchSelectDlg::OnClose()");
    Destroy();
}

void ClusterMatchSelectDlg::OnClickClose( wxCommandEvent& event)
{
    wxLogMessage("ClusterMatchSelectDlg::OnClose()");
    event.Skip();
    EndDialog(wxID_CANCEL);
}
