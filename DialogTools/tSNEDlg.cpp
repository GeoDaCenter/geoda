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
#include <wx/event.h>
#include <boost/lockfree/queue.hpp>

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../FramesManager.h"
#include "../DataViewer/TableInterface.h"
#include "../Project.h"
#include "../Algorithms/DataUtils.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/mds.h"
#include "../Algorithms/vptree.h"
#include "../Algorithms/splittree.h"
#include "../Explore/ScatterNewPlotView.h"
#include "../Explore/3DPlotView.h"
#include "SaveToTableDlg.h"
#include "tSNEDlg.h"


// exchange data between t-SNE and UI safely
boost::lockfree::queue<int> tsne_queue(0);

wxDEFINE_EVENT(myEVT_THREAD_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(myEVT_THREAD_DONE, wxThreadEvent);

BEGIN_EVENT_TABLE( TSNEDlg, wxDialog )
EVT_CLOSE( TSNEDlg::OnClose )
END_EVENT_TABLE()


TSNEDlg::TSNEDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("t-SNE Settings")),
data(0), Y(0), ragged_distances(0), tsne(0), tsne_job(0),
old_report(""), dist('e')
{
    wxLogMessage("Open tSNE Dialog.");
    CreateControls();

    this->Connect(myEVT_THREAD_UPDATE, wxThreadEventHandler(TSNEDlg::OnThreadUpdate ) );
    this->Connect(myEVT_THREAD_DONE, wxThreadEventHandler(TSNEDlg::OnThreadDone ) );

    // we want to start a long task, but we don't want our GUI to block
    // while it's executed, so we use a thread to do it.
    if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)  {
        wxLogError("Could not create the worker thread!");
        return;
    }
}

TSNEDlg::~TSNEDlg()
{
    if (data) delete[] data;
    if (Y) delete[] Y;
    if (tsne_job) {
        tsne_job->join();
        delete tsne_job;
    }
    if (tsne) {
        delete tsne;
    }
}

void TSNEDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close TSNEDlg");
    // stop tsne
    if (tsne) {
        tsne->set_paused(false);
        tsne->stop();
    }
    
    GetThread()->Kill();
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    // important: before terminating, we _must_ wait for our joinable
    // thread to end, if it's running; in fact it uses variables of this
    // instance and posts events to *this event handler
    if (GetThread() &&      // DoStartALongTask() may have not been called
        GetThread()->IsRunning()) {
        GetThread()->Wait();
    }
    Destroy();
}

void TSNEDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close TSNEDlg.");
    wxCloseEvent ev;
    OnClose(ev);
    event.Skip();
}

void TSNEDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(900,900), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    AddSimpleInputCtrls(panel, vbox);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(15,2,10,0);

    // perplexity
    size_t num_obs = project->GetNumRecords();
    wxString str_perplexity;
    if ((int)((num_obs - 1) /  3)  < 30) str_perplexity << (int)((num_obs - 1) /  3);
    else str_perplexity << 30;

    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Perplexity:"));
    txt_perplexity = new wxTextCtrl(panel, wxID_ANY, str_perplexity,wxDefaultPosition, wxSize(70,-1));
    txt_perplexity->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_perplexity, 1, wxEXPAND);

    // theta
    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY, _("Theta:"));
    txt_theta = new wxTextCtrl(panel, wxID_ANY, "0.5",wxDefaultPosition, wxSize(70,-1));
    txt_theta->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_theta, 1, wxEXPAND);

    // max iteration
    wxStaticText* st15 = new wxStaticText(panel, wxID_ANY, _("Max Iteration:"));
    txt_iteration = new wxTextCtrl(panel, wxID_ANY, "5000",wxDefaultPosition, wxSize(70,-1));
    txt_iteration->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st15, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_iteration, 1, wxEXPAND);


    // lr
    wxStaticText* st21 = new wxStaticText(panel, wxID_ANY, _("Learning Rate:"));
    txt_learningrate = new wxTextCtrl(panel, wxID_ANY, "200",wxDefaultPosition, wxSize(70,-1));
    txt_learningrate->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st21, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_learningrate, 1, wxEXPAND);

    // momentum
    wxStaticText* st18 = new wxStaticText(panel, wxID_ANY, _("Momentum:"));
    txt_momentum = new wxTextCtrl(panel, wxID_ANY, "0.5",wxDefaultPosition, wxSize(70,-1));
    txt_momentum->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st18, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_momentum, 1, wxEXPAND);

    // final momentum
    wxStaticText* st19 = new wxStaticText(panel, wxID_ANY, _("Final Momentum:"));
    txt_finalmomentum = new wxTextCtrl(panel, wxID_ANY, "0.8",wxDefaultPosition, wxSize(70,-1));
    txt_finalmomentum->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st19, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_finalmomentum, 1, wxEXPAND);

    // mom_switch_iter
    wxStaticText* st20 = new wxStaticText(panel, wxID_ANY, _("# Iteration Switch Momentum:"));
    txt_mom_switch_iter = new wxTextCtrl(panel, wxID_ANY, "250",wxDefaultPosition, wxSize(70,-1));
    txt_mom_switch_iter->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st20, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_mom_switch_iter, 1, wxEXPAND);
    
    
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Distance Function:"));
    wxString choices13[] = {"Euclidean", "Manhattan"};
    m_distance = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 2, choices13);
    m_distance->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_distance, 1, wxEXPAND);

    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Category Variable:"));
    wxBoxSizer *hbox18 = new wxBoxSizer(wxHORIZONTAL);
    chk_group = new wxCheckBox(panel, wxID_ANY, "");
    {
        std::vector<int> col_id_map;
        table_int->FillStringAndIntegerColIdMap(col_id_map);
        for (int i=0, iend=col_id_map.size(); i<iend; i++) {
            int id = col_id_map[i];
            wxString name = table_int->GetColName(id);
            if (!table_int->IsColTimeVariant(id)) {
                cat_var_items.Add(name);
            }
        }
    }
    m_group = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                               wxSize(128,-1), cat_var_items);
    hbox18->Add(chk_group,0, wxALIGN_CENTER_VERTICAL);
    hbox18->Add(m_group,0,wxALIGN_CENTER_VERTICAL);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox18, 1, wxEXPAND);

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

    // Output
    /*
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("# of Dimensions:"));
    const wxString dims[2] = {"2", "3"};
    combo_n = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(120,-1), 2, dims);
    combo_n->Enable(false);

    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(combo_n, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
     */
    
    // buttons
    runButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    stopButton = new wxButton(panel, wxID_OK, _("Stop"), wxDefaultPosition,
                             wxSize(70, 30));
    saveButton = new wxButton(panel, wxID_OK, _("Save"), wxDefaultPosition,
                                      wxSize(70, 30));
    saveButton->Enable(false);
    stopButton->Enable(false);
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(runButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(stopButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(saveButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    //vbox->Add(hbox1, 0, wxALL |wxEXPAND, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);

    wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    int n = project->GetNumRecords();
    std::vector<double> X(n, 0), Y(n,0);
    std::vector<bool> X_undef(n, false), Y_undef(n,false);
    int style = AnimatePlotcanvas::DEFAULT_STYLE | AnimatePlotcanvas::show_vert_axis_through_origin | AnimatePlotcanvas::show_data_points | AnimatePlotcanvas::show_horiz_axis_through_origin;
    m_animate = new AnimatePlotcanvas(this, NULL, project, X, Y, X_undef, Y_undef,
                                      "", "", style, std::vector<std::vector<int> >(),
                                      "", "", wxDefaultPosition,
                                      wxSize(300, 300));

    wxBoxSizer *hbox_4 = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* st29 = new wxStaticText(panel, wxID_ANY, _("Iteration"),
                                          wxDefaultPosition, wxSize(60, 30));
    m_slider = new wxSlider(panel, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_slider->Enable(false);
    hbox_4->Add(st29, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 15);
    hbox_4->Add(m_slider, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    wxBoxSizer *hbox_3 = new wxBoxSizer(wxHORIZONTAL);
    playButton = new wxButton(panel, wxID_OK, _(">"), wxDefaultPosition, wxSize(30, 30));
    pauseButton = new wxButton(panel, wxID_OK, _("||"), wxDefaultPosition, wxSize(30, 30));
    //stopButton = new wxButton(panel, wxID_OK, _("■"), wxDefaultPosition, wxSize(30, 30));
    playButton->Enable(false);
    pauseButton->Enable(false);
    //stopButton->Enable(false);
    wxStaticText* st28 = new wxStaticText(panel, wxID_ANY, _("Speed"),
                                          wxDefaultPosition, wxSize(40, 30));
    m_speed_slider = new wxSlider(panel, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(70, 30), wxSL_HORIZONTAL);

    hbox_3->Add(playButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox_3->Add(pauseButton, 0, wxALIGN_CENTER | wxALL, 5);
    //hbox_3->Add(stopButton, 0, wxALIGN_CENTER | wxALL, 5);
    hbox_3->Add(st28, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxLEFT, 10);
    hbox_3->Add(m_speed_slider, 1, wxALIGN_CENTER | wxALL, 5);
    
    m_textbox = new SimpleReportTextCtrl(panel, XRCID("ID_TEXTCTRL"), "", wxDefaultPosition, wxSize(300,400));
    vbox1->Add(m_animate, 1, wxEXPAND|wxLEFT|wxRIGHT,20);
    vbox1->Add(hbox_4, 0, wxEXPAND|wxLEFT|wxRIGHT,20);
    vbox1->Add(hbox_3, 0, wxEXPAND|wxLEFT|wxRIGHT,20);
    vbox1->Add(m_textbox, 0, wxEXPAND|wxALL,20);

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
    runButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnOK, this);
    playButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnPlay, this);
    pauseButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnPause, this);
    stopButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnStop, this);
    saveButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnSave, this);
    closeButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnCloseClick, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &TSNEDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &TSNEDlg::OnChangeSeed, this);
    m_slider->Bind(wxEVT_SLIDER, &TSNEDlg::OnSlider, this);
    m_speed_slider->Bind(wxEVT_SLIDER, &TSNEDlg::OnSpeedSlider, this);
}

void TSNEDlg::OnSeedCheck(wxCommandEvent& event)
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

void TSNEDlg::OnChangeSeed(wxCommandEvent& event)
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

void TSNEDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("InitVariableCombobox TSNEDlg.");
    
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
    if (!items.IsEmpty())
        var_box->InsertItems(items,0);
}

wxString TSNEDlg::_printConfiguration()
{
    return "";
}

double TSNEDlg::_calculateRankCorr(const std::vector<std::vector<double> >& result)
{
    double d;
    std::vector<double> x, y;
    for (size_t r=1; r<rows; ++r) {
        for (size_t c=0; c<r; ++c) {
            x.push_back(ragged_distances[r][c]);
            if (dist == 'b') {
                d = DataUtils::ManhattanDistance(result, r, c);
            } else {
                d = DataUtils::EuclideanDistance(result, r, c);
            }
            y.push_back(d);
        }
    }
    double r = GenUtils::RankCorrelation(x, y);
    return r;
}

void TSNEDlg::OnSlider(wxCommandEvent& ev)
{
    if (m_slider->IsEnabled()) {
        int idx = m_slider->GetValue();
        m_animate->UpdateCanvas(idx, tsne_results);
    }
}

void TSNEDlg::OnSpeedSlider(wxCommandEvent& ev)
{
    if (m_speed_slider->IsEnabled()) {
        if (tsne) {
            int idx = 100 - m_speed_slider->GetValue();
            tsne->set_speed(idx);
        }
    }
}

void TSNEDlg::OnPlay(wxCommandEvent& event )
{
    if (tsne) {
        tsne->set_paused(false);
        playButton->Enable(false);
        pauseButton->Enable(true);
        stopButton->Enable(true);
    }
}

void TSNEDlg::OnPause(wxCommandEvent& event )
{
    if (tsne) {
        tsne->set_paused(true);
        playButton->Enable(true);
        pauseButton->Enable(false);
        stopButton->Enable(true);
    }
}

void TSNEDlg::OnStop(wxCommandEvent& event )
{
    if (tsne) {
        tsne->set_paused(false);
        runButton->Enable(true);
        playButton->Enable(false);
        pauseButton->Enable(false);
        stopButton->Enable(false);
        tsne->stop();
    }
}

void TSNEDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click TSNEDlg::OnOK");

    int transform = combo_tranform->GetSelection();
   
    if (!GetInputData(transform, 1))
        return;

    double* weight = GetWeights(columns);

    double perplexity = 0;
    int suggest_perp = (int)((project->GetNumRecords() - 1) /  3);
    wxString val = txt_perplexity->GetValue();
    if (!val.ToDouble(&perplexity)) {
        wxString err_msg = _("Please input a valid numeric value for perplexity.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    if (perplexity > suggest_perp) {
        wxString err_msg = _("Perplexity parameter should not be larger than %d.");
        wxMessageDialog dlg(NULL, wxString::Format(err_msg, suggest_perp), _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double theta;
    val = txt_theta->GetValue();
    if (!val.ToDouble(&theta)) {
        wxString err_msg = _("Please input a valid numeric value for theta.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double momentum;
    val = txt_momentum->GetValue();
    if (!val.ToDouble(&momentum)) {
        wxString err_msg = _("Please input a valid numeric value for momentum.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    double finalmomentum;
    val = txt_finalmomentum->GetValue();
    if (!val.ToDouble(&finalmomentum)) {
        wxString err_msg = _("Please input a valid numeric value for final momentum.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    double learningrate;
    val = txt_learningrate->GetValue();
    if (!val.ToDouble(&learningrate)) {
        wxString err_msg = _("Please input a valid numeric value for learning rate.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    val = txt_iteration->GetValue();
    if (!val.ToLong(&max_iteration)) {
        wxString err_msg = _("Please input a valid numeric value for max iterations.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    long mom_switch_iter;
    val = txt_mom_switch_iter->GetValue();
    if (!val.ToLong(&mom_switch_iter)) {
        wxString err_msg = _("Please input a valid numeric value for iterations for momentum switch.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    groups.clear();
    group_labels.clear();
    if (chk_group->IsChecked()) {
        int idx = m_group->GetSelection();
        wxString nm = m_group->GetString(idx);
        int col = table_int->FindColId(nm);
        if (col != wxNOT_FOUND) {
            if (table_int->IsColNumeric(col)) {
                std::vector<double> group_variable(rows, 0);
                table_int->GetColData(col, 0, group_variable);
                std::map<int, std::vector<int> > group_ids;
                std::map<int, std::vector<int> >::iterator it;
                for (size_t i=0; i<rows; ++i) {
                    group_ids[group_variable[i]].push_back(i);
                }
                for (it=group_ids.begin(); it!=group_ids.end(); ++it ) {
                    groups.push_back(it->second);
                    group_labels.push_back(wxString::Format("%d",it->first));
                }
            } else {

                std::vector<wxString> group_variable(rows);
                table_int->GetColData(col, 0, group_variable);
                std::map<wxString, std::vector<int> > group_ids;
                std::map<wxString, std::vector<int> >::iterator it;
                for (size_t i=0; i<rows; ++i) {
                    group_ids[group_variable[i]].push_back(i);
                }
                for (it=group_ids.begin(); it!=group_ids.end(); ++it ) {
                    groups.push_back(it->second);
                    group_labels.push_back(it->first);
                }

            }
            m_animate->CreateAndUpdateCategories(groups);
        }
    }

    runButton->Enable(false);
    saveButton->Enable(false);

    playButton->Enable(false);
    pauseButton->Enable(true);
    stopButton->Enable(true);

    m_slider->Enable(false);
    m_textbox->SetValue("");

    long out_dim = 2; //combo_n->GetSelection() == 0 ? 2 : 3;
  
    int new_col = out_dim;

    if (data) delete[] data;
    data = new double[rows * columns];
    for (size_t i=0; i<rows; ++i) {
        for (size_t j=0; j<columns; ++j) {
            data[i*columns + j] = input_data[i][j];
        }
    }

    int transpose = 0; // row wise
    int dist_sel = m_distance->GetSelection();
    char dist_choices[] = {'e','b'};
    dist = dist_choices[dist_sel];

    if (ragged_distances) {
        for (size_t i=1; i< rows; ++i) free(ragged_distances[i]);
        free(ragged_distances);
    }
    ragged_distances = distancematrix(rows, columns, input_data,  mask, weight, dist, transpose);

    if (Y) delete[] Y;
    Y = new double[rows * new_col];

    m_slider->SetMin(1);
    m_slider->SetMax(max_iteration);

    int num_threads = 1;
    int verbose = 0;
#ifdef DEBUG
    verbose = 1;
#endif
    double early_exaggeration = 12; // default from R-tSNE
    final_cost = 0;
    m_textbox->SetValue(""); // clean text box

    int iter;
    while (tsne_queue.pop(iter)){} // clean tsne_queue
    
    // start tsne instance
    if (tsne) {
        delete tsne;
    }
    tsne = new TSNE(data, rows, columns, Y, new_col, perplexity, theta, num_threads,
                    max_iteration, (int)mom_switch_iter,
                    (int)GdaConst::gda_user_seed, !GdaConst::use_gda_user_seed,
                    verbose, early_exaggeration, learningrate, &final_cost);
    int idx = 100 - m_speed_slider->GetValue();
    tsne->set_speed(idx);

    // run tsne in a separate thread
    tsne_results.clear();
    tsne_results.resize(max_iteration);
    tsne_log.clear();
    
    if (tsne_job) {
        int t =  100 - m_speed_slider->GetValue();
        if (t > 0) tsne_job->interrupt();

        if (tsne_job->joinable()) {
            tsne_job->join();
        }
        delete tsne_job;
    }
    tsne_job = new boost::thread(&TSNE::run, tsne, boost::ref(tsne_queue),
                                 boost::ref(tsne_log), boost::ref(tsne_results));

    // start ui thread to listen to changes in tsne
    if (GetThread()->IsRunning() == false) {
        if (GetThread()->Run() != wxTHREAD_NO_ERROR) {
            wxLogError("Could not run the worker thread!");
            return;
        }
    }

    pauseButton->Enable(true);
}

void TSNEDlg::OnThreadUpdate(wxThreadEvent& evt)
{
    int iter = evt.GetInt();
    if (iter < 0 ) {
        // pause and stop
        max_iteration = m_slider->GetValue();
        m_slider->SetMax(max_iteration);
        m_animate->UpdateCanvas(iter, tsne_results);
        m_slider->Enable(true);
        saveButton->Enable(true);
    } else if (iter < max_iteration) {
        m_animate->UpdateCanvas(iter, tsne_results);
        m_slider->SetValue(iter+1);

        wxString log;
        for (int i=tsne_log.size()-1; i >=0; --i) {
            log << tsne_log[i];
        }
        m_textbox->SetValue(log);
    }
}

void TSNEDlg::OnThreadDone(wxThreadEvent& evt)
{
    saveButton->Enable(true);
    runButton->Enable(true);

    playButton->Enable(false);
    pauseButton->Enable(false);
    stopButton->Enable(false);

    m_slider->Enable(true);
    m_speed_slider->Enable(true);

    if (m_slider->GetValue() < m_slider->GetMax()) {
        m_slider->Enable(false);
    }

    wxString tsne_log;
    tsne_log << _("---\n\nt-SNE: ");
    tsne_log << "\nfinal cost:" << final_cost;
    tsne_log << "\n";

    tsne_log << m_textbox->GetValue();
    tsne_log << old_report;
    m_textbox->SetValue(tsne_log);

    old_report = tsne_log;
}

wxThread::ExitCode TSNEDlg::Entry()
{
    // IMPORTANT:
    // this function gets executed in the secondary thread context!
    // here we do our long task, periodically calling TestDestroy():
    while (!GetThread()->TestDestroy())
    {
        int iter = 0;
        while (iter < max_iteration - 1) {
            int processed_iter = 0;
            while (tsne_queue.pop(iter)) {
                processed_iter++;
            }
            if (processed_iter > 0) {
                wxThreadEvent* te = new wxThreadEvent(myEVT_THREAD_UPDATE);
                te->SetInt(iter);
                wxQueueEvent(this, te);
            }
            if (GetThread()->IsRunning()) {
                GetThread()->Sleep(100);
            }
        }

        // VERY IMPORTANT: do not call any GUI function inside this
        //                 function; rather use wxQueueEvent():
        wxQueueEvent(this, new wxThreadEvent(myEVT_THREAD_DONE));
        // we used pointer 'this' assuming it's safe; see OnClose()
    }
    // TestDestroy() returned true (which means the main thread asked us
    // to terminate as soon as possible) or we ended the long task...
    return (wxThread::ExitCode)0;
}

void TSNEDlg::OnSave( wxCommandEvent& event ) {
    long new_col = 2;//combo_n->GetSelection() == 0 ? 2 : 3;

    int sel_iter = m_slider->GetValue() - 1;

    // get results from selected iteration
    const std::vector<double>& data = tsne_results[sel_iter];
    std::vector<double> X(rows), Y(rows);
    for (int j = 0; j < rows; ++j) {
        X[j] = data[j*new_col];
    }
    for (int j = 0; j < rows; ++j) {
        Y[j] = data[j*new_col + 1];
    }
    std::vector<std::vector<double> > results;
    results.push_back(X);
    results.push_back(Y);

    // compute rank correlation
    double rank_corr = _calculateRankCorr(results);

    std::vector<std::pair<wxString, double> > output_vals;
    output_vals.push_back(std::make_pair("iterations", sel_iter + 1));
    output_vals.insert(output_vals.begin(), std::make_pair("rank correlation", rank_corr));
    output_vals.insert(output_vals.begin(), std::make_pair("final cost", final_cost));

    if (!results.empty()) {
        wxString method_str = "t-SNE";
        std::vector<wxString> info_str;
        for (size_t k=0; k<col_names.size(); k++) {
            info_str.push_back(col_names[k]);
        }
        
        std::vector<SaveToTableEntry> new_data(new_col);
        std::vector<std::vector<double> > vals(new_col);
        std::vector<std::vector<bool> > undefs(new_col);
        
        for (int j = 0; j < new_col; ++j) {
            vals[j].resize(rows);
            undefs[j].resize(rows);
            for (int i = 0; i < rows; ++i) {
                vals[j][i] = double(results[j][i]);
                undefs[j][i] = false;
            }
            new_data[j].d_val = &vals[j];
            new_data[j].label = wxString::Format("V%d", j+1);
            new_data[j].field_default = wxString::Format("V%d", j+1);
            new_data[j].type = GdaConst::double_type;
            new_data[j].undefined = &undefs[j];
        }
        
        SaveToTableDlg dlg(project, this, new_data,
                           _("Save Results: t-SNE"),
                           wxDefaultPosition, wxSize(400,400));
        if (dlg.ShowModal() == wxID_OK) {
            // show in a scatter plot
            std::vector<int>& new_col_ids = dlg.new_col_ids;
            std::vector<wxString>& new_col_names = dlg.new_col_names;

            // at least 2 variables
            if (new_col_ids.size() < 2) return;

            int num_new_vars = new_col_ids.size();

            std::vector<GdaVarTools::VarInfo> new_var_info;
            new_var_info.resize(num_new_vars);
            
            new_var_info[0].time = 0;
            // Set Primary GdaVarTools::VarInfo attributes
            new_var_info[0].name = new_col_names[0];
            new_var_info[0].is_time_variant = table_int->IsColTimeVariant(new_col_ids[0]);
            table_int->GetMinMaxVals(new_col_ids[0], new_var_info[0].min, new_var_info[0].max);
            new_var_info[0].sync_with_global_time = new_var_info[0].is_time_variant;
            new_var_info[0].fixed_scale = true;
            
            new_var_info[1].time = 0;
            // Set Primary GdaVarTools::VarInfo attributes
            new_var_info[1].name = new_col_names[1];
            new_var_info[1].is_time_variant = table_int->IsColTimeVariant(new_col_ids[1]);
            table_int->GetMinMaxVals(new_col_ids[1], new_var_info[1].min, new_var_info[1].max);
            new_var_info[1].sync_with_global_time = new_var_info[1].is_time_variant;
            new_var_info[1].fixed_scale = true;

            if (num_new_vars == 2) {
                wxString title = _("t-SNE Plot (%s) - %s, %s");
                title = wxString::Format(title, method_str, new_col_names[0], new_col_names[1]);
                MDSPlotFrame* subframe =
                new MDSPlotFrame(parent, project, groups, group_labels,
                                 info_str, output_vals,
                                    new_var_info, new_col_ids,
                                    false, title, wxDefaultPosition,
                                    GdaConst::scatterplot_default_size,
                                    wxDEFAULT_FRAME_STYLE);

            } else if (num_new_vars == 3) {

                new_var_info[2].time = 0;
                // Set Primary GdaVarTools::VarInfo attributes
                new_var_info[2].name = new_col_names[2];
                new_var_info[2].is_time_variant = table_int->IsColTimeVariant(new_col_ids[2]);
                table_int->GetMinMaxVals(new_col_ids[2], new_var_info[2].min, new_var_info[2].max);
                new_var_info[2].sync_with_global_time = new_var_info[2].is_time_variant;
                new_var_info[2].fixed_scale = true;


                wxString title = _("t-SNE 3D Plot - %s, %s, %s");
                title = wxString::Format(title,  new_col_names[0], new_col_names[1], new_col_names[2]);

                C3DPlotFrame *subframe =
                new C3DPlotFrame(parent, project,
                                 new_var_info, new_col_ids,
                                 title, info_str, output_vals, wxDefaultPosition,
                                 GdaConst::three_d_default_size,
                                 wxDEFAULT_FRAME_STYLE);
            }
        }

    }
}
