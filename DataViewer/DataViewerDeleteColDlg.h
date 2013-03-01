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

#ifndef __GEODA_CENTER_DATA_VIEWER_DELETE_COL_DLG_H__
#define __GEODA_CENTER_DATA_VIEWER_DELETE_COL_DLG_H__

#include <vector>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include "DbfGridTableBase.h"

class DataViewerDeleteColDlg: public wxDialog
{
public:
	DataViewerDeleteColDlg();
	DataViewerDeleteColDlg( DbfGridTableBase* grid_base,
						   wxWindow* parent);
	void CreateControls();
	void OnDelete( wxCommandEvent& ev );
	void OnChoice( wxCommandEvent& ev );
	void InitFieldChoices();
	
	wxButton* m_del_button;
	wxChoice* m_field;
	wxStaticText* m_message;
	DbfGridTableBase* grid_base;

private:
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	std::vector<int> col_id_map;
	
    DECLARE_EVENT_TABLE()
};

#endif

