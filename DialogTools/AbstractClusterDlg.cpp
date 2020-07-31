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
#include <list>
#include <map>
#include <algorithm>
#include <limits>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../ShapeOperations/PolysToContigWeights.h"
#include "../Algorithms/texttable.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "../GeoDa.h"
#include "SaveToTableDlg.h"
#include "AbstractClusterDlg.h"

bool AbstractClusterDlg::check_spatial_ref = true;

AbstractClusterDlg::AbstractClusterDlg(wxFrame* parent_s, Project* project_s,
                                       wxString title)
  : frames_manager(project_s->GetFramesManager()),
    table_state(project_s->GetTableState()),
    w_man_state(project_s->GetWManState()),
    wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
    validator(wxFILTER_INCLUDE_CHAR_LIST),
    input_data(NULL), mask(NULL), weight(NULL), m_use_centroids(NULL),
    m_weight_centroids(NULL), m_wc_txt(NULL), chk_floor(NULL),
    combo_floor(NULL), txt_floor(NULL),  txt_floor_pct(NULL),
    slider_floor(NULL), combo_var(NULL), m_reportbox(NULL), gal(NULL),
    return_additional_summary(false), m_spatial_weights(NULL),
    has_x_cent(false), has_y_cent(false)
{
    wxLogMessage("Open AbstractClusterDlg.");
   
    wxArrayString list;
    wxString valid_chars(".,0123456789");
    size_t len = valid_chars.Length();
    for (size_t i=0; i<len; i++) {
        list.Add(wxString(valid_chars.GetChar(i)));
    }
    validator.SetIncludes(list);
   
    parent = parent_s;
    project = project_s;
   
    if (project_s->GetTableInt()->GetNumberCols() == 0) {
        wxString err_msg = _("No numeric variables found in table.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        EndDialog(wxID_CANCEL);
    }
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    }
    frames_manager->registerObserver(this);
    table_state->registerObserver(this);
    w_man_state->registerObserver(this);
}

AbstractClusterDlg::~AbstractClusterDlg()
{
    CleanData();
    frames_manager->removeObserver(this);
    table_state->removeObserver(this);
    w_man_state->removeObserver(this);
}

void AbstractClusterDlg::CleanData()
{
    if (input_data) {
        for (int i=0; i<rows; i++) delete[] input_data[i];
        delete[] input_data;
        input_data = NULL;
    }
    if (mask) {
        for (int i=0; i<rows; i++) delete[] mask[i];
        delete[] mask;
        mask = NULL;
    }
    if (weight) {
        delete[] weight;
        weight = NULL;
    }
}

bool AbstractClusterDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    rows = project->GetNumRecords();
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void AbstractClusterDlg::update(FramesManager* o)
{
}

void AbstractClusterDlg::update(TableState* o)
{
    InitVariableCombobox(combo_var);
}

void AbstractClusterDlg::update(WeightsManState* o)
{
    // Need to refresh weights list
    if (m_spatial_weights) {
        InitSpatialWeights(m_spatial_weights);
    }
}

bool AbstractClusterDlg::CheckConnectivity(GalWeight* gw)
{
    if (rows == 0 || gw == NULL) return false;
    
    GalElement* W = gw->gal;
    if (W == NULL) return false;

    return CheckConnectivity(W);
}

bool AbstractClusterDlg::CheckConnectivity(GalElement* W)
{
    if (W == NULL) return false;

    // start from first node in W
    if (W[0].Size() == 0) return false;
   
    std::map<int, bool> access_dict; // prevent loop
    access_dict[0] = true;
    
    std::list<int> magzine;
    for (int i=0; i<W[0].Size(); i++) {
        if (access_dict.find((int)W[0][i]) == access_dict.end()) {
            magzine.push_back((int)W[0][i]);
            access_dict[(int)W[0][i]] = true;
        }
    }
    // breadth first traversal (BFS)
    while (!magzine.empty()) {
        int nbr = magzine.front();
        magzine.pop_front();
        for (int i=0; i<W[nbr].Size(); i++) {
            if (access_dict.find((int)W[nbr][i]) == access_dict.end()) {
                magzine.push_back((int)W[nbr][i]);
                access_dict[(int)W[nbr][i]] = true;
            }
        }
    }
   
    if (access_dict.size() < rows) {
        // check every one that is not connected via BFS,
        for (int i=0; i<rows; i++) {
            if (access_dict.find(i) == access_dict.end()) {
                bool rev_conn = false;
                // then manually check if this one is connected
                for (int j=0; j<W[i].Size(); j++) {
                    if (access_dict.find((int)W[i][j]) != access_dict.end()) {
                        rev_conn = true;
                        break;
                    }
                }
                if (rev_conn == false) {
                    // any one is checked being not connected, return false
                    return false;
                }
            }
        }
    }
    
    return true;
}

void AbstractClusterDlg::AddSimpleInputCtrls(wxPanel *panel, wxBoxSizer* vbox,
                                             bool integer_only, bool show_spatial_weights)
{
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables"));
    
    combo_var = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                              wxSize(250,250), 0, NULL,
                              wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    InitVariableCombobox(combo_var, integer_only);
    
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel,
                                                   _("Input:"));
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(combo_var, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // add spatial weights selection control
    wxBitmap w_bitmap(wxXmlResource::Get()->LoadBitmap("SpatialWeights_Bmp"));
    weights_btn = new wxBitmapButton(panel, wxID_ANY, w_bitmap, wxDefaultPosition,
                                     w_bitmap.GetSize(), wxTRANSPARENT_WINDOW | wxBORDER_NONE);
    st_spatial_w = new wxStaticText(panel, wxID_ANY, _("Select Spatial Weights:"));
    m_spatial_weights = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer *hbox_spatial_w = new wxBoxSizer(wxHORIZONTAL);
    hbox_spatial_w->Add(st_spatial_w, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    hbox_spatial_w->Add(m_spatial_weights, 0, wxALIGN_CENTER_VERTICAL| wxRIGHT, 10);
    hbox_spatial_w->Add(weights_btn,  0, wxALIGN_CENTER_VERTICAL);
    // init the spatial weights control
    InitSpatialWeights(m_spatial_weights);
    hbox0->Add(hbox_spatial_w, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    if (!show_spatial_weights) {
        st_spatial_w->Hide();
        m_spatial_weights->Hide();
        weights_btn->Hide();
    } else {
        weights_btn->Bind(wxEVT_BUTTON, &AbstractClusterDlg::OnSpatialWeights, this);
    }

    vbox->Add(hbox0, 1,  wxEXPAND | wxTOP | wxLEFT, 10);
}

void AbstractClusterDlg::AddInputCtrls(wxPanel *panel, wxBoxSizer* vbox,
                                       bool show_auto_button, bool show_spatial_weights)
{
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables"));
    
    combo_var = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                              wxSize(250,250), 0, NULL,
                              wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    InitVariableCombobox(combo_var);
    
    m_use_centroids = new wxCheckBox(panel, wxID_ANY, _("Use geometric centroids"));
    auto_btn = new wxButton(panel, wxID_OK, _("Auto Weighting"));
    auto_btn->Bind(wxEVT_BUTTON, &AbstractClusterDlg::OnAutoWeightCentroids, this);

    wxBoxSizer *hbox_c = new wxBoxSizer(wxHORIZONTAL);
    hbox_c->Add(m_use_centroids, 0);
    hbox_c->Add(auto_btn, 0);
    
    wxStaticText* st_wc = new wxStaticText (panel, wxID_ANY, _("Weighting:"));
    wxStaticText* st_w0 = new wxStaticText (panel, wxID_ANY, "0");
    wxStaticText* st_w1 = new wxStaticText (panel, wxID_ANY, "1");
    m_weight_centroids = new wxSlider(panel, wxID_ANY, 100, 0, 100,
                                      wxDefaultPosition, wxSize(140, -1),
                                      wxSL_HORIZONTAL);
    m_weight_centroids->SetRange(0,100);
    m_weight_centroids->SetValue(100);
    m_wc_txt = new wxTextCtrl(panel, wxID_ANY, "1", wxDefaultPosition,
                              wxSize(80,-1), 0, validator);
    wxBoxSizer *hbox_w = new wxBoxSizer(wxHORIZONTAL);
    hbox_w->Add(st_wc, 0, wxLEFT, 20);
    hbox_w->Add(st_w0, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    hbox_w->Add(m_weight_centroids, 0, wxEXPAND);
    hbox_w->Add(st_w1, 0, wxALIGN_CENTER_VERTICAL);
    hbox_w->Add(m_wc_txt, 0, wxALIGN_TOP|wxLEFT, 5);

    // add spatial weights selection control
    wxBitmap w_bitmap(wxXmlResource::Get()->LoadBitmap("SpatialWeights_Bmp"));
    weights_btn = new wxBitmapButton(panel, wxID_ANY, w_bitmap, wxDefaultPosition,
                                     w_bitmap.GetSize(), wxTRANSPARENT_WINDOW | wxBORDER_NONE);
    st_spatial_w = new wxStaticText(panel, wxID_ANY, _("Select Spatial Weights:"));
    m_spatial_weights = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer *hbox_spatial_w = new wxBoxSizer(wxHORIZONTAL);
    hbox_spatial_w->Add(st_spatial_w, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    hbox_spatial_w->Add(m_spatial_weights, 0, wxALIGN_CENTER_VERTICAL| wxRIGHT, 10);
    hbox_spatial_w->Add(weights_btn,  0, wxALIGN_CENTER_VERTICAL);
    // init the spatial weights control
    InitSpatialWeights(m_spatial_weights);

    if (!show_auto_button)  {
        m_use_centroids->Hide();
        auto_btn->Hide();
        st_wc->Hide();
        st_w0->Hide();
        st_w1->Hide();
        m_weight_centroids->Hide();
        m_wc_txt->Hide();
    } else {
        st_spatial_w->Disable();
        m_spatial_weights->Disable();
        weights_btn->Disable();
    }
    if (!show_spatial_weights) {
        st_spatial_w->Hide();
        m_spatial_weights->Hide();
        weights_btn->Hide();
    }

    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, _("Input:"));
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(combo_var, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox0->Add(hbox_c, 0, wxLEFT | wxRIGHT, 10);
    hbox0->Add(hbox_w, 0, wxLEFT | wxRIGHT, 10);
    hbox0->Add(hbox_spatial_w, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    
    if (project->IsTableOnlyProject()) {
        m_use_centroids->Disable();
    }
    m_weight_centroids->Disable();
    m_wc_txt->Disable();
    auto_btn->Disable();
    m_use_centroids->Bind(wxEVT_CHECKBOX, &AbstractClusterDlg::OnUseCentroids, this);
	m_weight_centroids->Bind(wxEVT_SLIDER, &AbstractClusterDlg::OnSlideWeight, this);
    m_wc_txt->Bind(wxEVT_TEXT, &AbstractClusterDlg::OnInputWeights, this);
    weights_btn->Bind(wxEVT_BUTTON, &AbstractClusterDlg::OnSpatialWeights, this);
}

void AbstractClusterDlg::InitSpatialWeights(wxChoice* combo_weights)
{
    // init spatial weights
    combo_weights->Clear();
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);

    size_t sel_pos=0;
    for (size_t i=0; i<weights_ids.size(); ++i) {
        combo_weights->Append(w_man_int->GetShortDispName(weights_ids[i]));
        if (w_man_int->GetDefault() == weights_ids[i]) {
            sel_pos = i;
        }
    }
    if (weights_ids.size() > 0) {
        combo_weights->SetSelection(sel_pos);
    }
}

void AbstractClusterDlg::OnSpatialWeights(wxCommandEvent& ev)
{
    // user click weights manager button
    GdaFrame::GetGdaFrame()->OnToolsWeightsManager(ev);
}

void AbstractClusterDlg::OnInputWeights(wxCommandEvent& ev)
{
    wxString val = m_wc_txt->GetValue();
    double w_val;
    if (val.ToDouble(&w_val)) {
        m_weight_centroids->SetValue( (int)(w_val * 100));
    }
}

void AbstractClusterDlg::OnSlideWeight(wxCommandEvent& ev)
{
    int val = m_weight_centroids->GetValue();
    wxString t_val = wxString::Format("%.2f", val/100.0);
    m_wc_txt->SetValue(t_val);
}

void AbstractClusterDlg::OnUseCentroids(wxCommandEvent& event)
{
    bool use_cent = m_use_centroids->IsChecked();
    m_weight_centroids->Enable(use_cent);
    m_weight_centroids->SetValue(use_cent ? 100 : 0);
    m_wc_txt->SetValue(use_cent? "1.00" : "0.00");
    m_wc_txt->Enable(use_cent);
    auto_btn->Enable(use_cent);
    st_spatial_w->Enable(use_cent);
    m_spatial_weights->Enable(use_cent);
    weights_btn->Enable(use_cent);
}

GalWeight* AbstractClusterDlg::GetInputSpatialWeights()
{
    GalWeight* gal = 0;
    if (m_spatial_weights) {
        int sel = m_spatial_weights->GetSelection();
        if (sel >= 0) {
            std::vector<boost::uuids::uuid> weights_ids;
            WeightsManInterface* w_man_int = project->GetWManInt();
            w_man_int->GetIds(weights_ids);

            if (sel >= weights_ids.size()) {
                sel = weights_ids.size() - 1;
            }

            boost::uuids::uuid w_id = weights_ids[sel];
            gal = w_man_int->GetGal(w_id);
        }
    }
    return gal;
}

bool AbstractClusterDlg::CheckContiguity(GalWeight* weights, double w, double& ssd)
{
    int val = w * 100;
    m_weight_centroids->SetValue(val);
    m_wc_txt->SetValue(wxString::Format("%f", w));

    std::vector<wxInt64> clusters;
    if (Run(clusters) == false) {
        m_weight_centroids->SetValue(100);
        m_wc_txt->SetValue("1.0");
        return false;
    }

    // not show print
    bool print_result = false;
    ssd = CreateSummary(clusters, print_result, return_additional_summary);

    return CheckContiguity(weights->gal, clusters);
}

bool AbstractClusterDlg::CheckContiguity(GalElement* gal, std::vector<wxInt64>& clusters)
{
    std::map<int, std::map<wxInt64, bool> > groups;
    std::map<int, std::map<wxInt64, bool> >::iterator it;
    for (int i=0; i<clusters.size(); i++) {
        int c = (int)clusters[i];
        if (c == 0) continue; // 0 means not clustered
        groups[c][i] = false;
    }

    for (it = groups.begin(); it != groups.end(); it++) {
        // check each group if contiguity
        // start from 1st object, do BFS and add all objects has c=it->first
        int cid = it->first;
        std::map<wxInt64, bool>& g = it->second;
        int fid = g.begin()->first;

        std::stack<int> processed_ids;
        processed_ids.push(fid);

        while (processed_ids.empty() == false) {
            fid = processed_ids.top();
            processed_ids.pop();
            g[fid] = true; // mark fid from current group as processed
            const std::vector<long>& nbrs = gal[fid].GetNbrs();
            for (int i=0; i<nbrs.size(); i++ ) {
                int nid = nbrs[i];
                if (g.find(nid) != g.end() && g[nid] == false) {
                    // only processed the neighbor in current group
                    processed_ids.push(nid);
                }
            }
        }
        std::map<wxInt64, bool>::iterator item_it;
        for (item_it = g.begin(); item_it != g.end(); ++item_it) {
            if (item_it->second == false) {
                return false;
            }
        }
    }

    return true;
}

double AbstractClusterDlg::BinarySearch(GalWeight* weights, double left, double right)
{
    double w = 1.0; // init value of w (weighting value)
    std::stack<std::pair<double, double> > ranges;
    ranges.push(std::make_pair(left, right));

    while (ranges.empty() == false) {
        std::pair<double, double>& rng = ranges.top();
        ranges.pop();

        left = rng.first;
        right = rng.second;

        double delta = right - left;
        double mid = left + delta /2.0;

        if (mid < 0.01 || mid > 0.99) {
            break;
        }
        
        if ( delta > GdaConst::gda_autoweight_stop ) {
            double m_ssd = 0;
            // assume left is always not contiguity and right is always contiguity
            bool m_conti = CheckContiguity(weights, mid, m_ssd);
            if (m_conti) {
                if (mid < w) {
                    w = mid;
                }
                ranges.push(std::make_pair(left,mid));
            } else {
                ranges.push(std::make_pair(mid, right));
            }
        }
    }
    return w;
}

bool AbstractClusterDlg::CheckAllInputs()
{
    // default CheckAllInputs only has "output number of cluster" check
    int ncluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        ncluster = (int)value_ncluster;
    }
    if (ncluster < 2 || ncluster > rows) {
        wxString err_msg = _("Please enter a valid number of clusters.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }
    // check if X-Centroids selected but not projected
    if ((has_x_cent || has_y_cent) && check_spatial_ref) {
        bool cont_process = project->CheckSpatialProjection(check_spatial_ref);
        if (cont_process == false) {
            return false;
        }
    }
    return true;
}

GalWeight* AbstractClusterDlg::CheckSpatialWeights()
{
    GalWeight* weights = GetInputSpatialWeights();
    if (weights == NULL) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify a spatial weights."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }
    return weights;
}

void AbstractClusterDlg::OnAutoWeightCentroids(wxCommandEvent& event)
{
    // check if centroids are not projected
    if (check_spatial_ref) {
        bool cont = project->CheckSpatialProjection(check_spatial_ref);
        if (cont == false) {
            return;
        }
    }

    // start from 1.0 on the far right side
    m_weight_centroids->SetValue(100);
    m_wc_txt->SetValue("1.0");
    
    if (CheckAllInputs() == false) return;

    GalWeight* weights = CheckSpatialWeights();
    if (weights == NULL) return;

    // apply custom algorithm to find optimal weighting value between 0 and 1
    // when w = 1 (fully geometry based)
    // when w = 0 (fully attributes based)
    double w = BinarySearch(weights, 0.0, 1.0);
    int val = w * 100;
    m_weight_centroids->SetValue(val);
    m_wc_txt->SetValue(wxString::Format("%f", w));
}

void AbstractClusterDlg::AddTransformation(wxPanel *panel, wxFlexGridSizer* gbox)
{
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Transformation:"));
    const wxString _transform[6] = {"Raw", "Demean", "Standardize (Z)",
        "Standardize (MAD)", "Range Adjust", "Range Standardize"};
    combo_tranform = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(140,-1), 6, _transform);
    combo_tranform->SetSelection(2);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_tranform, 1, wxEXPAND);
}

void AbstractClusterDlg::AddNumberOfClusterCtrl(wxPanel *panel,
                                                wxFlexGridSizer* gbox,
                                                bool allow_dropdown)
{
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"));
    combo_n = new wxComboBox(panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                             wxSize(200,-1), 0, NULL);
    max_n_clusters = rows < 100 ? rows : 100;
    if (allow_dropdown) {
        for (int i=2; i<max_n_clusters+1; i++) {
            combo_n->Append(wxString::Format("%d", i));
        }
    }
    gbox->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_n, 1, wxEXPAND);
}

void AbstractClusterDlg::AddMinBound(wxPanel *panel, wxFlexGridSizer* gbox,
                                     bool show_checkbox)
{
    wxStaticText* st = new wxStaticText(panel, wxID_ANY, _("Minimum Bound:"));
    
    wxBoxSizer *hbox0 = new wxBoxSizer(wxHORIZONTAL);
    chk_floor = new wxCheckBox(panel, wxID_ANY, "");
    combo_floor = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                               wxSize(128,-1), var_items);
    txt_floor = new wxTextCtrl(panel, wxID_ANY, "1", wxDefaultPosition,
                               wxSize(70,-1), 0, validator);
    hbox0->Add(chk_floor);
    hbox0->Add(combo_floor);
    hbox0->Add(txt_floor);
    
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    slider_floor = new wxSlider(panel, wxID_ANY, 10, 0, 100, wxDefaultPosition,
                                wxSize(150,-1), wxSL_HORIZONTAL);
    txt_floor_pct = new wxTextCtrl(panel, wxID_ANY, "10%", wxDefaultPosition,
                                   wxSize(70,-1), 0, validator);
    hbox1->Add(slider_floor);
    hbox1->Add(txt_floor_pct);
    
    wxBoxSizer *hbox = new wxBoxSizer(wxVERTICAL);
    hbox->Add(hbox0);
    hbox->Add(hbox1);
    
    gbox->Add(st, 0, wxALIGN_TOP| wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox, 1, wxEXPAND);
    
    chk_floor->Bind(wxEVT_CHECKBOX, &AbstractClusterDlg::OnCheckMinBound, this);
    combo_floor->Bind(wxEVT_CHOICE, &AbstractClusterDlg::OnSelMinBound, this);
    txt_floor->Bind(wxEVT_KEY_DOWN, &AbstractClusterDlg::OnTypeMinBound, this);
    txt_floor_pct->Bind(wxEVT_KEY_DOWN, &AbstractClusterDlg::OnTypeMinPctBound, this);
	slider_floor->Bind(wxEVT_SLIDER, &AbstractClusterDlg::OnSlideMinBound, this);
    
    if (!show_checkbox) {
        chk_floor->SetValue(true);
        chk_floor->Hide();
        combo_floor->SetSelection(-1);
    } else {
        combo_floor->Disable();
        txt_floor->Disable();
    }
    slider_floor->Disable();
    txt_floor_pct->Disable();
    
}

void AbstractClusterDlg::OnSlideMinBound(wxCommandEvent& event)
{
    int idx = combo_floor->GetSelection();
    if (idx >= 0) {
        int val = slider_floor->GetValue();
        wxString t_val = wxString::Format("%d%%", val);
        txt_floor_pct->SetValue(t_val);
       
        if (idx_sum.find(idx) != idx_sum.end()) {
            double slide_val = (val / 100.0) * idx_sum[idx];
            wxString str_val;
            str_val << slide_val;
            txt_floor->SetValue(str_val);
        }
    }
}

void AbstractClusterDlg::OnCheckMinBound(wxCommandEvent& event)
{
    if (chk_floor->IsChecked() ) {
        combo_floor->Enable();
        txt_floor->Enable();
        combo_floor->SetSelection(-1);
        txt_floor->SetValue("");
    } else {
        combo_floor->Disable();
        txt_floor->Disable();
        slider_floor->Disable();
        txt_floor_pct->Disable();
        combo_floor->SetSelection(-1);
        txt_floor->SetValue("");
    }
}

void AbstractClusterDlg::OnSelMinBound(wxCommandEvent& event)
{
    int rows = project->GetNumRecords();
    int idx = combo_floor->GetSelection();
    if (idx >= 0) {
        slider_floor->Enable();
        txt_floor_pct->Enable();
        
        std::vector<double> floor_variable(rows, 1);
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        int tm = name_to_tm_id[combo_floor->GetString(idx)];
        table_int->GetColData(col, tm, floor_variable);
        // 10% as default to txt_floor
        double sum = 0;
        for (int i=0; i<rows; i++) {
            sum += floor_variable[i];
        }
        idx_sum[idx] = sum;
        double suggest = sum * 0.1;
        wxString str_suggest;
        str_suggest << suggest;
        txt_floor->SetValue(str_suggest);
        slider_floor->SetValue(10);
        txt_floor_pct->SetValue("10%");
    } else {
        slider_floor->Disable();
        txt_floor_pct->Disable();
        slider_floor->SetValue(10);
        txt_floor_pct->SetValue("10%");
    }
}

void AbstractClusterDlg::OnTypeMinBound(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_RETURN) {
        wxString tmp_val = txt_floor->GetValue();
        tmp_val.Trim(false);
        tmp_val.Trim(true);

        double input_val;
        bool is_valid = tmp_val.ToDouble(&input_val);
        if (is_valid) {
            // adjust slider
            // adjust percentage text ctrl
            int idx = combo_floor->GetSelection();
            if (idx >= 0) {
                if (idx_sum.find(idx) != idx_sum.end()) {
                    double slide_val = input_val / idx_sum[idx] * 100.0;
                    if (slide_val < 0) slide_val = 0;
                    if (slide_val > 100) slide_val = 100;
                    slider_floor->SetValue(slide_val);

                    wxString t_val = wxString::Format("%f%%", slide_val);
                    txt_floor_pct->SetValue(t_val);
                }
            }
        }
    }
    event.Skip();
}

void AbstractClusterDlg::OnTypeMinPctBound(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_RETURN) {
        // input could be 14% or 0.14
        wxString tmp_val = txt_floor_pct->GetValue();
        tmp_val.Trim(false);
        tmp_val.Trim(true);

        bool has_percentage =  tmp_val.Find('%') > 0;
        if (has_percentage) {
            tmp_val = tmp_val.SubString(0, tmp_val.Find('%')-1);
        }
        double input_val;
        bool is_valid = tmp_val.ToDouble(&input_val);
        if (is_valid) {
            // adjust bound text ctrl
            // adjust slider
            int idx = combo_floor->GetSelection();
            if (idx >= 0) {
                if (idx_sum.find(idx) != idx_sum.end()) {
                    double pct_val = has_percentage ? input_val / 100.0 : input_val;
                    if (pct_val < 0) pct_val = 0;
                    if (pct_val > 1) pct_val = 1;

                    double bound_val = pct_val * idx_sum[idx];
                    txt_floor->SetValue(wxString::Format("%f", bound_val));
                    slider_floor->SetValue(pct_val * 100);
                }
            }
        }
    }
    event.Skip();
}

bool AbstractClusterDlg::CheckMinBound()
{
    if (chk_floor->IsChecked()) {
        if ( combo_floor->GetSelection() < 0 ||
             txt_floor->GetValue().Trim() == wxEmptyString) {
            wxString err_msg = _("Please input minimum bound value.");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return false;
        }
    }
    return true;
}

void AbstractClusterDlg::InitVariableCombobox(wxListBox* var_box,
                                              bool integer_only)
{
    combo_var->Clear();
    var_items.Clear();
    
    std::vector<int> col_id_map;
    if (integer_only) table_int->FillIntegerColIdMap(col_id_map);
    else table_int->FillNumericColIdMap(col_id_map);
    for (int i=0, iend=(int)col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = name;
                name_to_tm_id[nm] = t;
                var_items.Add(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            var_items.Add(name);
        }
    }

    // Add centroids variable
    var_items.Add("<X-Centroids>");
    name_to_nm["<X-Centroids>"] = "<X-Centroids>";
    name_to_tm_id["<X-Centroids>"] = 0;
    var_items.Add("<Y-Centroids>");
    name_to_nm["<Y-Centroids>"] = "<Y-Centroids>";
    name_to_tm_id["<Y-Centroids>"] = 0;

    if (!var_items.IsEmpty()) {
        var_box->InsertItems(var_items,0);
    }
    for (int i=0; i<select_vars.size(); i++) {
        var_box->SetStringSelection(select_vars[i], true);
    }
}

bool AbstractClusterDlg::IsUseCentroids()
{
    bool use_centroids = false;

    if (m_use_centroids) use_centroids = m_use_centroids->GetValue();

    if (use_centroids && m_weight_centroids) {
        //std::cout << m_weight_centroids->GetValue() << std::endl;
        if (m_weight_centroids->GetValue() == 0) {
            use_centroids =  false;
        }
    }

    return use_centroids;
}

bool AbstractClusterDlg::GetInputData(int transform, int min_num_var)
{
    CleanData();
    
    bool use_centroids = false;
   
    if (m_use_centroids) use_centroids = m_use_centroids->GetValue();
    
    if (use_centroids && m_weight_centroids) {
        if (m_weight_centroids->GetValue() == 0) {
            use_centroids =  false;
        }
    }
    
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = (int)selections.size();
    if (num_var < min_num_var && !use_centroids) {
        wxString err_msg = wxString::Format(_("Please select at least %d variables."), min_num_var);
        wxMessageDialog dlg(NULL, err_msg, _("Info"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }
   
    col_names.clear();
    select_vars.clear();
    col_ids.clear();
    var_info.clear();

    if ( num_var > 0 ||
        (use_centroids && m_weight_centroids && m_weight_centroids->GetValue() != 0))
    {
        has_x_cent = false;
        has_y_cent = false;

        for (int i=0; i<num_var; i++) {
            int idx = selections[i];
            wxString sel_str = combo_var->GetString(idx);
            select_vars.push_back(sel_str);
            col_names.push_back(sel_str);
            wxString nm = name_to_nm[sel_str];
            if (nm == "<X-Centroids>") {
                has_x_cent = true;
            } else if (nm == "<Y-Centroids>") {
                has_y_cent = true;
            } else {
                int col = table_int->FindColId(nm);
                if (col == wxNOT_FOUND) {
                    wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
                    wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
                    dlg.ShowModal();
                    return false;
                }
                int tm = name_to_tm_id[combo_var->GetString(idx)];

                GdaVarTools::VarInfo v;
                v.time = tm;
                // Set Primary GdaVarTools::VarInfo attributes
                v.name = nm;
                v.is_time_variant = table_int->IsColTimeVariant(nm);
                // v.time already set above
                table_int->GetMinMaxVals(col, v.min, v.max);
                v.sync_with_global_time = v.is_time_variant;
                v.fixed_scale = true;
                col_ids.push_back(col);
                var_info.push_back(v);
            }
        }
        
        // Call function to set all Secondary Attributes based on Primary Attributes
        GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
        columns = 0;
        rows = project->GetNumRecords();
        std::vector<std::vector<double> > data(num_var - has_x_cent - has_y_cent);
        for (int i=0; i<var_info.size(); i++) {
            table_int->GetColData(col_ids[i], var_info[i].time, data[i]);
        }

        // for special table variables: <X-Centroids>, <Y-Centroids>
        if (has_x_cent) {
            std::vector<GdaPoint*> cents = project->GetCentroids();
            std::vector<double> xvals(rows);
            for (int i=0; i< rows; i++) {
                xvals[i] = cents[i]->GetX();
            }
            data.push_back(xvals);
        }
        if (has_y_cent) {
            std::vector<GdaPoint*> cents = project->GetCentroids();
            std::vector<double> yvals(rows);
            for (int i=0; i< rows; i++) {
                yvals[i] = cents[i]->GetY();
            }
            data.push_back(yvals);
        }

        // if use centroids (checkbox)
        if (use_centroids) {
            columns += 2;
            col_names.insert(col_names.begin(), "CENTY");
            col_names.insert(col_names.begin(), "CENTX");
        }
        
        // get columns (time variables always show upgrouped)
        columns += data.size();
        
        weight = GetWeights(columns);

        // init input_data[rows][cols]
        input_data = new double*[rows];
        mask = new int*[rows];
        for (int i=0; i<rows; i++) {
            input_data[i] = new double[columns];
            mask[i] = new int[columns];
            for (int j=0; j<columns; j++) {
                mask[i][j] = 1;
            }
        }
        
        // assign value
        int col_ii = 0;
        
        if (use_centroids) {
            std::vector<GdaPoint*> cents = project->GetCentroids();
            cent_xs.clear();
            cent_ys.clear();
            for (int i=0; i< rows; i++) {
                cent_xs.push_back(cents[i]->GetX());
                cent_ys.push_back(cents[i]->GetY());
            }
            if (transform == 5) {
                GenUtils::RangeStandardize(cent_xs);
                GenUtils::RangeStandardize(cent_ys);
            } else if (transform == 4) {
                GenUtils::RangeAdjust(cent_xs);
                GenUtils::RangeAdjust(cent_ys);
            } else if (transform == 3) {
                GenUtils::MeanAbsoluteDeviation(cent_xs);
                GenUtils::MeanAbsoluteDeviation(cent_ys);
            } else if (transform == 2) {
                GenUtils::StandardizeData(cent_xs );
                GenUtils::StandardizeData(cent_ys );
            } else if (transform == 1 ) {
                GenUtils::DeviationFromMean(cent_xs );
                GenUtils::DeviationFromMean(cent_ys );
            }
            for (int i=0; i< rows; i++) {
                input_data[i][col_ii + 0] = cent_xs[i];
                input_data[i][col_ii + 1] = cent_ys[i];
            }
            col_ii = 2;
        }
        for (int i=0; i<data.size(); i++ ){ // col
            std::vector<double>& vals = data[i];
            if (transform == 5) {
                GenUtils::RangeStandardize(vals);
            } else if (transform == 4) {
                GenUtils::RangeAdjust(vals);
            } else if (transform == 3) {
                GenUtils::MeanAbsoluteDeviation(vals);
            } else if (transform == 2) {
                GenUtils::StandardizeData(vals);
            } else if (transform == 1 ) {
                GenUtils::DeviationFromMean(vals);
            }
            for (int k=0; k< rows;k++) { // row
                input_data[k][col_ii] = vals[k];
            }
            col_ii += 1;
        }
        return true;
    }
    return false;
}

double* AbstractClusterDlg::GetWeights(int columns)
{
    if (weight != NULL) {
        delete[] weight;
        weight = NULL;
    }
    double* _weight = new double[columns];
    double wc = 1;
    for (int j=0; j<columns; j++){
        _weight[j] = 1;
    }
    if (m_weight_centroids && m_use_centroids) {
        if ( m_weight_centroids->GetValue() > 0 &&
             m_use_centroids->IsChecked()) {
            double sel_wc = m_weight_centroids->GetValue();
            wc = sel_wc / 100.0;
            double n_var_cols = (double)(columns - 2);
            for (int j=0; j<columns; j++){
                if (j==0 || j==1) {
                    _weight[j] = wc * 0.5;
                } else {
                    _weight[j] = (1 - wc) / n_var_cols;
                }
            }
        }
    }
    weight = _weight;
    return weight;
}

double AbstractClusterDlg::GetMinBound()
{
    double bound = 0;
    if (chk_floor->IsChecked() && combo_floor->GetSelection()>-1) {
        wxString tmp_val = txt_floor->GetValue();
        tmp_val.ToDouble(&bound);
    }
    return bound;
}

double* AbstractClusterDlg::GetBoundVals()
{
    int rows = project->GetNumRecords();
    int idx = combo_floor->GetSelection();
    double* vals = NULL;
    if (chk_floor->IsChecked() && idx >= 0) {
        vals = new double[rows];
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col != wxNOT_FOUND) {
            std::vector<double> floor_variable(rows, 1);
            int tm = name_to_tm_id[combo_floor->GetString(idx)];
            table_int->GetColData(col, tm, floor_variable);
            for (int i=0; i<rows; i++) {
                vals[i] = floor_variable[i];
            }
        }
    }
    return vals;
}


wxNotebook* AbstractClusterDlg::AddSimpleReportCtrls(wxPanel *panel)
{
	wxNotebook* notebook = new wxNotebook( panel, wxID_ANY, wxDefaultPosition);
    m_reportbox = new SimpleReportTextCtrl(notebook, wxID_ANY, "");
    notebook->AddPage(m_reportbox, _("Summary"));
	return notebook;
}


////////////////////////////////////////////////////////////////
//
// Clustering Stats
//
////////////////////////////////////////////////////////////////

double AbstractClusterDlg::CreateSummary(const std::vector<wxInt64>& clusters,
                                         bool show_print,
                                         bool return_additional_summary)
{
    std::vector<std::vector<int> > solution;
    std::vector<int> isolated;
    for (int i=0; i<clusters.size(); i++) {
        int c = (int)clusters[i];
        if (c > solution.size()) solution.resize(c);
        
        if (c-1 >= 0)
            solution[c-1].push_back(i);
        else
            isolated.push_back(i);
    }
    return CreateSummary(solution, isolated, show_print, return_additional_summary);
}

double AbstractClusterDlg::CreateSummary(const std::vector<std::vector<int> >& solution,
                                         const std::vector<int>& isolated,
                                         bool show_print,
                                         bool return_additional_summary)
{
    // get noise data (not clustered)
    std::vector<bool> noises(rows, true);
    for (int i=0; i<solution.size(); ++i) {
        for (int j=0; j<solution[i].size(); ++j) {
            noises[solution[i][j]] = false;
        }
    }
    // compute Sum of Squared Differences (from means)
    // mean centers
    std::vector<std::vector<double> > mean_centers = _getMeanCenters(solution);
    // totss
    double totss = _getTotalSumOfSquares(noises);
    // withinss
    std::vector<double> withinss = _getWithinSumOfSquares(solution);
    // tot.withiness
    double totwithiness = GenUtils::Sum(withinss);
    // betweenss
    double betweenss = totss - totwithiness;
    // ratio
    double ratio = betweenss / totss;
    
    wxString summary;
    summary << "------\n";
    if (isolated.size()>0)
        summary << _("Number of observations not in a cluster: ") << isolated.size() << "\n";
    summary << _printConfiguration();
    
    // auto weighting
    if (m_use_centroids != NULL && m_use_centroids->IsChecked()) {
        wxString w_val = m_wc_txt->GetValue();
        double w_valf = 0;
        if (w_val.ToDouble(&w_valf)) {
            double w_valf_vars = 1 - w_valf;
            w_valf = w_valf * 0.5;
            w_valf_vars = w_valf_vars / (columns - 2);
            
            summary << _("Use geometric centroids (weighting): \n");
            for (int i=0; i<columns; i++) {
                if (col_names[i] == "CENTX") {
                    summary <<"  " << _("Centroid (X)") << " " << w_valf << "\n";
                } else if (col_names[i] == "CENTY") {
                    summary <<"  " << _("Centroid (Y)") << " " << w_valf << "\n";
                } else {
                    summary <<"  " << col_names[i] << " " << w_valf_vars << "\n";
                }
            }
        }
        
    }
    summary << "\n";
    
    summary << _printMeanCenters(mean_centers);
    summary << _("The total sum of squares:\t") << totss << "\n";
    summary << _printWithinSS(withinss);
    summary << _("The total within-cluster sum of squares:\t") << totwithiness << "\n";
    summary << _("The between-cluster sum of squares:\t") << betweenss << "\n";
    summary << _("The ratio of between to total sum of squares:\t") << ratio << "\n\n";
    
    // allow any inherited class to report additional text in summary
    double additional_ratio = 0;
    summary << _additionalSummary(solution, additional_ratio);
    if (return_additional_summary) {
        ratio = additional_ratio;
    }
    if (m_reportbox && show_print) {
        wxString report = m_reportbox->GetValue();
        report = summary + report;
        m_reportbox->SetValue(report);
    }
    
    return ratio;
}

std::vector<std::vector<double> > AbstractClusterDlg::_getMeanCenters(const std::vector<std::vector<int> >& solutions)
{
    int n_clusters = (int)solutions.size();
    std::vector<std::vector<double> > result(n_clusters);
    
    if (columns <= 0 || rows <= 0) return result;

    std::vector<std::vector<double> > raw_data;
    raw_data.resize(col_ids.size());
    for (int i=0; i<var_info.size(); i++) {
        table_int->GetColData(col_ids[i], var_info[i].time, raw_data[i]);
    }

    if (has_x_cent) {
        std::vector<GdaPoint*> cents = project->GetCentroids();
        std::vector<double> xvals(rows);
        for (int i=0; i< rows; i++) {
            xvals[i] = cents[i]->GetX();
        }
        raw_data.push_back(xvals);
    }
    if (has_y_cent) {
        std::vector<GdaPoint*> cents = project->GetCentroids();
        std::vector<double> yvals(rows);
        for (int i=0; i< rows; i++) {
            yvals[i] = cents[i]->GetY();
        }
        raw_data.push_back(yvals);
    }

    for (int i=0; i<solutions.size(); i++ ) {
        std::vector<double> means;
        int end = columns;
        if (IsUseCentroids()) {
            end = columns - 2;
            means.push_back(0); // CENT_X
            means.push_back(0); // CENT_Y
        }
        for (int c=0; c<end; c++) {
            double sum = 0;
            double n = 0;
            for (int j=0; j<solutions[i].size(); j++) {
                int r = solutions[i][j];
                if (mask[r][c] == 1) {
                    sum += raw_data[c][r];
                    n += 1;
                }
            }
            double mean = n > 0 ? sum / n : 0;
            //if (weight) mean = mean * weight[c];
            means.push_back(mean);
        }
        result[i] = means;
    }
    
    return result;
}

double AbstractClusterDlg::_getTotalSumOfSquares(const std::vector<bool>& noises)
{
    if (columns <= 0 || rows <= 0) return 0;
   
    double ssq = 0.0;
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY") {
            continue;
        }
        std::vector<double> vals;
        for (int j=0; j<rows; j++) {
            if (mask[j][i] == 1 && noises[j] == false) {
                vals.push_back(input_data[j][i]);
            }
        }
        double ss = GenUtils::SumOfSquares(vals);
        ssq += ss;
    }
    return ssq;
}

std::vector<double> AbstractClusterDlg::_getWithinSumOfSquares(const std::vector<std::vector<int> >& solution)
{
    // solution is a list of lists of region ids [[1,7,2],[0,4,3],...] such
    // that the first solution has areas 1,7,2 the second solution 0,4,3 and so
    // on. cluster_ids does not have to be exhaustive
    std::vector<double> wss;
    for (int i=0; i<solution.size(); i++ ) {
        double ss = _calcSumOfSquares(solution[i]);
        wss.push_back(ss);
    }
    return wss;
}


double AbstractClusterDlg::_calcSumOfSquares(const std::vector<int>& cluster_ids)
{
    if (cluster_ids.empty() || input_data==NULL || mask == NULL)
        return 0;
    
    double ssq = 0;
    
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY") {
            continue;
        }
        std::vector<double> vals;
        for (int j=0; j<cluster_ids.size(); j++) {
            int r = cluster_ids[j];
            if (mask[r][i] == 1)
                vals.push_back(input_data[r][i]);
        }
        double ss = GenUtils::SumOfSquares(vals);
        ssq += ss;
    }
    
    return ssq;
}


wxString AbstractClusterDlg::_printMeanCenters(const std::vector<std::vector<double> >& mean_centers)
{
    wxString txt;
    txt << _("Cluster centers:") << mean_center_type << "\n";
    
    stringstream ss;
    TextTable t( TextTable::MD );
   
    //       v1     v2    v3
    //  c1   1      2      3
    //  c2   1      2      3
    
    // first row
    t.add("");
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY")
            continue;
        t.add(col_names[i].ToStdString());
    }
    t.endOfRow();
    
    // second row
    for (int i=0; i<mean_centers.size(); i++) {
        ss.str("");
        ss << "C" << i+1;
        t.add(ss.str());
        
        const std::vector<double>& vals = mean_centers[i];
        for (int j=0; j<vals.size(); j++) {
            if (col_names[j] == "CENTX" || col_names[j] == "CENTY")
                continue;
            ss.str("");
            ss << vals[j];
            t.add(ss.str());
        }
        t.endOfRow();
    }
    
    stringstream ss1;
    ss1 << t;
    txt << ss1.str();
    txt << "\n";
    return txt;
}

wxString AbstractClusterDlg::_printWithinSS(const std::vector<double>& within_ss,
                                            const wxString& title, const wxString& header)
{
    wxString summary;
    summary << title;
    
    //            # obs  Within cluster SS
    // C1          12            62.1
    // C2          3             42.3
    // C3
    
    wxString ss_str = header;
    
    stringstream ss;
    TextTable t( TextTable::MD );
    
    // first row
    t.add("");
    //t.add("#obs");
    t.add(ss_str.ToStdString());
    t.endOfRow();
   
    // second row
    for (int i=0; i<within_ss.size(); i++) {
        ss.str("");
        ss << "C" << i+1;
        t.add(ss.str());
        
        ss.str("");
        ss << within_ss[i];
        t.add(ss.str());
        t.endOfRow();
    }
    //t.setAlignment( 4, TextTable::Alignment::RIGHT );
    
    stringstream ss1;
    ss1 << t;
    summary << ss1.str();
    summary << "\n";
    
    return summary;
}

wxString AbstractClusterDlg::_printWithinSS(const std::vector<double>& within_ss,
                                     const std::vector<double>& avgs,
                                     const wxString& title,
                                     const wxString& header1,
                                     const wxString& header2)
{
    wxString summary;
    summary << title;

    //            # obs  Within cluster SS  Average
    // C1          12            62.1           x
    // C2          3             42.3           x
    // C3

    wxString ss_str = header1;
    wxString ss_str2 = header2;
    stringstream ss;
    TextTable t( TextTable::MD );

    // first row
    t.add("");
    //t.add("#obs");
    t.add(ss_str.ToStdString());
    t.add(ss_str2.ToStdString());
    t.endOfRow();

    // second row
    for (int i=0; i<within_ss.size(); i++) {
        ss.str("");
        ss << "C" << i+1;
        t.add(ss.str());

        ss.str("");
        ss << within_ss[i];
        t.add(ss.str());

        ss.str("");
        ss << avgs[i];
        t.add(ss.str());

        t.endOfRow();
    }
    //t.setAlignment( 4, TextTable::Alignment::RIGHT );

    stringstream ss1;
    ss1 << t;
    summary << ss1.str();
    summary << "\n";

    return summary;
}

