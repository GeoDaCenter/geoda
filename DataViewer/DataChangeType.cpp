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

#include <boost/foreach.hpp>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/textdlg.h>
#include <wx/valnum.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>

#include "../GdaConst.h"
#include "../Project.h"
#include "DataViewerAddColDlg.h"
#include "TableInterface.h"
#include "TableState.h"
#include "../logger.h"
#include "DataChangeType.h"

using namespace std;

BEGIN_EVENT_TABLE(DataChangeTypeFrame, TemplateFrame)
	EVT_ACTIVATE(DataChangeTypeFrame::OnActivate)
END_EVENT_TABLE()

DataChangeTypeFrame::DataChangeTypeFrame(wxFrame *parent, Project* project,
								 const wxString& title, const wxPoint& pos,
								 const wxSize& size, const long style)
: TemplateFrame(parent, project, title, pos, size, style),
table_state(project->GetTableState()), table_int(project->GetTableInt()),
from_vars(0), from_data(0),
add_var_btn(0), copy_btn(0),  
to_vars(0), to_data(0),
ignore_callbacks(false)
{
	wxLogMessage("Open DataChangeTypeFrame.");
	
	panel = new wxPanel(this);
	//panel->SetBackgroundColour(*wxWHITE);
	//SetBackgroundColour(*wxWHITE);
	
	const int list_width = 300;
	const int vars_list_height = 100;
	const int data_list_height = 200;
	
	const int name_col_width = 160;
	const int type_col_width = 60;
	const int time_col_width = 60;
	
	const int data_id_col_width = 70;
	const int data_val_col_width = 300-90;
	
	wxStaticText* from_title = new wxStaticText(panel, wxID_ANY, "Current Variable");
	from_vars = new wxListCtrl(panel, XRCID("ID_FROM_VARS"), wxDefaultPosition,
							   wxSize(list_width, vars_list_height),
							   wxLC_REPORT);
	from_vars->AppendColumn("Name");
	from_vars->AppendColumn("Type");
	from_vars->AppendColumn("Time");
	from_vars->SetColumnWidth(NAME_COL, name_col_width);
	from_vars->SetColumnWidth(TYPE_COL, type_col_width);
	from_vars->SetColumnWidth(TIME_COL, time_col_width);
	Connect(XRCID("ID_FROM_VARS"), wxEVT_LIST_ITEM_SELECTED,
			wxListEventHandler(DataChangeTypeFrame::OnFromVarsSel));
	Connect(XRCID("ID_FROM_VARS"), wxEVT_LIST_ITEM_ACTIVATED,
			wxListEventHandler(DataChangeTypeFrame::OnFromVarsSel));
	
	from_data = new GdaColListCtrl(table_int, panel, XRCID("ID_FROM_DATA"),
								   wxDefaultPosition,
								   wxSize(list_width, data_list_height));
	from_data->AppendColumn("Row");
	from_data->AppendColumn("Value");
	from_data->SetColumnWidth(DATA_ID_COL, data_id_col_width);
	from_data->SetColumnWidth(DATA_VAL_COL, data_val_col_width);
	from_data->SetItemCount(table_int->GetNumberRows());
	Connect(XRCID("ID_FROM_DATA"), wxEVT_LIST_ITEM_SELECTED,
			wxListEventHandler(DataChangeTypeFrame::OnFromDataSel));
	Connect(XRCID("ID_FROM_DATA"), wxEVT_LIST_ITEM_ACTIVATED,
			wxListEventHandler(DataChangeTypeFrame::OnFromDataSel));
	
	add_var_btn = new wxButton(panel, XRCID("ID_ADD_VAR_BTN"), " Add Transformed Variable",
							   wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);	
	Connect(XRCID("ID_ADD_VAR_BTN"), wxEVT_BUTTON,
			wxCommandEventHandler(DataChangeTypeFrame::OnAddVarBtn));
	
	//copy_btn = new wxButton(panel, XRCID("ID_COPY_BTN"), "-> Copy ->", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	//Connect(XRCID("ID_COPY_BTN"), wxEVT_BUTTON, wxCommandEventHandler(DataChangeTypeFrame::OnCopyBtn));

	wxStaticText* to_title = new wxStaticText(panel, wxID_ANY, "Transformed Variable");
	to_vars = new wxListCtrl(panel, XRCID("ID_TO_VARS"), wxDefaultPosition,
							   wxSize(list_width, vars_list_height),
							 wxLC_REPORT);
	to_vars->AppendColumn("Name");
	to_vars->AppendColumn("Type");
	to_vars->AppendColumn("Time");
	to_vars->SetColumnWidth(NAME_COL, name_col_width);
	to_vars->SetColumnWidth(TYPE_COL, type_col_width);
	to_vars->SetColumnWidth(TIME_COL, time_col_width);
	Connect(XRCID("ID_TO_VARS"), wxEVT_LIST_ITEM_SELECTED,
			wxListEventHandler(DataChangeTypeFrame::OnToVarsSel));
	Connect(XRCID("ID_TO_VARS"), wxEVT_LIST_ITEM_ACTIVATED,
			wxListEventHandler(DataChangeTypeFrame::OnToVarsSel));
	
	to_data = new GdaColListCtrl(table_int, panel, XRCID("ID_TO_DATA"),
								 wxDefaultPosition,
								 wxSize(list_width, data_list_height));
	to_data->AppendColumn("Row");
	to_data->AppendColumn("Value");
	to_data->SetColumnWidth(DATA_ID_COL, data_id_col_width);
	to_data->SetColumnWidth(DATA_VAL_COL, data_val_col_width);
	to_data->SetItemCount(table_int->GetNumberRows());
	Connect(XRCID("ID_TO_DATA"), wxEVT_LIST_ITEM_SELECTED,
			wxListEventHandler(DataChangeTypeFrame::OnToDataSel));
	Connect(XRCID("ID_TO_DATA"), wxEVT_LIST_ITEM_ACTIVATED,
			wxListEventHandler(DataChangeTypeFrame::OnToDataSel));
	
	wxBoxSizer* from_top_vert_szr = new wxBoxSizer(wxVERTICAL);
	
	from_top_vert_szr->Add(from_title, 0, wxALIGN_CENTER_HORIZONTAL);
	from_top_vert_szr->AddSpacer(5);
	from_top_vert_szr->Add(from_vars, 1, wxEXPAND);
	from_top_vert_szr->AddSpacer(5);
	from_top_vert_szr->Add(from_data, 2, wxEXPAND);
	
	
	wxBoxSizer* btns_top_vert_szr = new wxBoxSizer(wxVERTICAL);
	//btns_top_vert_szr->Add(copy_btn, 0, wxALIGN_CENTER_HORIZONTAL);
	//btns_top_vert_szr->AddSpacer(20);
	btns_top_vert_szr->Add(add_var_btn, 0, wxALIGN_CENTER_HORIZONTAL);
	

	wxBoxSizer* to_top_vert_szr = new wxBoxSizer(wxVERTICAL);

	to_top_vert_szr->Add(to_title, 0, wxALIGN_CENTER_HORIZONTAL);
	to_top_vert_szr->AddSpacer(5);
	to_top_vert_szr->Add(to_vars, 1, wxEXPAND);
	to_top_vert_szr->AddSpacer(5);
	to_top_vert_szr->Add(to_data, 2, wxEXPAND);
	

	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(from_top_vert_szr, 1, wxEXPAND);
	panel_h_szr->AddSpacer(10);
	panel_h_szr->Add(btns_top_vert_szr, 0, wxALIGN_CENTER_VERTICAL);
	panel_h_szr->AddSpacer(10);
	panel_h_szr->Add(to_top_vert_szr, 1, wxEXPAND);
	
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);	
	panel_v_szr->Add(panel_h_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_v_szr);
	
	// Top Sizer for Frame
	wxBoxSizer* top_v_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_v_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
		
	SetSizerAndFit(top_v_sizer);
	DisplayStatusBar(false);
	
	RefreshFromVars();
	RefreshToVars();
	UpdateButtons();
	
	table_state->registerObserver(this);
	Show(true);
}

DataChangeTypeFrame::~DataChangeTypeFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
	table_state->removeObserver(this);
}

void DataChangeTypeFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In DataChangeTypeFrame::OnActivate");
		RegisterAsActive("DataChangeTypeFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void DataChangeTypeFrame::OnAddVarBtn(wxCommandEvent& ev)
{
	wxLogMessage("In DataChangeTypeFrame::OnAddVarBtn");
    
	if (!from_vars || !from_data || !to_vars || !to_data)
        return;
    
    wxString from_name;
    wxString from_time;
    int from_col = -1;
    int from_tm = 0;
    GdaConst::FieldType from_type;
    int from_sel = GetFromVarSel(from_name, from_time);
    bool from_tm_variant = true;

    if (from_sel < 0) {
        return;
    }
    
    wxString to_name;
    wxString to_time;
    int to_col = -1;
    int to_tm = 0;
    GdaConst::FieldType to_type;
    int to_sel = GetToVarSel(to_name, to_time);
    bool to_tm_variant = true;
    
    if (to_sel < 0) {
        // Add New Field
        DataViewerAddColDlg dlg(project, this);
        if (dlg.ShowModal() != wxID_OK) return;
        wxString new_name = dlg.GetColName();
        RefreshToVars();
        long item = -1;
        for (long i=0, sz = to_vars->GetItemCount(); i<sz; ++i) {
            if (to_vars->GetItemText(i, NAME_COL) == new_name) {
                item = i;
                break;
            }
        }
        if (item != -1) {
            for (long i=0, sz = to_vars->GetItemCount(); i<sz; ++i) {
                if (i == item) {
                    to_vars->SetItemState(i, wxLIST_STATE_SELECTED,  wxLIST_STATE_SELECTED);
                    to_vars->EnsureVisible(i);
                } else {
                    to_vars->SetItemState(i, 0, wxLIST_STATE_SELECTED);
                }
            }
        }
        to_sel = GetToVarSel(to_name, to_time);
    } else {
        wxString msg = _("Tip: clear selection on Transformed Variable list to add a new transformed variable");
        wxString ttl = wxString::Format(_("Do you want to use \"%s\" as Transformed Variable?"), to_name);
        wxMessageDialog dlg (this, msg, ttl, wxYES_NO | wxNO_DEFAULT);
        if (dlg.ShowModal() != wxID_YES) return;
    }
	    
    // Copy
    from_col = table_int->FindColId(from_name);
    from_tm_variant = table_int->IsColTimeVariant(from_col);
    if (from_tm_variant) {
        from_tm = table_int->GetTimeInt(from_time);
    }
    from_type = table_int->GetColType(from_col, from_tm);
    
    to_col = table_int->FindColId(to_name);
    to_tm_variant = table_int->IsColTimeVariant(to_col);
    if (to_tm_variant) {
        to_tm = table_int->GetTimeInt(to_time);
    }
    to_type = table_int->GetColType(to_col, to_tm);
    
    if (from_col < 0 ||
        from_type == GdaConst::unknown_type ||
        from_type == GdaConst::placeholder_type ||
        to_col < 0 ||
        to_type == GdaConst::unknown_type ||
        to_type == GdaConst::placeholder_type)
    {
        wxString s = _("An unknown problem occurred. Could not copy data.");
        wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        //copy_btn->Disable();
        return;
    }
    
    if (to_type == GdaConst::date_type ||
        to_type == GdaConst::time_type ||
        to_type == GdaConst::datetime_type)
    {
        wxString s = _("GeoDa does not support copying to date/time variables currently.");
        wxMessageDialog dlg(NULL, s, _("Information"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    
    int num_rows = table_int->GetNumberRows();
    
    vector<bool> undefined(num_rows, false);
    if (from_type == GdaConst::long64_type ||
        from_type == GdaConst::date_type ||
        from_type == GdaConst::time_type ||
        from_type == GdaConst::datetime_type)
    {
        vector<wxInt64> data;
        table_int->GetColData(from_col, from_tm, data);
        table_int->GetColUndefined(from_col, from_tm, undefined);
        if (to_type == GdaConst::long64_type ||
            to_type == GdaConst::double_type )
        {
            table_int->SetColData(to_col, to_tm, data);
            table_int->SetColUndefined(to_col, to_tm, undefined);
        }
        else if (to_type == GdaConst::string_type)
        {
            vector<wxString> str(num_rows);
            for (size_t i=0, sz=num_rows; i<sz; ++i) {
                str[i] << data[i];
            }
            table_int->SetColData(to_col, to_tm, str);
            table_int->SetColUndefined(to_col, to_tm, undefined);
        }
    }
    else if (from_type == GdaConst::double_type)
    {
        vector<double> data;
        table_int->GetColData(from_col, from_tm, data);
        table_int->GetColUndefined(from_col, from_tm, undefined);
        if (to_type == GdaConst::long64_type ||
            to_type == GdaConst::double_type )
        {
            table_int->SetColData(to_col, to_tm, data);
            table_int->SetColUndefined(to_col, to_tm, undefined);
        }
        else if (to_type == GdaConst::string_type)
        {
            vector<wxString> str(num_rows);
            for (size_t i=0, sz=num_rows; i<sz; ++i) {
                str[i] << data[i];
            }
            table_int->SetColData(to_col, to_tm, str);
            table_int->SetColUndefined(to_col, to_tm, undefined);
        }
    }
    else if (from_type == GdaConst::string_type)
    {
        vector<wxString> data;
        table_int->GetColData(from_col, from_tm, data);
        if (to_type == GdaConst::long64_type)
        {
            vector<wxInt64> nums(num_rows, 0);
            for (size_t i=0, sz=num_rows; i<sz; ++i) {
                wxInt64 val;
                wxString _data = data[i].Trim(true).Trim(false);
                if( _data.ToLongLong(&val)) {
                    nums[i] = val;
                }else {
                    undefined[i] = true;
                }
            }
            table_int->SetColData(to_col, to_tm, nums);
            table_int->SetColUndefined(to_col, to_tm, undefined);
        }
        else if (to_type == GdaConst::double_type )
        {
            vector<double> nums(num_rows, 0);
            for (size_t i=0, sz=num_rows; i<sz; ++i) {
                double val;
                wxString _data = data[i].Trim(true).Trim(false);
                if (_data.ToDouble(&val)) {
                    nums[i] = val;
                } else {
                    undefined[i] = true;
                }
            }
            table_int->SetColData(to_col, to_tm, nums);
            table_int->SetColUndefined(to_col, to_tm, undefined);
        }
        else if (to_type == GdaConst::string_type)
        {
            table_int->SetColData(to_col, to_tm, data);
        }
    }

	//UpdateButtons();
}

void DataChangeTypeFrame::OnCopyBtn(wxCommandEvent& ev)
{
	wxLogMessage("Entering DataChangeTypeFrame::OnCopyBtn");
	wxString from_name;
	wxString from_time;
	int from_col = -1;
	int from_tm = 0;
	GdaConst::FieldType from_type;
	int from_sel = GetFromVarSel(from_name, from_time);
	bool from_tm_variant = true;
	
	wxString to_name;
	wxString to_time;
	int to_col = -1;
	int to_tm = 0;
	GdaConst::FieldType to_type;
	int to_sel = GetToVarSel(to_name, to_time);
	bool to_tm_variant = true;
	
	if (from_sel < 0 || to_sel < 0) {
		//copy_btn->Disable();
		return;
	}
	
	from_col = table_int->FindColId(from_name);
	from_tm_variant = table_int->IsColTimeVariant(from_col);
	if (from_tm_variant) {
		from_tm = table_int->GetTimeInt(from_time);
	}
	from_type = table_int->GetColType(from_col, from_tm);
	
	to_col = table_int->FindColId(to_name);
	to_tm_variant = table_int->IsColTimeVariant(to_col);
	if (to_tm_variant) {
		to_tm = table_int->GetTimeInt(to_time);
	}
	to_type = table_int->GetColType(to_col, to_tm);
	
	if (from_col < 0 ||
        from_type == GdaConst::unknown_type ||
        from_type == GdaConst::placeholder_type ||
		to_col < 0 ||
        to_type == GdaConst::unknown_type ||
        to_type == GdaConst::placeholder_type)
	{
		wxString s = _("An unknown problem occurred. Could not copy data.");
		wxMessageDialog dlg(NULL, s, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		//copy_btn->Disable();
		return;
	}
	
    if (to_type == GdaConst::date_type ||
        to_type == GdaConst::time_type ||
        to_type == GdaConst::datetime_type)
	{
		wxString s = _("GeoDa does not support copying to date variables currently.");
		wxMessageDialog dlg(NULL, s, "Information", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	
	int num_rows = table_int->GetNumberRows();
	vector<bool> undefined(num_rows, false);
	if (from_type == GdaConst::long64_type ||
        from_type == GdaConst::date_type ||
        from_type == GdaConst::time_type ||
        from_type == GdaConst::datetime_type)
	{
		vector<wxInt64> data;
		table_int->GetColData(from_col, from_tm, data);
		table_int->GetColUndefined(from_col, from_tm, undefined);
		if (to_type == GdaConst::long64_type ||
			to_type == GdaConst::double_type )
		{
			table_int->SetColData(to_col, to_tm, data);
			table_int->SetColUndefined(to_col, to_tm, undefined);
		}
		else if (to_type == GdaConst::string_type)
		{
			vector<wxString> str(num_rows);
			for (size_t i=0, sz=num_rows; i<sz; ++i) {
				if (undefined[i]) continue;
				str[i] << data[i];
			}
			table_int->SetColData(to_col, to_tm, str);
		}
	}
	else if (from_type == GdaConst::double_type)
	{
		vector<double> data;
		table_int->GetColData(from_col, from_tm, data);
		table_int->GetColUndefined(from_col, from_tm, undefined);
		if (to_type == GdaConst::long64_type ||
			to_type == GdaConst::double_type )
		{
			table_int->SetColData(to_col, to_tm, data);
			table_int->SetColUndefined(to_col, to_tm, undefined);
		}
		else if (to_type == GdaConst::string_type)
		{
			vector<wxString> str(num_rows);
			for (size_t i=0, sz=num_rows; i<sz; ++i) {
				if (undefined[i]) continue;
				str[i] << data[i];
			}
			table_int->SetColData(to_col, to_tm, str);
		}
	}
	else if (from_type == GdaConst::string_type)
	{
		vector<wxString> data;
		table_int->GetColData(from_col, from_tm, data);
		if (to_type == GdaConst::long64_type)
		{
			vector<wxInt64> nums(num_rows, 0);
			for (size_t i=0, sz=num_rows; i<sz; ++i) {
				wxInt64 val;
                wxString _data = data[i].Trim();
				if( _data.ToLongLong(&val))
					nums[i] = val;
				else {
                    undefined[i] = true;
                }
			}
			table_int->SetColData(to_col, to_tm, nums);
			table_int->SetColUndefined(to_col, to_tm, undefined);
		}
		else if (to_type == GdaConst::double_type )
		{
			vector<double> nums(num_rows, 0);
			for (size_t i=0, sz=num_rows; i<sz; ++i) {
				double val;
				undefined[i] = !data[i].Trim().ToDouble(&val);
				if (!undefined[i]) nums[i] = val;
			}
			table_int->SetColData(to_col, to_tm, nums);
			table_int->SetColUndefined(to_col, to_tm, undefined);
		}
		else if (to_type == GdaConst::string_type)
		{
			table_int->SetColData(to_col, to_tm, data);
		}
	}
}

void DataChangeTypeFrame::OnFromVarsSel(wxListEvent& ev)
{
	wxLogMessage("In DataChangeTypeFrame::OnFromVarsSel");
	if (!from_vars || !from_data || ignore_callbacks) return;
	wxString name;
	wxString time;
	int sel = GetFromVarSel(name, time);
	if (sel == -1) {
		//LOG_MSG("GetFromVarSel returned -1");
	}	
	RefreshData(from_data, name, time);
	UpdateButtons();
}

void DataChangeTypeFrame::OnFromDataSel(wxListEvent& ev)
{
	wxLogMessage("In DataChangeTypeFrame::OnFromDataSel");
	if (!from_data || ignore_callbacks) return;
	int sel = ev.GetIndex();
	if (sel < 0 || sel >= from_data->GetItemCount()) return;
	from_data->SetItemState(sel, 0, wxLIST_STATE_SELECTED);
}

void DataChangeTypeFrame::OnToVarsSel(wxListEvent& ev)
{
	wxLogMessage("In DataChangeTypeFrame::OnToVarsSel");
	if (!to_vars || !to_data || ignore_callbacks) return;
	wxString name;
	wxString time;
	int sel = GetToVarSel(name, time);
	if (sel == -1) {
		//LOG_MSG("GetToVarSel returned -1");
	}
	RefreshData(to_data, name, time);
	UpdateButtons();
}

void DataChangeTypeFrame::OnToDataSel(wxListEvent& ev)
{
	wxLogMessage("In DataChangeTypeFrame::OnToDataSel");
	if (!to_data || ignore_callbacks) return;
	int sel = ev.GetIndex();
	if (sel < 0 || sel >= to_data->GetItemCount()) return;
	to_data->SetItemState(sel, 0, wxLIST_STATE_SELECTED);
}

/** Implementation of TableStateObserver interface */
void DataChangeTypeFrame::update(TableState* o)
{
	RefreshFromVars();
	RefreshToVars();
	UpdateButtons();
}

void DataChangeTypeFrame::UpdateButtons()
{
	//if (!copy_btn) return;
	if (!from_vars || !from_data || !to_vars || !to_data) {
		//copy_btn->Disable();
		return;
	}
	wxString n, t;
	//copy_btn->Enable(GetFromVarSel(n, t) >= 0 && GetToVarSel(n, t) >= 0);
}

int DataChangeTypeFrame::GetFromVarSel(wxString& name, wxString& time)
{
	return GetVarSel(from_vars, name, time);
}

void DataChangeTypeFrame::RefreshFromVars()
{
	RefreshVars(from_vars, from_data);
}

int DataChangeTypeFrame::GetToVarSel(wxString& name, wxString& time)
{
	return GetVarSel(to_vars, name, time);
}


void DataChangeTypeFrame::RefreshToVars()
{
	RefreshVars(to_vars, to_data);
}

int DataChangeTypeFrame::GetVarSel(wxListCtrl* vars_list,
								   wxString& name, wxString& time)
{
	name = "";
	time = "";
	int sel = -1;
	if (!vars_list) return sel;
	for (size_t i=0, sz=vars_list->GetItemCount(); i<sz; ++i) {
		if (vars_list->GetItemState(i, wxLIST_STATE_SELECTED) != 0) {
			sel = i;
			break;
		}
	}
	if (sel >= 0) {
		name = vars_list->GetItemText(sel, NAME_COL);
		time = vars_list->GetItemText(sel, TIME_COL);
	}
	wxString s("GetVarSel: ");
	s << "name = " << name << ", time = " << time << ", sel = " << sel;
	return sel;
}

void DataChangeTypeFrame::RefreshVars(wxListCtrl* vars_list,
									  GdaColListCtrl* data_list)
{
	if (!vars_list)
        return;
    
	ignore_callbacks = true;
	wxString name;
	wxString time;
	int sel = GetVarSel(vars_list, name, time);
    
	vars_list->DeleteAllItems();
	int num_cols = table_int->GetNumberCols();
	long item_cnt=0;
	bool found_prev_sel = false;
	for (int i=0; i<num_cols; ++i) {
		wxString i_name = table_int->GetColName(i);
		std::vector<wxString> tms;
		table_int->GetColNonPlaceholderTmStrs(i, tms);
		bool is_tm_variant = table_int->IsColTimeVariant(i);
		for (size_t t=0; t<tms.size(); ++t) {
			GdaConst::FieldType ft = table_int->GetColType(i, t);
			long x = vars_list->InsertItem(item_cnt, wxEmptyString);
			vars_list->SetItem(x, NAME_COL, i_name);
			vars_list->SetItem(x, TYPE_COL, GdaConst::FieldTypeToStr(ft));
			vars_list->SetItem(x, TIME_COL, (is_tm_variant ? tms[t] : ""));
			if (name == i_name &&
				(time == tms[t] || time == "" && t==0)) {
				vars_list->SetItemState(x, wxLIST_STATE_SELECTED,
										wxLIST_STATE_SELECTED);
				wxString s("RefreshVars reselecting item ");
				s << x;
				found_prev_sel = true;
			}
			++item_cnt;
		}
	}
	if (!found_prev_sel) {
		RefreshData(data_list, "", "");
	} else {
		RefreshData(data_list, name, time);
	}
	ignore_callbacks = false;
}

void DataChangeTypeFrame::RefreshData(GdaColListCtrl* data_list,
									  const wxString& name,
									  const wxString& time)
{
	if (!data_list) return;
	int tm = 0;
	int col = table_int->FindColId(name);
	if (col >= 0) {
		bool is_tm_variant = table_int->IsColTimeVariant(col);
		if (is_tm_variant) {
			tm = table_int->GetTimeInt(time);
			if (tm < 0) col = -1;
		} else {
			if (!time.IsEmpty() && table_int->GetTimeInt(time) != 0) col = -1;
		}
	}
	data_list->SetColNumAndTime(col, tm);
	data_list->Refresh();
}

GdaColListCtrl::GdaColListCtrl(TableInterface* table_int_,
							   wxWindow* parent, wxWindowID id,
							   const wxPoint& pos, const wxSize& size)
: wxListCtrl(parent, id, pos, size, wxLC_VIRTUAL|wxLC_REPORT),
table_int(table_int_), col(-1), tm(0)
{
}

void GdaColListCtrl::SetColNumAndTime(int column, int time)
{
	col = column;
	tm = time;
}

wxString GdaColListCtrl::OnGetItemText(long item, long column) const
{
	wxString r;
	if (col < 0) return "";
	if (column == DataChangeTypeFrame::DATA_ID_COL) {
		r << item+1;
		return r;
	}
	return table_int->GetCellString(item, col, tm);
}
