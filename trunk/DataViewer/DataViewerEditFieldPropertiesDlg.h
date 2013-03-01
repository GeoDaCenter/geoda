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

#ifndef __GEODA_CENTER_DATA_VIEWER_EDIT_FIELD_PROPERTIES_DLG_H__
#define __GEODA_CENTER_DATA_VIEWER_EDIT_FIELD_PROPERTIES_DLG_H__

#include <map>
#include <vector>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/dialog.h>
#include <wx/button.h>
#include "DbfGridTableBase.h"

class DataViewerEditFieldPropertiesDlg: public wxDialog
{
public:
    DataViewerEditFieldPropertiesDlg(DbfGridTableBase* grid_base,
									 const wxPoint &pos=wxDefaultPosition,
									 const wxSize &size=wxDefaultSize );
    void CreateControls();
    void OnApplyButton( wxCommandEvent& ev );
	void OnCloseButton( wxCommandEvent& ev );
	void OnClose( wxCloseEvent& ev );
	void OnCellEdit( wxGridEvent& ev );
	void OnCellClickLeft( wxGridEvent& ev );
	void OnCellEditorShown( wxGridEvent& ev );
	void OnCellEditorHidden( wxGridEvent& ev );
	void OnLabelLeftClickEvent( wxGridEvent& ev );
	void ShowFieldProperties(int row);
	
	wxGrid* field_grid;
	DbfGridTableBase* grid_base;
	
	wxGridSizer* g_sizer;
	wxStaticText* m_max_label;
	wxStaticText* m_max_val;
	wxStaticText* m_min_label;
	wxStaticText* m_min_val;
	
private:
	std::map<wxString, int> fn_freq;
	wxButton* apply_button;
	bool reenable_apply_after_cell_editor_hidden;
	bool cell_editor_open;
	bool is_space_time;
	
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	std::vector<int> col_id_map;
	
	DECLARE_EVENT_TABLE()
};

#endif
