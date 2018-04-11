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

#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm> // for vector sorting
#include <set>
#include <boost/foreach.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <wx/colour.h>
#include <wx/msgdlg.h>
#include "DbfColContainer.h"
#include "../HighlightState.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../ShapeOperations/CsvFileUtils.h"
#include "TableState.h"
#include "DbfColContainer.h"


DbfColContainer::DbfColContainer()
: size(0), undefined_initialized(false), raw_data(0),
l_vec(0), d_vec(0), s_vec(0)
{
	info.type = GdaConst::unknown_type;
}

DbfColContainer::~DbfColContainer()
{
	if (IsRawDataAlloc()) FreeRawData();
	FreeVecData();
}

bool DbfColContainer::Init(int size_s,
						   const GdaConst::FieldInfo& field_info_s,
						   bool alloc_raw_data,
						   bool alloc_vector_data,
						   bool mark_all_defined)
{
	if (size_s <= 0) return false;
	if (field_info_s.type == GdaConst::unknown_type ||
		field_info_s.type == GdaConst::placeholder_type) return false;
	
	info = field_info_s;
	size = size_s;
	
	stale_min_max_val = true;
	min_val = 0;
	max_val = 0;
	
	// if mark_all_defined is true, then mark all as begin defined.
	undefined.resize(size_s);
	std::fill(undefined.begin(), undefined.end(), !mark_all_defined);
	undefined_initialized = mark_all_defined;
	
	if (alloc_raw_data) AllocRawData();	
	if (alloc_vector_data) AllocVecData();

	return true;
}

void DbfColContainer::FreeRawData()
{
	if (raw_data) delete [] raw_data; raw_data = 0;
}

void DbfColContainer::AllocRawData()
{
	if (IsRawDataAlloc()) FreeRawData();
	if (info.type != GdaConst::unknown_type &&
		info.type != GdaConst::placeholder_type) {
		raw_data = new char[size * (info.field_len+1)];
	} else {
		raw_data = 0;
	}
}

bool DbfColContainer::IsRawDataAlloc()
{
	return (info.type != GdaConst::unknown_type &&
			info.type != GdaConst::placeholder_type &&
			raw_data);
}

void DbfColContainer::FreeVecData()
{
	if (l_vec) delete [] l_vec; l_vec = 0;
	if (d_vec) delete [] d_vec; d_vec = 0;
	if (s_vec) delete [] s_vec; s_vec = 0;
}

void DbfColContainer::AllocVecData()
{
	if (IsVecDataAlloc()) return;
	if (GetType() == GdaConst::long64_type ||
		GetType() == GdaConst::date_type) {
		l_vec = new wxInt64[size]();
	} else if (GetType() == GdaConst::double_type) {
		d_vec = new double[size]();
	} else if (GetType() == GdaConst::string_type) {
		s_vec = new wxString[size];
	}
}

bool DbfColContainer::IsVecDataAlloc()
{
	if (GetType() == GdaConst::long64_type) {
		return l_vec != 0;
	} else if (GetType() == GdaConst::date_type) {
		return l_vec != 0;
	} else if (GetType() == GdaConst::double_type) {
		return d_vec != 0;
	} else if (GetType() == GdaConst::string_type) {
		return s_vec != 0;
	}
	return false;
}


wxString DbfColContainer::GetName()
{
	return info.name;
}

wxString DbfColContainer::GetDbfColName()
{
	return info.name;
}

GdaConst::FieldType DbfColContainer::GetType()
{
	return info.type;
}

int DbfColContainer::GetFieldLen()
{
	return info.field_len;
}

int DbfColContainer::GetDecimals()
{
	return info.decimals;
}

void DbfColContainer::GetMinMaxVals(double& min_v, double& max_v)
{
	UpdateMinMaxVals();
	min_v = min_val;
	max_v = max_val;
}

void DbfColContainer::UpdateMinMaxVals()
{
	if (GetType() != GdaConst::double_type &&
		GetType() != GdaConst::long64_type) return;
	if (stale_min_max_val) {
		std::vector<double> vals;
		GetVec(vals);
		min_val = vals[0];
		max_val = vals[0];
		for (int i=0; i<size; i++) {
			if (vals[i] < min_val) {
				min_val = vals[i];
			} else if (vals[i] > max_val) {
				max_val = vals[i];
			}
		}
		stale_min_max_val = false;
	}
}

// Change Properties will convert data to vector format if length or
// decimals are changed.
bool DbfColContainer::ChangeProperties(int new_len, int new_dec)
{
	if (!IsRawDataAlloc() && !IsVecDataAlloc()) {
		// this violates the assumption that at least either raw_data
		// exists or a valid vector exists.  This should never happen
		return false;
	}
	
	if (GetType() == GdaConst::string_type) {
		if (new_len < GdaConst::min_dbf_string_len ||
			new_len > GdaConst::max_dbf_string_len) {
			return false;
		}
		if (IsRawDataAlloc() && !IsVecDataAlloc()) {
			AllocVecData();
			raw_data_to_vec(s_vec);
		}
		// shorten all strings as needed.
		if (new_len < info.field_len) {
			for (int i=0; i<size; i++) {
				if (new_len < s_vec[i].length()) {
					s_vec[i] = s_vec[i].SubString(0, new_len-1);
				}
			}
		}
	} else if (GetType() == GdaConst::long64_type) {
		if (new_len < GdaConst::min_dbf_long_len ||
			new_len > GdaConst::max_dbf_long_len) {
			return false;
		}
		if (IsRawDataAlloc() && !IsVecDataAlloc()) {
			AllocVecData();
			raw_data_to_vec(l_vec);
		}
		// limit all vector values to acceptable range
		//wxInt64 max_val = DbfFileUtils::GetMaxInt(new_len);
		//wxInt64 min_val = DbfFileUtils::GetMinInt(new_len);
		//for (int i=0; i<size; i++) {
		//	if (max_val < l_vec[i]) {
		//		l_vec[i] = max_val;
		//	} else if (min_val > l_vec[i]) {
		//		l_vec[i] = min_val;
		//	}
		//}
	} else if (GetType() == GdaConst::double_type) {
		if (new_len < GdaConst::min_dbf_double_len ||
			new_len > GdaConst::max_dbf_double_len ||
			new_dec < GdaConst::min_dbf_double_decimals ||
			new_dec > GdaConst::max_dbf_double_decimals) {
			return false;
		}
		int suggest_len;
		int suggest_dec;
		DbfFileUtils::SuggestDoubleParams(new_len, new_dec,
										  &suggest_len, &suggest_dec);
		if (new_len != suggest_len || new_dec != suggest_dec) {
			return false;
		}
		if (IsRawDataAlloc() && !IsVecDataAlloc()) {
			AllocVecData();
			raw_data_to_vec(d_vec);
		}
		// limit all vectors to acceptable range
		//double max_val = DbfFileUtils::GetMaxDouble(new_len, new_dec);
		//double min_val = DbfFileUtils::GetMinDouble(new_len, new_dec);
		//for (int i=0, iend=size; i != iend; i++) {
		//	if (max_val < d_vec[i]) {
		//		d_vec[i] = max_val;
		//	} else if (min_val > d_vec[i]) {
		//		d_vec[i] = min_val;
		//	}
		//}
		info.decimals = new_dec;
	} else { // GdaConst::date_type
		// can only change field name for date_type
		if (new_len != GdaConst::max_dbf_date_len) return false;
	}
	
	if (IsRawDataAlloc()) FreeRawData();
	info.field_len = new_len;
	return true;
}


bool DbfColContainer::ChangeName(const wxString& new_name)
{
	if (!DbfFileUtils::isValidFieldName(new_name)) return false;
	info.name = new_name;
	return true;
}

bool DbfColContainer::sprintf_period_for_decimal()
{
	char buf[10];
	sprintf(buf, "%#3.1f", 2.5);
	//LOG_MSG(wxString::Format("DbfColContainer::sprintf_period_for_decimal()"
	//						 " = %s", buf[1] == '.' ? "true" : "false"));
	return buf[1] == '.';
}

// Allow for filling of double from long64 field
void DbfColContainer::GetVec(std::vector<double>& vec)
{
	if (GetType() != GdaConst::double_type &&
		GetType() != GdaConst::long64_type) return;
	if (!IsVecDataAlloc() && IsRawDataAlloc()) CopyRawDataToVector();
	if (vec.size() != size) vec.resize(size);
	if (GetType() == GdaConst::double_type) {
		if (IsVecDataAlloc()) {
			for (int i=0; i<size; i++) vec[i] = d_vec[i];
		} else {
			raw_data_to_vec(vec);
		}
	} else {
		if (IsVecDataAlloc()) {
			for (int i=0; i<size; i++) vec[i] = (double) l_vec[i];
		} else {
			std::vector<wxInt64> t(size);
			raw_data_to_vec(t);
			for (int i=0; i<size; i++) vec[i] = (double) t[i];
		}
	}
}

// Allow for filling of long64 from double field
void DbfColContainer::GetVec(std::vector<wxInt64>& vec)
{
	if (GetType() != GdaConst::double_type &&
		GetType() != GdaConst::long64_type &&
        GetType() != GdaConst::date_type ) return;
	if (!IsVecDataAlloc() && IsRawDataAlloc()) CopyRawDataToVector();
	if (vec.size() != size) vec.resize(size);
	if (GetType() == GdaConst::long64_type ||
        GetType() == GdaConst::date_type ) {
		if (IsVecDataAlloc()) {
			for (int i=0; i<size; i++) vec[i] = l_vec[i];
		} else {
			raw_data_to_vec(vec);
		}
	} else {
		if (IsVecDataAlloc()) {
			for (int i=0; i<size; i++) vec[i] = (wxInt64) d_vec[i];
		} else {
			std::vector<double> t(size);
			raw_data_to_vec(t);
			for (int i=0; i<size; i++) vec[i] = (wxInt64) t[i];
		}		
	}
}

void DbfColContainer::GetVec(std::vector<wxString>& vec)
{
	if (vec.size() != size) vec.resize(size);
	if (IsVecDataAlloc()) {
		for (int i=0; i<size; i++) vec[i] = s_vec[i];
	} else {
		raw_data_to_vec(vec);
	}
}

void DbfColContainer::SetFromVec(const std::vector<double>& vec)
{
	if (vec.size() != size) return;
	if (GetType() != GdaConst::long64_type &&
		GetType() != GdaConst::double_type) return;
	if (!IsVecDataAlloc()) AllocVecData();
	if (IsRawDataAlloc()) FreeRawData();
 	
	CheckUndefined();
	for (int i=0; i<size; i++) {
		undefined[i] = !boost::math::isfinite<double>(vec[i]);
	}
	undefined_initialized = true;
	
	if (GetType() == GdaConst::long64_type) {
		for (int i=0; i<size; i++) {
			l_vec[i] = undefined[i] ? 0 : (wxInt64) vec[i];
		}
	} else { // must be double_type
		for (int i=0; i<size; i++) {
			d_vec[i] = undefined[i] ? 0 : vec[i];
		}
	}
	stale_min_max_val = true;
	UpdateMinMaxVals();
}

void DbfColContainer::SetFromVec(const std::vector<wxInt64>& vec)
{
	if (vec.size() != size) return;
	if (GetType() != GdaConst::long64_type &&
		GetType() != GdaConst::double_type) return;
	if (!IsVecDataAlloc()) AllocVecData();
	if (IsRawDataAlloc()) FreeRawData();
	
	CheckUndefined();
	for (int i=0; i<size; i++) undefined[i] = false;
	undefined_initialized = true;
	if (GetType() == GdaConst::long64_type) {
		for (int i=0; i<size; i++) l_vec[i] = vec[i];
	} else { // must be double_type
		for (int i=0; i<size; i++) d_vec[i] = (double) vec[i];
	}
	stale_min_max_val = true;
	UpdateMinMaxVals();
}

void DbfColContainer::SetFromVec(const std::vector<wxString>& vec)
{
	if (vec.size() != size) return;
	if (!IsVecDataAlloc()) AllocVecData();
	if (IsRawDataAlloc()) FreeRawData();
	
	for (int i=0; i<size; i++) undefined[i] = false;
	undefined_initialized = true;
	for (int i=0; i<size; i++) s_vec[i] = vec[i];
	stale_min_max_val = false;
}

void DbfColContainer::SetUndefined(const std::vector<bool>& undef_vec)
{
	if (undefined.size() != size) undefined.resize(size);
	CheckUndefined();
	for (int i=0; i<size; i++) undefined[i] = undef_vec[i];
}

void DbfColContainer::GetUndefined(std::vector<bool>& undef_vec)
{
	if (undef_vec.size() != size) undef_vec.resize(size);
	if (!undefined_initialized) CheckUndefined();
	for (int i=0; i<size; i++) undef_vec[i] = undefined[i];
}

void DbfColContainer::CheckUndefined()
{
	if (undefined_initialized) return;
	undefined_initialized = true;
	if (undefined.size() != size) undefined.resize(size);
	
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	const int inc = info.field_len+1;
	for (int i=0; i<size; i++) undefined[i] = false;
	if (GetType() == GdaConst::double_type) {
		if (IsRawDataAlloc()) {
			char* buf=raw_data;
			for (int i=0; i<size; i++) {
				// we are not using atof since we it seems to be difficult
				// to choose a US locale on all systems so as to assume the
				// DBF-required use of '.' for the decimal character
				wxString temp(buf);
				temp.Trim(true);
				temp.Trim(false);
				double x;
				bool r = temp.ToCDouble(&x);
				undefined[i] = !(r && boost::math::isfinite<double>(x));
				buf += inc;
			}	
		} else {
			for (int i=0; i<size; i++) {
				undefined[i]=!boost::math::isfinite<double>(d_vec[i]);
			}
		}
	} else if (GetType() == GdaConst::long64_type) {
		if (IsRawDataAlloc()) {
			char* buf=raw_data;
			for (int i=0; i<size; i++) {
				undefined[i] = !GenUtils::validInt(buf);
				buf += inc;
			}
		} else {
			// all integers are valid, there is no way to
			// represent infinity or NaN as in float and double
		}
	}
}

void DbfColContainer::raw_data_to_vec(std::vector<double>& vec)
{
	if (vec.size() != size) {
		vec.resize(size);
		std::fill(vec.begin(), vec.end(), 0);
	}
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	CheckUndefined();
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		// we are not using atof since we it seems to be difficult
		// to choose a US locale on all systems so as to assume the
		// DBF-required use of '.' for the decimal character
		wxString temp(buf);
		temp.Trim(true);
		temp.Trim(false);
		bool r = temp.ToCDouble(&vec[i]);
		undefined[i] = !(r && boost::math::isfinite<double>(vec[i]));
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(double* vec)
{
	if (!vec) return;
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	CheckUndefined();
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		// we are not using atof since we it seems to be difficult
		// to choose a US locale on all systems so as to assume the
		// DBF-required use of '.' for the decimal character
		wxString temp(buf);
		temp.Trim(true);
		temp.Trim(false);
		vec[i] = 0;
		bool r = temp.ToCDouble(&vec[i]);
		undefined[i] = !(r && boost::math::isfinite<double>(vec[i]));
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(std::vector<wxInt64>& vec)
{
	if (vec.size() != size) {
		vec.resize(size);
		std::fill(vec.begin(), vec.end(), 0);
	}
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	CheckUndefined();
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		undefined[i] = !GenUtils::validInt(buf);
		GenUtils::strToInt64(buf, &vec[i]);  // will set to 0 if undefined
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(wxInt64* vec)
{
	if (!vec) return;
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	CheckUndefined();
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		undefined[i] = !GenUtils::validInt(buf);
		vec[i] = 0;
		GenUtils::strToInt64(buf, &vec[i]);  // will set to 0 if undefined
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(std::vector<wxString>& vec)
{
	if (vec.size() != size) vec.resize(size);
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	CheckUndefined();
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		vec[i] = wxString(buf);
		undefined[i] = false;
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(wxString* vec)
{
	if (!vec) return;
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	CheckUndefined();
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		vec[i] = wxString(buf);
		undefined[i] = false;
		buf += inc;
	}
}

void DbfColContainer::d_vec_to_raw_data()
{
	char temp[255];
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		if (!boost::math::isfinite<double>(d_vec[i])) {
			// the number is either NaN (not a number) or +/- infinity
			undefined[i] = true;
		}
		if (undefined[i]) {
			for (int j=0; j<info.field_len; j++) buf[j] = ' ';
		} else {
			sprintf(temp, "%#*.*f", info.field_len,
					info.decimals, d_vec[i]);
			for (int j=0; j<info.field_len; j++) buf[j] = temp[j];
		}
		buf[info.field_len] = '\0';
		buf += inc;
	}
	if (!sprintf_period_for_decimal()) {
		for (int i=0, iend=size*inc; i<iend; i++) {
			if (raw_data[i] == ',') raw_data[i] = '.';
		}
	}
}

void DbfColContainer::l_vec_to_raw_data()
{
	char temp[255];
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		if (undefined[i]) {
			for (int j=0; j<info.field_len; j++) buf[j] = ' ';
		} else {
			sprintf(temp, "%*lld", info.field_len, l_vec[i]);
			for (int j=0; j<info.field_len; j++) buf[j] = temp[j];
		}
		buf[info.field_len] = '\0';
		buf += inc;
	}
}

void DbfColContainer::s_vec_to_raw_data()
{
	char temp[255];
	if (GetType() == GdaConst::unknown_type ||
		GetType() == GdaConst::placeholder_type) return;
	const int inc = info.field_len+1;
	char* buf=raw_data;
	for (int i=0; i<size; i++) {
		if (undefined[i] || s_vec[i].IsEmpty()) {
			for (int j=0; j<info.field_len; j++) buf[j] = ' ';
		} else {
			sprintf(temp, "%*s", info.field_len,
					(const_cast<char*>((const char*)s_vec[i].mb_str())));
			for (int j=0; j<info.field_len; j++) buf[j] = temp[j];
		}
		buf[info.field_len] = '\0';
		buf += inc;
	}
}

void DbfColContainer::CopyRawDataToVector()
{
	if (!IsRawDataAlloc()) return;
	if (!IsVecDataAlloc()) AllocVecData();
	switch (GetType()) {
		case GdaConst::date_type:
			raw_data_to_vec(l_vec);
			break;
		case GdaConst::long64_type:
			raw_data_to_vec(l_vec);
			break;
		case GdaConst::double_type:
			raw_data_to_vec(d_vec);
			break;
		case GdaConst::string_type:
			raw_data_to_vec(s_vec);
			break;
		default:
			break;
	}
}


void DbfColContainer::CopyVectorToRawData()
{
	if (!IsVecDataAlloc()) return;
	if (!IsRawDataAlloc()) AllocRawData();
	switch (GetType()) {
		case GdaConst::date_type:
		{
			l_vec_to_raw_data();
		}
			break;
		case GdaConst::long64_type:
		{
			l_vec_to_raw_data();
		}
			break;
		case GdaConst::double_type:
		{
			d_vec_to_raw_data();
		}
			break;
		case GdaConst::string_type:
		{
			s_vec_to_raw_data();
		}
			break;
		default:
			break;
	}
}
