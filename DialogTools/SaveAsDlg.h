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
#ifndef __GEODA_CENTER_SAVE_AS_PROJECT_DLG_H__
#define __GEODA_CENTER_SAVE_AS_PROJECT_DLG_H__


#include <vector>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>

#include "../Project.h"

class SaveAsDlg: public wxDialog 
{
public:
	SaveAsDlg(wxWindow* parent,
		      Project* project,
              wxWindowID id = wxID_ANY,
              const wxString& title = "Save Project File As...",
			  const wxPoint& pos = wxDefaultPosition,
			  const wxSize& size = wxDefaultSize );
    void OnOkClick( wxCommandEvent& event );
	
private:
	wxCheckBox* m_chk_create_datasource;
	wxCheckBox* m_chk_create_project;
    wxTextCtrl* m_project_path_txt;
    wxTextCtrl* m_datasource_path_txt;
    wxBitmapButton* m_browse_project_btn;
    wxBitmapButton* m_browse_datasource_btn;

	Project* project_p;
    GdaConst::DataSourceType ds_type;
    bool is_create_project;
    bool is_create_datasource;
    wxFileName proj_file_path;
    wxFileName ds_file_path;
   
    void OnProjectCheck( wxCommandEvent& event );
    void OnDatasourceCheck( wxCommandEvent& event );
    void OnBrowseProjectFileBtn( wxCommandEvent& event );
    void OnBrowseDatasourceBtn( wxCommandEvent& event );
    void OnOKClick( wxCommandEvent& event );
 
	DECLARE_EVENT_TABLE()
};

#endif
