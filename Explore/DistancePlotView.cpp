#include <wx/splitter.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "../Project.h"
#include "../HighlightState.h"
#include "SimpleScatterPlotCanvas.h"
#include "DistancePlotView.h"

IMPLEMENT_CLASS(DistancePlotFrame, TemplateFrame)
BEGIN_EVENT_TABLE(DistancePlotFrame, TemplateFrame)
EVT_ACTIVATE(DistancePlotFrame::OnActivate)
END_EVENT_TABLE()

DistancePlotFrame::DistancePlotFrame(wxFrame *parent, Project* project,
                                     const std::vector<double>& X,
                                     const std::vector<double>& Y,
                                     const std::vector<bool>& X_undef,
                                     const std::vector<bool>& Y_undef,
                                     double x_min, double x_max,
                                     double y_min, double y_max,
                                     const wxString& X_label,
                                     const wxString& Y_label,
                                     const wxPoint& pos, const wxSize& size,
                                     const long style)
: ScatterNewPlotFrame(parent, project, pos, size, style)
{
    int width, height;
    GetClientSize(&width, &height);
    wxSplitterWindow* splitter_win = 0;

    HighlightState* local_hl_state = project->GetHighlightState();
    SimpleScatterPlotCanvasCbInt* ssp_canv_cb = 0;
    bool show_axes = true;
    template_canvas = new SimpleScatterPlotCanvas(this, this, project,
                                                  local_hl_state, ssp_canv_cb,
                                                  X, Y, X_undef, Y_undef,
                                                  X_label, Y_label,
                                                  x_min, x_max, y_min, y_max,
                                                  false, false, true,
                                                  "ID_CORRELOGRAM_MENU_OPTIONS",
                                                  show_axes, // show axes
                                                  true, // show horiz axis thru orig
                                                  false, // show vert axis thru orig
                                                  false, // regimes
                                                  false, // linear smoother
                                                  true, // show LOWESS fit
                                                  false);


    template_canvas->SetScrollRate(1,1);
    DisplayStatusBar(true);
    SetTitle("Distance Scatter Plot");

    Show(true);
}

DistancePlotFrame::~DistancePlotFrame()
{
}

void DistancePlotFrame::OnActivate(wxActivateEvent& event)
{
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

DistancePlotDlg::DistancePlotDlg(wxWindow* parent, Project* project, wxWindowID id)
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    SetBackgroundColour(*wxWHITE);



    thresh_cbx = new wxCheckBox(panel, XRCID("ID_THRESH_CBX"), _("Max Distance:"));
    thresh_cbx->SetValue(false);
    thresh_tctrl = new wxTextCtrl(panel, XRCID("ID_THRESH_TCTRL"), "",
                                  wxDefaultPosition, wxSize(100,-1),
                                  wxTE_PROCESS_ENTER);
    thresh_tctrl->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    thresh_tctrl->Enable(false);
    //UpdateThreshTctrlVal();
    Connect(XRCID("ID_THRESH_CBX"), wxEVT_CHECKBOX,
            wxCommandEventHandler(DistancePlotDlg::OnThreshCheckBox));
    Connect(XRCID("ID_THRESH_TCTRL"), wxEVT_TEXT_ENTER,
            wxCommandEventHandler(DistancePlotDlg::OnThreshTextCtrl));

    wxBoxSizer* thresh_h_szr = new wxBoxSizer(wxHORIZONTAL);
    thresh_h_szr->Add(thresh_cbx, 0, wxALIGN_CENTER_VERTICAL);
    thresh_h_szr->AddSpacer(5);
    thresh_h_szr->Add(thresh_tctrl, 0, wxALIGN_CENTER_VERTICAL);
    thresh_slider = new wxSlider(panel, XRCID("ID_THRESH_SLDR"),
                                 sldr_tcks/2, 0, sldr_tcks,
                                 wxDefaultPosition, wxSize(180,-1));

    Connect(XRCID("ID_THRESH_SLDR"), wxEVT_SLIDER,
            wxCommandEventHandler(DistancePlotDlg::OnThreshSlider));

    thresh_slider->Enable(false);
    wxBoxSizer* thresh_sld_h_szr = new wxBoxSizer(wxHORIZONTAL);
    thresh_sld_h_szr->Add(thresh_slider, 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* thresh_v_szr = new wxBoxSizer(wxVERTICAL);
    thresh_v_szr->Add(thresh_h_szr, 0, wxBOTTOM, 5);
    thresh_v_szr->Add(thresh_sld_h_szr, 0, wxALIGN_CENTER_HORIZONTAL);

    all_pairs_rad = new wxRadioButton(panel, XRCID("ID_ALL_PAIRS_RAD"),
                                      _("All Pairs"), wxDefaultPosition,
                                      wxDefaultSize,
                                      wxALIGN_CENTER_VERTICAL | wxRB_GROUP);
    all_pairs_rad->SetValue(true);
    Connect(XRCID("ID_ALL_PAIRS_RAD"), wxEVT_RADIOBUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnAllPairsRadioSelected));

    est_pairs_txt = new wxStaticText(panel, XRCID("ID_EST_PAIRS_TXT"), _("Estimated Pairs:"));
    est_pairs_num_txt = new wxStaticText(panel, XRCID("ID_EST_PAIRS_NUM_TXT"), "4,000,000");
    wxBoxSizer* est_pairs_h_szr = new wxBoxSizer(wxHORIZONTAL);
    est_pairs_h_szr->Add(est_pairs_txt, 0, wxALIGN_CENTER_VERTICAL);
    est_pairs_h_szr->AddSpacer(5);
    est_pairs_h_szr->Add(est_pairs_num_txt, 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* all_pairs_v_szr = new wxBoxSizer(wxVERTICAL);
    all_pairs_v_szr->Add(all_pairs_rad);
    all_pairs_v_szr->AddSpacer(2);
    all_pairs_v_szr->Add(est_pairs_h_szr, 0, wxLEFT, 18);

    rand_samp_rad = new wxRadioButton(panel, XRCID("ID_RAND_SAMP_RAD"), _("Random Sample"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL);
    rand_samp_rad->SetValue(false);
    Connect(XRCID("ID_RAND_SAMP_RAD"), wxEVT_RADIOBUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnRandSampRadioSelected));
    max_iter_txt = new wxStaticText(panel, XRCID("ID_MAX_ITER_TXT"), _("Sample Size:"));
    {
        wxString vs;
        vs << correl_params.max_iterations;
        max_iter_tctrl = new wxTextCtrl(panel, XRCID("ID_MAX_ITER_TCTRL"),
                                        vs, wxDefaultPosition,
                                        wxSize(100,-1), wxTE_PROCESS_ENTER);
        max_iter_tctrl->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
        Connect(XRCID("ID_MAX_ITER_TCTRL"), wxEVT_TEXT_ENTER,
                wxCommandEventHandler(DistancePlotDlg::OnMaxIterTextCtrl));
    }
    wxBoxSizer* max_iter_h_szr = new wxBoxSizer(wxHORIZONTAL);
    max_iter_h_szr->Add(max_iter_txt, 0, wxALIGN_CENTER_VERTICAL);
    max_iter_h_szr->AddSpacer(8);
    max_iter_h_szr->Add(max_iter_tctrl, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* random_opt_h_szr = new wxBoxSizer(wxHORIZONTAL);
    {
        wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Use specified seed:"),
                                              wxDefaultPosition, wxSize(128,-1));
        wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
        chk_seed = new wxCheckBox(panel, wxID_ANY, "");
        seedButton = new wxButton(panel, wxID_OK, _("Change"), wxDefaultPosition, wxSize(64, -1));
        random_opt_h_szr->Add(st17, 0, wxALIGN_CENTER_VERTICAL);
        random_opt_h_szr->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
        random_opt_h_szr->Add(seedButton, 0, wxALIGN_CENTER_VERTICAL);
        if (GdaConst::use_gda_user_seed) {
            chk_seed->SetValue(true);
            seedButton->Enable();
        }
        chk_seed->Bind(wxEVT_CHECKBOX, &DistancePlotDlg::OnSeedCheck, this);
        seedButton->Bind(wxEVT_BUTTON, &DistancePlotDlg::OnChangeSeed, this);
    }

    wxBoxSizer* rand_samp_v_szr = new wxBoxSizer(wxVERTICAL);
    rand_samp_v_szr->Add(rand_samp_rad);
    rand_samp_v_szr->AddSpacer(2);
    rand_samp_v_szr->Add(max_iter_h_szr, 0, wxLEFT, 12);
    rand_samp_v_szr->Add(random_opt_h_szr, 0, wxLEFT, 12);

    help_btn = new wxButton(panel, XRCID("ID_HELP_BTN"), _("Cancel"),
                            wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    apply_btn = new wxButton(panel, XRCID("ID_APPLY_BTN"), _("Apply"),
                             wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    Connect(XRCID("ID_CANCEL"), wxEVT_BUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnCancelBtn));
    Connect(XRCID("ID_APPLY_BTN"), wxEVT_BUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnApplyBtn));

    wxBoxSizer* btns_h_szr = new wxBoxSizer(wxHORIZONTAL);
    btns_h_szr->Add(help_btn, 0, wxALIGN_CENTER_VERTICAL);
    btns_h_szr->AddSpacer(15);
    btns_h_szr->Add(apply_btn, 0, wxALIGN_CENTER_VERTICAL);

    UpdateEstPairs();

    // Arrange above widgets in panel using sizers.
    // Top level panel sizer will be panel_h_szr
    // Below that will be panel_v_szr
    // panel_v_szr will directly receive widgets

    wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
    panel_v_szr->Add(thresh_v_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    panel_v_szr->Add(all_pairs_v_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    panel_v_szr->AddSpacer(5);
    panel_v_szr->Add(rand_samp_v_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    panel_v_szr->AddSpacer(10);
    panel_v_szr->Add(btns_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
    panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
    panel->SetSizer(panel_h_szr);

    // Top Sizer for Frame
    wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);

    SetSizerAndFit(top_h_sizer);

    wxCommandEvent ev;
    if (project->GetNumRecords() > 5000)
        OnRandSampRadioSelected(ev);
    else
        OnAllPairsRadioSelected(ev);

    SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
    Show(true);
    var_choice->SetFocus();
}
