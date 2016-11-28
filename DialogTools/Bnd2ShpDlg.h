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

#ifndef __GEODA_CENTER_BND2SHP_DLG_H__
#define __GEODA_CENTER_BND2SHP_DLG_H__

class Bnd2ShpDlg: public wxDialog
{    
    DECLARE_CLASS( Bnd2ShpDlg )
    DECLARE_EVENT_TABLE()

public:
    Bnd2ShpDlg( );
    Bnd2ShpDlg( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Convert Boundary to SHP"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Convert Boundary to SHP"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    void CreateControls();

    void OnCreateClick( wxCommandEvent& event );
    void OnCOpenIascClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    wxTextCtrl* m_inputfile;


	wxString fn;
};

#endif
