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

#include <set>
#include <map>
#include <vector>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include "MergeTableDlg.h"
#include "DataSource.h"
#include "DbfColContainer.h"
#include "TableBase.h"
#include "TableInterface.h"
#include "../FramesManagerObserver.h"
#include "../FramesManager.h"
#include "../DbfFile.h"
#include "../ShapeOperations/OGRLayerProxy.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../DialogTools/ConnectDatasourceDlg.h"
#include "../DialogTools/FieldNameCorrectionDlg.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../Project.h"

BEGIN_EVENT_TABLE( MergeTableDlg, wxDialog )

	EVT_RADIOBUTTON( XRCID("ID_KEY_VAL_RB"), MergeTableDlg::OnKeyValRB )
	EVT_RADIOBUTTON( XRCID("ID_REC_ORDER_RB"), MergeTableDlg::OnRecOrderRB )
	EVT_BUTTON( XRCID("ID_OPEN_BUTTON"), MergeTableDlg::OnOpenClick )
	EVT_BUTTON( XRCID("ID_INC_ALL_BUTTON"), MergeTableDlg::OnIncAllClick )
	EVT_BUTTON( XRCID("ID_INC_ONE_BUTTON"), MergeTableDlg::OnIncOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"), MergeTableDlg::OnIncListDClick )
	EVT_BUTTON( XRCID("ID_EXCL_ALL_BUTTON"), MergeTableDlg::OnExclAllClick )
	EVT_BUTTON( XRCID("ID_EXCL_ONE_BUTTON"), MergeTableDlg::OnExclOneClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_EXCLUDE_LIST"), MergeTableDlg::OnExclListDClick )
	EVT_CHOICE( XRCID("ID_CURRENT_KEY_CHOICE"), MergeTableDlg::OnKeyChoice )
	EVT_CHOICE( XRCID("ID_IMPORT_KEY_CHOICE"), MergeTableDlg::OnKeyChoice )
	EVT_BUTTON( XRCID("wxID_MERGE"), MergeTableDlg::OnMergeClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), MergeTableDlg::OnCloseClick )
    EVT_CLOSE( MergeTableDlg::OnClose )
END_EVENT_TABLE()

using namespace std;

MergeTableDlg::MergeTableDlg(wxWindow* parent, Project* _project_s, const wxPoint& pos)
: merge_datasource_proxy(NULL), project_s(_project_s)
{
    wxLogMessage("Open MergeTableDlg.");
	SetParent(parent);
    
	//table_int->FillColIdMap(col_id_map);
    table_int = project_s->GetTableInt(),
    frames_manager = project_s->GetFramesManager(),
    
	CreateControls();
	Init();
	wxString nm;
	SetTitle(_("Merge - ") + table_int->GetTableName());
	SetPosition(pos);
    Centre();
    
	frames_manager->registerObserver(this);
}

MergeTableDlg::~MergeTableDlg()
{
    if (merge_datasource_proxy) {
        delete merge_datasource_proxy;
        merge_datasource_proxy = NULL;
    }
	
    frames_manager->removeObserver(this);
}

void MergeTableDlg::update(FramesManager* o)
{
}

void MergeTableDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_MERGE_TABLE_DLG");
	m_input_file_name = wxDynamicCast(FindWindow(XRCID("ID_INPUT_FILE_TEXT")), wxTextCtrl);
	m_key_val_rb = wxDynamicCast(FindWindow(XRCID("ID_KEY_VAL_RB")), wxRadioButton);
	m_rec_order_rb = wxDynamicCast(FindWindow(XRCID("ID_REC_ORDER_RB")), wxRadioButton);
	m_current_key = wxDynamicCast(FindWindow(XRCID("ID_CURRENT_KEY_CHOICE")), wxChoice);
	m_import_key = wxDynamicCast(FindWindow(XRCID("ID_IMPORT_KEY_CHOICE")), wxChoice);
	m_exclude_list = wxDynamicCast(FindWindow(XRCID("ID_EXCLUDE_LIST")), wxListBox);
	m_include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")), wxListBox);
	m_left_join = wxDynamicCast(FindWindow(XRCID("ID_MERGE_LEFT_JOIN")), wxRadioButton);
	m_outer_join = wxDynamicCast(FindWindow(XRCID("ID_MERGE_OUTER_JOIN")), wxRadioButton);
	m_overwrite_field = wxDynamicCast(FindWindow(XRCID("ID_MERGE_OVERWRITE_SAME_FIELD")), wxCheckBox);
    
    m_left_join->Bind(wxEVT_RADIOBUTTON, &MergeTableDlg::OnLeftJoinClick, this);
    m_outer_join->Bind(wxEVT_RADIOBUTTON, &MergeTableDlg::OnOuterJoinClick, this);
   
    m_left_join->Disable();
    m_outer_join->Disable();
    m_key_val_rb->Disable();
    m_rec_order_rb->Disable();
    m_current_key->Disable();
    m_import_key->Disable();
    m_exclude_list->Disable();
    m_include_list->Disable();
    m_overwrite_field->Disable();
}

void MergeTableDlg::OnLeftJoinClick(wxCommandEvent& ev)
{
    m_overwrite_field->Disable();
    m_overwrite_field->SetValue(false);
    m_rec_order_rb->SetValue(false);
    m_key_val_rb->SetValue(true);
}

void MergeTableDlg::OnOuterJoinClick(wxCommandEvent& ev)
{
    m_overwrite_field->Enable();
    m_overwrite_field->SetValue(true);
    m_rec_order_rb->SetValue(true);
    m_key_val_rb->SetValue(false);
}

void MergeTableDlg::Init()
{
    m_input_file_name->SetValue(wxEmptyString);
    m_import_key->Clear();
    m_current_key->Clear();
    m_include_list->Clear();
    m_exclude_list->Clear();
   
    m_left_join->Disable();
    m_outer_join->Disable();
    m_key_val_rb->Disable();
    m_rec_order_rb->Disable();
    m_current_key->Disable();
    m_import_key->Disable();
    m_exclude_list->Disable();
    m_include_list->Disable();
    
	vector<wxString> col_names;
	table_fnames.clear();
	// get the field names from table interface
    set<wxString> field_name_set;
    int time_steps = table_int->GetTimeSteps();
    int n_fields   = table_int->GetNumberCols();
    for (int cid=0; cid<n_fields; cid++) {
        wxString group_name = table_int->GetColName(cid);
        table_fnames.insert(group_name);
        for (int i=0; i<time_steps; i++) {
            GdaConst::FieldType field_type = table_int->GetColType(cid,i);
            wxString field_name = table_int->GetColName(cid, i);
            // only String, Integer can be keys for merging
            if ( field_type == GdaConst::long64_type ||
                field_type == GdaConst::string_type )
            {
                if ( field_name_set.count(field_name) == 0) {
                    m_current_key->Append(field_name);
                    field_name_set.insert(field_name);
                }
            }
            table_fnames.insert(field_name);
        }
    }
	UpdateMergeButton();
}

void MergeTableDlg::OnKeyValRB( wxCommandEvent& ev )
{
	UpdateMergeButton();
}

void MergeTableDlg::OnRecOrderRB( wxCommandEvent& ev )
{
	UpdateMergeButton();
}

void MergeTableDlg::OnOpenClick( wxCommandEvent& ev )
{
    wxLogMessage("Entering MergeTableDlg::OnOpenClick()");
    try {
        bool showCsvConfigure = GdaConst::show_csv_configure_in_merge;
        wxPoint pos = GetPosition();
        wxSize sz = GetSize();
        pos.x += sz.GetWidth();
       
        int dialog_type = 1;
        bool showRecentPanel = false;
        ConnectDatasourceDlg connect_dlg(this, pos, wxDefaultSize, showCsvConfigure, showRecentPanel, dialog_type);
        
        if (connect_dlg.ShowModal() != wxID_OK) {
            return;
        }
        
        wxString proj_title = connect_dlg.GetProjectTitle();
        wxString layer_name = connect_dlg.GetLayerName();
        IDataSource* datasource = connect_dlg.GetDataSource();
        wxString datasource_name = datasource->GetOGRConnectStr();
        GdaConst::DataSourceType ds_type = datasource->GetType();
       
        wxLogMessage(_("ds:") + datasource_name + _(" layer: ") + layer_name);
        
        if (merge_datasource_proxy != NULL) {
            delete merge_datasource_proxy;
            merge_datasource_proxy = NULL;
        }
        merge_datasource_proxy = new OGRDatasourceProxy(datasource_name, ds_type, true);
        merge_layer_proxy = merge_datasource_proxy->GetLayerProxy(layer_name.ToStdString());
        merge_layer_proxy->ReadData();
        m_input_file_name->SetValue(layer_name);
        
        // get the unique field names, and fill to m_import_key (wxChoice)
        map<wxString, int> dbf_fn_freq;
        dups.clear();
        dedup_to_id.clear();
        
        //m_input_file_name->SetValue(wxEmptyString);
        m_import_key->Clear();
        m_include_list->Clear();
        m_exclude_list->Clear();
        
        for (int i=0, iend=merge_layer_proxy->GetNumFields(); i<iend; i++) {
            GdaConst::FieldType field_type = merge_layer_proxy->GetFieldType(i);
            wxString name = merge_layer_proxy->GetFieldName(i);
            wxString dedup_name = name;
            if (dbf_fn_freq.find(name) != dbf_fn_freq.end()) {
                dedup_name << " (" << dbf_fn_freq[name]++ << ")";
            } else {
                dbf_fn_freq[name] = 1;
            }
            dups.insert(dedup_name);
            dedup_to_id[dedup_name] = i; // map to DBF col id
            if ( field_type == GdaConst::long64_type ||
                 field_type == GdaConst::string_type )
            {
                m_import_key->Append(dedup_name);
            }
            m_exclude_list->Append(dedup_name);
        }
        
        m_left_join->Enable();
        m_outer_join->Enable();
        m_key_val_rb->Enable();
        m_rec_order_rb->Enable();
        m_current_key->Enable();
        m_import_key->Enable();
        m_exclude_list->Enable();
        m_include_list->Enable();
        
    }catch(GdaException& e) {
        wxMessageDialog dlg (this, e.what(), _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
}

void MergeTableDlg::OnIncAllClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering MergeTableDlg::OnIncAllClick()");
	for (int i=0, iend=m_exclude_list->GetCount(); i<iend; i++) {
		m_include_list->Append(m_exclude_list->GetString(i));
	}
	m_exclude_list->Clear();
	UpdateMergeButton();
}

void MergeTableDlg::OnIncOneClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering MergeTableDlg::OnIncOneClick()");
	if (m_exclude_list->GetSelection() >= 0) {
		wxString k = m_exclude_list->GetString(m_exclude_list->GetSelection());
		m_include_list->Append(k);
		m_exclude_list->Delete(m_exclude_list->GetSelection());
	}
	UpdateMergeButton();
}

void MergeTableDlg::OnIncListDClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering MergeTableDlg::OnIncListDClick()");
	OnExclOneClick(ev);
}

void MergeTableDlg::OnExclAllClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering MergeTableDlg::OnExclAllClick()");
	for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
		m_exclude_list->Append(m_include_list->GetString(i));
	}
	m_include_list->Clear();
	UpdateMergeButton();
}

void MergeTableDlg::OnExclOneClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering MergeTableDlg::OnExclOneClick()");
	if (m_include_list->GetSelection() >= 0) {
		m_exclude_list->
			Append(m_include_list->GetString(m_include_list->GetSelection()));
		m_include_list->Delete(m_include_list->GetSelection());
	}
	UpdateMergeButton();
}

void MergeTableDlg::OnExclListDClick( wxCommandEvent& ev)
{
    wxLogMessage("Entering MergeTableDlg::OnExclListDClick()");
	OnIncOneClick(ev);
}

bool MergeTableDlg::CheckKeys(wxString key_name, vector<wxString>& key_vec,
                              map<wxString, int>& key_map)
{
    std::map<wxString, std::vector<int> > dup_dict; // value:[]
	
    for (int i=0, iend=key_vec.size(); i<iend; i++) {
        wxString tmpK = key_vec[i];
        tmpK.Trim(false);
        tmpK.Trim(true);
        if (key_map.find(tmpK) == key_map.end()) {
            key_map[tmpK] = i;
            std::vector<int> ids;
            dup_dict[tmpK] = ids;
        }
        dup_dict[tmpK].push_back(i);
    }
	
    if (key_vec.size() != dup_dict.size()) {
        wxString msg = wxString::Format(_("Your table cannot be merged because the key field \"%s\" is not unique. \nIt contains undefined or duplicate values.\n\nDetails:"), key_name);
        
        wxString details = "value, row\n";
        std::map<wxString, std::vector<int> >::iterator it;
        for (it=dup_dict.begin(); it!=dup_dict.end(); it++) {
            wxString val = it->first;
            std::vector<int>& ids = it->second;
            if (ids.size() > 1) {
                for (int i=0; i<ids.size(); i++) {
                    details << val << ", " << ids[i]+1 << "\n";
                }
            }
        }
        
        ScrolledDetailMsgDialog *dlg = new ScrolledDetailMsgDialog("Warning", msg, details);
        dlg->Show(true);
        return false;
    }
    return true;
}

vector<wxString> MergeTableDlg::
GetSelectedFieldNames(map<wxString,wxString>& merged_fnames_dict)
{
    vector<wxString> merged_field_names;
    set<wxString> dup_merged_field_names, bad_merged_field_names;

    for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
        wxString inc_n = m_include_list->GetString(i);
        merged_field_names.push_back(inc_n);
        
        if (!table_int->IsValidDBColName(inc_n)) {
            bad_merged_field_names.insert(inc_n);
        } else {
            // Detect duplicate field names
            std::set<wxString>::iterator it;
            for (it = table_fnames.begin(); it != table_fnames.end(); it ++) {
                wxString nm = *it;
                if (nm.Upper() == inc_n.Upper()) {
                    dup_merged_field_names.insert(inc_n);
                    break;
                }
            }
        }
    }
    
    if ( bad_merged_field_names.size() + dup_merged_field_names.size() > 0) {
        // show a field name correction dialog
        GdaConst::DataSourceType ds_type = table_int->GetDataSourceType();
        FieldNameCorrectionDlg fc_dlg(ds_type,
                                      merged_fnames_dict,
                                      merged_field_names,
                                      dup_merged_field_names,
                                      bad_merged_field_names);
        if ( fc_dlg.ShowModal() != wxID_OK ) 
            merged_field_names.clear();
        else
            merged_fnames_dict = fc_dlg.GetMergedFieldNameDict();
    }
    return merged_field_names;
}

void MergeTableDlg::OnMergeClick( wxCommandEvent& ev )
{
    wxLogMessage("In MergeTableDlg::OnMergeClick()");
    if (m_left_join->GetValue()) {
        LeftJoinMerge();
    } else {
        OuterJoinMerge();
    }
	ev.Skip();
}

OGRColumn* MergeTableDlg::CreateNewOGRColumn(int new_rows, TableInterface* table_int, vector<bool>& undefs, int idx, int t)
{
    wxString f_name = table_int->GetColName(idx, t);
    int f_length = table_int->GetColLength(idx, t);
    int f_decimal = table_int->GetColDecimals(idx, t);
    GdaConst::FieldType f_type = table_int->GetColType(idx, t);
    
    OGRColumn* _col;
    if (f_type == GdaConst::long64_type) {
        _col = new OGRColumnInteger(f_name, f_length, f_decimal, new_rows);
        _col->SetUndefinedMarkers(undefs);
        vector<wxInt64> vals;
        table_int->GetColData(idx, t, vals);
        for(int i=0; i<vals.size(); i++) _col->SetValueAt(i, vals[i]);
    } else if (f_type == GdaConst::double_type) {
        _col = new OGRColumnDouble(f_name, f_length, f_decimal, new_rows);
        _col->SetUndefinedMarkers(undefs);
        vector<double> vals;
        table_int->GetColData(idx, t, vals);
        for(int i=0; i<vals.size(); i++) _col->SetValueAt(i, vals[i]);
    } else {
        _col = new OGRColumnString(f_name, f_length, f_decimal, new_rows);
        _col->SetUndefinedMarkers(undefs);
        vector<wxString> vals;
        table_int->GetColData(idx, t, vals);
        for(int i=0; i<vals.size(); i++) _col->SetValueAt(i, vals[i]);
    }
    return _col;
}

OGRColumn* MergeTableDlg::CreateNewOGRColumn(int new_rows, OGRLayerProxy* layer_proxy, vector<bool>& undefs, wxString f_name, map<int, int>& idx2_dict)
{
    int col_idx = layer_proxy->GetFieldPos(f_name);
    GdaConst::FieldType f_type = layer_proxy->GetFieldType(col_idx);
    int f_length = layer_proxy->GetFieldLength(col_idx);
    int f_decimal = layer_proxy->GetFieldDecimals(col_idx);
    int n_rows = layer_proxy->n_rows;
    
    OGRColumn* _col;
    if (f_type == GdaConst::long64_type) {
        _col = new OGRColumnInteger(f_name, f_length, f_decimal, new_rows);
        _col->SetUndefinedMarkers(undefs);
        for (int i=0; i<n_rows; i++) {
            OGRFeature* feat = layer_proxy->GetFeatureAt(i);
            wxInt64 val = feat->GetFieldAsInteger64(col_idx);
            _col->SetValueAt(idx2_dict[i], val);
        }
    } else if (f_type == GdaConst::double_type) {
        _col = new OGRColumnDouble(f_name, f_length, f_decimal, new_rows);
        _col->SetUndefinedMarkers(undefs);
        for (int i=0; i<n_rows; i++) {
            OGRFeature* feat = layer_proxy->GetFeatureAt(i);
            double val = feat->GetFieldAsDouble(col_idx);
            _col->SetValueAt(idx2_dict[i], val);
        }
    } else {
        _col = new OGRColumnString(f_name, f_length, f_decimal, new_rows);
        _col->SetUndefinedMarkers(undefs);
        for (int i=0; i<n_rows; i++) {
            wxString val = layer_proxy->GetValueAt(i, col_idx);
            _col->SetValueAt(idx2_dict[i], val);
        }
    }
    return _col;
}

void MergeTableDlg::UpdateOGRColumn(OGRColumn* _col, OGRLayerProxy* layer_proxy, wxString f_name, map<int, int>& idx2_dict)
{
    int col_idx = layer_proxy->GetFieldPos(f_name);
    GdaConst::FieldType f_type = layer_proxy->GetFieldType(col_idx);
    int f_length = layer_proxy->GetFieldLength(col_idx);
    int f_decimal = layer_proxy->GetFieldDecimals(col_idx);
    int n_rows = layer_proxy->n_rows;
    
    if (f_type == GdaConst::long64_type) {
        for (int i=0; i<n_rows; i++) {
            OGRFeature* feat = layer_proxy->GetFeatureAt(i);
            wxInt64 val = feat->GetFieldAsInteger64(col_idx);
            _col->SetValueAt(idx2_dict[i], val);
        }
    } else if (f_type == GdaConst::double_type) {
        for (int i=0; i<n_rows; i++) {
            OGRFeature* feat = layer_proxy->GetFeatureAt(i);
            double val = feat->GetFieldAsDouble(col_idx);
            _col->SetValueAt(idx2_dict[i], val);
        }
    } else {
        for (int i=0; i<n_rows; i++) {
            wxString val = layer_proxy->GetValueAt(i, col_idx);
            _col->SetValueAt(idx2_dict[i], val);
        }
    }
}

void MergeTableDlg::OuterJoinMerge()
{
    try {
        wxString error_msg;
        
        // get selected field names from merging table
        vector<wxString> merged_field_names;
        for (int i=0, iend=m_include_list->GetCount(); i<iend; i++) {
            wxString inc_n = m_include_list->GetString(i);
            merged_field_names.push_back(inc_n);
        }
        if (merged_field_names.empty())
            return;
        
        int n_rows = table_int->GetNumberRows();
        int n_merge_field = merged_field_names.size();
        
        map<int, int> rowid_map;
        
        vector<wxString> key1_vec;
        map<wxString,int> key1_map;
        vector<wxString> key2_vec;
        
        if (m_key_val_rb->GetValue()==1) { // check merge by key/record order
            // get and check keys from original table
            int key1_id = m_current_key->GetSelection();
            wxString key1_name = m_current_key->GetString(key1_id);
            int col1_id = table_int->FindColId(key1_name);
            if (table_int->IsColTimeVariant(col1_id)) {
                error_msg = wxString::Format(_("Chosen key field '%s' s a time variant. Please choose a non-time variant field as key."), key1_name);
                throw GdaException(error_msg.mb_str());
            }
            
            vector<wxInt64>  key1_l_vec;
            
            if (table_int->GetColType(col1_id, 0) == GdaConst::string_type) {
                table_int->GetColData(col1_id, 0, key1_vec);
            }else if (table_int->GetColType(col1_id,0)==GdaConst::long64_type){
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
            
            // get and check keys from import table
            int key2_id = m_import_key->GetSelection();
            wxString key2_name = m_import_key->GetString(key2_id);
            int col2_id = merge_layer_proxy->GetFieldPos(key2_name);
            int n_merge_rows = merge_layer_proxy->GetNumRecords();
            map<wxString,int> key2_map;
            for (int i=0; i < n_merge_rows; i++) {
                wxString tmp;
                tmp << merge_layer_proxy->GetValueAt(i, col2_id);
                key2_vec.push_back(tmp);
            }
            if (CheckKeys(key2_name, key2_vec, key2_map) == false)
                return;
            
        } else if (m_rec_order_rb->GetValue() == 1) { // merge by order sequence, just append
           
            for (int i=0; i<n_rows; i++) {
                wxString tmp;
                tmp << i;
                key1_vec.push_back(tmp);
            }
            int n_merge_rows = merge_layer_proxy->GetNumRecords();
            for (int i=0; i < n_merge_rows; i++) {
                wxString tmp;
                tmp << i;
                key2_vec.push_back(tmp);
            }
        }
        
        std::vector<GdaShape*> geoms;
        OGRSpatialReference* spatial_ref = project_s->GetSpatialReference();
        Shapefile::ShapeType shape_type = project_s->GetGdaGeometries(geoms);
        
        std::vector<GdaShape*> in_geoms;
        OGRSpatialReference* in_spatial_ref = merge_layer_proxy->GetSpatialReference();
        
        OGRCoordinateTransformation *poCT = NULL;
        if (spatial_ref !=NULL && in_spatial_ref != NULL) {
            if (!spatial_ref->IsSame(in_spatial_ref) ) {
                // convert geometry with original projection if needed
                poCT = OGRCreateCoordinateTransformation(in_spatial_ref, spatial_ref);
                merge_layer_proxy->ApplyProjection(poCT);
            }
        }
        Shapefile::ShapeType in_shape_type=merge_layer_proxy->GetGdaGeometries(in_geoms);
        if (shape_type != in_shape_type) {
            error_msg = _("Merge error: Geometric type of selected datasource has to be the same with current datasource.");
            throw GdaException(error_msg.mb_str());
        }
        
        std::vector<GdaShape*> new_geoms = geoms;
        
        vector<wxString> new_key_vec = key1_vec;
        map<int, int> idx2_dict;
        int idx2 = key1_vec.size();
        for (int i=0; i<key2_vec.size(); i++) {
            wxString tmp = key2_vec[i];
            if (key1_map.find(tmp) == key1_map.end()) {
                new_key_vec.push_back(tmp);
                new_geoms.push_back(in_geoms[i]);
                idx2_dict[i] = idx2++;
            } else {
                idx2_dict[i] = key1_map[tmp];
            }
        }
        
        // Create in-memory geometries&table
        int new_rows = new_key_vec.size();
        OGRTable* mem_table = new OGRTable(new_rows);
        vector<bool> undefs(new_rows, true);
        
        map<wxString, OGRColumn*> new_fields_dict;
        vector<wxString> new_fields;
        // all columns from table
        int time_steps = table_int->GetTimeSteps();
        for ( int id=0; id < table_int->GetNumberCols(); id++ ) {
            OGRColumn* col;
            if (table_int->IsColTimeVariant(id)) {
                for ( int t=0; t < time_steps; t++ ) {
                    col =  CreateNewOGRColumn(new_rows, table_int, undefs, id, t);
                    new_fields_dict[col->GetName()] = col;
                    new_fields.push_back(col->GetName());
                }
            } else {
                col =  CreateNewOGRColumn(new_rows, table_int, undefs, id);
                new_fields_dict[col->GetName()] = col;
                new_fields.push_back(col->GetName());
            }
        }
        // all columns from datasource
        int in_cols = merged_field_names.size();
        bool overwrite_field = m_overwrite_field->IsChecked();
        
        for (int i=0; i<in_cols; i++) {
            wxString fname = merged_field_names[i];
            if (new_fields_dict.find(fname) != new_fields_dict.end()) {
                // duplicated field
                if (overwrite_field) {
                    // update column content
                    OGRColumn* col = new_fields_dict[fname];
                    UpdateOGRColumn(col, merge_layer_proxy, fname, idx2_dict);
                } else {
                    OGRColumn* col = CreateNewOGRColumn(new_rows, merge_layer_proxy, undefs, fname, idx2_dict);
                    fname = fname + "_" + Gda::CreateUUID(4);
                    col->Rename(fname);
                    new_fields_dict[fname] = col;
                    new_fields.push_back(fname);
                }
            } else {
                // new field
                OGRColumn* col = CreateNewOGRColumn(new_rows, merge_layer_proxy, undefs, fname, idx2_dict);
                new_fields_dict[fname] = col;
                new_fields.push_back(fname);
            }
        }
        
        for (int i=0; i<new_fields.size(); i++) {
            mem_table->AddOGRColumn(new_fields_dict[new_fields[i]]);
        }
        
        ExportDataDlg export_dlg(this, shape_type, new_geoms, spatial_ref, mem_table);
        if (export_dlg.ShowModal() == wxID_OK) {
            wxMessageDialog dlg(this, _("File merged into Table successfully."), _("Success"), wxOK);
            dlg.ShowModal();
        }
        
        delete mem_table;
        // see ExportDataDlg.cpp line:620
        //for (int i=0; i<new_geoms.size(); i++) {
        //    delete new_geoms[i];
        //}
        EndDialog(wxID_OK);
        
    } catch (GdaException& ex) {
        if (ex.type() == GdaException::NORMAL)
            return;
        wxMessageDialog dlg(this, ex.what(), _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
}

void MergeTableDlg::LeftJoinMerge()
{
    try {
        wxString error_msg;
        
        // get selected field names from merging table
        map<wxString, wxString> merged_fnames_dict;
        for (set<wxString>::iterator it = table_fnames.begin();
             it != table_fnames.end(); ++it ) {
             merged_fnames_dict[ *it ] = *it;
        }
        vector<wxString> merged_field_names = GetSelectedFieldNames(merged_fnames_dict);
        
        if (merged_field_names.empty())
            return;
        
        int n_rows = table_int->GetNumberRows();
        int n_merge_field = merged_field_names.size();
       
        map<int, int> rowid_map;
        if (m_key_val_rb->GetValue()==1) { // check merge by key/record order
            // get and check keys from original table
            int key1_id = m_current_key->GetSelection();
            wxString key1_name = m_current_key->GetString(key1_id);
            int col1_id = table_int->FindColId(key1_name);
            if (table_int->IsColTimeVariant(col1_id)) {
                error_msg = wxString::Format(_("Chosen key field '%s' s a time variant. Please choose a non-time variant field as key."), key1_name);
                throw GdaException(error_msg.mb_str());
            }
            
            vector<wxString> key1_vec;
            vector<wxInt64>  key1_l_vec;
            map<wxString,int> key1_map;
            
            if ( table_int->GetColType(col1_id, 0) == GdaConst::string_type ) {
                table_int->GetColData(col1_id, 0, key1_vec);
            }else if (table_int->GetColType(col1_id,0)==GdaConst::long64_type){
                table_int->GetColData(col1_id, 0, key1_l_vec);
            }
            
            if (key1_vec.empty()) {
                for( int i=0; i< key1_l_vec.size(); i++){
                    wxString tmp;
                    tmp << key1_l_vec[i];
                    key1_vec.push_back(tmp);
                }
            }
            if (CheckKeys(key1_name, key1_vec, key1_map) == false)
                return;
            
            // get and check keys from import table
            int key2_id = m_import_key->GetSelection();
            wxString key2_name = m_import_key->GetString(key2_id);
            int col2_id = merge_layer_proxy->GetFieldPos(key2_name);
            int n_merge_rows = merge_layer_proxy->GetNumRecords();
            vector<wxString> key2_vec;
            map<wxString,int> key2_map;
            for (int i=0; i < n_merge_rows; i++) {
                key2_vec.push_back(merge_layer_proxy->GetValueAt(i, col2_id));
            }
            if (CheckKeys(key2_name, key2_vec, key2_map) == false)
                return;
            
            // make sure key1 <= key2, and store their mappings
            int n_matches = 0;
            map<wxString,int>::iterator key1_it, key2_it;
            for (key1_it=key1_map.begin(); key1_it!=key1_map.end(); key1_it++) {
                key2_it = key2_map.find(key1_it->first);
                
                if ( key2_it != key2_map.end()){
                    rowid_map[key1_it->second] = key2_it->second;
                    n_matches += 1;
                }
            }
            
            if ( n_matches == 0 ){
                error_msg = _("The set of values in the import key fields has no match in current table. Please choose keys with matching sets of values.");
                throw GdaException(error_msg.mb_str());
            }
        }
        // merge by order sequence
        else if (m_rec_order_rb->GetValue() == 1) {
            if (table_int->GetNumberRows() > merge_layer_proxy->GetNumRecords()) {
                error_msg = wxString::Format(_("The number of records in current table is larger than the number of records in import table. Please choose import table >= %d records"), table_int->GetNumberRows());
                throw GdaException(error_msg.mb_str());
            }
        }
        
        // append new fields to original table via TableInterface
        for (int i=0; i<n_merge_field; i++) {
            wxString real_field_name = merged_field_names[i];
            wxString field_name  = real_field_name;
            if (merged_fnames_dict.find(real_field_name) !=
                merged_fnames_dict.end())
            {
                field_name = merged_fnames_dict[real_field_name];
            }
            AppendNewField(field_name, real_field_name, n_rows, rowid_map);
        }
	}
    catch (GdaException& ex) {
        if (ex.type() == GdaException::NORMAL) return;
        wxMessageDialog dlg(this, ex.what(), _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	wxMessageDialog dlg(this, _("File merged into Table successfully."),
						_("Success"), wxOK );
	dlg.ShowModal();
    EndDialog(wxID_OK);
}

void MergeTableDlg::AppendNewField(wxString field_name,
                                   wxString real_field_name,
                                   int n_rows,
                                   map<int,int>& rowid_map)
{
    int fid = merge_layer_proxy->GetFieldPos(real_field_name);
    GdaConst::FieldType ftype = merge_layer_proxy->GetFieldType(fid);
    
    if ( ftype == GdaConst::string_type ) {
        int add_pos = table_int->InsertCol(ftype, field_name);
        vector<wxString> data(n_rows);
        vector<bool> undefs(n_rows, false);
        for (int i=0; i<n_rows; i++) {
            int import_rid = i; // default merge by row
            
            if (!rowid_map.empty()) {
                // merge by key
                import_rid = rowid_map.find(i) == rowid_map.end() ? -1 : rowid_map[i];
            }
            
            if (import_rid >=0) {
                if (merge_layer_proxy->IsUndefined(import_rid,fid)) {
                    data[i] = wxEmptyString;
                    undefs[i] = true;
                } else {
                    data[i] = wxString(merge_layer_proxy->GetValueAt(import_rid,fid));
                    undefs[i] = false;
                }
            } else {
                data[i] = wxEmptyString;
                undefs[i] = true;
            }
        }
        table_int->SetColData(add_pos, 0, data, undefs);
        
    } else if ( ftype == GdaConst::long64_type ) {
        int add_pos = table_int->InsertCol(ftype, field_name);
        vector<wxInt64> data(n_rows);
        vector<bool> undefs(n_rows);
        for (int i=0; i<n_rows; i++) {
            int import_rid = i;
            if (!rowid_map.empty()) {
                //import_rid = rowid_map[i];
                import_rid = rowid_map.find(i) == rowid_map.end() ? -1 : rowid_map[i];
            }
            if (import_rid >=0 ) {
                if (merge_layer_proxy->IsUndefined(import_rid,fid)) {
                    data[i] = 0;
                    undefs[i] = true;
                } else {
                    OGRFeature* feat = merge_layer_proxy->GetFeatureAt(import_rid);
                    data[i] = feat->GetFieldAsInteger64(fid);
                    undefs[i] = false;
                }
            } else {
                data[i] = 0;
                undefs[i] = true;
            }
        }
        table_int->SetColData(add_pos, 0, data, undefs);
        
    } else if ( ftype == GdaConst::double_type ) {
        int add_pos=table_int->InsertCol(ftype, field_name);
        vector<double> data(n_rows);
        vector<bool> undefs(n_rows);
        for (int i=0; i<n_rows; i++) {
            int import_rid = i;
            if (!rowid_map.empty()) {
                //import_rid = rowid_map[i];
                import_rid = rowid_map.find(i) == rowid_map.end() ? -1 : rowid_map[i];
            }
            if (import_rid >=0 ) {
                if (merge_layer_proxy->IsUndefined(import_rid,fid)) {
                    data[i] = 0.0;
                    undefs[i] = true;
                } else {
                    OGRFeature* feat = merge_layer_proxy->GetFeatureAt(import_rid);
                    data[i] = feat->GetFieldAsDouble(fid);
                    undefs[i] = false;
                }
            } else {
                data[i] = 0.0;
                undefs[i] = true;
            }
        }
        table_int->SetColData(add_pos, 0, data, undefs);
    }
}

void MergeTableDlg::OnCloseClick( wxCommandEvent& ev )
{
    wxLogMessage("In MergeTableDlg::OnCloseClick()");
	EndDialog(wxID_CLOSE);
}

void MergeTableDlg::OnClose( wxCloseEvent& ev)
{
    wxLogMessage("In MergeTableDlg::OnClose()");
    Destroy();
}

void MergeTableDlg::OnKeyChoice( wxCommandEvent& ev )
{
    wxLogMessage("In MergeTableDlg::OnKeyChoice()");
	UpdateMergeButton();
}

void MergeTableDlg::UpdateMergeButton()
{
	bool enable = (!m_include_list->IsEmpty() &&
				   (m_rec_order_rb->GetValue()==1 ||
					(m_key_val_rb->GetValue()==1 &&
					 m_current_key->GetSelection() != wxNOT_FOUND &&
					 m_import_key->GetSelection() != wxNOT_FOUND)));
	FindWindow(XRCID("wxID_MERGE"))->Enable(enable);
}
