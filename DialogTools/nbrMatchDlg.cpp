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
#include <limits>
#include <cstddef>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/dialog.h>
#include <wx/xrc/xmlres.h>
#include <wx/tokenzr.h>

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
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
#include "PermutationCounterDlg.h"
#include "RandomizationDlg.h"
#include "../GeoDa.h"
#include "../Explore/MapNewView.h"
#include "../GenColor.h"
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
                                                  wxSize(440,600), wxHSCROLL|wxVSCROLL );
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
    wxStaticText* st27 = new wxStaticText(panel, wxID_ANY, _("Use Specified Seed:"));
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
    
    chk_seed->Hide();
    st27->Hide();
    seedButton->Hide();
    
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
    std::vector<GeoDaWeight*> two_weights;
    two_weights.push_back(sw);
    two_weights.push_back(gw);
    GalWeight* intersect_w = WeightUtils::WeightsIntersection(two_weights);


    // compute cnbrs (number of common neighbors), p value
    gal = intersect_w->gal;
    std::vector<wxInt64> val_cnbrs(rows);
    for (size_t i=0; i<rows; ++i) {
        val_cnbrs[i] = (wxInt64)gal[i].Size();
    }

    int k = (int)knn;
    std::vector<double> pval_dict(knn,  -1);
    for (int v=1; v<knn; ++v) {
        // p = C(k,v).C(N-k,k-v) / C(N,k),
        pval_dict[v] = Gda::combinatorial(k, v) * Gda::combinatorial(rows-k-1, k-v);
        pval_dict[v] /= Gda::combinatorial(rows-1, k);
    }
    std::vector<double> val_p(rows);
    for (size_t i=0; i<rows; ++i) {
        val_p[i] = pval_dict[val_cnbrs[i]];
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
        delete intersect_w;
        return;
    }

    //w_man_int->MakeDefault(id);
    
    // save the results: cnbrs (number of common neighbors), p value
    size_t new_col = 2;
    std::vector<SaveToTableEntry> new_data(new_col);
    std::vector<std::vector<double> > vals(new_col);
    std::vector<bool> undefs_cnbrs(rows, false), undefs_pval(rows, false);
    
    for (int i=0; i<rows; ++i) {
        if (val_p[i] == -1.0) {
            undefs_pval[i] = true;
        }
    }
    wxString field_name = "card";
    
    new_data[0].l_val = &val_cnbrs;
    new_data[0].label = "Cardinality";
    new_data[0].field_default = field_name;
    new_data[0].type = GdaConst::long64_type;
    new_data[0].undefined = &undefs_cnbrs;
    
    new_data[1].d_val = &val_p;
    new_data[1].label = "Probability";
    new_data[1].field_default = "cpval";
    new_data[1].type = GdaConst::double_type;
    new_data[1].undefined = &undefs_pval;
    
    SaveToTableDlg dlg(project, this, new_data,
                       _("Save Results: Local Neighbor Match Test"),
                       wxDefaultPosition, wxSize(400,400));
    if (dlg.ShowModal() != wxID_OK)
        return;

    // show map
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
    
    std::vector<std::vector<int> > groups(k);
    for (int i=0; i<rows; ++i) {
        groups[val_cnbrs[i]].push_back(i);
    }
    LocalMatchMapFrame* nf = new LocalMatchMapFrame(parent, project,
                                groups, val_p, id, ttl,
                                wxDefaultPosition, GdaConst::map_default_size);

    /*
    // run perm sig, ignore for now since p = C(k,v).C(N-k,k-v) / C(N,k) is used
    LocalMatchCoordinator* lm_coord = new LocalMatchCoordinator(sw, gw, val_cnbrs, 999,
                                                                GdaConst::gda_user_seed,
                                                                GdaConst::use_gda_user_seed);
    LocalMatchSignificanceFrame* snf = new LocalMatchSignificanceFrame(parent, project,
                                                                       lm_coord);
     */
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

LocalMatchMapCanvas::LocalMatchMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                         Project* project,
                                         const std::vector<std::vector<int> >& groups,
                                         const std::vector<double>& pval,
                                         boost::uuids::uuid weights_id,
                                         const wxPoint& pos, const wxSize& size)
:MapCanvas(parent, t_frame, project, vector<GdaVarTools::VarInfo>(0), vector<int>(0), CatClassification::no_theme, no_smoothing, 1, weights_id, pos, size),
groups(groups), pval(pval)
{
    wxLogMessage("Entering LocalMatchMapCanvas::LocalMatchMapCanvas");
   
    display_weights_graph = true;
    graph_color = *wxRED;
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
    return "Cardinality";
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
    int num_categories = (int)groups.size();
    if (num_categories == 0) return;
    
    int t = 0;
    //cat_data.CreateEmptyCategories(1, num_obs);
    cat_data.CreateCategoriesAtCanvasTm(num_categories, t);
    cat_data.ClearAllCategoryIds();
    // update color scheme
    wxColour BuGn[][9] = {{wxColour(229,245,249)}, {wxColour(229,245,249),wxColour(153,216,201)},{wxColour(229,245,249),wxColour(153,216,201),wxColour(44,162,95)}, /*4*/{wxColour(237,248,251),wxColour(178,226,226),wxColour(102,194,164),wxColour(35,139,69)},/*5*/{wxColour(237,248,251),wxColour(178,226,226),wxColour(102,194,164),wxColour(44,162,95),wxColour(0,109,44)},/*6*/{wxColour(237,248,251),wxColour(204,236,230),wxColour(153,216,201),wxColour(102,194,164),wxColour(44,162,95),wxColour(0,109,44)},/*7*/{wxColour(237,248,251),wxColour(204,236,230),wxColour(153,216,201),wxColour(102,194,164),wxColour(65,174,118),wxColour(35,139,69), wxColour(0,88,36)},/*8*/{wxColour(247,252,253),wxColour(229,245,249),wxColour(204,236,230),wxColour(153,216,201),wxColour(102,194,164),wxColour(65,174,118),wxColour(35,139,69),wxColour(0,88,36)},/*9*/{wxColour(247,252,253),wxColour(229,245,249),wxColour(204,236,230),wxColour(153,216,201),wxColour(102,194,164),wxColour(65,174,118),wxColour(35,139,69),wxColour(0,109,44),wxColour(0,68,27)}};

    if (num_categories <= 9) {
        for (int i=0; i<num_categories; ++i) {
            cat_data.SetCategoryLabel(t, i, wxString::Format("%d", i));
            cat_data.SetCategoryColor(t, i, BuGn[num_categories-1][i]);
            for (size_t j=0; j<groups[i].size(); ++j) {
                cat_data.AppendIdToCategory(t, i, groups[i][j]);
            }
        }
    } else {
        ColorSpace::Rgb a(247,252,253);
        ColorSpace::Rgb b(0,68,27);
        std::vector<ColorSpace::Rgb> color = ColorSpace::ColorSpectrumHSV(a, b, num_categories);
        for (int i=0; i<num_categories; ++i) {
            cat_data.SetCategoryLabel(t, i, wxString::Format("%d", i));
            cat_data.SetCategoryColor(t, i, wxColour(color[i].r, color[i].g, color[i].b));
            for (size_t j=0; j<groups[i].size(); ++j) {
                cat_data.AppendIdToCategory(t, i, groups[i][j]);
            }
        }
    }
    
    for (int cat=0; cat<num_categories; cat++) {
        cat_data.SetCategoryCount(t, cat, cat_data.GetNumObsInCategory(t, cat));
    }
    cat_data.SetCategoryLabel(t, 0, "No Match");
    
    full_map_redraw_needed = true;
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
            s << " p=" << wxString::Format("%f", pval[hover_obs[0]]);
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
///////////////////////////////////////////////////////////////////////////////
//
// NbrMatchSaveWeightsDialog
//
///////////////////////////////////////////////////////////////////////////////
NbrMatchSaveWeightsDialog::NbrMatchSaveWeightsDialog(TableInterface* table_int, const wxString & title)
       : wxDialog(NULL, -1, title, wxDefaultPosition, wxSize(250, 230)),
table_int(table_int)
{
    wxPanel *panel = new wxPanel(this, -1);

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Select ID Variable"));
    m_id_field = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 0, NULL);
    table_int->FillColIdMap(col_id_map);
    for (size_t i=0, iend=col_id_map.size(); i<iend; i++) {
        int col = col_id_map[i];
        wxString name = table_int->GetColName(col);
        if (table_int->GetColType(col) == GdaConst::long64_type ||
            table_int->GetColType(col) == GdaConst::string_type) {
            if (!table_int->IsColTimeVariant(col)) {
                m_id_field->Append(table_int->GetColName(col));
            }
        }
    }
    m_id_field->SetSelection(-1);
    m_id_field->Connect(wxEVT_CHOICE,
                        wxCommandEventHandler(NbrMatchSaveWeightsDialog::OnIdVariableSelected),
                        NULL, this);
    
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st14, 1, wxRIGHT, 5);
    hbox1->Add(m_id_field, 1, wxLEFT, 5);
    
    wxButton *okButton = new wxButton(this, wxID_OK, _("Ok"),
        wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CANCEL, _("Close"),
        wxDefaultPosition, wxSize(70, 30));

    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(okButton, 1);
    hbox->Add(closeButton, 1, wxLEFT, 5);

    vbox->Add(hbox1, 1, wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
     
    panel->SetSizer(container);
    
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);
     
     
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panelSizer, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    //SetSizer(vbox);
    Centre();
}

void NbrMatchSaveWeightsDialog::OnIdVariableSelected( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnIdVariableSelected");
    // we must have key id variable
    bool isValid = m_id_field->GetSelection() != wxNOT_FOUND;
    if (!isValid)
        return;
    
    wxString id = m_id_field->GetString(m_id_field->GetSelection());
    if (!CheckID(id)) {
        m_id_field->SetSelection(-1);
        return;
    }
}

wxString NbrMatchSaveWeightsDialog::GetSelectID()
{
    int sel_idx = m_id_field->GetSelection();
    if (sel_idx < 0) {
        wxMessageBox(_("Please select a ID variable."));
        return wxEmptyString;
    }
    return m_id_field->GetString(sel_idx);
}

bool NbrMatchSaveWeightsDialog::CheckID(const wxString& id)
{
    std::vector<wxString> str_id_vec(table_int->GetNumberRows());
    int col = table_int->FindColId(id);
    if (table_int->GetColType(col) == GdaConst::long64_type){
        table_int->GetColData(col, 0, str_id_vec);
        
    } else if (table_int->GetColType(col) == GdaConst::string_type) {
        // to handle string field with only numbers
        // Note: can't handle real string (a-zA-Z) here since it's hard
        // to control in weights file (.gal/.gwt/..)
        table_int->GetColData(col, 0, str_id_vec);
        
        wxRegEx regex;
        regex.Compile("^[0-9a-zA-Z_]+$");
        
        for (size_t i=0, iend=str_id_vec.size(); i<iend; i++) {
            wxString item  = str_id_vec[i];
            if (regex.Matches(item)) {
                str_id_vec[i] = item;
            } else {
                wxString msg = id;
                msg += _(" should contains only numbers/letters as IDs.  Please choose a different ID Variable.");
                wxMessageBox(msg);
                return false;
            }
        }
    }
    
    std::set<wxString> dup_ids;
    std::set<wxString> id_set;
    std::map<wxString, std::vector<int> > dup_dict; // value:[]
    
    for (size_t i=0, iend=str_id_vec.size(); i<iend; i++) {
        wxString str_id = str_id_vec[i];
        if (id_set.find(str_id) == id_set.end()) {
            id_set.insert(str_id);
            std::vector<int> ids;
            dup_dict[str_id] = ids;
        }
        dup_dict[str_id].push_back((int)i);
    }
    if (id_set.size() != table_int->GetNumberRows()) {
        wxString msg = id + _(" has duplicate values. Please choose a different ID Variable.\n\nDetails:");
        wxString details = "value, row\n";
        
        std::map<wxString, std::vector<int> >::iterator it;
        for (it=dup_dict.begin(); it!=dup_dict.end(); it++) {
            wxString val = it->first;
            std::vector<int>& ids = it->second;
            if (ids.size() > 1) {
                for (int i=0; i<ids.size(); i++) {
                    details << val << ", " << ids[i]+1 << "\n";
                }
            }
        }
        
        ScrolledDetailMsgDialog *dlg = new ScrolledDetailMsgDialog(_("Warning"),
                                                                   msg, details);
        dlg->Show(true);
        
        return false;
    }
    return true;
}
///////////////////////////////////////////////////////////////////////////////
//
// LocalMatchMapFrame
//
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(LocalMatchMapFrame, MapFrame)
    BEGIN_EVENT_TABLE(LocalMatchMapFrame, MapFrame)
    EVT_ACTIVATE(LocalMatchMapFrame::OnActivate)
END_EVENT_TABLE()

LocalMatchMapFrame::LocalMatchMapFrame(wxFrame *parent, Project* project,
                                       const std::vector<std::vector<int> >& groups,
                                       const std::vector<double>& pval,
                                       boost::uuids::uuid weights_id, const wxString& title, const wxPoint& pos, const wxSize& size)
:MapFrame(parent, project, pos, size), weights_id(weights_id)
{
    wxLogMessage("Entering LocalMatchMapFrame::LocalMatchMapFrame");
    
    SetTitle(title);
    
    int width, height;
    GetClientSize(&width, &height);
    
    wxSplitterWindow* splitter_win = new wxSplitterWindow(this,wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
    splitter_win->SetMinimumPaneSize(10);
        
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new LocalMatchMapCanvas(rpanel, this, project, groups, pval, weights_id);
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
    //((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
    ((LocalMatchMapCanvas*) template_canvas)->SetCheckMarks(optMenu);
    GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);
    UpdateOptionMenuItems();
    
    wxMenuItem* save_menu = optMenu->FindItem(XRCID("ID_SAVE_LOCALMATCH_WEIGHTS"));
    Connect(save_menu->GetId(), wxEVT_MENU,  wxCommandEventHandler(LocalMatchMapFrame::OnSave));
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

/** Implementation of WeightsManStateObserver interface */
void LocalMatchMapFrame::update(WeightsManState* o)
{
    // do nothing since it has its own weights structure
}

void LocalMatchMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
    TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LocalMatchMapFrame::OnSave(wxCommandEvent& event)
{
    NbrMatchSaveWeightsDialog dlg(project->GetTableInt(), _("Save Local Match Weights"));
    if(dlg.ShowModal() == wxID_OK) {
        wxString id_var = dlg.GetSelectID();
        
        TableInterface* table_int = project->GetTableInt();
        int m_num_obs = table_int->GetNumberRows();
        
        if (table_int && !id_var.IsEmpty()) {
            int col = project->GetTableInt()->FindColId(id_var);
            if (col < 0) return;
            
            bool flag = false;
            wxString wildcard = _("GAL files (*.gal)|*.gal");
            wxString defaultFile(project->GetProjectTitle());
            defaultFile += "_match.gal";
             
            wxString working_dir = project->GetWorkingDir().GetPath();
            wxFileDialog dlg(this, _("Choose an output weights file name."),
                             working_dir, defaultFile, wildcard,
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            
            wxString ofn;
            if (dlg.ShowModal() != wxID_OK) return;
            ofn = dlg.GetPath();
            
            wxString layer_name = project->GetProjectTitle();
            
            WeightsManInterface* w_man_int = project->GetWManInt();
            GalWeight* w = w_man_int->GetGal(weights_id);
            
            if (table_int->GetColType(col) == GdaConst::long64_type){
                std::vector<wxInt64> id_vec(m_num_obs);
                table_int->GetColData(col, 0, id_vec);
                flag = Gda::SaveGal(w->gal, layer_name, ofn, id_var, id_vec);
                
            } else if (table_int->GetColType(col) == GdaConst::string_type) {
                std::vector<wxString> id_vec(m_num_obs);
                table_int->GetColData(col, 0, id_vec);
                flag = Gda::SaveGal(w->gal, layer_name, ofn, id_var, id_vec);
            }
            if (!flag) {
                wxString msg = _("Failed to create the weights file.");
                wxMessageDialog dlg(NULL, msg, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
            } else {
                wxString msg = _("Weights file created successfully.");
                wxMessageDialog dlg(NULL, msg, _("Success"), wxOK | wxICON_INFORMATION);
                dlg.ShowModal();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// LocalMatchCoordinator
//
///////////////////////////////////////////////////////////////////////////////
LocalMatchCoordinator::LocalMatchCoordinator(GwtWeight* spatial_w, GalWeight* variable_w, const std::vector<wxInt64>& cadinality, int permutations, uint64_t last_seed_used, bool reuse_last_seed)
: spatial_w(spatial_w), variable_w(variable_w), cadinality(cadinality),
last_seed_used(last_seed_used), reuse_last_seed(reuse_last_seed), permutations(permutations), num_obs(spatial_w->num_obs)
{
    SetSignificanceFilter(1);
    user_sig_cutoff = 0;
    
    sigVal.resize(num_obs);
    sigCat.resize(num_obs);
    run();
}

LocalMatchCoordinator::~LocalMatchCoordinator()
{

}

void LocalMatchCoordinator::job(size_t nbr_sz, size_t idx, uint64_t seed_start)
{
    GeoDaSet workPermutation(num_obs);
    int max_rand = num_obs-1;
    
    int rand=0, newRandom, countLarger=0;
    double rng_val;

    for (size_t i=0; i<permutations; ++i) {
        // for each observation, get random neighbors
        while (rand < nbr_sz) {
            // computing 'perfect' permutation of given size
            rng_val = Gda::ThomasWangHashDouble(seed_start++) * max_rand;
            // round is needed to fix issue
            // https://github.com/GeoDaCenter/geoda/issues/488
            newRandom = (int)(rng_val<0.0?ceil(rng_val - 0.5):floor(rng_val + 0.5));
                
            if (newRandom != idx && !workPermutation.Belongs(newRandom) && spatial_w->gwt[newRandom].Size()>0) // neighborless is out of shuffle
            {
                workPermutation.Push(newRandom);
                rand++;
            }
        }
        // compute common local match
        int perm_nbr, match = 0;
        for (int cp=0; cp<nbr_sz; cp++) {
            perm_nbr = workPermutation.Pop();
            if (variable_w->CheckNeighbor(idx, perm_nbr)) {
                match += 1;
            }
        }
        if (match >= cadinality[idx]) {
            countLarger += 1;
        }
    }
    
    // pick the smallest
    if (permutations-countLarger <= countLarger) {
        countLarger = permutations-countLarger;
    }
    // compute pseudo-p-value
    sigVal[idx] = (countLarger + 1.0)/(permutations+1);
    
    if (sigVal[idx] <= 0.0001) sigCat[idx] = 4;
    else if (sigVal[idx] <= 0.001) sigCat[idx] = 3;
    else if (sigVal[idx] <= 0.01) sigCat[idx] = 2;
    else if (sigVal[idx] <= 0.05) sigCat[idx]= 1;
    else sigCat[idx]= 0;
}

void LocalMatchCoordinator::run()
{
    long nbr_sz;
    for(size_t i=0; i< num_obs; ++i) {
        // for each observation, get random neighbors
        nbr_sz = spatial_w->gwt[i].Size();
        uint64_t seed_start = reuse_last_seed ? last_seed_used + i : i;
        job(nbr_sz, i, seed_start);
    }
}

void LocalMatchCoordinator::SetSignificanceFilter(int filter_id)
{
    wxLogMessage("Entering LocalMatchCoordinator::SetSignificanceFilter()");
    if (filter_id == -1) {
        // user input cutoff
        significance_filter = filter_id;
        return;
    }
    // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
    if (filter_id < 1 || filter_id > 4) return;
    significance_filter = filter_id;
    if (filter_id == 1) significance_cutoff = 0.05;
    if (filter_id == 2) significance_cutoff = 0.01;
    if (filter_id == 3) significance_cutoff = 0.001;
    if (filter_id == 4) significance_cutoff = 0.0001;
    wxLogMessage("Exiting AbstractCoordinator::SetSignificanceFilter()");
}

std::vector<wxString> LocalMatchCoordinator::GetDefaultCategories()
{
    std::vector<wxString> cats;
    cats.push_back("p = 0.05");
    cats.push_back("p = 0.01");
    cats.push_back("p = 0.001");
    cats.push_back("p = 0.0001");
    return cats;
}

std::vector<double> LocalMatchCoordinator::GetDefaultCutoffs()
{
    std::vector<double> cutoffs;
    cutoffs.push_back(0.05);
    cutoffs.push_back(0.01);
    cutoffs.push_back(0.001);
    cutoffs.push_back(0.0001);
    return cutoffs;
}

///////////////////////////////////////////////////////////////////////////////
//
// LocalMatchSignificanceCanvas
//
///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(LocalMatchSignificanceCanvas, MapCanvas)
BEGIN_EVENT_TABLE(LocalMatchSignificanceCanvas, MapCanvas)
    EVT_PAINT(TemplateCanvas::OnPaint)
    EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
    EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()


LocalMatchSignificanceCanvas::
LocalMatchSignificanceCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project, LocalMatchCoordinator* gs_coordinator, const wxPoint& pos, const wxSize& size)
: MapCanvas(parent, t_frame, project, std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0), CatClassification::no_theme, no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
gs_coord(gs_coordinator)
{
    wxLogMessage("Entering LocalMatchSignificanceCanvas::LocalMatchSignificanceCanvas");
    
    str_sig = _("Not Significant");
    str_low = _("No Colocation");
    str_med = _("Has Colocation");
    str_high = _("Colocation Cluster");
    str_undefined = _("Undefined");
    str_neighborless = _("Neighborless");
    str_p005 = "p = 0.05";
    str_p001 = "p = 0.01";
    str_p0001 = "p = 0.001";
    str_p00001 ="p = 0.0001";
    
    SetPredefinedColor(str_sig, wxColour(240, 240, 240));
    SetPredefinedColor(str_high, wxColour(255, 0, 0));
    SetPredefinedColor(str_med, wxColour(0, 255, 0));
    SetPredefinedColor(str_low, wxColour(0, 0, 255));
    SetPredefinedColor(str_undefined, wxColour(70, 70, 70));
    SetPredefinedColor(str_neighborless, wxColour(140, 140, 140));
    SetPredefinedColor(str_p005, wxColour(75, 255, 80));
    SetPredefinedColor(str_p001, wxColour(6, 196, 11));
    SetPredefinedColor(str_p0001, wxColour(3, 116, 6));
    SetPredefinedColor(str_p00001, wxColour(1, 70, 3));

    cat_classif_def.cat_classif_type = CatClassification::no_theme;
    
    // must set var_info times from JCCoordinator initially
    //var_info = gs_coord->var_info;
    //template_frame->ClearAllGroupDependencies();
    //for (int t=0, sz=var_info.size(); t<sz; ++t) {
    //    template_frame->AddGroupDependancy(var_info[t].name);
    //}
    
    CreateAndUpdateCategories();
    UpdateStatusBar();
    
    wxLogMessage("Exiting LocalMatchSignificanceCanvas::LocalMatchSignificanceCanvas");
}

LocalMatchSignificanceCanvas::~LocalMatchSignificanceCanvas()
{
    wxLogMessage("In LocalMatchSignificanceCanvas::~LocalMatchSignificanceCanvas");
}

void LocalMatchSignificanceCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
    wxLogMessage("Entering LocalMatchSignificanceCanvas::DisplayRightClickMenu");
    // Workaround for right-click not changing window focus in OSX / wxW 3.0
    wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
    ((LocalMatchSignificanceFrame*) template_frame)->OnActivate(ae);
    
    wxMenu* optMenu = wxXmlResource::Get()->
        LoadMenu("ID_LOCALJOINCOUNT_NEW_VIEW_MENU_OPTIONS");
    AddTimeVariantOptionsToMenu(optMenu);
    SetCheckMarks(optMenu);
    
    template_frame->UpdateContextMenuItems(optMenu);
    template_frame->PopupMenu(optMenu, pos + GetPosition());
    template_frame->UpdateOptionMenuItems();
    wxLogMessage("Exiting LocalMatchSignificanceCanvas::DisplayRightClickMenu");
}

wxString LocalMatchSignificanceCanvas::GetCanvasTitle()
{
    wxString new_title;
    
    new_title << _("Local Neighbor Match Test");
    new_title << " Significance Map ";
    new_title << wxString::Format(", pseudo p (%d perm)", gs_coord->permutations);
    
    return new_title;
}

wxString LocalMatchSignificanceCanvas::GetVariableNames()
{
    wxString new_title;

    return new_title;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by MLJC maps */
bool LocalMatchSignificanceCanvas::ChangeMapType(CatClassification::CatClassifType new_theme, SmoothingType new_smoothing)
{
    wxLogMessage("In LocalMatchSignificanceCanvas::ChangeMapType");
    return false;
}

void LocalMatchSignificanceCanvas::SetCheckMarks(wxMenu* menu)
{
    // Update the checkmarks and enable/disable state for the
    // following menu items if they were specified for this particular
    // view in the xrc file.  Items that cannot be enable/disabled,
    // or are not checkable do not appear.
    MapCanvas::SetCheckMarks(menu);
    
    int sig_filter = ((LocalMatchSignificanceFrame*) template_frame)->GetLocalMatchCoordinator()->GetSignificanceFilter();
    
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
                                  sig_filter == 1);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
                                  sig_filter == 2);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
                                  sig_filter == 3);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
                                  sig_filter == 4);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_SETUP"),
                                  sig_filter == -1);

    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_SPECIFIED_SEED"),
                                  gs_coord->IsReuseLastSeed());
}

void LocalMatchSignificanceCanvas::TimeChange()
{
    wxLogMessage("Entering LocalMatchSignificanceCanvas::TimeChange");

    wxLogMessage("Exiting LocalMatchSignificanceCanvas::TimeChange");
}

/** Update Categories based on info in JCCoordinator */
void LocalMatchSignificanceCanvas::CreateAndUpdateCategories()
{
    //SyncVarInfoFromCoordinator();
    template_frame->ClearAllGroupDependencies();
    is_any_time_variant = false;
    is_any_sync_with_global_time = false;
    ref_var_index = -1;
    num_time_vals = 1;
    //map_valid = true;
    //map_error_message = "";

    int t = 0;
    int num_cats = 0;
    double stop_sig = 0;


    int set_perm = gs_coord->permutations;
    stop_sig = 1.0 / (1.0 + set_perm);
    double sig_cutoff = gs_coord->significance_cutoff;

    if (gs_coord->GetSignificanceFilter() < 0) {
        // user specified cutoff
        num_cats += 2;
    } else {
        num_cats += 6 - gs_coord->GetSignificanceFilter();

        if ( sig_cutoff >= 0.0001 && stop_sig > 0.0001) {
            num_cats -= 1;
        }
        if ( sig_cutoff >= 0.001 && stop_sig > 0.001 ) {
            num_cats -= 1;
        }
        if ( sig_cutoff >= 0.01 && stop_sig > 0.01 ) {
            num_cats -= 1;
        }
    }
    
    cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
    cat_data.ClearAllCategoryIds();

    cat_data.SetCategoryLabel(t, 0, str_sig);
    cat_data.SetCategoryColor(t, 0, lbl_color_dict[str_sig]);

    if (gs_coord->GetSignificanceFilter() < 0) {
        // user specified cutoff
        wxString lbl = wxString::Format("p = %g", gs_coord->significance_cutoff);
        cat_data.SetCategoryLabel(t, 1, lbl);
        cat_data.SetCategoryColor(t, 1, wxColour(3, 116, 6));

    } else {
        int s_f = gs_coord->GetSignificanceFilter();
        int set_perm = gs_coord->permutations;
        stop_sig = 1.0 / (1.0 + set_perm);

        wxString def_cats[4] = {str_p005, str_p001, str_p0001, str_p00001};
        double def_cutoffs[4] = {0.05, 0.01, 0.001, 0.0001};

        int cat_idx = 1;
        for (int j=s_f-1; j < 4; j++) {
            if (def_cutoffs[j] >= stop_sig) {
                cat_data.SetCategoryLabel(t, cat_idx, def_cats[j]);
                cat_data.SetCategoryColor(t, cat_idx++, lbl_color_dict[def_cats[j]]);
            }
        }
    }

    std::vector<double> p_val = gs_coord->sigVal;
    std::vector<int> cluster = gs_coord->sigCat;
    if (gs_coord->GetSignificanceFilter() < 0) {
        // user specified cutoff
        double sig_cutoff = gs_coord->significance_cutoff;
        for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
            if (p_val[i] <= sig_cutoff) {
                cat_data.AppendIdToCategory(t, 1, i);
            } else {
                cat_data.AppendIdToCategory(t, 0, i); // not significant
            }
        }
    } else {
        int s_f = gs_coord->GetSignificanceFilter();
        for (int i=0, iend=gs_coord->num_obs; i<iend; i++) {
            if (p_val[i] <= 0.0001) {
                cat_data.AppendIdToCategory(t, 5-s_f, i);
            } else if (p_val[i] <= 0.001) {
                cat_data.AppendIdToCategory(t, 4-s_f, i);
            } else if (p_val[i] <= 0.01) {
                cat_data.AppendIdToCategory(t, 3-s_f, i);
            } else if (p_val[i] <= 0.05) {
                cat_data.AppendIdToCategory(t, 2-s_f, i);
            } else {
                cat_data.AppendIdToCategory(t, 1-s_f, i);
            }
        }
    }

    for (int cat=0; cat<num_cats; cat++) {
        cat_data.SetCategoryCount(t, cat, cat_data.GetNumObsInCategory(t, cat));
    }
    PopulateCanvas();
}

void LocalMatchSignificanceCanvas::UpdateStatusBar()
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
    if (is_clust && gs_coord) {
        double p_val = gs_coord->significance_cutoff;
        wxString inf_str = wxString::Format(" p <= %g", p_val);
        s << inf_str;
    }
    sb->SetStatusText(s);
}

void LocalMatchSignificanceCanvas::TimeSyncVariableToggle(int var_index)
{
    wxLogMessage("In LocalMatchSignificanceCanvas::TimeSyncVariableToggle");
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void LocalMatchSignificanceCanvas::SyncVarInfoFromCoordinator()
{
}

IMPLEMENT_CLASS(LocalMatchSignificanceFrame, MapFrame)
    BEGIN_EVENT_TABLE(LocalMatchSignificanceFrame, MapFrame)
    EVT_ACTIVATE(LocalMatchSignificanceFrame::OnActivate)
END_EVENT_TABLE()

LocalMatchSignificanceFrame::LocalMatchSignificanceFrame(wxFrame *parent, Project* project, LocalMatchCoordinator* gs_coordinator, const wxPoint& pos, const wxSize& size, const long style)
: MapFrame(parent, project, pos, size, style), gs_coord(gs_coordinator)
{
    wxLogMessage("Entering LocalMatchSignificanceFrame::LocalMatchSignificanceFrame");
    
    no_update_weights = true;
    int width, height;
    GetClientSize(&width, &height);

    DisplayStatusBar(true);
    
    wxSplitterWindow* splitter_win = new wxSplitterWindow(this,wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
    splitter_win->SetMinimumPaneSize(10);
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new LocalMatchSignificanceCanvas(rpanel, this, project, gs_coordinator);
    template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
    //WeightsManInterface* w_man_int = project->GetWManInt();
    //((MapCanvas*) template_canvas)->SetWeightsId(w_man_int->GetDefault());

    wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new MapNewLegend(lpanel, template_canvas, wxPoint(0,0), wxSize(0,0));
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
    SetAutoLayout(true);
       
    SetTitle(template_canvas->GetCanvasTitle());
    Show(true);
    wxLogMessage("Exiting LocalMatchSignificanceFrame::LocalMatchSignificanceFrame");
}

LocalMatchSignificanceFrame::~LocalMatchSignificanceFrame()
{
    wxLogMessage("In LocalMatchSignificanceFrame::~LocalMatchSignificanceFrame");
    if (gs_coord) {
        delete gs_coord;
        gs_coord = 0;
    }
}

void LocalMatchSignificanceFrame::OnActivate(wxActivateEvent& event)
{
    wxLogMessage("In LocalMatchSignificanceFrame::OnActivate");
    if (event.GetActive()) {
        RegisterAsActive("LocalMatchSignificanceFrame", GetTitle());
    }
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void LocalMatchSignificanceFrame::MapMenus()
{
    wxLogMessage("In LocalMatchSignificanceFrame::MapMenus");
    wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
    // Map Options Menus
    wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_LOCALJOINCOUNT_NEW_VIEW_MENU_OPTIONS");
    ((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
    ((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
    GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);
    UpdateOptionMenuItems();
}

void LocalMatchSignificanceFrame::UpdateOptionMenuItems()
{
    TemplateFrame::UpdateOptionMenuItems(); // set common items first
    wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
    int menu = mb->FindMenu(_("Options"));
    if (menu == wxNOT_FOUND) {
        wxLogMessage("LocalMatchSignificanceFrame::UpdateOptionMenuItems: Options menu not found");
    } else {
        ((LocalMatchSignificanceCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
    }
}

void LocalMatchSignificanceFrame::UpdateContextMenuItems(wxMenu* menu)
{
    // Update the checkmarks and enable/disable state for the
    // following menu items if they were specified for this particular
    // view in the xrc file.  Items that cannot be enable/disabled,
    // or are not checkable do not appear.
    TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void LocalMatchSignificanceFrame::RanXPer(int permutation)
{
    if (permutation < 9) permutation = 9;
    if (permutation > 99999) permutation = 99999;
    gs_coord->permutations = permutation;
    gs_coord->run();
}

void LocalMatchSignificanceFrame::OnRan99Per(wxCommandEvent& event)
{
    RanXPer(99);
}

void LocalMatchSignificanceFrame::OnRan199Per(wxCommandEvent& event)
{
    RanXPer(199);
}

void LocalMatchSignificanceFrame::OnRan499Per(wxCommandEvent& event)
{
    RanXPer(499);
}

void LocalMatchSignificanceFrame::OnRan999Per(wxCommandEvent& event)
{
    RanXPer(999);
}

void LocalMatchSignificanceFrame::OnRanOtherPer(wxCommandEvent& event)
{
    PermutationCounterDlg dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        long num;
        wxString input = dlg.m_number->GetValue();
        input.ToLong(&num);
        RanXPer((int)num);
    }
}

void LocalMatchSignificanceFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
    gs_coord->SetReuseLastSeed(!gs_coord->IsReuseLastSeed());
}

void LocalMatchSignificanceFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
    uint64_t last_seed = gs_coord->GetLastUsedSeed();
    wxString m;
    m << "The last seed used by the pseudo random\nnumber ";
    m << "generator was " << last_seed << ".\n";
    m << "Enter a seed value to use:";
    long long unsigned int val;
    wxString dlg_val;
    wxString cur_val;
    cur_val << last_seed;
    
    wxTextEntryDialog dlg(NULL, m, "\nEnter a seed value", cur_val);
    if (dlg.ShowModal() != wxID_OK) return;
    dlg_val = dlg.GetValue();
    
    wxLogMessage(dlg_val);
    
    dlg_val.Trim(true);
    dlg_val.Trim(false);
    if (dlg_val.IsEmpty()) return;
    if (dlg_val.ToULongLong(&val)) {
        if (!gs_coord->IsReuseLastSeed()) gs_coord->SetLastUsedSeed(true);
        uint64_t new_seed_val = val;
        gs_coord->SetLastUsedSeed(new_seed_val);
    } else {
        wxString m;
        m << "\"" << dlg_val << "\" is not a valid seed. Seed unchanged.";
        wxMessageDialog dlg(NULL, m, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }
}

void LocalMatchSignificanceFrame::SetSigFilterX(int filter)
{
    if (filter == gs_coord->GetSignificanceFilter())
        return;
    gs_coord->SetSignificanceFilter(filter);
    
    //gs_coord->notifyObservers();
    LocalMatchSignificanceCanvas* lc = (LocalMatchSignificanceCanvas*) template_canvas;
    lc->CreateAndUpdateCategories();
    if (template_legend) template_legend->Recreate();
    SetTitle(lc->GetCanvasTitle());
    lc->Refresh();
    lc->UpdateStatusBar();
    
    UpdateOptionMenuItems();
}

void LocalMatchSignificanceFrame::OnSigFilter05(wxCommandEvent& event)
{
    SetSigFilterX(1);
}

void LocalMatchSignificanceFrame::OnSigFilter01(wxCommandEvent& event)
{
    SetSigFilterX(2);
}

void LocalMatchSignificanceFrame::OnSigFilter001(wxCommandEvent& event)
{
    SetSigFilterX(3);
}

void LocalMatchSignificanceFrame::OnSigFilter0001(wxCommandEvent& event)
{
    SetSigFilterX(4);
}

void LocalMatchSignificanceFrame::OnSigFilterSetup(wxCommandEvent& event)
{
    LocalMatchSignificanceCanvas* lc = (LocalMatchSignificanceCanvas*)template_canvas;
    int n = gs_coord->num_obs;
    double* p_val = new double[n];
    for (size_t i=0; i<n; ++i) p_val[i] = gs_coord->sigVal[i];
    
    wxString ttl = _("Inference Settings");
    ttl << "  (" << gs_coord->permutations << " perm)";
    
    double user_sig = gs_coord->significance_cutoff;
    if (gs_coord->GetSignificanceFilter()<0)
        user_sig = gs_coord->user_sig_cutoff;
  
    if (n > 0) {
        InferenceSettingsDlg dlg(this, user_sig, p_val, n, ttl);
        if (dlg.ShowModal() == wxID_OK) {
            gs_coord->SetSignificanceFilter(-1);
            gs_coord->significance_cutoff = dlg.GetAlphaLevel();
            gs_coord->user_sig_cutoff = dlg.GetUserInput();
            //gs_coord->notifyObservers();
            gs_coord->bo = dlg.GetBO();
            gs_coord->fdr = dlg.GetFDR();
            UpdateOptionMenuItems();
        }
        delete[] p_val;
    }
}

void LocalMatchSignificanceFrame::OnSaveMLJC(wxCommandEvent& event)
{
    wxString title = _("Save Results: Local Match Test, ");
    title += wxString::Format("pseudo p (%d perm), ", gs_coord->permutations);

    int num_obs = gs_coord->num_obs;
    std::vector<bool> p_undefs(num_obs, false);
    std::vector<int> c_val = gs_coord->sigCat;
    
    std::vector<double> pp_val = gs_coord->sigVal;
    wxString pp_label = "Pseudo p-value";
    wxString pp_field_default = "PP_VAL";

    int num_data = 1;
    
    std::vector<SaveToTableEntry> data(num_data);
    std::vector<bool> undefs(gs_coord->num_obs, false);
   
    int data_i = 0;
    
    data[data_i].d_val = &pp_val;
    data[data_i].label = pp_label;
    data[data_i].field_default = pp_field_default;
    data[data_i].type = GdaConst::double_type;
    data[data_i].undefined = &p_undefs;
    data_i++;
    
    SaveToTableDlg dlg(project, this, data, title,
                       wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
}

void LocalMatchSignificanceFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
    HighlightState* highlight_state = project->GetHighlightState();
    std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
    
    for (int i=0; i<gs_coord->num_obs; i++) {
        if (!hs[i] && elem[i]) {
            hs[i] = true;
            selection_changed  = true;
        } else if (hs[i] && !elem[i]) {
            hs[i] = false;
            selection_changed  = true;
        }
    }
    if (selection_changed ) {
        highlight_state->SetEventType(HLStateInt::delta);
        highlight_state->notifyObservers();
    }
}

void LocalMatchSignificanceFrame::OnSelectCores(wxCommandEvent& event)
{
    wxLogMessage("Entering LocalMatchSignificanceFrame::OnSelectCores");
    wxLogMessage("Exiting LocalMatchSignificanceFrame::OnSelectCores");
}

void LocalMatchSignificanceFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
    wxLogMessage("Entering LocalMatchSignificanceFrame::OnSelectNeighborsOfCores");
    wxLogMessage("Exiting LocalMatchSignificanceFrame::OnSelectNeighborsOfCores");
}

void LocalMatchSignificanceFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
    wxLogMessage("Entering LocalMatchSignificanceFrame::OnSelectCoresAndNeighbors");
    wxLogMessage("Exiting LocalMatchSignificanceFrame::OnSelectCoresAndNeighbors");
}

void LocalMatchSignificanceFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
                            _("Conditional Local Match Test Map"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"));
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
}


