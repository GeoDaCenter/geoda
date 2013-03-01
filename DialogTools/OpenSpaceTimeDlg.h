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

#ifndef __GEODA_CENTER_OPEN_SPACE_TIME_DLG_H__
#define __GEODA_CENTER_OPEN_SPACE_TIME_DLG_H__

#include <vector>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include "../ShapeOperations/DbfFile.h"

class OpenSpaceTimeDlg: public wxDialog
{
public:
    struct ColIdNamePair {
		ColIdNamePair(const int& c, const wxString& n) :col_id(c), name(n) {}
		int col_id;
		wxString name;
	};
    
	OpenSpaceTimeDlg(bool table_only,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize );
    void CreateControls();

    void OnOpenShapefileBtn( wxCommandEvent& event );
	void OnOpenSpaceTableBtn( wxCommandEvent& event );
	void OnOpenTimeTableBtn( wxCommandEvent& event );
	void OnFieldChoice( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
	
	wxTextCtrl* m_open_Shapefile_txt;
	wxTextCtrl* m_time_invariant_table_txt;
	wxTextCtrl* m_time_variant_table_txt;
	wxBitmapButton* m_time_variant_table_btn;
	wxChoice* m_space_id_field;
    wxChoice* m_time_id_field;
	
public:
	wxFileName time_invariant_dbf_name;
	wxFileName time_variant_dbf_name;
	wxString sp_table_space_col_name;
	wxString tm_table_space_col_name;
	int sp_table_space_col;
	int tm_table_space_col;
	wxString tm_table_time_col_name;
	int tm_table_time_col;
	int time_steps;
	std::vector<ColIdNamePair> sp_table_sp_id_map;
	std::vector<ColIdNamePair> tm_table_sp_id_map;
	std::vector<ColIdNamePair> tm_table_tm_id_map;
	
private:
	bool table_only; // true: no Shapefile, false: Shapefile
	void OpenTimeTable(const wxFileName& tbl_fname, bool check_silent);
	bool FindSpTimeFields(const std::vector<DbfFieldDesc>& dbf_fields_sp,
						  const std::vector<DbfFieldDesc>& dbf_fields_tm,
						  std::vector<ColIdNamePair>& sp_table_sp_id_map,
						  std::vector<ColIdNamePair>& tm_table_sp_id_map,
						  std::vector<ColIdNamePair>& tm_table_tm_id_map,
						  int& sp_tbl_col_sp, int& tm_tbl_col_sp,
						  int& tm_tbl_col_tm, bool check_silent);
	
	bool all_init;
	
	DECLARE_EVENT_TABLE()
};

#endif
