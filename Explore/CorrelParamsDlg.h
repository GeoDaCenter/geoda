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

#ifndef __GEODA_CENTER_CORREL_PARAMS_DLG_H__
#define __GEODA_CENTER_CORREL_PARAMS_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "CorrelParamsObservable.h"
#include "CorrelogramAlgs.h"

class Project;

class CorrelParamsFrame : public wxFrame, public CorrelParamsObservable
{
public:
	CorrelParamsFrame(const CorrelParams& correl_params,
										GdaVarTools::Manager& var_man,
										Project* project);
	virtual ~CorrelParamsFrame();
	
	void OnHelpBtn(wxCommandEvent& ev);
	void OnApplyBtn(wxCommandEvent& ev);

	void OnVarChoiceSelected(wxCommandEvent& ev);
	void OnAllPairsRadioSelected(wxCommandEvent& ev);
	void OnRandSampRadioSelected(wxCommandEvent& ev);
	void OnBinsTextCtrl(wxCommandEvent& ev);
	void OnBinsSpinEvent(wxSpinEvent& ev);
	void OnDistanceChoiceSelected(wxCommandEvent& ev);
	void OnThreshCheckBox(wxCommandEvent& ev);
	void OnThreshTextCtrl(wxCommandEvent& ev);
	void OnThreshTctrlKillFocus(wxFocusEvent& ev);
	void OnThreshSlider(wxCommandEvent& ev);
	void OnMaxIterTextCtrl(wxCommandEvent& ev);
	void OnMaxIterTctrlKillFocus(wxFocusEvent& ev);
	
	/** Validates variable list against table.
	 New variables are added, order is updated, and missing variables are removed.
	 If any changes to GdaVarTools::Manager are made, a notify event is
	 generated. */
	void UpdateFromTable();
	
	/** Override CorrelParamsObservable::closeAndDeleteWhenEmpty */
	virtual void closeAndDeleteWhenEmpty();
	
private:
	bool IsArc();
	bool IsMi();
	/** min according to current arc/euc and units */
	double GetThreshMin();
	/** max according to current arc/euc and units */
	double GetThreshMax();

	void UpdateVarChoiceFromTable(const wxString& default_var);
	void UpdateApplyState();
	/** update threshold text control value according 
	 to current position of threshold slider, or according to
	 default slider position if slider not yet initialized */
	void UpdateThreshTctrlVal();
	void UpdateEstPairs();
	wxString GetHelpPageHtml() const;
	
	Project* project;

	wxStaticText* var_txt; // ID_VAR_TXT
	wxChoice* var_choice; // ID_VAR_CHOICE
	wxStaticText* dist_txt; // ID_DIST_TXT
	wxChoice* dist_choice; // ID_DIST_CHOICE
	wxStaticText* bins_txt; // ID_BINS_TXT
	wxSpinCtrl* bins_spn_ctrl; // ID_BINS_SPN_CTRL
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
	
	static const long sldr_tcks = 1000;
};

#endif
