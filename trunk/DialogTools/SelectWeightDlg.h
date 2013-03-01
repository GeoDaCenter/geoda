/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_SELECT_WEIGHT_DLG_H__
#define __GEODA_CENTER_SELECT_WEIGHT_DLG_H__

#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>

class Project;
class WeightsManager;
class DbfGridTableBase;

class SelectWeightDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
	SelectWeightDlg(Project* project,
					wxWindow* parent,
					wxWindowID id = -1,
					const wxString& caption = "Select Weights",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxCAPTION|wxSYSTEM_MENU );

    bool Create(wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Select Weights",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();

    void OnCRadioOpenweight1Selected( wxCommandEvent& event );
    void OnCRadioOpenweight2Selected( wxCommandEvent& event );
    void OnCOpenFileweightClick( wxCommandEvent& event );
    void OnCCreateWeightsClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );

	void PumpingWeight();

    wxRadioButton* m_radio1;
	wxChoice* m_weights;
	wxRadioButton* m_radio2;
    wxTextCtrl* m_field;
    wxCheckBox* m_checkButton;

	Project* project;
	WeightsManager* w_manager;
	DbfGridTableBase* grid_base;
	wxString m_wfile_fpath; // weights file name with full path
};

#endif
