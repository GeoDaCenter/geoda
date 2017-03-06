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

#include "../GenUtils.h"
#include "../logger.h"
#include "TableInterface.h"
#include "../DbfFile.h"

TableInterface::TableInterface(TableState* table_state_s,
							   TimeState* time_state_s)
: table_state(table_state_s), time_state(time_state_s),
encoding_type(wxFONTENCODING_SYSTEM), m_wx_encoding(0),
cols_case_sensitive(false), cols_max_length(10),
cols_ascii_only(true), is_valid(false)
{
}

TableInterface::~TableInterface()
{
	if (m_wx_encoding) delete m_wx_encoding;
}

bool TableInterface::IsValid()
{
	return is_valid;
}

wxString TableInterface::GetOpenErrorMessage()
{
	return open_err_msg;
}

int TableInterface::GetTimeInt(const wxString& tm_str)
{
	std::vector<wxString> tm_strs;
	GetTimeStrings(tm_strs);
	for (int t=0, sz=tm_strs.size(); t<sz; ++t) {
		if (tm_str == tm_strs[t]) return t;
	}
	return 0;
}

bool TableInterface::ChangedSinceLastSave()
{
	return changed_since_last_save;
}

void TableInterface::SetChangedSinceLastSave(bool chg)
{
	changed_since_last_save = chg;
}

bool TableInterface::ProjectChangedSinceLastSave()
{
    return project_changed_since_last_save;
}

void TableInterface::SetProjectChangedSinceLastSave(bool chg)
{
    project_changed_since_last_save = chg;
}

bool TableInterface::ColNameExists(const wxString& name)
{
	return (FindColId(name) != wxNOT_FOUND);
}

bool TableInterface::IsColTimeVariant(const wxString& name)
{
	if (!ColNameExists(name)) return false;
	return IsColTimeVariant(FindColId(name));
}

bool TableInterface::IsSetCellFromStringFail()
{
	return is_set_cell_from_string_fail;
}

wxString TableInterface::GetSetCellFromStringFailMsg()
{
	if (is_set_cell_from_string_fail) {
		return set_cell_from_string_fail_msg;
	} else {
		return wxEmptyString;
	}
}

void TableInterface::SetEncoding(wxFontEncoding enc_type)
{
	encoding_type = enc_type;
    if (m_wx_encoding) delete m_wx_encoding;
    m_wx_encoding = new wxCSConv(encoding_type);
	table_state->notifyObservers();
}

wxString TableInterface::SuggestGroupName(std::vector<wxString> cols) const
{
	using namespace std;
	wxString nm(GenUtils::FindLongestSubString(cols));
	// remove trailing and leading whitespace, underscores and numbers
	bool done=false;
	while (!done && !nm.IsEmpty()) {
		int len = nm.length();
		if (nm.length() > 0 &&
			(nm.find("_") == 0 || nm.find("0") == 0 || nm.find("1") == 0 ||
			 nm.find("2") == 0 || nm.find("3") == 0 || nm.find("4") == 0 ||
			 nm.find("5") == 0 || nm.find("6") == 0 || nm.find("7") == 0 ||
			 nm.find("8") == 0 || nm.find("9") == 0)) {
			nm = nm.substr(1, nm.length()-1);
		}
		int lp = nm.length()-1;
		if (nm.length() > 0 &&
			(nm.find("_") == lp || nm.find("0") == lp || nm.find("1") == lp ||
			 nm.find("2") == lp || nm.find("3") == lp || nm.find("4") == lp ||
			 nm.find("5") == lp || nm.find("6") == lp || nm.find("7") == lp ||
			 nm.find("8") == lp || nm.find("9") == lp)) {
			nm = nm.substr(lp, nm.length()-1);
		}
		nm.Trim(true);
		nm.Trim(false);
		if (nm.length() == len) done = true;
	}
	return GetUniqueGroupName(nm);
}

std::vector<wxString> TableInterface::SuggestDBColNames(wxString new_grp_name, wxString prefix, int n) const
{
	return GetUniqueColNames(prefix, n);
}

wxString TableInterface::GetUniqueGroupName(wxString grp_nm) const
{
	const int MAX_TRIES = 100000;
	if (grp_nm.IsEmpty()) grp_nm = "Group";
	wxString u(grp_nm);
	for (int i=0; i<MAX_TRIES; ++i) {
		if (!DoesNameExist(u, cols_case_sensitive)) return u;
		u = grp_nm;
		u << " " << i+1;
	}
	return u;
}

std::vector<wxString> TableInterface::GetUniqueColNames(wxString col_nm,
														int n) const
{
	using namespace std;
	vector<wxString> ret(n, col_nm);
	if (n==1 && !DoesNameExist(col_nm, cols_case_sensitive))
        return ret;
	
	const int MAX_TRIES = 100000;
	if (col_nm.IsEmpty()) col_nm = "VAR";
	if (col_nm.length() > cols_max_length) {
		col_nm = col_nm.substr(0, cols_max_length);
	}
	
	int ret_cnt = 0;
	for (int i=1; i<MAX_TRIES && ret_cnt < n; ++i) {
		int ilen = ((int) log((double)i)) + 1;
		int diff = (col_nm.length() + ilen + 1) - cols_max_length;
		if (diff > 0) {
			col_nm = col_nm.substr(0, col_nm.length()-diff);
		}
		wxString u = col_nm;
		u << "_" << i;
		if (!DoesNameExist(u, cols_case_sensitive))
            ret[ret_cnt++] = u;
	}
	return ret;
}

bool TableInterface::IsValidGroupName(const wxString& grp_nm) const
{
	// Almost no restrictions on group names since this info is stored
	// in project file.
	return true;
}

std::vector<wxString> TableInterface::GetGroupNames()
{
    std::vector<wxString> grp_names;
    
    int n_col = GetNumberCols();
    for (int i=0; i<n_col; i++) {
        if (IsColTimeVariant(i)) {
            grp_names.push_back(GetColName(i));
        }
    }
    
    return grp_names;
}


int TableInterface::GetColIdx(const wxString& name, bool ignore_case)
{
    return -1;
}


void TableInterface::SetColData(int col, int time,
                                const std::vector<double>& data,
                                const std::vector<bool>& undefs)
{
    SetColData(col, time, data);
    SetColUndefined(col, time, undefs);
}

void TableInterface::SetColData(int col, int time,
                                const std::vector<wxInt64>& data,
                                const std::vector<bool>& undefs)
{
    SetColData(col, time, data);
    SetColUndefined(col, time, undefs);
}

void TableInterface::SetColData(int col, int time,
                                const std::vector<wxString>& data,
                                const std::vector<bool>& undefs)
{
    SetColData(col, time, data);
    SetColUndefined(col, time, undefs);
}

void TableInterface::GetColData(int col, int time, std::vector<double>& data,
                                std::vector<bool>& undefs)
{
    GetColData(col, time, data);
    GetColUndefined(col, time, undefs);
}

void TableInterface::GetColData(int col, int time, std::vector<wxInt64>& data,
                                std::vector<bool>& undefs)
{
    GetColData(col, time, data);
    GetColUndefined(col, time, undefs);
}

void TableInterface::GetColData(int col, int time, std::vector<wxString>& data,
                                std::vector<bool>& undefs)
{
    GetColData(col, time, data);
    GetColUndefined(col, time, undefs);
}
