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

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../FramesManager.h"
#include "../DataViewer/TableInterface.h"
#include "../Project.h"
#include "../GenUtils.h"
#include "../SpatialIndAlgs.h"
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/mds.h"
#include "../Algorithms/vptree.h"
#include "../Algorithms/splittree.h"
#include "../Algorithms/tsne.h"
#include "../Algorithms/threadpool.h"
#include "../Explore/ScatterNewPlotView.h"
#include "../Explore/3DPlotView.h"
#include "../kNN/ANN/ANN.h"
#include "../GeoDa.h"
#include "../Explore/MapNewView.h"
#include "SaveToTableDlg.h"
#include "nbrMatchDlg.h"

BEGIN_EVENT_TABLE( NbrMatchDlg, wxDialog )
EVT_CLOSE( NbrMatchDlg::OnClose )
END_EVENT_TABLE()

NbrMatchDlg::NbrMatchDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Local Neighbor Match Test Settings"))
{
    wxLogMessage("Open NbrMatchDlg Dialog.");
   
    CreateControls();
}

NbrMatchDlg::~NbrMatchDlg()
{
}

void NbrMatchDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(440,620), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    AddSimpleInputCtrls(panel, vbox);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(15,2,10,0);

    // knn
    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Number of Neighbors:"));
    txt_knn = new wxTextCtrl(panel, wxID_ANY, "6",wxDefaultPosition, wxSize(70,-1));
    txt_knn->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_knn, 1, wxEXPAND);
    
    // distance function
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Variable Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);
    
    // geo distance function
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Geographic Distance Metric:"));
    wxString choices14[] = {"Euclidean Distance", "Arc Distance (mile)", "Arc Distance (km)"};
    m_geo_dist_metric = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 3, choices14);
    m_geo_dist_metric->SetSelection(0);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_geo_dist_metric, 1, wxEXPAND);
    
    // Transformation
    AddTransformation(panel, gbox);

    // seed
    wxStaticText* st27 = new wxStaticText(panel, wxID_ANY, _("Use specified seed:"));
    wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
    chk_seed = new wxCheckBox(panel, wxID_ANY, "");
    seedButton = new wxButton(panel, wxID_OK, _("Change Seed"));

    hbox17->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
    hbox17->Add(seedButton,0,wxALIGN_CENTER_VERTICAL);
    seedButton->Disable();
    gbox->Add(st27, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox17, 1, wxEXPAND);

    if (GdaConst::use_gda_user_seed) {
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);
    
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
    okButton->Bind(wxEVT_BUTTON, &NbrMatchDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &NbrMatchDlg::OnCloseClick, this);
}

void NbrMatchDlg::OnDistanceChoice(wxCommandEvent& event)
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


void NbrMatchDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close NbrMatchDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void NbrMatchDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close NbrMatchDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void NbrMatchDlg::OnSeedCheck(wxCommandEvent& event)
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

void NbrMatchDlg::OnChangeSeed(wxCommandEvent& event)
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

void NbrMatchDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox NbrMatchDlg.");
    
    wxArrayString items;
    
    std::vector<int> col_id_map;
    table_int->FillNumericColIdMap(col_id_map);
    for (size_t i=0, iend=col_id_map.size(); i<iend; i++) {
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

wxString NbrMatchDlg::_printConfiguration()
{
    return "";
}

void NbrMatchDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click NbrMatchDlg::OnOK");
   
    int transform = combo_tranform->GetSelection();
   
    if (!GetInputData(transform, 1))
        return;

    // knn
    long knn;
    wxString val = txt_knn->GetValue();
    if (!val.ToLong(&knn)) {
        wxString err_msg = _("Please input a valid numeric value for number of neighbors.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    if (knn >= project->GetNumRecords() || knn < 1) {
        wxString err_msg = _("The number of neighbors should be less than number of observations.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    char dist = 'e'; // euclidean
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];

    // create knn spatial weights GAL
    bool is_arc = m_geo_dist_metric->GetSelection() > 0;
    bool is_mile = m_geo_dist_metric->GetSelection() == 1;
    bool is_inverse = false;
    double power = 1.0;
    std::vector<double> xcoo, ycoo;
    project->GetCentroids(xcoo, ycoo);
    GwtWeight* sw = SpatialIndAlgs::knn_build(xcoo, ycoo, (int)knn, is_arc,
                                           is_mile, is_inverse, power);

    // create knn variable weights
    double eps = 0; // error bound
    if (dist == 'e') ANN_DIST_TYPE = 2; // euclidean
    else if (dist == 'b') ANN_DIST_TYPE = 1; // manhattan
    
    // since KNN search will always return the query point itself, so add 1
    // to make sure returning min_samples number of results
    //min_samples = min_samples + 1;
    GalElement* gal = new GalElement[rows];
    
    ANNkd_tree* kdTree = new ANNkd_tree(input_data, rows, columns);
    ANNidxArray nnIdx = new ANNidx[knn+1];
    ANNdistArray dists = new ANNdist[knn+1];
    for (size_t i=0; i<rows; ++i) {
        kdTree->annkSearch(input_data[i], (int)knn+1, nnIdx, dists, eps);
        //core_d[i] = sqrt(dists[min_samples-1]);
        gal[i].SetSizeNbrs(knn);
        for (size_t j=0; j<knn; j++) {
            gal[i].SetNbr(j, nnIdx[j+1], 1.0);
        }
    }
    delete[] nnIdx;
    delete[] dists;
    delete kdTree;
    
    GalWeight* gw = new GalWeight();
    gw->num_obs = rows;
    gw->wflnm = "";
    gw->id_field = "";
    gw->gal = gal;
    gw->GetNbrStats();
    
    // intersection weights
    std::vector<GeoDaWeight*> two_weights = {sw, gw};
    GalWeight* intersect_w = WeightUtils::WeightsIntersection(two_weights);
    delete sw; // clean up
    delete gw;

    // compute cnbrs (number of common neighbors), p value
    gal = intersect_w->gal;
    std::vector<wxInt64> val_cnbrs(rows);
    for (size_t i=0; i<rows; ++i) {
        val_cnbrs[i] = (wxInt64)gal[i].Size();
    }
    std::vector<double> val_p(rows);
    int k = (int)knn, v;
    for (size_t i=0; i<rows; ++i) {
        // p = C(k,v).C(nn-k,k-v) / C(N,k),
        v = val_cnbrs[i];
        val_p[i] = Gda::combinatorial(k, v) * Gda::combinatorial(rows-1-k, k-v)
            / Gda::combinatorial(rows, k);
    }
    
    // save the weights intersection
    WeightsMetaInfo wmi;
    wmi.num_obs = intersect_w->GetNumObs();
    wmi.SetMinNumNbrs(intersect_w->GetMinNumNbrs());
    wmi.SetMaxNumNbrs(intersect_w->GetMaxNumNbrs());
    wmi.SetMeanNumNbrs(intersect_w->GetMeanNumNbrs());
    wmi.SetMedianNumNbrs(intersect_w->GetMedianNumNbrs());
    wmi.SetSparsity(intersect_w->GetSparsity());
    wmi.SetDensity(intersect_w->GetDensity());
    wmi.weights_type = WeightsMetaInfo::WT_internal;
    
    WeightsManInterface* w_man_int = project->GetWManInt();
    boost::uuids::uuid id = w_man_int->RequestWeights(wmi);
    if (id.is_nil()) {
        wxString msg = _("There was a problem requesting the weights file.");
        wxMessageDialog dlg(this, msg, _("Error"), wxOK|wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    if (!((WeightsNewManager*) w_man_int)->AssociateGal(id, intersect_w)) {
        wxString msg = _("There was a problem associating the weights file.");
        wxMessageDialog dlg(this, msg, _("Error"), wxOK|wxICON_ERROR);
        dlg.ShowModal();
        delete gw;
        return;
    }

    //w_man_int->MakeDefault(id);
    
    // save the results: cnbrs (number of common neighbors), p value
    size_t new_col = 2;
    std::vector<SaveToTableEntry> new_data(new_col);
    std::vector<std::vector<double> > vals(new_col);
    std::vector<bool> undefs(rows, false);
    
    wxString field_name = "Card";
    
    new_data[0].l_val = &val_cnbrs;
    new_data[0].label = field_name;
    new_data[0].field_default = field_name;
    new_data[0].type = GdaConst::long64_type;
    new_data[0].undefined = &undefs;
    
    new_data[1].d_val = &val_p;
    new_data[1].label = "cpval";
    new_data[1].field_default = "cpval";
    new_data[1].type = GdaConst::double_type;
    new_data[1].undefined = &undefs;
    
    SaveToTableDlg dlg(project, this, new_data,
                       _("Save Results: Local Neighbor Match Test"),
                       wxDefaultPosition, wxSize(400,400));
    if (dlg.ShowModal() != wxID_OK)
        return;
    
    // show map
    std::vector<GdaVarTools::VarInfo> new_var_info;
    std::vector<int> new_col_ids;
    new_col_ids.resize(1);
    new_var_info.resize(1);
    new_col_ids[0] = table_int->GetColIdx(field_name);
    new_var_info[0].time = 0;
    // Set Primary GdaVarTools::VarInfo attributes
    new_var_info[0].name = field_name;
    new_var_info[0].is_time_variant = table_int->IsColTimeVariant(table_int->GetColIdx(field_name));
    table_int->GetMinMaxVals(new_col_ids[0], new_var_info[0].min, new_var_info[0].max);
    new_var_info[0].sync_with_global_time = new_var_info[0].is_time_variant;
    new_var_info[0].fixed_scale = true;
    
    MapFrame* nf = new MapFrame(parent, project,
                                new_var_info, new_col_ids,
                                CatClassification::unique_values,
                                MapCanvas::no_smoothing, k,
                                id,
                                wxDefaultPosition,
                                GdaConst::map_default_size);
    wxString var_name;
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    for (size_t i=0; i<selections.size(); i++) {
        wxString sel_str = combo_var->GetString(selections[i]);
        var_name << sel_str;
        if (i <selections.size()-1) var_name << "/";
    }
    wxString tmp = _("Local Neighbor Match Test Map (%d-nn: %s)");
    wxString ttl = wxString::Format(tmp, k, var_name);
    nf->SetTitle(ttl);
    nf->SetLegendLabel(0, _("No Match"));
}

////////////////////////////////////////////////////////////////////////////////
//
// LocalMatchMapCanvas/ LocalMatchMapFrame
//
////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(LocalMatchMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(LocalMatchMapCanvas, MapCanvas)
    EVT_PAINT(TemplateCanvas::OnPaint)
    EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
    EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

LocalMatchMapCanvas::LocalMatchMapCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project, vector<wxString>& _select_vars,vector<wxString>& _co_vals, vector<wxColour>& _co_clrs, vector<wxString>& _co_lbls, vector<vector<int> >& _co_ids, CatClassification::CatClassifType theme_type_s, boost::uuids::uuid weights_id_s, const wxPoint& pos, const wxSize& size)
:MapCanvas(parent, t_frame, project, vector<GdaVarTools::VarInfo>(0), vector<int>(0), CatClassification::no_theme, no_smoothing, 1, weights_id_s, pos, size),
select_vars(_select_vars), co_vals(_co_vals), co_clrs(_co_clrs), co_ids(_co_ids), co_lbls(_co_lbls)
{
    wxLogMessage("Entering LocalMatchMapCanvas::LocalMatchMapCanvas");

    cat_classif_def.cat_classif_type = theme_type_s;
   
    CreateAndUpdateCategories();
    UpdateStatusBar();
    
    wxLogMessage("Exiting LocalMatchMapCanvas::LocalMatchMapCanvas");
}

LocalMatchMapCanvas::~LocalMatchMapCanvas()
{
    wxLogMessage("In LocalMatchMapCanvas::~LocalMatchMapCanvas");
}

void LocalMatchMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
    wxLogMessage("Entering LocalMatchMapCanvas::DisplayRightClickMenu");
    // Workaround for right-click not changing window focus in OSX / wxW 3.0
    wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
    ((LocalMatchMapFrame*) template_frame)->OnActivate(ae);
    
    wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_LOCALMATCH_VIEW_MENU_OPTIONS");
    AddTimeVariantOptionsToMenu(optMenu);
    SetCheckMarks(optMenu);
    
    template_frame->UpdateContextMenuItems(optMenu);
    template_frame->PopupMenu(optMenu, pos + GetPosition());
    template_frame->UpdateOptionMenuItems();
    wxLogMessage("Exiting LocalMatchMapCanvas::DisplayRightClickMenu");
}

wxString LocalMatchMapCanvas::GetCanvasTitle()
{
    wxString ttl;
    ttl = _("Local Neighbor Match Test Map: ");
    for (int i=0; i<select_vars.size(); i++) {
        ttl << select_vars[i];
        if (i < select_vars.size() -1) {
            ttl << "/";
        }
    }
    return ttl;
}

wxString LocalMatchMapCanvas::GetVariableNames()
{
    wxString ttl;
    for (int i=0; i<select_vars.size(); i++) {
        ttl << select_vars[i];
        if (i < select_vars.size() -1) {
            ttl << "/";
        }
    }
    return ttl;
}

bool LocalMatchMapCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme, SmoothingType new_map_smoothing)
{
    wxLogMessage("In LocalMatchMapCanvas::ChangeMapType");
    return false;
}

void LocalMatchMapCanvas::SetCheckMarks(wxMenu* menu)
{
    MapCanvas::SetCheckMarks(menu);
}

void LocalMatchMapCanvas::TimeChange()
{
    wxLogMessage("Entering LocalMatchMapCanvas::TimeChange");
    wxLogMessage("Exiting LocalMatchMapCanvas::TimeChange");
}

void LocalMatchMapCanvas::CreateAndUpdateCategories()
{
    cat_data.CreateEmptyCategories(1, num_obs);
    
    int t = 0;
    int undefined_cat = -1;
    int num_cats = co_vals.size() + 1;
    
    cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
    
    map<int, bool> sig_dict;
    for (int i=0; i<num_obs; i++) {
        sig_dict[i] = false;
    }
    for (int i=0; i<co_vals.size(); i++) {
        cat_data.SetCategoryLabel(t, i+1, co_lbls[i]);
        cat_data.SetCategoryColor(t, i+1, co_clrs[i]);
        for (int j=0; j<co_ids[i].size();j++) {
            cat_data.AppendIdToCategory(t, i+1, co_ids[i][j]);
            sig_dict[ co_ids[i][j] ] = true;
        }
    }
    
    // not significant
    cat_data.SetCategoryLabel(t, 0, "Others");
    cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
    for (int i=0; i<num_obs; i++) {
        if (sig_dict[i]==false) {
            cat_data.AppendIdToCategory(t, 0, i);
        }
    }
    
    for (int cat=0; cat<num_cats; cat++) {
        cat_data.SetCategoryCount(t, cat, cat_data.GetNumObsInCategory(t, cat));
    }
    
    PopulateCanvas();
}

void LocalMatchMapCanvas::TimeSyncVariableToggle(int var_index)
{
    wxLogMessage("In LocalMatchMapCanvas::TimeSyncVariableToggle");
}

void LocalMatchMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = 0;
    if (template_frame) {
        sb = template_frame->GetStatusBar();
    }
    if (!sb)
        return;
    wxString s;
    s << _("#obs=") << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
        s << _("#selected=") << highlight_state->GetTotalHighlighted()<< "  ";
    }
    if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << _("#hover obs ") << hover_obs[0]+1;
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << _("obs ") << hover_obs[1]+1;
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << _("obs ") << hover_obs[2]+1;
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    }
    sb->SetStatusText(s);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(LocalMatchMapFrame, MapFrame)
    BEGIN_EVENT_TABLE(LocalMatchMapFrame, MapFrame)
    EVT_ACTIVATE(LocalMatchMapFrame::OnActivate)
END_EVENT_TABLE()

LocalMatchMapFrame::LocalMatchMapFrame(wxFrame *parent, Project* project, vector<wxString>& select_vars, vector<wxString>& co_vals, vector<wxColour>& co_clrs, vector<wxString>& co_lbls, vector<vector<int> >& co_ids, boost::uuids::uuid weights_id_s, const wxString title, const wxPoint& pos, const wxSize& size, const long style)
: MapFrame(parent, project, pos, size, style)
{
    wxLogMessage("Entering LocalMatchMapFrame::LocalMatchMapFrame");
    
    int width, height;
    GetClientSize(&width, &height);
    
    wxSplitterWindow* splitter_win = new wxSplitterWindow(this,wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
    splitter_win->SetMinimumPaneSize(10);
    
    CatClassification::CatClassifType theme_type_s = CatClassification::colocation;
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new LocalMatchMapCanvas(rpanel, this, project, select_vars, co_vals, co_clrs, co_lbls, co_ids, theme_type_s, weights_id_s);
    template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
    wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new MapNewLegend(lpanel, template_canvas,
                                       wxPoint(0,0), wxSize(0,0));
    wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);
    
    splitter_win->SplitVertically(lpanel, rpanel, GdaConst::map_default_legend_width);
    
    wxPanel* toolbar_panel = new wxPanel(this,wxID_ANY, wxDefaultPosition);
    wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    toolbar = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
    toolbar_sizer->Add(toolbar, 0, wxEXPAND|wxALL);
    toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL);
    sizer->Add(splitter_win, 1, wxEXPAND|wxALL);
    SetSizer(sizer);
    //splitter_win->SetSize(wxSize(width,height));
    
    SetAutoLayout(true);
    SetTitle(title);
    DisplayStatusBar(true);
    
    Show(true);
    wxLogMessage("Exiting LocalMatchMapFrame::LocalMatchMapFrame");
}

LocalMatchMapFrame::~LocalMatchMapFrame()
{
    wxLogMessage("In LocalMatchMapFrame::~LocalMatchMapFrame");
}

void LocalMatchMapFrame::OnActivate(wxActivateEvent& event)
{
    wxLogMessage("In LocalMatchMapFrame::OnActivate");
    if (event.GetActive()) {
        RegisterAsActive("LocalMatchMapFrame", GetTitle());
    }
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void LocalMatchMapFrame::MapMenus()
{
    wxLogMessage("In LocalMatchMapFrame::MapMenus");
    wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
    // Map Options Menus
    wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_LOCALMATCH_VIEW_MENU_OPTIONS");
    ((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
    ((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
    GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);
    UpdateOptionMenuItems();
}

void LocalMatchMapFrame::UpdateOptionMenuItems()
{
    TemplateFrame::UpdateOptionMenuItems(); // set common items first
    wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
    int menu = mb->FindMenu(_("Options"));
    if (menu == wxNOT_FOUND) {
        wxLogMessage("LocalMatchMapFrame::UpdateOptionMenuItems: "
                "Options menu not found");
    } else {
        ((LocalMatchMapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
    }
}

void LocalMatchMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
    TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LocalMatchMapFrame::OnSave(wxCommandEvent& event)
{
    int t = this->GetCurrentCanvasTimeStep();
    LocalMatchMapCanvas* lc = (LocalMatchMapCanvas*)template_canvas;
    int num_obs = lc->num_obs;
    
    std::vector<SaveToTableEntry> data(1);
    std::vector<bool> undefs(num_obs, true);
    std::vector<wxInt64> dt(num_obs);
    
    for (int i=0; i<lc->co_vals.size(); i++) {
        wxString tmp = lc->co_vals[i];
        long l_tmp;
        if (tmp.ToLong(&l_tmp)) {
            for (int j=0; j<lc->co_ids[i].size(); j++) {
                undefs[ lc->co_ids[i][j] ] = false;
                dt[ lc->co_ids[i][j] ] = l_tmp;
            }
        }
    }
    
    data[0].l_val = &dt;
    data[0].label = "Coloc Indices";
    data[0].field_default = "CO_I";
    data[0].type = GdaConst::long64_type;
    data[0].undefined = &undefs;
    
    SaveToTableDlg dlg(project, this, data,
                       "Save Results: Co-locations",
                       wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
}

void LocalMatchMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
                            _("Conditional Co-location Map Variables"),
                            _("Horizontal Cells"), _("Vertical Cells"),
                            "", "", false, false, false, // default
                            true, true, false, false);
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    
    LocalMatchMapCanvas* lc = (LocalMatchMapCanvas*) template_canvas;
    wxString title = lc->GetCanvasTitle();
    //ConditionalClusterMapFrame* subframe = new ConditionalClusterMapFrame(this, project, dlg.var_info, dlg.col_ids, lisa_coord, title, wxDefaultPosition, GdaConst::cond_view_default_size);
}

void LocalMatchMapFrame::closeObserver(LisaCoordinator* o)
{
    wxLogMessage("In LocalMatchMapFrame::closeObserver(LisaCoordinator*)");
    Close(true);
}
