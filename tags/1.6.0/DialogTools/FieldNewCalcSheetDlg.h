/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_FIELD_NEW_CALC_SHEET_DLG_H__
#define __GEODA_CENTER_FIELD_NEW_CALC_SHEET_DLG_H__

#include <wx/notebook.h>
#include <wx/panel.h>
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"
#include "FieldNewCalcUniDlg.h"

class Project;

class FieldNewCalcSheetDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    FieldNewCalcSheetDlg(Project* project,
						 wxWindow* parent, wxWindowID id = wxID_ANY,
						 const wxString& caption = "FieldCal Container",
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize,
						 long style = wxDEFAULT_DIALOG_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "FieldCal Container",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxDEFAULT_DIALOG_STYLE );

    void CreateControls();

    void OnApplyClick( wxCommandEvent& event );

    wxNotebook* m_note;
	FieldNewCalcSpecialDlg* pSpecial;
	FieldNewCalcBinDlg* pBin;
	FieldNewCalcLagDlg* pLag;
	FieldNewCalcRateDlg* pRate;
	FieldNewCalcUniDlg* pUni;
	Project* project;
};

#endif
