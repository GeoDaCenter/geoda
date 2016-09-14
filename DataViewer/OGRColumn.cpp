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
    if (!set_markers.empty()) {
        return set_markers[row];
    }
	return false;
}

bool OGRColumn::IsUndefined(int row)
{
    return !set_markers[row];
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

void OGRColumn::GetCellValue(int row, wxInt64& val)
{
    wxString msg = "Internal error: GetCellValue(wxInt64) not implemented.";
    throw GdaException(msg.mb_str());

}

void OGRColumn::GetCellValue(int row, double& val)
{
    wxString msg = "Internal error: GetCellValue(double) not implemented.";
    throw GdaException(msg.mb_str());

}

void OGRColumn::GetCellValue(int row, wxString& val)
{
    wxString msg = "Internal error: GetCellValue(wxString) not implemented.";
    throw GdaException(msg.mb_str());

}

////////////////////////////////////////////////////////////////////////////////
//
OGRColumnInteger::OGRColumnInteger(wxString name, int field_length, int decimals, int n_rows)
: OGRColumn(name, field_length, decimals, n_rows)
{
    // a new in-memory integer column
    is_new = true;
    new_data.resize(rows);
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0;
        set_markers[i] = false;
    }
}

OGRColumnInteger::OGRColumnInteger(OGRLayerProxy* ogr_layer, wxString name,
                                   int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new integer column
    is_new = true;
    new_data.resize(rows);
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0;
        set_markers[i] = false;
    }
}

OGRColumnInteger::OGRColumnInteger(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a integer column from OGRLayer
    is_new = false;
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        // for non-undefined value
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            set_markers[i] = true;
        else
            set_markers[i] = false;
    }
}

OGRColumnInteger::~OGRColumnInteger()
{
    if (new_data.size() > 0 ) new_data.clear();
    if (set_markers.size() > 0) set_markers.clear();
}

void OGRColumnInteger::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            data[i] = new_data[i];
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            data[i]=(wxInt64)ogr_layer->data[i]->GetFieldAsInteger64(col_idx);
        }
    }
}


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

void OGRColumnInteger::UpdateData(const vector<wxInt64>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (GIntBig)data[i]);
            set_markers[i] = true;
        }
    }
}

void OGRColumnInteger::UpdateData(const vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = (int)data[i];
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (GIntBig)data[i]);
            set_markers[i] = true;
        }
    }
}

void OGRColumnInteger::GetCellValue(int row, wxInt64& val)
{
    if (is_new) {
        val = new_data[row];
    } else {
        int col_idx = GetColIndex();
        val = (wxInt64)ogr_layer->data[row]->GetFieldAsInteger64(col_idx);
    }
}

wxString OGRColumnInteger::GetValueAt(int row_idx, int disp_decimals,
                                      wxCSConv* m_wx_encoding)
{
    if (is_new) {
        if (set_markers[row_idx] == false )
            return wxEmptyString;
        return wxString::Format("%lld",new_data[row_idx]);
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1) return wxEmptyString;
        //const char* val = ogr_layer->data[row_idx]->GetFieldAsString(col_idx);
        wxLongLong val(ogr_layer->data[row_idx]->GetFieldAsInteger64(col_idx));
        
        return val.ToString();
    }
}

void OGRColumnInteger::SetValueAt(int row_idx, const wxString &value)
{
    wxInt64 l_val;
    if (GenUtils::validInt(value)) {
        GenUtils::strToInt64(value, &l_val);
        if (is_new) {
            new_data[row_idx] = l_val;
        } else {
            int col_idx = GetColIndex();
            ogr_layer->data[row_idx]->SetField(col_idx, (GIntBig)l_val);
        }
        set_markers[row_idx] = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
OGRColumnDouble::OGRColumnDouble(wxString name, int field_length, int decimals, int n_rows)
: OGRColumn(name, field_length, decimals, n_rows)
{
    // a new in-memory integer column
    if ( decimals < 0) decimals = GdaConst::default_dbf_double_decimals;
    is_new = true;
    new_data.resize(rows);
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0.0;
        set_markers[i] = false;
    }
}
OGRColumnDouble::OGRColumnDouble(OGRLayerProxy* ogr_layer, wxString name,
                                 int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new double column
    if ( decimals < 0) decimals = GdaConst::default_dbf_double_decimals;
    is_new = true;
    new_data.resize(rows);
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = 0.0;
        set_markers[i] = false;
    }
}

OGRColumnDouble::OGRColumnDouble(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a double column from OGRLayer
    if ( decimals < 0) decimals = GdaConst::default_dbf_double_decimals;
    is_new = false;
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        // for non-undefined value
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            set_markers[i] = true;
        else
            set_markers[i] = false;
    }
}

OGRColumnDouble::~OGRColumnDouble()
{
    if (new_data.size() > 0 ) new_data.clear();
    if (set_markers.size() > 0) set_markers.clear();
}


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

void OGRColumnDouble::UpdateData(const vector<double>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, data[i]);
            set_markers[i] = true;
        }
    }
}

void OGRColumnDouble::UpdateData(const vector<wxInt64>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = (double)data[i];
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, (double)data[i]);
            set_markers[i] = true;
        }
    }
}
void OGRColumnDouble::GetCellValue(int row, double& val)
{
    if (is_new) {
        val = new_data[row];
    } else {
        int col_idx = GetColIndex();
        val = ogr_layer->data[row]->GetFieldAsDouble(col_idx);
    }
}

wxString OGRColumnDouble::GetValueAt(int row_idx, int disp_decimals,
                                     wxCSConv* m_wx_encoding)
{
    disp_decimals = 0;
    double val;
    if (is_new) {
        if (set_markers[row_idx]== false)
            return wxEmptyString;
        val = new_data[row_idx];
        wxString rst = wxString::Format("%f", val);
        return rst;
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1) return wxEmptyString;
        const char* tmp = ogr_layer->data[row_idx]->GetFieldAsString(col_idx);
        if (*tmp == '\0' ) {
            return wxEmptyString;
        } else {
            // return raw values if no display decimals setup
            if ( disp_decimals <=0 ) {
                return wxString(tmp);
            }
        }
        val = ogr_layer->data[row_idx]->GetFieldAsDouble(col_idx);
        wxString rst = wxString::Format("%.*f", disp_decimals, val);
        return rst;
    }
}

void OGRColumnDouble::SetValueAt(int row_idx, const wxString &value)
{
    double d_val;
    if (value.ToDouble(&d_val)) {
        if (is_new) {
            new_data[row_idx] = d_val;
        } else {
            int col_idx = GetColIndex();
            ogr_layer->data[row_idx]->SetField(col_idx, d_val);
        }
        set_markers[row_idx] = true;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
OGRColumnString::OGRColumnString(wxString name, int field_length, int decimals, int n_rows)
: OGRColumn(name, field_length, decimals, n_rows)
{
    // a new in-memory string column
    is_new = true;
    new_data.resize(rows);
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = wxEmptyString;
        set_markers[i] = false;
    }
}
OGRColumnString::OGRColumnString(OGRLayerProxy* ogr_layer, wxString name,
                                 int field_length, int decimals)
: OGRColumn(ogr_layer, name, field_length, decimals)
{
    // a new string column
    is_new = true;
    new_data.resize(rows);
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        new_data[i] = wxEmptyString;
        set_markers[i] = false;
    }
}

OGRColumnString::OGRColumnString(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    // a string column from OGRLayer
    is_new = false;
    set_markers.resize(rows);
    for (int i=0; i<rows; ++i) {
        if ( ogr_layer->data[i]->IsFieldSet(idx) )
            set_markers[i] = true;
        else
            set_markers[i] = false;
    }
}

OGRColumnString::~OGRColumnString()
{
    if (new_data.size() > 0 ) new_data.clear();
    if (set_markers.size() > 0) set_markers.clear();
}

void OGRColumnString::FillData(vector<double> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            double val;
            // internal is always local "C"
            if (!new_data[i].ToDouble(&val)) {
                wxString error_msg;
                error_msg << "Fill data error: can't convert '" << new_data[i]
                << "' to floating-point number.";
                throw GdaException(error_msg.c_str());
            }
            data[i] = val;
        }
    } else {
        int col_idx = GetColIndex();
        bool conv_success = true;
        wxString tmp;
        
        // default C locale
        for (int i=0; i<rows; ++i) {
            tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            double val;
            if (tmp.ToDouble(&val)) {
                data[i] = val;
            } else if (tmp.IsEmpty()) {
                data[i] = 0;
            } else {
                conv_success = false;
                break;
            }
        }
        
        if (conv_success == false) {
            conv_success = true;
            // test if C thousands separator inside
            wxString thousands_sep = CPLGetConfigOption("GDAL_LOCALE_SEPARATOR", "");
            if (thousands_sep == ",") {
                for (int i=0; i<rows; ++i) {
                    tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
                    tmp.Replace(thousands_sep, "");
                    double val;
                    if (tmp.ToDouble(&val)) {
                        data[i] = val;
                    } else if (tmp.IsEmpty()) {
                        data[i] = 0;
                    } else {
                        conv_success = false;
                        break;
                    }
                }
            } else {
                // try comma as decimal point
                setlocale(LC_NUMERIC, "de_DE");
                for (int i=0; i<rows; ++i) {
                    tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
                    double val;
                    if (tmp.ToDouble(&val)) {
                        data[i] = val;
                    } else if (tmp.IsEmpty()) {
                        data[i] = 0;
                    } else {
                        conv_success = false;
                        break;
                    }
                    data[i] = val;
                }
                setlocale(LC_NUMERIC, "C");
            }
        }
        
        if (conv_success == false ) {
            wxString error_msg;
            error_msg << "Fill data error: can't convert '" << tmp
            << "' to floating-point number.";
            throw GdaException(error_msg.mb_str());
        }
    }
}

void OGRColumnString::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            wxInt64 val;
            if (!new_data[i].ToLongLong(&val)) {
                wxString error_msg;
                error_msg << "Fill data error: can't convert '" << new_data[i]
                << "' to floating-point number.";
                throw GdaException(error_msg.mb_str());
            }
            data[i] = val;
        }
    } else {
        int col_idx = GetColIndex();
        bool conv_success = true;
        wxString tmp;
        
        // default C locale
        for (int i=0; i<rows; ++i) {
            wxString tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            wxInt64 val;
            double val_d;
            if (tmp.ToLongLong(&val)) {
                data[i] = val;
                
            } else if (tmp.ToDouble(&val_d)) {
                val = static_cast<wxInt64>(val_d);
                data[i] = val;
                    
            } else if (tmp.IsEmpty()) {
                data[i] = 0;
                
            } else {
                conv_success = false;
                break;
            }
        }
        
        if (conv_success == false) {
            conv_success = true;
            // test if C thousands separator inside
            wxString thousands_sep = CPLGetConfigOption("GDAL_LOCALE_SEPARATOR", "");
            if (thousands_sep == ",") {
                for (int i=0; i<rows; ++i) {
                    tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
                    tmp.Replace(thousands_sep, "");
                    wxInt64 val;
                    double val_d;
                    if (tmp.ToLongLong(&val)) {
                        data[i] = val;
                    } else if (tmp.ToDouble(&val_d)) {
                        val = static_cast<wxInt64>(val_d);
                        data[i] = val;
                    } else if (tmp.IsEmpty()) {
                        data[i] = 0;
                    } else {
                        conv_success = false;
                        break;
                    }
                    
                }
            } else {
                // try comma as decimal point
                setlocale(LC_NUMERIC, "de_DE");
                for (int i=0; i<rows; ++i) {
                    tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
                    wxInt64 val;
                    double val_d;
                    if (tmp.ToLongLong(&val)) {
                        data[i] = val;
                    } else if (tmp.ToDouble(&val_d)) {
                        val = static_cast<wxInt64>(val_d);
                        data[i] = val;
                    } else if (tmp.IsEmpty()) {
                        data[i] = 0;
                    } else {
                        conv_success = false;
                        break;
                    }
                }
                setlocale(LC_NUMERIC, "C");
            }
        }
        
        if (conv_success == false ) {
            wxString error_msg;
            error_msg << "Fill data error: can't convert '" << tmp
            << "' to numeric.";
            throw GdaException(error_msg.mb_str());
        }
    }
}

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

void OGRColumnString::UpdateData(const vector<wxString>& data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            new_data[i] = data[i];
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            ogr_layer->data[i]->SetField(col_idx, data[i].c_str());
            set_markers[i] = true;
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
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            ogr_layer->data[i]->SetField(col_idx, tmp.c_str());
            set_markers[i] = true;
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
            set_markers[i] = true;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp;
            tmp << data[i];
            ogr_layer->data[i]->SetField(col_idx, tmp.c_str());
            set_markers[i] = true;
        }
    }
}

void OGRColumnString::GetCellValue(int row, wxString& val)
{
    if (is_new) {
        val = new_data[row];
    } else {
        int col_idx = GetColIndex();
        const char* val = ogr_layer->data[row]->GetFieldAsString(col_idx);
        val = wxString(val);
    }
}

wxString OGRColumnString::GetValueAt(int row_idx, int disp_decimals,
                                     wxCSConv* m_wx_encoding)
{
    if (is_new) {
        if (set_markers[row_idx] == false )
            return wxEmptyString;
        return new_data[row_idx];
    } else {
        int col_idx = GetColIndex();
        if (col_idx == -1) return wxEmptyString;
        const char* val = ogr_layer->data[row_idx]->GetFieldAsString(col_idx);
        if (m_wx_encoding == NULL) return wxString(val);
        else return wxString(val,*m_wx_encoding);
    }
}

void OGRColumnString::SetValueAt(int row_idx, const wxString &value)
{
    if (is_new) {
        new_data[row_idx] = value;
    } else {
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, value.c_str());
    }
    set_markers[row_idx] = true;
}

////////////////////////////////////////////////////////////////////////////////
// XXX current GeoDa don't support adding new date column
//
OGRColumnDate::OGRColumnDate(OGRLayerProxy* ogr_layer, int idx)
:OGRColumn(ogr_layer, idx)
{
    is_new = false;
    
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

void OGRColumnDate::FillData(vector<double> &data)
{
    wxString error_msg;
    error_msg << "Internal error: GeoDa doesn't support fill floating-point "
    << "data array with Date column.";
    throw GdaException(error_msg.mb_str());
}

void OGRColumnDate::FillData(vector<wxString> &data)
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

void OGRColumnDate::GetCellValue(int row, wxInt64& val)
{
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

void OGRColumnTime::FillData(vector<double> &data)
{
    wxString error_msg;
    error_msg << "Internal error: GeoDa doesn't support fill floating-point "
    << "data array with Time column.";
    throw GdaException(error_msg.mb_str());
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

void OGRColumnTime::GetCellValue(int row, wxInt64& val)
{
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

void OGRColumnDateTime::FillData(vector<double> &data)
{
    wxString error_msg;
    error_msg << "Internal error: GeoDa doesn't support fill floating-point "
    << "data array with Date column.";
    throw GdaException(error_msg.mb_str());
}

void OGRColumnDateTime::FillData(vector<wxString> &data)
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

void OGRColumnDateTime::GetCellValue(int row, wxInt64& val)
{
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