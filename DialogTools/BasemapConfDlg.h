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
#ifndef __GEODA_CENTER_BASEMAP_CONF_DLG_H__
#define __GEODA_CENTER_BASEMAP_CONF_DLG_H__


#include <vector>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>

#include "../Project.h"

class Project;

class BasemapConfDlg: public wxDialog
{
public:
    BasemapConfDlg(wxWindow* parent, Project* p,
               wxWindowID id = wxID_ANY,
               const wxString& title = "Basemap Configuration Dialog",
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize );
    
private:
    Project* p;
    wxTextCtrl* m_txt_nokia_uname;
    wxTextCtrl* m_txt_nokia_key;

    
    void OnOkClick( wxCommandEvent& event );
    void OnResetClick( wxCommandEvent& event );
    
    DECLARE_EVENT_TABLE()
};

#endif
