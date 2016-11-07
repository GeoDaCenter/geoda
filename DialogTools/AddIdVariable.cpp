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

#include <cctype>
#include <string>
#include <vector>
#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DbfFile.h"
#include "AddIdVariable.h"

BEGIN_EVENT_TABLE( AddIdVariable, wxDialog )
    EVT_BUTTON( wxID_OK, AddIdVariable::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, AddIdVariable::OnCancelClick )
END_EVENT_TABLE()

AddIdVariable::AddIdVariable(TableInterface* table_int_s,
							 wxWindow* parent, wxWindowID id,
							 const wxString& caption,
							 const wxPoint& pos, const wxSize& size,
							 long style )
: table_int(table_int_s)
{
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
}

void AddIdVariable::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_ADD_ID_VARIABLE");
	new_id_var = wxDynamicCast(FindWindow(XRCID("IDC_NEW_ID_VAR")), wxTextCtrl);
    Connect(XRCID("IDC_NEW_ID_VAR"), wxEVT_COMMAND_TEXT_ENTER,
           wxCommandEventHandler(AddIdVariable::OnOkClick));

	existing_vars_list =
		wxDynamicCast(FindWindow(XRCID("IDC_EXISTING_VARS_LIST")), wxListBox);
	existing_vars_list->Clear();

	for (int i=0, iend=table_int->GetNumberCols(); i<iend; i++) {
		existing_vars_list->Append(table_int->GetColName(i));
	}
}

void AddIdVariable::OnOkClick( wxCommandEvent& event )
{
	new_id_var_name = new_id_var->GetValue();
	new_id_var_name.Trim(true);
	new_id_var_name.Trim(false);

    bool m_name_valid = table_int->IsValidDBColName(new_id_var_name);

    if (!m_name_valid) {
		wxString msg;
		msg << "Error: \"" + new_id_var_name + "\" is an invalid ";
		msg << "variable name. The first character must be alphabetic,";
		msg << " and the remaining characters can be either alphanumeric ";
		msg << "or underscores.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	bool name_exists = false;
	name_exists = table_int->ColNameExists(new_id_var_name);
	if (name_exists) {
		wxString msg;
		msg << "Variable name \"" + new_id_var_name;
		msg	<< "\" already exists. Please choose a different name.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
    wxLogMessage(wxString::Format("Try to insert unique id (integer, %s) column in table.", new_id_var_name));
    
    int col_insert_pos = 0;
	int add_pos = table_int->InsertCol(GdaConst::long64_type,new_id_var_name, col_insert_pos);
	if (add_pos >= 0) {
		std::vector<wxInt64> data(table_int->GetNumberRows());
		for (wxInt64 i=0, iend=data.size(); i<iend; i++)
            data[i] = i+1;
		table_int->SetColData(add_pos, 0/*time*/, data);
	} else {
		wxString msg("Could not create a new variable. "
					 "Possibly a read-only data source.");
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
    
	event.Skip();
	EndDialog(wxID_OK);
}

void AddIdVariable::OnCancelClick( wxCommandEvent& event )
{
    
	event.Skip();
}

wxString AddIdVariable::GetIdVarName()
{
	return new_id_var_name;
}
