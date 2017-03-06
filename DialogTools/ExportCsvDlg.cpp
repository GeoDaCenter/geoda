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

#include <algorithm>
#include <fstream>
#include <set>
#include <wx/wx.h>
#include <wx/bmpbuttn.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
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
: project(project_s), table_int(project_s->GetTableInt()), all_init(false)
{
    wxLogMessage("Open ExportCsvDlg");
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
    wxLogMessage("In ExportCsvDlg::OnOkClick()");
	using namespace std;
	if (!all_init) return;
	
	bool inc_var_names = (include_var_names_cb->GetValue() == 1);
	
	// Attempt to create a new CSV file	
	wxFileDialog dlg( this, "Name of CSV File", wxEmptyString,
					 wxEmptyString,
					 "CSV files (*.csv)|*.csv",
					 wxFD_SAVE );
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxFileName new_csv_fname(dlg.GetPath());
	wxString new_main_dir = new_csv_fname.GetPathWithSep();
	wxString new_main_name = new_csv_fname.GetName();
	wxString new_csv = new_main_dir + new_main_name + ".csv";
	
	// Prompt for overwrite permission
	if (wxFileExists(new_csv)) {
		wxString msg;
		msg << new_csv << " already exists.  OK to overwrite?";
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

    wxLogMessage(_("csv file:") + new_csv);
	std::ofstream out_file;	
	out_file.open(GET_ENCODED_FILENAME(new_csv),
				  std::ios::out | std::ios::binary);

	if (!(out_file.is_open() && out_file.good())) {
		wxString msg;
		msg << "Unable to create CSV file.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
		
	vector<int> col_map;
	table_int->FillColIdMap(col_map);
	
	int tot_cols = 0;
	
	int rows = table_int->GetNumberRows();
	int cols = table_int->GetNumberCols();
	vector<string> v;
	
	
	// This will export with the group name and time period rather
	// than the original datasource column names.
	if (inc_var_names) {
		for (int col=0; col<cols; col++) {
			int cid = col_map[col];
			for (int t=0; t<table_int->GetColTimeSteps(cid); t++) {
				if (table_int->GetColType(cid, t)
					== GdaConst::placeholder_type) continue;
				ostringstream ss;
				ss << table_int->GetColName(cid).ToStdString();
				if (table_int->GetColTimeSteps(cid) > 1) {
					ss << "_" << project->GetTableInt()->GetTimeString(t);
				}
				v.push_back(ss.str());
				++tot_cols;
			}
		}
		string record;
		Gda::StringsToCsvRecord(v, record);
		out_file << record << "\n";
	}
	
	for (int row=0; row<rows; row++) {
		int v_ind = 0;
		for (int col=0; col<cols; col++) {
			int cid = col_map[col];
			for (int t=0, tt=table_int->GetColTimeSteps(cid); t<tt; t++) {
				if (table_int->GetColType(cid, t)
					== GdaConst::placeholder_type) continue;
				v[v_ind++] = table_int->GetCellString(row,col,t).ToStdString();
			}
		}
		string record;
		Gda::StringsToCsvRecord(v, record);
		out_file << record << "\n";
	}
	
	out_file.close();
		
	EndDialog(wxID_OK);
}

void ExportCsvDlg::OnIncludeVarNamesHelp( wxCommandEvent& event )
{
    wxLogMessage("In ExportCsvDlg::OnIncludeVarNamesHelp()");
	wxString msg;
	msg << "Check checkbox to write out the Table variable names ";
	msg << "as the first row of data in the comma seperated value file.";
	msg << " If unchecked, only the Table data will be exported.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}
