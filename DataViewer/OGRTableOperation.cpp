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

#include "OGRColumn.h"
#include "OGRTableOperation.h"
#include "../ShapeOperations/OGRLayerProxy.h"

OGRTableOperation::OGRTableOperation(OGRColumn* col)
{
    ogr_col = col;
    ogr_type = ogr_col->GetType();
    ogr_layer = ogr_col->GetOGRLayerProxy();
}

OGRTableOperation::~OGRTableOperation()
{
}

////////////////////////////////////////////////////////////////////////////////
OGRTableOpInsertColumn::OGRTableOpInsertColumn(OGRColumn* col)
: OGRTableOperation(col)
{}

void OGRTableOpInsertColumn::Commit()
{
    // insert ogr column is only applied for appending new column
    if (ogr_col->IsNewColumn()) {
        int pos = ogr_layer->AddField(ogr_col->GetName().ToStdString(),
                                      ogr_col->GetType(),ogr_col->GetLength(),
                                      ogr_col->GetDecimals());
        // column content will be done in OGRTableUpdateColumn
    }
}
void OGRTableOpInsertColumn::Rollback()
{
    // delete the column which has been added to OGRTable
    if (ogr_col->IsNewColumn()){
        int col_idx = ogr_layer->GetFieldPos(ogr_col->GetName());
        ogr_layer->DeleteField(col_idx);
    }
}

////////////////////////////////////////////////////////////////////////////////
OGRTableOpDeleteColumn::OGRTableOpDeleteColumn(OGRColumn* col)
: OGRTableOperation(col)
{}

OGRTableOpDeleteColumn::~OGRTableOpDeleteColumn()
{
    // When OGRColumn was erased from vector<OGRColumn>, the actual memory of
    // erased OGRColumn is still there for future committing/rollingback. Once
    // the operation is done, we need to clean this piece of memory.
    if (ogr_col!= NULL) {
        delete ogr_col;
        ogr_col = NULL;
    }
}

void OGRTableOpDeleteColumn::Commit()
{
    //int col_idx = ogr_col->GetColIndex();
    int col_idx = ogr_layer->GetFieldPos(ogr_col->GetName());
    ogr_layer->DeleteField(col_idx);
}

void OGRTableOpDeleteColumn::Rollback()
{
    // add the deteled column back to OGRTable
    if (ogr_col->IsNewColumn()) {
        // just mark deletion back, and add column back to
        // vector<OGRColumn*>
        ogr_col->SetDeletion(false);
    } else {
        // append deleted column back to table
        int pos = ogr_layer->AddField(ogr_col->GetName().ToStdString(),
                                      ogr_col->GetType(),
                                      ogr_col->GetLength(),
                                      ogr_col->GetDecimals());
        //ogr_col->SetColIndex(pos);
        // restore column content
        GdaConst::FieldType type = ogr_col->GetType();
        int n_rows = ogr_col->GetNumRows();
        if ( type == GdaConst::long64_type){
            vector<wxInt64> col_data;
            vector<bool> undefs;
            ogr_col->FillData(col_data, undefs);
            for (int i=0; i<n_rows; i++) {
                ogr_layer->SetValueAt(i, pos, (GIntBig)col_data[i]);
            }
        } else if ( type == GdaConst::double_type){
            vector<double> col_data;
            vector<bool> undefs;
            ogr_col->FillData(col_data, undefs);
            for (int i=0; i<n_rows; i++) {
                ogr_layer->SetValueAt(i, pos, col_data[i]);
            }
        } else if (type == GdaConst::date_type ||
                   type == GdaConst::time_type ||
                   type == GdaConst::datetime_type ){
            
            vector<wxInt64> col_data;
            vector<bool> undefs;
            ogr_col->FillData(col_data, undefs);
            for (int i=0; i<n_rows; i++) {
                wxInt64 val = col_data[i];
                // 20081203120100 YYYYMMDDHHMMSS
                int year = val / 10000000000;
                val = val % 10000000000;
                int month = val / 100000000;
                val = val % 100000000;
                int day = val  / 1000000;
                val = val % 1000000;
                int hour = val / 10000;
                val = val % 10000;
                int minute = val / 100;
                int second = val % 100;
                ogr_layer->SetValueAt(i, pos, year, month, day, hour, minute, second);
            }
        } else {
            vector<wxString> col_data;
            vector<bool> undefs;
            ogr_col->FillData(col_data, undefs);
            for (int i=0; i<n_rows; i++) {
                ogr_layer->SetValueAt(i, pos, col_data[i].mb_str());
            }

        }
    }
}

////////////////////////////////////////////////////////////////////////////////
OGRTableOpUpdateField::OGRTableOpUpdateField(OGRColumn* col,int new_flength,
                                             int new_fdecimals)
: OGRTableOperation(col), new_field_length(new_flength),
new_field_decimals(new_fdecimals)
{
    old_field_length = col->GetLength();
    old_field_decimals = col->GetDecimals();
}

void OGRTableOpUpdateField::Commit()
{
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    ogr_layer->SetFieldLength(col_idx, new_field_length);
    ogr_layer->SetFieldDecimals(col_idx, new_field_decimals);
    ogr_layer->UpdateFieldProperties(col_idx);
}

void OGRTableOpUpdateField::Rollback()
{
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    ogr_layer->SetFieldLength(col_idx, old_field_length);
    ogr_layer->SetFieldDecimals(col_idx, old_field_decimals);
    ogr_layer->UpdateFieldProperties(col_idx);
}

////////////////////////////////////////////////////////////////////////////////
OGRTableOpRenameColumn::OGRTableOpRenameColumn( OGRColumn* col,
                                               wxString old_name,
                                               wxString new_name)
: OGRTableOperation(col)
{
    old_col_name = old_name;
    new_col_name = new_name;
}

void OGRTableOpRenameColumn::Commit()
{
    // for renaming a new column, it has to be done after inserting this
    // column and commit to OGRTable
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    ogr_layer->SetFieldName(col_idx, new_col_name);
    ogr_layer->UpdateFieldProperties(col_idx);
}
void OGRTableOpRenameColumn::Rollback()
{
    // for renaming a new column, it has to be done after inserting this
    // column and commit to OGRTable
    int col_idx = ogr_layer->GetFieldPos(new_col_name);
    ogr_layer->SetFieldName(col_idx, old_col_name);
    ogr_layer->UpdateFieldProperties(col_idx);
}

////////////////////////////////////////////////////////////////////////////////
OGRTableOpUpdateColumn::OGRTableOpUpdateColumn(OGRColumn* col,
                                        const std::vector<double>& new_data)
: OGRTableOperation(col)
{
    int n_rows = ogr_col->GetNumRows();
    d_old_data.resize(n_rows);
    undef_old_data.resize(n_rows);
    ogr_col->FillData(d_old_data, undef_old_data);
    d_new_data = new_data;
}

OGRTableOpUpdateColumn::OGRTableOpUpdateColumn(OGRColumn* col,
                                        const std::vector<wxInt64>& new_data)
: OGRTableOperation(col)
{
    int n_rows = ogr_col->GetNumRows();
    l_old_data.resize(n_rows);
    undef_old_data.resize(n_rows);
    ogr_col->FillData(l_old_data, undef_old_data);
    l_new_data = new_data;
}

OGRTableOpUpdateColumn::OGRTableOpUpdateColumn(OGRColumn* col,
                                        const std::vector<wxString>& new_data)
: OGRTableOperation(col)
{
    int n_rows = ogr_col->GetNumRows();
    s_old_data.resize(n_rows);
    undef_old_data.resize(n_rows);
    ogr_col->FillData(s_old_data, undef_old_data);
    s_new_data = new_data;
}

void OGRTableOpUpdateColumn::Commit()
{
    // get old cell value from OGRTable, save it for rolling back
    // set new cell value to OGRTabe
    int n_rows = ogr_col->GetNumRows();
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    GdaConst::FieldType type = ogr_col->GetType();
   
    if ( type == GdaConst::long64_type) {
        ogr_layer->UpdateColumn(col_idx, l_new_data);
        
    } else if (type == GdaConst::double_type) {
        ogr_layer->UpdateColumn(col_idx, d_new_data);
        
    } else if (type == GdaConst::string_type) {
        ogr_layer->UpdateColumn(col_idx, s_new_data);
    }
}

void OGRTableOpUpdateColumn::Rollback()
{
    int n_rows = ogr_col->GetNumRows();
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    GdaConst::FieldType type = ogr_col->GetType();
   
    if (ogr_layer->ds_type == GdaConst::ds_cartodb) {
        // don't support rollback direct multirow SQL update of CartoDB
        return;
    }
    if ( type == GdaConst::long64_type) {
        for (int rid=0; rid < n_rows; rid++) {
            if ( !ogr_col->IsUndefined(rid) )
                ogr_layer->SetValueAt(rid, col_idx, (GIntBig)l_old_data[rid]);
        }
        
    } else if (type == GdaConst::double_type) {
        for (int rid=0; rid < n_rows; rid++) {
            if ( !ogr_col->IsUndefined(rid) )
                ogr_layer->SetValueAt(rid, col_idx, d_old_data[rid]);
        }
        
    } else if (type == GdaConst::string_type) {
        for (int rid=0; rid < n_rows; rid++) {
            if ( !ogr_col->IsUndefined(rid) )
                ogr_layer->SetValueAt(rid, col_idx, s_old_data[rid].mb_str());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
OGRTableOpUpdateCell::OGRTableOpUpdateCell(OGRColumn* col, int row_idx,
                                           double new_val)
: OGRTableOperation(col), d_new_value(0.0), d_old_value(0.0)
{
    // this if for adding new Double column
    this->row_idx = row_idx;
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    d_old_value = ogr_layer->data[row_idx]->GetFieldAsDouble(col_idx);
    d_new_value = new_val;
}

OGRTableOpUpdateCell::OGRTableOpUpdateCell(OGRColumn* col, int row_idx,
                                           wxInt64 new_val)
: OGRTableOperation(col), d_new_value(0.0), d_old_value(0.0)
{
    // this if for adding new Integer column
    this->row_idx = row_idx;
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    l_old_value = (wxInt64)ogr_layer->data[row_idx]->GetFieldAsInteger64(col_idx);
    l_new_value = new_val;
}

OGRTableOpUpdateCell::OGRTableOpUpdateCell(OGRColumn* col, int row_idx,
                                             wxString new_val)
: OGRTableOperation(col), d_new_value(0.0), d_old_value(0.0)
{
    // this is for user editing cell value and save to table, so the
    // wxString(new_val) could be any data type. Here converts new_val
    // to related data type 
    this->row_idx = row_idx;
    wxString col_name = ogr_col->GetName();
    GdaConst::FieldType type = ogr_col->GetType();
    if ( type == GdaConst::long64_type) {
        new_val.ToLongLong(&l_new_value);
    } else if (type == GdaConst::double_type) {
        new_val.ToDouble(&d_new_value);
    } else if (type == GdaConst::string_type) {
        s_new_value = new_val;
    }
    GetOriginalCellValue();
}

void OGRTableOpUpdateCell::GetOriginalCellValue()
{
    int col_idx = ogr_layer->GetFieldPos(ogr_col->GetName());
    
    GdaConst::FieldType type = ogr_col->GetType();
    if ( type == GdaConst::long64_type) {
        if (col_idx < 0)
            l_old_value = 0;
        else
            l_old_value = ogr_layer->data[row_idx]->GetFieldAsInteger64(col_idx);
    } else if (type == GdaConst::double_type) {
        if (col_idx < 0)
            d_old_value = 0.0;
        else
            d_old_value = ogr_layer->data[row_idx]->GetFieldAsDouble(col_idx);
    } else if (type == GdaConst::string_type) {
        if (col_idx < 0)
            s_old_value = wxEmptyString;
        else
            s_old_value =
                wxString(ogr_layer->data[row_idx]->GetFieldAsString(col_idx));
    }
}

void OGRTableOpUpdateCell::Commit()
{
    // get old cell value from OGRTable, save it for rolling back
    // set new cell value to OGRTabe
    int n_rows = ogr_col->GetNumRows();
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    if (col_idx < 0) {
        wxString msg = "Internal Error: can't update an in-memory cell.";
        throw GdaException(msg.mb_str());
    }
    
    GdaConst::FieldType type = ogr_col->GetType();
    
    if ( type == GdaConst::long64_type) {
        if ( ogr_col->IsCellUpdated(row_idx) || (l_new_value != l_old_value) ) {
            ogr_layer->SetValueAt(row_idx, col_idx, (GIntBig)l_new_value);
        }
    } else if (type == GdaConst::double_type) {
        if ( ogr_col->IsCellUpdated(row_idx) || (d_new_value != d_old_value) ) {
            ogr_layer->SetValueAt(row_idx, col_idx, d_new_value);
        }
    } else if (type == GdaConst::string_type) {
        if ( ogr_col->IsCellUpdated(row_idx) || (s_new_value != s_old_value) ) {
            ogr_layer->SetValueAt(row_idx, col_idx, s_new_value);
        }
    }
}

void OGRTableOpUpdateCell::Rollback()
{
    int n_rows = ogr_col->GetNumRows();
    wxString col_name = ogr_col->GetName();
    int col_idx = ogr_layer->GetFieldPos(col_name);
    if (col_idx < 0) {
        wxString msg = "Internal Error: can't update an in-memory cell.";
        throw GdaException(msg.mb_str());
    }
    
    GdaConst::FieldType type = ogr_col->GetType();
    
    if ( type == GdaConst::long64_type) {
        if ( ogr_col->IsCellUpdated(row_idx) || (l_new_value != l_old_value) ) {
            ogr_layer->SetValueAt(row_idx, col_idx, (GIntBig)l_new_value);
        }
    } else if (type == GdaConst::double_type) {
        if ( ogr_col->IsCellUpdated(row_idx) || (d_new_value != d_old_value) ) {
            ogr_layer->SetValueAt(row_idx, col_idx, d_new_value);
        }
    } else if (type == GdaConst::string_type) {
        if ( ogr_col->IsCellUpdated(row_idx) || (s_new_value != s_old_value) ) {
            ogr_layer->SetValueAt(row_idx, col_idx, s_new_value);
        }
    }
}