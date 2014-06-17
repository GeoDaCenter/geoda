/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#include <set>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../logger.h"
#include "../Project.h"
#include "PCPDlg.h"

BEGIN_EVENT_TABLE( PCPDlg, wxDialog )
    EVT_BUTTON( wxID_OK, PCPDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, PCPDlg::OnCancelClick )

	EVT_BUTTON( XRCID("ID_INC_ALL_BUTTON"), PCPDlg::OnIncAllClick )
	EVT_BUTTON( XRCID("ID_INC_ONE_BUTTON"), PCPDlg::OnIncOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"),
					   PCPDlg::OnIncListDClick )
	EVT_BUTTON( XRCID("ID_EXCL_ALL_BUTTON"), PCPDlg::OnExclAllClick )
	EVT_BUTTON( XRCID("ID_EXCL_ONE_BUTTON"), PCPDlg::OnExclOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_EXCLUDE_LIST"),
					   PCPDlg::OnExclListDClick )
END_EVENT_TABLE()

PCPDlg::PCPDlg(Project* project_s, wxWindow* parent,
			   wxWindowID id, const wxString& title, const wxPoint& pos,
			   const wxSize& size, long style )
: project(project_s), table_int(project_s->GetTableInt())
{
    SetParent(parent);
    CreateControls();
	Init();
	SetPosition(pos);
    Centre();
}

void PCPDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_PCP");
	m_exclude_list = wxDynamicCast(FindWindow(XRCID("ID_EXCLUDE_LIST")),
								   wxListBox);
	m_include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")),
								   wxListBox);	
}

void PCPDlg::Init()
{
	table_int->FillNumericColIdMap(col_id_map);
	name_to_id.clear(); // map to table_int col id
	name_to_tm_id.clear(); // map to corresponding time id
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int id = col_id_map[i];
		wxString name = table_int->GetColName(id).Upper();
		if (table_int->IsColTimeVariant(id)) {
			for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
				wxString nm = name;
				nm << " (" << project->GetTableInt()->GetTimeString(t) << ")";
				name_to_id[nm] = id;
				name_to_tm_id[nm] = t;
				m_exclude_list->Append(nm);
			}
		} else {
			name_to_id[name] = id;
			name_to_tm_id[name] = 0;
			m_exclude_list->Append(name);
		}
	}
	
	UpdateOkButton();
}

void PCPDlg::OnIncAllClick( wxCommandEvent& ev)
{
	for (int i=0, iend=m_exclude_list->GetCount(); i<iend; i++) {
		m_include_list->Append(m_exclude_list->GetString(i));
	}
	m_exclude_list->Clear();
	
	UpdateOkButton();
}

void PCPDlg::OnIncOneClick( wxCommandEvent& ev)
{
	if (m_exclude_list->GetSelection() >= 0) {
		wxString k = m_exclude_list->GetString(m_exclude_list->GetSelection());
		m_include_list->Append(k);
		m_exclude_list->Delete(m_exclude_list->GetSelection());
	}
	UpdateOkButton();
}

void PCPDlg::OnIncListDClick( wxCommandEvent& ev)
{
	OnExclOneClick(ev);
}

void PCPDlg::OnExclAllClick( wxCommandEvent& ev)
{
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		m_exclude_list->Append(m_include_list->GetString(i));
	}
	m_include_list->Clear();
	UpdateOkButton();
}

void PCPDlg::OnExclOneClick( wxCommandEvent& ev)
{
	if (m_include_list->GetSelection() >= 0) {
		m_exclude_list->
		Append(m_include_list->GetString(m_include_list->GetSelection()));
		m_include_list->Delete(m_include_list->GetSelection());
	}
	UpdateOkButton();
}

void PCPDlg::OnExclListDClick( wxCommandEvent& ev)
{
	OnIncOneClick(ev);
}

void PCPDlg::OnOkClick( wxCommandEvent& event )
{
	int n_pcp_obs_sel = m_include_list->GetCount();
	
	col_ids.resize(m_include_list->GetCount());
	var_info.resize(m_include_list->GetCount());
	
	pcp_col_ids.resize(m_include_list->GetCount());
	pcp_col_tm_ids.resize(m_include_list->GetCount());

	// name_to_id tell us which col id this maps to in the original dbf
	// we need to create a final list of names to col id.
	
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		pcp_col_ids[i] = name_to_id[m_include_list->GetString(i)];
		pcp_col_tm_ids[i] = name_to_tm_id[m_include_list->GetString(i)];
		
		col_ids[i] = pcp_col_ids[i];
		var_info[i].time = pcp_col_tm_ids[i];
		var_info[i].name = table_int->GetColName(col_ids[i]);
		var_info[i].is_time_variant = table_int->IsColTimeVariant(col_ids[i]);
		table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
		var_info[i].sync_with_global_time = var_info[i].is_time_variant;
		var_info[i].fixed_scale = true;
	}
	// Call function to set all Secondary Attributes based on Primary Attributes
	Gda::UpdateVarInfoSecondaryAttribs(var_info);
	Gda::PrintVarInfoVector(var_info);
	
	event.Skip();
	EndDialog(wxID_OK);
}

void PCPDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);

}

void PCPDlg::UpdateOkButton()
{
	FindWindow(XRCID("wxID_OK"))->Enable(m_include_list->GetCount() >= 2);
}
