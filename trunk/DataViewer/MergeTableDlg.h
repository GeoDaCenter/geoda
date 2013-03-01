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

#ifndef __GEODA_CENTER_MERGE_TO_TALBE_DLG_H__
#define __GEODA_CENTER_MERGE_TO_TALBE_DLG_H__

#include <set>
#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/grid.h>

class DbfFileReader;
class DbfGridTableBase;

class MergeTableDlg: public wxDialog
{    
public:
    MergeTableDlg(DbfGridTableBase* grid_base,
				  const wxPoint& pos = wxDefaultPosition);
	virtual ~MergeTableDlg();

    void CreateControls();
	void Init();
	void OnKeyValRB( wxCommandEvent& ev );
	void OnRecOrderRB( wxCommandEvent& ev );
	void OnOpenClick( wxCommandEvent& ev );
	void OnIncAllClick( wxCommandEvent& ev );
	void OnIncOneClick( wxCommandEvent& ev );
	void OnIncListDClick(wxCommandEvent& ev );
	void OnExclAllClick( wxCommandEvent& ev );
	void OnExclOneClick( wxCommandEvent& ev );
	void OnExclListDClick(wxCommandEvent& ev );
    void OnMergeClick( wxCommandEvent& ev );
	void OnKeyChoice( wxCommandEvent& ev );
	void OnCloseClick( wxCommandEvent& ev );
	void UpdateMergeButton();
	void RemoveDbfReader();
	void UpdateIncListItems();
	
	wxTextCtrl* m_input_file_name;
	wxRadioButton* m_key_val_rb;
	wxRadioButton* m_rec_order_rb;
	wxChoice* m_current_key;
	wxChoice* m_import_key;
	wxListBox* m_exclude_list;
	wxListBox* m_include_list;
	
	DbfGridTableBase* grid_base;
	DbfFileReader* dbf_reader;
	std::set<wxString> table_fnames;
	
private:
	wxString getValidName(const std::map<wxString,int>& fname_to_id,
						  const std::set<wxString>& table_fnames,
						  const wxString& fname);
	std::map<wxString, int> dedup_to_id;
	std::set<wxString> dups;
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	std::vector<int> col_id_map;
	
	DECLARE_EVENT_TABLE()
};

#endif
