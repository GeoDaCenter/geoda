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

/*
 EditLowessParams
 */

#ifndef __GEODA_CENTER_LOWESS_PARAM_DLG_H__
#define __GEODA_CENTER_LOWESS_PARAM_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "LowessParamObservable.h"

class Project;

class LowessParamFrame : public wxFrame, public LowessParamObservable
{
public:
	LowessParamFrame(double f, int iter, double delta_factor, Project* project);
	virtual ~LowessParamFrame();
	
	void OnHelpBtn(wxCommandEvent& ev);
	void OnApplyBtn(wxCommandEvent& ev);
	void OnResetDefaultsBtn(wxCommandEvent& ev);

	void OnFTextChange(wxCommandEvent& ev);
	void OnIterTextChange(wxCommandEvent& ev);
	void OnDeltaFactorTextChange(wxCommandEvent& ev);
	
	/** Override LowessParamObservable::closeAndDeleteWhenEmpty */
	virtual void closeAndDeleteWhenEmpty();

private:
	void UpdateParamsFromFields();
	wxString GetHelpPageHtml() const;
	
	Project* project;
	
	wxTextCtrl* f_text;
	wxTextCtrl* iter_text;
	wxTextCtrl* delta_factor_text;
};

#endif
