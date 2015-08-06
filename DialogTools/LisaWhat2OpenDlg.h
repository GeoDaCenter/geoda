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

#ifndef __GEODA_CENTER_LISA_WHAT_2_OPEN_DLG_H__
#define __GEODA_CENTER_LISA_WHAT_2_OPEN_DLG_H__

class LisaWhat2OpenDlg: public wxDialog
{    
    DECLARE_CLASS( LisaWhat2OpenDlg )
    DECLARE_EVENT_TABLE()

public:
    LisaWhat2OpenDlg( wxWindow* parent, wxWindowID id = -1,
					 const wxString& caption = "What windows to open?",
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize,
					 long style = wxCAPTION|wxSYSTEM_MENU );
    void CreateControls();
    void OnOkClick( wxCommandEvent& event );

    wxCheckBox* m_check1;
    wxCheckBox* m_check2;
    wxCheckBox* m_check3;

	bool m_SigMap;
	bool m_ClustMap;
	bool m_Moran;
};

#endif
