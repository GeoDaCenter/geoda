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
#include <boost/date_time.hpp>
#include <wx/fontmap.h>
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "TableInterface.h"

namespace bt = boost::posix_time;


TableInterface::TableInterface(TableState* table_state_s,
							   TimeState* time_state_s)
: table_state(table_state_s), time_state(time_state_s),
encoding_type(wxFONTENCODING_SYSTEM), m_wx_encoding(0),
cols_case_sensitive(true), cols_max_length(10),
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

bool TableInterface::IsColTimeVariant(const wxString& name)
{
	if (!DoesNameExist(name, cols_case_sensitive)) return false;
    
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

std::vector<wxString> TableInterface::SuggestDBColNames(wxString new_grp_name,
                                                        wxString prefix, int n) const
{
	return GetUniqueColNames(prefix, n);
}

wxString TableInterface::GetUniqueGroupName(wxString grp_nm) const
{
	const int MAX_TRIES = 100000;
    if (grp_nm.IsEmpty()) {
        grp_nm = "Group";
    }
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
    std::vector<wxString> ret(n, col_nm);
	if (n==1 && !DoesNameExist(col_nm, cols_case_sensitive)) return ret;
	
	const int MAX_TRIES = 100000;
    if (col_nm.IsEmpty()) {
        col_nm = "VAR";
    }
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

void TableInterface::SetColData(int col, int time,
                                const std::vector<unsigned long long>& data,
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

void TableInterface::GetColData(int col, int time, std::vector<unsigned long long>& data,
                                std::vector<bool>& undefs)
{
    GetColData(col, time, data);
    GetColUndefined(col, time, undefs);
}

bool TableInterface::CheckID(const wxString& id)
{
    std::vector<wxString> str_id_vec(GetNumberRows());
    int col = FindColId(id);
    if (GetColType(col) == GdaConst::long64_type) {
        GetColData(col, 0, str_id_vec);

    } else if (GetColType(col) == GdaConst::string_type) {
        // to handle string field with only numbers
        // Note: can't handle real string (a-zA-Z) here since it's hard
        // to control in weights file (.gal/.gwt/..)
        GetColData(col, 0, str_id_vec);

        wxRegEx regex;
        regex.Compile("^[0-9a-zA-Z_]+$");

        for (size_t i=0, iend=str_id_vec.size(); i<iend; i++) {
            wxString item  = str_id_vec[i];
            if (regex.Matches(item)) {
                str_id_vec[i] = item;
            } else {
                wxString msg = id;
                msg += _(" should contains only numbers/letters as IDs.  Please choose a different ID Variable.");
                wxMessageBox(msg);
                return false;
            }
        }
    }

    std::set<wxString> dup_ids;
    std::set<wxString> id_set;
    std::map<wxString, std::vector<int> > dup_dict; // value:[]

    for (size_t i=0, iend=str_id_vec.size(); i<iend; i++) {
        wxString str_id = str_id_vec[i];
        if (id_set.find(str_id) == id_set.end()) {
            id_set.insert(str_id);
            std::vector<int> ids;
            dup_dict[str_id] = ids;
        }
        dup_dict[str_id].push_back((int)i);
    }
    if (id_set.size() != GetNumberRows()) {
        wxString msg = id + _(" has duplicate values. Please choose a different ID Variable.\n\nDetails:");
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

        ScrolledDetailMsgDialog *dlg = new ScrolledDetailMsgDialog(_("Warning"), msg, details);
        dlg->Show(true);

        return false;
    }
    return true;
}

wxString TableInterface::GetEncodingName()
{
    if (encoding_type == wxFONTENCODING_UTF8) {
        return "UTF-8";
    } else if (encoding_type == wxFONTENCODING_UTF16LE) {
        return "UTF-16";
    } else if (encoding_type == wxFONTENCODING_UTF16LE) {
        return "UTF-16";
    } else if (encoding_type == wxFONTENCODING_CP1250) {
        return "CP1250";
    } else if (encoding_type == wxFONTENCODING_CP1251) {
        return "CP1251";
    } else if (encoding_type == wxFONTENCODING_CP1252) {
        return "CP1252";
    } else if (encoding_type == wxFONTENCODING_CP1253) {
        return "CP1253";
    } else if (encoding_type == wxFONTENCODING_CP1254) {
        return "CP1254";
    } else if (encoding_type == wxFONTENCODING_CP1255) {
        return "CP1255";
    } else if (encoding_type == wxFONTENCODING_CP1256) {
        return "CP1256";
    } else if (encoding_type == wxFONTENCODING_CP1257) {
        return "CP1257";
    } else if (encoding_type == wxFONTENCODING_CP1258) {
        return "CP1258";
    } else if (encoding_type == wxFONTENCODING_CP437) {
        return "CP437";
    } else if (encoding_type == wxFONTENCODING_CP850) {
        return "CP850";
    } else if (encoding_type == wxFONTENCODING_CP855) {
        return "CP855";
    } else if (encoding_type == wxFONTENCODING_CP866) {
        return "CP866";
    } else if (encoding_type == wxFONTENCODING_CP874) {
        return "CP874";
    } else if (encoding_type == wxFONTENCODING_CP932) {
        return "CP932";
    } else if (encoding_type == wxFONTENCODING_CP936) {
        return "CP936";
    } else if (encoding_type == wxFONTENCODING_CP949) {
        return "CP949";
    } else if (encoding_type == wxFONTENCODING_CP950) {
        return "CP950";
    } else if (encoding_type == wxFONTENCODING_ISO8859_10) {
        return "ISO8859_10";
    } else if (encoding_type == wxFONTENCODING_ISO8859_11) {
        return "ISO8859_11";
    } else if (encoding_type == wxFONTENCODING_ISO8859_12) {
        return "ISO8859_12";
    } else if (encoding_type == wxFONTENCODING_ISO8859_13) {
        return "ISO8859_13";
    } else if (encoding_type == wxFONTENCODING_ISO8859_14) {
        return "ISO8859_14";
    } else if (encoding_type == wxFONTENCODING_ISO8859_15) {
        return "ISO8859_15";
    } else if (encoding_type == wxFONTENCODING_ISO8859_1) {
        return "ISO8859_1";
    } else if (encoding_type == wxFONTENCODING_ISO8859_2) {
        return "ISO8859_2";
    } else if (encoding_type == wxFONTENCODING_ISO8859_3) {
        return "ISO8859_3";
    } else if (encoding_type == wxFONTENCODING_ISO8859_4) {
        return "ISO8859_4";
    } else if (encoding_type == wxFONTENCODING_ISO8859_5) {
        return "ISO8859_5";
    } else if (encoding_type == wxFONTENCODING_ISO8859_6) {
        return "ISO8859_6";
    } else if (encoding_type == wxFONTENCODING_ISO8859_7) {
        return "ISO8859_7";
    } else if (encoding_type == wxFONTENCODING_ISO8859_8) {
        return "ISO8859_8";
    } else if (encoding_type == wxFONTENCODING_ISO8859_9) {
        return "ISO8859_9";
    } else if (encoding_type == wxFONTENCODING_GB2312) {
        return "GB2312";
    } else if (encoding_type == wxFONTENCODING_BIG5) {
        return "BIG5";
    } else if (encoding_type == wxFONTENCODING_KOI8) {
        return "KOI8";
    } else if (encoding_type == wxFONTENCODING_SHIFT_JIS) {
        return "SHIFT_JIS";
    } else if (encoding_type == wxFONTENCODING_EUC_JP) {
        return "JP";
    } else if (encoding_type == wxFONTENCODING_EUC_KR) {
        return "KR";
    }
    return wxEmptyString;
}
