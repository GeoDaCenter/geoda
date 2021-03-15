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

#include <boost/thread.hpp>
#include <boost/bind.hpp>

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

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/pam.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "KMeansDlg.h"


BEGIN_EVENT_TABLE( KClusterDlg, wxDialog )
EVT_CLOSE( KClusterDlg::OnClose )
END_EVENT_TABLE()

KClusterDlg::KClusterDlg(wxFrame* parent_s, Project* project_s, wxString title)
: AbstractClusterDlg(parent_s, project_s, title)
{
    wxLogMessage("In KClusterDlg()");
    distmatrix = NULL;
    show_iteration = true;
}

KClusterDlg::~KClusterDlg()
{
    wxLogMessage("In ~KClusterDlg()");
}

void KClusterDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(940,820), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    bool show_auto_button = true;
    AddInputCtrls(panel, vbox, show_auto_button);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);
    
	// NumberOfCluster Control
    AddNumberOfClusterCtrl(panel, gbox);
    
	// Minimum Bound Control
    AddMinBound(panel, gbox);
    
    // Transformation Control
    AddTransformation(panel, gbox);
    
    // Initialization Method
    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY, _("Initialization Method:"));
    wxString choices16[] = {"KMeans++", "Random"};
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(200,-1), 2, choices16);
    combo_method->SetSelection(0);

    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);
    
    if (!show_initmethod) {
        st16->Hide();
        combo_method->Hide();
        combo_method->SetSelection(1); // use Random if hide init
    } 
    
    wxStaticText* st10 = new wxStaticText(panel, wxID_ANY, _("Initialization Re-runs:"));
    m_pass = new wxTextCtrl(panel, wxID_ANY, "150", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st10, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_pass, 1, wxEXPAND);
    
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
    
    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("Maximum Iterations:"));
    m_iterations = new wxTextCtrl(panel, wxID_ANY, "1000", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_iterations, 1, wxEXPAND);
    
    if (!show_iteration) {
        st11->Hide();
        m_iterations->Hide();
    }
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {_("Euclidean"), _("Manhattan")};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    if (!show_distance) {
        st13->Hide();
        m_distance->Hide();
        m_distance->SetSelection(1); // set manhattan
    }

    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"));
    m_textbox = new wxTextCtrl(panel, wxID_ANY, "CL", wxDefaultPosition, wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(m_textbox, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
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
    okButton->Bind(wxEVT_BUTTON, &KClusterDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &KClusterDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &KClusterDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &KClusterDlg::OnChangeSeed, this);
    combo_method->Bind(wxEVT_CHOICE, &KClusterDlg::OnInitMethodChoice, this);
    m_distance->Bind(wxEVT_CHOICE, &KClusterDlg::OnDistanceChoice, this);
}

vector<vector<double> > KClusterDlg::_getMeanCenters(const vector<vector<int> >& solution)
{
    return AbstractClusterDlg::_getMeanCenters(solution);
}

void KClusterDlg::OnDistanceChoice(wxCommandEvent& event)
{
    if (m_distance->GetSelection() == 1) {
        // when Manhattan
        // make sure KMedian  and KMedoids is select
    }
}

void KClusterDlg::OnInitMethodChoice(wxCommandEvent& event)
{
    if (combo_method->GetSelection()== 0) {
        // when KMeans++
        // make sure no KMedian is select
    }
}

void KClusterDlg::OnSeedCheck(wxCommandEvent& event)
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

void KClusterDlg::OnChangeSeed(wxCommandEvent& event)
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

void KClusterDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose KClusterDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void KClusterDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close KClusterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

wxString KClusterDlg::_printConfiguration()
{
    int ncluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        ncluster = (int)value_ncluster;
    }
    
    wxString txt;
    txt << _("Method:\t") << cluster_method << "\n";
    txt << _("Number of clusters:\t") << ncluster << "\n";
    txt << _("Initialization method:\t") << combo_method->GetString(combo_method->GetSelection()) << "\n";
    txt << _("Initialization re-runs:\t") << m_pass->GetValue() << "\n";
    txt << _("Maximum iterations:\t") << m_iterations->GetValue() << "\n";
    
    if (chk_floor && chk_floor->IsChecked()) {
        int idx = combo_floor->GetSelection();
        wxString nm = name_to_nm[combo_floor->GetString(idx)];
        txt << _("Minimum bound:\t") << txt_floor->GetValue() << "(" << nm << ")" << "\n";
    }
    
    txt << _("Transformation:\t") << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";
   
    txt << _("Distance function:\t") << m_distance->GetString(m_distance->GetSelection()) << "\n";
    
    return txt;
}

void KClusterDlg::ComputeDistMatrix(int dist_sel)
{
    // this only be called by KMedoid, which distmatrix will be used as input
}

bool KClusterDlg::CheckAllInputs()
{
    n_cluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        n_cluster = (int)value_ncluster;
    }
    if (n_cluster < 2 || n_cluster > rows) {
        wxString err_msg = _("Please enter a valid number of clusters.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }

    transform = combo_tranform->GetSelection();

    if (GetInputData(transform,1) == false) return false;
    // check if X-Centroids selected but not projected
    if ((has_x_cent || has_y_cent) && check_spatial_ref) {
        bool cont_process = project->CheckSpatialProjection(check_spatial_ref);
        if (cont_process == false) {
            return false;
        }
    }

    if (!CheckMinBound()) return false;

    n_pass = 10;
    wxString str_pass = m_pass->GetValue();
    long l_pass;
    if(str_pass.ToLong(&l_pass)) {
        n_pass = (int)l_pass;
    }

    n_maxiter = 300; // max iteration of EM
    wxString iterations = m_iterations->GetValue();
    long l_maxiter;
    if(iterations.ToLong(&l_maxiter)) {
        n_maxiter = (int)l_maxiter;
    }

    meth_sel = combo_method->GetSelection();

    dist_sel = m_distance->GetSelection();

    return true;
}

bool KClusterDlg::Run(vector<wxInt64>& clusters)
{
    if (GdaConst::use_gda_user_seed) {
        setrandomstate((int)GdaConst::gda_user_seed);
        resetrandom();
    } else {
        setrandomstate(-1);
        resetrandom();
    }

    // NOTE input_data should be retrieved first!!
    // get input: weights (auto)
    // this function has to be called when use auto-weighting
    weight = GetWeights(columns);

    // start working
    int nCPUs = boost::thread::hardware_concurrency();
    int quotient = n_pass / nCPUs;
    int remainder = n_pass % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
    map<double, vector<wxInt64> >::iterator it;
    for (it=sub_clusters.begin(); it!=sub_clusters.end(); it++) {
        it->second.clear();
    }
    sub_clusters.clear();

    ComputeDistMatrix(dist_sel);
    
    double min_bound = GetMinBound();
    double* bound_vals = GetBoundVals();
    
    int s1 = 0;
    if (GdaConst::use_gda_user_seed) {
        srand((int)GdaConst::gda_user_seed);
        s1 = rand();
    }
    
    boost::thread_group threadPool;
    for (int i=0; i<tot_threads; i++) {
        int a=0;
        int b=0;
        if (i < remainder) {
            a = i*(quotient+1);
            b = a+quotient;
        } else {
            a = remainder*(quotient+1) + (i-remainder)*quotient;
            b = a+quotient-1;
        }
        
        if (s1 >0) s1 = a + 1;
        int n_runs = b - a + 1;
        
        boost::thread* worker = new boost::thread(
            boost::bind(&KClusterDlg::doRun, this, s1, n_cluster, n_runs,
                        n_maxiter, meth_sel, dist_sel, min_bound, bound_vals));
        
        threadPool.add_thread(worker);
    }
    threadPool.join_all();
    
    delete[] bound_vals;
    
    bool start = false;
    double min_error = 0;

    for (it=sub_clusters.begin(); it!=sub_clusters.end(); it++) {
        double error = it->first;
        vector<wxInt64>& clst = it->second;
        if (start == false ) {
            min_error = error;
            clusters = clst;
            start = true;
        } else {
            if (error < min_error) {
                min_error = error;
                clusters = clst;
            }
        }
    }
    return true;
}

void KClusterDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click KClusterDlg::OnOK");

    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    if (CheckAllInputs() == false) return;

    vector<wxInt64> clusters;
    if (Run(clusters) == false) return;
    
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

    // summary
    CreateSummary(cluster_ids);
    
    // save to table
    int time=0;
    int col = table_int->FindColId(field_name);
    if ( col == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_long_len;
        int m_decimals_val = 0;
        
        col = table_int->InsertCol(GdaConst::long64_type, field_name,
                                   col_insert_pos, time_steps,
                                   m_length_val, m_decimals_val);
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
    if (project->IsTableOnlyProject())  return;
    
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
    wxString tmp = _("%s Cluster Map (%d clusters)");
    wxString ttl = wxString::Format(tmp, cluster_method, n_cluster);
    nf->SetTitle(ttl);
}

////////////////////////////////////////////////////////////////////////
//
// KMeans
////////////////////////////////////////////////////////////////////////
KMeansDlg::KMeansDlg(wxFrame *parent, Project* project)
: KClusterDlg(parent, project, _("KMeans Clustering Settings"))
{
    wxLogMessage("In KMeansDlg()");
   
    show_initmethod = true;
    show_distance = true;
    show_iteration = true;
    cluster_method = "KMeans";
    
    CreateControls();
    m_distance->Disable();
}

KMeansDlg::~KMeansDlg()
{
    wxLogMessage("In ~KMeansDlg()");
}

void KMeansDlg::doRun(int s1,int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals)
{
    char method = 'a'; // 'a' mean/random, 'b' kmeans++ 'm' median
    if (meth_sel == 0) method = 'b';
    
    char dist_choices[] = {'e','b'};
    char dist = 'e'; // euclidean
    dist = dist_choices[dist_sel];
    
    int transpose = 0; // row wise
    double error;
    int ifound;
    int* clusterid = new int[rows];
    
    int s2 = s1==0 ? 0 : s1 + npass;
    kcluster(ncluster, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound, bound_vals, min_bound, s1, s2);
    
    vector<wxInt64> clusters;
    for (int i=0; i<rows; i++) {
        clusters.push_back(clusterid[i] + 1);
    }
    sub_clusters[error] = clusters;
    
    delete[] clusterid;
}

////////////////////////////////////////////////////////////////////////
//
// KMedians
////////////////////////////////////////////////////////////////////////
KMediansDlg::KMediansDlg(wxFrame *parent, Project* project)
: KClusterDlg(parent, project, _("KMedians Clustering Settings"))
{
    wxLogMessage("In KMediansDlg()");
    
    show_initmethod = false;
    show_distance = true;
    show_iteration = true;
    cluster_method = "KMedians";
    mean_center_type = " (median)";
    return_additional_summary = true; // for binary search, using kmedian measure

    CreateControls();
    m_distance->SetSelection(1); // set manhattan
    m_distance->Disable();
}

KMediansDlg::~KMediansDlg()
{
    wxLogMessage("In ~KMedians()");
}

void KMediansDlg::doRun(int s1,int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals)
{
    char method = 'm'; // 'm' median/random
    int transpose = 0; // row wise
    
    char dist_choices[] = {'e','b'};
    char dist = 'e'; // euclidean
    dist = dist_choices[dist_sel];
    
    double error;
    int ifound;
    int* clusterid = new int[rows];
    
    int s2 = s1==0 ? 0 : s1 + npass;
    kcluster(ncluster, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound, bound_vals, min_bound, s1, s2);
    
    vector<wxInt64> clusters;
    for (int i=0; i<rows; i++) {
        clusters.push_back(clusterid[i] + 1);
    }
    sub_clusters[error] = clusters;
    
    delete[] clusterid;
}

vector<vector<double> > KMediansDlg::_getMeanCenters(const vector<vector<int> >& solutions)
{
    int n_clusters = (int)solutions.size();
    vector<vector<double> > result(n_clusters);
    
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

    //int start = IsUseCentroids() ? 2 : 0;
    for (int i=0; i<solutions.size(); i++ ) {
        vector<double> medians;
        int end = columns;
        if (IsUseCentroids()) {
            end = columns - 2;
            medians.push_back(0); // CENT_X
            medians.push_back(0); // CENT_Y
        }
        for (int c=0; c<end; c++) {
            //double sum = 0;
            int n = 0;
            double* data = new double[solutions[i].size()];
            for (int j=0; j<solutions[i].size(); j++) {
                int r = solutions[i][j];
                if (mask[r][c] == 1) {
                    //data[n] = input_data[r][c];
                    data[n] = raw_data[c][r];
                    n += 1;
                }
            }
            double v = median(n, data);
            //if (weight) mean = mean * weight[c];
            medians.push_back(v);
            delete[] data;
        }
        result[i] = medians;
    }
    
    return result;
}

double KMediansDlg::_calcSumOfSquaresMedian(const vector<int>& cluster_ids)
{
    if (cluster_ids.empty() || input_data==NULL || mask == NULL)
        return 0;
    
    double ssq = 0;
    
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY") {
            continue;
        }
        vector<double> vals;
        for (int j=0; j<cluster_ids.size(); j++) {
            int r = cluster_ids[j];
            if (mask[r][i] == 1)
                vals.push_back(input_data[r][i]);
        }
        double ss = GenUtils::SumOfSquaresMedian(vals);
        ssq += ss;
    }
    
    return ssq;
}

double KMediansDlg::_calcSumOfManhattanMedian(const vector<int>& cluster_ids)
{
    if (cluster_ids.empty() || input_data==NULL || mask == NULL)
        return 0;
    
    double ssq = 0;
    
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY") {
            continue;
        }
        vector<double> vals;
        for (int j=0; j<cluster_ids.size(); j++) {
            int r = cluster_ids[j];
            if (mask[r][i] == 1)
                vals.push_back(input_data[r][i]);
        }
        double ss = GenUtils::SumOfManhattanMedian(vals);
        ssq += ss;
    }
    
    return ssq;
}

wxString KMediansDlg::_additionalSummary(const vector<vector<int> >& solution,
                                         double& additional_ratio)
{
    // computing Sum of Square Differences from Medoids
    if (columns <= 0 || rows <= 0) return wxEmptyString;
    
    int dist_sel = m_distance->GetSelection();
    
    double totss = 0, totwithiness, betweenss, ratio;
    vector<double> withinss, avgs;
    
    wxString summary;
    
    if (dist_sel == 0) {
        // euclidean distance
        summary << _("(Using Euclidean distance (squared) to medians)\n");
        // totss double totss = _getTotalSumOfSquares();
        for (int i=0; i<columns; i++) {
            if (col_names[i] == "CENTX" || col_names[i] == "CENTY")
                continue;
            vector<double> vals;
            for (int j=0; j<rows; j++) {
                if (mask[j][i] == 1)
                    vals.push_back(input_data[j][i]);
            }
            double ss = GenUtils::SumOfSquaresMedian(vals);
            totss += ss;
        }
        // withinss
        for (int i=0; i<solution.size(); i++ ) {
            double ss = _calcSumOfSquaresMedian(solution[i]);
            withinss.push_back(ss);
            avgs.push_back(ss / solution[i].size());
        }
        // tot.withiness
        totwithiness = GenUtils::Sum(withinss);
        // ratio
        ratio = totwithiness / totss;

    } else {
        // manhattan distance
        summary << _("(Using Manhattan distance to medians)\n");
        // totss double totss = _getTotalSumOfSquares();
        for (int i=0; i<columns; i++) {
            if (col_names[i] == "CENTX" || col_names[i] == "CENTY")
                continue;
            vector<double> vals;
            for (int j=0; j<rows; j++) {
                if (mask[j][i] == 1)
                    vals.push_back(input_data[j][i]);
            }
            double ss = GenUtils::SumOfManhattanMedian(vals);
            totss += ss;
        }
        // withinss
        for (int i=0; i<solution.size(); i++ ) {
            double ss = _calcSumOfManhattanMedian(solution[i]);
            withinss.push_back(ss);
            avgs.push_back(ss / solution[i].size());
        }
        // tot.withiness
        totwithiness = GenUtils::Sum(withinss);
        // betweenss
        //betweenss = totss - totwithiness;
        // ratio
        ratio = totwithiness / totss;
    }

    additional_ratio = 1 - ratio;

    summary << _("The total sum of distance:\t") << totss << "\n";
    summary << _printWithinSS(withinss, avgs, _("Within-cluster sum of distances:\n"),
                              _("Within Cluster D"), _("Average"));
    summary << _("The total within-cluster sum of distance:\t") << totwithiness << "\n";
    summary << _("The ratio of total within to total sum of distance: ") << ratio << "\n\n";
    return summary;
}
////////////////////////////////////////////////////////////////////////
//
// KMedoids
////////////////////////////////////////////////////////////////////////
KMedoidsDlg::KMedoidsDlg(wxFrame *parent, Project* project)
: KClusterDlg(parent, project, _("KMedoids Clustering Settings"))
{
    wxLogMessage("In KMedoidsDlg()");
    
    show_initmethod = false;
    show_distance = true;
    show_iteration = true;
    cluster_method = "KMedoids";
    mean_center_type = " (medoid)";
    return_additional_summary = true; // for binary search, using kmeoids measure
    
    CreateControls();
    m_distance->SetSelection(1); // set manhattan
}

KMedoidsDlg::~KMedoidsDlg()
{
    wxLogMessage("In ~KMedoidsDlg()");
}

void KMedoidsDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(940,840), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );

    wxPanel *panel = new wxPanel(scrl);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

    // Input
    bool show_auto_button = true;
    AddInputCtrls(panel, vbox, show_auto_button);

    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(13, 2, 5, 0);

    // NumberOfCluster Control
    AddNumberOfClusterCtrl(panel, gbox);

    // Minimum Bound Control
   // AddMinBound(panel, gbox);

    // Transformation Control
    AddTransformation(panel, gbox);

    // KMedoids method
    wxStaticText* st15 = new wxStaticText(panel, wxID_ANY, _("Method:"));
    wxString choices15[] = {"FastPAM", "FastCLARA", "FastCLARANS"};
    combo_method = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(200,-1), 3, choices15);
    combo_method->SetSelection(0);

    gbox->Add(st15, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_method, 1, wxEXPAND);

    // Initialization Method
    txt_initmethod = new wxStaticText(panel, wxID_ANY, _("Initialization Method:"));
    wxString choices16[] = {"BUILD", "LAB"};
    combo_initmethod = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(200,-1), 2, choices16);
    combo_initmethod->SetSelection(1);
    //txt_initmethod->Hide();
    //combo_initmethod->Hide();

    gbox->Add(txt_initmethod, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_initmethod, 1, wxEXPAND);

    txt_iterations = new wxStaticText(panel, wxID_ANY, _("Maximum Iterations:"));
    m_iterations = new wxTextCtrl(panel, wxID_ANY, "10", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(txt_iterations, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_iterations, 1, wxEXPAND);
    txt_iterations->Hide();
    m_iterations->Hide();

    wxStaticText* st10 = new wxStaticText(panel, wxID_ANY, "");
    m_fastswap = new wxCheckBox(panel, wxID_ANY, _("Use Additonal Swaps (FastPAM2)"));
    m_fastswap->SetValue(true); // default 1
    gbox->Add(st10, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_fastswap, 1, wxEXPAND);
    st10->Hide();
    m_fastswap->Hide();

    // FastCLARA and FastCLARANS
    txt_numsamples = new wxStaticText(panel, wxID_ANY, _("Number of Samples/Iterations:"));
    m_numsamples = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(txt_numsamples, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_numsamples, 1, wxEXPAND);

    txt_sampling = new wxStaticText(panel, wxID_ANY, _("Sample Size/Rate:"));
    m_sampling = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(200,-1));
    gbox->Add(txt_sampling, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_sampling, 1, wxEXPAND);

    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, "");
    m_keepmed = new wxCheckBox(panel, wxID_ANY, "Include Previous Medoids");
    m_keepmed->SetValue(true);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_keepmed, 1, wxEXPAND);

    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {_("Euclidean"), _("Manhattan")};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    if (!show_distance) {
        st13->Hide();
        m_distance->Hide();
        m_distance->SetSelection(1); // set manhattan
    }

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
    hbox1->Add(m_textbox, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
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
    okButton->Bind(wxEVT_BUTTON, &KClusterDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &KClusterDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &KClusterDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &KClusterDlg::OnChangeSeed, this);
    combo_method->Bind(wxEVT_CHOICE, &KMedoidsDlg::OnMethodChoice, this);
    m_distance->Bind(wxEVT_CHOICE, &KClusterDlg::OnDistanceChoice, this);

    wxCommandEvent ev;
    OnMethodChoice(ev);
}

void KMedoidsDlg::OnMethodChoice(wxCommandEvent& evt)
{
    long k = 0;
    combo_n->GetValue().ToLong(&k);

    if (evt.GetSelection() == 0) {
        // FastPAM
        bool flag = true;
        txt_initmethod->Enable(flag);
        combo_initmethod->Enable(flag);
        txt_iterations->Enable(flag);
        m_iterations->Enable(flag);
        m_fastswap->Enable(flag);

        flag = false;
        txt_numsamples->Enable(flag);
        m_numsamples->Enable(flag);
        txt_sampling->Enable(flag);
        m_sampling->Enable(flag);
        m_keepmed->Enable(flag);
    } else if (evt.GetSelection() == 1) {
        // FastCLARA
        bool flag = true;
        txt_initmethod->Enable(flag);
        combo_initmethod->Enable(flag);
        txt_iterations->Enable(flag);
        m_iterations->Enable(flag);
        m_fastswap->Enable(flag);

        txt_numsamples->Enable(flag);
        m_numsamples->Enable(flag);
        txt_sampling->Enable(flag);
        m_sampling->Enable(flag);
        m_keepmed->Enable(flag);
        m_keepmed->SetValue(true);

        m_numsamples->SetValue(rows <= 100 ? "5" : "10");
        // Larger sample size, used by Schubert and Rousseeuw, 2019
        //  80 + 4. * k
        int ns = rows <= 100 ? 40 + 2*k : 80 + 4*k;
        if (ns >= rows) ns = rows;
        m_sampling->SetValue(wxString::Format("%d", ns));
    } else {
        // FastCLARANS
        bool flag = false;
        txt_initmethod->Enable(flag);
        combo_initmethod->Enable(flag);
        txt_iterations->Enable(flag);
        m_iterations->Enable(flag);
        m_fastswap->Enable(flag);

        flag = true;
        txt_numsamples->Enable(flag);
        m_numsamples->Enable(flag);
        txt_sampling->Enable(flag);
        m_sampling->Enable(flag);
        m_keepmed->Enable(!flag);

        m_numsamples->SetValue("2");
        m_sampling->SetValue(wxString::Format("%f", 0.025));
    }
}

void KMedoidsDlg::ComputeDistMatrix(int dist_sel)
{
    int transpose = 0; // row wise
    char dist = 'b'; // city-block
    if (dist_sel == 0) dist = 'e';
    
    distmatrix = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);
}

bool KMedoidsDlg::CheckAllInputs()
{
    n_cluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        n_cluster = (int)value_ncluster;
    }
    if (n_cluster < 1 || n_cluster > rows) {
        wxString err_msg = _("Please enter a valid number of clusters.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }

    transform = combo_tranform->GetSelection();

    if (GetInputData(transform,1) == false) return false;

    // check if X-Centroids selected but not projected
    if ((has_x_cent || has_y_cent) && check_spatial_ref) {
        bool cont_process = project->CheckSpatialProjection(check_spatial_ref);
        if (cont_process == false) {
            return false;
        }
    }

    n_maxiter = 10;
    wxString iterations = m_iterations->GetValue();
    long l_maxiter;
    if(iterations.ToLong(&l_maxiter)) {
        n_maxiter = (int)l_maxiter;
    }

    dist_sel = m_distance->GetSelection();

    return true;
}

int KMedoidsDlg::GetFirstMedoid(double** distmatrix)
{
    int n = 0;
    double min_sum = DBL_MAX, tmp_sum=0;
    for (size_t i=0; i<rows; ++i) {
        // sum of distance from i to everyone else
        tmp_sum = 0;
        for (size_t j=0; j<rows; ++j) {
            if (i != j) {
                tmp_sum += i > j ? distmatrix[i][j] : distmatrix[j][i];
            }
        }
        if (tmp_sum < min_sum) {
            n = i;
            min_sum = tmp_sum;
        }
    }

    return n;
}

bool KMedoidsDlg::Run(vector<wxInt64>& clusters)
{
    if (GdaConst::use_gda_user_seed) {
        setrandomstate((int)GdaConst::gda_user_seed);
        resetrandom();
    } else {
        setrandomstate(-1);
        resetrandom();
    }

    double cost;
    std::vector<int> clusterid;

    // NOTE input_data should be retrieved first!!
    // get input: weights (auto)
    // this function has to be called when use auto-weighting
    weight = GetWeights(columns);
    
    // compute distance matrix
    ComputeDistMatrix(dist_sel);
    RawDistMatrix dist_matrix(distmatrix);
    first_medoid = GetFirstMedoid(distmatrix);

    double pam_fasttol = m_fastswap->GetValue() ? 1 : 0;
    int init_method = combo_initmethod->GetSelection();
    bool keepmed = m_keepmed->GetValue();
    int method = combo_method->GetSelection();
    
    int seed = 0;
    if (GdaConst::use_gda_user_seed) {
        seed = (int)GdaConst::gda_user_seed;
    }
    
    if (method < 2) {
        // fastPAM & fastCLARA
        PAMInitializer* pam_init;
        if (init_method == 0) {
            pam_init = new BUILD(&dist_matrix);
        } else {
            pam_init = new LAB(&dist_matrix, seed);
        }
        if (method == 0) {
            FastPAM pam(rows, &dist_matrix, pam_init, n_cluster, 0,  pam_fasttol);
            cost = pam.run();
            clusterid = pam.getResults();
            medoid_ids = pam.getMedoids();
        } else {
            // FastCLARA
            long samples = 5;
            m_numsamples->GetValue().ToLong(&samples);
            double sample_rate = 0.025;
            m_sampling->GetValue().ToDouble(&sample_rate);

            if (sample_rate <= 1 && sample_rate*rows < 3 * n_cluster) {
                wxString err_msg = _("The sampling rate is set to a small value, please set another value to make sample size larger than 3*k.");
                wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                return false;
            }

            FastCLARA clara(rows, &dist_matrix, pam_init, n_cluster, 0,
                            pam_fasttol, (int)samples, sample_rate, !keepmed, seed);
            cost = clara.run();
            clusterid = clara.getResults();
            medoid_ids = clara.getMedoids();
        }
        delete pam_init;

    } else if (combo_method->GetSelection() == 2) {
        // FastCLARANS
        long samples = 2;
        m_numsamples->GetValue().ToLong(&samples);
        double sample_rate = 0.025;
        m_sampling->GetValue().ToDouble(&sample_rate);

        if (sample_rate <=0 || sample_rate > 1.0) {
            wxString err_msg = _("Please input a valid value between 0 and 1 for sample rate.");
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return false;
        }
        
        FastCLARANS clarans(rows, &dist_matrix, n_cluster, (int)samples, sample_rate, seed);
        cost = clarans.run();
        clusterid = clarans.getResults();
        medoid_ids = clarans.getMedoids();
    }

    for (int i=0; i<clusterid.size(); ++i) {
        clusters.push_back(wxInt64(clusterid[i]));
    }

    return true;
}
void KMedoidsDlg::doRun(int s1,int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals)
{
    // do nothing
}

vector<vector<double> > KMedoidsDlg::_getMeanCenters(
                                        const vector<vector<int> >& solutions)
{
    // Using medoids instead of mean centers
    int n_clusters = (int)solutions.size();
    vector<vector<double> > result(n_clusters);

    // update order of medoids using solutions
    boost::unordered_map<int, int> medoids_dict;
    for (int i=0; i<medoid_ids.size(); ++i) {
        medoids_dict[medoid_ids[i]] = 0;
    }
    std::vector<int> ordered_medoids;
    std::vector<int>::iterator it;
    for (int i=0; i<solutions.size(); ++i) {
        for (int j=0; j<solutions[i].size(); ++j) {
            int idx = solutions[i][j];
            if (medoids_dict.find(idx) != medoids_dict.end()) {
                ordered_medoids.push_back(idx);
            }
        }
    }
    medoid_ids = ordered_medoids;

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
        vector<double> means;
        int end = columns;
        if (IsUseCentroids()) {
            end = columns - 2;
            means.push_back(cent_xs[medoid_ids[i]]); // CENT_X
            means.push_back(cent_ys[medoid_ids[i]]); // CENT_Y
        }
        for (int c=0; c<raw_data.size(); c++) {
            double mean = raw_data[c][medoid_ids[i]];
            means.push_back(mean);
        }
        result[i] = means;
    }
    
    return result;
}

wxString KMedoidsDlg::_printConfiguration()
{
    int ncluster = 0;
    wxString str_ncluster = combo_n->GetValue();
    long value_ncluster;
    if (str_ncluster.ToLong(&value_ncluster)) {
        ncluster = (int)value_ncluster;
    }

    wxString txt;
    txt << _("Method:\t") << cluster_method << " (";
    txt << combo_method->GetString(combo_method->GetSelection()) << ")" << "\n";
    txt << _("Number of clusters:\t") << ncluster << "\n";

    //if (chk_floor && chk_floor->IsChecked()) {
    //    int idx = combo_floor->GetSelection();
    //    wxString nm = name_to_nm[combo_floor->GetString(idx)];
    //    txt << _("Minimum bound:\t") << txt_floor->GetValue() << "(" << nm << ")" << "\n";
    //}

    if (combo_method->GetSelection() < 2) {
        txt << _("Initialization method:\t") << combo_initmethod->GetString(combo_initmethod->GetSelection()) << "\n";
        //txt << _("Maximum iterations:\t") << m_iterations->GetValue() << "\n";
        if (m_fastswap->GetValue()) {
            //txt << _("\tUse additional swaps(FastPAM2).\n");
        }
        if (combo_method->GetSelection() == 1 ) {
            txt << _("Number of samples/iterations:\t") << m_numsamples->GetValue() << "\n";
            txt << _("Sample size/rate:\t") << m_sampling->GetValue() << "\n";
            if (m_keepmed->GetValue()) {
                txt << _("\tInclude previous medoids\n");
            }
        }
    } else if (combo_method->GetSelection() == 2) {
        txt << _("Number of samples/iterations:\t") << m_numsamples->GetValue() << "\n";
        txt << _("Sample size/rate:\t") << m_sampling->GetValue() << "\n";

    }

    txt << _("Transformation:\t") << combo_tranform->GetString(combo_tranform->GetSelection()) << "\n";

    txt << _("Distance function:\t") << m_distance->GetString(m_distance->GetSelection()) << "\n";

    txt << _("Medoids:\n");
    for (int i=0; i<medoid_ids.size(); ++i) {
        txt <<"\t" << medoid_ids[i] + 1 << "\n"; // row order starts from 0
    }
    return txt;
}

double KMedoidsDlg::_calcSumOfSquaresMedoid(const vector<int>& cluster_ids, int medoid_idx)
{
    if (cluster_ids.empty() || input_data==NULL || mask == NULL)
        return 0;
    
    double ssq = 0;
    
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY") {
            continue;
        }
        vector<double> vals;
        for (int j=0; j<cluster_ids.size(); j++) {
            int r = cluster_ids[j];
            if (mask[r][i] == 1)
                vals.push_back(input_data[r][i]);
        }
        double ss = GenUtils::SumOfSquaresMedoid(vals, input_data[medoid_idx][i]);
        ssq += ss;
    }
    
    return ssq;
}

double KMedoidsDlg::_calcSumOfManhattanMedoid(const vector<int>& cluster_ids, int medoid_idx)
{
    if (cluster_ids.empty() || input_data==NULL || mask == NULL)
        return 0;
    
    double ssq = 0;
    
    for (int i=0; i<columns; i++) {
        if (col_names[i] == "CENTX" || col_names[i] == "CENTY") {
            continue;
        }
        vector<double> vals;
        for (int j=0; j<cluster_ids.size(); j++) {
            int r = cluster_ids[j];
            if (mask[r][i] == 1)
                vals.push_back(input_data[r][i]);
        }
        double ss = GenUtils::SumOfManhattanMedoid(vals, input_data[medoid_idx][i]);
        ssq += ss;
    }
    
    return ssq;
}

wxString KMedoidsDlg::_additionalSummary(const vector<vector<int> >& solution,
                                         double& additional_ratio)
{
    // computing Sum of Square Differences from Medoids
    if (columns <= 0 || rows <= 0) return wxEmptyString;
    
    int dist_sel = m_distance->GetSelection();
    
    double totss = 0, totwithiness, betweenss, ratio;
    vector<double> withinss, avgs;
    
    wxString summary;
    
    if (dist_sel == 0) {
        // euclidean distance
        summary << _("(Using Euclidean distance (squared) to medoids)\n");
        // totss double totss = _getTotalSumOfSquares();
        for (int i=0; i<columns; i++) {
            if (col_names[i] == "CENTX" || col_names[i] == "CENTY")
                continue;
            vector<double> vals;
            for (int j=0; j<rows; j++) {
                if (mask[j][i] == 1)
                    vals.push_back(input_data[j][i]);
            }
            double ss = GenUtils::SumOfSquaresMedoid(vals, input_data[first_medoid][i]);
            totss += ss;
        }
        // withinss
        for (int i=0; i<solution.size(); i++ ) {
            double ss = _calcSumOfSquaresMedoid(solution[i], medoid_ids[i]);
            withinss.push_back(ss);
            avgs.push_back(ss/solution[i].size());
        }
        // tot.withiness
        totwithiness = GenUtils::Sum(withinss);
        // ratio
        ratio = totwithiness / totss;

    } else {
        // manhattan distance
        summary << _("(Using Manhattan distance to medoids)\n");
        // totss double totss = _getTotalSumOfSquares();
        for (int i=0; i<columns; i++) {
            if (col_names[i] == "CENTX" || col_names[i] == "CENTY")
                continue;
            vector<double> vals;
            for (int j=0; j<rows; j++) {
                if (mask[j][i] == 1)
                    vals.push_back(input_data[j][i]);
            }
            double ss = GenUtils::SumOfManhattanMedoid(vals, input_data[first_medoid][i]);
            totss += ss;
        }
        // withinss
        for (int i=0; i<solution.size(); i++ ) {
            double ss = _calcSumOfManhattanMedoid(solution[i], medoid_ids[i]);
            withinss.push_back(ss);
            avgs.push_back(ss/solution[i].size());
        }
        // tot.withiness
        totwithiness = GenUtils::Sum(withinss);
        // ratio
        ratio = totwithiness / totss;
    }
    summary << _("The total sum of distance:\t") << totss << "\n";
    summary << _printWithinSS(withinss, avgs, _("Within-cluster sum of distances:\n"),
                              _("Within Cluster D"), _("Averages"));
    summary << _("The total within-cluster sum of distance:\t") << totwithiness << "\n";
    summary << _("The ratio of total within to total sum of distance: ") << ratio << "\n\n";

    additional_ratio = 1 - ratio;
    return summary;
}

