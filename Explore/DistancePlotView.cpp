#include <wx/splitter.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "../Project.h"
#include "../HighlightState.h"
#include"../DataViewer/TableInterface.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../DialogTools/AdjustYAxisDlg.h"
#include "../Algorithms/distanceplot.h"
#include "../Explore/DistancePlotView.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "LoessPlotCanvas.h"
#include "SimpleScatterPlotCanvas.h"
#include "DistancePlotView.h"

IMPLEMENT_CLASS(DistancePlotCanvas, LoessPlotCanvas)
BEGIN_EVENT_TABLE(DistancePlotCanvas, LoessPlotCanvas)
END_EVENT_TABLE()

const int DistancePlotCanvas::default_style = LoessPlotCanvas::DEFAULT_STYLE |
LoessPlotCanvas::show_axes | LoessPlotCanvas::show_lowess_smoother;

DistancePlotCanvas::DistancePlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                       Project* project,
                                       const std::vector<double>& X,
                                       const std::vector<double>& Y,
                                       const std::vector<bool>& X_undef,
                                       const std::vector<bool>& Y_undef,
                                       double x_min, double x_max,
                                       double y_min, double y_max,
                                       const wxString& X_label,
                                       const wxString& Y_label,
                                       const wxPoint& pos,
                                       const wxSize& size)
: LoessPlotCanvas(parent, t_frame, project, project->GetHighlightState(),
                  X, Y, X_undef, Y_undef, X_label, Y_label, default_style,
                  "ID_DISTPLOT_MENU_OPTIONS", "", pos, size),
use_def_y_range(false),prev_y_axis_min(DBL_MIN), prev_y_axis_max(DBL_MAX)
{

}

DistancePlotCanvas::~DistancePlotCanvas()
{

}

void DistancePlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
    wxMenu* optMenu;
    optMenu = wxXmlResource::Get()->LoadMenu("ID_DISTPLOT_MENU_OPTIONS");
    if (!optMenu) return;

    template_frame->UpdateContextMenuItems(optMenu);

    SetCheckMarks(optMenu);
    template_frame->UpdateOptionMenuItems();

    wxMenuItem* save_menu = optMenu->FindItem(XRCID("ID_SAVE_DISTPLOT"));
    Connect(save_menu->GetId(), wxEVT_MENU,  wxCommandEventHandler(DistancePlotCanvas::OnSaveResult));

    wxMenuItem* toggle_menu = optMenu->FindItem(XRCID("ID_DISTPLOT_SHOW_POINTS"));
    Connect(toggle_menu->GetId(), wxEVT_MENU,  wxCommandEventHandler(DistancePlotCanvas::OnToggleDataPoints));

    //Connect(XRCID("ID_DISTPLOT_SHOW_CONFIDENCE_INTERVAL"), wxEVT_MENU,  wxCommandEventHandler(DistancePlotCanvas::OnToggleConfidenceInterval));

    Connect(XRCID("ID_EDIT_LOESS_PARAMS"),  wxEVT_MENU,  wxCommandEventHandler(DistancePlotCanvas::OnEditLoess));

    Connect(XRCID("ID_USE_ADJUST_Y_AXIS"),  wxEVT_MENU,  wxCommandEventHandler(DistancePlotCanvas::OnUseAdjustYAxis));
    Connect(XRCID("ID_ADJUST_Y_AXIS"), wxEVT_MENU, wxCommandEventHandler(DistancePlotCanvas::OnAdjustYAxis));

    PopupMenu(optMenu, pos);
}

void DistancePlotCanvas::OnUseAdjustYAxis(wxCommandEvent& event)
{
    if (use_def_y_range == false) {
        use_def_y_range = true;
        OnAdjustYAxis(event);

    } else {
        use_def_y_range = false;
        def_y_min = "";
        def_y_max = "";
        invalidateBms();
        PopulateCanvas();
        Refresh();
    }
}

void DistancePlotCanvas::OnAdjustYAxis(wxCommandEvent& event)
{
    double y_axis_min = axis_scale_y.scale_min;
    double y_axis_max = axis_scale_y.scale_max;

    AdjustYAxisDlg dlg(y_axis_min, y_axis_max, this);
    if (dlg.ShowModal () != wxID_OK)
        return;

    def_y_min = dlg.s_min_val;
    def_y_max = dlg.s_max_val;

    invalidateBms();
    PopulateCanvas();
    Refresh();
}

void DistancePlotCanvas::OnSaveResult(wxCommandEvent& event)
{
    wxString wildcard = _("CSV files (*.csv)|*.csv");
    wxString defaultFile(project->GetProjectTitle());
    defaultFile += "_dist.csv";

    wxString working_dir = project->GetWorkingDir().GetPath();
    wxFileDialog dlg(this, _("Save to a csv file."),
                     working_dir, defaultFile, wildcard,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    wxString ofn;
    if (dlg.ShowModal() != wxID_OK) return;
    wxFile file( dlg.GetPath(), wxFile::write );
    if( file.IsOpened() ) {
        file.Write( "geo_distance, var_distance\n");
        for (size_t i=0; i< X.size(); ++i) {
            file.Write(wxString::Format("%f,%f\n", X[i], Y[i]));
        }
        file.Close();
    }
}

void DistancePlotCanvas::OnToggleDataPoints(wxCommandEvent& event)
{
    if (style & show_data_points) {
        style = style & (~show_data_points);
    } else {
        style = style | show_data_points;
    }
    PopulateCanvas();
}

void DistancePlotCanvas::OnToggleConfidenceInterval(wxCommandEvent& event)
{
    if (style & show_confidence_interval) {
        style = style & (~show_confidence_interval);
    } else {
        style = style | show_confidence_interval;
    }
    PopulateCanvas();
}


void DistancePlotCanvas::SetCheckMarks(wxMenu* menu)
{
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISTPLOT_SHOW_POINTS"), style & show_data_points);
    //GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISTPLOT_SHOW_CONFIDENCE_INTERVAL"), style & show_confidence_interval);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_ADJUST_Y_AXIS"), use_def_y_range);
}

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
                                     const wxString& title,
                                     const wxPoint& pos, const wxSize& size,
                                     const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
    int width, height;
    GetClientSize(&width, &height);
    wxSplitterWindow* splitter_win = 0;

    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    SetBackgroundColour(*wxWHITE);
    panel->Bind(wxEVT_MOTION, &DistancePlotFrame::OnMouseEvent, this);

    template_canvas = new DistancePlotCanvas(panel, this, project, X, Y,
                                             X_undef, Y_undef,
                                             x_min, x_max, y_min, y_max, X_label,
                                             Y_label);

    wxBoxSizer* bag_szr = new wxBoxSizer(wxHORIZONTAL);
    bag_szr->Add(template_canvas, 1, wxEXPAND);

    wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
    panel_v_szr->Add(bag_szr, 1, wxEXPAND);

    panel->SetSizer(panel_v_szr);

    wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);

    SetSizer(top_h_sizer);

    //template_canvas->SetScrollRate(1,1);
    DisplayStatusBar(true);

    Show(true);
}

DistancePlotFrame::~DistancePlotFrame()
{
    DeregisterAsActive();
}

void DistancePlotFrame::OnMouseEvent(wxMouseEvent& event)
{
    if (event.RightDown()) {
        const wxPoint& pos = event.GetPosition();
        // Workaround for right-click not changing window focus in OSX / wxW 3.0
        wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
        OnActivate(ae);

        OnRightClick(pos);
    }
}

void DistancePlotFrame::OnActivate(wxActivateEvent& event)
{
    if (event.GetActive()) {
        RegisterAsActive("DistancePlotFrame", GetTitle());
    }
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void DistancePlotFrame::OnRightClick(const wxPoint& pos)
{
    wxMenu* optMenu;
    optMenu = wxXmlResource::Get()->LoadMenu("ID_DISTPLOT_MENU_OPTIONS");
    if (!optMenu) return;

    UpdateContextMenuItems(optMenu);

    PopupMenu(optMenu, pos);
    UpdateOptionMenuItems();

    wxMenuItem* save_menu = optMenu->FindItem(XRCID("ID_SAVE_DISTPLOT"));
    //Connect(save_menu->GetId(), wxEVT_MENU,  wxCommandEventHandler(CorrelogramFrame::OnSaveResult));
}

void DistancePlotFrame::UpdateOptionMenuItems()
{
    //TemplateFrame::UpdateOptionMenuItems(); // set common items first
    wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
    int menu = mb->FindMenu(_("Options"));
    if (menu != wxNOT_FOUND) {
        DistancePlotFrame::UpdateContextMenuItems(mb->GetMenu(menu));
        ((DistancePlotCanvas*)template_canvas)->SetCheckMarks(mb->GetMenu(menu));
    }
}

void DistancePlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
    // Update the checkmarks and enable/disable state for the
    // following menu items if they were specified for this particular
    // view in the xrc file.  Items that cannot be enable/disabled,
    // or are not checkable do not appear.
    TemplateFrame::UpdateContextMenuItems(menu); // set common items
}


///////////////////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE( DistancePlotDlg, wxDialog )
EVT_CLOSE( DistancePlotDlg::OnClose )
END_EVENT_TABLE()

DistancePlotDlg::DistancePlotDlg(wxFrame* parent, Project* project)
: AbstractClusterDlg(parent, project,  _("Distance Scatter Plot"))
{
    CreateControls();
}

DistancePlotDlg::~DistancePlotDlg()
{

}

void DistancePlotDlg::CreateControls()
{
    wxPanel* panel = new wxPanel(this);
    //panel->SetBackgroundColour(*wxWHITE);

    // label & listbox
    wxStaticText* var_st = new wxStaticText (panel, wxID_ANY,
                                         _("Select Variables (Multi-Selection)"),
                                         wxDefaultPosition, wxDefaultSize);

    combo_var = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(320,200), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);

    // variable distance
    vardist_txt = new wxStaticText(panel, XRCID("ID_VARDIST_TXT"), _("Variable Distance:"));
    vardist_choice = new wxChoice(panel, XRCID("ID_VARDIST_CHOICE"),
                               wxDefaultPosition, wxSize(160,-1));
    vardist_choice->Append(_("Euclidean Distance"));
    vardist_choice->Append(_("Manhatten Distance"));
	vardist_choice->SetSelection(0);

    wxBoxSizer* vardist_h_szr = new wxBoxSizer(wxHORIZONTAL);
    vardist_h_szr->Add(vardist_txt, 0, wxALIGN_CENTER_VERTICAL);
    vardist_h_szr->AddSpacer(5);
    vardist_h_szr->Add(vardist_choice, 0, wxALIGN_CENTER_VERTICAL);

    dist_txt = new wxStaticText(panel, XRCID("ID_DIST_TXT"), _("Geographic Distance:"));
    dist_choice = new wxChoice(panel, XRCID("ID_DIST_CHOICE"),
                               wxDefaultPosition, wxSize(160,-1));
    dist_choice->Append(_("Euclidean Distance"));
    dist_choice->Append(_("Arc Distance (mi)"));
    dist_choice->Append(_("Arc Distance (km)"));
	dist_choice->SetSelection(0);

    wxBoxSizer* dist_h_szr = new wxBoxSizer(wxHORIZONTAL);
    dist_h_szr->Add(dist_txt, 0, wxALIGN_CENTER_VERTICAL);
    dist_h_szr->AddSpacer(5);
    dist_h_szr->Add(dist_choice, 0, wxALIGN_CENTER_VERTICAL);

    Connect(XRCID("ID_DIST_CHOICE"), wxEVT_CHOICE,
            wxCommandEventHandler(DistancePlotDlg::OnDistanceChoiceSelected));

    // distance
    thresh_cbx = new wxCheckBox(panel, XRCID("ID_THRESH_CBX"), _("Max Distance:"));
    thresh_cbx->SetValue(true);
    maxdist_choice = new wxChoice(panel, XRCID("ID_MAXDIST_CHOICE"),
                               wxDefaultPosition, wxSize(160,-1));
    maxdist_choice->Append(_("Maximum pairwise distance"));
    maxdist_choice->Append(_("1/2 diagonal of bounding box"));

    size_t nobs = project->GetNumRecords();
    if (nobs < 10000) maxdist_choice->SetSelection(0);
    else maxdist_choice->SetSelection(1);

    maxdist_choice->Bind(wxEVT_CHOICE, &DistancePlotDlg::OnMaxDistMethodChoice, this);
    Connect(XRCID("ID_THRESH_CBX"), wxEVT_CHECKBOX,
            wxCommandEventHandler(DistancePlotDlg::OnThreshCheckBox));
    Connect(XRCID("ID_THRESH_TCTRL"), wxEVT_TEXT_ENTER,
            wxCommandEventHandler(DistancePlotDlg::OnThreshTextCtrl));

    wxBoxSizer* thresh_h_szr = new wxBoxSizer(wxHORIZONTAL);
    thresh_h_szr->Add(thresh_cbx, 0, wxALIGN_CENTER_VERTICAL);
    thresh_h_szr->AddSpacer(5);
    thresh_h_szr->Add(maxdist_choice, 0, wxALIGN_CENTER_VERTICAL);


    thresh_tctrl = new wxTextCtrl(panel, XRCID("ID_THRESH_TCTRL"), "",
                                  wxDefaultPosition, wxSize(100,-1),
                                  wxTE_PROCESS_ENTER);
    thresh_tctrl->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    thresh_tctrl->Enable(false);
    thresh_tctrl->Bind(wxEVT_TEXT_ENTER, &DistancePlotDlg::OnMaxDistanceTextCtrl, this);
    
    thresh_slider = new wxSlider(panel, XRCID("ID_THRESH_SLDR"),
                                 sldr_tcks/2, 0, sldr_tcks,
                                 wxDefaultPosition, wxSize(180,-1));
    Connect(XRCID("ID_THRESH_SLDR"), wxEVT_SLIDER,
            wxCommandEventHandler(DistancePlotDlg::OnThreshSlider));
    thresh_slider->Enable(false);
    
    UpdateThreshTctrlVal();

    wxBoxSizer* thresh_sld_h_szr = new wxBoxSizer(wxHORIZONTAL);
    thresh_sld_h_szr->AddSpacer(18);
    thresh_sld_h_szr->Add(thresh_slider, 0, wxALIGN_CENTER_VERTICAL);
    thresh_sld_h_szr->Add(thresh_tctrl, 0, wxALIGN_CENTER_VERTICAL);

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

    est_pairs_txt = new wxStaticText(panel, XRCID("ID_EST_PAIRS_TXT"),
                                     _("Estimated Pairs:"));
    est_pairs_num_txt = new wxStaticText(panel, XRCID("ID_EST_PAIRS_NUM_TXT"),
                                         "4,000,000");
    wxBoxSizer* est_pairs_h_szr = new wxBoxSizer(wxHORIZONTAL);
    est_pairs_h_szr->Add(est_pairs_txt, 0, wxALIGN_CENTER_VERTICAL);
    est_pairs_h_szr->AddSpacer(5);
    est_pairs_h_szr->Add(est_pairs_num_txt, 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* all_pairs_v_szr = new wxBoxSizer(wxVERTICAL);
    all_pairs_v_szr->Add(all_pairs_rad);
    all_pairs_v_szr->AddSpacer(2);
    all_pairs_v_szr->Add(est_pairs_h_szr, 0, wxLEFT, 18);

    rand_samp_rad = new wxRadioButton(panel, XRCID("ID_RAND_SAMP_RAD"),
                                      _("Random Sample"), wxDefaultPosition,
                                      wxDefaultSize, wxALIGN_CENTER_VERTICAL);
    rand_samp_rad->SetValue(false);
    Connect(XRCID("ID_RAND_SAMP_RAD"), wxEVT_RADIOBUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnRandSampRadioSelected));
    max_iter_txt = new wxStaticText(panel, XRCID("ID_MAX_ITER_TXT"),
                                    _("Sample Size:"));
    {
        size_t max_pairs = (nobs*(nobs-1))/2;

        wxString max_iterations;
        if (max_pairs > 1000000)
            max_iterations = "1000000";
        else
            max_iterations << max_pairs;
        max_iter_tctrl = new wxTextCtrl(panel, XRCID("ID_MAX_ITER_TCTRL"),
                                        max_iterations, wxDefaultPosition,
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
        wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Use Specified Seed:"),
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
    rand_samp_v_szr->Add(rand_samp_rad,0, wxLEFT, 2);
    rand_samp_v_szr->AddSpacer(2);
    rand_samp_v_szr->Add(max_iter_h_szr, 0, wxLEFT, 12);
    rand_samp_v_szr->Add(random_opt_h_szr, 0, wxLEFT, 12);

    help_btn = new wxButton(panel, XRCID("ID_CANCEL_BTN"), _("Cancel"),
                            wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    apply_btn = new wxButton(panel, XRCID("ID_APPLY_BTN"), _("Apply"),
                             wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    Connect(XRCID("ID_CANCEL_BTN"), wxEVT_BUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnCancelBtn));
    Connect(XRCID("ID_APPLY_BTN"), wxEVT_BUTTON,
            wxCommandEventHandler(DistancePlotDlg::OnOK));

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
    panel_v_szr->Add(var_st, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    panel_v_szr->Add(combo_var, 1, wxALL|wxEXPAND, 5);
    panel_v_szr->AddSpacer(2);
    panel_v_szr->Add(vardist_h_szr, 0, wxALIGN_LEFT|wxALL, 5);
    panel_v_szr->AddSpacer(5);
    panel_v_szr->Add(dist_h_szr, 0, wxALIGN_LEFT|wxALL, 5);
    panel_v_szr->AddSpacer(2);
    panel_v_szr->Add(thresh_v_szr, 0, wxALIGN_LEFT|wxALL, 5);
    panel_v_szr->Add(all_pairs_v_szr, 0, wxALIGN_LEFT|wxALL, 5);
    panel_v_szr->AddSpacer(5);
    panel_v_szr->Add(rand_samp_v_szr, 0, wxALIGN_LEFT|wxALL, 5);
    panel_v_szr->AddSpacer(10);
    panel_v_szr->Add(btns_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
    panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
    panel->SetSizer(panel_h_szr);

    // Top Sizer for Frame
    wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);

    SetSizerAndFit(top_h_sizer);

    InitVariableCombobox(combo_var);

    wxCommandEvent ev;
    if (project->GetNumRecords() > 5000)
        OnRandSampRadioSelected(ev);
    else
        OnAllPairsRadioSelected(ev);
	OnThreshCheckBox(ev);

    Centre();
}

void DistancePlotDlg::InitVariableCombobox(wxListBox* var_box)
{
    var_box->Clear();
    wxArrayString var_items;

    std::vector<int> col_id_map;
    TableInterface* table_int = project->GetTableInt();
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
                var_items.Add(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            var_items.Add(name);
        }
    }
    if (!var_items.IsEmpty()) {
        var_box->InsertItems(var_items,0);
    }
}

void DistancePlotDlg::GetSelectedVariables(wxListBox* var_box,
                                           std::vector<std::vector<double> >& data,
                                           std::vector<std::vector<bool> >& data_undefs)
{
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    int num_var = (int)selections.size();

    data.resize(num_var);
    data_undefs.resize(num_var);

    TableInterface* table_int = project->GetTableInt();

    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString sel_str = combo_var->GetString(idx);
        //select_vars.push_back(sel_str);
        wxString nm = name_to_nm[sel_str];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table. Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, _("Error"),
                                wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        int tm = name_to_tm_id[combo_var->GetString(idx)];
        table_int->GetColData(col, tm, data[i]);
        table_int->GetColUndefined(col, tm, data_undefs[i]);
        GenUtils::StandardizeData(data[i], data_undefs[i]);
    }
}

wxString DistancePlotDlg::_printConfiguration()
{
    return "";
}


void DistancePlotDlg::OnCancelBtn(wxCommandEvent &ev)
{
    ev.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void DistancePlotDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close DistancePlotDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void DistancePlotDlg::OnOK(wxCommandEvent &ev)
{
    bool is_all_pairs = all_pairs_rad->GetValue();
    bool is_threshold = thresh_cbx->GetValue();
    bool reuse_last_seed = chk_seed->GetValue();
    uint64_t last_seed_used = GdaConst::gda_user_seed;

    const std::vector<GdaPoint*>& centroids = project->GetCentroids();
    std::vector<std::vector<double> > data;
    std::vector<std::vector<bool> > data_undefs;
    GetSelectedVariables(combo_var, data, data_undefs);

    if (data.size() == 0) {
        wxString err_msg = _("Please select more than one variable.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    wxString val = thresh_tctrl->GetValue();
    val.Trim(false);
    val.Trim(true);
    double thres;
    bool is_valid = val.ToDouble(&thres);
    if (is_threshold && !is_valid) {
        wxString err_msg = _("Please input a valid numeric value for max distance.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    val = max_iter_tctrl->GetValue();
    val.Trim(false);
    val.Trim(true);
    long num_rand;
    is_valid = val.ToLong(&num_rand);
    if (!is_all_pairs && !is_valid) {
        wxString err_msg = _("Please input a valid numeric value for sample size.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"),
                            wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    char vardist_method = vardist_choice->GetSelection()==0 ? 'e' : 'm';

    distplot = new DistancePlot(centroids, data, data_undefs, vardist_method,
                          IsArc(), IsMi(), last_seed_used, reuse_last_seed);

    str_threshold = "";
    title = _("Distance Scatter Plot: ");
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    for (size_t i=0; i<selections.size(); i++) {
        wxString sel_str = combo_var->GetString(selections[i]);
        title << sel_str;
        if (i <selections.size()-1) title << "/";
    }

    if (is_all_pairs) {
        if (!is_threshold) {
            // all pairs
            distplot->run();
        } else {
            // threshold
            str_threshold << thres;
            if (dist_choice->GetSelection()==0) {
                rtree_pt_2d_t& rtree = project->GetEucPlaneRtree();
                distplot->run(rtree, thres);
            } else {
                rtree_pt_3d_t& rtree = project->GetUnitSphereRtree();
                distplot->run(rtree, thres);
            }
        }

    } else {
        // random
        distplot->run(true, num_rand);
    }

    const std::vector<double>& X = distplot->GetX();
    const std::vector<double>& Y = distplot->GetY();
    const std::vector<bool>& X_undefs = distplot->GetXUndefs();
    const std::vector<bool>& Y_undefs = distplot->GetYUndefs();
    double x_min = distplot->GetMinX();
    double x_max = distplot->GetMaxX();
    double y_min = distplot->GetMinY();
    double y_max = distplot->GetMaxY();

    wxString X_label = _("Geographical distance");
    if (!str_threshold.IsEmpty())
        X_label << ": " << _("threshold") << "=" << str_threshold;
    if (distplot->IsArc() && distplot->IsMile())
        X_label << " (" << "mile" << ")";
    else if (distplot->IsArc() && !distplot->IsMile())
        X_label << " (" << "km" << ")";
    wxString Y_label = _("Variable distance");
    if (distplot->DistMethod() == 'e')
        Y_label << " (" << "Euclidean" << ")";
    else if (distplot->DistMethod() == 'm')
        Y_label << " (" << "Manhattan" << ")";

    wxString win_title = title;
    win_title << " (" << X.size() << " " << "data points" << ")";
    DistancePlotFrame* f = new DistancePlotFrame(parent, project,
                                                 X, Y, X_undefs, Y_undefs,
                                                 x_min, x_max,
                                                 y_min, y_max,
                                                 X_label, Y_label, win_title);
    delete distplot;
}

DistancePlot* DistancePlotDlg::GetDistancePlot()
{
    return distplot;
}

void DistancePlotDlg::OnDistanceChoiceSelected(wxCommandEvent& ev)
{
    UpdateThreshTctrlVal();
    ev.Skip();
}

void DistancePlotDlg::OnAllPairsRadioSelected(wxCommandEvent &ev) {
    all_pairs_rad->SetValue(true);
    est_pairs_txt->Enable(true);
    est_pairs_num_txt->Enable(true);
    rand_samp_rad->SetValue(false);
    max_iter_txt->Enable(false);
    max_iter_tctrl->Enable(false);
}

void DistancePlotDlg::OnRandSampRadioSelected(wxCommandEvent &ev) {
    all_pairs_rad->SetValue(false);
    est_pairs_txt->Enable(false);
    est_pairs_num_txt->Enable(false);
    rand_samp_rad->SetValue(true);
    max_iter_txt->Enable(true);
    max_iter_tctrl->Enable(true);
}

void DistancePlotDlg::OnThreshCheckBox(wxCommandEvent &ev) {
    bool checked = thresh_cbx->GetValue();
    thresh_tctrl->Enable(checked);
    thresh_slider->Enable(checked);
    UpdateEstPairs();
    UpdateThreshTctrlVal();
    ev.Skip();
}

void DistancePlotDlg::OnMaxDistMethodChoice(wxCommandEvent &ev) {
    UpdateEstPairs();
    UpdateThreshTctrlVal();
    ev.Skip();
}

void DistancePlotDlg::OnThreshTextCtrl(wxCommandEvent &ev)
{
    wxString val = thresh_tctrl->GetValue();
    val.Trim(false);
    val.Trim(true);
    double t;
    bool is_valid = val.ToDouble(&t);
    if (is_valid) {
        if (t <= GetThreshMin()) {
            thresh_slider->SetValue(0);
        } else if (t >= GetThreshMax()) {
            thresh_slider->SetValue(sldr_tcks);
        } else {
            double s = ((t-GetThreshMin())/(GetThreshMax()-GetThreshMin()) *
                        ((double) sldr_tcks));
            thresh_slider->SetValue((int) s);
        }
    } else {
        UpdateThreshTctrlVal();
    }
}

void DistancePlotDlg::OnThreshSlider(wxCommandEvent &ev) {
    UpdateThreshTctrlVal();
    if (thresh_cbx && thresh_cbx->GetValue()) UpdateEstPairs();
}

void DistancePlotDlg::OnMaxIterTextCtrl(wxCommandEvent &ev) {
    wxString val = max_iter_tctrl->GetValue();
    val.Trim(false);
    val.Trim(true);
    long t;
    bool is_valid = val.ToLong(&t);
    if (is_valid) {
        if (t < 10) { // cannot be less than 10
            wxString s;
            s << 10;
            max_iter_tctrl->ChangeValue(s);
        } else if (t > 1000000000) { // 1billion
            wxString s;
            s << 1000000000;
            max_iter_tctrl->ChangeValue(s);
        }
    } else {
        wxString s;
        s << 1000000; // 1million
        max_iter_tctrl->ChangeValue(s);
    }

}

void DistancePlotDlg::OnMaxDistanceTextCtrl(wxCommandEvent &ev) {
    wxString val = thresh_tctrl->GetValue();
    val.Trim(false);
    val.Trim(true);
    long t;
    bool is_valid = val.ToLong(&t);
    if (is_valid) {
        double s = ((t-GetThreshMin())/(GetThreshMax()-GetThreshMin()) *
                    ((double) sldr_tcks));
        thresh_slider->SetValue((int) s);
    }
}

void DistancePlotDlg::OnMaxIterTctrlKillFocus(wxFocusEvent &ev) {
    wxString val = max_iter_tctrl->GetValue();
    val.Trim(false);
    val.Trim(true);
    long t;
    bool is_valid = val.ToLong(&t);
    if (is_valid) {
        if (t < 10) {
            wxString s;
            s << 10;
            max_iter_tctrl->ChangeValue(s);
        } else if (t > 1000000000) {
            wxString s;
            s << 1000000000;
            max_iter_tctrl->ChangeValue(s);
        }
    } else {
        wxString s;
        s << 1000000;
        max_iter_tctrl->ChangeValue(s);
    }
}

void DistancePlotDlg::OnSeedCheck(wxCommandEvent &event) {
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
        seedButton->Disable();
    }
}

void DistancePlotDlg::OnChangeSeed(wxCommandEvent &ev) {
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

void DistancePlotDlg::UpdateEstPairs() {
    wxString s;
    size_t nobs = project->GetNumRecords();
    size_t max_pairs = (nobs*(nobs-1))/2;
    if (thresh_cbx->GetValue()) {
        double sf = 0.5;
        if (thresh_slider) {
            sf = (((double) thresh_slider->GetValue()) / ((double) sldr_tcks));
        }
        double mn = (double) nobs;
        double mx = (double) max_pairs;
        double est = mn + (mx-mn)*sf;
        long est_l = (long) est;
        s << est_l;
    } else {
        s << max_pairs;
    }
    est_pairs_num_txt->SetLabelText(s);
}

void DistancePlotDlg::UpdateThreshTctrlVal()
{
    if (!thresh_tctrl) return;
    if (thresh_cbx->GetValue()==false) {
        thresh_tctrl->ChangeValue("");
        return;
    }
    double sf = 0.5;
    if (thresh_slider) {
        sf = (((double) thresh_slider->GetValue()) / ((double) sldr_tcks));
    }
    wxString s;
    double v = GetThreshMin() + (GetThreshMax() - GetThreshMin())*sf;
    s << v;
    thresh_tctrl->ChangeValue(s);
}

bool DistancePlotDlg::IsArc()
{
    return dist_choice->GetSelection() > 0;
}

bool DistancePlotDlg::IsMi()
{
    return dist_choice->GetSelection() == 1;
}

double DistancePlotDlg::GetThreshMin()
{
    if (IsArc()) {
        double r = project->GetMin1nnDistArc();
        if (IsMi()) return GenGeomAlgs::EarthRadToMi(r);
        return GenGeomAlgs::EarthRadToKm(r);
    }
    return project->GetMin1nnDistEuc();
}

double DistancePlotDlg::GetThreshMax()
{
    if (maxdist_choice->GetSelection() == 0) {
        if (IsArc()) {
            double r = project->GetMaxDistArc();
            if (IsMi()) return GenGeomAlgs::EarthRadToMi(r);
            return GenGeomAlgs::EarthRadToKm(r);
        }
        return project->GetMaxDistEuc();
    } else {
        // using bbox diagonal
        double minx, miny, maxx, maxy;
        project->GetMapExtent(minx, miny, maxx, maxy);
        double xx = (maxx - minx);
        double yy = (maxy - miny);
        double r = sqrt(xx * xx + yy * yy) / 2.0;

        if (IsArc()) {
            if (IsMi()) return GenGeomAlgs::EarthRadToMi(r);
            return GenGeomAlgs::EarthRadToKm(r);
        } else
            return r;
    }
}





