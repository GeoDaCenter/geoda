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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include "DataViewerDeleteColDlg.h"
#include "../GdaException.h"
#include "../logger.h"

BEGIN_EVENT_TABLE( DataViewerDeleteColDlg, wxDialog )
	EVT_BUTTON( XRCID("ID_DELETE_BUTTON"), DataViewerDeleteColDlg::OnDelete )
	EVT_LISTBOX( XRCID("ID_FIELD_CHOICE"), DataViewerDeleteColDlg::OnChoice )
END_EVENT_TABLE()

DataViewerDeleteColDlg::DataViewerDeleteColDlg( )
{
}

DataViewerDeleteColDlg::DataViewerDeleteColDlg( TableInterface* table_int_s,
											   wxWindow* parent)
: table_int(table_int_s)
{
    wxLogMessage("Open DataViewerDeleteColDlg.");
	SetParent(parent);
    CreateControls();
    Centre();

}

void DataViewerDeleteColDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_DATA_VIEWER_DELETE_COL_DLG");
	m_del_button = wxDynamicCast(FindWindow(XRCID("ID_DELETE_BUTTON")),
								 wxButton);
	m_del_button->Enable(false);
	m_field = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE")), wxListBox);
	InitFieldChoices();
	m_message = wxDynamicCast(FindWindow(XRCID("ID_TEXT_MSG")),
							  wxStaticText);
	m_message->SetLabelText("");	
}

void DataViewerDeleteColDlg::OnDelete( wxCommandEvent& ev )
{
    wxLogMessage("In DataViewerDeleteColDlg::OnDelete()");
    
    wxArrayInt selections;
    m_field->GetSelections(selections);
    
    int n =selections.GetCount();
	if (n== 0) {
		m_del_button->Enable(false);
		return;
	}
    col_id_map.clear();
    table_int->FillColIdMap(col_id_map);
   
    // check selected variables first
    bool check_success = true;
    for (int i=n-1; i>=0; i--) {
        int idx = selections[i];
        wxString nm = name_to_nm[m_field->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            check_success = false;
            break;
        }
        if (table_int->IsColTimeVariant(col)) {
            wxString err_msg = wxString::Format(_("Variable %s is a time-grouped variable.  Please ungroup this variable to delete."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Into", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            check_success = false;
            break;
            
        }
    }
    if (!check_success)
        return;
    
    for (int i=n-1; i>=0; i--) {
        int idx = selections[i];
        wxString nm = name_to_nm[m_field->GetString(idx)];
        int col = table_int->FindColId(nm);
        wxLogMessage(nm);

    	try{
    		table_int->DeleteCol(col);
    		m_del_button->Enable(false);

    	} catch (GdaException e) {
    		wxString msg;
    		msg << "Delete " << nm << ". " <<e.what();
    		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
    		dlg.ShowModal();
    		return;
    	}
    }
    InitFieldChoices();
    wxString msg;
    msg <<"Deleted " << n << " fields";
    m_message->SetLabelText(msg);
}

void DataViewerDeleteColDlg::OnChoice( wxCommandEvent& ev )
{
    wxLogMessage("In DataViewerDeleteColDlg::OnChoice()");
	m_message->SetLabelText("");
    wxArrayInt selections;
    m_field->GetSelections(selections);
	if (selections.GetCount() > 0) {
		m_del_button->Enable(true);
	} else {
		m_del_button->Enable(false);
    }
}

void DataViewerDeleteColDlg::InitFieldChoices()
{
	col_id_map.clear();
	m_field->Clear();
    wxArrayString var_items;
    table_int->FillColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            wxString nm = name;
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                nm << " (" << table_int->GetTimeString(t) << ")";
            }
            name_to_nm[nm] = name;
            var_items.Add(nm);
        } else {
            name_to_nm[name] = name;
            var_items.Add(name);
        }
    }
    m_field->InsertItems(var_items,0);
}
