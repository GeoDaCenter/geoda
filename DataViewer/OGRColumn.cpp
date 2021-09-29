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

#include <string.h>
#include <istream>
#include <time.h>
#include <sstream>
#include <algorithm>
#include <vector>
#include <set>
#include <boost/foreach.hpp>
#include <boost/date_time.hpp>
#include <locale>
#include <wx/regex.h>
#include <wx/numformatter.h>

#include "../GenUtils.h"
//#include "../GeoDa.h"
#include "../logger.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GdaException.h"
#include "OGRColumn.h"
#include "VarOrderMapper.h"

using namespace std;
namespace bt = boost::posix_time;

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

OGRColumn::OGRColumn(OGRLayerProxy* _ogr_layer, int _idx)
{
    // note: idx is only valid when create a OGRColumn. It's value could be
    // updated when delete columns in OGRLayer. Therefore, return current
    // column index will always dynamically fetch from GetColIndex() by using
    // the column name.
    idx = _idx;
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
    if (is_new)
        return -1;
    
    if (name == ogr_layer->GetFieldName(idx))
        return idx;
    else
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

void OGRColumn::UpdateData(const vector<unsigned long long> &data)
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

void OGRColumn::UpdateData(const vector<unsigned long long> &data,
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

void OGRColumn::FillData(vector<wxString>& data, wxCSConv* m_wx_encoding)
{
    wxString msg = "Internal error: FillData(wxString) not implemented.";
    throw GdaException(msg.mb_str());
}

void OGRColumn::FillData(vector<unsigned long long>& data)
{
    wxString msg = "Internal error: FillData(date) not implemented.";
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
                         vector<bool>& undef_markers_,
                         wxCSConv* m_wx_encoding)
{
    FillData(data);
    undef_markers_ = undef_markers;
}

void OGRColumn::FillData(vector<unsigned long long> &data,
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
        undef_markers[i] = true;
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
        undef_markers[i] = true;
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
void OGRColumnInteger::FillData(vector<wxString> &data, wxCSConv* m_wx_encoding)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = wxString::Format("%"  wxLongLongFmtSpec  "d", new_data[i]);
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i] = wxString::Format("%" wxLongLongFmtSpec "d",
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
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (GIntBig)data[i]);
            undef_markers[i] = false;
        }
    }
}

void OGRColumnInteger::UpdateData(const vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = (int)data[i];
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (GIntBig)data[i]);
            undef_markers[i] = false;
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
void OGRColumnInteger::SetValueAt(int row_idx, const wxString &value,
                                  wxCSConv* m_wx_encoding)
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

    if (value.ToLongLong(&l_val)) {
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

void OGRColumnInteger::SetValueAt(int row_idx, wxInt64 l_val)
{
    int col_idx = GetColIndex();
    
    if (is_new) {
        new_data[row_idx] = l_val;
    } else {
        if (col_idx == -1)
            return;
        ogr_layer->data[row_idx]->SetField(col_idx, (GIntBig)l_val);
    }
    undef_markers[row_idx] = false;
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
        undef_markers[i] = true;
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

void OGRColumnDouble::FillData(vector<wxString> &data, wxCSConv* m_wx_encoding)
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
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, data[i]);
            undef_markers[i] = false;
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
    
    if ( disp_decimals <= 0) {
        // if has decimals read from datasource, set disp_decimals to decimals
        if (decimals > 0) disp_decimals = decimals;
        else disp_decimals = GdaConst::default_dbf_double_decimals;
    }

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
void OGRColumnDouble::SetValueAt(int row_idx, const wxString &value,
                                 wxCSConv* m_wx_encoding)
{
    // if user inputs nothing for a double valued cell, GeoDa treats it as NULL
    if ( value.IsEmpty() ) {
        undef_markers[row_idx] = true;
        if (is_new ) {
            new_data[row_idx] = 0.0;
        } else {
            // set undefined/null
            int col_idx = GetColIndex();
            ogr_layer->data[row_idx]->UnsetField(col_idx);
        }
        return;
    }
    
    double d_val;
    //if ( value.ToDouble(&d_val) ) {
    if (wxNumberFormatter::FromString(value, &d_val)) {
        if (is_new) {
            new_data[row_idx] = d_val;
        } else {
            int col_idx = GetColIndex();
            ogr_layer->data[row_idx]->SetField(col_idx, d_val);
        }
        undef_markers[row_idx] = false;
    }
}

void OGRColumnDouble::SetValueAt(int row_idx, double d_val)
{
    if (is_new) {
        new_data[row_idx] = d_val;
    } else {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, d_val);
    }
    undef_markers[row_idx] = false;
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
        undef_markers[i] = true;
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
        undef_markers[i] = true;
    }
}

OGRColumnString::OGRColumnString(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a string column from OGRLayer
    is_new = false;
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        if ( ogr_layer->data[i]->IsFieldSetAndNotNull(idx))
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
    const char* thousand_sep = CPLGetConfigOption("GEODA_LOCALE_SEPARATOR", ",");
    const char* decimal_sep = CPLGetConfigOption("GEODA_LOCALE_DECIMAL", ".");
    bool use_custom_locale = false;
    if ((strlen(thousand_sep) > 0 && strcmp(thousand_sep, ",") != 0) ||
        (strlen(decimal_sep) > 0 && strcmp(decimal_sep, ".") != 0)) {
        // customized locale numeric
        use_custom_locale = true;
    }

    if (is_new) {
        for (int i=0; i<rows; ++i) {
            double val = 0.0;
            if (use_custom_locale) {
                new_data[i].Replace(thousand_sep, "");
                new_data[i].Replace(decimal_sep, ".");
            }
            wxNumberFormatter::FromString(new_data[i], &val);
            data[i] = val;
        }
        
    } else {
        int col_idx = GetColIndex();
        wxString tmp;

        for (int i=0; i<rows; ++i) {
            if ( undef_markers[i] == true) {
                data[i] = 0.0;
                continue;
            }
            tmp = wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));

            if (use_custom_locale) {
                tmp.Replace(thousand_sep, "");
                tmp.Replace(decimal_sep, ".");
            }

            double val = 0.0;
            wxNumberFormatter::FromString(tmp, &val);
            data[i] = val;
        }
    }
}

// This column -> vector<wxInt64>
void OGRColumnString::FillData(vector<wxInt64> &data)
{
    const char* thousand_sep = CPLGetConfigOption("GEODA_LOCALE_SEPARATOR", ",");
    const char* decimal_sep = CPLGetConfigOption("GEODA_LOCALE_DECIMAL", ".");
    bool use_custom_locale = false;
    if ((strlen(thousand_sep) > 0 && strcmp(thousand_sep, ",") != 0) ||
        (strlen(decimal_sep) > 0 && strcmp(decimal_sep, ".") != 0)) {
        // customized locale numeric
        use_custom_locale = true;
    }

    if (is_new) {
        for (int i=0; i<rows; ++i) {
            wxInt64 val = 0;
            if (use_custom_locale) {
                new_data[i].Replace(thousand_sep, "");
                new_data[i].Replace(decimal_sep, ".");
            }
            wxNumberFormatter::FromString(new_data[i], &val);
            data[i] = val;
        }
    } else {
        int col_idx = GetColIndex();
        bool conv_success = true;
        wxString tmp;

        for (int i=0; i<rows; ++i) {
            if ( undef_markers[i] == true) {
                data[i] = 0;
                continue;
            }
            tmp = wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            wxInt64 val = 0;

            if (use_custom_locale) {
                tmp.Replace(thousand_sep, "");
                tmp.Replace(decimal_sep, ".");
            }
            
            wxNumberFormatter::FromString(tmp, &val);
            data[i] = val;
        }
    }
}

// This column -> vector<wxString>
void OGRColumnString::FillData(vector<wxString> &data, wxCSConv* m_wx_encoding)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = new_data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            const char* val = ogr_layer->data[i]->GetFieldAsString(col_idx);
            if ( m_wx_encoding == NULL ) data[i] = wxString(val);
            else data[i] = wxString(val, *m_wx_encoding);
        }
    }
}

// for date/time
void OGRColumnString::FillData(vector<unsigned long long>& data)
{
    if (is_new) {
        wxString test_s = new_data[0];
        test_s.Trim(true).Trim(false);
        vector<wxString> date_items;
        wxString pattern = Gda::DetectDateFormat(test_s, date_items);
        if (pattern.IsEmpty()) {
            wxString error_msg = wxString::Format("Fill data error: can't convert '%s' to date/time.", test_s);
            throw GdaException(error_msg.mb_str());
        }
        wxRegEx regex(pattern);
        if (!regex.IsValid()){
            wxString error_msg = wxString::Format("Fill data error: can't convert '%s' to date/time.", test_s);
            throw GdaException(error_msg.mb_str());
        }
        for (int i=0; i<rows; ++i) {
            new_data[i].Trim(true).Trim(false);
            data[i] = Gda::DateToNumber(new_data[i], regex, date_items);
        }
    } else {
        int col_idx = GetColIndex();
        wxString test_s = ogr_layer->data[0]->GetFieldAsString(col_idx);
        test_s.Trim(true).Trim(false);
        vector<wxString> date_items;
        wxString pattern = Gda::DetectDateFormat(test_s, date_items);
        
        if (pattern.IsEmpty()) {
            wxString error_msg = wxString::Format("Fill data error: can't convert '%s' to date/time.", test_s);
            throw GdaException(error_msg.mb_str());
        }
      
        wxRegEx regex(pattern);
        if (!regex.IsValid()){
            wxString error_msg = wxString::Format("Fill data error: can't convert '%s' to date/time.", test_s);
            throw GdaException(error_msg.mb_str());
        }
        
        for (int i=0; i<rows; ++i) {
            wxString s = ogr_layer->data[i]->GetFieldAsString(col_idx);
            s.Trim(true).Trim(false);
            unsigned long long val = Gda::DateToNumber(s, regex, date_items);
            data[i] = val;
        }
    }
}

// vector<wxString> -> this column
void OGRColumnString::UpdateData(const vector<wxString>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, data[i].c_str());
            undef_markers[i] = false;
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
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            ogr_layer->data[i]->SetField(col_idx, tmp.c_str());
            undef_markers[i] = false;
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
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            ogr_layer->data[i]->SetField(col_idx, tmp.c_str());
            undef_markers[i] = false;
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
        // should be already encoded in String, so return it directly
        wxString rtn = new_data[row_idx];
        return rtn;
        
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1)
            return wxEmptyString;
        
        const char* val = ogr_layer->data[row_idx]->GetFieldAsString(col_idx);

        wxString rtn;
        if (m_wx_encoding == NULL)
            rtn = wxString(val);
        else
            rtn = wxString(val, *m_wx_encoding);
        
        return rtn;
    }
}

void OGRColumnString::SetValueAt(int row_idx, const wxString &value,
                                 wxCSConv* m_wx_encoding)
{
    // if user inputs nothing for a undefined cell
    if ( undef_markers[row_idx] == true && value.IsEmpty() ) {
        return;
    }
    
    if (is_new) {
        new_data[row_idx] = value;
    } else {
        int col_idx = GetColIndex();
        if (m_wx_encoding)
            ogr_layer->data[row_idx]->SetField(col_idx, value.mb_str(*m_wx_encoding));
        else
            ogr_layer->data[row_idx]->SetField(col_idx, value.mb_str());
    }
    undef_markers[row_idx] = value.IsEmpty();
}

////////////////////////////////////////////////////////////////////////////////
// XXX current GeoDa don't support adding new date column
//
OGRColumnDate::OGRColumnDate(OGRLayerProxy* ogr_layer, wxString name, int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new string column
    is_new = true;
    new_data.resize(rows);
    undef_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        undef_markers[i] = false;
    }
}

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
        for (int i=0; i<rows; ++i) {
            data[i] = new_data[i];
        }
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

void OGRColumnDate::FillData(vector<unsigned long long> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] =  new_data[i];
        }
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
           
            unsigned long long ldatetime = year * 10000000000 + month * 100000000 + day * 1000000 + hour * 10000 + minute * 100 + second;
            data[i] = ldatetime;
        }
    }
}

void OGRColumnDate::FillData(vector<wxString> &data, wxCSConv* m_wx_encoding)
{
    int year, month, day, hour, minute, second, tzflag;
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            year = new_data[i] / 10000000000;
            month = (new_data[i] % 10000000000) / 100000000;
            day = (new_data[i] % 100000000) / 1000000;
            if (year >0 && month > 0 && day > 0) {
                tmp << wxString::Format("%i-%i-%i", year, month, day);
            }
            
            int hms = new_data[i] % 1000000;
            if (hms > 0) {
                hour = (new_data[i] % 1000000) / 10000;
                minute = (new_data[i] % 10000) / 100;
                second = new_data[i] % 100;
                if (hour >0 || minute > 0 || second > 0) {
                    if (!tmp.IsEmpty()) tmp << " ";
                    tmp << wxString::Format("%i:%i:%i", hour, minute, second);
                }
            }
            data[i] = tmp;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            if (undef_markers[i]) {
                data[i] = "";
                continue;
            }
            year = 0;
            month = 0;
            day = 0;
            hour = 0;
            minute = 0;
            second = 0;
            tzflag = 0;
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month,
                                                   &day,&hour,&minute,
                                                   &second, &tzflag);
            wxString tmp;
            if (year >0 && month > 0 && day > 0) {
                tmp << wxString::Format("%i-%i-%i", year, month, day);
            }
            if (hour >0 || minute > 0 || second > 0) {
                if (!tmp.IsEmpty()) tmp << " ";
                tmp << wxString::Format("%i:%i:%i", hour, minute, second);
            }
            data[i] = tmp;
        }
    }
}

void OGRColumnDate::UpdateData(const vector<unsigned long long> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
            undef_markers[i] = false;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            long l_year =0,  l_month=0, l_day=0, l_hour=0, l_minute=0, l_second=0;
           
            l_year = data[i] / 10000000000;
            l_month = (data[i] % 10000000000) / 100000000;
            l_day = (data[i] % 100000000) / 1000000;
            l_hour = (data[i] % 1000000) / 10000;
            l_minute = (data[i] % 10000) / 100;
            l_second = data[i] % 100;
            
            ogr_layer->data[i]->SetField(col_idx, l_year, l_month, l_day, l_hour, l_minute, l_second, 0); // last TZFlag
            undef_markers[i] = false;
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
    int minute = -1;
    int second = -1;
    int tzflag = -1;
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->GetFieldAsDateTime(col_idx, &year, &month,
                                                 &day,&hour,&minute,
                                                 &second, &tzflag);
    } else {
        year = new_data[row_idx] / 10000000000;
        month = (new_data[row_idx] % 10000000000) / 100000000;
        day = (new_data[row_idx] % 100000000) / 1000000;
        hour = (new_data[row_idx] % 1000000) / 10000;
        minute = (new_data[row_idx] % 10000) / 100;
        second = new_data[row_idx] % 100;
    }
    
    wxString sDateTime;
    
    if (year >0 && month > 0 && day > 0) {
        sDateTime << wxString::Format("%i-%i-%i", year, month, day);
    }
    
    return sDateTime;
}

void OGRColumnDate::SetValueAt(int row_idx, const wxString &value,
                               wxCSConv* m_wx_encoding)
{
    int col_idx = GetColIndex();
    if (value.IsEmpty()) {
        undef_markers[row_idx] = true;
        ogr_layer->data[row_idx]->UnsetField(col_idx);
        return;
    }
    wxString _value = value;
    _value.Trim(true).Trim(false);
    vector<wxString> date_items;
    wxString pattern = Gda::DetectDateFormat(_value, date_items);
    wxRegEx regex;
    regex.Compile(pattern);
    unsigned long long val = Gda::DateToNumber(_value, regex, date_items);
    long _l_year =0,  _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
    if (is_new) {
        new_data[row_idx] = val;
    } else {
        _l_year = val / 10000000000;
        _l_month = (val  % 10000000000) / 100000000;
        _l_day = (val % 100000000) / 1000000;
        _l_hour = (val % 1000000) / 10000;
        _l_minute = (val % 10000) / 100;
        _l_second = val % 100;
        ogr_layer->data[row_idx]->SetField(col_idx, _l_year, _l_month, _l_day, _l_hour, _l_minute, _l_second, 0); // last TZFlag
    }
}

////////////////////////////////////////////////////////////////////////////////
// OGRColumnTime
OGRColumnTime::OGRColumnTime(OGRLayerProxy* ogr_layer, int idx)
:OGRColumnDate(ogr_layer, idx)
{
}

OGRColumnTime::OGRColumnTime(OGRLayerProxy* ogr_layer, wxString name, int field_length, int decimals)
:OGRColumnDate(ogr_layer, name, field_length, decimals)
{
}

OGRColumnTime::~OGRColumnTime()
{
}

wxString OGRColumnTime::GetValueAt(int row_idx, int disp_decimals,
                                   wxCSConv* m_wx_encoding)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = -1;
    int minute = -1;
    int second = -1;
    int tzflag = -1;
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &second, &tzflag);
    } else {
        hour = (new_data[row_idx] % 1000000) / 10000;
        minute = (new_data[row_idx] % 10000) / 100;
        second = new_data[row_idx] % 100;
    }
    
    wxString sDateTime;
    
    if (hour >=0 || minute >= 0 || second >= 0) {
        if (!sDateTime.IsEmpty()) sDateTime << " ";
        sDateTime << wxString::Format("%i:%i:%i", hour, minute, second);
    }
    
    return sDateTime;
}

void OGRColumnTime::SetValueAt(int row_idx, const wxString &value,
                               wxCSConv* m_wx_encoding)
{
    int col_idx = GetColIndex();
    if (value.IsEmpty()) {
        undef_markers[row_idx] = true;
        ogr_layer->data[row_idx]->UnsetField(col_idx);
        return;
    }
    wxString _value = value;
    _value.Trim(true).Trim(false);
    vector<wxString> date_items;
    wxString pattern = Gda::DetectDateFormat(_value, date_items);
    wxRegEx regex;
    regex.Compile(pattern);
    unsigned long long val = Gda::DateToNumber(_value, regex, date_items);
    long _l_year =0,  _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
    if (is_new) {
        new_data[row_idx] = val;
    } else {
        _l_year = val / 10000000000;
        _l_month = (val  % 10000000000) / 100000000;
        _l_day = (val % 100000000) / 1000000;
        _l_hour = (val % 1000000) / 10000;
        _l_minute = (val % 10000) / 100;
        _l_second = val % 100;
        ogr_layer->data[row_idx]->SetField(col_idx, _l_year, _l_month, _l_day, _l_hour, _l_minute, _l_second, 0); // last TZFlag
    }
}

////////////////////////////////////////////////////////////////////////////////
//OGRColumnDateTime
OGRColumnDateTime::OGRColumnDateTime(OGRLayerProxy* ogr_layer, int idx)
:OGRColumnDate(ogr_layer, idx)
{
}

OGRColumnDateTime::OGRColumnDateTime(OGRLayerProxy* ogr_layer, wxString name, int field_length, int decimals)
:OGRColumnDate(ogr_layer, name, field_length, decimals)
{
}

OGRColumnDateTime::~OGRColumnDateTime()
{
}

wxString OGRColumnDateTime::GetValueAt(int row_idx, int disp_decimals,
                                   wxCSConv* m_wx_encoding)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = -1;
    int minute = -1;
    int second = -1;
    int tzflag = -1;
    
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &second, &tzflag);
    } else {
        year = new_data[row_idx] / 10000000000;
        month = (new_data[row_idx] % 10000000000) / 100000000;
        day = (new_data[row_idx] % 100000000) / 1000000;
        hour = (new_data[row_idx] % 1000000) / 10000;
        minute = (new_data[row_idx] % 10000) / 100;
        second = new_data[row_idx] % 100;
    }
    
    wxString sDateTime;
    
    if (year >0 && month > 0 && day > 0) {
        sDateTime << wxString::Format("%i-%i-%i", year, month, day);
    }
    
    if (hour >=0 || minute >= 0 || second >= 0) {
        if (!sDateTime.IsEmpty()) sDateTime << " ";
        sDateTime << wxString::Format("%i:%i:%i", hour, minute, second);
    }
    
    return sDateTime;
}

void OGRColumnDateTime::SetValueAt(int row_idx, const wxString &value,
                                   wxCSConv* m_wx_encoding)
{
    int col_idx = GetColIndex();
    if (value.IsEmpty()) {
        undef_markers[row_idx] = true;
        ogr_layer->data[row_idx]->UnsetField(col_idx);
        return;
    }
    wxString _value = value;
    _value.Trim(true).Trim(false);
    vector<wxString> date_items;
    wxString pattern = Gda::DetectDateFormat(_value, date_items);
    wxRegEx regex;
    regex.Compile(pattern);
    unsigned long long val = Gda::DateToNumber(_value, regex, date_items);
    long _l_year =0,  _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
    if (is_new) {
        new_data[row_idx] = val;
    } else {
        _l_year = val / 10000000000;
        _l_month = (val  % 10000000000) / 100000000;
        _l_day = (val % 100000000) / 1000000;
        _l_hour = (val % 1000000) / 10000;
        _l_minute = (val % 10000) / 100;
        _l_second = val % 100;
        ogr_layer->data[row_idx]->SetField(col_idx, _l_year, _l_month, _l_day, _l_hour, _l_minute, _l_second, 0); // last TZFlag
    }
}
