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

#ifndef __GEODA_CENTER_PREFERENCE_DLG_H__
#define __GEODA_CENTER_PREFERENCE_DLG_H__

#include <string>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/hyperlink.h>
#include <wx/xrc/xmlres.h>

#include "../HLStateInt.h"
#include "../DataViewer/TableState.h"
#include "../HighlightStateObserver.h"

////////////////////////////////////////////////////////////////////////////////
//
// PreferenceDlg
//
////////////////////////////////////////////////////////////////////////////////
class PreferenceDlg : public wxDialog
{
public:
    PreferenceDlg(wxWindow* parent,
                  wxWindowID id = wxID_ANY,
                  const wxString& title = _("GeoDa Preference Setup"),
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxSize(580,680));
    
    PreferenceDlg(wxWindow* parent,
                  HLStateInt* highlight_state,
                  TableState* table_state,
                  wxWindowID id = wxID_ANY,
                  const wxString& title = _("GeoDa Preference Setup"),
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxSize(580,680));
 
    static void ReadFromCache();
    
protected:
    HLStateInt* highlight_state;
    TableState* table_state;
    // PREF_USE_CROSSHATCH
    wxCheckBox* cbox0;
    // PREF_SLIDER1_TXT
    wxSlider* slider1;
    wxTextCtrl* slider_txt1;
    // PREF_SLIDER2_TXT
    wxSlider* slider2;
    wxTextCtrl* slider_txt2;
    // basemap auto
    wxComboBox* cmb33;
	// Transparency of highlighted object
    wxSlider* slider6;
    // plot unhighlighted transp
    wxSlider* slider7;
    wxTextCtrl* slider_txt7;
    // crash detect
    wxCheckBox* cbox4;
    // auto upgrade
    wxCheckBox* cbox5;
    // use seed
    wxCheckBox* cbox6;
    // seed value
    wxTextCtrl* txt_seed;
    // show recent
    wxCheckBox* cbox8;
    // show cvs in merge
    wxCheckBox* cbox9;
    // enable High DPI 
    wxCheckBox* cbox10;
    // postgresql
    wxCheckBox* cbox21;
    // sqlite
    wxCheckBox* cbox22;
    // timeout
    wxTextCtrl* txt23;
    // data/tome 
    wxTextCtrl* txt24;
    // displayed decimals
    wxTextCtrl* txt25;
    // stop criterion for auto-weighting
    wxTextCtrl* txt26;
    // cpu cores
    wxCheckBox* cbox18;
    wxTextCtrl* txt_cores;
    // eps of power iteration
    wxTextCtrl* txt_poweriter_eps;
    // lanuage
    wxComboBox* cmb113;
    // gpu
    wxCheckBox* cbox_gpu;
    // transp
    wxCheckBox* cbox26;
    // csvt
    wxCheckBox* cbox_csvt;
    // labels
    wxCheckBox* cbox_lbl;
    wxTextCtrl* txt_lbl_font;
    
    void Init();
    void SetupControls();    
    void OnChooseLanguage(wxCommandEvent& ev);
    void OnCrossHatch(wxCommandEvent& ev);
    void OnSlider1(wxCommandEvent& ev);
    void OnSlider2(wxCommandEvent& ev);
    //void OnSlider3(wxCommandEvent& ev);
    void OnChoice3(wxCommandEvent& ev);
    void OnDisableCrashDetect(wxCommandEvent& ev);
    void OnDisableAutoUpgrade(wxCommandEvent& ev);
    void OnShowRecent(wxCommandEvent& ev);
    void OnShowCsvInMerge(wxCommandEvent& ev);
    void OnEnableHDPISupport(wxCommandEvent& ev);
   
    void OnSlider6(wxCommandEvent& ev);
    void OnSlider7(wxCommandEvent& ev);
    
    void OnHideTablePostGIS(wxCommandEvent& ev);
    void OnHideTableSQLITE(wxCommandEvent& ev);
    void OnTimeoutInput(wxCommandEvent& ev);
    void OnDateTimeInput(wxCommandEvent& ev);
    void OnDisplayDecimal(wxCommandEvent& ev);
    void OnUseSpecifiedSeed(wxCommandEvent& ev);
    void OnSeedEnter(wxCommandEvent& ev);
    void OnAutoWeightStopCriterion(wxCommandEvent& ev);

    void OnSetCPUCores(wxCommandEvent& ev);
    void OnCPUCoresEnter(wxCommandEvent& ev);
   
    void OnPowerEpsEnter(wxCommandEvent& ev);
    void OnUseGPU(wxCommandEvent& ev);
    void OnCreateCSVT(wxCommandEvent& ev);
    void OnEnableTransparencyWin(wxCommandEvent& ev);
    
    void OnDrawLabels(wxCommandEvent& ev);
    void OnLabelFontSizeEnter(wxCommandEvent& ev);
    
    void OnReset(wxCommandEvent& ev);
};

#endif
