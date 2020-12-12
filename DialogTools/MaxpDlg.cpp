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
#include "../Algorithms/azp.h"

#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "MaxpDlg.h"


BEGIN_EVENT_TABLE( MaxpDlg, wxDialog )
EVT_CLOSE( MaxpDlg::OnClose )
END_EVENT_TABLE()

MaxpDlg::MaxpDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Max-p Settings"))
{
    wxLogMessage("Open Max-p dialog.");
    CreateControls();
}

MaxpDlg::~MaxpDlg()
{
    wxLogMessage("On MaxpDlg::~MaxpDlg");
}

void MaxpDlg::CreateControls()
{
    wxLogMessage("On MaxpDlg::CreateControls");
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(900,820), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    AddSimpleInputCtrls(panel, vbox, false, true/*show spatial weights controls*/);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);
   
	// Minimum Bound Control
    AddMinBound(panel, gbox);

    // Min regions
    st_minregions = new wxStaticText(panel, wxID_ANY, _("Min Region Size:"));
    txt_minregions = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(200,-1));
    txt_minregions->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    gbox->Add(st_minregions, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_minregions, 1, wxEXPAND);
    
    wxStaticText* st18 = new wxStaticText(panel, wxID_ANY, _("Initial Groups:"));
    wxBoxSizer *hbox18 = new wxBoxSizer(wxHORIZONTAL);
    chk_lisa = new wxCheckBox(panel, wxID_ANY, "");
    combo_lisa = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    st18->Hide();
    chk_lisa->Hide();
    combo_lisa->Hide();
    
    hbox18->Add(chk_lisa,0, wxALIGN_CENTER_VERTICAL);
    hbox18->Add(combo_lisa,0,wxALIGN_CENTER_VERTICAL);
    combo_lisa->Disable();
    gbox->Add(st18, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox18, 1, wxEXPAND);
    
    InitLISACombobox();
    
	wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("# Iterations:"));
    m_iterations = new wxTextCtrl(panel, wxID_ANY, "99", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_iterations, 1, wxEXPAND);
    
	wxStaticText* st19 = new wxStaticText(panel, wxID_ANY, _("Local Search:"));
    wxString choices19[] = {"Greedy", "Tabu Search", "Simulated Annealing"};
    m_localsearch = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 3, choices19);
    m_localsearch->SetSelection(0);
    
    wxBoxSizer *hbox19_1 = new wxBoxSizer(wxHORIZONTAL);
    m_tabulength = new wxTextCtrl(panel, wxID_ANY, "10", wxDefaultPosition, wxSize(45,-1));
    m_convtabu = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(45,-1));
    hbox19_1->Add(new wxStaticText(panel, wxID_ANY, _("Tabu Length:")), 0, wxALIGN_CENTER_VERTICAL);
    hbox19_1->Add(m_tabulength, 0, wxALIGN_CENTER_VERTICAL);
    hbox19_1->Add(new wxStaticText(panel, wxID_ANY, _("ConvTabu:")), 0, wxALIGN_CENTER_VERTICAL);
    hbox19_1->Add(m_convtabu, 0, wxALIGN_CENTER_VERTICAL);
    m_tabulength->Disable();
    m_convtabu->Disable();
    
    wxBoxSizer *hbox19_2 = new wxBoxSizer(wxHORIZONTAL);
    m_coolrate= new wxTextCtrl(panel, wxID_ANY, "0.85", wxDefaultPosition, wxSize(45,-1));
    m_sait= new wxTextCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxSize(30,-1));
    hbox19_2->Add(new wxStaticText(panel, wxID_ANY, _("Cooling Rate:")), 0, wxALIGN_CENTER_VERTICAL);
    hbox19_2->Add(m_coolrate, 0, wxALIGN_CENTER_VERTICAL);
    hbox19_2->Add(new wxStaticText(panel, wxID_ANY, _("MaxIt:")), 0, wxALIGN_CENTER_VERTICAL);
    hbox19_2->Add(m_sait, 0, wxALIGN_CENTER_VERTICAL);
    m_sait->Disable();
    m_coolrate->Disable();
    
    wxBoxSizer *vbox19 = new wxBoxSizer(wxVERTICAL);
    vbox19->Add(m_localsearch, 1, wxEXPAND);
    vbox19->Add(hbox19_1, 1, wxEXPAND);
    vbox19->Add(hbox19_2, 1, wxEXPAND);
    gbox->Add(st19, 0, wxALIGN_TOP | wxRIGHT | wxLEFT, 10);
    gbox->Add(vbox19, 1, wxEXPAND);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
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
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(m_textbox, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
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
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &MaxpDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &MaxpDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &MaxpDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &MaxpDlg::OnChangeSeed, this);
    chk_lisa->Bind(wxEVT_CHECKBOX, &MaxpDlg::OnLISACheck, this);
    m_localsearch->Bind(wxEVT_CHOICE, &MaxpDlg::OnLocalSearch, this);

}

void MaxpDlg::OnLocalSearch(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnLocalSearch");
    if ( m_localsearch->GetSelection() == 0) {
        m_tabulength->Disable();
        m_convtabu->Disable();
        m_coolrate->Disable();
        m_sait->Disable();
    } else if ( m_localsearch->GetSelection() == 1) {
        m_tabulength->Enable();
        m_convtabu->Enable();
        m_coolrate->Disable();
        m_sait->Disable();
    } else if ( m_localsearch->GetSelection() == 2) {
        m_tabulength->Disable();
        m_convtabu->Disable();
        m_coolrate->Enable();
        m_sait->Enable();
    }
}
void MaxpDlg::OnCheckMinBound(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnLISACheck");
    AbstractClusterDlg::OnCheckMinBound(event);
   
    if (chk_floor->IsChecked()) {
        st_minregions->Disable();
        txt_minregions->Disable();
    } else {
        st_minregions->Enable();
        txt_minregions->Enable();
    }
}

void MaxpDlg::OnLISACheck(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnLISACheck");
    bool use_lisa_seed = chk_lisa->GetValue();
    
    if (use_lisa_seed) {
        combo_lisa->Enable();
    } else {
        combo_lisa->Disable();
    }
}

void MaxpDlg::OnSeedCheck(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnSeedCheck");
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

void MaxpDlg::OnChangeSeed(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnChangeSeed");
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

void MaxpDlg::InitLISACombobox()
{
    wxLogMessage("On MaxpDlg::InitVariableCombobox");
    wxArrayString items;
 
    combo_lisa->Clear();
    
    int cnt_lisa = 0;
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
                if (ftype == GdaConst::long64_type)
                    combo_lisa->Insert(nm, cnt_lisa++);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            items.Add(name);
            if (ftype == GdaConst::long64_type)
                combo_lisa->Insert(name, cnt_lisa++);
        }
    }
    
    combo_lisa->SetSelection(-1);
    combo_lisa->SetStringSelection(select_lisa);
}

void MaxpDlg::update(TableState* o)
{
    InitVariableCombobox(combo_var);
    InitLISACombobox();
}

void MaxpDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose MaxpDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void MaxpDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close MaxpDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString MaxpDlg::_printConfiguration()
{
    wxString txt;
    
    txt << _("Weights:") << "\t" << m_spatial_weights->GetString(m_spatial_weights->GetSelection()) << "\n";
    
    if (chk_floor && chk_floor->IsChecked() && combo_floor->GetSelection() >= 0) {
        int idx = combo_floor->GetSelection();
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        txt << _("Minimum bound:\t") << txt_floor->GetValue() << "(" << nm << ")" << "\n";
    } else {
        txt << _("Minimum region size:\t") << txt_minregions->GetValue() << "\n";
    }
   
    if (chk_lisa->IsChecked() && combo_lisa->GetSelection() >=0) {
        txt << _("Initial groups:\t") << combo_lisa->GetString(combo_lisa->GetSelection()) << "\n";
    }
    
    txt << _("# iterations:\t") << m_iterations->GetValue() << "\n";
   
    int local_search_method = m_localsearch->GetSelection();
    if (local_search_method == 0) {
        txt << _("Local search:") << "\t" << _("Greedy") << "\n";
    } else if (local_search_method == 1) {
        txt << _("Local search:") << "\t" << _("Tabu Search") << "\n";
        txt << _("Tabu length:") << "\t" << m_tabulength->GetValue() << "\n";
        txt << _("ConvTabu:") << "\t" << conv_tabu << "\n";
    } else if (local_search_method == 2) {
        txt << _("Local search:") << "\t" << _("Simulated Annealing") << "\n";
        txt << _("Cooling rate:") << "\t" << m_coolrate->GetValue() << "\n";
        txt << _("MaxIt:") << "\t" << m_sait->GetValue() << "\n";
    }
    
    txt << _("Distance function:\t") << m_distance->GetString(m_distance->GetSelection()) << "\n";
    
    txt << _("Transformation:\t") << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
    
    return txt;
}

void MaxpDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click MaxpDlg::OnOK");
   
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
    
    wxString str_initial = m_iterations->GetValue();
    if (str_initial.IsEmpty()) {
        wxString err_msg = _("Please enter iteration number");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	// Get Distance Selection
    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];
    
    // Weights selection
    GalWeight* gw = CheckSpatialWeights();
    if (gw == NULL) {
        return;
    }
    
    // Check connectivity
    if (!CheckConnectivity(gw)) {
        wxString msg = _("The connectivity of selected spatial weights is incomplete, please adjust the spatial weights.");
        wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
        dlg.ShowModal();
    }
    
	// zonecontrls
    std::vector<ZoneControl> controllers;
    
    // Get Min regions
    wxString str_min_region = txt_minregions->GetValue();
    long l_min_region  = 0;
    if (!str_min_region.IsEmpty() && str_min_region.ToLong(&l_min_region) == false) {
        wxString err_msg = _("Please enter a valid number for Min Region Size.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    if  (l_min_region  > 0) {
        std::vector<double> ids(rows, 1);
        ZoneControl zc(ids);
        zc.AddControl(ZoneControl::SUM,
                      ZoneControl::MORE_THAN, l_min_region);
        controllers.push_back(zc);
    }
    
    // Get Bounds
    double min_bound = GetMinBound();
    double* bound_vals = GetBoundVals();
    if (chk_floor->IsChecked()) {
        if (combo_floor->GetSelection() < 0) {
            wxString err_msg = _("Please enter minimum bound value");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        select_floor = combo_floor->GetString(combo_floor->GetSelection());
        ZoneControl zc(rows, bound_vals);
        zc.AddControl(ZoneControl::SUM,
                      ZoneControl::MORE_THAN, min_bound);
        controllers.push_back(zc);
        delete[] bound_vals;
    } else {
        wxString str_floor = txt_minregions->GetValue();
        if (str_floor.IsEmpty()) {
            wxString err_msg = _("Please enter minimum number of observations per regions, or use minimum bound instead.");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
    }
        
	// Get iteration numbers
    int iterations = 99;
    long value_iter;
    if(str_initial.ToLong(&value_iter)) {
        iterations = value_iter;
    }
    
	// Get initial seed e.g LISA clusters
    std::vector<int> init_regions;
    bool use_init_regions = chk_lisa->GetValue();
    if (use_init_regions) {
        int idx = combo_lisa->GetSelection();
        if (idx < 0) {
            use_init_regions = false;
        } else {
            select_lisa = combo_lisa->GetString(idx);
            wxString nm = name_to_nm[select_lisa];
            int col = table_int->FindColId(nm);
            if (col == wxNOT_FOUND) {
                wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
                wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                return;
            }
            int tm = name_to_tm_id[combo_lisa->GetString(idx)];
            std::vector<wxInt64> vals;
            table_int->GetColData(col, tm, vals);
            init_regions.resize(vals.size());
            for (int i=0; i<vals.size(); ++i) init_regions[i] = vals[i];
        }
    }
   
    // Get local search method
    int local_search_method = m_localsearch->GetSelection();
    int tabu_length = 10;
    conv_tabu = 0;
    double cool_rate = 0.85;
    int sa_iter = 1;
    if ( local_search_method == 0) {
        m_tabulength->Disable();
        m_coolrate->Disable();
    } else if ( local_search_method == 1) {
        wxString str_tabulength= m_tabulength->GetValue();
        long n_tabulength;
        if (str_tabulength.ToLong(&n_tabulength)) {
            tabu_length = n_tabulength;
        }
        if (tabu_length < 1) {
            wxString err_msg = _("Tabu length for Tabu Search algorithm has to be an integer number larger than 1 (e.g. 10).");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        wxString str_convtabu= m_convtabu->GetValue();
        long n_convtabu;
        if (str_convtabu.ToLong(&n_convtabu)) {
            conv_tabu = (int)n_convtabu;
        }
    } else if ( local_search_method == 2) {
        wxString str_coolrate = m_coolrate->GetValue();
        str_coolrate.ToDouble(&cool_rate);
        if ( cool_rate > 1 || cool_rate <= 0) {
            if (bound_vals) delete[] bound_vals;
            wxString err_msg = _("Cooling rate for Simulated Annealing algorithm has to be a float number between 0 and 1 (e.g. 0.85).");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        wxString str_maxit = m_sait->GetValue();
        long l_max_it = 1;
        str_maxit.ToLong(&l_max_it);
        if ( l_max_it <= 0) {
            wxString err_msg = _("MaxIt for Simulated Annealing algorithm has to be a positive integer number.");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        sa_iter = l_max_it;
    }

	// Get random seed
    int rnd_seed = -1;
    if (chk_seed->GetValue()) rnd_seed = GdaConst::gda_user_seed;
    
    //maxp
    int inits = 0; // ARiSel
    int transpose = 0; // row wise
    double** ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
    RawDistMatrix dm(ragged_distances);

    std::vector<int> final_solution;
    RegionMaker* maxp;
    if ( local_search_method == 0) {
        maxp = new MaxpRegion(iterations, gw->gal, input_data, &dm, rows, columns,
                             controllers, inits, init_regions, rnd_seed);
    } else if ( local_search_method == 1) {
        maxp = new MaxpTabu(iterations, gw->gal, input_data, &dm, rows, columns,
                            controllers, tabu_length, conv_tabu, inits, init_regions, rnd_seed);
        conv_tabu = ((MaxpTabu*)maxp)->GetConvTabu();
    } else {
        maxp = new MaxpSA(iterations, gw->gal, input_data, &dm, rows, columns,
                          controllers, cool_rate, sa_iter, inits, init_regions, rnd_seed);
    }
    if (maxp->IsSatisfyControls() == false) {
        wxString msg = _("The clustering results violate the requirement of minimum bound  or minimum number per region. Please adjust the input and try again.");
        wxMessageDialog dlg(NULL, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
    }
    final_solution = maxp->GetResults();
    //initial_of = maxp->GetInitObjectiveFunction();
    //final_of = maxp->GetFinalObjectiveFunction();
    delete maxp;
    
    vector<vector<int> > cluster_ids;
    std::map<int, std::vector<int> > solution;
    for (int i=0; i<final_solution.size(); ++i) {
        solution[final_solution[i]].push_back(i);
    }
    std::map<int, std::vector<int> >::iterator it;
    for (it = solution.begin(); it != solution.end(); ++it) {
        cluster_ids.push_back(it->second);
    }

    for (int i = 1; i < rows; i++) free(ragged_distances[i]);
    free(ragged_distances);
    
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
    ttl << "Max-p " << _("Cluster Map ") << "(";
    ttl << ncluster;
    ttl << " clusters)";
    nf->SetTitle(ttl);
}
