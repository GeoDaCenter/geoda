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
#include <wx/filedlg.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>
#include "AddIdVariable.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "CreateSpTmProjectDlg.h"

BEGIN_EVENT_TABLE( CreateSpTmProjectDlg, wxDialog )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE"), CreateSpTmProjectDlg::OnFieldChoice )
	EVT_BUTTON( XRCID("wxID_OK"), CreateSpTmProjectDlg::OnOkClick )
	EVT_BUTTON( XRCID("ID_ADD_NEW_FIELD_BTN"),
			   CreateSpTmProjectDlg::OnAddFieldClick )
	EVT_BUTTON( XRCID("ID_OUTPUT_TIME_DBF_BTN"),
			   CreateSpTmProjectDlg::OnOutputDbfFileClick )
	EVT_TEXT( XRCID("ID_TIME_FIELD_NAME_TXT"),
			 CreateSpTmProjectDlg::OnTimeFieldNameTxtChange )
	EVT_TEXT( XRCID("ID_INTERGER_TXT"),
			 CreateSpTmProjectDlg::OnIntegerTxtChange )
	EVT_BUTTON( XRCID("ID_SPACE_ID_VAR_HELP"),
			   CreateSpTmProjectDlg::OnSpaceIdVarHelp )
	EVT_BUTTON( XRCID("ID_SPACE_DBF_FILE_HELP"),
			   CreateSpTmProjectDlg::OnSpaceDbfFileHelp )
	EVT_BUTTON( XRCID("ID_TIME_DBF_FILE_HELP"),
			   CreateSpTmProjectDlg::OnTimeDbfFileHelp )
	EVT_BUTTON( XRCID("ID_NEW_TIME_ID_VAR_HELP"),
			   CreateSpTmProjectDlg::OnNewTimeIdVarHelp )
	EVT_BUTTON( XRCID("ID_TIME_PERIODS_HELP"),
			   CreateSpTmProjectDlg::OnTimePeriodsHelp )
END_EVENT_TABLE()

CreateSpTmProjectDlg::CreateSpTmProjectDlg(wxWindow* parent, Project* project_s,
										   const wxPoint& pos,
										   const wxSize& size)
: project(project_s), grid_base(project_s->GetGridBase()),
num_obs(project_s->GetNumRecords()), time_ids_valid(false), all_init(false)
{
	SetParent(parent);
    CreateControls();
	SetPosition(pos);
	Centre();
}

void CreateSpTmProjectDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_CREATE_SP_TM_PROJECT_DLG");
	add_new_field_btn = XRCCTRL(*this, "ID_ADD_NEW_FIELD_BTN", wxButton);
	field_choice = XRCCTRL(*this, "ID_FIELD_CHOICE", wxChoice);
	sp_dbf_file_static_txt = XRCCTRL(*this, "ID_SP_DBF_FILE_STATIC_TXT",
									 wxStaticText);
	output_tm_dbf_txt = XRCCTRL(*this, "ID_OUTPUT_TM_DBF_TXT", wxTextCtrl);
	output_time_dbf_btn = XRCCTRL(*this, "ID_OUTPUT_TIME_DBF_BTN",
								  wxBitmapButton);
	time_field_name_txt = XRCCTRL(*this, "ID_TIME_FIELD_NAME_TXT", wxTextCtrl);
	integer_txt = XRCCTRL(*this, "ID_INTERGER_TXT", wxTextCtrl);
	num_times_static_txt = XRCCTRL(*this, "ID_NUM_TIMES_STATIC_TXT",
								   wxStaticText);

	InitFieldChoices();

	wxString sp_txt("Space DBF File: ");
	sp_txt << project->GetMainName() << ".dbf";
	sp_dbf_file_static_txt->SetLabelText(sp_txt);
	
	wxString o_dbf;
	o_dbf << project->GetMainDir() << project->GetMainName() << "_time.dbf";
	output_tm_dbf_txt->SetValue(o_dbf);
	
	if ( !grid_base->ColNameExists("TIME") ) {
		time_field_name_txt->SetValue("TIME");
	}
	
	wxString n_tms_txt("Current number time periods: 0");
	num_times_static_txt->SetLabelText(n_tms_txt);
	
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	
	all_init = true;
}

void CreateSpTmProjectDlg::InitFieldChoices()
{
	int prev_cnt = field_choice->GetCount();
	wxString str_sel = field_choice->GetStringSelection();
	//int sel = field_choice->GetSelection();
	field_choice->Clear();
	
	grid_base->FillIntegerColIdMap(col_id_map);
	for (int i=0; i<col_id_map.size(); i++) {
		wxString name = grid_base->GetColName(col_id_map[i]);
		field_choice->Append(name);
	}

	if (prev_cnt == 0 && field_choice->GetCount() > 0) {
		// dialog has just been opened, so choose the first
		// item on the list
		field_choice->SetSelection(0);
	} else {
		// a new variable might have been added, so find old string
		field_choice->SetSelection(field_choice->FindString(str_sel));
	}
}

void CreateSpTmProjectDlg::OnOkClick( wxCommandEvent& event )
{
	if (!all_init) return;
	
	// we know that integer list is valid because OK button is enabled
	
	// warn if only one time period
	if (time_ids.size() == 1) {
		wxString msg;
		msg << "Only one time period specified. Normally a Space-Time project";
		msg << " has at least two time periods.  Ok to proceed with just ";
		msg << " one time period?";
		wxMessageDialog dlg (this, msg, "Proceed with one time period?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}
	
	// to ensure that unique ID column is in the shapefile, require that
	// all changes are saved before proceeding
	if (grid_base->ChangedSinceLastSave()) {
		wxString msg;
		msg << "Current project has unsaved changes to the Table. Changes ";
		msg << "must be saved before proceeding.  Ok to save changes now?";
		wxMessageDialog dlg (this, msg, "Save Changes Now?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
		if (!MyFrame::theFrame->SaveTableSpace()) {
			msg << "There was a problem saving Table changes. Aborting ";
			msg << "current operation.";
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		MyFrame::theFrame->UpdateToolbarAndMenus();
	}
	
	// check that time field name is valid
	wxString time_field = time_field_name_txt->GetValue();
	time_field.Trim(true);
	time_field.Trim(false);
	if ( !DbfFileUtils::isValidFieldName(time_field) ) {
		wxString msg;
		msg += "Error: \"" + time_field + "\" is an invalid ";
		msg += "field name.  A valid field name is between one and ten ";
		msg += "characters long.  The first character must be alphabetic,";
		msg += " and the remaining characters can be either alphanumeric ";
		msg += "or underscores.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	// check that time field name is unused
	if ( grid_base->ColNameExists(time_field) ) {
		wxString msg;
		msg << time_field << " already exists in the Table. Please choose ";
		msg << "a different name for the new time field.";
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	// check that unique id field is valid
	int space_id_col = -1;
	{
		std::vector<wxInt64> id_vec(num_obs);
		int sel = field_choice->GetSelection();
		if (sel < 0) {
			wxString msg;
			msg << "No Unquie Id Field choosen.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		space_id_col = col_id_map[sel];
		wxString col_string = field_choice->GetStringSelection();
		grid_base->col_data[space_id_col]->GetVec(id_vec, 0);
		std::set<wxInt64> id_set;
		for (int i=0, iend=id_vec.size(); i<iend; i++) id_set.insert(id_vec[i]);
		if (id_vec.size() != id_set.size()) {
			wxString msg;
			msg << col_string << " has duplicate values.  Please choose ";
			msg << "a different ID Field, or create one with the ";
			msg << "New ID Field button.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	
	// check that time file is valid, and if exists, give an overwrite warning
	// check for failure to create file / delete existing file or for failure
	// to write to file
	
	wxFileName new_dbf_fname(output_tm_dbf_txt->GetValue());
	wxString new_main_dir = new_dbf_fname.GetPathWithSep();
	wxString new_main_name = new_dbf_fname.GetName();
	wxString new_dbf = new_main_dir + new_main_name + ".dbf";
	wxString curr_dbf = project->GetMainDir() + project->GetMainName()+".dbf";
	LOG(new_dbf);
	LOG(curr_dbf);
	
	if (curr_dbf.CmpNoCase(new_dbf) == 0) {
		wxString msg;
		msg << "Current Space DBF name chosen for Time DBF!  Must choose ";
		msg << "a different name.  A space-time project requires both ";
		msg << "a space DBF file and a time DBF file.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	bool overwriting_required = false;
	if (wxFileExists(new_dbf)) {
		overwriting_required = true;
		wxString msg;
		msg << new_dbf << " already exists.  Ok to overwrite?";
		wxMessageDialog dlg (this, msg, "Overwrite?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}
		
	// Attempt to create a new Time DBF file
	wxString err_msg;
	bool success = grid_base->ConvertToSpTime(curr_dbf, new_dbf, space_id_col,
											  time_field, time_ids, err_msg);

	if (!success) {
		wxString msg;
		msg << "Unable to create Time DBF. " << err_msg; 
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	// Update Table menu to reflect this being a space-time project
	MyFrame::theFrame->UpdateToolbarAndMenus();
	
	EndDialog(wxID_OK);
}

void CreateSpTmProjectDlg::OnFieldChoice( wxCommandEvent& event )
{
	if (!all_init) return;
	UpdateOkButton();
}

void CreateSpTmProjectDlg::OnAddFieldClick( wxCommandEvent& event )
{
	if (!all_init) return;
	AddIdVariable dlg(grid_base, this);
	if (dlg.ShowModal() == wxID_OK) {
		// We know that the new id has been added to the table
		grid_base->SetChangedSinceLastSave(true);
		InitFieldChoices();
		field_choice->SetSelection(
							field_choice->FindString(dlg.GetIdVarName()));
		MyFrame::theFrame->UpdateToolbarAndMenus();
	}
	UpdateOkButton();
}

void CreateSpTmProjectDlg::OnTimeFieldNameTxtChange( wxCommandEvent& event )
{
	if (!all_init) return;
	LOG_MSG("In CreateSpTmProjectDlg::OnTimeFieldNameTxtChange");
	UpdateOkButton();
}	

void CreateSpTmProjectDlg::OnIntegerTxtChange( wxCommandEvent& event )
{
	if (!all_init) return;
	LOG_MSG("In CreateSpTmProjectDlg::OnIntegerTxtChange");
	
	time_ids_valid = false;
	wxString s = integer_txt->GetValue();
	wxStringTokenizer tkns(s);
	// tkns contains a sequence of space-seperated ints or dates
	//int n_tkns = tkns.CountTokens();
	time_ids.clear();
	int tkn_cnt = 0;
	bool valid = true;
	while (tkns.HasMoreTokens() && valid) {
		long val;
		valid = tkns.GetNextToken().ToCLong(&val);
		if (valid) {
			time_ids.push_back((wxInt64) val);
		}
	}
	wxString n_tms_txt("Current number time periods: ");
	if (!valid) {
		n_tms_txt << "invalid";
	} else if (time_ids.size() > CreateSpTmProjectDlg::max_time_periods) {
		n_tms_txt << "too many";
	} else {
		std::sort(time_ids.begin(), time_ids.end());
		bool duplicate = false;
		if (time_ids.size() > 1) {
			for (int i=0, its=time_ids.size()-1; !duplicate && i<its; i++) {
				if (time_ids[i] == time_ids[i+1]) duplicate = true;
			}
		}
		if (duplicate) {
			n_tms_txt << "duplicate";
		} else {
			n_tms_txt << time_ids.size();
			if (time_ids.size() > 0) time_ids_valid = true; 
		}
	}
	num_times_static_txt->SetLabelText(n_tms_txt);
	if (time_ids_valid) {
		wxTextAttr style(integer_txt->GetDefaultStyle());
		style.SetTextColour(*wxBLACK);
		integer_txt->SetStyle(0, s.length(), style);
	} else {
		wxTextAttr style(integer_txt->GetDefaultStyle());
		style.SetTextColour(*wxRED);
		integer_txt->SetStyle(0, s.length(), style);
	}
	
	UpdateOkButton();
}

void CreateSpTmProjectDlg::OnOutputDbfFileClick( wxCommandEvent& event )
{
	if (!all_init) return;

	wxFileDialog dlg( this, "Name of New Time DBF File", wxEmptyString,
					 wxEmptyString,
					 "DBF files (*.dbf)|*.dbf",
					 wxFD_SAVE );
	dlg.SetPath(project->GetMainDir());
	dlg.SetName(project->GetMainName() + "_table.dbf");
	if (dlg.ShowModal() == wxID_OK) {
		wxFileName new_dbf_fname(dlg.GetPath());
		wxString new_main_dir = new_dbf_fname.GetPathWithSep();
		wxString new_main_name = new_dbf_fname.GetName();
		wxString new_dbf = new_main_dir + new_main_name + ".dbf";
		output_tm_dbf_txt->SetValue(new_dbf);
	}
	UpdateOkButton();
}

void CreateSpTmProjectDlg::OnSpaceIdVarHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Variable used to identify each observation in space. Choose an ";
	msg << "existing variable, or create one with the New ID Variable button.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void CreateSpTmProjectDlg::OnSpaceDbfFileHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Space-Time Project data is stored in two DBF files: ";
	msg << "a space DBF file, and a time DBF file. Adding a time ";
	msg << "dimension does not immediately change the original (space) ";
	msg << "DBF file.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void CreateSpTmProjectDlg::OnTimeDbfFileHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "The default (and suggested) name for the time file ";
	msg << "portion of the space-time DBF file pair is \n";
	msg << project->GetMainName() << "_time.dbf";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void CreateSpTmProjectDlg::OnNewTimeIdVarHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "This variable name will be used internally in the Time DBF file. ";
	msg << "It will be hidden when the project is opened as a Space-Time ";
	msg << "project. If not already used, the variable name TIME is ";
	msg << "suggested.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void CreateSpTmProjectDlg::OnTimePeriodsHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Enter a list of space-separated integers. Example:\n\n";
	msg << "1900 1995 2000 2005 2010\n\n";
	msg << "The order you enter them does not matter (they will be sorted ";
	msg << "automatically sorted), and negative numbers are permissible. ";
	msg << "The number of time periods entered here will be the number ";
	msg << "of time-periods required for every space-time variable created ";
	msg << "in the new Space-Time project.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}


void CreateSpTmProjectDlg::UpdateOkButton()
{
	if (!all_init) return;
	
	wxString time_field = time_field_name_txt->GetValue();
	time_field.Trim(true);
	time_field.Trim(false);
	bool enable = (time_ids_valid && !output_tm_dbf_txt->GetValue().IsEmpty()
				   && !field_choice->GetStringSelection().IsEmpty()
				   && !time_field.IsEmpty());
	FindWindow(XRCID("wxID_OK"))->Enable(enable);
}
