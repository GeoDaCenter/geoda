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

#include <cfloat>
#include <set>
#include <map>
#include <vector>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>

#include "../DataViewer/DataSource.h"
#include "../DataViewer/OGRTable.h"
#include "../DataViewer/OGRColumn.h"
#include "../DataViewer/TableInterface.h"
#include "../FramesManagerObserver.h"
#include "../FramesManager.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../Project.h"
#include "DissolveDlg.h"

BEGIN_EVENT_TABLE( DissolveDlg, wxDialog )
	EVT_BUTTON( XRCID("ID_INC_ALL_BUTTON"), DissolveDlg::OnIncAllClick )
	EVT_BUTTON( XRCID("ID_INC_ONE_BUTTON"), DissolveDlg::OnIncOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"), DissolveDlg::OnIncListDClick )
	EVT_BUTTON( XRCID("ID_EXCL_ALL_BUTTON"), DissolveDlg::OnExclAllClick )
	EVT_BUTTON( XRCID("ID_EXCL_ONE_BUTTON"), DissolveDlg::OnExclOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_EXCLUDE_LIST"), DissolveDlg::OnExclListDClick )
	EVT_CHOICE( XRCID("ID_CURRENT_KEY_CHOICE"), DissolveDlg::OnKeyChoice )
	EVT_CHOICE( XRCID("ID_IMPORT_KEY_CHOICE"), DissolveDlg::OnKeyChoice )
	EVT_BUTTON( XRCID("wxID_DISSOLVE"), DissolveDlg::OnOKClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), DissolveDlg::OnCloseClick )
    EVT_CLOSE( DissolveDlg::OnClose )
END_EVENT_TABLE()

using namespace std;

DissolveDlg::DissolveDlg(wxWindow* parent, Project* _project_s, const wxPoint& pos)
: project_s(_project_s), export_dlg(NULL)
{
    wxLogMessage("Open DissolveDlg.");
	SetParent(parent);
    
    table_int = project_s->GetTableInt(),
    frames_manager = project_s->GetFramesManager(),
    
	CreateControls();
	Init();
	wxString nm = _("Dissolve - ") + table_int->GetTableName();
	SetTitle(nm);
	SetPosition(pos);
    Centre();
    
	frames_manager->registerObserver(this);
}

DissolveDlg::~DissolveDlg()
{
    frames_manager->removeObserver(this);
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
}

void DissolveDlg::update(FramesManager* o)
{
}

void DissolveDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_DISSOLVE_DLG");
	m_current_key = wxDynamicCast(FindWindow(XRCID("ID_CURRENT_KEY_CHOICE")), wxChoice);
	m_exclude_list = wxDynamicCast(FindWindow(XRCID("ID_EXCLUDE_LIST")), GdaListBox);
	m_include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")), wxListBox);
	m_count = wxDynamicCast(FindWindow(XRCID("ID_DISSOLVE_COUNT")), wxRadioButton);
	m_sum = wxDynamicCast(FindWindow(XRCID("ID_DISSOLVE_SUM")), wxRadioButton);
	m_avg = wxDynamicCast(FindWindow(XRCID("ID_DISSOLVE_AVG")), wxRadioButton);
	m_min = wxDynamicCast(FindWindow(XRCID("ID_DISSOLVE_MIN")), wxRadioButton);
	m_max = wxDynamicCast(FindWindow(XRCID("ID_DISSOLVE_MAX")), wxRadioButton);
    m_inc_all = wxDynamicCast(FindWindow(XRCID("ID_INC_ALL_BUTTON")), wxButton);
    m_inc_one = wxDynamicCast(FindWindow( XRCID("ID_INC_ONE_BUTTON")), wxButton);
    m_exc_all = wxDynamicCast(FindWindow( XRCID("ID_EXCL_ALL_BUTTON")), wxButton);
    m_exc_one = wxDynamicCast(FindWindow( XRCID("ID_EXCL_ONE_BUTTON")), wxButton);
    
    wxScrolledWindow* win = wxDynamicCast(FindWindow( XRCID("ID_DISSOLVE_SCROLL_WIN")), wxScrolledWindow);
   
    win->SetAutoLayout(true);
    win->FitInside();
    win->SetScrollRate(5, 5);
    
    FitInside();
    
    m_count->Bind(wxEVT_RADIOBUTTON, &DissolveDlg::OnMethodSelect, this);
    m_sum->Bind(wxEVT_RADIOBUTTON, &DissolveDlg::OnMethodSelect, this);
    m_avg->Bind(wxEVT_RADIOBUTTON, &DissolveDlg::OnMethodSelect, this);
    m_min->Bind(wxEVT_RADIOBUTTON, &DissolveDlg::OnMethodSelect, this);
    m_max->Bind(wxEVT_RADIOBUTTON, &DissolveDlg::OnMethodSelect, this);
}

void DissolveDlg::Init()
{
    m_current_key->Clear();
    m_include_list->Clear();
    m_exclude_list->Clear();

    m_exclude_list->InitContent(table_int, GdaListBox::SHOW_STRING_INTEGER);

	vector<wxString> col_names;
	// get the field names from table interface
    set<wxString> key_name_set;
    set<wxString> field_name_set;
    int time_steps = table_int->GetTimeSteps();
    int n_fields   = table_int->GetNumberCols();
    for (size_t cid=0; cid<n_fields; cid++) {
        wxString group_name = table_int->GetColName(cid);
        for (size_t i=0; i<time_steps; i++) {
            GdaConst::FieldType field_type = table_int->GetColType(cid,i);
            wxString field_name = table_int->GetColName(cid, i);
            // only String, Integer can be keys for merging
            if (field_type == GdaConst::long64_type ||
                field_type == GdaConst::string_type )
            {
                if ( key_name_set.count(field_name) == 0) {
                    m_current_key->Append(field_name);
                    key_name_set.insert(field_name);
                }
            }
            if (field_type == GdaConst::long64_type || field_type == GdaConst::double_type ) {
                if ( field_name_set.count(field_name) == 0) {
                    //m_exclude_list->Append(field_name);
                    field_name_set.insert(field_name);
                }
            }
        }
    }
    
	UpdateMergeButton();
}


void DissolveDlg::OnMethodSelect( wxCommandEvent& ev)
{
    UpdateMergeButton();
}

void DissolveDlg::OnIncAllClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering DissolveDlg::OnIncAllClick()");
	for (int i=0, iend=m_exclude_list->GetCount(); i<iend; i++) {
		m_include_list->Append(m_exclude_list->GetString(i));
	}
	m_exclude_list->Clear();
	UpdateMergeButton();
}

void DissolveDlg::OnIncOneClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering DissolveDlg::OnIncOneClick()");
	if (m_exclude_list->GetSelection() >= 0) {
		wxString k = m_exclude_list->GetString(m_exclude_list->GetSelection());
		m_include_list->Append(k);
		m_exclude_list->Delete(m_exclude_list->GetSelection());
	}
	UpdateMergeButton();
}

void DissolveDlg::OnIncListDClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering DissolveDlg::OnIncListDClick()");
	OnExclOneClick(ev);
}

void DissolveDlg::OnExclAllClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering DissolveDlg::OnExclAllClick()");
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		m_exclude_list->Append(m_include_list->GetString(i));
	}
	m_include_list->Clear();
	UpdateMergeButton();
}

void DissolveDlg::OnExclOneClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering DissolveDlg::OnExclOneClick()");
	if (m_include_list->GetSelection() >= 0) {
		m_exclude_list->
			Append(m_include_list->GetString(m_include_list->GetSelection()));
		m_include_list->Delete(m_include_list->GetSelection());
	}
	UpdateMergeButton();
}

void DissolveDlg::OnExclListDClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering DissolveDlg::OnExclListDClick()");
    if (m_count->GetValue() == false) {
        OnIncOneClick(ev);
    }
}

bool DissolveDlg::CheckKeys(wxString key_name, vector<wxString>& key_vec, map<int, vector<int> >& key_map)
{
    std::map<wxString, std::vector<int> > dup_dict; // value:[]
    std::vector<wxString> uniq_fnames;
    
    for (int i=0, iend=key_vec.size(); i<iend; i++) {
        wxString tmpK = key_vec[i];
        tmpK.Trim(false);
        tmpK.Trim(true);
        if (dup_dict.find(tmpK) == dup_dict.end()) {
            dup_dict[tmpK].push_back(i);
            uniq_fnames.push_back(tmpK);
        } else {
            dup_dict[tmpK].push_back(i);
        }
    }
    if (key_vec.size() == dup_dict.size()) {
        wxString msg = wxString::Format(_("Your table cannot be aggregated because the key field \"%s\" is unique. Please use another key."), key_name);
        wxMessageDialog dlg(this, msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }
    for (int i=0; i<uniq_fnames.size(); i++) {
        key_map[i] = dup_dict[uniq_fnames[i]];
    }
    return true;
}


void DissolveDlg::OnOKClick( wxCommandEvent& ev )
{
    wxLogMessage("In DissolveDlg::OnOKClick()");
   
    try {
        wxString error_msg;
       
        // get selected field names from merging table
        vector<wxString> aggregate_field_names;
        for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
            wxString inc_n = m_include_list->GetString(i);
            aggregate_field_names.push_back(inc_n);
        }
        int n_rows = table_int->GetNumberRows();
        
        vector<wxString> key1_vec;
        map<int,vector<int> > key1_map;
        
        // get and check keys from original table
        int key1_id = m_current_key->GetSelection();
        wxString key1_name = m_current_key->GetString(key1_id);
        int col1_id = table_int->FindColId(key1_name);
        if (table_int->IsColTimeVariant(col1_id)) {
            error_msg = wxString::Format(_("Chosen key field '%s' s a time variant. Please choose a non-time variant field as key."), key1_name);
            throw GdaException(error_msg.mb_str());
        }
        
        vector<wxInt64>  key1_l_vec;
        GdaConst::FieldType key_ftype = table_int->GetColType(col1_id, 0);
        
        if (key_ftype == GdaConst::string_type) {
            table_int->GetColData(col1_id, 0, key1_vec);
        }else if (key_ftype==GdaConst::long64_type){
            table_int->GetColData(col1_id, 0, key1_l_vec);
        }
        
        if (key1_vec.empty()) { // convert everything (key) to wxString
            for( int i=0; i< key1_l_vec.size(); i++){
                wxString tmp;
                tmp << key1_l_vec[i];
                key1_vec.push_back(tmp);
            }
        }
        if (CheckKeys(key1_name, key1_vec, key1_map) == false)
            return;
        
        // Create in-memory geometries&table
        int new_rows = key1_map.size();
        OGRTable* mem_table = new OGRTable(new_rows);
        vector<bool> undefs(new_rows, true);
       
        int in_cols = aggregate_field_names.size();
        map<wxString, OGRColumn*> new_fields_dict;
        vector<wxString> new_fields;
    
        // create key column
        OGRColumn* key_col;
        if (key_ftype == GdaConst::string_type) {
            key_col = new OGRColumnString(key1_name, 50, 0, new_rows);
            for(int i=0; i<new_rows; i++) key_col->SetValueAt(i, key1_vec[key1_map[i][0]]);
        }else if (key_ftype==GdaConst::long64_type){
            key_col = new OGRColumnInteger(key1_name, 18, 0, new_rows);
            for(int i=0; i<new_rows; i++) key_col->SetValueAt(i, key1_l_vec[key1_map[i][0]]);
        }
        new_fields_dict[key1_name] = key_col;
        new_fields.push_back(key1_name);
        
        // create count column
        OGRColumn* _col = new OGRColumnInteger("AGG_COUNT", 18, 0, new_rows);
        for(int i=0; i<new_rows; i++)
            _col->SetValueAt(i, (wxInt64)(key1_map[i].size()));
        new_fields_dict[_col->GetName()] = _col;
        new_fields.push_back(_col->GetName());
        
        // get columns from table
        for ( int i=0; i < in_cols; i++ ) {
            wxString fname = aggregate_field_names[i];
            OGRColumn* col =  CreateNewOGRColumn(new_rows, table_int, key1_map, fname);
            new_fields_dict[fname] = col;
            new_fields.push_back(col->GetName());
        }
        
        for (int i=0; i<new_fields.size(); i++) {
            mem_table->AddOGRColumn(new_fields_dict[new_fields[i]]);
        }
        
        if (export_dlg != NULL) {
            export_dlg->Destroy();
            delete export_dlg;
        }
        export_dlg = new ExportDataDlg(this, mem_table);
        if (export_dlg->ShowModal() == wxID_OK) {
            wxMessageDialog dlg(this, _("Successful aggregation."), _("Success"), wxOK);
            dlg.ShowModal();
        }
        delete mem_table;
    } catch (GdaException& ex) {
        if (ex.type() == GdaException::NORMAL)
            return;
        wxMessageDialog dlg(this, ex.what(), _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
	ev.Skip();
}

double DissolveDlg::ComputeAgg(vector<double>& vals, vector<bool>& undefs, vector<int>& ids)
{
    if (m_sum->GetValue()) {
        double v_sum = 0;
        for (int i=0; i<ids.size(); i++) {
            int idx = ids[i];
            if (!undefs[idx]) {
                v_sum += vals[idx];
            }
        }
        return v_sum;
        
    } else if (m_avg->GetValue()) {
        double v_sum = 0;
        int n = 0;
        for (int i=0; i<ids.size(); i++) {
            int idx = ids[i];
            if (!undefs[idx]) {
                v_sum += vals[idx];
                n += 1;
            }
        }
        return n==0 ? 0 : v_sum / (double)n;
        
    } else if (m_max->GetValue()) {
        double v_max = DBL_MIN;
        for (int i=0; i<ids.size(); i++) {
            int idx = ids[i];
            if (!undefs[idx]) {
                if (vals[idx] > v_max)
                    v_max = vals[idx];
            }
        }
        return v_max;
    }
    
    else if (m_min->GetValue()) {
        double v_min = DBL_MAX;
        for (int i=0; i<ids.size(); i++) {
            int idx = ids[i];
            if (!undefs[idx]) {
                if (vals[idx] < v_min)
                    v_min = vals[idx];
            }
        }
        return v_min;
    }
	return 0;
}

OGRColumn* DissolveDlg::CreateNewOGRColumn(int new_rows, TableInterface* table_int, std::map<int, vector<int> >& key_map, wxString f_name)
{
    int idx = table_int->FindColId(f_name);
    int t = 0;
    int f_length = table_int->GetColLength(idx, 0);
    int f_decimal = table_int->GetColDecimals(idx, 0);
    GdaConst::FieldType f_type = table_int->GetColType(idx, 0);
    
    OGRColumn* _col;
    if (f_type == GdaConst::long64_type) {
        bool is_integer = false;
        if (m_max->GetValue() || m_min->GetValue() || m_sum->GetValue()) {
            _col = new OGRColumnInteger(f_name, f_length, f_decimal, new_rows);
            is_integer = true;
        } else 
            _col = new OGRColumnDouble(f_name, GdaConst::default_dbf_double_len, GdaConst::default_dbf_double_decimals, new_rows);
            
        vector<double> vals;
        vector<bool> undefs;
        table_int->GetColData(idx, t, vals, undefs);
        
        for(int i=0; i<new_rows; i++) {
            vector<int>& ids = key_map[i];
            double v = ComputeAgg(vals, undefs, ids);
            if (is_integer) {
                wxInt64 vv = v;
                _col->SetValueAt(i, vv);
            } else
                _col->SetValueAt(i, v);
        }
        
    } else if (f_type == GdaConst::double_type) {
        _col = new OGRColumnDouble(f_name, f_length, f_decimal, new_rows);
        vector<double> vals;
        vector<bool> undefs;
        table_int->GetColData(idx, t, vals, undefs);
        for(int i=0; i<new_rows; i++) {
            vector<int>& ids = key_map[i];
            double v = ComputeAgg(vals, undefs, ids);
            _col->SetValueAt(i, v);
        }
        
    }
    return _col;
}

void DissolveDlg::OnCloseClick( wxCommandEvent& ev )
{
    wxLogMessage("In DissolveDlg::OnCloseClick()");
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
	EndDialog(wxID_CLOSE);
}

void DissolveDlg::OnClose( wxCloseEvent& ev)
{
    wxLogMessage("In DissolveDlg::OnClose()");
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
    Destroy();
}

void DissolveDlg::OnKeyChoice( wxCommandEvent& ev )
{
    wxLogMessage("In DissolveDlg::OnKeyChoice()");
	UpdateMergeButton();
}

void DissolveDlg::UpdateMergeButton()
{
	bool enable = m_count->GetValue() || (!m_include_list->IsEmpty() && m_current_key->GetSelection() != wxNOT_FOUND);
	FindWindow(XRCID("wxID_DISSOLVE"))->Enable(enable);
   
    m_inc_all->Enable(!m_count->GetValue());
    m_inc_one->Enable(!m_count->GetValue());
    m_exc_all->Enable(!m_count->GetValue());
    m_exc_one->Enable(!m_count->GetValue());
}
