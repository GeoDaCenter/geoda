#ifndef __DISTANCE_PLOT_VIEW_H__
#define __DISTANCE_PLOT_VIEW_H__

#include "ScatterNewPlotView.h"

class DistancePlotFrame : public ScatterNewPlotFrame
{
    DECLARE_CLASS(DistancePlotFrame)
public:
    DistancePlotFrame(wxFrame *parent, Project* project,
                      const std::vector<double>& X,
                      const std::vector<double>& Y,
                      const std::vector<bool>& X_undef,
                      const std::vector<bool>& Y_undef,
                      double x_min, double x_max,
                      double y_min, double y_max,
                      const wxString& X_label,
                      const wxString& Y_label,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      const long style = wxDEFAULT_FRAME_STYLE);

    virtual ~DistancePlotFrame();

    virtual void OnActivate(wxActivateEvent& event);

    DECLARE_EVENT_TABLE()
};

class DistancePlotDlg : public wxDialog
{
public:
    DistancePlotDlg(wxWindow* parent, Project* project, wxWindowID id=wxID_ANY);

    void OnCancelBtn(wxCommandEvent& ev);
    void OnApplyBtn(wxCommandEvent& ev);

    void OnAllPairsRadioSelected(wxCommandEvent& ev);
    void OnRandSampRadioSelected(wxCommandEvent& ev);
    void OnThreshCheckBox(wxCommandEvent& ev);
    void OnThreshTextCtrl(wxCommandEvent& ev);
    void OnThreshSlider(wxCommandEvent& ev);
    void OnMaxIterTextCtrl(wxCommandEvent& ev);
    void OnMaxIterTctrlKillFocus(wxFocusEvent& ev);
    void OnSeedCheck(wxCommandEvent& ev);
    void OnChangeSeed(wxCommandEvent& ev);

    void UpdateEstPairs();

protected:
    wxCheckBox* thresh_cbx; // ID_THRESH_CBX
    wxTextCtrl* thresh_tctrl; // ID_THRESH_TCTRL
    wxSlider* thresh_slider; // ID_THRESH_SLDR
    wxRadioButton* all_pairs_rad; // ID_ALL_PAIRS_RAD
    wxStaticText* est_pairs_txt; // ID_EST_PAIRS_TXT
    wxStaticText* est_pairs_num_txt; // ID_EST_PAIRS_NUM_TXT
    wxRadioButton* rand_samp_rad; // ID_RAND_SAMP_RAD
    wxStaticText* max_iter_txt; // ID_MAX_ITER_TXT
    wxTextCtrl* max_iter_tctrl; // ID_MAX_ITER_TCTRL

    wxButton* help_btn; // ID_HELP_BTN
    wxButton* apply_btn; // ID_APPLY_BTN

    wxCheckBox* chk_seed;
    wxButton* seedButton;

    static const long sldr_tcks = 1000;
};

#endif
