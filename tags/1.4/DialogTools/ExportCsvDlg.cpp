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

#include <algorithm>
#include <fstream>
#include <set>
#include <wx/bmpbuttn.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../Project.h"
#include "../GenUtils.h"
#include "../ShapeOperations/CsvFileUtils.h"
#include "../logger.h"
#include "ExportCsvDlg.h"

BEGIN_EVENT_TABLE( ExportCsvDlg, wxDialog )
	EVT_BUTTON( XRCID("wxID_OK"), ExportCsvDlg::OnOkClick )
	EVT_BUTTON( XRCID("ID_INCLUDE_VAR_NAMES_HELP"),
			   ExportCsvDlg::OnIncludeVarNamesHelp )
END_EVENT_TABLE()

ExportCsvDlg::ExportCsvDlg(wxWindow* parent, Project* project_s,
						   const wxPoint& pos, const wxSize& size)
: project(project_s), grid_base(project_s->GetGridBase()), all_init(false)
{
	SetParent(parent);
    CreateControls();
	SetPosition(pos);
	Centre();
}

void ExportCsvDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_EXPORT_CSV_DLG");
	
	include_var_names_cb = XRCCTRL(*this, "ID_INCLUDE_VAR_NAMES_CB",
								   wxCheckBox);
	
	FindWindow(XRCID("wxID_OK"))->Enable(true);
	
	all_init = true;
}

void ExportCsvDlg::OnOkClick( wxCommandEvent& event )
{
	using namespace std;
	if (!all_init) return;
	
	bool inc_var_names = (include_var_names_cb->GetValue() == 1);
	
	// Attempt to create a new CSV file	
	wxFileDialog dlg( this, "Name of CSV File", wxEmptyString,
					 wxEmptyString,
					 "CSV files (*.csv)|*.csv",
					 wxFD_SAVE );
	dlg.SetPath(project->GetMainDir());
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxFileName new_csv_fname(dlg.GetPath());
	wxString new_main_dir = new_csv_fname.GetPathWithSep();
	wxString new_main_name = new_csv_fname.GetName();
	wxString new_csv = new_main_dir + new_main_name + ".csv";
	
	// Prompt for overwrite permission
	if (wxFileExists(new_csv)) {
		wxString msg;
		msg << new_csv << " already exists.  Ok to overwrite?";
		wxMessageDialog dlg (this, msg, "Overwrite?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}
	
	// Automatically overwrite existing csv since we have 
	// permission to overwrite.
	if (wxFileExists(new_csv)) {
		if (!wxRemoveFile(new_csv)) {
			wxString msg("Unable to overwrite ");
			msg << new_main_name + ".csv";
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	
	std::ofstream out_file;
	out_file.open(new_csv.mb_str(wxConvUTF8), std::ios::out);
	if (!(out_file.is_open() && out_file.good())) {
		wxString msg;
		msg << "Unable to create CSV file.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
		
	vector<int> col_map;
	grid_base->FillColIdMap(col_map);
	
	// Ensure string data is available
	int tot_cols = 0;
	for (int i=0, iend=grid_base->col_data.size(); i<iend; i++) {
		grid_base->col_data[i]->CopyVectorToRawData();
		tot_cols += grid_base->col_data[i]->time_steps;
	}
	
	int rows = grid_base->GetNumberRows();
	int cols = grid_base->GetNumberCols();
	vector<string> v(tot_cols);
	int v_ind;
	
	if (inc_var_names) {
		v_ind = 0;
		for (int col=0; col<cols; col++) {
			DbfColContainer* cc = grid_base->col_data[col_map[col]];
			for (int t=0; t<cc->time_steps; t++) {
				ostringstream ss;
				ss << string(cc->name.mb_str(wxConvUTF8));
				if (cc->time_steps > 1) {
					ss << "_" << grid_base->time_ids[t];
				}
				v[v_ind++] = ss.str();
			}
		}
		string record;
		GeoDa::StringsToCsvRecord(v, record);
		out_file << record << "\n";
	}
	
	for (int row=0; row<rows; row++) {
		v_ind = 0;
		for (int col=0; col<cols; col++) {
			DbfColContainer* cc = grid_base->col_data[col_map[col]];
			for (int t=0; t<cc->time_steps; t++) {
				v[v_ind++] = string((char*)(cc->raw_data[t]+
											row*(cc->field_len+1)));
			}
		}
		string record;
		GeoDa::StringsToCsvRecord(v, record);
		out_file << record << "\n";
	}
	
	out_file.close();
		
	EndDialog(wxID_OK);
}

void ExportCsvDlg::OnIncludeVarNamesHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Check checkbox to write out the Table variable names ";
	msg << "as the first row of data in the comma seperated value file.";
	msg << " If unchecked, only the Table data will be exported.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}
