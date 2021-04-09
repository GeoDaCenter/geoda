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
   
    // update
    var_selections.Clear();

    InitVariableCombobox(combo_var, true/*integer only*/);
    wxCommandEvent ev;
    OnVarSelect(ev);
}

void ClusterMatchSelectDlg::CreateControls()
{
    wxScrolledWindow* all_scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(420,500), wxHSCROLL|wxVSCROLL );
    all_scrl->SetScrollRate( 5, 5 );
    
    panel = new wxPanel(all_scrl);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    AddSimpleInputCtrls(panel, vbox, true, false, false);

    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Common Cluster in Field:"));
    m_textbox = new wxTextCtrl(panel, wxID_ANY, "CL_COM", wxDefaultPosition, wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(m_textbox, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("OK"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
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
    combo_var->Bind(wxEVT_LISTBOX, &ClusterMatchSelectDlg::OnVarSelect, this);
    okButton->Bind(wxEVT_BUTTON, &ClusterMatchSelectDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &ClusterMatchSelectDlg::OnClickClose, this);
}

void ClusterMatchSelectDlg::OnVarSelect( wxCommandEvent& event)
{
    var_selections.Clear();
    select_vars.clear();

    std::vector<int> tms;
    
    combo_var->GetSelections(var_selections);
    size_t num_var = var_selections.size();
    if (num_var >= 2) {
        // check selected variables for any category values, and add them to choice box
        col_ids.resize(num_var);
        std::vector<wxString> col_names;
        col_names.resize(num_var);
        for (int i=0; i<num_var; i++) {
            int idx = var_selections[i];
            wxString sel_str = combo_var->GetString(idx);
            select_vars.push_back(sel_str);
            
            wxString nm = name_to_nm[sel_str];
            col_names.push_back(nm);
            
            int tm = name_to_tm_id[combo_var->GetString(idx)];
            tms.push_back(tm);
            
            int col = table_int->FindColId(nm);
            if (col == wxNOT_FOUND) {
                wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
                wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                return;
            }
            col_ids[i] = col;
        }
    }
    
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
    int num_obs = 0;
    int num_vars = (int)select_vars.size();
    std::vector<std::vector<wxInt64> > cat_values;

    for (int i=0; i<num_vars; ++i) {
        // get data
        int col_id = col_ids[i];
        wxString col_name = select_vars[i];
        int col_t = name_to_tm_id[col_name];
        std::vector<wxInt64> cat_vals;
        table_int->GetColData(col_id, col_t, cat_vals);

        // check if categorical values
        num_obs = (int)cat_vals.size();
        boost::unordered_map<wxInt64, std::set<int> > cat_dict;
        for (int j=0; j<num_obs; ++j) {
            cat_dict[ cat_vals[j] ].insert(j);
        }
        if ((int)cat_dict.size() == num_obs) {
            wxMessageDialog dlg(this,
                                _("The selected variable is not categorical."),
                                _("Warning"), wxOK_DEFAULT | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }

        cat_values.push_back(cat_vals);
    }

    // create a queen contiguity weights
    GeoDaWeight* queen_w = CreateQueenWeights();

    BlockWeights block_w(cat_values, queen_w);

    std::vector<wxInt64> clusters = block_w.GetClusters();
    std::vector<bool> clusters_undef(num_obs, false);
    for (int i=0; i<(int)clusters.size(); ++i) {
        if (clusters[i] == 0) {
            clusters_undef[i] = true;
        }
    }

    // field name
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

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

    wxString ttl = _("Cluster Match Map");
    ttl << ": ";
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
