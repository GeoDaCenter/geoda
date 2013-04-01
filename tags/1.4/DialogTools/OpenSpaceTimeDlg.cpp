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

#include <fstream>
#include <wx/filedlg.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../Project.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "OpenSpaceTimeDlg.h"

BEGIN_EVENT_TABLE( OpenSpaceTimeDlg, wxDialog )
	EVT_CHOICE( XRCID("ID_SPACE_ID_FIELD"), OpenSpaceTimeDlg::OnFieldChoice )
	EVT_CHOICE( XRCID("ID_TIME_ID_FIELD"), OpenSpaceTimeDlg::OnFieldChoice )
    EVT_BUTTON( XRCID("ID_OPEN_SHAPEFILE_BTN"),
			   OpenSpaceTimeDlg::OnOpenShapefileBtn )
	EVT_BUTTON( XRCID("ID_TIME_INVARIANT_TABLE_BTN"),
			   OpenSpaceTimeDlg::OnOpenSpaceTableBtn )
	EVT_BUTTON( XRCID("ID_TIME_VARIANT_TABLE_BTN"),
			   OpenSpaceTimeDlg::OnOpenTimeTableBtn )
    EVT_BUTTON( wxID_OK, OpenSpaceTimeDlg::OnOkClick )
END_EVENT_TABLE()

OpenSpaceTimeDlg::OpenSpaceTimeDlg(bool table_only_s,
								   const wxPoint& pos, const wxSize& size)
: all_init(false),
table_only(table_only_s), time_steps(1)
{
	m_open_Shapefile_txt = 0;
	m_time_invariant_table_txt = 0;
	m_time_variant_table_txt = 0;
	m_time_variant_table_btn = 0;
	m_space_id_field = 0;
    m_time_id_field = 0;
	
	time_invariant_dbf_name = wxFileName("");
	time_variant_dbf_name = wxFileName("");
	sp_table_space_col = -1;
	sp_table_space_col_name = "";
	tm_table_space_col = -1;
	tm_table_space_col_name = "";
	tm_table_time_col = -1;
	tm_table_time_col_name = "";
    CreateControls();
	SetPosition(pos);
	Centre();
}

void OpenSpaceTimeDlg::CreateControls()
{
	if (table_only) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_OPEN_SPACE_TIME_DBF");
	} else {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_OPEN_SPACE_TIME_SHP");
		m_open_Shapefile_txt = XRCCTRL(*this, "ID_OPEN_SHAPEFILE_TXT",
									   wxTextCtrl);
	}
	m_time_invariant_table_txt = XRCCTRL(*this, "ID_TIME_INVARIANT_TABLE_TXT",
										 wxTextCtrl);
	m_time_variant_table_txt = XRCCTRL(*this, "ID_TIME_VARIANT_TABLE_TXT",
									   wxTextCtrl);
	m_time_variant_table_btn = XRCCTRL(*this, "ID_TIME_VARIANT_TABLE_BTN",
									   wxBitmapButton);
	m_space_id_field = XRCCTRL(*this, "ID_SPACE_ID_FIELD", wxChoice);
	m_time_id_field = XRCCTRL(*this, "ID_TIME_ID_FIELD", wxChoice);
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	all_init = true;
}


void OpenSpaceTimeDlg::OnOpenShapefileBtn( wxCommandEvent& event )
{
	if (!all_init) return;
	wxFileDialog dlg(this, "Choose a Shapefile to open", "", "",
					 "Shapefiles (*.shp)|*.shp");
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxFileName shp_fname = dlg.GetPath();
	
	bool shx_found;
	bool dbf_found;
	if (!GenUtils::ExistsShpShxDbf(shp_fname, 0, &shx_found, &dbf_found)) {
		wxString msg;
		msg <<  shp_fname.GetName() << ".shp, ";
		msg << shp_fname.GetName() << ".shx, and ";
		msg << shp_fname.GetName() << ".dbf ";
		msg << "were not found together in the same file directory. ";
		msg << "Could not find ";
		if (!shx_found && dbf_found) {
			msg << shp_fname.GetName() << ".shx.";
		} else if (shx_found && !dbf_found) {
			msg << shp_fname.GetName() << ".dbf.";
		} else {
			msg << shp_fname.GetName() << ".shx and ";
			msg << shp_fname.GetName() << ".dbf.";
		}
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	wxFileName dbf_fname(dlg.GetPath());
	dbf_fname.SetExt("dbf");
	DbfFileReader dbf_reader(dbf_fname.GetFullPath());
	if (!dbf_reader.isDbfReadSuccess()) {
		wxString msg;
		msg << "There was a problem opening ";
		msg << dbf_fname.GetName() << ".dbf.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
		
	time_invariant_dbf_name = dbf_fname;
	m_open_Shapefile_txt->SetValue(shp_fname.GetFullPath());
	m_time_invariant_table_txt->SetValue(dbf_fname.GetFullPath());

	m_time_variant_table_txt->Clear();
	m_time_variant_table_btn->Enable(true);
	m_space_id_field->Clear();
	m_space_id_field->Enable(false);
	m_time_id_field->Clear();
	m_time_id_field->Enable(false);
	sp_table_space_col = -1;
	sp_table_space_col_name = "";
	tm_table_space_col = -1;
	tm_table_space_col_name = "";
	tm_table_time_col = -1;
	tm_table_time_col_name = "";
	time_steps = 1;
	
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	
	wxString time_f1 = time_invariant_dbf_name.GetPathWithSep();
	time_f1 << time_invariant_dbf_name.GetName();
	time_f1 << "_time." << time_invariant_dbf_name.GetExt();
	wxString time_f2 = time_invariant_dbf_name.GetPathWithSep();
	time_f2 << time_invariant_dbf_name.GetName();
	time_f2 << "_TIME." << time_invariant_dbf_name.GetExt();
	if (wxFileExists(time_f1)) {
		OpenTimeTable(wxFileName(time_f1), true);
	} else if (wxFileExists(time_f2)) {
		OpenTimeTable(wxFileName(time_f2), true);
	}	
}

void OpenSpaceTimeDlg::OnOpenSpaceTableBtn( wxCommandEvent& event )
{
	if (!all_init) return;
	wxFileDialog dlg(this, "Choose a time-independant table to open", "", "",
					 "DBF files (*.dbf)|*.dbf");
	if (dlg.ShowModal() != wxID_OK) return;

	wxFileName dbf_fname(dlg.GetPath());
	dbf_fname.SetExt("dbf");
	DbfFileReader dbf_reader(dbf_fname.GetFullPath());
	if (!dbf_reader.isDbfReadSuccess()) {
		wxString msg;
		msg << "There was a problem opening ";
		msg << dbf_fname.GetName() << ".dbf.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	time_invariant_dbf_name = dbf_fname;
	m_time_invariant_table_txt->SetValue(dbf_fname.GetFullPath());
	
	m_time_variant_table_txt->Clear();
	m_time_variant_table_btn->Enable(true);
	m_space_id_field->Clear();
	m_space_id_field->Enable(false);
	m_time_id_field->Clear();
	m_time_id_field->Enable(false);
	sp_table_space_col = -1;
	sp_table_space_col_name = "";
	tm_table_space_col = -1;
	tm_table_space_col_name = "";
	tm_table_time_col = -1;
	tm_table_time_col_name = "";
	time_steps = 1;
	
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	
	wxString time_f1 = time_invariant_dbf_name.GetPathWithSep();
	time_f1 << time_invariant_dbf_name.GetName();
	time_f1 << "_time." << time_invariant_dbf_name.GetExt();
	wxString time_f2 = time_invariant_dbf_name.GetPathWithSep();
	time_f2 << time_invariant_dbf_name.GetName();
	time_f2 << "_TIME." << time_invariant_dbf_name.GetExt();
	if (wxFileExists(time_f1)) {
		OpenTimeTable(wxFileName(time_f1), true);
	} else if (wxFileExists(time_f2)) {
		OpenTimeTable(wxFileName(time_f2), true);
	}
}


void OpenSpaceTimeDlg::OnOpenTimeTableBtn( wxCommandEvent& event )
{
	if (!all_init) return;
	wxFileDialog dlg(this, "Choose a time-dependant table to open", "", "",
					 "DBF files (*.dbf)|*.dbf");
	if (dlg.ShowModal() != wxID_OK) return;
		
	OpenTimeTable(wxFileName(dlg.GetPath()), false);
}

void OpenSpaceTimeDlg::OpenTimeTable(const wxFileName& tbl_fname,
									 bool check_silent)
{
	DbfFileReader dbf_reader_tm(tbl_fname.GetFullPath());
	if (!dbf_reader_tm.isDbfReadSuccess()) {
		if (!check_silent) {
			wxString msg;
			msg << "There was a problem opening ";
			msg << tbl_fname.GetName() << ".dbf.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
		}
		return;
	}
	if (tbl_fname == time_invariant_dbf_name) {
		if (!check_silent) {
			wxString msg;
			msg << "Time-variant table must be different from ";
			msg << "time-invariant table.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
		}
		return;
	}
	
	std::vector<DbfFieldDesc> dbf_fields_tm = dbf_reader_tm.getFieldDescs();
	
	DbfFileReader dbf_reader_sp(time_invariant_dbf_name.GetFullPath());
	if (!dbf_reader_sp.isDbfReadSuccess()) {
		if (!check_silent) {
			wxString msg;
			msg << "There was a problem opening ";
			msg << time_invariant_dbf_name.GetName() << ".dbf.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
		}
		return;
	}
	
	std::vector<DbfFieldDesc> dbf_fields_sp = dbf_reader_sp.getFieldDescs();

	int sp_tbl_col_sp;
	int tm_tbl_col_sp;
	int tm_tbl_col_tm;
	std::vector<ColIdNamePair> sp_tbl_sp_id_map;
	std::vector<ColIdNamePair> tm_tbl_sp_id_map;
	std::vector<ColIdNamePair> tm_tbl_tm_id_map;
	
	bool matches_found = FindSpTimeFields(dbf_fields_sp, dbf_fields_tm,
										  sp_tbl_sp_id_map,
										  tm_tbl_sp_id_map,
										  tm_tbl_tm_id_map,
										  sp_tbl_col_sp,
										  tm_tbl_col_sp,
										  tm_tbl_col_tm, 
										  check_silent);
	if (!matches_found) return;
	
	sp_table_space_col = sp_tbl_col_sp;
	tm_table_space_col = tm_tbl_col_sp;
	tm_table_time_col = tm_tbl_col_tm;
	sp_table_sp_id_map = sp_tbl_sp_id_map;
	tm_table_sp_id_map = tm_tbl_sp_id_map;
	tm_table_tm_id_map = tm_tbl_tm_id_map;
	
	time_variant_dbf_name = tbl_fname;
	m_time_variant_table_txt->SetValue(time_variant_dbf_name.GetFullPath());
	m_space_id_field->Clear();
	m_time_id_field->Clear();
	m_space_id_field->Enable(true);
	m_time_id_field->Enable(true);
	int space_sel = -1;
	for (int i=0, iend=sp_table_sp_id_map.size(); i<iend; i++) {
		m_space_id_field->Append(sp_table_sp_id_map[i].name);
		if (sp_tbl_col_sp == sp_table_sp_id_map[i].col_id) {
			space_sel = i;
		}
	}
	m_space_id_field->SetSelection(space_sel);
	int time_sel = -1;
	for (int i=0, iend=tm_table_tm_id_map.size(); i<iend; i++) {
		m_time_id_field->Append(tm_table_tm_id_map[i].name);
		if (tm_tbl_col_tm == tm_table_tm_id_map[i].col_id) {
			time_sel = i;
		}
	}
	m_time_id_field->SetSelection(time_sel);
	
	FindWindow(XRCID("wxID_OK"))->Enable(true);
}

/**
 Looks for a common space-id field in space and time tables and a
 time field in time table.  If check_silent is false, then an error
 message will be shown if no valid fields found.
 */
bool OpenSpaceTimeDlg::FindSpTimeFields(
					const std::vector<DbfFieldDesc>& dbf_fields_sp,
					const std::vector<DbfFieldDesc>& dbf_fields_tm,
					std::vector<ColIdNamePair>& sp_table_sp_id_map,
					std::vector<ColIdNamePair>& tm_table_sp_id_map,
					std::vector<ColIdNamePair>& tm_table_tm_id_map,
					int& sp_tbl_col_sp, int& tm_tbl_col_sp,
					int& tm_tbl_col_tm, bool check_silent)
{
	sp_tbl_col_sp = -1;
	tm_tbl_col_sp = -1;
	sp_table_sp_id_map.clear();
	tm_table_sp_id_map.clear();
	bool match = false;
	for (int i=0, iend=dbf_fields_sp.size(); i<iend; i++) {
		for (int j=0, jend=dbf_fields_tm.size(); j<jend; j++) {
			if (dbf_fields_sp[i].name.CmpNoCase(dbf_fields_tm[j].name) == 0 &&
				dbf_fields_sp[i].type == dbf_fields_sp[j].type &&
				((dbf_fields_sp[i].type == 'N' || dbf_fields_sp[i].type == 'F')
				 && dbf_fields_sp[i].decimals == 0 &&
				 dbf_fields_tm[i].decimals == 0))
			{
				if (!match) {
					match = true;
					sp_tbl_col_sp = i;
					tm_tbl_col_sp = j;
				}
				sp_table_sp_id_map.push_back(
									ColIdNamePair(i,dbf_fields_sp[i].name));
				tm_table_sp_id_map.push_back(
									ColIdNamePair(j,dbf_fields_tm[j].name));
			}		 
		}
	}
	if (!match) {
		if (!check_silent) {
			wxString msg;
			msg << "No matching date or integer fields were found in chosen ";
			msg << "time-invariant and time-variant tables.  There must be ";
			msg << "at least one matching integer field for space and another ";
			msg << "a different integer or date field for time in the ";
			msg << "time-variant table.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
		}
		return false;
	}
	
	tm_tbl_col_tm = -1;
	tm_table_tm_id_map.clear();
	match = false;	
	for (int j=0, jend=dbf_fields_tm.size(); j<jend; j++) {
		if (dbf_fields_tm[j].type == 'D' ||
			((dbf_fields_tm[j].type == 'N' || dbf_fields_tm[j].type == 'F') &&
			 dbf_fields_tm[j].decimals == 0))
		{
			if (!match && j != tm_tbl_col_sp) {
				match = true;
				tm_tbl_col_tm = j;
			}
			tm_table_tm_id_map.push_back(
									ColIdNamePair(j,dbf_fields_tm[j].name));
		}
	}
	if (!match) {
		if (!check_silent) {
			wxString msg;
			msg << "No matching date or integer fields were found in chosen ";
			msg << "time-invariant and time-variant tables.  There must be ";
			msg << "at least one matching integer field for space and another ";
			msg << "a different integer or date field for time in the ";
			msg << "time-variant table.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
		}
		return false;
	}
	return true;
}

void OpenSpaceTimeDlg::OnFieldChoice( wxCommandEvent& event )
{
	if (!all_init) return;
	
	int sp_tbl_sp_sel = m_space_id_field->GetSelection();
	int tm_tbl_tm_sel = m_time_id_field->GetSelection();
	if (sp_tbl_sp_sel == -1 || sp_tbl_sp_sel == -1) return;
	
	sp_table_space_col = sp_table_sp_id_map[sp_tbl_sp_sel].col_id;
	sp_table_space_col_name = sp_table_sp_id_map[sp_tbl_sp_sel].name;

	bool match = false;
	for (int i=0, iend=tm_table_space_col; i<iend && !match; i++) {
		if (sp_table_space_col_name.CmpNoCase(tm_table_sp_id_map[i].name)==0) {
			match = true;
			tm_table_space_col = tm_table_sp_id_map[i].col_id;
			tm_table_space_col_name = tm_table_sp_id_map[i].name;
		}
	}
	
	tm_table_time_col = tm_table_tm_id_map[tm_tbl_tm_sel].col_id;
	tm_table_time_col_name = tm_table_tm_id_map[tm_tbl_tm_sel].name;
	
	bool same = sp_table_space_col_name.CmpNoCase(tm_table_time_col_name)==0;
	FindWindow(XRCID("wxID_OK"))->Enable(!same);
}

void OpenSpaceTimeDlg::OnOkClick( wxCommandEvent& event )
{
	// need to check that space & time fields are valid choices.
	// if so, then exit with wxID_OK, otherwise show error message
	// and don't exit
	EndDialog(wxID_OK);
}
