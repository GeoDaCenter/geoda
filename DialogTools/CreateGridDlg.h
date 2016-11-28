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

#ifndef __GEODA_CENTER_CREATE_GRID_DLG_H__
#define __GEODA_CENTER_CREATE_GRID_DLG_H__

#include "../ShapeOperations/Box.h"

class CreateGridDlg: public wxDialog
{    
    DECLARE_CLASS( CreateGridDlg )
    DECLARE_EVENT_TABLE()

public:
    CreateGridDlg( );
    CreateGridDlg( wxWindow* parent, wxWindowID id = -1,
				   const wxString& caption = _("Creating Grid"),
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Creating Grid"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    void CreateControls();

    void OnCancelClick( wxCommandEvent& event );
    void OnCReferencefileClick( wxCommandEvent& event );
    void OnCBrowseOfileClick( wxCommandEvent& event );
    void OnCReferencefile2Click( wxCommandEvent& event );
    void OnCEdit1Updated( wxCommandEvent& event );
    void OnCEdit3Updated( wxCommandEvent& event );
    void OnCEdit2Updated( wxCommandEvent& event );
    void OnCEdit4Updated( wxCommandEvent& event );
    void OnCreateClick( wxCommandEvent& event );
    void OnCRadio1Selected( wxCommandEvent& event );
    void OnCRadio2Selected( wxCommandEvent& event );
    void OnCRadio3Selected( wxCommandEvent& event );

    wxTextCtrl* m_outputfile;
    wxTextCtrl* m_inputfile_ascii;
    wxTextCtrl* m_lower_x;
    wxTextCtrl* m_upper_x;
    wxTextCtrl* m_lower_y;
    wxTextCtrl* m_upper_y;
    wxTextCtrl* m_inputfileshp;
    wxTextCtrl* m_rows;
    wxTextCtrl* m_cols;

	void EnableItems();
	bool CheckBBox();
	void CreateGrid();  

	int m_check;

	int	m_nCount;
	int	m_nTimer;
	enum { nMaxCount = 10000 };
	Box	m_BigBox;
	double m_xBot,m_yBot,m_xTop,m_yTop;
	bool hasCreated;

	wxString s_lower_x, s_lower_y, s_top_x, s_top_y;
	wxString s_row, s_col, fn;

	bool isCreated;
};

#endif
