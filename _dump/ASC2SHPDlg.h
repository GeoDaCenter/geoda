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

#ifndef __GEODA_CENTER_ASC2SHP_DLG_H__
#define __GEODA_CENTER_ASC2SHP_DLG_H__

class ASC2SHPDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    ASC2SHPDlg( );
    ASC2SHPDlg( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Convert ASCII to SHP"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Convert ASCII to SHP"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
    void CreateControls();

    void OnOkAddClick( wxCommandEvent& event );
    void OnCOpenIascClick( wxCommandEvent& event );
    void OnCOpenOshpClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    wxTextCtrl* m_inputfile;
    wxTextCtrl* m_outputfile;
    wxChoice* m_X;
    wxChoice* m_Y;

	wxString fn;
};

#endif

