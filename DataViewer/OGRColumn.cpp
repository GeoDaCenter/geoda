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

#include <istream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <set>
#include <boost/foreach.hpp>
#include <locale>
#include <wx/regex.h>
#include <wx/numformatter.h>

#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GdaException.h"
#include "OGRColumn.h"
#include "VarOrderMapper.h"

using namespace std;

OGRColumn::OGRColumn(wxString name, int field_length, int decimals, int n_rows)
: name(name), length(field_length), decimals(decimals), is_new(true), is_deleted(false), rows(n_rows)
{
}

OGRColumn::OGRColumn(OGRLayerProxy* _ogr_layer,
                     wxString name, int field_length,int decimals)
: name(name), ogr_layer(_ogr_layer), length(field_length), decimals(decimals),
is_new(true), is_deleted(false)
{
    rows = ogr_layer->GetNumRecords();
}

OGRColumn::OGRColumn(OGRLayerProxy* _ogr_layer, int idx)
{
    // note: idx is only valid when create a OGRColumn. It's value could be
    // updated when delete columns in OGRLayer. Therefore, return current
    // column index will always dynamically fetch from GetColIndex() by using
    // the column name.
    is_new = false;
    is_deleted = false;
    ogr_layer = _ogr_layer;
    rows = ogr_layer->GetNumRecords();
    name = ogr_layer->GetFieldName(idx);
    length = ogr_layer->GetFieldLength(idx);
    decimals = ogr_layer->GetFieldDecimals(idx);
}

int OGRColumn::GetColIndex()
{
    if (is_new) return -1;
    return ogr_layer->GetFieldPos(name);
}

int OGRColumn::GetLength()
{
    if (is_new) return length;
    length = ogr_layer->GetFieldLength(GetColIndex());
    return length;
}

void OGRColumn::SetLength(int new_length)
{
    if (!is_new)
        ogr_layer->SetFieldLength(GetColIndex(), new_length);
    length = new_length;
}

int OGRColumn::GetDecimals()
{
    if (is_new) return decimals;
    decimals = ogr_layer->GetFieldDecimals(GetColIndex());
    return decimals;
}

void OGRColumn::SetDecimals(int new_decimals)
{
    if (!is_new)
        ogr_layer->SetFieldDecimals(GetColIndex(), new_decimals);
    decimals = new_decimals;
}

wxString OGRColumn::GetName()
{
    if (!is_new) name = ogr_layer->GetFieldName(GetColIndex());
    return name;
}

void OGRColumn::Rename(const wxString& new_name)
{
    if (!is_new)
        ogr_layer->SetFieldName(GetColIndex(), new_name);
    name = new_name;
}

void OGRColumn::UpdateOGRLayer(OGRLayerProxy* new_ogr_layer)
{
    ogr_layer = new_ogr_layer;
}

bool OGRColumn::IsCellUpdated(int row)
{
    if (!undef_markers.empty()) {
        return undef_markers[row];
    }
	return false;
}

bool OGRColumn::IsUndefined(int row)
{
    return undef_markers[row];
}

void OGRColumn::UpdateData(const vector<double> &data)
{
    wxString msg = "Internal error: UpdateData(double) not implemented.";
    throw GdaException(msg.mb_str());
}

void OGRColumn::UpdateData(const vector<wxInt64> &data)
{
    wxString msg = "Internal error: UpdateData(wxInt64) not implemented.";
    throw GdaException(msg.mb_str());
    
}

void OGRColumn::UpdateData(const vector<wxString> &data)
{
    wxString msg = "Internal error: UpdateData(wxString) not implemented.";
    throw GdaException(msg.mb_str());
    
}

void OGRColumn::UpdateData(const vector<double> &data,
                           const vector<bool>& undef_markers_)
{
    UpdateData(data);
    undef_markers = undef_markers_;
}

void OGRColumn::UpdateData(const vector<wxInt64> &data,
                           const vector<bool>& undef_markers_)
{
    UpdateData(data);
    undef_markers = undef_markers_;
}

void OGRColumn::UpdateData(const vector<wxString> &data,
                           const vector<bool>& undef_markers_)
{
    UpdateData(data);
    undef_markers = undef_markers_;
}

void OGRColumn::FillData(vector<double>& data)
{
    wxString msg = "Internal error: FillData(double) not implemented.";
    throw GdaException(msg.mb_str());
    
}

void OGRColumn::FillData(vector<wxInt64>& data)
{
    wxString msg = "Internal error: FillData(wxInt64) not implemented.";
    throw GdaException(msg.mb_str());
    
}

void OGRColumn::FillData(vector<wxString>& data)
{
    wxString msg = "Internal error: FillData(wxString) not implemented.";
    throw GdaException(msg.mb_str());
    
}


void OGRColumn::FillData(vector<double> &data,
                         vector<bool>& undef_markers_)
{
    FillData(data);
    undef_markers_ = undef_markers;
}

void OGRColumn::FillData(vector<wxInt64> &data,
                         vector<bool>& undef_markers_)
{
    FillData(data);
    undef_markers_ = undef_markers;
}

void OGRColumn::FillData(vector<wxString> &data,
                         vector<bool>& undef_markers_)
{
    FillData(data);
    undef_markers_ = undef_markers;
}


bool OGRColumn::GetCellValue(int row, wxInt64& val)
{
    wxString msg = "Internal error: GetCellValue(wxInt64) not implemented.";
    throw GdaException(msg.mb_str());

}

bool OGRColumn::GetCellValue(int row, double& val)
{
    wxString msg = "Internal error: GetCellValue(double) not implemented.";
    throw GdaException(msg.mb_str());

}

bool OGRColumn::GetCellValue(int row, wxString& val)
{
    wxString msg = "Internal error: GetCellValue(wxString) not implemented.";
    throw GdaException(msg.mb_str());

}

void OGRColumn::UpdateNullMarkers(const vector<bool>& undef_markers_)
{
    if (!undef_markers_.empty())
        undef_markers = undef_markers_;
}

////////////////////////////////////////////////////////////////////////////////
//
OGRColumnInteger::OGRColumnInteger(wxString name, int field_length, int decimals, int n_rows)
: OGRColumn(name, field_length, decimals, n_rows)
{
    // a new in-memory integer column
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0;
        undef_markers[i] = false;
    }
}

OGRColumnInteger::OGRColumnInteger(OGRLayerProxy* ogr_layer, wxString name,
                                   int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new integer column
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0;
        undef_markers[i] = false;
    }
}

OGRColumnInteger::OGRColumnInteger(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a integer column from OGRLayer
    is_new = false;
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        // for non-undefined value
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            undef_markers[i] = false;
        else
            undef_markers[i] = true;
    }
}

OGRColumnInteger::~OGRColumnInteger()
{
    if (new_data.size() > 0 ) new_data.clear();
    if (undef_markers.size() > 0) undef_markers.clear();
}

// Return this column to a vector of wxInt64
void OGRColumnInteger::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = new_data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = (wxInt64)ogr_layer->data[i]->GetFieldAsInteger64(col_idx);
        }
    }
}

// Return this column to a vector of double
void OGRColumnInteger::FillData(vector<double> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = (double)new_data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = (double)ogr_layer->data[i]->GetFieldAsInteger64(col_idx);
        }
    }
}

// Return this column to a vector of wxString
void OGRColumnInteger::FillData(vector<wxString> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = wxString::Format(wxT("%")  wxT(wxLongLongFmtSpec)  wxT("d"), new_data[i]);
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = wxString::Format(wxT("%") wxT(wxLongLongFmtSpec) wxT("d"),
                ogr_layer->data[i]->GetFieldAsInteger64(col_idx));
        }
    }
}

// Update this column from a vector of wxInt64
void OGRColumnInteger::UpdateData(const vector<wxInt64>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (GIntBig)data[i]);
        }
    }
}

void OGRColumnInteger::UpdateData(const vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = (int)data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (GIntBig)data[i]);
        }
    }
}


// Return an integer value from a cell at position (row)
bool OGRColumnInteger::GetCellValue(int row, wxInt64& val)
{
    if (undef_markers[row] == true) {
        val = 0;
        return false;
    }
    
    if (is_new) {
        val = new_data[row];
        
    } else {
        int col_idx = GetColIndex();
        val = (wxInt64)ogr_layer->data[row]->GetFieldAsInteger64(col_idx);
    }
    
    return true;
}

// Return a String value from a cell at position (row)
// Note: used by wxGrid Table
wxString OGRColumnInteger::GetValueAt(int row_idx, int disp_decimals,
                                      wxCSConv* m_wx_encoding)
{
    // if is undefined, return empty string
    if ( undef_markers[row_idx] == true)
        return wxEmptyString;
    
    if (is_new) {
        return wxString::Format("%lld",new_data[row_idx]);
        
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1)
            return wxEmptyString;
        wxLongLong val(ogr_layer->data[row_idx]->GetFieldAsInteger64(col_idx));
        
        return val.ToString();
    }
}

// Set a cell value from user input wxString (in Table/wxGrid)
void OGRColumnInteger::SetValueAt(int row_idx, const wxString &value)
{
    // if is already undefined, and user inputs nothing
    if ( undef_markers[row_idx] == true && value.IsEmpty() ) {
        return;
    }
   
    int col_idx = GetColIndex();
    
    if ( value.IsEmpty() ) {
        undef_markers[row_idx] = true;
        if (!is_new) {
            if (col_idx >=0) {
                ogr_layer->data[row_idx]->UnsetField(col_idx);
            }
        }
        return;
    }
    
    wxInt64 l_val;
    if ( GenUtils::validInt(value) ) {
        GenUtils::strToInt64(value, &l_val);
        if (is_new) {
            new_data[row_idx] = l_val;
        } else {
            if (col_idx == -1)
                return;
            ogr_layer->data[row_idx]->SetField(col_idx, (GIntBig)l_val);
        }
        undef_markers[row_idx] = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
OGRColumnDouble::OGRColumnDouble(wxString name, int field_length,
                                 int decimals, int n_rows)
: OGRColumn(name, field_length, decimals, n_rows)
{
    // a new in-memory integer column
    if ( decimals < 0)
        decimals = GdaConst::default_dbf_double_decimals;
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0.0;
        undef_markers[i] = false;
    }
}
OGRColumnDouble::OGRColumnDouble(OGRLayerProxy* ogr_layer, wxString name,
                                 int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new double column
    if ( decimals < 0)
        decimals = GdaConst::default_dbf_double_decimals;
    
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0.0;
        undef_markers[i] = false;
    }
}

OGRColumnDouble::OGRColumnDouble(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a double column from OGRLayer
    if ( decimals < 0)
        decimals = GdaConst::default_dbf_double_decimals;
    is_new = false;
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        // for non-undefined value
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            undef_markers[i] = false;
        else
            undef_markers[i] = true;
    }
}

OGRColumnDouble::~OGRColumnDouble()
{
    if (new_data.size() > 0 )
        new_data.clear();
    if (undef_markers.size() > 0)
        undef_markers.clear();
}


// Assign this column to a vector of wxInt64
void OGRColumnDouble::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = (wxInt64)new_data[i];
        }
        
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = (wxInt64)ogr_layer->data[i]->GetFieldAsDouble(col_idx);
        }
        
    }
}

// Assign this column to a vector of double
void OGRColumnDouble::FillData(vector<double> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = new_data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = ogr_layer->data[i]->GetFieldAsDouble(col_idx);
        }
    }
}

void OGRColumnDouble::FillData(vector<wxString> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = wxString::Format("%f", new_data[i]);
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = wxString::Format("%f",
                ogr_layer->data[i]->GetFieldAsDouble(col_idx));
        }
    }
}

// Update this column from a vector of double
void OGRColumnDouble::UpdateData(const vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, data[i]);
        }
    }
}

void OGRColumnDouble::UpdateData(const vector<wxInt64>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = (double)data[i];
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (double)data[i]);
            undef_markers[i] = false;
        }
    }
}


// Fill a double value from a cell at position (row)
bool OGRColumnDouble::GetCellValue(int row, double& val)
{
    if (undef_markers[row] == true) {
        val = 0.0;
        return false;
    }
    
    if (is_new) {
        val = new_data[row];
    } else {
        int col_idx = GetColIndex();
        val = ogr_layer->data[row]->GetFieldAsDouble(col_idx);
    }
    return true;
}

// Return a String value from a cell at position (row)
// Note: used by wxGrid Table
wxString OGRColumnDouble::GetValueAt(int row_idx, int disp_decimals,
                                     wxCSConv* m_wx_encoding)
{
    if (undef_markers[row_idx] == true)
        return wxEmptyString;
    
    if ( disp_decimals < 0)
        disp_decimals = GdaConst::default_dbf_double_decimals;
    
    double val;
    if (is_new) {
        
        val = new_data[row_idx];
        wxString rst = wxNumberFormatter::ToString(val, disp_decimals,
                                                   wxNumberFormatter::Style_None);
        return rst;
        
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1)
            return wxEmptyString;
        
        const char* tmp = ogr_layer->data[row_idx]->GetFieldAsString(col_idx);
        if (*tmp == '\0' ) {
            return wxEmptyString;
        }
        
        val = ogr_layer->data[row_idx]->GetFieldAsDouble(col_idx);
        wxString rst = wxNumberFormatter::ToString(val, disp_decimals,
                                                   wxNumberFormatter::Style_None);
        return rst;
    }
}

// Set a cell value from user input wxString (in Table/wxGrid)
void OGRColumnDouble::SetValueAt(int row_idx, const wxString &value)
{
    // if user inputs nothing for a double valued cell, GeoDa treats it as NULL
    if ( value.IsEmpty() ) {
        undef_markers[row_idx] = true;
        if (is_new ) {
            new_data[row_idx] = 0.0;
        } else {
            // set undefined/null
            ogr_layer->data[row_idx]->UnsetField(row_idx);
        }
        return;
    }
    
    double d_val;
    if ( value.ToDouble(&d_val) ) {
        if (is_new) {
            new_data[row_idx] = d_val;
        } else {
            int col_idx = GetColIndex();
            ogr_layer->data[row_idx]->SetField(col_idx, d_val);
        }
        undef_markers[row_idx] = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
OGRColumnString::OGRColumnString(wxString name, int field_length,
                                 int decimals, int n_rows)
: OGRColumn(name, field_length, decimals, n_rows)
{
    // a new in-memory string column
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = wxEmptyString;
        undef_markers[i] = false;
    }
}
OGRColumnString::OGRColumnString(OGRLayerProxy* ogr_layer, wxString name,
                                 int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new string column
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = wxEmptyString;
        undef_markers[i] = false;
    }
}

OGRColumnString::OGRColumnString(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a string column from OGRLayer
    is_new = false;
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            undef_markers[i] = false;
        else
            undef_markers[i] = true;
    }
}

OGRColumnString::~OGRColumnString()
{
    if (new_data.size() > 0 )
        new_data.clear();
    if (undef_markers.size() > 0)
        undef_markers.clear();
}

// This column -> vector<double>
void OGRColumnString::FillData(vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            double val = 0.0;
            if ( !new_data[i].ToDouble(&val) ) {
                // internal is always local "C"
                //wxString error_msg = wxString::Format( "Fill data error: can't convert '%s' to floating-point number.", new_data[i]);
                //throw GdaException(error_msg.c_str());
                undef_markers[i] = true;
            }
            data[i] = val;
        }
        
    } else {
        int col_idx = GetColIndex();
        wxString tmp;
        
        // default C locale
        for (int i=0; i<rows; ++i) {
            if ( undef_markers[i] == true) {
                data[i] = 0.0;
                continue;
            }
           
            tmp = wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            
            double val;
            if (tmp.IsEmpty()) {
                data[i] = 0.0;
                undef_markers[i] = true;
            } else if (tmp.ToDouble(&val)) {
                data[i] = val;
            } else {
                // try comma as decimal point
                setlocale(LC_NUMERIC, "de_DE");
                double _val;
                if (tmp.ToDouble(&_val)) {
                    data[i] = _val;
                } else {
                    data[i] = 0.0;
                    undef_markers[i] = true;
                }
                setlocale(LC_NUMERIC, "C");
            }
        }
    }
}

// This column -> vector<wxInt64>
void OGRColumnString::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            wxInt64 val = 0;
            if (!new_data[i].ToLongLong(&val)) {
                //wxString error_msg = wxString::Format("Fill data error: can't convert '%s' to integer number.", new_data[i]);
                //throw GdaException(error_msg.mb_str());
                double d_val;
                if (new_data[i].ToDouble(&d_val)) {
                    val = static_cast<wxInt64>(d_val);
                    data[i] = val;
                } else {
                    undef_markers[i] = true;
                    data[i] = 0;
                }
            } else {
                data[i] = val;
            }
        }
    } else {
        int col_idx = GetColIndex();
        bool conv_success = true;
        wxString tmp;
        
        // default C locale
        for (int i=0; i<rows; ++i) {
            if ( undef_markers[i] == true) {
                data[i] = 0;
                continue;
            }
            
            tmp = wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            wxInt64 val;
            double val_d;
            
            if (tmp.IsEmpty()) {
                undef_markers[i] = true;
                data[i] = 0;
                
            } else if (tmp.ToLongLong(&val)) {
                data[i] = val;
                
            } else if (tmp.ToDouble(&val_d)) {
                val = static_cast<wxInt64>(val_d);
                data[i] = val;
                
            } else {
                setlocale(LC_NUMERIC, "de_DE");
                wxInt64 val_;
                double val_d_;
                if (tmp.ToLongLong(&val_)) {
                    data[i] = val_;
                    
                } else if (tmp.ToDouble(&val_d_)) {
                    val_ = static_cast<wxInt64>(val_d_);
                    data[i] = val_;
                    
                } else {
                    data[i] = 0;
                    undef_markers[i] = true;
                }
                setlocale(LC_NUMERIC, "C");
            }
        }
    }
}

// This column -> vector<wxString>
void OGRColumnString::FillData(vector<wxString> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = new_data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
        }
    }
}

// vector<wxString> -> this column
void OGRColumnString::UpdateData(const vector<wxString>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, data[i].c_str());
        }
    }
}

void OGRColumnString::UpdateData(const vector<wxInt64>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            new_data[i] = tmp.c_str();
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            ogr_layer->data[i]->SetField(col_idx, tmp.c_str());
        }
    }
}

void OGRColumnString::UpdateData(const vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            new_data[i] = tmp.c_str();
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            ogr_layer->data[i]->SetField(col_idx, tmp.c_str());
        }
    }
}

// Fill a wxString value from a cell at position (row)
bool OGRColumnString::GetCellValue(int row, wxString& val)
{
    if (undef_markers[row] == true) {
        val = wxEmptyString;
        return false;
    }
    
    if (is_new) {
        val = new_data[row];
        
    } else {
        int col_idx = GetColIndex();
        const char* val = ogr_layer->data[row]->GetFieldAsString(col_idx);
        val = wxString(val);
    }
    return true;
}

wxString OGRColumnString::GetValueAt(int row_idx, int disp_decimals,
                                     wxCSConv* m_wx_encoding)
{
    if (undef_markers[row_idx] == true)
        return wxEmptyString;
    
    if (is_new) {
        return new_data[row_idx];
        
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1)
            return wxEmptyString;
        
        const char* val = ogr_layer->data[row_idx]->GetFieldAsString(col_idx);
        
        wxString rtn;
        if (m_wx_encoding == NULL)
            rtn = wxString(val);
        else
            rtn = wxString(val,*m_wx_encoding);
        
        return rtn;
    }
}

void OGRColumnString::SetValueAt(int row_idx, const wxString &value)
{
    // if user inputs nothing for a undefined cell
    if ( undef_markers[row_idx] == true && value.IsEmpty() ) {
        return;
    }
    
    
    if (is_new) {
        new_data[row_idx] = value;
    } else {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, value.c_str());
    }
    undef_markers[row_idx] = false;
}

////////////////////////////////////////////////////////////////////////////////
// XXX current GeoDa don't support adding new date column
//
OGRColumnDate::OGRColumnDate(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    is_new = false;
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            undef_markers[i] = false;
        else
            undef_markers[i] = true;
    }
}

OGRColumnDate::~OGRColumnDate()
{
    if (new_data.size() > 0 ) new_data.clear();
}

void OGRColumnDate::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        wxString msg = "Internal error: GeoDa doesn't support new date column.";
        throw GdaException(msg.mb_str());
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            
            if (undef_markers[i]) {
                data[i] = 0.0;
                continue;
            }
            
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int tzflag = 0;
            
            int col_idx = GetColIndex();
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month, &day,&hour, &minute, &second, &tzflag);
            
            wxInt64 ldatetime = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;

            data[i] = ldatetime;
        }
    }
}

void OGRColumnDate::FillData(vector<wxString> &data)
{
    if (is_new) {
        wxString msg = "Internal error: GeoDa doesn't support new date column.";
        throw GdaException(msg.mb_str());
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            if (undef_markers[i]) {
                data[i] = "";
                continue;
            }
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int tzflag = 0;
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month,
                                               &day,&hour,&minute,
                                               &second, &tzflag);
            data[i] = ogr_layer->data[i]->GetFieldAsString(col_idx);
        }
    }
}

bool OGRColumnDate::GetCellValue(int row, wxInt64& val)
{
    if (undef_markers[row] == true) {
        val = 0;
        return false;
    }
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int tzflag = 0;
        ogr_layer->data[row]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &second, &tzflag);
        val = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;
    } else {
        val = new_data[row];
    }
    
    return true;
}

wxString OGRColumnDate::GetValueAt(int row_idx, int disp_decimals,
                                   wxCSConv* m_wx_encoding)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int tzflag = 0;
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->GetFieldAsDateTime(col_idx, &year, &month,
                                                 &day,&hour,&minute,
                                                 &second, &tzflag);
    } else {
        //val = new_data[row_idx];
    }
    
    wxString sDateTime;
    
    if (year >0 && month > 0 && day > 0) {
        sDateTime << wxString::Format("%i-%i-%i", year, month, day);
    }
    
    if (hour >0 || minute > 0 || second > 0) {
        if (!sDateTime.IsEmpty()) sDateTime << " ";
        sDateTime << wxString::Format("%i:%i:%i", hour, minute, second);
    }
    
    return sDateTime;
}

void OGRColumnDate::SetValueAt(int row_idx, const wxString &value)
{
    wxRegEx regex;
    
    wxString date_regex_str = "([0-9]{4})-([0-9]{1,2})-([0-9]{1,2})";
    
    wxString _year, _month, _day, _hour, _minute, _second;
    
    regex.Compile(date_regex_str);
    if (regex.Matches(value)) {
        //wxString _all = regex.GetMatch(value,0);
        _year = regex.GetMatch(value,1);
        _month = regex.GetMatch(value,2);
        _day = regex.GetMatch(value,3);
    }
  
    long _l_year =0,  _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
    
    _year.ToLong(&_l_year);
    _month.ToLong(&_l_month);
    _day.ToLong(&_l_day);
    
    wxInt64 val = _l_year * 10000000000 + _l_month * 100000000 + _l_day * 1000000 + _l_hour * 10000 + _l_minute * 100 + _l_second;
    
    if (is_new) {
        new_data[row_idx] = val;
    } else {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, _l_year, _l_month, _l_day, _l_hour, _l_minute, _l_second, 0); // last TZFlag
    }
}

////////////////////////////////////////////////////////////////////////////////
// OGRColumnTime


OGRColumnTime::OGRColumnTime(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    is_new = false;
   
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            undef_markers[i] = false;
        else
            undef_markers[i] = true;
    }
}

OGRColumnTime::~OGRColumnTime()
{
    if (new_data.size() > 0 ) new_data.clear();
}

void OGRColumnTime::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        wxString msg = "Internal error: GeoDa doesn't support new date column.";
        throw GdaException(msg.mb_str());
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int tzflag = 0;
            
            int col_idx = GetColIndex();
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month, &day,&hour, &minute, &second, &tzflag);
            
            wxInt64 ldatetime = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;
            
            data[i] = ldatetime;
        }
    }
}

void OGRColumnTime::FillData(vector<wxString> &data)
{
    if (is_new) {
        wxString msg = "Internal error: GeoDa doesn't support new Time column.";
        throw GdaException(msg.mb_str());
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int tzflag = 0;
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month,
                                                   &day,&hour,&minute,
                                                   &second, &tzflag);
            data[i] = ogr_layer->data[i]->GetFieldAsString(col_idx);
        }
    }
}

bool OGRColumnTime::GetCellValue(int row, wxInt64& val)
{
    if (undef_markers[row] == true) {
        val = 0;
        return false;
    }
    /*
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int tzflag = 0;
        ogr_layer->data[row]->GetFieldAsDateTime(col_idx, &year, &month,
                                                 &day,&hour,&minute,
                                                 &second, &tzflag);
        //val = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;
    } else {
        //val = new_data[row];
    }
     */
    return true;
}

wxString OGRColumnTime::GetValueAt(int row_idx, int disp_decimals,
                                   wxCSConv* m_wx_encoding)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int tzflag = 0;
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &second, &tzflag);
    } else {
        //val = new_data[row_idx];
    }
    
    wxString sDateTime;
    
    if (hour >0 || minute > 0 || second > 0) {
        sDateTime << wxString::Format("%i:%i:%i", hour, minute, second);
    }
    
    return sDateTime;
}

void OGRColumnTime::SetValueAt(int row_idx, const wxString &value)
{
    wxRegEx regex;
    
    wxString time_regex_str = "([0-9]{2}):([0-9]{2}):([0-9]{2})";
    
    wxString _hour, _minute, _second;
    
    regex.Compile(time_regex_str);
    if (regex.Matches(value)) {
        _hour = regex.GetMatch(value,1);
        _minute = regex.GetMatch(value,2);
        _second = regex.GetMatch(value,3);
        
    }
    
    long _l_year =0,  _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
    
    _hour.ToLong(&_l_hour);
    _minute.ToLong(&_l_minute);
    _second.ToLong(&_l_second);
    
    wxInt64 val = _l_year * 10000000000 + _l_month * 100000000 + _l_day * 1000000 + _l_hour * 10000 + _l_minute * 100 + _l_second;
    
    if (is_new) {
        new_data[row_idx] = val;
    } else {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, _l_year, _l_month, _l_day, _l_hour, _l_minute, _l_second, 0); // last TZFlag
    }
}

////////////////////////////////////////////////////////////////////////////////
//OGRColumnDateTime

OGRColumnDateTime::OGRColumnDateTime(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    is_new = false;
    
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            undef_markers[i] = false;
        else
            undef_markers[i] = true;
    }
}

OGRColumnDateTime::~OGRColumnDateTime()
{
    if (new_data.size() > 0 ) new_data.clear();
}

void OGRColumnDateTime::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        wxString msg = "Internal error: GeoDa doesn't support new date column.";
        throw GdaException(msg.mb_str());
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int tzflag = 0;
            
            int col_idx = GetColIndex();
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month, &day,&hour, &minute, &second, &tzflag);
            
            wxInt64 ldatetime = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;
            
            data[i] = ldatetime;
        }
    }
}


void OGRColumnDateTime::FillData(vector<wxString>& data)
{
    if (is_new) {
        wxString msg = "Internal error: GeoDa doesn't support new date column.";
        throw GdaException(msg.mb_str());
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            int tzflag = 0;
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month,
                                                   &day,&hour,&minute,
                                                   &second, &tzflag);
            data[i] = ogr_layer->data[i]->GetFieldAsString(col_idx);
        }
    }
}

bool OGRColumnDateTime::GetCellValue(int row, wxInt64& val)
{
    if (undef_markers[row] == true) {
        val = 0;
        return false;
    }
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int tzflag = 0;
        ogr_layer->data[row]->GetFieldAsDateTime(col_idx, &year, &month,
                                                 &day,&hour,&minute,
                                                 &second, &tzflag);
        val = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;
    } else {
        val = new_data[row];
    }
    
    return true;
}

wxString OGRColumnDateTime::GetValueAt(int row_idx, int disp_decimals,
                                   wxCSConv* m_wx_encoding)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int tzflag = 0;
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &second, &tzflag);
    } else {
        //val = new_data[row_idx];
    }
    
    wxString sDateTime;
    
    if (year >0 && month > 0 && day > 0) {
        sDateTime << wxString::Format("%i-%i-%i", year, month, day);
    }
    
    if (hour >0 || minute > 0 || second > 0) {
        if (!sDateTime.IsEmpty()) sDateTime << " ";
        sDateTime << wxString::Format("%i:%i:%i", hour, minute, second);
    }
    
    return sDateTime;
}

void OGRColumnDateTime::SetValueAt(int row_idx, const wxString &value)
{
    wxRegEx regex;
    
    wxString time_regex_str = "([0-9]{2}):([0-9]{2}):([0-9]{2})";
    wxString date_regex_str = "([0-9]{4})-([0-9]{1,2})-([0-9]{1,2})";
    wxString datetime_regex_str = "([0-9]{4})-([0-9]{1,2})-([0-9]{1,2}) ([0-9]{2}):([0-9]{2}):([0-9]{2})";
    
    wxString _year, _month, _day, _hour, _minute, _second;
    
    regex.Compile(time_regex_str);
    if (regex.Matches(value)) {
        _hour = regex.GetMatch(value,1);
        _minute = regex.GetMatch(value,2);
        _second = regex.GetMatch(value,3);
        
    } else {
        
        regex.Compile(date_regex_str);
        if (regex.Matches(value)) {
            //wxString _all = regex.GetMatch(value,0);
            _year = regex.GetMatch(value,1);
            _month = regex.GetMatch(value,2);
            _day = regex.GetMatch(value,3);
            
        } else {
            
            regex.Compile(datetime_regex_str);
            if (regex.Matches(value)) {
                _year = regex.GetMatch(value,1);
                _month = regex.GetMatch(value,2);
                _day = regex.GetMatch(value,3);
                _hour = regex.GetMatch(value,4);
                _minute = regex.GetMatch(value,5);
                _second = regex.GetMatch(value,6);
            }
        }
    }
    
    long _l_year =0,  _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
    
    _year.ToLong(&_l_year);
    _month.ToLong(&_l_month);
    _day.ToLong(&_l_day);
    _hour.ToLong(&_l_hour);
    _minute.ToLong(&_l_minute);
    _second.ToLong(&_l_second);
    
    wxInt64 val = _l_year * 10000000000 + _l_month * 100000000 + _l_day * 1000000 + _l_hour * 10000 + _l_minute * 100 + _l_second;
    
    if (is_new) {
        new_data[row_idx] = val;
    } else {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, _l_year, _l_month, _l_day, _l_hour, _l_minute, _l_second, 0); // last TZFlag
    }
}
