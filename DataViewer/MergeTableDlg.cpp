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

#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include "MergeTableDlg.h"
#include "../ShapeOperations/DbfFile.h"
#include "../DataViewer/DbfGridTableBase.h"

BEGIN_EVENT_TABLE( MergeTableDlg, wxDialog )
	EVT_RADIOBUTTON( XRCID("ID_KEY_VAL_RB"), MergeTableDlg::OnKeyValRB )
	EVT_RADIOBUTTON( XRCID("ID_REC_ORDER_RB"), MergeTableDlg::OnRecOrderRB )
	EVT_BUTTON( XRCID("ID_OPEN_BUTTON"), MergeTableDlg::OnOpenClick )
	EVT_BUTTON( XRCID("ID_INC_ALL_BUTTON"), MergeTableDlg::OnIncAllClick )
	EVT_BUTTON( XRCID("ID_INC_ONE_BUTTON"), MergeTableDlg::OnIncOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"),
					   MergeTableDlg::OnIncListDClick )
	EVT_BUTTON( XRCID("ID_EXCL_ALL_BUTTON"), MergeTableDlg::OnExclAllClick )
	EVT_BUTTON( XRCID("ID_EXCL_ONE_BUTTON"), MergeTableDlg::OnExclOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_EXCLUDE_LIST"),
					   MergeTableDlg::OnExclListDClick )
	EVT_CHOICE( XRCID("ID_CURRENT_KEY_CHOICE"), MergeTableDlg::OnKeyChoice )
	EVT_CHOICE( XRCID("ID_IMPORT_KEY_CHOICE"), MergeTableDlg::OnKeyChoice )
	EVT_BUTTON( XRCID("wxID_OK"), MergeTableDlg::OnMergeClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), MergeTableDlg::OnCloseClick )
END_EVENT_TABLE()

MergeTableDlg::MergeTableDlg(DbfGridTableBase* grid_base_s,
							 const wxPoint& pos)
: dbf_reader(0), grid_base(grid_base_s)
{
	SetParent(NULL);
	grid_base->FillColIdMap(col_id_map);
	CreateControls();
	Init();
	SetTitle("Merge - " + grid_base->GetDbfNameNoExt());
	SetPosition(pos);
    Centre();
}

MergeTableDlg::~MergeTableDlg()
{
	if (dbf_reader) delete dbf_reader;
}

void MergeTableDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_MERGE_TABLE_DLG");
	m_input_file_name = wxDynamicCast(FindWindow(XRCID("ID_INPUT_FILE_TEXT")),
									  wxTextCtrl);
	m_key_val_rb = wxDynamicCast(FindWindow(XRCID("ID_KEY_VAL_RB")),
								 wxRadioButton);
	m_rec_order_rb = wxDynamicCast(FindWindow(XRCID("ID_REC_ORDER_RB")),
								   wxRadioButton);
	m_current_key = wxDynamicCast(FindWindow(XRCID("ID_CURRENT_KEY_CHOICE")),
								  wxChoice);
	m_import_key = wxDynamicCast(FindWindow(XRCID("ID_IMPORT_KEY_CHOICE")),
								 wxChoice);
	m_exclude_list = wxDynamicCast(FindWindow(XRCID("ID_EXCLUDE_LIST")),
								   wxListBox);
	m_include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")),
								   wxListBox);
}

void MergeTableDlg::Init()
{
	table_fnames.clear();
	for (int i=0, iend=grid_base->col_data.size(); i<iend; i++) {
		wxString n = grid_base->col_data[col_id_map[i]]->name;
		n.MakeUpper();
		m_current_key->Append(n);
		table_fnames.insert(n);
	}
	UpdateMergeButton();
}

void MergeTableDlg::OnKeyValRB( wxCommandEvent& ev )
{
	UpdateMergeButton();
}

void MergeTableDlg::OnRecOrderRB( wxCommandEvent& ev )
{
	UpdateMergeButton();
}

void MergeTableDlg::OnOpenClick( wxCommandEvent& ev )
{
    wxFileDialog dlg( this, "DBF File To Merge", wxEmptyString,
					 wxEmptyString, "DBF files (*.dbf)|*.dbf",
					 wxFD_OPEN|wxFD_FILE_MUST_EXIST );
	
	if (dlg.ShowModal() == wxID_CANCEL) {
		return;
	}
	RemoveDbfReader();
	// GetPath returns the path and the file name + ext
	dbf_reader = new DbfFileReader(dlg.GetPath());
	if (!dbf_reader || !dbf_reader->isDbfReadSuccess()) {
		wxString msg("There was a problem reading the DBF file.");
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		RemoveDbfReader();
		return;
	}
	if (dbf_reader->getNumRecords() != grid_base->GetNumberRows()) {
		wxString msg("The selected DBF file contains ");
		msg << dbf_reader->getNumRecords() << " while the current Table ";
		msg << "contains " << grid_base->GetNumberRows();
		msg << " records. Please choose a DBF file with ";
		msg << grid_base->GetNumberRows() << " records.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		RemoveDbfReader();
		return;
	}
	m_input_file_name->SetValue(dbf_reader->getFileName());
	
	std::map<wxString, int> dbf_fn_freq;
	dups.clear();
	dedup_to_id.clear();
	for (int i=0, iend=dbf_reader->getNumFields(); i<iend; i++) {
		wxString name = dbf_reader->getFieldDesc(i).name.Upper();
		if (dbf_fn_freq.find(name) != dbf_fn_freq.end()) {
			dbf_fn_freq[name]++;
		} else {
			dbf_fn_freq[name] = 1;
		}
	}

	for (std::map<wxString, int>::iterator it=dbf_fn_freq.begin();
		 it!=dbf_fn_freq.end(); it++) {
		if ((*it).second > 1) dups.insert((*it).first);
	}
	
	std::map<wxString, int> dups_cntr;
	for (std::set<wxString>::iterator it=dups.begin(); it!=dups.end(); it++) {
		dups_cntr[(*it)] = 1;
	}
	
	dups.clear();
	for (int i=0, iend=dbf_reader->getNumFields(); i<iend; i++) {
		wxString name = dbf_reader->getFieldDesc(i).name.Upper();
		wxString dedup_name = name;
		if (dbf_fn_freq[name] > 1) {
			dedup_name << " (" << dups_cntr[name]++ << ")";
			dups.insert(dedup_name);
		}
		dedup_to_id[dedup_name] = i; // map to DBF col id
		m_import_key->Append(dedup_name);
		m_exclude_list->Append(dedup_name);
	}
}

void MergeTableDlg::OnIncAllClick( wxCommandEvent& ev)
{
	for (int i=0, iend=m_exclude_list->GetCount(); i<iend; i++) {
		m_include_list->Append(m_exclude_list->GetString(i));
	}
	m_exclude_list->Clear();

	UpdateMergeButton();
}

void MergeTableDlg::OnIncOneClick( wxCommandEvent& ev)
{
	if (m_exclude_list->GetSelection() >= 0) {
		wxString k = m_exclude_list->GetString(m_exclude_list->GetSelection());
		m_include_list->Append(k);
		m_exclude_list->Delete(m_exclude_list->GetSelection());
	}
	UpdateMergeButton();
}

void MergeTableDlg::OnIncListDClick( wxCommandEvent& ev)
{
	OnExclOneClick(ev);
}

void MergeTableDlg::OnExclAllClick( wxCommandEvent& ev)
{
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		m_exclude_list->Append(m_include_list->GetString(i));
	}
	m_include_list->Clear();
	UpdateMergeButton();
}

void MergeTableDlg::OnExclOneClick( wxCommandEvent& ev)
{
	if (m_include_list->GetSelection() >= 0) {
		m_exclude_list->
			Append(m_include_list->GetString(m_include_list->GetSelection()));
		m_include_list->Delete(m_include_list->GetSelection());
	}
	UpdateMergeButton();
}

void MergeTableDlg::OnExclListDClick( wxCommandEvent& ev)
{
	OnIncOneClick(ev);
}

wxString MergeTableDlg::getValidName(const std::map<wxString,int>& fname_to_id,
									 const std::set<wxString>& table_fnames,
									 const wxString& fname)
{
	wxString new_fname = fname;
	bool done = false;
	while (!done) {
		wxString msg;
		msg << "Field name \"" << new_fname << "\" is either a duplicate\n";
		msg << "or is invalid. Please enter an alternative, non-duplicate\n";
		msg << "field name. A valid field name is between one and ten\n";
		msg << "characters long. The first character must be a letter,\n";
		msg << "and the remaining characters can be either letters,\n";
		msg << "numbers or underscores.";

		wxTextEntryDialog dlg(this, msg, "Rename Field");
		if (dlg.ShowModal() == wxID_OK) {
			new_fname = dlg.GetValue().Upper();
			new_fname.Trim(false);
			new_fname.Trim(true);
			if (DbfFileUtils::isValidFieldName(new_fname) &&
				table_fnames.find(new_fname) == table_fnames.end() &&
				fname_to_id.find(new_fname) == fname_to_id.end())
			{
				done = true;
			}
		} else {
			new_fname = wxEmptyString;
			done = true;
		}
	}
	return new_fname;
}

void MergeTableDlg::OnMergeClick( wxCommandEvent& ev )
{
	//id_map: map from the ith row of the imported DBF to a row in the Table
	std::vector<int> id_map(grid_base->GetNumberRows());
	for (int i=0, iend=id_map.size(); i<iend; i++) id_map[i] = i;

	if (m_key_val_rb->GetValue()==1) {
		int key1_id = m_current_key->GetSelection();
		int key2_id = m_import_key->GetSelection();
		wxString key1_name = m_current_key->GetString(key1_id);
		wxString key2_name = m_import_key->GetString(key2_id);
	
		DbfColContainer& col1 = *(grid_base->col_data[col_id_map[key1_id]]);
		DbfColContainer col2(*dbf_reader, key2_id, grid_base);
		if (col1.type != col2.type) {
			wxString msg;
			msg << "Chosen merge key field " << key1_name << " and ";
			msg << key2_name << " are not of the same type.  Please choose ";
			msg << "compatible merge key fields.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	
		std::vector<wxString> key1_vec;
		std::map<wxString,int> key1_map;
		col1.GetVec(key1_vec);
		for (int i=0, iend=key1_vec.size(); i<iend; i++) {
			key1_vec[i].Trim(false);
			key1_vec[i].Trim(true);
			key1_map[key1_vec[i]] = i;
		}
	
		if (key1_vec.size() != key1_map.size()) {
			wxString msg;
			msg << "Chosen table merge key field " << key1_name;
			msg << " contains duplicate values. Key fields must contain all ";
			msg << "unique values.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;		
		}
	
		std::vector<wxString> key2_vec;
		std::set<wxString> key2_set;
		col2.GetVec(key2_vec);
		for (std::vector<wxString>::iterator it=key2_vec.begin();
			 it!=key2_vec.end(); it++) {
			(*it).Trim(false);
			(*it).Trim(true);
			key2_set.insert(*it);
		}
	
		if (key2_vec.size() != key2_set.size()) {
			wxString msg;
			msg << "Chosen DBF merge key field " << key2_name;
			msg << " contains duplicate values. Key fields must contain all ";
			msg << "unique values.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;		
		}
	
		std::map<wxString,int>::iterator key1_it;
		for (int i=0, iend = key2_vec.size(); i<iend; i++) {
			key1_it = key1_map.find(key2_vec[i]);
			if (key1_it == key1_map.end()) {
				wxString msg;
				msg << "The set of values in the two chosen key fields do ";
				msg << "not match exactly.  Please choose keys with matching ";
				msg << "sets of values.";
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			id_map[i] = (*key1_it).second;
		}
	}
	
	std::vector<wxString> dbf_fnames(m_include_list->GetCount());
	
	// dups tell us which strings need to be renamed, while
	// dedup_to_id tell us which col id this maps to in the original dbf
	// we need to create a final list of names to col id.
	
	std::map<wxString,int> fname_to_id;
	std::map<int,wxString> inc_order_to_fname; // keep track of order
		
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		wxString inc_n = m_include_list->GetString(i);
		wxString final_n = inc_n;
		if (dups.find(inc_n) != dups.end() ||
			!DbfFileUtils::isValidFieldName(inc_n) ||
			table_fnames.find(inc_n) != table_fnames.end())
		{
			final_n = getValidName(fname_to_id, table_fnames, inc_n);
		}
		if (final_n == wxEmptyString) return;
		fname_to_id[final_n] = dedup_to_id[inc_n];
		inc_order_to_fname[i] = final_n;
	}
 	
	// want to insert at end of table, but in order specified by
	// m_include_list
	// Create new columns in Table using fname_to_id map
	// first_col is the index into col_data of the first column of 
	// newly merged data.
	std::vector<int> dbf_col_to_table_col(dbf_reader->getNumFields(), -1);
	
	int first_col = grid_base->GetNumberCols();
	
	wxGrid* grid = grid_base->GetView();
	if (grid) grid->BeginBatch();
	int time_steps = 1; // non space-time column
	std::vector<DbfFieldDesc> fields = dbf_reader->getFieldDescs();
	for (int i=0, iend=inc_order_to_fname.size(); i<iend; i++) {
		wxString fname = inc_order_to_fname[i];
		int id = fname_to_id[fname];
		GeoDaConst::FieldType ftype = GeoDaConst::string_type;
		if (fields[id].type == 'D') ftype = GeoDaConst::date_type;
		if (fields[id].type == 'N' || fields[id].type == 'F') {
			ftype = fields[id].decimals == 0 ?
			GeoDaConst::long64_type : GeoDaConst::double_type;
		}
		dbf_col_to_table_col[id] = first_col+i;
		grid_base->InsertCol(first_col+i, time_steps, ftype,
							 fname, fields[id].length,
							 fields[id].decimals, fields[id].decimals,
							 true, false);
	}

	std::vector<DbfColContainer*>& col_data = grid_base->col_data;
	DbfFileReader& dbf = *dbf_reader;
	int rows = dbf.getNumRecords();
	int cols = dbf.getNumFields();
	// read in each row of the DBF, and write to the correct raw
	// data cell (row and column) as we read in.
	if (!dbf.file.is_open()) {
		dbf.file.open(dbf.fname.mb_str(wxConvUTF8),
					  std::ios::in | std::ios::binary);
	}
	if (!(dbf.file.is_open() && dbf.file.good())) return;
	
	int del_flag_len = 1;  // the record deletion flag
	dbf.file.seekg(dbf.header.header_length, std::ios::beg);
	char* buf = new char[dbf.getFileHeader().length_each_record];
	for (int row=0; row<rows; row++) {
		int t_row = id_map[row];
		dbf.file.seekg(del_flag_len, std::ios::cur);
		for (int col=0; col<cols; col++) {
			int field_len = fields[col].length;
			if (dbf_col_to_table_col[col] != -1) {
				int t_col = dbf_col_to_table_col[col];
				dbf.file.read((char*)(col_data[t_col]->raw_data[0] +
									  t_row*(field_len+1)),
							  field_len);
				col_data[t_col]->raw_data[0][t_row*(field_len+1)+field_len] = '\0';
			} else { // skip over this field
				// NOTE: successive read ops are much faster than
				// successive read, seekg, read, seekg.  Amazingly faster!
				dbf.file.read(buf, field_len);
			}
		}
	}
	delete [] buf;
	if (grid) grid->EndBatch();
	
	wxMessageDialog dlg(this, "File merged into Table successfully.",
						"Success", wxOK );
	dlg.ShowModal();

	ev.Skip();
	EndDialog(wxID_OK);	
}

void MergeTableDlg::OnCloseClick( wxCommandEvent& ev )
{
	ev.Skip();
	EndDialog(wxID_CLOSE);
}

void MergeTableDlg::OnKeyChoice( wxCommandEvent& ev )
{
	UpdateMergeButton();	
}

void MergeTableDlg::UpdateMergeButton()
{
	bool enable = (dbf_reader && !m_include_list->IsEmpty() &&
				   (m_rec_order_rb->GetValue()==1 ||
					(m_key_val_rb->GetValue()==1 &&
					 m_current_key->GetSelection() != wxNOT_FOUND &&
					 m_import_key->GetSelection() != wxNOT_FOUND)));
	FindWindow(XRCID("wxID_OK"))->Enable(enable);
}

void MergeTableDlg::RemoveDbfReader()
{
	if (dbf_reader) delete dbf_reader; dbf_reader = 0;
	m_input_file_name->SetValue("");
	m_import_key->Clear();
	m_exclude_list->Clear();
	m_include_list->Clear();
	UpdateMergeButton();
}
	
void MergeTableDlg::UpdateIncListItems()
{
	// highlight in red and items that will require a name change
	// in order to be merged into the table.  These includes illegal
	// and duplicate names
	
	std::set<wxString> dbf_fnames;
	
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		wxString s(m_include_list->GetString(i));
		std::set<wxString>::iterator it1 = table_fnames.find(s);
		std::set<wxString>::iterator it2 = dbf_fnames.find(s);
		bool black = (DbfFileUtils::isValidFieldName(s) && 
					  it1 == table_fnames.end() && it2 == dbf_fnames.end());
		// m_include_list->SetString(i, s);
		dbf_fnames.insert(s);
	}
}

