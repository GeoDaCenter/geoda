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

#ifndef __GEODA_CENTER_PCP_DLG_H__
#define __GEODA_CENTER_PCP_DLG_H__

#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include "../VarTools.h"

class Project;
class TableInterface;

class PCPDlg: public wxDialog
{    
public:
	PCPDlg(Project* project, wxWindow* parent,
				 wxWindowID id = wxID_ANY,
				 const wxString& title = _("Parallel Coordinate Plot"),
				 const wxPoint& pos = wxDefaultPosition, 
				 const wxSize& size = wxDefaultSize,
				 long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

	std::vector<int> pcp_col_ids;
	std::vector<int> pcp_col_tm_ids;
	
	std::vector<int> col_ids;
	std::vector<GdaVarTools::VarInfo> var_info;
		
	void CreateControls();
	void Init();
	
	void OnOkClick(wxCommandEvent& ev);
	void OnCancelClick(wxCommandEvent& ev);
	void OnIncAllClick(wxCommandEvent& ev);
	void OnIncOneClick(wxCommandEvent& ev);
	void OnIncListDClick(wxCommandEvent& ev);
	void OnExclAllClick(wxCommandEvent& ev);
	void OnExclOneClick(wxCommandEvent& ev);
	void OnExclListDClick(wxCommandEvent& ev);
	void UpdateOkButton();

protected:
	wxListBox* m_exclude_list;
	wxListBox* m_include_list;
	
	std::map<wxString, int> name_to_id;
	std::map<wxString, int> name_to_tm_id;
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4.  Numeric fields only.
	std::vector<int> col_id_map;
	
	Project* project;
	TableInterface* table_int;
	
	DECLARE_EVENT_TABLE();
};

#endif
