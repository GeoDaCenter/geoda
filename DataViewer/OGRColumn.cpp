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
        for (int i=0; i<rows; ++i) {
            wxString tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            double val;
            if (!tmp.ToDouble(&val)) {
                wxString error_msg;
                error_msg << "Fill data error: can't convert '" << tmp
                << "' to floating-point number.";
                throw GdaException(error_msg.mb_str());
            }
            data[i] = val;
        }
    }
}

void OGRColumnString::FillData(vector<wxInt64> &data)
{
    if (is_new) {
        for (int i=0; i<rows; ++i) {
            long val;
            if (!new_data[i].ToLong(&val)) {
                wxString error_msg;
                error_msg << "Fill data error: can't convert '" << new_data[i]
                << "' to floating-point number.";
                throw GdaException(error_msg.mb_str());
            }
            data[i] = val;
        }
    } else {
        int col_idx = GetColIndex();
        for (int i=0; i<rows; ++i) {
            wxString tmp=wxString(ogr_layer->data[i]->GetFieldAsString(col_idx));
            long val;
            if (!tmp.ToLong(&val)) {
                wxString error_msg;
                error_msg << "Fill data error: can't convert '" << tmp
                << "' to floating-point number.";
                throw GdaException(error_msg.mb_str());
            }
            data[i] = val;
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
            int col_idx = GetColIndex();
            int year=0;
            int month=0;
            int day=0;
            int hour=0;
            int minute = 0;
            int seconds = 0;
            int tzflag = 0;
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &seconds, &tzflag);
            wxInt64 val = year* 10000 + month*100 + day;
            data[i] = val;
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
            int year=0;
            int month=0;
            int day=0;
            int hour=0;
            int minute = 0;
            int seconds = 0;
            int tzflag = 0;
            ogr_layer->data[i]->GetFieldAsDateTime(col_idx, &year, &month,
                                               &day,&hour,&minute,
                                               &seconds, &tzflag);
            wxInt64 val = year* 10000 + month*100 + day;
            data[i] = wxString::Format("%i", val);
        }
    }
}

void OGRColumnDate::GetCellValue(int row, wxInt64& val)
{
    if (new_data.empty()) {
        int col_idx = GetColIndex();
        int year=0;
        int month=0;
        int day=0;
        int hour=0;
        int minute = 0;
        int seconds = 0;
        int tzflag = 0;
        ogr_layer->data[row]->GetFieldAsDateTime(col_idx, &year, &month,
                                                     &day,&hour,&minute,
                                                     &seconds, &tzflag);
        val = year* 10000 + month*100 + day;
    } else {
        val = new_data[row];
    }
}

wxString OGRColumnDate::GetValueAt(int row_idx, int disp_decimals,
                                   wxCSConv* m_wx_encoding)
{
    wxInt64 val;
    GetCellValue(row_idx, val);
    return wxString::Format("%lld", val);
}

void OGRColumnDate::SetValueAt(int row_idx, const wxString &value)
{
    // XXX don't support adding new date column
    wxInt64 l_val;
    const char* tmp = (const char*)value.mb_str();
    bool valid = GenUtils::validInt(const_cast<char*>(tmp));
    if (value.length() == 6 && valid) {
        GenUtils::strToInt64(const_cast<char*>(tmp), &l_val);
        int n_year = l_val/10000;
        int n_month = (l_val % 10000) /100;
        int n_day = l_val % 10;
        int col_idx = GetColIndex();
        ogr_layer->data[row_idx]->SetField(col_idx, n_year, n_month, n_day);
        //modifed_features.push_back(feature);
    }
}