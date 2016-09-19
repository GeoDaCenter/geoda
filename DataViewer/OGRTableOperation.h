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

#ifndef __GEODA_CENTER_OGR_TABLE_OPERATION_H__
#define __GEODA_CENTER_OGR_TABLE_OPERATION_H__

#include <vector>
#include "OGRColumn.h"

using namespace std;

/**
 * OGRTableOperation
 *   OGRTableOpInsertColumn
 *   OGRTableOpDeleteColumn
 *   OGRTableOpRenameColumn
 *   OGRTableOpUpdateColumn
 *   OGRTableOpUpdateCell
 */
class OGRTableOperation
{
public:
    OGRTableOperation(OGRColumn* col);
    virtual ~OGRTableOperation();
    
protected:
    OGRColumn* ogr_col;
    OGRLayerProxy* ogr_layer;
    GdaConst::FieldType ogr_type;
    
public:
    virtual void Commit() = 0;
    virtual void Rollback() = 0;
};

/**
 *
 */
class OGRTableOpInsertColumn : public OGRTableOperation
{
public:
    OGRTableOpInsertColumn(OGRColumn* col);
    ~OGRTableOpInsertColumn(){}
    
    virtual void Commit();
    virtual void Rollback();
};

/**
 *
 */
class OGRTableOpDeleteColumn : public OGRTableOperation
{
public:
    OGRTableOpDeleteColumn(OGRColumn* col);
    ~OGRTableOpDeleteColumn();
    
    virtual void Commit();
    virtual void Rollback();
};

/**
 *
 */
class OGRTableOpRenameColumn : public OGRTableOperation
{
private:
    wxString old_col_name;
    wxString new_col_name;
public:
    OGRTableOpRenameColumn(OGRColumn* col, wxString old_name,
                           wxString new_name);
    ~OGRTableOpRenameColumn(){}
    
    virtual void Commit();
    virtual void Rollback();
};

/**
 * Update the field (specifically, length & decimals) in actual OGRTable
 */
class OGRTableOpUpdateField: public OGRTableOperation
{
private:
    int old_field_length;
    int old_field_decimals;
    int new_field_length;
    int new_field_decimals;
public:
    OGRTableOpUpdateField(OGRColumn* col, int new_flength, int new_fdecimals);
    ~OGRTableOpUpdateField(){}
    
    virtual void Commit();
    virtual void Rollback();
};

/**
 * Content / entire column
 */
class OGRTableOpUpdateColumn : public OGRTableOperation
{
private:
    int n_rows;
    
    vector<bool> undef_old_data, undef_new_data;
    vector<double> d_old_data, d_new_data;
    vector<wxInt64> l_old_data, l_new_data;
    vector<wxString> s_old_data, s_new_data;
    
public:
    OGRTableOpUpdateColumn(OGRColumn* col, const vector<double>& new_data);
    OGRTableOpUpdateColumn(OGRColumn* col, const vector<wxInt64>& new_data);
    OGRTableOpUpdateColumn(OGRColumn* col, const vector<wxString>& new_data);
    ~OGRTableOpUpdateColumn(){}
    
    virtual void Commit();
    virtual void Rollback();
};

/**
 * Content / cell value
 */
class OGRTableOpUpdateCell : public OGRTableOperation
{
private:
    int row_idx;
    int col_idx;
    double d_old_value, d_new_value;
    wxInt64 l_old_value, l_new_value;
    wxString s_old_value, s_new_value;
    void GetOriginalCellValue();
    
public:
    OGRTableOpUpdateCell(OGRColumn* col, int row_idx ,double new_val);
    OGRTableOpUpdateCell(OGRColumn* col, int row_idx ,wxInt64 new_val);
    OGRTableOpUpdateCell(OGRColumn* col, int row_idx ,wxString new_val);
    ~OGRTableOpUpdateCell(){}
    
    virtual void Commit();
    virtual void Rollback();
};

#endif