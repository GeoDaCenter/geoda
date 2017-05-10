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
#include "../cluster.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"

#include "SaveToTableDlg.h"
#include "HClusterDlg.h"

BEGIN_EVENT_TABLE( HClusterDlg, wxDialog )
EVT_CLOSE( HClusterDlg::OnClose )
END_EVENT_TABLE()


HClusterDlg::HClusterDlg(wxFrame* parent_s, Project* project_s)
: frames_manager(project_s->GetFramesManager()),
wxDialog(NULL, -1, _("Hierarchical Clustering Settings"), wxDefaultPosition, wxSize(860, 640), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open HClusterDlg.");
    
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

HClusterDlg::~HClusterDlg()
{
    frames_manager->removeObserver(this);
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
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,2,5,0);

    wxString choices[] = {"2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20"};
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Number of Clusters:"),
                                         wxDefaultPosition, wxSize(120,-1));
    wxChoice* box1 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                      wxSize(120,-1), 19, choices, wxCB_READONLY);
    gbox->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box1, 1, wxEXPAND);
    
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Data Transformation:"),
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
                                       wxSize(120,-1), 4, choices12, wxCB_READONLY);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box12, 1, wxEXPAND);
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"),
                                          wxDefaultPosition, wxSize(120,-1));
    wxString choices13[] = {"Distance", "--Euclidean", "--City-block", "Correlation", "--Pearson","--Absolute Pearson", "Cosine", "--Signed", "--Un-signed", "Rank", "--Spearman", "--Kendal"};
    wxChoice* box13 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 12, choices13);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box13, 1, wxEXPAND);

    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"),
                                         wxDefaultPosition, wxDefaultSize);
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(120,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box3, 1, wxEXPAND);
    
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    //wxButton *saveButton = new wxButton(panel, wxID_SAVE, wxT("Save"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    //hbox2->Add(saveButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 1, wxALIGN_CENTER | wxALL, 10);
    
    
    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    m_panel = new DendrogramPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(320,830));
    //m_panel->SetBackgroundColour(*wxWHITE);
    vbox1->Add(m_panel, 1, wxEXPAND|wxALL,20);
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    container->Add(vbox1,1, wxEXPAND | wxALL);
    
    panel->SetSizer(container);
    
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
    closeButton->Bind(wxEVT_BUTTON, &HClusterDlg::OnClickClose, this);
    m_distance->Connect(wxEVT_CHOICE,
                        wxCommandEventHandler(HClusterDlg::OnDistanceChoice),
                        NULL, this);
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
    
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
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
    
    cuttree (rows, htree, ncluster, clusterid);
    
    vector<wxInt64> clusters;
    vector<bool> clusters_undef;
    
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
    
    // draw dendrogram
    int margin = 10;
    int leaves = 0;
    int levels = 0;
    //double height_per_leaf = (m_panel->GetHeight() - margin - margin) / (double) leaves;
    //double width_per_level = (m_panel->GetWidth() - margin - margin) / (double) levels;
    
    double current_y = 0;
    
    m_panel->Setup(htree, rows);
    
    free(htree);
    
    // save to table

    int time=0;
    int col = table_int->FindColId(field_name);
    if ( col == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_long_len;
        int m_decimals_val = 0;
        
        col = table_int->InsertCol(GdaConst::long64_type, field_name, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    }
    
    if (col > 0) {
        table_int->SetColData(col, time, clusters);
        table_int->SetColUndefined(col, time, clusters_undef);
    }
    
    // show a cluster map
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



DendrogramPanel::DendrogramPanel(wxWindow* parent, wxWindowID id, const wxPoint &pos, const wxSize &size)
: wxPanel(parent, id, pos, size)
{
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(*wxWHITE);
    Connect(wxEVT_PAINT, wxPaintEventHandler(DendrogramPanel::OnPaint));
    Connect(wxEVT_SIZE, wxSizeEventHandler(DendrogramPanel::OnSize));
    Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(DendrogramPanel::OnMouse));

    layer_bm = NULL;
}


void DendrogramPanel::OnMouse( wxMouseEvent& event )
{
    
}

void DendrogramPanel::OnSize(  wxSizeEvent& event)
{
    Refresh();
}



void DendrogramPanel::OnPaint( wxPaintEvent& event )
{
    /*
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    wxSize sz = this->GetClientSize();
    wxBrush Brush;
    Brush.SetColour(GdaConst::canvas_background_color);
    dc.SetBrush(Brush);
    dc.DrawRectangle(wxRect(0, 0, sz.x, sz.y));
    */
    if (layer_bm) {
        wxSize sz = GetClientSize();
        wxMemoryDC dc(*layer_bm);
        
        wxPaintDC paint_dc(this);
        paint_dc.Blit(0, 0, sz.x, sz.y, &dc, 0, 0);
    }
    event.Skip();
}

void DendrogramPanel::Setup(GdaNode* _root, int _nelements) {
    root = _root;
    nelements = _nelements;
    
    // top Node will be nelements - 2
    leaves = countLeaves(-(nelements-2) - 1);
    levels = countLevels(-(nelements-2) - 1);
    
    wxSize sz = this->GetClientSize();
    double hh = sz.y;
    double ww = sz.x;
    
    layer_bm = new wxBitmap(ww, hh, 32);
    
    margin = 10.0;
    
    heightPerLeaf = (hh - margin - margin) / (double)leaves;
    widthPerLevel = (ww - margin - margin)/ (double)levels;
    
    currentY = 0;
    
    wxMemoryDC dc(*layer_bm);
    dc.Clear();
    
    doDraw(dc, -(nelements-2) - 1, 0);

    Refresh();
}

wxPoint DendrogramPanel::doDraw(wxMemoryDC &dc, int node_idx, int y)
{
    wxSize sz = this->GetClientSize();
    double hh = sz.y;
    double ww = sz.x;
    
    if (node_idx >= 0) {
        int x = ww - widthPerLevel - 2 * margin;
        wxString text;
        text << node_idx;
        
        dc.DrawText(text, x + 8, currentY + 8);
        
        int resultX = x;
        int resultY = currentY;
        
        currentY += heightPerLeaf;
        
        return wxPoint(resultX, resultY);
    }
    
    wxPoint p0 = doDraw(dc, root[-node_idx -1].left, y);
    wxPoint p1 = doDraw(dc, root[-node_idx -1].right, y+heightPerLeaf);
    
    dc.DrawRectangle(wxRect(p0.x-2, p0.y-2, 4, 4));
    dc.DrawRectangle(wxRect(p1.x-2, p1.y-2, 4, 4));
    
    int dx = widthPerLevel;
    int vx = min(p0.x-dx, p1.x-dx);
    
    dc.DrawLine(vx, p0.y, p0.x, p0.y);
    dc.DrawLine(vx, p1.y, p1.x, p1.y);
    dc.DrawLine(vx, p0.y, vx, p1.y);
    wxPoint p(vx, p0.y+(p1.y - p0.y)/2);
    return p;
    
}

int DendrogramPanel::countLevels(int node_idx)
{
    if (node_idx >= 0) {
        return 1;
    }
    
    return 1 + max(countLevels(root[-node_idx-1].left), countLevels(root[-node_idx-1].right));
}

int DendrogramPanel::countLeaves(int node_idx)
{
    if (node_idx >= 0) {
        return 1;
    }
    
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
