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

#include <limits>
#include <set>
#include <boost/foreach.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <wx/grid.h>
#include <wx/regex.h>

#include "../Project.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "DbfColContainer.h"
#include "DbfTable.h"

using namespace std;

/** It is assumed that var_order has been corrected against the data
 in dbf.  If this assumption is wrong, results are unpredictable.
 */
DbfTable::DbfTable(TableState* table_state, TimeState* time_state,
				   DbfFileReader& dbf, const VarOrderPtree& var_order_ptree)
: TableInterface(table_state, time_state), orig_header(dbf.getFileHeader()),
var_order(var_order_ptree)
{
	LOG_MSG("Entering DbfTable::DbfTable");
    encoding_type = wxFONTENCODING_UTF8;
	changed_since_last_save = false;
	rows = dbf.getNumRecords();
	int cols = dbf.getNumFields();
	dbf_file_name = wxFileName(dbf.fname);
	
	int num_tms = var_order.GetNumTms();

    encoding_type = wxFONTENCODING_UTF8;
	m_wx_encoding = new wxCSConv(wxFONTENCODING_UTF8);
	map<wxString, DbfFieldDesc> desc_map;
	vector<DbfFieldDesc> desc_vec = dbf.getFieldDescs();
	BOOST_FOREACH(const DbfFieldDesc& desc, desc_vec) {
		desc_map[desc.name] = desc;
	}
	// If displayed decimals attribute in var_order is set to
	// default, then set displayed decimals to decimals
	for (int i=0, num_grps=var_order.GetNumVarGroups(); i<num_grps; ++i) {
		VarGroup g = var_order.FindVarGroup(i);
		if (g.IsSimple()) {
			map<wxString, DbfFieldDesc>::iterator it;
			it = desc_map.find(g.GetGroupName());
			if (it != desc_map.end() && 
				(it->second.type == 'F' || it->second.type == 'N') &&
				it->second.decimals != 0)
			{
				var_order.SetDisplayedDecimals(i, it->second.decimals);
			}
		}
	}
	BOOST_FOREACH(const VarGroup& g, var_order.GetVarGroupsRef()) {
		GdaConst::FieldInfo info;
		if (g.vars.size() == 0) {
			FillFieldInfoFromDesc(info, desc_map[g.name]);
			var_map[g.name] = new DbfColContainer;
			var_map[g.name]->Init(rows, info, true, false, false);
		} else {
			BOOST_FOREACH(const wxString& v, g.vars) {
				if (!v.empty()) { // skip placeholders
					FillFieldInfoFromDesc(info, desc_map[v]);
					var_map[v] = new DbfColContainer;
					var_map[v]->Init(rows, info, true, false, false);
				}
			}
		}
	}
	
	if (!dbf.file.is_open()) {
		//dbf.file.open(dbf.fname.mb_str(wxConvUTF8),
		dbf.file.open(GET_ENCODED_FILENAME(dbf.fname),
					  std::ios::in | std::ios::binary);
	}
	if (!(dbf.file.is_open() && dbf.file.good())) {
		open_err_msg << "Problem reading from DBF file.";
		return;
	}
	
	// To speed this up, make a map from col number to DbfColContainer*
	vector<DbfColContainer*> quick_map(desc_vec.size());
	for (size_t i=0; i<desc_vec.size(); ++i) {
		quick_map[i] = var_map[desc_vec[i].name];
	}
	
	// Note: first byte of every DBF row is the record deletion flag, so
	// we always skip this.
	int del_flag_len = 1;  // the record deletion flag
	dbf.file.seekg(dbf.header.header_length, std::ios::beg);
	for (int row=0; row<rows; row++) {
		dbf.file.seekg(del_flag_len, std::ios::cur);
		for (int col=0; col<cols; col++) {
			int field_len = desc_vec[col].length;
			DbfColContainer* c_ptr = quick_map[col];
			//LOG(dbf.file.tellg());
			dbf.file.read((char*)(c_ptr->raw_data + row*(field_len+1)),
						  field_len);
			c_ptr->raw_data[row*(field_len+1)+field_len] = '\0';
			//LOG_MSG(wxString((char*)(c_ptr->raw_data + row*(field_len+1))));
		}
	}
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	is_valid = true;
	LOG_MSG("Exiting DbfTable::DbfTable");
}

DbfTable::~DbfTable()
{
	LOG_MSG("Entering DbfTable::~DbfTable");
	for (std::map<wxString, DbfColContainer*>::iterator it=var_map.begin();
		 it != var_map.end(); it++) {
		delete (it->second);
	}
	LOG_MSG("Exiting DbfTable::~DbfTable");
}


void DbfTable::update(TableState* o)
{
    
}

GdaConst::DataSourceType DbfTable::GetDataSourceType()
{
    return GdaConst::ds_dbf;
}

void DbfTable::GetTimeStrings(std::vector<wxString>& tm_strs)
{
	tm_strs = var_order.GetTimeIdsRef();
}

void DbfTable::GetColNonPlaceholderTmStrs(int col,
										  std::vector<wxString>& tm_strs)
{
	if (!IsColTimeVariant(col)) {
		tm_strs.resize(1);
		tm_strs[0] = var_order.GetTimeIdsRef()[0];
	} else {
		tm_strs.clear();
		int tms = GetColTimeSteps(col);
		for (int t=0; t<tms; ++t) {
			if (GetColType(col, t) != GdaConst::placeholder_type) {
				tm_strs.push_back(GetTimeString(t));
			}
		}
	}
}

wxString DbfTable::GetTimeString(int time)
{
	if (time >= 0 && time < var_order.GetNumTms()) {
		return var_order.GetTimeIdsRef()[time];
	}
	return wxEmptyString;
}

int DbfTable::GetTimeInt(const wxString& tm_string)
{
	int t=0;
	BOOST_FOREACH(const wxString& ts, var_order.GetTimeIdsRef()) {
		if (tm_string == ts) return t;
		++t;
	}
	return -1;
}

bool DbfTable::IsTimeVariant()
{
	return var_order.GetNumTms() > 1;
}

int DbfTable::GetTimeSteps()
{
	return var_order.GetNumTms();
}

wxString DbfTable::GetTableName()
{
	return dbf_file_name.GetName();
}

bool DbfTable::Save(wxString& err_msg)
{
	return WriteToDbf(dbf_file_name.GetFullPath(), err_msg);
}

bool DbfTable::IsReadOnly()
{
	return false;
}

/** Returns the column id in the underlying grid, not the visual grid
 displayed order.  wxNOT_FOUND is returned if not found.  Always
 returns the first result found. */
int DbfTable::FindColId(const wxString& name)
{
	wxString c_name = name;
	c_name.Trim(false);
	c_name.Trim(true);	
	return var_order.GetColId(c_name);
}

/** If there is an associated wxGrid, then return the column ids in the order
 they are displayed in the table.  Otherwise, just return 0, 1, 2, ... The
 vector is automatically resized to var_order.GetNumVarGroups() 
 A mapping from displayed col order to actual col ids in table
 Eg, in underlying table, we might have A, B, C, D, E, F,
 but because of user wxGrid col reorder operaions might see these
 as C, B, A, F, D, E.  In this case, the col_map would be
 0->2, 1->1, 2->0, 3->5, 4->3, 5->4  */
void DbfTable::FillColIdMap(std::vector<int>& col_map)
{
	col_map.resize(var_order.GetNumVarGroups());
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
void DbfTable::FillNumericColIdMap(std::vector<int>& col_map)
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

/** Similar to FillColIdMap except this is a map of long64 type columns
 only.  The size of the resulting corresponds to the number of numeric
 columns */
void DbfTable::FillIntegerColIdMap(std::vector<int>& col_map)
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

void DbfTable::FillNumericNameList(std::vector<wxString>& num_names)
{
	std::vector<int> t;
	FillNumericColIdMap(t);
	num_names.resize(t.size());
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		num_names[i] = GetColName(t[i]);
	}
}

/** Number of cols in wxGrid, not total number of DB columns
 */
int DbfTable::GetNumberCols()
{
	return var_order.GetNumVarGroups();
}

int DbfTable::GetNumberRows()
{
	return rows;
}

bool DbfTable::IsColTimeVariant(int col)
{
	return !var_order.FindVarGroup(col).IsSimple();
}

int DbfTable::GetColTimeSteps(int col)
{
	return var_order.FindVarGroup(col).GetNumTms();
}

bool DbfTable::IsColNumeric(int col)
{
	return (GetColType(col) == GdaConst::double_type ||
			GetColType(col) == GdaConst::long64_type ||
			GetColType(col) == GdaConst::date_type);
}

GdaConst::FieldType DbfTable::GetColType(int col)
{
	int ts = GetColTimeSteps(col);
	GdaConst::FieldType type = GdaConst::unknown_type;
	for (int t=0; t<ts; ++t) {
		type = GetColType(col, t);
		if (type != GdaConst::placeholder_type &&
			type != GdaConst::unknown_type) return type;
	}
	return type;
}

std::vector<GdaConst::FieldType> DbfTable::GetColTypes(int col)
{
	size_t ts = GetColTimeSteps(col);
	std::vector<GdaConst::FieldType> ret(ts);
	for (size_t t=0; t<ts; ++t) {
		ret[t] = GetColType(col, t);
	}
	return ret;
}

GdaConst::FieldType DbfTable::GetColType(int col, int time)
{
	DbfColContainer* c = FindDbfCol(col, time);
	return c ? c->GetType() : GdaConst::placeholder_type;
}

bool DbfTable::DoesNameExist(const wxString& name, bool case_sensitive) const
{
	return var_order.DoesNameExist(name, case_sensitive);
}

wxString DbfTable::GetColName(int col)
{
	return var_order.FindVarGroup(col).name;
}

wxString DbfTable::GetColName(int col, int time)
{
	DbfColContainer* c = FindDbfCol(col, time);
	return c ? c->GetDbfColName() : "";
}

int DbfTable::GetColLength(int col, int time)
{
	DbfColContainer* c = FindDbfCol(col, time);
	return c ? c->GetFieldLen() : 0;
}

int DbfTable::GetColDecimals(int col, int time)
{
	DbfColContainer* c = FindDbfCol(col, time);
	return c ? c->GetDecimals() : 0;
}

int DbfTable::GetColDispDecimals(int col)
{
	VarGroup vg = var_order.FindVarGroup(col);
	if (vg.GetDispDecs() == -1) {
		return GdaConst::default_display_decimals;
	} else {
		return vg.GetDispDecs();
	}
	//DbfColContainer* c = FindDbfCol(col, time);
	//return c ? c->GetDispDecimals() : 0;
}

void DbfTable::GetColData(int col, GdaFlexValue& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	std::vector<DbfColContainer*> cols;
	GetDbfCols(col, cols);
	size_t tms = cols.size();
	data.SetSize(rows, tms);
	std::vector<double> vec;
	std::valarray<double>& V = data.GetValArrayRef();
	std::valarray<double> v_tmp(rows);
	const double quiet_nan = std::numeric_limits<double>::quiet_NaN();
	for (size_t t=0; t<tms; ++t) {
		if (cols[t]) {
			cols[t]->CheckUndefined();
			cols[t]->GetVec(vec);
			for (size_t i=0; i<rows; i++) {
				v_tmp[i] = cols[t]->undefined[i] ? quiet_nan : vec[i];
			}
			V[std::slice(t,rows,tms)] = v_tmp;
		} else {
			V[std::slice(t,rows,tms)] = quiet_nan;
		}
	}	
}

void DbfTable::GetColData(int col, d_array_type& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	std::vector<DbfColContainer*> cols;
	GetDbfCols(col, cols);
	size_t tms = cols.size();
	data.resize(boost::extents[tms][rows]);
	std::vector<double> vec;
	for (size_t t=0; t<tms; ++t) {
		if (cols[t]) {
			cols[t]->CheckUndefined();
			cols[t]->GetVec(vec);
			for (size_t i=0; i<rows; i++) {
				data[t][i] = cols[t]->undefined[i] ? 0 : vec[i];
			}
		} else {
			for (size_t i=0; i<rows; i++) data[t][i] = 0;
		}
	}
}

void DbfTable::GetColData(int col, l_array_type& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	std::vector<DbfColContainer*> cols;
	GetDbfCols(col, cols);
	size_t tms = cols.size();
	data.resize(boost::extents[tms][rows]);
	std::vector<wxInt64> vec;
	for (size_t t=0; t<tms; ++t) {
		if (cols[t]) {
			cols[t]->CheckUndefined();
			cols[t]->GetVec(vec);
			for (size_t i=0; i<rows; i++) {
				data[t][i] = cols[t]->undefined[i] ? 0 : vec[i];
			}
		} else {
			for (size_t i=0; i<rows; i++) data[t][i] = 0;
		}
	}
}

void DbfTable::GetColData(int col, s_array_type& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		|| !IsColNumeric(col)) return;
	std::vector<DbfColContainer*> cols;
	GetDbfCols(col, cols);
	size_t tms = cols.size();
	data.resize(boost::extents[tms][rows]);
	std::vector<wxString> vec;
	for (size_t t=0; t<tms; ++t) {
		if (cols[t]) {
			cols[t]->CheckUndefined();
			cols[t]->GetVec(vec);
			for (size_t i=0; i<rows; i++) {
				data[t][i] = cols[t]->undefined[i] ? "" : vec[i];
			}
		} else {
			for (size_t i=0; i<rows; i++) data[t][i] = "";
		}
	}
}

void DbfTable::GetColData(int col, int time, std::vector<double>& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		||! IsColNumeric(col)) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->CheckUndefined();
	c->GetVec(data);
	for (size_t i=0; i<rows; i++) if (c->undefined[i]) data[i] = 0;
}

void DbfTable::GetColData(int col, int time, std::vector<wxInt64>& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()
		||! IsColNumeric(col)) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->CheckUndefined();
	c->GetVec(data);
	for (size_t i=0; i<rows; i++) if (c->undefined[i]) data[i] = 0;
}

void DbfTable::GetColData(int col, int time, std::vector<wxString>& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->GetVec(data);
}

void DbfTable::GetColUndefined(int col, b_array_type& undefined)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	std::vector<DbfColContainer*> cols;
	GetDbfCols(col, cols);
	size_t tms = cols.size();
	undefined.resize(boost::extents[tms][rows]);
	std::vector<double> vec;
	for (size_t t=0; t<tms; ++t) {
		if (cols[t]) {
			cols[t]->CheckUndefined();
			for (size_t i=0; i<rows; i++) {
				undefined[t][i] = cols[t]->undefined[i];
			}
		} else {
			for (size_t i=0; i<rows; i++) undefined[t][i] = true;
		}
	}
}

void DbfTable::GetColUndefined(int col, int time,
							   std::vector<bool>& undefined)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->GetUndefined(undefined);
}

void DbfTable::GetMinMaxVals(int col, std::vector<double>& min_vals,
							 std::vector<double>& max_vals)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (!IsColNumeric(col)) return;
	std::vector<DbfColContainer*> cols;
	GetDbfCols(col, cols);
	size_t tms = cols.size();
	min_vals.resize(tms);
	max_vals.resize(tms);
	for (size_t t=0; t<tms; ++t) {
		if (cols[t]) {
			cols[t]->GetMinMaxVals(min_vals[t], max_vals[t]);
		} else {
			min_vals[t] = 0;
			max_vals[t] = 0;
		}
	}
}

void DbfTable::GetMinMaxVals(int col, int time,
			   double& min_val, double& max_val)
{
	min_val = 0;
	max_val = 0;
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (!IsColNumeric(col)) return;
	if (time < 0 || time > GetColTimeSteps(col)) return;
	
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->GetMinMaxVals(min_val, max_val);
}

void DbfTable::SetColData(int col, int time,
						  const std::vector<double>& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (!IsColNumeric(col)) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->SetFromVec(data);
	table_state->SetColDataChangeEvtTyp(c->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

void DbfTable::SetColData(int col, int time,
						  const std::vector<wxInt64>& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (!IsColNumeric(col)) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->SetFromVec(data);
	table_state->SetColDataChangeEvtTyp(c->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

void DbfTable::SetColData(int col, int time,
						  const std::vector<wxString>& data)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->SetFromVec(data);
	table_state->SetColDataChangeEvtTyp(c->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

void DbfTable::SetColUndefined(int col, int time,
							   const std::vector<bool>& undefined)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (!IsColNumeric(col)) return;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return;
	c->SetUndefined(undefined);
	table_state->SetColDataChangeEvtTyp(c->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
}

bool DbfTable::ColChangeProperties(int col, int time,
								   int new_len, int new_dec)
{
	if (col < 0 || col >= var_order.GetNumVarGroups()) return false;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return false;
	return c->ChangeProperties(new_len, new_dec);
	table_state->SetColPropertiesChangeEvtTyp(c->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
	return true;
}

bool DbfTable::ColChangeDisplayedDecimals(int col, int new_disp_dec)
{
	if (!PermitChangeDisplayedDecimals() ||
		GetColType(col) != GdaConst::double_type) return false;
	
	var_order.SetDisplayedDecimals(col, new_disp_dec);
	table_state->SetColDispDecimalsEvtTyp(GetColName(col), col);
	table_state->notifyObservers();
	//SetChangedSinceLastSave(true);
	return true;
}

bool DbfTable::RenameGroup(int col, const wxString& new_name)
{
	if (!IsColTimeVariant(col)) return RenameSimpleCol(col, 0, new_name);
	if (DoesNameExist(new_name, false) ||
		!IsValidGroupName(new_name)) return false;
	wxString old_name = GetColName(col);
	
	var_order.SetGroupName(col, new_name);
	table_state->SetColRenameEvtTyp(old_name, new_name, false);
	table_state->notifyObservers();
	//SetChangedSinceLastSave(true);
	return true;
}

/** This changes the underlying DB column/field name, not the group
 name.  If it is a non-time-variant group, then the DB column and
 group name are the same. */
bool DbfTable::RenameSimpleCol(int col, int time, const wxString& new_name)
{
	if (!PermitRenameSimpleCol() ||
		(!IsColTimeVariant(col) && time != 0)) return false;
	if (DoesNameExist(new_name, false) ||
		!IsValidDBColName(new_name)) return false;
	
	wxString old_name = GetColName(col, time);
	std::map<wxString, DbfColContainer*>::iterator i =
		var_map.find(var_order.GetSimpleColName(col));
	if (i == var_map.end()) return false;
	DbfColContainer* c = i->second;
	if (!c->ChangeName(new_name)) return false;
	var_map.erase(i);
	var_map.insert( std::pair<wxString, DbfColContainer*>(new_name, c) );

	var_order.SetGroupName(col, new_name);
	table_state->SetColRenameEvtTyp(old_name, new_name, true);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
	return true;
}

wxString DbfTable::GetCellString(int row, int col, int time)
{
	// NOTE: if called from wxGrid, must use row_order[row] to permute
	if (row<0 || row>=rows) return wxEmptyString;

	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return wxEmptyString;
	int field_len = c->GetFieldLen();
	
	if (c->undefined_initialized &&
		c->undefined[row]) {
		return wxEmptyString;
	}
	
	switch (c->GetType()) {
		case GdaConst::date_type:
		{
			if (c->IsVecDataAlloc()) {
				int x = c->l_vec[row];
				int day = x % 100; x /= 100;
				int month = x % 100; x /= 100;
				int year = x;
				return wxString::Format("%04d %02d %02d", year, month, day);
			}
			if (c->IsRawDataAlloc()) {
				wxString temp((char*)(c->raw_data + row*(field_len+1)));
				long val;
				bool success = temp.ToCLong(&val);
				
				if (c->undefined_initialized || success) {
					int x = val;
					int day = x % 100; x /= 100;
					int month = x % 100; x /= 100;
					int year = x;
					return wxString::Format("%04d %02d %02d", year, month, day);
				} else {
					return wxEmptyString;
				}
			}
		}
		case GdaConst::long64_type:
		{
			if (c->IsVecDataAlloc()) {
				return wxString::Format("%lld", c->l_vec[row]);
			}
			if (c->IsRawDataAlloc()) {
				const char* str = (char*)(c->raw_data + row*(field_len+1));
				//LOG_MSG(wxString::Format("row: %d, col: %d, raw: %s",
				//						 row, col, str));
				
				if (c->undefined_initialized ||
					GenUtils::validInt(str)) {
					wxInt64 val=0;
					GenUtils::strToInt64(str, &val);
					return wxString::Format("%lld", val);
				} else {
					return wxEmptyString;
				}
			}
		}
			break;
		case GdaConst::double_type:
		{
			// We have to be careful to return a formated string with digits
			// after the decimal place at most min(decimals, displayed_decimals)
			int decimals = c->GetDecimals();
            if (decimals >=0 ) decimals += 1; // one extra decimal
			int disp_dec = GetColDispDecimals(col);
			if (disp_dec == -1) disp_dec = decimals-1;
            if ( c->GetDecimals() > 0) {
                disp_dec = GenUtils::min<int>(c->GetDecimals(),disp_dec);
            }
			wxString d_char = DbfColContainer::sprintf_period_for_decimal()
			? "." : ",";
			if (c->IsVecDataAlloc()) {
				double val = c->d_vec[row];
				//MMM: due to the prevalence of DBF files with non-conforming
				// data, we are disabling this feature at present.
				// limit val to acceptable range
				//int d = c->decimals;
				//int fl = c->field_len;
				//double max_val = DbfFileUtils::GetMaxDouble(fl, d);
				//double min_val = DbfFileUtils::GetMinDouble(fl, d);
				//if (max_val < val) {
				//	val = max_val;
				//} else if (min_val > val) {
				//	val = min_val;
				//}
				wxString s = wxString::Format("%.*f", disp_dec, val);
				return s.SubString(0, s.Find(d_char) + disp_dec);
			}
			if (c->IsRawDataAlloc()) {
				wxString temp((char*)(c->raw_data + row*(field_len+1)));
				// trim any leading or trailing spaces.  For some reason
				// a trailing space causes ToCDouble to return false even
				// though it set val to the correct value.
				temp.Trim(true);
				temp.Trim(false);
				double val;
				bool success = temp.ToCDouble(&val);
				if (success) success = boost::math::isfinite<double>(val);
				
				if (c->undefined_initialized || success) {
					wxString s = wxString::Format("%.*f", disp_dec, val);
					return s.SubString(0, s.Find(d_char) + disp_dec);
				} else {
					return wxEmptyString;
				}
			}
		}
			break;
		case GdaConst::string_type:
		{
			if (c->IsVecDataAlloc()) {
				return c->s_vec[row];
			}
			if (c->IsRawDataAlloc()) {
                if (m_wx_encoding == NULL)
                    return wxString((char*)(c->raw_data + row*(field_len+1)));
				return wxString((char*)(c->raw_data + row*(field_len+1)),
                                *m_wx_encoding);
			}
		}
			break;
		default:
			break;
	}
	return wxEmptyString;
}


// Note: when writing to raw_data, we must be careful not to overwrite
//       the buffer and also to respect the DBF formating requirements,
//       especially for floats.  Aditionally, must check that all numbers
//       are valid and set undefined flag appropriately.  Also, this
//       method should only be called by wxGrid since we automatically
//       compute the correct row.
bool DbfTable::SetCellFromString(int row, int col, int time,
								 const wxString &value)
{
	// NOTE: if called from wxGrid, must use row_order[row] to permute
	is_set_cell_from_string_fail = false;
	if (row<0 || row>=rows) return false;
	if (col<0 || col>=rows) return false;
	DbfColContainer* c = FindDbfCol(col, time);
	if (!c) return false;
	
	if (table_state->GetNumDisallowGroupModify(GetColName(col)) > 0) {
		is_set_cell_from_string_fail = true;
		set_cell_from_string_fail_msg =
			table_state->GetDisallowGroupModifyMsg(GetColName(col));
		return false;
	}
	
	int field_len = c->GetFieldLen();
	char temp[2048];
	char* buf=0;
	if (c->IsRawDataAlloc()) {
		buf = c->raw_data + row*(field_len+1);
		buf[field_len] = '\0';
	}
	
	c->stale_min_max_val = true;
	c->UpdateMinMaxVals();
	
	// assume defined by default
	c->undefined[row] = false;
	switch (c->GetType()) {
		case GdaConst::date_type: {
			// first, check that value is valid.  If invalid, we will
			// write some default value and will set undefined to true
			wxInt64 l_val;
			bool valid = GenUtils::validInt(
							const_cast<char*>((const char*)value.mb_str()));
			if (valid) {
				GenUtils::strToInt64(
						const_cast<char*>((const char*)value.mb_str()), &l_val);
			} else {
				c->undefined[row] = true;
			}
			if (c->IsVecDataAlloc()) {
				if (c->undefined[row]) {
					c->l_vec[row] = 0;
				} else {
					c->l_vec[row] = l_val;
				}
			}
			if (buf) {
				if (c->undefined[row]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%*lld", field_len, l_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
				}
			}
			break;
		}
		case GdaConst::long64_type: {
			// first, check that value is valid.  If invalid, we will
			// write some default value and will set undefined to true
			wxInt64 l_val;
			if (!GenUtils::validInt(value)) {
				c->undefined[row] = true;
			} else {
				GenUtils::strToInt64(value, &l_val);
				// limit l_val to acceptable range
				//int fl = c->GetFieldLen();
				//wxInt64 max_val = DbfFileUtils::GetMaxInt(fl);
				//wxInt64 min_val = DbfFileUtils::GetMinInt(fl);
				//if (max_val < l_val) {
				//	l_val = max_val;
				//} else if (min_val > l_val) {
				//	l_val = min_val;
				//}
			}
			
			if (c->IsVecDataAlloc()) {
				if (c->undefined[row]) {
					c->l_vec[row] = 0;
				} else {
					c->l_vec[row] = l_val;
				}
			}
			if (buf) {
				if (c->undefined[row]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%*lld", field_len, l_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
				}
			}
			break;
		}
		case GdaConst::double_type: {
			double d_val;
			if (!value.ToDouble(&d_val)) {
				c->undefined[row] = true;
			} else if (!boost::math::isfinite<double>(d_val)) {
				c->undefined[row] = true;
			}
			if (!c->undefined[row]) {
				// limit d_val to acceptable range
				//int d = c->GetDecimals();
				//int fl = c->GetFieldLen(); 
				//double max_val = DbfFileUtils::GetMaxDouble(fl, d);
				//double min_val = DbfFileUtils::GetMinDouble(fl, d);
				//if (max_val < d_val) {
				//	d_val = max_val;
				//} else if (min_val > d_val) {
				//	d_val = min_val;
				//}
			}
			if (c->IsVecDataAlloc()) {
				if (c->undefined[row]) {
					c->d_vec[row] = 0;
				} else {
					c->d_vec[row] = d_val;
				}
			}
			if (buf) {
				if (c->undefined[row]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%#*.*f", field_len,
							c->GetDecimals(), d_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
					if (!DbfColContainer::sprintf_period_for_decimal()) {
						for (int j=0; j<field_len; j++) {
							if (buf[j] == ',') buf[j] = '.';
						}
					}
				}
			}
			break;
		}
		case GdaConst::string_type: {
			if (c->IsVecDataAlloc()) {
				if (value.Length() > field_len) {
					c->s_vec[row] = value.Mid(0, field_len);	
				} else {
					c->s_vec[row] = value;
				}
			}
			if (c->IsRawDataAlloc()) {
				if (value.IsEmpty()) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					//strncpy(buf,
					// (const char*)value.mb_str(*m_wx_encoding),field_len);
					strncpy(buf,
							(const char*)value.mb_str(wxConvUTF8),field_len);
					buf[field_len]='\0';
				}
			}
			break;
		}
		default:
			break;
	}
	table_state->SetColDataChangeEvtTyp(c->GetName(), col);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
	return true;
}

int DbfTable::InsertCol(GdaConst::FieldType type, const wxString& name,
						int pos, int time_steps, int field_len,
						int decimals)
{
	using namespace std;
	bool alloc_raw_data = false;
	bool mark_all_defined = true;
    if (pos > var_order.GetNumVarGroups())
        return -1;
    
    // this case if for appending new column at the end of table
	if (pos < 0)
        pos = var_order.GetNumVarGroups();
    
	LOG_MSG(wxString::Format("Inserting column into table at postion %d", pos));
	if (time_steps <= 0)
        return -1;
	if (type != GdaConst::double_type)
        decimals = 0;
	if (field_len == -1 && (type == GdaConst::placeholder_type ||
                            type == GdaConst::unknown_type))
        return -1;
	if (field_len == -1) {
		if (type == GdaConst::double_type) {
			field_len = GdaConst::default_dbf_double_len;
		} else if (type == GdaConst::long64_type) {
			field_len = GdaConst::default_dbf_long_len;
		} else if (type == GdaConst::date_type) {
			field_len = GdaConst::default_dbf_date_len;
		} else {  // type == GdaConst::string_type
			field_len = GdaConst::default_dbf_string_len;
		}
	}
	if (decimals < 0) {
		if (type == GdaConst::double_type) {
			decimals = GdaConst::default_display_decimals;
		} else {
			decimals = 0;
		}
	}

    if (name == "geoid2_1") {
        int test = 1;
    }
	vector<wxString> names(SuggestDBColNames(name, name, time_steps));

	GdaConst::FieldInfo info;
	info.type = type;
	info.name = name;
	info.field_len = field_len;
	info.decimals = decimals;
	for (size_t t=0; t<names.size(); t++) {
		DbfColContainer* c = new DbfColContainer;
		info.name = names[t];
		if (type == GdaConst::date_type) {
			// will leave unitialized
			c->Init(rows, info, alloc_raw_data, !alloc_raw_data, false);
			c->undefined_initialized = true;
		} else {
			c->Init(rows, info, alloc_raw_data, !alloc_raw_data,
					mark_all_defined);
		}
		var_map[names[t]] = c;
	}
	
	VarGroup g(name, decimals);
	if (time_steps > 1)
        g.vars = names;
	var_order.InsertVarGroup(g, pos);
	
	SetChangedSinceLastSave(true);

    if (decimals < 0)
        decimals = GdaConst::default_display_decimals;
	TableDeltaList_type tdl;
	TableDeltaEntry tde(name, true, pos);
	tde.pos_final = pos;
	tde.decimals = decimals;
	tde.displayed_decimals = decimals;
	tde.type = type;
	tde.length = field_len;
	tde.change_to_db = true;
	tdl.push_back(tde);
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
	return pos;
}

bool DbfTable::DeleteCol(int pos)
{
	using namespace std;
	LOG_MSG("Inside DbfTable::DeleteCol");
	LOG_MSG(wxString::Format("Deleting column from table at postion %d", pos));
	if (pos < 0 || pos >= var_order.GetNumVarGroups() ||
		var_order.GetNumVarGroups() == 0) return false;
		
	// Must remove all items from var_map first
	vector<DbfColContainer*> cols;
	GetDbfCols(pos, cols);
	for (size_t t=0; t<cols.size(); t++) {
		if (cols[t] != 0) {
			wxString nm = cols[t]->GetDbfColName();
			map<wxString, DbfColContainer*>::iterator i = var_map.find(nm);
			if (i != var_map.end()) var_map.erase(i);
			delete cols[t];
		}
	}

	wxString name = var_order.GetGroupName(pos);
	var_order.RemoveVarGroup(pos);
	
	TableDeltaList_type tdl;
	TableDeltaEntry tde(name, false, pos);
	tde.change_to_db = true;
	tdl.push_back(tde);
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
	SetChangedSinceLastSave(true);
	return true;
}

void DbfTable::UngroupCol(int col)
{
	using namespace std;
	LOG_MSG("Inside DbfTable::UngroupCol");
	if (col < 0 || col >= var_order.GetNumVarGroups()) return;
	if (GetColTimeSteps(col) <= 1) return;
	
	map<wxString, GdaConst::FieldInfo> nm_to_fi;
	for (size_t t=0; t<GetColTimeSteps(col); ++t) {
		GdaConst::FieldInfo fi;
		fi.type = GetColType(col, t);
		if (fi.type == GdaConst::placeholder_type ||
			fi.type == GdaConst::unknown_type) continue;
		fi.field_len = GetColLength(col, t);
		fi.decimals = GetColDecimals(col, t);
		nm_to_fi[GetColName(col, t)] = fi;
	}
	int displayed_decimals = GetColDispDecimals(col);
	
	TableDeltaList_type tdl;
	var_order.Ungroup(col, tdl);
	
	// Add missing information to tdl entries.
	for (TableDeltaList_type::iterator i=tdl.begin(); i!=tdl.end(); ++i) {
		if (!i->insert) continue;
		GdaConst::FieldInfo& fi = nm_to_fi[i->group_name];
		i->type = fi.type;
		i->decimals = fi.decimals;
		i->displayed_decimals = displayed_decimals;
		i->length = fi.field_len;
	}
	LOG_MSG("Table delta entries:");
	BOOST_FOREACH(const TableDeltaEntry& tde, tdl) LOG_MSG(tde.ToString());
	
	//SetChangedSinceLastSave(true);
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
}

void DbfTable::GroupCols(const std::vector<int>& cols,
						 const wxString& name, int pos)
{
	using namespace std;
	LOG_MSG("Inside DbfTable::GroupCols");
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
		if (cols[i] >= 0 && GetColType(cols[i]) != GdaConst::unknown_type
			&& GetColType(cols[i]) != GdaConst::placeholder_type) {
			decimals = GetColDecimals(cols[i]);
			displayed_decimals = GetColDecimals(cols[i]);
			length = GetColLength(cols[i]);
			type = GetColType(cols[i]);
			found_nonplaceholder = true;
		}
	}

	TableDeltaList_type tdl;
	var_order.Group(cols, name, pos, tdl);
	// Last entry in tdl should be an insert operation.  Add missing
	// information to this operation.
	TableDeltaList_type::iterator i=tdl.end();
	--i;
	i->decimals = decimals;
	i->displayed_decimals = displayed_decimals;
	i->length = length;
	i->type = type;
	LOG_MSG("Table delta entries:");
	BOOST_FOREACH(const TableDeltaEntry& tde, tdl) LOG_MSG(tde.ToString());
	
	//SetChangedSinceLastSave(true);
	table_state->SetColsDeltaEvtTyp(tdl);
	table_state->notifyObservers();
}

void DbfTable::InsertTimeStep(int time, const wxString& name)
{
	if (time < 0 || time > var_order.GetNumTms()) return;
	var_order.InsertTime(time, name);
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsAddRemoveEvtTyp();
	table_state->notifyObservers();
}

void DbfTable::RemoveTimeStep(int time)
{
	if (time < 0 || time >= var_order.GetNumTms()) return;
	// TableDeltaList is needed for the case where removing a time
	// period results in onr or more VarGroup with only placeholders remaining.
	// In this case we need to delete these empty columns from the table.
	TableDeltaList_type tdl;
	var_order.RemoveTime(time, tdl);
	
	// Must fill in details for simple inserted columns
	for (TableDeltaList_type::iterator i=tdl.begin(); i!=tdl.end(); ++i) {
		if (!i->insert) continue;
		DbfColContainer* c = var_map[i->group_name];
		i->decimals = c->GetDecimals();
		i->displayed_decimals = c->GetDecimals();
		i->length = c->GetFieldLen();
		i->type = c->GetType();
	}
	
	// Notify Table of inserted and removed columns
	if (tdl.size() > 0) {
		table_state->SetColsDeltaEvtTyp(tdl);
		table_state->notifyObservers();
	}
	
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsAddRemoveEvtTyp();
	table_state->notifyObservers();
}

void DbfTable::SwapTimeSteps(int time1, int time2)
{
	var_order.SwapTimes(time1, time2);
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsSwapEvtTyp();
	table_state->notifyObservers();
}

void DbfTable::RenameTimeStep(int time, const wxString& new_name)
{
	var_order.RenameTime(time, new_name);
	time_state->SetTimeIds(var_order.GetTimeIdsRef());
	table_state->SetTimeIdsRenameEvtTyp();
	table_state->notifyObservers();
}

bool DbfTable::WriteToDbf(const wxString& fname, wxString& err_msg)
{
	std::ofstream out_file;	
	out_file.open(GET_ENCODED_FILENAME(fname),
				  std::ios::out | std::ios::binary);
	
	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Problem opening \"" + fname + "\"";
		return false;
	}
	
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	// We must write the DBF in the current displayed column order
	// However, with since we allow grouped columns, we have to expand
	// each group and ignore placeholders.
	typedef DbfColContainer* dbf_col_ptr;
	std::vector<dbf_col_ptr> dbf_cols;
	GetAllSimpleDbfCols(dbf_cols);	
	
	// Ensure that raw_data exists.  If raw_data exists, then each item is
	// assumed to be ready for writing to disk.
	BOOST_FOREACH(const dbf_col_ptr& c, dbf_cols) {
		if (!c->IsRawDataAlloc()) c->CopyVectorToRawData();
	}
	
	// update orig_header
	orig_header.num_records = GetNumberRows();
	orig_header.num_fields = dbf_cols.size(); // should == var_map.size()
	// Each field descriptor is 32 bits and begins at byte 32 and terminates
	// with an additional byte 0x0D.
	orig_header.header_length = 32 + orig_header.num_fields*32 + 1;
	orig_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	BOOST_FOREACH(const dbf_col_ptr& c, dbf_cols) {
		orig_header.length_each_record += c->GetFieldLen();
	}
	DbfFileHeader header = orig_header;
	
	wxUint32 u_int32;
	wxUint32* u_int32p = &u_int32;
	wxUint16 u_int16;
	wxUint16* u_int16p = &u_int16;
	wxUint8 u_int8;
	wxUint8* u_int8p = &u_int8;
	char membyte;
	
	// byte 0
	membyte = header.version;
	out_file.put(membyte);
	
	// byte 1
	membyte = (char) (header.year - 1900);
	out_file.put(membyte);
	
	// byte 2
	membyte = (char) header.month;
	out_file.put(membyte);
	
	// byte 3
	membyte = (char) header.day;
	out_file.put(membyte);
	
	// byte 4-7
	u_int32 = header.num_records;
	u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
	out_file.write((char*) u_int32p, 4);
	
	// byte 8-9
	u_int16 = header.header_length;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file.write((char*) u_int16p, 2);
	
	// byte 10-11
	u_int16 = header.length_each_record;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file.write((char*) u_int16p, 2);
	
	// byte 12-13 (0x0000)
	u_int16 = 0x0;
	out_file.write((char*) u_int16p, 2);
	
	// bytes 14-31: write 0 
	membyte = 0;
	for (int i=0; i<(31-14)+1; i++) out_file.put(membyte);
	
	// out_file now points to byte 32, which is the beginning of the list
	// of fields.  There must be at least one field.  Each field descriptor
	// is 32 bytes long with byte 0xd following the last field descriptor.
	char* byte32_buff = new char[32];
	
	
	BOOST_FOREACH(const dbf_col_ptr& c, dbf_cols) {
		for (int j=0; j<32; j++) byte32_buff[j] = 0;
		//strncpy(byte32_buff,
		//   (const char*)c->name.mb_str(*m_wx_encoding), 11);
		strncpy(byte32_buff,
				(const char*)c->GetName().mb_str(wxConvUTF8), 11);
		switch (c->GetType()) {
			case GdaConst::date_type:
				byte32_buff[11] = 'D';
				break;
			case GdaConst::long64_type:
				byte32_buff[11] = 'N';
				break;
			case GdaConst::double_type:
				byte32_buff[11] = 'N';
				break;
			default:
				byte32_buff[11] = 'C';
				break;
		}
		byte32_buff[16] = (wxUint8) c->GetFieldLen();
		byte32_buff[17] = (wxUint8) c->GetDecimals();
		out_file.write(byte32_buff, 32);
	}
	delete [] byte32_buff;
	// mark end of field descriptors with 0x0D
	out_file.put((char) 0x0D);
	
	// Write out each record
	for (int row=0; row<header.num_records; row++) {
		out_file.put((char) 0x20); // each record starts with a space character
		BOOST_FOREACH(const dbf_col_ptr& c, dbf_cols) {
			int f_len = c->GetFieldLen();
			out_file.write(c->raw_data + row*(f_len+1), f_len);	
		}
	}
	// 0x1A is the EOF marker
	out_file.put((char) 0x1A);
	out_file.close();
	SetChangedSinceLastSave(false);
	
	return true;	
}

void DbfTable::FillFieldInfoFromDesc(GdaConst::FieldInfo& fi,
									 const DbfFieldDesc& desc)
{
	if (desc.type == 'N' || desc.type == 'F') {
		if (desc.decimals > 0) {
			fi.type = GdaConst::double_type;
		} else {
			fi.type = GdaConst::long64_type;
		}
		fi.decimals = desc.decimals;

	} else if (desc.type == 'D') {
		fi.type = GdaConst::date_type;
		fi.decimals = 0;
	} else {
		// We will assume (desc[i].type == 'C')
		fi.type = GdaConst::string_type;
		fi.decimals = desc.decimals;
	}
	fi.name = desc.name;
	fi.field_len = desc.length;
}

bool DbfTable::DbColNmToColAndTm(const wxString& name, int& col, int& tm)
{
	return var_order.SimpleColNameToColAndTm(name, col, tm);
}

/** Return pointer to DbfColContainer for col and time.  If not found,
 or if placeholder, then null is returned.
 */
DbfColContainer* DbfTable::FindDbfCol(int col, int time)
{
    wxString col_name = var_order.GetSimpleColName(col, time);
	std::map<wxString, DbfColContainer*>::iterator iter = var_map.find(col_name);
    if (iter == var_map.end())
        return 0;
    else
        return iter->second;
}

void DbfTable::GetDbfCols(int col, std::vector<DbfColContainer*>& cols)
{
	VarGroup g(var_order.FindVarGroup(col));
	cols.resize(g.GetNumTms());
	for (size_t t=0, tms=g.GetNumTms(); t<tms; ++t) {
		cols[t] = FindDbfCol(col, t);
	}
}

/** Get pointers to all DbfColContainer in wxGrid order.  Groups will be
 expanded in place and placeholders ignored
 */
void DbfTable::GetAllSimpleDbfCols(std::vector<DbfColContainer*>& cols)
{
	std::vector<int> col_id_map;
	FillColIdMap(col_id_map);
	for (size_t i=0; i<col_id_map.size(); ++i) {
		std::vector<DbfColContainer*> cc;
		GetDbfCols(col_id_map[i], cc);
		for (size_t t=0; t<cc.size(); ++t) if (cc[t]) cols.push_back(cc[t]);
	}
}

bool DbfTable::IsValidDBColName(const wxString& col_nm,
								  wxString* fld_warn_msg)
{
	// Use normal DBF restrctions
    int field_len = GdaConst::datasrc_field_lens[GdaConst::ds_dbf];
    if ( field_len < col_nm.length() ) {
		if ( fld_warn_msg ) {
		    *fld_warn_msg = "The length of field name should be between 1 and ";
		    *fld_warn_msg << field_len  << ".\n"
            << "Current field length (" << col_nm.length() << ") is not valid.";
		}
		return false;
	}
    
    wxString field_regex = GdaConst::datasrc_field_regex[GdaConst::ds_dbf];
    wxRegEx regex;
    regex.Compile( field_regex );
    if ( regex.Matches(col_nm) ) {
        return true;
    }

	if ( fld_warn_msg ) {
    	*fld_warn_msg = GdaConst::datasrc_field_warning[GdaConst::ds_dbf];
	}
    return false;
}