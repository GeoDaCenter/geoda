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
#include <wx/dcbuffer.h>


#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"

#include "SaveToTableDlg.h"
#include "HClusterDlg.h"

BEGIN_EVENT_TABLE( HClusterDlg, wxDialog )
EVT_CLOSE( HClusterDlg::OnClose )
END_EVENT_TABLE()


HClusterDlg::HClusterDlg(wxFrame* parent_s, Project* project_s)
: frames_manager(project_s->GetFramesManager()),
wxDialog(NULL, -1, _("Hierarchical Clustering Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open HClusterDlg.");
    
	SetMinSize(wxSize(860,640));

    parent = parent_s;
    project = project_s;
    
    highlight_state = project->GetHighlightState();
                    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
    
    frames_manager->registerObserver(this);
    highlight_state->registerObserver(this);
}

HClusterDlg::~HClusterDlg()
{
    frames_manager->removeObserver(this);
    highlight_state->removeObserver(this);
}

void HClusterDlg::Highlight(int id)
{
    vector<bool>& hs = highlight_state->GetHighlight();
    
    for (int i=0; i<hs.size(); i++) hs[i] = false;
    hs[id] = true;
    
    highlight_state->SetEventType(HLStateInt::delta);
    highlight_state->notifyObservers(this);
}

bool HClusterDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void HClusterDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables"),
                                         wxDefaultPosition, wxDefaultSize);
    
    wxListBox* box = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(250,250), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    wxCheckBox* cbox = new wxCheckBox(panel, wxID_ANY, _("Use Geometric Centroids"));
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, "Input:");
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(box, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox0->Add(cbox, 0, wxLEFT | wxRIGHT, 10);
    
    if (project->IsTableOnlyProject()) {
        cbox->Disable();
    }
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,5,0);

    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Transformation:"),
                                          wxDefaultPosition, wxSize(120,-1));
    const wxString _transform[3] = {"Raw", "Demean", "Standardize"};
    combo_tranform = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 3, _transform);
    combo_tranform->SetSelection(2);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_tranform, 1, wxEXPAND);
    
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Method:"),
                                          wxDefaultPosition, wxSize(120,-1));
    wxString choices12[] = {"Single-linkage","Complete-linkage","Average-linkage","Centroid-linkage"};
    wxChoice* box12 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                       wxSize(120,-1), 4, choices12);
    box12->SetSelection(1);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box12, 1, wxEXPAND);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(120,-1));
    wxString choices13[] = {"Distance", "--Euclidean", "--City-block", "Correlation", "--Pearson","--Absolute Pearson", "Cosine", "--Signed", "--Un-signed", "Rank", "--Spearman", "--Kendal"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 12, choices13);
    box13->SetSelection(1);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box13, 1, wxEXPAND);

    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    
    // Output
    wxFlexGridSizer* gbox1 = new wxFlexGridSizer(5,2,5,0);

    wxString choices[] = {"2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20"};
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"),
                                         wxDefaultPosition, wxDefaultSize);
    wxChoice* box1 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(120,-1), 19, choices);
    box1->SetSelection(3);
    gbox1->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(box1, 1, wxEXPAND);

    
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"),
                                         wxDefaultPosition, wxDefaultSize);
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(120,-1));
    gbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox1->Add(box3, 1, wxEXPAND);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    hbox1->Add(gbox1, 1, wxEXPAND);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    saveButton = new wxButton(panel, wxID_SAVE, wxT("Save/Show Map"), wxDefaultPosition, wxDefaultSize);
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(saveButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    m_panel = new DendrogramPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(500,630));
    //m_panel->SetBackgroundColour(*wxWHITE);
    vbox1->Add(m_panel, 1, wxEXPAND|wxALL,20);
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    container->Add(vbox1,1, wxEXPAND | wxALL);
    
    
    panel->SetSizerAndFit(container);
    
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
    
    // Content
    InitVariableCombobox(box);
    combo_n = box1;
    m_textbox = box3;
    combo_var = box;
    m_use_centroids = cbox;
    //m_iterations = box11;
    m_method = box12;
    m_distance = box13;
    
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &HClusterDlg::OnOK, this);
    saveButton->Bind(wxEVT_BUTTON, &HClusterDlg::OnSave, this);

    closeButton->Bind(wxEVT_BUTTON, &HClusterDlg::OnClickClose, this);
    m_distance->Connect(wxEVT_CHOICE,
                        wxCommandEventHandler(HClusterDlg::OnDistanceChoice),
                        NULL, this);
    combo_n->Connect(wxEVT_CHOICE,
                     wxCommandEventHandler(HClusterDlg::OnClusterChoice),
                     NULL, this);
    
    saveButton->Disable();
    combo_n->Disable();
}

void HClusterDlg::OnSave(wxCommandEvent& event )
{
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
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
    
}

void HClusterDlg::OnDistanceChoice(wxCommandEvent& event)
{
    
    if (m_distance->GetSelection() == 0) {
        m_distance->SetSelection(1);
    } else if (m_distance->GetSelection() == 3) {
        m_distance->SetSelection(4);
    } else if (m_distance->GetSelection() == 6) {
        m_distance->SetSelection(7);
    } else if (m_distance->GetSelection() == 9) {
        m_distance->SetSelection(10);
    }
}

void HClusterDlg::OnClusterChoice(wxCommandEvent& event)
{
    int sel_ncluster = combo_n->GetSelection() + 2;
    
    // update dendrogram
    m_panel->UpdateCluster(sel_ncluster, clusters);
}

void HClusterDlg::UpdateClusterChoice(int n, std::vector<wxInt64>& _clusters)
{
    int sel = n - 2;
    combo_n->SetSelection(sel);
    for (int i=0; i<clusters.size(); i++){
        clusters[i] = _clusters[i];
    }
}

void HClusterDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox HClusterDlg.");

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
    
    var_box->InsertItems(items,0);
}

void HClusterDlg::update(FramesManager* o)
{
}

void HClusterDlg::update(HLStateInt* o)
{
}

void HClusterDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose HClusterDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void HClusterDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close HClusterDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}



void HClusterDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click HClusterDlg::OnOK");
    
    int ncluster = combo_n->GetSelection() + 2;
    
    bool use_centroids = m_use_centroids->GetValue();
    
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = selections.size();
    if (num_var < 2 && !use_centroids) {
        // show message box
        wxString err_msg = _("Please select at least 2 variables.");
        wxMessageDialog dlg(NULL, err_msg, "Info", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    col_ids.resize(num_var);
    var_info.resize(num_var);
    
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString nm = name_to_nm[combo_var->GetString(idx)];
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen the Regression Dialog to synchronize with Table data."), nm); wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        int tm = name_to_tm_id[combo_var->GetString(idx)];
        
        col_ids[i] = col;
        var_info[i].time = tm;
        
        // Set Primary GdaVarTools::VarInfo attributes
        var_info[i].name = nm;
        var_info[i].is_time_variant = table_int->IsColTimeVariant(idx);
        
        // var_info[i].time already set above
        table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
        var_info[i].sync_with_global_time = var_info[i].is_time_variant;
        var_info[i].fixed_scale = true;
    }
    
    // Call function to set all Secondary Attributes based on Primary Attributes
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
    
    int rows = project->GetNumRecords();
    int columns =  0;
    
    std::vector<d_array_type> data; // data[variable][time][obs]
    data.resize(col_ids.size());
    for (int i=0; i<var_info.size(); i++) {
        table_int->GetColData(col_ids[i], data[i]);
    }
    // get columns (if time variables show)
    for (int i=0; i<data.size(); i++ ){
        for (int j=0; j<data[i].size(); j++) {
            columns += 1;
        }
    }
    
    // if use centroids
    if (use_centroids) {
        columns += 2;
    }
    
    int transform = combo_tranform->GetSelection();
    char method = 's';
    char dist = 'e';
    
    int transpose = 0; // row wise
    int* clusterid = new int[rows];
    double* weight = new double[columns];
    for (int j=0; j<columns; j++){ weight[j] = 1;}
    
    
    int method_sel = m_method->GetSelection();
    char method_choices[] = {'s','m','a','c'};
    method = method_choices[method_sel];

    
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','e','b','c','c','a','u','u','x','s','s','k'};
    dist = dist_choices[dist_sel];
    
    // init input_data[rows][cols]
    double** input_data = new double*[rows];
    int** mask = new int*[rows];
    for (int i=0; i<rows; i++) {
        input_data[i] = new double[columns];
        mask[i] = new int[columns];
        for (int j=0; j<columns; j++){
            mask[i][j] = 1;
        }
    }
    
    // assign value
    int col_ii = 0;
    for (int i=0; i<data.size(); i++ ){ // col
        
        for (int j=0; j<data[i].size(); j++) { // time
            
            std::vector<double> vals;
            
            for (int k=0; k< rows;k++) { // row
                vals.push_back(data[i][j][k]);
            }
            
            if (transform == 2) {
                GenUtils::StandardizeData(vals);
            } else if (transform == 1 ) {
                GenUtils::DeviationFromMean(vals);
            }
            
            for (int k=0; k< rows;k++) { // row
                input_data[k][col_ii] = vals[k];
            }
            col_ii += 1;
        }
    }
    if (use_centroids) {
        std::vector<GdaPoint*> cents = project->GetCentroids();
        std::vector<double> cent_xs;
        std::vector<double> cent_ys;
        
        for (int i=0; i< rows; i++) {
            cent_xs.push_back(cents[i]->GetX());
            cent_ys.push_back(cents[i]->GetY());
        }
        
        if (transform == 2) {
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
    }
    
    GdaNode* htree = treecluster(rows, columns, input_data, mask, weight, transpose, dist, method, NULL);
    
    double cutoffDistance = cuttree (rows, htree, ncluster, clusterid);
    
    clusters.clear();
    clusters_undef.clear();
    
    // clean memory
    for (int i=0; i<rows; i++) {
        delete[] input_data[i];
        delete[] mask[i];
        clusters.push_back(clusterid[i]+1);
        clusters_undef.push_back(false);
    }
    delete[] input_data;
    delete[] weight;
    delete[] clusterid;
    
    /*
    // sort result
    std::vector<std::vector<int> > cluster_ids(ncluster);
    
    for (int i=0; i < clusters.size(); i++) {
        cluster_ids[ clusters[i] - 1 ].push_back(i);
    }
    
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < ncluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    */
    
    // draw dendrogram
    m_panel->Setup(htree, rows, ncluster, clusters, cutoffDistance);
    // free(htree); should be freed in m_panel since drawing still needs it's instance
    

    saveButton->Enable();
    combo_n->Enable();
}



IMPLEMENT_ABSTRACT_CLASS(DendrogramPanel, wxPanel)

BEGIN_EVENT_TABLE(DendrogramPanel, wxPanel)
EVT_MOUSE_EVENTS(DendrogramPanel::OnEvent)
EVT_IDLE(DendrogramPanel::OnIdle)
END_EVENT_TABLE()

DendrogramPanel::DendrogramPanel(wxWindow* parent, wxWindowID id, const wxPoint &pos, const wxSize &size)
: wxPanel(parent, id, pos, size)
{
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(*wxWHITE);
    Connect(wxEVT_PAINT, wxPaintEventHandler(DendrogramPanel::OnPaint));
    Connect(wxEVT_SIZE, wxSizeEventHandler(DendrogramPanel::OnSize));
    layer_bm = NULL;
    root = NULL;
    isResize = false;
    isLeftDown = false;
    isLeftMove = false;
    split_line = NULL;
    isMovingSplitLine= false;
    isLayerValid = false;
    maxDistance = 0.0;
}

DendrogramPanel::~DendrogramPanel()
{
    for (int i=0; i<end_nodes.size(); i++) {
        delete end_nodes[i];
    }
    end_nodes.clear();
    if (root) free(root); root = NULL;
    if (layer_bm) {
        delete layer_bm;
        layer_bm= NULL;
    }

    if (split_line) {
        delete split_line;
        split_line = NULL;
        
    }
}

void DendrogramPanel::OnEvent( wxMouseEvent& event )
{
    if (event.LeftDown()) {
        isLeftDown = true;
        
        startPos = event.GetPosition();
        // test SplitLine
        if (split_line) {
            if (split_line->contains(startPos)) {
                isMovingSplitLine = true;
            }
        }
        if (!isMovingSplitLine) {
        // test end_nodes
        for (int i=0;i<end_nodes.size();i++) {
            if (end_nodes[i]->contains(startPos)){
                // highlight i selected
                wxWindow* parent = GetParent()->GetParent();
                HClusterDlg* dlg = static_cast<HClusterDlg*>(parent);
                dlg->Highlight(end_nodes[i]->idx);
                break;
            }
        }
        }
    } else if (event.Dragging()) {
        if (isLeftDown) {
            isLeftMove = true;
            // moving
            if (isMovingSplitLine && split_line) {
                split_line->move(event.GetPosition(), startPos);
                int x = split_line->getX();
                Refresh();
                OnSplitLineChange(x);
            }
            startPos = event.GetPosition();
        }
    } else if (event.LeftUp()) {
        if (isLeftMove) {
            isLeftMove = false;
            // stop move
            isMovingSplitLine = false;
        } else {
            // only left click
        }
        isLeftDown = false;
    }
}

void DendrogramPanel::OnSize(  wxSizeEvent& event)
{
    isResize = true;
    event.Skip();
}

void DendrogramPanel::OnIdle(wxIdleEvent& event)
{
    if (isResize) {
        isResize = false;
        
        wxSize sz = GetClientSize();
        if (layer_bm)  {
            delete layer_bm;
            layer_bm = 0;
        }
        
        double scale_factor = GetContentScaleFactor();
        layer_bm = new wxBitmap;
        layer_bm->CreateScaled(sz.x, sz.y, 32, scale_factor);

        if (root) {
            init();
        }
    }
    event.Skip();
}

void DendrogramPanel::OnPaint( wxPaintEvent& event )
{

    wxSize sz = GetClientSize();
    if (layer_bm && isLayerValid) {
        wxMemoryDC dc;
        dc.SelectObject(*layer_bm);

        wxPaintDC paint_dc(this);
        paint_dc.Blit(0, 0, sz.x, sz.y, &dc, 0, 0);
        if (split_line) {
            split_line->draw(paint_dc);
        }
        dc.SelectObject(wxNullBitmap);
    } else {
        
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();
        dc.SetPen(*wxTRANSPARENT_PEN);
        wxBrush Brush;
        Brush.SetColour(GdaConst::canvas_background_color);
        dc.SetBrush(Brush);
        dc.DrawRectangle(wxRect(0, 0, sz.x, sz.y));
    }
    event.Skip();
}

void DendrogramPanel::Setup(GdaNode* _root, int _nelements, int _nclusters, std::vector<wxInt64>& _clusters, double _cutoff) {
    if (root && root != _root) {
        // free previous tree in memory
        free(root);
        root = NULL;
    }
    root = _root;
    nelements = _nelements;
    clusters = _clusters;
    nclusters = _nclusters;
    cutoffDistance = _cutoff;
    
    color_vec.clear();
    CatClassification::PickColorSet(color_vec, CatClassification::unique_color_scheme, nclusters);
    
    // top Node will be nelements - 2
    accessed_node.clear();
    leaves = countLeaves(-(nelements-2) - 1);
    level_node.clear();
    levels = countLevels(-(nelements-2) - 1, 0);
    
    maxDistance = root[nelements-2].distance;
    
    init();
}

void DendrogramPanel::OnSplitLineChange(int x)
{
    wxSize sz = this->GetClientSize();
    double hh = sz.y;
    double ww = sz.x;
    
    cutoffDistance = maxDistance * (ww - margin - 30 - x) / (double) (ww - margin*2 - 30);
    
    for (int i = nelements-2; i >= 0; i--)
    {
        if (cutoffDistance >=  root[i].distance) {
            nclusters = nelements - i - 1;
            break;
        }
    }

    if (nclusters > 20) nclusters = 20;
    
    int* clusterid = new int[nelements];
    cuttree (nelements, root, nclusters, clusterid);
    
    for (int i=0; i<nelements; i++) {
        clusters[i] = clusterid[i]+1;
    }
    delete[] clusterid;
    
    // sort result
    std::vector<std::vector<int> > cluster_ids(nclusters);
    
    for (int i=0; i < clusters.size(); i++) {
        cluster_ids[ clusters[i] - 1 ].push_back(i);
    }
    
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < nclusters; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
     
    wxWindow* parent = GetParent()->GetParent();
    HClusterDlg* dlg = static_cast<HClusterDlg*>(parent);
    dlg->UpdateClusterChoice(nclusters, clusters);
    
    color_vec.clear();
    CatClassification::PickColorSet(color_vec, CatClassification::unique_color_scheme, nclusters);
    
    init();
}

void DendrogramPanel::UpdateCluster(int _nclusters, std::vector<wxInt64>& _clusters)
{
    nclusters = _nclusters;
    
    int* clusterid = new int[nelements];
    cutoffDistance = cuttree (nelements, root, nclusters, clusterid);
    
    for (int i=0; i<nelements; i++) {
        clusters[i] = clusterid[i]+1;
    }
    delete[] clusterid;
    
    // sort result
    std::vector<std::vector<int> > cluster_ids(nclusters);
    
    for (int i=0; i < clusters.size(); i++) {
        cluster_ids[ clusters[i] - 1 ].push_back(i);
    }
    
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < nclusters; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    
    for (int i=0; i<nelements; i++) {
        _clusters[i] = clusters[i];
    }
    
    color_vec.clear();
    CatClassification::PickColorSet(color_vec, CatClassification::unique_color_scheme, nclusters);
    
    init();
}

void DendrogramPanel::init() {
    
    isLayerValid = false;
    
    wxSize sz = this->GetClientSize();
    double hh = sz.y;
    double ww = sz.x;
    
    for (int i=0; i<end_nodes.size(); i++) {
        delete end_nodes[i];
    }
    end_nodes.clear();
    
    
    if (layer_bm == NULL) {
        double scale_factor = GetContentScaleFactor();
        layer_bm = new wxBitmap;
        layer_bm->CreateScaled(ww, hh, 32, scale_factor);
    }
    
    margin = 10.0;
    
    heightPerLeaf = (hh - margin - margin) / (double)leaves;
    widthPerLevel = (ww - margin - margin)/ (double)levels;
    
    currentY = 10;
    
    wxMemoryDC dc;
    dc.SelectObject(*layer_bm);
    dc.Clear();
    dc.SetFont(*GdaConst::extra_small_font);
    
    int start_y = 0;
    accessed_node.clear();
    doDraw(dc, -(nelements-2) - 1, start_y);

    // draw verticle line
    if (!isMovingSplitLine) {
        int v_start =  ww - margin - 30 - (ww - 2*margin - 30) * cutoffDistance / maxDistance;
        wxPoint v_p0(v_start, 0);
        wxPoint v_p1(v_start, hh);
        if (split_line == NULL) {
            split_line = new DendroSplitLine(v_p0, v_p1);
        } else {
            split_line->update(v_p0, v_p1);
        }
    }
    
    
    dc.SelectObject(wxNullBitmap);
    
    isLayerValid = true;
    Refresh();
}

DendroColorPoint DendrogramPanel::doDraw(wxDC &dc, int node_idx, int y)
{
    
    wxSize sz = this->GetClientSize();
    double hh = sz.y;
    double ww = sz.x;
    
    if (node_idx >= 0) {
        int x = ww - margin - 30;

        wxColour clr =  color_vec[clusters[node_idx] -1];
        RectNode* end = new RectNode(node_idx, x, currentY, clr);
        end->draw(dc);
        end_nodes.push_back(end);
        
        int resultX = x;
        int resultY = currentY;
        
        currentY += heightPerLeaf;
        
        wxPoint pt(resultX, resultY);
        return DendroColorPoint(pt, clr);
    }
    
    if (accessed_node.find(node_idx) != accessed_node.end()) {
        // loop!!!
        return DendroColorPoint();
    }
    accessed_node[node_idx] = 1;
    
    DendroColorPoint cp0 = doDraw(dc, root[-node_idx -1].left, y);
    DendroColorPoint cp1 = doDraw(dc, root[-node_idx -1].right, y+heightPerLeaf);
    
    wxPoint p0 = cp0.pt;
    wxColour c0 = cp0.color;
    
    wxPoint p1 = cp1.pt;
    wxColour c1 = cp1.color;
    
    //dc.DrawRectangle(wxRect(p0.x-2, p0.y-2, 4, 4));
    //dc.DrawRectangle(wxRect(p1.x-2, p1.y-2, 4, 4));
    
    double dist = level_node[node_idx];
    int vx = ww - margin - 30 - (ww - 2*margin - 30) * dist / maxDistance ;
    
    wxPen pen0(c0);
    dc.SetPen(pen0);
    dc.DrawLine(vx, p0.y, p0.x, p0.y);
    
    wxPen pen1(c1);
    dc.SetPen(pen1);
    dc.DrawLine(vx, p1.y, p1.x, p1.y);
    
    if (c0 == c1) {
        dc.DrawLine(vx, p0.y, vx, p1.y);
    } else {
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(vx, p0.y, vx, p1.y);
    }
    dc.SetPen(*wxBLACK_PEN);
    
    wxPoint p(vx, p0.y+(p1.y - p0.y)/2);
    
    if (c0 == c1) {
        return DendroColorPoint(p, c0);
    } else {
        return DendroColorPoint(p, *wxBLACK);
    }
}

int DendrogramPanel::countLevels(int node_idx, int cur_lvl)
{
    if (node_idx >= 0) {
        return 1;
    }
    
    if (level_node.find(node_idx) != level_node.end()) {
        // loop!!!
        return 1;
    }
    
    level_node[node_idx] = root[-node_idx-1].distance;
    
    int left = countLevels(root[-node_idx-1].left, cur_lvl+1);
    int right = countLevels(root[-node_idx-1].right, cur_lvl+1);
    
    int lvl =  1 + max(left, right);
    
    return lvl;
}

int DendrogramPanel::countLeaves(int node_idx)
{
    if (node_idx >= 0) {
        return 1;
    }
    if (accessed_node.find(node_idx) != accessed_node.end()) {
        // loop!!!
        return 0;
    }
    accessed_node[node_idx] = 1;
    
    return countLeaves(root[-node_idx-1].left) + countLeaves(root[-node_idx-1].right);
}


int DendrogramPanel::countLeaves(GdaNode* node)
{
    if (node->left >= 0 && node->right >= 0) {
        return 2;
    }
    
    if (node->left >= 0 && node->right < 0) {
        return 1 + countLeaves(&root[-node->right-1]);
    }
    
    if (node->left < 0 && node->right >= 0) {
        return 1 + countLeaves(&root[-node->left-1]);
    }
    
    return countLeaves(&root[-node->left-1]) + countLeaves(&root[-node->right-1]);
}

int DendrogramPanel::countLevels(GdaNode* node)
{
    if (node->left >= 0 && node->right >= 0) {
        return 1;
    }
    
    if (node->left >= 0 && node->right < 0) {
        return 1 + countLevels(&root[-node->right-1]);
    }
    
    if (node->left < 0 && node->right >= 0) {
        return 1 + countLevels(&root[-node->left-1]);
    }
    
    return 1 + max(countLevels(&root[-node->left-1]), countLevels(&root[-node->right-1]));
}
