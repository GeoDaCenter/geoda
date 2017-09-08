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
#include "AggregateDlg.h"
#include "DataSource.h"
#include "DbfColContainer.h"
#include "TableBase.h"
#include "TableInterface.h"
#include "../FramesManagerObserver.h"
#include "../FramesManager.h"
#include "../ShapeOperations/OGRLayerProxy.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../DialogTools/ConnectDatasourceDlg.h"
#include "../DialogTools/FieldNameCorrectionDlg.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../Project.h"

BEGIN_EVENT_TABLE( AggregationDlg, wxDialog )

	EVT_BUTTON( XRCID("ID_INC_ALL_BUTTON"), AggregationDlg::OnIncAllClick )
	EVT_BUTTON( XRCID("ID_INC_ONE_BUTTON"), AggregationDlg::OnIncOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"), AggregationDlg::OnIncListDClick )
	EVT_BUTTON( XRCID("ID_EXCL_ALL_BUTTON"), AggregationDlg::OnExclAllClick )
	EVT_BUTTON( XRCID("ID_EXCL_ONE_BUTTON"), AggregationDlg::OnExclOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_EXCLUDE_LIST"), AggregationDlg::OnExclListDClick )
	EVT_CHOICE( XRCID("ID_CURRENT_KEY_CHOICE"), AggregationDlg::OnKeyChoice )
	EVT_CHOICE( XRCID("ID_IMPORT_KEY_CHOICE"), AggregationDlg::OnKeyChoice )
	EVT_BUTTON( XRCID("wxID_OK"), AggregationDlg::OnOKClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), AggregationDlg::OnCloseClick )
    EVT_CLOSE( AggregationDlg::OnClose )
END_EVENT_TABLE()

using namespace std;

AggregationDlg::AggregationDlg(wxWindow* parent, Project* _project_s, const wxPoint& pos)
: project_s(_project_s), export_dlg(NULL)
{
    wxLogMessage("Open AggregationDlg.");
	SetParent(parent);
    
	//table_int->FillColIdMap(col_id_map);
    table_int = project_s->GetTableInt(),
    frames_manager = project_s->GetFramesManager(),
    
	CreateControls();
	Init();
	wxString nm;
	SetTitle(_("Aggregate - ") + table_int->GetTableName());
	SetPosition(pos);
    Centre();
    
	frames_manager->registerObserver(this);
}

AggregationDlg::~AggregationDlg()
{
    frames_manager->removeObserver(this);
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
}

void AggregationDlg::update(FramesManager* o)
{
}

void AggregationDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_AGGREGATE_DATA_DLG");
	//m_input_file_name = wxDynamicCast(FindWindow(XRCID("ID_INPUT_FILE_TEXT")), wxTextCtrl);
	//m_key_val_rb = wxDynamicCast(FindWindow(XRCID("ID_KEY_VAL_RB")), wxRadioButton);
	//m_rec_order_rb = wxDynamicCast(FindWindow(XRCID("ID_REC_ORDER_RB")), wxRadioButton);
	m_current_key = wxDynamicCast(FindWindow(XRCID("ID_CURRENT_KEY_CHOICE")), wxChoice);
	//m_import_key = wxDynamicCast(FindWindow(XRCID("ID_IMPORT_KEY_CHOICE")), wxChoice);
	m_exclude_list = wxDynamicCast(FindWindow(XRCID("ID_EXCLUDE_LIST")), wxListBox);
	m_include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")), wxListBox);
	m_sum = wxDynamicCast(FindWindow(XRCID("ID_AGGREGATE_SUM")), wxRadioButton);
	m_avg = wxDynamicCast(FindWindow(XRCID("ID_AGGREGATE_AVG")), wxRadioButton);
	m_min = wxDynamicCast(FindWindow(XRCID("ID_AGGREGATE_MIN")), wxRadioButton);
	m_max = wxDynamicCast(FindWindow(XRCID("ID_AGGREGATE_MAX")), wxRadioButton);
	//m_overwrite_field = wxDynamicCast(FindWindow(XRCID("ID_MERGE_OVERWRITE_SAME_FIELD")), wxCheckBox);
}

void AggregationDlg::Init()
{
    m_current_key->Clear();
    m_include_list->Clear();
    m_exclude_list->Clear();
   
	vector<wxString> col_names;
	// get the field names from table interface
    set<wxString> key_name_set;
    set<wxString> field_name_set;
    int time_steps = table_int->GetTimeSteps();
    int n_fields   = table_int->GetNumberCols();
    for (int cid=0; cid<n_fields; cid++) {
        wxString group_name = table_int->GetColName(cid);
        for (int i=0; i<time_steps; i++) {
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
                    m_exclude_list->Append(field_name);
                    field_name_set.insert(field_name);
                }
            }
        }
    }
	UpdateMergeButton();
}


void AggregationDlg::OnIncAllClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering AggregationDlg::OnIncAllClick()");
	for (int i=0, iend=m_exclude_list->GetCount(); i<iend; i++) {
		m_include_list->Append(m_exclude_list->GetString(i));
	}
	m_exclude_list->Clear();
	UpdateMergeButton();
}

void AggregationDlg::OnIncOneClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering AggregationDlg::OnIncOneClick()");
	if (m_exclude_list->GetSelection() >= 0) {
		wxString k = m_exclude_list->GetString(m_exclude_list->GetSelection());
		m_include_list->Append(k);
		m_exclude_list->Delete(m_exclude_list->GetSelection());
	}
	UpdateMergeButton();
}

void AggregationDlg::OnIncListDClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering AggregationDlg::OnIncListDClick()");
	OnExclOneClick(ev);
}

void AggregationDlg::OnExclAllClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering AggregationDlg::OnExclAllClick()");
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		m_exclude_list->Append(m_include_list->GetString(i));
	}
	m_include_list->Clear();
	UpdateMergeButton();
}

void AggregationDlg::OnExclOneClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering AggregationDlg::OnExclOneClick()");
	if (m_include_list->GetSelection() >= 0) {
		m_exclude_list->
			Append(m_include_list->GetString(m_include_list->GetSelection()));
		m_include_list->Delete(m_include_list->GetSelection());
	}
	UpdateMergeButton();
}

void AggregationDlg::OnExclListDClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering AggregationDlg::OnExclListDClick()");
	OnIncOneClick(ev);
}

bool AggregationDlg::CheckKeys(wxString key_name, vector<wxString>& key_vec, map<int, vector<int> >& key_map)
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

vector<wxString> AggregationDlg::GetSelectedFieldNames(map<wxString,wxString>& merged_fnames_dict)
{
    vector<wxString> aggregate_field_names;

    for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
        wxString inc_n = m_include_list->GetString(i);
        aggregate_field_names.push_back(inc_n);
    }
    
    return aggregate_field_names;
}

void AggregationDlg::OnOKClick( wxCommandEvent& ev )
{
    wxLogMessage("In AggregationDlg::OnOKClick()");
   
    try {
        wxString error_msg;
        
        // get selected field names from merging table
        vector<wxString> aggregate_field_names;
        for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
            wxString inc_n = m_include_list->GetString(i);
            aggregate_field_names.push_back(inc_n);
        }
        if (aggregate_field_names.empty())
            return;
        
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
        for(int i=0; i<new_rows; i++) _col->SetValueAt(i, (wxInt64)(key1_map[i].size()));
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
            wxMessageDialog dlg(this, _("File aggregate into Table successfully."), _("Success"), wxOK);
            dlg.ShowModal();
            EndDialog(wxID_OK);
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

double AggregationDlg::ComputeAgg(vector<double>& vals, vector<bool>& undefs, vector<int>& ids)
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
}

OGRColumn* AggregationDlg::CreateNewOGRColumn(int new_rows, TableInterface* table_int, std::map<int, vector<int> >& key_map, wxString f_name)
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

void AggregationDlg::OnCloseClick( wxCommandEvent& ev )
{
    wxLogMessage("In AggregationDlg::OnCloseClick()");
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
	EndDialog(wxID_CLOSE);
}

void AggregationDlg::OnClose( wxCloseEvent& ev)
{
    wxLogMessage("In AggregationDlg::OnClose()");
    if (export_dlg) {
        export_dlg->Destroy();
        delete export_dlg;
        export_dlg = NULL;
    }
    Destroy();
}

void AggregationDlg::OnKeyChoice( wxCommandEvent& ev )
{
    wxLogMessage("In AggregationDlg::OnKeyChoice()");
	UpdateMergeButton();
}

void AggregationDlg::UpdateMergeButton()
{
	bool enable = !m_include_list->IsEmpty() &&
                   m_current_key->GetSelection() != wxNOT_FOUND;
	FindWindow(XRCID("wxID_OK"))->Enable(enable);
}
