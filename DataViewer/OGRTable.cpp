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
#include <limits>
#include <sstream>
#include <algorithm>
#include <vector>
#include <set>
#include <boost/date_time.hpp>
#include <boost/foreach.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <wx/grid.h>
#include <wx/regex.h>


#include "../GeoDa.h"
#include "../logger.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GdaException.h"
#include "OGRColumn.h"
#include "OGRTable.h"
#include "OGRTableOperation.h"
#include "VarOrderMapper.h"

OGRTable::OGRTable(int n_rows)
: TableInterface(NULL, NULL)
{
    // This is in-memory table only.
    ogr_layer = NULL;
    rows = n_rows;
}

OGRTable::OGRTable(OGRLayerProxy* _ogr_layer,
                   GdaConst::DataSourceType ds_type,
                   TableState* table_state,
                   TimeState* time_state,
                   const VarOrderPtree& var_order_ptree)
: TableInterface(table_state, time_state),
ogr_layer(_ogr_layer), var_order(var_order_ptree), datasource_type(ds_type)
{
	wxLogMessage("Entering OGRTable::OGRTable");
    cols_case_sensitive = OGRLayerProxy::IsFieldCaseSensitive(ds_type);

    // default UTF-8, use can change it in menu: Table->Encoding
    encoding_type = wxFONTENCODING_SYSTEM;
    m_wx_encoding = NULL;

	for (size_t i=0; i<ogr_layer->fields.size(); ++i) {
        AddOGRColumn(ogr_layer, i);
    }
   
	rows = ogr_layer->n_rows;
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	changed_since_last_save = false;
	project_changed_since_last_save = false;
	is_valid = true;
    
    table_state->registerObserver(this);
}

OGRTable::~OGRTable()
{
    for ( int i=0; i<columns.size(); ++i ) {
        delete columns[i];
    }
    columns.clear();
    if (m_wx_encoding) {
        delete m_wx_encoding;
    }
    encoding_type = wxFONTENCODING_SYSTEM;
	wxLogMessage("In OGRTable::~OGRTable");
}

void OGRTable::Update(const VarOrderPtree& var_order_ptree)
{
    var_order.Update(var_order_ptree);
    table_state->SetRefreshEvtTyp();
    table_state->notifyObservers();
    
	SetProjectChangedSinceLastSave(true);
}

void OGRTable::update(TableState* o)
{
    if (o->GetEventType() == TableState::col_order_change) {
        this->SetChangedSinceLastSave(true);
    }
}

GdaConst::DataSourceType OGRTable::GetDataSourceType()
{
    return datasource_type;
}

bool OGRTable::PermitRenameSimpleCol()
{
    return true;
}

bool OGRTable::HasFixedLengths()
{
    if (datasource_type == GdaConst::ds_dbf ||
        datasource_type == GdaConst::ds_shapefile) {
        return true;
    }
    return false;
}

bool OGRTable::PermitChangeLength()
{
    if (datasource_type == GdaConst::ds_dbf ||
        datasource_type == GdaConst::ds_shapefile) {
        return true;
    }
    return false;
}

bool OGRTable::HasFixedDecimals()
{
    if (datasource_type == GdaConst::ds_dbf ||
        datasource_type == GdaConst::ds_shapefile) {
        return true;
    }
    return false;
}

bool OGRTable::PermitChangeDecimals()
{
    if (datasource_type == GdaConst::ds_dbf ||
        datasource_type == GdaConst::ds_shapefile) {
        return true;
    }
    return false;
}

bool OGRTable::PermitChangeDisplayedDecimals()
{
    return true;
}

void OGRTable::AddOGRColumn(OGRLayerProxy* ogr_layer_proxy, int idx)
{
    GdaConst::FieldType type = ogr_layer_proxy->GetFieldType(idx);
    OGRColumn* ogr_col = NULL;
    if (type == GdaConst::long64_type){
        ogr_col = new OGRColumnInteger(ogr_layer_proxy,idx);
        
    } else if (type==GdaConst::double_type){
        ogr_col = new OGRColumnDouble(ogr_layer_proxy,idx);
        
    } else if (type==GdaConst::string_type){
        ogr_col = new OGRColumnString(ogr_layer_proxy,idx);
        
    } else if (type==GdaConst::date_type){
        ogr_col = new OGRColumnDate(ogr_layer_proxy,idx);
        
    } else if (type==GdaConst::time_type){
        ogr_col = new OGRColumnTime(ogr_layer_proxy,idx);
        
    } else if (type==GdaConst::datetime_type){
        ogr_col = new OGRColumnDateTime(ogr_layer_proxy,idx);
        
    } else {
        wxString msg = _("Add OGR column error. Field type is unknown.");
        throw GdaException(msg.mb_str());
    }
    columns.push_back(ogr_col);
    org_var_names.push_back(ogr_col->GetName());
}

// Following 2 functions are for in-memory OGRTable
void OGRTable::AddOGRColumn(OGRColumn* ogr_col)
{
    int pos = columns.size();
	VarGroup g(ogr_col->GetName(), ogr_col->GetDecimals());
    var_order.InsertVarGroup(g, pos);
    
    columns.push_back(ogr_col);
    org_var_names.push_back(ogr_col->GetName());
}

OGRColumn* OGRTable::GetOGRColumn(int idx)
{
    return columns[idx];
}

void OGRTable::GetTimeStrings(std::vector<wxString>& tm_strs)
{
	tm_strs = var_order.GetTimeIdsRef();
}

void OGRTable::GetColNonPlaceholderTmStrs(int col,
										  std::vector<wxString>& tm_strs)
{
	if (!IsColTimeVariant(col)) {
		tm_strs.resize(1);
		tm_strs[0] = var_order.GetTimeIdsRef()[0];
	}  else {
		tm_strs.clear();
		int tms = GetColTimeSteps(col);
		for (int t=0; t<tms; ++t) {
			if (GetColType(col, t) != GdaConst::placeholder_type) {
				tm_strs.push_back(GetTimeString(t));
			}
		}
	}
}

wxString OGRTable::GetTimeString(int time)
{
	if (time >= 0 && time < var_order.GetNumTms()) {
		return var_order.GetTimeIdsRef()[time];
	}
	return wxEmptyString;
}

int OGRTable::GetTimeInt(const wxString& tm_string)
{
	int t=0;
	BOOST_FOREACH(const wxString& ts, var_order.GetTimeIdsRef()) {
		if (tm_string == ts) return t;
		++t;
	}
	return -1;
}

bool OGRTable::IsTimeVariant()
{
    int n_vargrp = var_order.GetNumVarGroups();
    for (int i=0; i<n_vargrp; i++){
        if (!var_order.FindVarGroup(i).IsSimple())
            return true;
    }
	
    return false;
}

int OGRTable::GetTimeSteps()
{
	return var_order.GetNumTms();
}

wxString OGRTable::GetTableName()
{
    if (ogr_layer)
        return ogr_layer->name;
    return "NO_NAME";
}

bool OGRTable::Save(wxString& err_msg)
{
    // OGRTable::Save() will only be used for OGR sources that supports Update
    // (e.g. OGR databases, ESRI File Geodatabase etc.)
    // Other OGR File datasources, which is read only or doesn't support Update
    // will throw a GdaException and will be handled by
    // Project::SaveOGRDataSource() function
    if (!IsReadOnly() ) {
        try {
            while (!operations_queue.empty()) {
                OGRTableOperation* op = operations_queue.front();
                op->Commit();
                completed_stack.push(op);
                operations_queue.pop();
            }
        } catch(...) {
            while (!completed_stack.empty()) {
                OGRTableOperation* op = completed_stack.top();
                op->Rollback();
                operations_queue.push(op);
                completed_stack.pop();
            }
            err_msg << _("GeoDa can't save changes to this datasource. Please try to use File->Export.");
            return false;
        }
        // clean Operations
        while (!completed_stack.empty()) {
            //XXX needs work
            OGRTableOperation* op = completed_stack.top();
            delete op;
            completed_stack.pop();
        }
        SetChangedSinceLastSave(false);
        return true;
    }
    // if it's readonly, it can be and will be exported.So we set no "Change"
	SetChangedSinceLastSave(false);
    wxString msg = _("GeoDa can't save changes to this datasource. Please try to use File->Export.");
    throw GdaException(msg.mb_str(), GdaException::NORMAL);
	return false;
}

bool OGRTable::IsReadOnly()
{
	return !ogr_layer->is_writable;
}

bool OGRTable::DbColNmToColAndTm(const wxString& name, int& col, int& tm)
{
	return var_order.SimpleColNameToColAndTm(name, col, tm, cols_case_sensitive);
}


/** Returns the column id in the underlying grid, not the visual grid
 displayed order.  wxNOT_FOUND is returned if not found.  Always
 returns the first result found. */
int OGRTable::FindColId(const wxString& name)
{
	wxString c_name = name;
	c_name.Trim(false);
	c_name.Trim(true);
	return var_order.GetColId(c_name, cols_case_sensitive);
}


int OGRTable::GetColIdx(const wxString& name, bool ignore_case)
{
    wxString c_name = name;
    c_name.Trim(false);
    c_name.Trim(true);

    for (size_t i=0; i<org_var_names.size(); i++) {
        if (ignore_case) {
            if (name.CmpNoCase(org_var_names[i]) ==0)
                return i;
        } else {
            if (name == org_var_names[i] )
                return i;
        }
    }
    return -1;
}

/** If there is an associated wxGrid, then return the column ids in the order
 they are displayed in the table.  Otherwise, just return 0, 1, 2, ... 
 A mapping from displayed col order to actual col ids in table
 Eg, in underlying table, we might have A, B, C, D, E, F,
 but because of user wxGrid col reorder operaions might see these
 as C, B, A, F, D, E.  In this case, the col_map would be
 0->2, 1->1, 2->0, 3->5, 4->3, 5->4  */
void OGRTable::FillColIdMap(std::vector<int>& col_map)
{
	col_map.resize(GetNumberCols());
	// This is a small cheat.  Perhaps the current column order
	// should be stored in TableInterface rather than getting it from
	// wxGrid
	wxGrid* grid = GdaFrame::GetProject()->FindTableGrid();
	if (grid) {
		for (int i=0, e=col_map.size(); i<e; i++) {
			col_map[grid->GetColPos(i)]=i;
		}
	} else {
		for (int i=0, e=col_map.size(); i<e; i++) {
			col_map[i] = i;
		}
	}
}

/** Similar to FillColIdMap except this is a map of numeric type columns
 only.  The size of the resulting corresponds to the number of numeric
 columns */
void OGRTable::FillNumericColIdMap(std::vector<int>& col_map)
{
	std::vector<int> t;
	FillColIdMap(t);
	int numeric_cnt = 0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (GetColType(t[i]) == GdaConst::long64_type ||
			GetColType(t[i]) == GdaConst::double_type) {
			numeric_cnt++;
		}
	}
	col_map.resize(numeric_cnt);
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (GetColType(t[i]) == GdaConst::long64_type ||
			GetColType(t[i]) == GdaConst::double_type) {
			col_map[cnt++] = t[i];
		}
	}
}

void OGRTable::FillStringAndIntegerColIdMap(std::vector<int>& col_map)
{
    std::vector<int> t;
    FillColIdMap(t);
    int numeric_cnt = 0;
    for (int i=0, iend=t.size(); i<iend; i++) {
        if (GetColType(t[i]) == GdaConst::long64_type ||
            GetColType(t[i]) == GdaConst::string_type) {
            numeric_cnt++;
        }
    }
    col_map.resize(numeric_cnt);
    int cnt=0;
    for (int i=0, iend=t.size(); i<iend; i++) {
        if (GetColType(t[i]) == GdaConst::long64_type ||
            GetColType(t[i]) == GdaConst::string_type) {
            col_map[cnt++] = t[i];
        }
    }
}

void OGRTable::FillStringColIdMap(std::vector<int>& col_map)
{
    std::vector<int> t;
    FillColIdMap(t);
    int string_cnt = 0;
    for (int i=0, iend=t.size(); i<iend; i++) {
        if (GetColType(t[i]) == GdaConst::string_type)
            string_cnt++;
    }
    col_map.resize(string_cnt);
    int cnt=0;
    for (int i=0, iend=t.size(); i<iend; i++) {
        if (GetColType(t[i]) == GdaConst::string_type)
            col_map[cnt++] = t[i];
    }
}

void OGRTable::FillDateTimeColIdMap(std::vector<int>& col_map)
{
    std::vector<int> t;
    FillColIdMap(t);
    int numeric_cnt = 0;
    for (int i=0, iend=t.size(); i<iend; i++) {
        if (GetColType(t[i]) == GdaConst::date_type ||
            GetColType(t[i]) == GdaConst::datetime_type) {
            numeric_cnt++;
        }
    }
    col_map.resize(numeric_cnt);
    int cnt=0;
    for (int i=0, iend=t.size(); i<iend; i++) {
        if (GetColType(t[i]) == GdaConst::date_type ||
            GetColType(t[i]) == GdaConst::datetime_type) {
            col_map[cnt++] = t[i];
        }
    }
}

/** Similar to FillColIdMap except this is a map of long64 type columns
 only.  The size of the resulting corresponds to the number of numeric
 columns */
void OGRTable::FillIntegerColIdMap(std::vector<int>& col_map)
{
	std::vector<int> t;
	FillColIdMap(t);
	int numeric_cnt = 0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (GetColType(t[i]) == GdaConst::long64_type) {
			numeric_cnt++;
		}
	}
	col_map.resize(numeric_cnt);
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (GetColType(t[i]) == GdaConst::long64_type) {
			col_map[cnt++] = t[i];
		}
	}
}

void OGRTable::FillNumericNameList(std::vector<wxString>& num_names)
{
	std::vector<int> t;
	FillNumericColIdMap(t);
	num_names.resize(t.size());
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		num_names[i] = GetColName(t[i]);
	}
}

//FillStringColIdMap
void OGRTable::FillStringNameList(std::vector<wxString>& num_names)
{
	std::vector<int> t;
	FillStringColIdMap(t);
	num_names.resize(t.size());
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		num_names[i] = GetColName(t[i]);
	}
}

/**
 * Return the number of columns that displayed in wxGrid.
 *
 * Note: This number may be less than the actual number of columns in
 * underlying OGRTable.
 */
int OGRTable::GetNumberCols()
{
	return var_order.GetNumVarGroups();
}

int OGRTable::GetNumberRows()
{
	return rows;
}

bool OGRTable::IsColTimeVariant(int col)
{
	return !var_order.FindVarGroup(col).IsSimple();
}

int OGRTable::GetColTimeSteps(int col)
{
	return var_order.FindVarGroup(col).GetNumTms();
}

bool OGRTable::IsColNumeric(int col)
{
	return (GetColType(col) == GdaConst::double_type ||
			GetColType(col) == GdaConst::long64_type ||
            GetColType(col) == GdaConst::date_type ||
            GetColType(col) == GdaConst::time_type ||
            GetColType(col) == GdaConst::datetime_type
            );
}

GdaConst::FieldType OGRTable::GetColType(int col)
{
	int ts = GetColTimeSteps(col);
	GdaConst::FieldType type = GdaConst::unknown_type;
	for (int t=0; t<ts; ++t) {
		type = GetColType(col, t);
		if (type != GdaConst::placeholder_type &&
			type != GdaConst::unknown_type)
            return type;
	}
	return type;
}

std::vector<GdaConst::FieldType> OGRTable::GetColTypes(int col)
{
	size_t ts = GetColTimeSteps(col);
	std::vector<GdaConst::FieldType> ret(ts);
	for (size_t t=0; t<ts; ++t) {
		ret[t] = GetColType(col, t);
        if (ret[t] == GdaConst::placeholder_type) {
            throw GdaException("Placeholder found in grouped column.");
        }
	}
	return ret;
}

GdaConst::FieldType OGRTable::GetColType(int col, int time)
{
	OGRColumn* ogr_col = FindOGRColumn(col, time);
	return ogr_col ? ogr_col->GetType() : GdaConst::placeholder_type;
}


bool OGRTable::DoesNameExist(const wxString& name, bool case_sensitive) const
{
	return var_order.DoesNameExist(name, case_sensitive);
}

int OGRTable::GetFirstNumericCol()
{
    int n = GetNumberCols(); // var_order size
    for (size_t i=0; i<n; i++) {
        if (IsColNumeric(i)) {
            return i;
        }
    }
    return 0;
}

/** Return the Group column name. */
wxString OGRTable::GetColName(int col)
{
	wxString name_in_project = var_order.FindVarGroup(col).name;
    
    return name_in_project;
}

/** Should return column name of actual Table */
wxString OGRTable::GetColName(int col, int time)
{
    bool is_for_display = true;
	OGRColumn* ogr_col = FindOGRColumn(col, time);
	return ogr_col ? ogr_col->GetName() : "";
}

int OGRTable::GetColLength(int col, int time)
{
	OGRColumn* ogr_col = FindOGRColumn(col, time);
	return ogr_col ? ogr_col->GetLength() : 0;
}

int OGRTable::GetColDecimals(int col, int time)
{
	OGRColumn* ogr_col = FindOGRColumn(col, time);
	return ogr_col ? ogr_col->GetDecimals() : 0;
}

int OGRTable::GetColDispDecimals(int col)
{
    // Displayed decimals will be configured in project file
	VarGroup vg = var_order.FindVarGroup(col);
	if (vg.GetDispDecs() == -1) {
        if (GetColType(col) == GdaConst::double_type)
            return GdaConst::default_display_decimals;
        else
            return -1;
	} else {
        //Use user specified display decimals (in gda project file)
		return vg.GetDispDecs();
	}
}

void OGRTable::GetColData(int col, GdaFlexValue& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	VarGroup vg = var_order.FindVarGroup(col);
	if (vg.IsEmpty()) return;
    std::vector<wxString> vars;
	vg.GetVarNames(vars);
	size_t tms = vars.size();
    std::vector<int> ftr_c(tms); // OGRFeature column id
	for (size_t t=0; t<vars.size(); ++t) {
		ftr_c[t] = vars[t].IsEmpty() ? -1 : FindOGRColId(vars[t]);
	}
	
	data.SetSize(rows, tms);
	std::valarray<double>& V = data.GetValArrayRef();
	std::valarray<double> v_tmp(rows);
	const double quiet_nan = std::numeric_limits<double>::quiet_NaN();
	for (size_t t=0; t<tms; ++t) {
		if (ftr_c[t] != -1) {
            int col_idx = ftr_c[t];
            std::vector<double> data(rows, quiet_nan);
            columns[col_idx]->FillData(data);
			for (size_t i=0; i<rows; ++i) {
				v_tmp[i] = data[i];
			}
			V[std::slice(t,rows,tms)] = v_tmp;
		} else {
			V[std::slice(t,rows,tms)] = quiet_nan;
		}
	}
}

/**
 * d_array_type is boost::multi_array<double, 2>
 */
void OGRTable::GetColData(int col, d_array_type& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	VarGroup vg = var_order.FindVarGroup(col);
	if (vg.IsEmpty()) return;
    std::vector<wxString> vars;
	vg.GetVarNames(vars);
	size_t tms = vars.size();
    std::vector<int> ftr_c(tms); // OGRFeature column id
	for (size_t t=0; t<vars.size(); ++t) {
		ftr_c[t] = vars[t].IsEmpty() ? -1 : FindOGRColId(vars[t]);
	}
	
	data.resize(boost::extents[tms][rows]);
	for (size_t t=0; t<tms; ++t) {
		if (ftr_c[t] != -1) {
            int col_idx = ftr_c[t];
            std::vector<double> d(rows, 0);
            columns[col_idx]->FillData(d);
			for (size_t i=0; i<rows; ++i) {
				data[t][i] = d[i];
			}
		} else {
			for (size_t i=0; i<rows; ++i) data[t][i] = 0;
		}
	}
}

void OGRTable::GetColData(int col, l_array_type& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	VarGroup vg = var_order.FindVarGroup(col);
	if (vg.IsEmpty()) return;
    std::vector<wxString> vars;
	vg.GetVarNames(vars);
	size_t tms = vars.size();
    std::vector<int> ftr_c(tms); // OGRFeature column id
	for (size_t t=0; t<vars.size(); ++t) {
		ftr_c[t] = vars[t].IsEmpty() ? -1 : FindOGRColId(vars[t]);
	}
	
	data.resize(boost::extents[tms][rows]);
	for (size_t t=0; t<tms; ++t) {
		if (ftr_c[t] != -1) {
            int col_idx = ftr_c[t];
            std::vector<wxInt64> d(rows, 0);
            columns[col_idx]->FillData(d);
			for (size_t i=0; i<rows; ++i) {
				data[t][i] = d[i];
			}
		} else {
			for (size_t i=0; i<rows; ++i) data[t][i] = 0;
		}
	}
}

void OGRTable::GetColData(int col, s_array_type& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups())
        return;
    
	VarGroup vg = var_order.FindVarGroup(col);
    
	if (vg.IsEmpty())
        return;
    
    std::vector<wxString> vars;
	vg.GetVarNames(vars);
    
	size_t tms = vars.size();
    std::vector<int> ftr_c(tms); // OGRFeature column id
	for (size_t t=0; t<vars.size(); ++t) {
		ftr_c[t] = vars[t].IsEmpty() ? -1 : FindOGRColId(vars[t]);
	}
	
	data.resize(boost::extents[tms][rows]);
	for (size_t t=0; t<tms; ++t) {
		if (ftr_c[t] != -1) {
            int col_idx = ftr_c[t];
            std::vector<wxString> d(rows);
            columns[col_idx]->FillData(d, m_wx_encoding);
			for (size_t i=0; i<rows; ++i) {
				data[t][i] = d[i];
			}
		} else {
			for (size_t i=0; i<rows; ++i) data[t][i] = "";
		}
	}
}

int OGRTable::GetDirectColIdx(wxString col_nm)
{
    for (size_t i=0; i<columns.size(); ++i) {
        OGRColumn* ogr_col = columns[i];
        if (col_nm == ogr_col->GetName()) {
            return i;
        }
    }
    return -1;
}

void OGRTable::GetDirectColData(int col, std::vector<double>& data)
{
    // using underneath columns[]
    if (col < 0 || col >= columns.size())
        return;
    
    OGRColumn* ogr_col = columns[col];
    if (ogr_col == NULL) return;
    data.resize(rows);
    ogr_col->FillData(data);
}

void OGRTable::GetDirectColData(int col, std::vector<wxInt64>& data)
{
    // using underneath columns[]
    if (col < 0 || col >= columns.size())
        return;
    
    OGRColumn* ogr_col = columns[col];
    if (ogr_col == NULL) return;
    data.resize(rows);
    ogr_col->FillData(data);
}

void OGRTable::GetDirectColData(int col, std::vector<wxString>& data)
{
    // using underneath columns[]
    if (col < 0 || col >= columns.size())
        return;
    
    OGRColumn* ogr_col = columns[col];
    if (ogr_col == NULL) return;
    data.resize(rows);
    ogr_col->FillData(data, m_wx_encoding);
}

void OGRTable::GetDirectColData(int col, std::vector<unsigned long long>& data)
{
    // using underneath columns[]
    if (col < 0 || col >= columns.size())
        return;
    
    OGRColumn* ogr_col = columns[col];
    if (ogr_col == NULL) return;
    data.resize(rows);
    ogr_col->FillData(data);
}

void OGRTable::GetColData(int col, int time, std::vector<double>& data)
{
	//if (!IsColNumeric(col)) return;
	wxString nm(var_order.GetSimpleColName(col, time));
	if (nm.IsEmpty()) return;
	OGRColumn* ogr_col = FindOGRColumn(nm);
	if (ogr_col == NULL) return;
	data.resize(rows);
    ogr_col->FillData(data);
}

void OGRTable::GetColData(int col, int time, std::vector<wxInt64>& data)
{
	//if (!IsColNumeric(col)) return;
	wxString nm(var_order.GetSimpleColName(col, time));
	if (nm.IsEmpty()) return;
	OGRColumn* ogr_col = FindOGRColumn(nm);
	if (ogr_col == NULL) return;
	data.resize(rows);
    ogr_col->FillData(data);
}

void OGRTable::GetColData(int col, int time, std::vector<wxString>& data)
{
	wxString nm(var_order.GetSimpleColName(col, time));
	if (nm.IsEmpty()) return;
	OGRColumn* ogr_col = FindOGRColumn(nm);
	if (ogr_col == NULL) return;
	data.resize(rows);
    ogr_col->FillData(data, m_wx_encoding);
}

void OGRTable::GetColData(int col, int time, std::vector<unsigned long long>& data)
{
    wxString nm(var_order.GetSimpleColName(col, time));
    if (nm.IsEmpty()) return;
    OGRColumn* ogr_col = FindOGRColumn(nm);
    if (ogr_col == NULL) return;
    data.resize(rows);
    ogr_col->FillData(data);
}

void OGRTable::GetDataByColumns(const std::vector<wxString>& col_names,
                                std::vector<std::vector<double> >& data,
                                std::vector<std::vector<bool> >& undefs)
{
    // column names could be time (grouped) variable
    int col_idx;
    int tm_idx;
    int dt_idx = 0;
    wxString col_name;
    wxString sub_col_name;
    wxString time_id;
    int n_col_names = col_names.size();
    for (int i=0; i<n_col_names; i++) {
        col_name = col_names[i];
        col_idx = FindColId(col_name);
        tm_idx = 0;
        if (col_idx == -1) {
            // could be a time (grouped) variable: xxx_(xxx)
            sub_col_name = col_name.BeforeFirst(' ');
            time_id = col_name.After('(').Before(')');
            tm_idx = GetTimeInt(time_id);
            col_idx = FindColId(sub_col_name);
        }
        GetColData(col_idx, tm_idx, data[dt_idx]);
        GetColUndefined(col_idx, tm_idx, undefs[dt_idx]);
        dt_idx += 1;
    }
}

bool OGRTable::GetColUndefined(int col, b_array_type& undefined)
{
    if (col < 0 || col >= var_order.GetNumVarGroups())
        return false;
    
    VarGroup vg = var_order.FindVarGroup(col);
    
    if (vg.IsEmpty()) return false;
    
    std::vector<wxString> vars;
    vg.GetVarNames(vars);
    size_t tms = vars.size();
    std::vector<int> ftr_c(tms); // OGRFeature column id
    for (size_t t=0; t<vars.size(); ++t) {
        ftr_c[t] = vars[t].IsEmpty() ? -1 : FindOGRColId(vars[t]);
    }
    bool has_undefined = false;
    undefined.resize(boost::extents[tms][rows]);
    for (size_t t=0; t<tms; ++t) {
        if (ftr_c[t] != -1) {
            int col_idx = ftr_c[t];
            std::vector<bool> markers = columns[col_idx]->GetUndefinedMarkers();
            for (size_t i=0; i<rows; ++i) {
                undefined[t][i] = markers[i];
                if (undefined[t][i]) has_undefined = true;
            }
        } else {
            for (size_t i=0; i<rows; ++i) undefined[t][i] = false;
        }
    }
    return has_undefined;
}

bool OGRTable::GetColUndefined(int col, int time, std::vector<bool>& undefined)
{
	if (col < 0 || col >= GetNumberCols())
        return false;

    int ogr_col_id = FindOGRColId(col, time);
    
	if (ogr_col_id == wxNOT_FOUND)  return false;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    
    undefined = ogr_col->GetUndefinedMarkers();
    
    for (size_t i=0; i<undefined.size(); i++) {
        if (undefined[i] == true) return true;
    }
    return false;
}

bool OGRTable::GetDirectColUndefined(int col, std::vector<bool>& undefined)
{
    if (col < 0 || col >= columns.size())
        return false;
    
    OGRColumn* ogr_col = columns[col];
    
    undefined = ogr_col->GetUndefinedMarkers();
    
    for (size_t i=0; i<undefined.size(); i++) {
        if (undefined[i] == true) return true;
    }
    return false;
}

/**
 * min_vals, max_vals: the values of same column at different time steps
 *
 */
void OGRTable::GetMinMaxVals(int col, std::vector<double>& min_vals,
							 std::vector<double>& max_vals)
{
	if (col < 0 || col >= GetNumberCols()) return;
	if (!IsColNumeric(col)) return;
	min_vals.clear();
	max_vals.clear();
	double tmp_min_val, tmp_max_val, tmp;
	
	VarGroup vg = var_order.FindVarGroup(col);
	if (vg.IsEmpty()) return;
	int times = vg.GetNumTms();
    
    std::vector<wxString> vars;
	vg.GetVarNames(vars);

	for (size_t t=0; t<times; ++t) {
		int col_idx = vars[t].IsEmpty() ? -1 : FindOGRColId(vars[t]);
		if (col_idx != -1) {
            std::vector<double> data(rows, 0);
            std::vector<bool> undef(rows, false);
            columns[col_idx]->FillData(data, undef);
            bool has_init = false;
			for (size_t i=0; i<rows; ++i) {
                if (undef[i])  continue;
				tmp = data[i];
                if (!has_init) {
                    has_init = true;
                    tmp_min_val = tmp;
                    tmp_max_val = tmp;
                    continue;
                }
                if ( tmp_min_val > tmp ) tmp_min_val = tmp;
                if ( tmp_max_val < tmp ) tmp_max_val = tmp;
			}
            if (has_init) {
                min_vals.push_back(tmp_min_val);
                max_vals.push_back(tmp_max_val);
            }
		}
	}
}

bool OGRTable::GetMinMaxVals(int col, int time,
							 double& min_val, double& max_val)
{
	min_val = 0;
	max_val = 0;
	if (col < 0 || col >= GetNumberCols()) return false;
	if (!IsColNumeric(col)) return false;
	if (time < 0 || time > GetColTimeSteps(col)) return false;

	std::vector<double> t_min;
	std::vector<double> t_max;
    GetMinMaxVals(col, t_min, t_max);
    if (t_min.empty() || t_max.empty()) {
        return false;
    }
    min_val = t_min[time];
    max_val = t_max[time];
    return true;
}

void OGRTable::SetColData(int col, int time,
                          const std::vector<double>& data)
{
	if (col < 0 || col >= GetNumberCols())
        return;
	if (!IsColNumeric(col)) return;
	int ogr_col_id = FindOGRColId(col, time);
	if (ogr_col_id == wxNOT_FOUND) return;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    operations_queue.push(new OGRTableOpUpdateColumn(ogr_col, data));
    ogr_col->UpdateData(data);
	table_state->SetColDataChangeEvtTyp(ogr_col->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

void OGRTable::SetColData(int col, int time,
                          const std::vector<wxInt64>& data)
{
	if (col < 0 || col >= GetNumberCols()) return;
	if (!IsColNumeric(col)) return;
	int ogr_col_id = FindOGRColId(col, time);
	if (ogr_col_id == wxNOT_FOUND) return;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    operations_queue.push(new OGRTableOpUpdateColumn(ogr_col, data));
    ogr_col->UpdateData(data);
	table_state->SetColDataChangeEvtTyp(ogr_col->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

void OGRTable::SetColData(int col, int time, 
						  const std::vector<wxString>& data)
{
	if (col < 0 || col >= GetNumberCols()) return;
    int ogr_col_id = FindOGRColId(col, time);
	if (ogr_col_id == wxNOT_FOUND) return;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    operations_queue.push(new OGRTableOpUpdateColumn(ogr_col, data));
    ogr_col->UpdateData(data);
	table_state->SetColDataChangeEvtTyp(ogr_col->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

void OGRTable::SetColData(int col, int time,
                          const std::vector<unsigned long long>& data)
{
    if (col < 0 || col >= GetNumberCols()) return;
    int ogr_col_id = FindOGRColId(col, time);
    if (ogr_col_id == wxNOT_FOUND) return;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    operations_queue.push(new OGRTableOpUpdateColumn(ogr_col, data));
    ogr_col->UpdateData(data);
    table_state->SetColDataChangeEvtTyp(ogr_col->GetName(), col);
    table_state->notifyObservers();
    SetChangedSinceLastSave(true);
}

void OGRTable::SetColUndefined(int col, int time,
							   const std::vector<bool>& undefs)
{
    if (col < 0 || col >= GetNumberCols()) return;
    int ogr_col_id = FindOGRColId(col, time);
    if (ogr_col_id == wxNOT_FOUND) return;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    ogr_col->UpdateNullMarkers(undefs);
	return;
}

/**
 * OGR data sources (mostly) immediately commit any changes to field
 * to the original source.  Furthermore, length and decimals properties
 * are mostly ignored by OGR data sources.  Therefore this method
 * always returns false at present.
 */
bool OGRTable::ColChangeProperties(int col, int time,
								   int new_len, int new_dec)
{
	if (col < 0 || col >= GetNumberCols()) return false;
    int ogr_col_id = FindOGRColId(col, time);
	if (ogr_col_id == wxNOT_FOUND) return false;
    
    OGRColumn* ogr_col = columns[ogr_col_id];
    operations_queue.push(new OGRTableOpUpdateField(ogr_col, new_len, new_dec));
    ogr_col->SetLength(new_len);
    ogr_col->SetDecimals(new_dec);
    var_order.SetDisplayedDecimals(col, new_dec); // visually change
    
    table_state->SetColPropertiesChangeEvtTyp(GetColName(col), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
    
	return true;
}

bool OGRTable::ColChangeDisplayedDecimals(int col, int new_disp_dec)
{
	if (!PermitChangeDisplayedDecimals() ||
		GetColType(col) != GdaConst::double_type) return false;
	
	var_order.SetDisplayedDecimals(col, new_disp_dec);
	table_state->SetColDispDecimalsEvtTyp(GetColName(col), col);
	table_state->notifyObservers();
	//SetChangedSinceLastSave(true);
    
	return true;
}

bool OGRTable::RenameGroup(int col, const wxString& new_name)
{
    if (!IsColTimeVariant(col)) {
        return RenameSimpleCol(col, 0, new_name);
    }
    // may allow e.g. ID->id
    bool case_sensitive = cols_case_sensitive;
	if (DoesNameExist(new_name, case_sensitive) ||
		!IsValidGroupName(new_name)) return false;

    wxString old_name = GetColName(col);
	var_order.SetGroupName(col, new_name);
	table_state->SetColRenameEvtTyp(old_name, new_name, false);
	table_state->notifyObservers();
	SetProjectChangedSinceLastSave(true);
    
	return true;
}

/** This changes the underlying DB column/field name, not the group
 name.  If it is a non-time-variant group, then the DB column and
 group name are the same. */
bool OGRTable::RenameSimpleCol(int col, int time, const wxString& new_name)
{
	if (!PermitRenameSimpleCol()) return false;
    if (col < 0 || col >= GetNumberCols()) return false;
   
    int ogr_col_id = FindOGRColId(col, time);
	if (ogr_col_id == wxNOT_FOUND) return false;
    
	wxString old_name = GetColName(col, time);
    
    OGRColumn* cur_col = columns[ogr_col_id];
    operations_queue.push(new OGRTableOpRenameColumn(cur_col,
                                                     cur_col->GetName(),
                                                     new_name));
    cur_col->Rename(new_name);
    
    //update var_map
    org_var_names[ogr_col_id] = new_name;

    // update variable parameters
    var_order.SetGroupName(col, new_name);
    
    // update Table
    if (table_state) {
        table_state->SetColRenameEvtTyp(old_name, new_name, true);
        table_state->notifyObservers();
        SetChangedSinceLastSave(true);
    }
	return true;
}

int OGRTable::GetCellStringLength(int row, int col, bool use_disp_decimal)
{
    int str_length = 0;
    int ts = GetColTimeSteps(col);
    for (int t=0; t<ts; ++t) {
        wxString val = GetCellString(row, col, t, use_disp_decimal);
        if (val.length() > str_length) str_length = val.length();
    }
    return str_length;
}

wxString OGRTable::GetCellString(int row, int col, int time, bool use_disp_decimal)
{
	// NOTE: if called from wxGrid, must use row_order[row] to permute
	if (row < 0 || row >= rows) return wxEmptyString;
	GdaConst::FieldType cur_type = GetColType(col,time);
    if (cur_type == GdaConst::placeholder_type ||
        cur_type == GdaConst::unknown_type) {
        return wxEmptyString;
    }
    
	// mapping col+time to underneath OGR col
    OGRColumn* ogr_col = FindOGRColumn(col, time);
	if (ogr_col == NULL) {
		return "";
	}
    // get display number of decimals
    int col_dec = GetColDecimals(col, time);
    if (use_disp_decimal) col_dec = GetColDispDecimals(col);
    return ogr_col->GetValueAt(row, col_dec, m_wx_encoding);
}


// Note: Aditionally, must check that all numbers
//       are valid and set undefined flag appropriately.  Also, this
//       method should only be called by wxGrid since we automatically
//       compute the correct row.
bool OGRTable::SetCellFromString(int row, int col, int time,
								 const wxString &value)
{
	// NOTE: if called from wxGrid, must use row_order[row] to permute
	is_set_cell_from_string_fail = false;
	if (row<0 || row>=rows) return false;
	if (col<0 || col>=GetNumberCols()) return false;
	
	int t_col = FindOGRColId(col, time);
	if (t_col == -1) {
		return false;
	} else {
		if (table_state->GetNumDisallowGroupModify(GetColName(col)) > 0) {
			is_set_cell_from_string_fail = true;
			set_cell_from_string_fail_msg =
				table_state->GetDisallowGroupModifyMsg(GetColName(col));
			return false;
		}
	}
    operations_queue.push(new OGRTableOpUpdateCell(columns[t_col], row, value));
	columns[t_col]->SetValueAt(row, value, m_wx_encoding);
	SetChangedSinceLastSave(true);
    table_state->SetColDataChangeEvtTyp(GetColName(col), col);
	table_state->notifyObservers();
	return true;
}

int OGRTable::InsertCol(GdaConst::FieldType type,
                        const wxString& name,
                        int pos,
                        int time_steps,
                        int field_len,
                        int decimals)
{
    if (pos > GetNumberCols() || time_steps < 0 ) {
        return -1;
    }
    
    // don't support the following column type
	if (type == GdaConst::placeholder_type ||
        type == GdaConst::unknown_type)
    {
        return -1;
    }
    
    // if for appending new column at the end of table
    if (pos < 0) {
        pos = GetNumberCols();
    }
    
    if (type != GdaConst::double_type) {
        decimals = 0;
    }
    
	if (field_len == -1) {
		if (type == GdaConst::double_type) {
			field_len = GdaConst::default_dbf_double_len;
		} else if (type == GdaConst::long64_type) {
			field_len = GdaConst::default_dbf_long_len;
		} else {  // type == GdaConst::string_type
			field_len = GdaConst::default_dbf_string_len;
		}
	}
    
	if (decimals < 0) {
		if (type == GdaConst::double_type) {
			decimals = GdaConst::default_dbf_double_decimals;
		} else {
			decimals = 0;
		}
	}
	
    // note the differences between "Group/Ungroup" and "Append new column"
    // (new field always added to the end of existing columns)
    
    std::vector<wxString> names(SuggestDBColNames(name, name, time_steps));

    // return could be group of names (e.g. pop2001, pop2002, pop2003)
	for (size_t t=0; t<names.size(); t++) {
        
        OGRColumn* ogr_col = NULL;
        if (type == GdaConst::long64_type){
            ogr_col = new OGRColumnInteger(ogr_layer, names[t], field_len,decimals);
            
        } else if (type==GdaConst::double_type){
            ogr_col = new OGRColumnDouble(ogr_layer, names[t], field_len, decimals);
            
        } else if (type==GdaConst::string_type){
            ogr_col = new OGRColumnString(ogr_layer, names[t], field_len, decimals);
            
        } else if (type==GdaConst::date_type){
            ogr_col = new OGRColumnDate(ogr_layer, names[t], field_len, decimals);
            
        } else if (type==GdaConst::time_type){
            ogr_col = new OGRColumnTime(ogr_layer, names[t], field_len, decimals);
            
        } else if (type==GdaConst::datetime_type){
            ogr_col = new OGRColumnDateTime(ogr_layer, names[t], field_len, decimals);
            
        } else {
            wxString msg = _("Add OGR column error. Field type is unknown or not supported.");
            throw GdaException(msg.mb_str());
        }
        columns.insert(columns.begin()+pos, ogr_col);
        operations_queue.push(new OGRTableOpInsertColumn(ogr_col));
        
        std::vector<wxString>::iterator iter = org_var_names.begin() + pos;
        org_var_names.insert(iter, names[t]);
	}
    // when init a column, set display decimals to -1 so that UI will determine
    // the value based on actual dicmals.
    int group_disp_decimals = -1;
	VarGroup g(name, group_disp_decimals);
    if (time_steps > 1) {
        g.vars = names;
    }
	var_order.InsertVarGroup(g, pos);
	
	SetChangedSinceLastSave(true);
	
	TableDeltaList_type tdl;
	TableDeltaEntry tde(name, true, pos);
	tde.pos_final = pos;
	tde.decimals = decimals < 0 ? GdaConst::default_dbf_double_decimals : decimals;
    // to keep screenshot the same, when display decimals is larger
    // (e.g. DBF uses 15 decimals) than default display decimals(6),
    // use default display decimals
    if (type==GdaConst::double_type)
        tde.displayed_decimals = GdaConst::default_display_decimals;
	tde.type = type;
	tde.length = field_len;
	tde.change_to_db = true;
	tdl.push_back(tde);
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
	return pos;
}

bool OGRTable::DeleteCol(int pos)
{
	wxLogMessage("Inside OGRTable::DeleteCol");
	if (pos < 0 || pos >= var_order.GetNumVarGroups() ||
		var_order.GetNumVarGroups() == 0) {
        return false;
    }

    bool case_sensitive = cols_case_sensitive;
    wxString col_name = var_order.GetGroupName(pos);
	if (!col_name.IsEmpty()) {
        for( size_t i=0; i<columns.size(); ++i) {
            if (col_name.IsSameAs(columns[i]->GetName(), case_sensitive)) {
                operations_queue.push(new OGRTableOpDeleteColumn(columns[i]));
                columns.erase(columns.begin()+i);
                break;
            }
        }
	}
    
    for (size_t i=0; i<org_var_names.size(); i++) {
        if (col_name.IsSameAs(org_var_names[i], case_sensitive) ) {
            org_var_names.erase( org_var_names.begin() + i );
            break;
        }
    }
		
	var_order.RemoveVarGroup(pos);
	
	TableDeltaList_type tdl;
	TableDeltaEntry tde(col_name, false, pos);
	tde.change_to_db = true;
	tdl.push_back(tde);
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
    
	SetChangedSinceLastSave(true);
    wxLogMessage("Exit OGRTable::DeleteCol");
	return true;
}

void OGRTable::UngroupCol(int col) 
{
	wxLogMessage("Inside OGRTable::UngroupCol");
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (GetColTimeSteps(col) <= 1) return;
	
    std::map<wxString, GdaConst::FieldInfo> nm_to_fi;
	for (size_t t=0; t<GetColTimeSteps(col); ++t) {
		GdaConst::FieldInfo fi;
		fi.type = GetColType(col, t);
		if (fi.type == GdaConst::placeholder_type ||
			fi.type == GdaConst::unknown_type)
            continue;
		fi.field_len = GetColLength(col, t);
		fi.decimals = GetColDecimals(col, t);
		nm_to_fi[GetColName(col, t)] = fi;
	}
	int displayed_decimals = GetColDispDecimals(col);
	
	TableDeltaList_type tdl;
	var_order.Ungroup(col, tdl, cols_case_sensitive);
	
	// Add missing information to tdl entries.
	for (TableDeltaList_type::iterator i=tdl.begin(); i!=tdl.end(); ++i) {
		if (!i->insert)
            continue;
		GdaConst::FieldInfo& fi = nm_to_fi[i->group_name];
		i->type = fi.type;
		i->decimals = fi.decimals;
		i->displayed_decimals = displayed_decimals;
		i->length = fi.field_len;
	}
	
	SetProjectChangedSinceLastSave(true);
    
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
}

void OGRTable::GroupCols(const std::vector<int>& cols,
						 const wxString& name, int pos) 
{
	wxLogMessage("Inside OGRTable::GroupCols");
	if (pos < 0 || pos > var_order.GetNumVarGroups()) return;
	if (cols.size() <= 1) return;
	if (GetTimeSteps() > 1 && cols.size() != GetTimeSteps()) return;
	
	if (cols.size() > GetTimeSteps()) {
		// need to increase number of time steps
		RenameTimeStep(0, "time 0");
		for (size_t t=1; t<cols.size(); t++) {
			var_order.InsertTime(t, wxString::Format("time %d", (int) t));
		}
		time_state->SetTimeIds(var_order.GetTimeIdsRef());
		table_state->SetTimeIdsAddRemoveEvtTyp();
		table_state->notifyObservers();
	}
	
	// Determine group type information from first non-placeholder
	// field in cols.
	int decimals = 0;
	int displayed_decimals = 0;
	int length = -1;
	GdaConst::FieldType type = GdaConst::unknown_type;
	bool found_nonplaceholder = false;
	for (size_t i=0; i<cols.size() && !found_nonplaceholder; ++i) {
		if (cols[i] >= 0 &&
            GetColType(cols[i]) != GdaConst::unknown_type &&
            GetColType(cols[i]) != GdaConst::placeholder_type)
        {
			decimals = GetColDecimals(cols[i]);
			displayed_decimals = GetColDispDecimals(cols[i]);
			length = GetColLength(cols[i]);
			type = GetColType(cols[i]);
			found_nonplaceholder = true;
		}
	}
	
    if (decimals <=0) decimals = GdaConst::default_dbf_double_decimals;
    if (displayed_decimals<=0) displayed_decimals = GdaConst::default_display_decimals;
    
	TableDeltaList_type tdl;
	var_order.Group(cols, name, pos, tdl, cols_case_sensitive);
	// Last entry in tdl should be an insert operation.  Add missing
	// information to this operation.
	TableDeltaList_type::iterator i=tdl.end();
	--i;
	i->decimals = decimals;
	i->displayed_decimals = displayed_decimals;
	i->length = length;
	i->type = type;
	
	SetProjectChangedSinceLastSave(true);
    
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
}

void OGRTable::InsertTimeStep(int time, const wxString& name)
{
	if (time < 0 || time > var_order.GetNumTms()) return;
    
	var_order.InsertTime(time, name);
    
	SetProjectChangedSinceLastSave(true);
    
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsAddRemoveEvtTyp();
	table_state->notifyObservers();
}

void OGRTable::RemoveTimeStep(int time)
{
	if (time < 0 || time >= var_order.GetNumTms()) return;
	// TableDeltaList is needed for the case where removing a time
	// period results in onr or more VarGroup with only placeholders remaining.
	// In this case we need to delete these empty columns from the table.
	TableDeltaList_type tdl;
	var_order.RemoveTime(time, tdl, cols_case_sensitive);
	
	// Must fill in details for simple inserted columns
	for (TableDeltaList_type::iterator i=tdl.begin(); i!=tdl.end(); ++i) {
		if (!i->insert) continue;
		int ogr_field_id = FindOGRColId(i->group_name);
		i->decimals = columns[ogr_field_id]->GetDecimals();
		i->length = columns[ogr_field_id]->GetLength();
		i->type = columns[ogr_field_id]->GetType();
	}
	
	if (tdl.size() > 0) {
		// Notify of columns that were removed because they only contained
		// placeholders.
		table_state->SetColsDeltaEvtTyp(tdl);
		table_state->notifyObservers();
	}
	
	SetProjectChangedSinceLastSave(true);
    
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsAddRemoveEvtTyp();
	table_state->notifyObservers();
}

void OGRTable::SwapTimeSteps(int time1, int time2)
{
	var_order.SwapTimes(time1, time2);
    
	SetProjectChangedSinceLastSave(true);
    
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsSwapEvtTyp();
	table_state->notifyObservers();
}

void OGRTable::RenameTimeStep(int time, const wxString& new_name)
{
	var_order.RenameTime(time, new_name);
    
	SetProjectChangedSinceLastSave(true);
    
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsRenameEvtTyp();
	table_state->notifyObservers();
}

/**
 * Return the column (field) position in OGR table by given a time step
 * and a column position in current wxGrid.  wxGrid automatically keeps
 * track of columns that have been moved.
 */
int OGRTable::FindOGRColId(int wxgrid_col_pos, int time)
{
    wxString name = var_order.GetSimpleColName(wxgrid_col_pos, time);
	return FindOGRColId(name);
    //const VarGroup& vg = var_order.FindVarGroup(wxgrid_col_pos);
    //vg.GetNameByTime(time);
}

int OGRTable::FindOGRColId(const wxString& name)
{
    bool case_sensitive = cols_case_sensitive;
    for (size_t i=0; i < org_var_names.size(); i++ ) {
        if (name.IsSameAs(org_var_names[i], case_sensitive)) {
            return i;
        }
    }
    return -1;
}

OGRColumn* OGRTable::FindOGRColumn(int col, int time)
{
    
	wxString nm(var_order.GetSimpleColName(col, time));
	return FindOGRColumn(nm);
}

OGRColumn* OGRTable::FindOGRColumn(const wxString& name)
{
    if (name.IsEmpty()) return NULL;
    bool case_sensitive = cols_case_sensitive;
    for (size_t i=0; i<org_var_names.size(); i++ ) {
        if ( name.IsSameAs(org_var_names[i], case_sensitive)) {
            return columns[i];
        }
    }
    return NULL;
}

bool OGRTable::IsValidDBColName(const wxString& col_nm, 
							    wxString* fld_warn_msg)
{
    if ( GdaConst::datasrc_field_lens.find(datasource_type) ==
         GdaConst::datasrc_field_lens.end() )
    {
        // no valid entry in datasrc_field_lens, could be a unwritable ds
		if ( fld_warn_msg ) {
			*fld_warn_msg = _("This datasource is not supported. Please export to other datasource that GeoDa supports first.");
		}
        return false;
    }
    
    int field_len = GdaConst::datasrc_field_lens[datasource_type];
    if ( field_len < col_nm.length() ) {
		if ( fld_warn_msg ) {
		    *fld_warn_msg = _("The length of field name should be between 1 and %d.\nCurrent field length (%d) is not valid");
            *fld_warn_msg = wxString::Format(*fld_warn_msg, (int)field_len, (int)col_nm.length());
		}
		return false;
	}
    
    wxString field_regex = GdaConst::datasrc_field_regex[datasource_type];
    wxRegEx regex;
    regex.Compile(field_regex);
    if ( regex.Matches(col_nm) ) {
        return true;
    }

	if ( fld_warn_msg ) {
    	*fld_warn_msg = GdaConst::datasrc_field_warning[datasource_type];
	}
    return false;
}

void OGRTable::AddMetaInfo(const wxString col_nm, const wxString& key,
                           const wxString& val)
{
    // Add meta info for a specific colmn
    int col_id = var_order.GetColId(col_nm, true);
    if (col_id <0) return;
    var_order.AddMetaInfo(col_id, key, val);
}

std::map<wxString, wxString> OGRTable::GetMetaData(int col_id)
{
    VarGroup vg = var_order.FindVarGroup(col_id);
    return vg.meta_data;
}
