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

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../logger.h"
#include "ImportCsvDlg.h"

BEGIN_EVENT_TABLE( ImportCsvDlg, wxDialog )
	EVT_BUTTON( XRCID("wxID_OK"), ImportCsvDlg::OnOkClick )
	EVT_BUTTON( XRCID("ID_FIRST_ROW_CSV_HELP"),
			   ImportCsvDlg::OnFirstRowCsvHelp )
	EVT_BUTTON( XRCID("ID_INCLUDE_VAR_NAMES_HELP"),
			   ImportCsvDlg::OnIncludeVarNamesHelp)
END_EVENT_TABLE()

ImportCsvDlg::ImportCsvDlg(wxWindow* parent,
						   std::vector<std::string>& first_row_s,
						   const wxPoint& pos, const wxSize& size)
: contains_var_names(false), first_row(first_row_s), all_init(false)
{
	SetParent(parent);
    CreateControls();
	SetPosition(pos);
	Centre();
}

void ImportCsvDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_IMPORT_CSV_DLG");
	first_row_csv_txt = XRCCTRL(*this, "ID_FIRST_ROW_CSV_TXT", wxTextCtrl);
	include_var_names_cb = XRCCTRL(*this, "ID_INCLUDE_VAR_NAMES_CB",
								   wxCheckBox);
	Init();
	all_init = true;
}


void ImportCsvDlg::Init()
{
	using boost::lexical_cast;
    using boost::bad_lexical_cast;
	
	wxString s;
	for (int i=0; i<first_row.size(); i++) {
		s << wxString(first_row[i]);
		if (i < first_row.size()-1) s << "  ";
	}
	first_row_csv_txt->SetValue(s);
	
	bool all_numbers = true;
	for (int i=0; i<first_row.size(); i++) {
		try {
			double v = lexical_cast<double>(first_row[i]);
		} catch (bad_lexical_cast &) {
			all_numbers = false;
			break;
		}
	}
	include_var_names_cb->SetValue(all_numbers ? 0 : 1);
}

void ImportCsvDlg::OnOkClick( wxCommandEvent& event )
{
	if (!all_init) return;
	
	contains_var_names = (include_var_names_cb->GetValue() == 1);
	
	EndDialog(wxID_OK);
}

void ImportCsvDlg::OnFirstRowCsvHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "The text box below displays the first row of data ";
	msg << "extracted from the CSV file.  Use this information to determine ";
	msg << "if first row is table data or variable names";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void ImportCsvDlg::OnIncludeVarNamesHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Check this checkbox if first line of CSV file is table ";
	msg << "variable names.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}
