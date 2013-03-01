/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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
#include "../Generic/HighlightState.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../ShapeOperations/CsvFileUtils.h"
#include "TableState.h"
#include "DbfGridTableBase.h"

DbfColContainer::DbfColContainer(DbfGridTableBase* grid_base_s)
: size(0), type(GeoDaConst::unknown_type),
field_len(0), decimals(0),
d_vec(boost::extents[0][0]), l_vec(boost::extents[0][0]),
s_vec(boost::extents[0][0]), undefined(boost::extents[0][0]),
undefined_initialized(false),
vector_valid(false), raw_data(0), grid_base(grid_base_s)
{
}

DbfColContainer::DbfColContainer(DbfFileReader& dbf, int field,
								 DbfGridTableBase* grid_base_s)
: size(0), time_steps(1), type(GeoDaConst::unknown_type),
field_len(0), decimals(0),
d_vec(boost::extents[0][0]), l_vec(boost::extents[0][0]),
s_vec(boost::extents[0][0]), undefined(boost::extents[0][0]),
undefined_initialized(false),
stale_min_max_val(0), min_val(0), max_val(0),
vector_valid(false), raw_data(0), grid_base(grid_base_s)
{
	int rows = dbf.getNumRecords();
	if (field < 0 || field >= dbf.getNumFields()) return;
	std::vector<DbfFieldDesc> fields = dbf.getFieldDescs();
	
	DbfFieldDesc desc = dbf.getFieldDesc(field);
	DbfFileHeader header = dbf.getFileHeader();
	GeoDaConst::FieldType type;
	
	if (desc.type == 'N' || desc.type == 'F') {
		if (desc.decimals > 0) {
			type = GeoDaConst::double_type;
			Init(rows, time_steps, type, desc.name, desc.length, desc.decimals,
				 desc.decimals, true, false, false);
		} else {
			type = GeoDaConst::long64_type;
			Init(rows, time_steps, type, desc.name, desc.length, desc.decimals,
				 desc.decimals, true, false, false);
		}
	} else if (desc.type == 'D') {
		if (desc.length != 8) {
			LOG_MSG("Error: Date field found with incorrect length!"
					" We recomend fixing this in your DBF before "
					"proceeding.");
		}
		type = GeoDaConst::date_type;
		Init(rows, time_steps, type, desc.name, desc.length, 0, 0,
			 true, false, false);
	} else {
		// We will assume (desc.type == 'C')
		type = GeoDaConst::string_type;
		Init(rows, time_steps, type, desc.name, desc.length,
			 desc.decimals, desc.decimals,
			 true, false, false);
	}
	if (!dbf.file.is_open()) {
		dbf.file.open(dbf.fname.mb_str(wxConvUTF8),
					  std::ios::in | std::ios::binary);
	}
	if (!(dbf.file.is_open() && dbf.file.good())) return;
	
	// calculate field offset
	int record_offset = 1; // the record deletion flag
	for (int i=0; i<field; i++) {
        record_offset += fields[i].length;
    }
    int field_length = fields[field].length;
		
    dbf.file.seekg(header.header_length + record_offset, std::ios::beg);
	
    for (int i=0; i< (int) header.num_records; i++) {
        dbf.file.read((char*)(raw_data[0] + i*(field_length+1)),
					  field_length);
		raw_data[0][i*(field_length+1)+field_length] = '\0';
        // seek to next record in file
        dbf.file.seekg(header.length_each_record-field_length, std::ios::cur);
	}
}

DbfColContainer::~DbfColContainer()
{
	if (IsRawDataAlloc()) FreeRawData();
}

bool DbfColContainer::Init(int size_s, int time_steps_s,
						   GeoDaConst::FieldType type_s,
						   const wxString& name_s, int field_len_s,
						   int decimals_s,
						   int displayed_decimals_s,
						   bool alloc_raw_data,
						   bool alloc_vector_data,
						   bool mark_all_defined)
{
	if (type != GeoDaConst::unknown_type || type_s == GeoDaConst::unknown_type
		|| size_s <= 0) {
		return false; // can't change type once set
	}
	
	name = name_s;
	size = size_s;
	time_steps = time_steps_s;
	type = type_s;
	field_len = field_len_s;
	decimals = decimals_s;
	displayed_decimals = displayed_decimals_s;
	stale_min_max_val.resize(time_steps);
	min_val.resize(time_steps);
	max_val.resize(time_steps);
	for (int t=0; t<time_steps; t++) {
		stale_min_max_val[t] = true;
		min_val[t] = 0;
		max_val[t] = 0;
	}
	
	// if mark_all_defined is true, then mark all as begin defined.
	undefined.resize(boost::extents[time_steps][size]);
	std::fill(undefined.origin(), undefined.origin()+undefined.num_elements(),
			  !mark_all_defined);
	undefined_initialized = mark_all_defined;
	
	if (alloc_raw_data) AllocRawData();
	
	if (alloc_vector_data) {
		vector_valid = true;
		switch (type) {
			case GeoDaConst::date_type:
				l_vec.resize(boost::extents[time_steps][size]);
				std::fill(l_vec.origin(), l_vec.origin()+l_vec.num_elements(),
						  0);
				return true;
				break;
			case GeoDaConst::long64_type:
				l_vec.resize(boost::extents[time_steps][size]);
				std::fill(l_vec.origin(), l_vec.origin()+l_vec.num_elements(),
						  0);
				return true;
				break;
			case GeoDaConst::double_type:
				d_vec.resize(boost::extents[time_steps][size]);
				std::fill(d_vec.origin(), d_vec.origin()+d_vec.num_elements(),
						  0);
				return true;
				break;
			case GeoDaConst::string_type:
				s_vec.resize(boost::extents[time_steps][size]);
				return true;
				break;
			default:
				break;
		}
		vector_valid = false;
		return false;
	}
	return true;
}

void DbfColContainer::FreeRawData()
{
	for (int i=0; i<time_steps; i++) {
		if (raw_data[i]) {
			delete [] raw_data[i];
			raw_data[i] = 0;
		}
	}
	raw_data.clear();
}

void DbfColContainer::AllocRawData()
{
	if (IsRawDataAlloc()) FreeRawData();
	raw_data.resize(time_steps);
	for (int i=0; i<time_steps; i++) {
		raw_data[i] = new char[size * (field_len+1)];
	}
}

bool DbfColContainer::IsRawDataAlloc()
{
	return (raw_data.size() == time_steps && raw_data[0]);
}


void DbfColContainer::GetMinMaxVals(std::vector<double>& min_vals,
									std::vector<double>& max_vals)
{
	UpdateMinMaxVals();
	if (min_vals.size() != time_steps) min_vals.resize(time_steps);
	if (max_vals.size() != time_steps) max_vals.resize(time_steps);
	for (int t=0; t<time_steps; t++) {
		min_vals[t] = min_val[t];
		max_vals[t] = max_val[t];
	}
}

void DbfColContainer::UpdateMinMaxVals()
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	for (int t=0; t<time_steps; t++) {
		if (stale_min_max_val[t]) {
			std::vector<double> vals;
			GetVec(vals, t);
			min_val[t] = vals[0];
			max_val[t] = vals[0];
			for (int i=0; i<size; i++) {
				if (vals[i] < min_val[t]) {
					min_val[t] = vals[i];
				} else if (vals[i] > max_val[t]) {
					max_val[t] = vals[i];
				}
			}
		}
	}
}

// Change Properties will convert data to vector format if length or
// decimals are changed.
bool DbfColContainer::ChangeProperties(const wxString& new_name, int new_len,
									   int new_dec, int new_disp_dec)
{
	if (!IsRawDataAlloc() && !vector_valid) {
		// this violates the assumption that at least either raw_data
		// exists or a valid vector exists.  This should never happen
		return false;
	}
	
	if (!DbfFileUtils::isValidFieldName(new_name)) return false;
	if (type == GeoDaConst::string_type) {
		if (new_len < GeoDaConst::min_dbf_string_len ||
			new_len > GeoDaConst::max_dbf_string_len) {
			return false;
		}
		if (IsRawDataAlloc() && !vector_valid) {
			s_vec.resize(boost::extents[time_steps][size]);
			raw_data_to_vec(s_vec);
		}
		// shorten all strings as needed.
		if (new_len < field_len) {
			int t=0;
			for (int i=0; i<size; i++) {
				if (new_len < s_vec[t][i].length()) {
					s_vec[t][i] = s_vec[t][i].SubString(0, new_len-1);
				}
			}
		}
		name = new_name;
	} else if (type == GeoDaConst::long64_type) {
		if (new_len < GeoDaConst::min_dbf_long_len ||
			new_len > GeoDaConst::max_dbf_long_len) {
			return false;
		}
		if (IsRawDataAlloc() && !vector_valid) {
			l_vec.resize(boost::extents[time_steps][size]);
			raw_data_to_vec(l_vec);
		}
		// limit all vector values to acceptable range
		wxInt64 max_val = DbfFileUtils::GetMaxInt(new_len);
		wxInt64 min_val = DbfFileUtils::GetMinInt(new_len);
		int t=0;
		for (int i=0; i<size; i++) {
			if (max_val < l_vec[t][i]) {
				l_vec[t][i] = max_val;
			} else if (min_val > l_vec[t][i]) {
				l_vec[t][i] = min_val;
			}
		}
		name = new_name;
	} else if (type == GeoDaConst::double_type) {
		if (new_disp_dec < 0 ||
			new_disp_dec > GeoDaConst::max_dbf_double_decimals ||
			new_len < GeoDaConst::min_dbf_double_len ||
			new_len > GeoDaConst::max_dbf_double_len ||
			new_dec < GeoDaConst::min_dbf_double_decimals ||
			new_dec > GeoDaConst::max_dbf_double_decimals) {
			return false;
		}
		int suggest_len;
		int suggest_dec;
		DbfFileUtils::SuggestDoubleParams(new_len, new_dec,
										  &suggest_len, &suggest_dec);
		if (new_len != suggest_len || new_dec != suggest_dec) {
			return false;
		}
		if (IsRawDataAlloc() && !vector_valid) {
			d_vec.resize(boost::extents[time_steps][size]);
			raw_data_to_vec(d_vec);
		}
		// limit all vectors to acceptable range
		double max_val = DbfFileUtils::GetMaxDouble(new_len, new_dec);
		double min_val = DbfFileUtils::GetMinDouble(new_len, new_dec);
		int t=0;
		for (int i=0, iend=size; i != iend; i++) {
			if (max_val < d_vec[t][i]) {
				d_vec[t][i] = max_val;
			} else if (min_val > d_vec[t][i]) {
				d_vec[t][i] = min_val;
			}
		}
		decimals = new_dec;
		displayed_decimals = new_disp_dec;
		name = new_name;
	} else { // GeoDaConst::date_type
		// can only change field name for date_type
		if (new_len != GeoDaConst::max_dbf_date_len) return false;
		name = new_name;
	}
	
	if (IsRawDataAlloc()) FreeRawData();
	vector_valid = true;
	field_len = new_len;
	grid_base->SetChangedSinceLastSave(true);
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

bool DbfColContainer::IsVecItemDefined(int i, int time)
{
	if (!vector_valid || (undefined_initialized && undefined[time][i])) {
		return false;
	}
	// can assume vector_valid and !undefined_initialized, so need to
	// determine if vector data is valid depending on the type
	if (type == GeoDaConst::string_type) return true;
	if (type == GeoDaConst::long64_type) return true;
	if (type == GeoDaConst::date_type) return (l_vec[time][i] != 0);
	// can assume that type is double_type
	return boost::math::isfinite<double>(d_vec[time][i]);
}

bool DbfColContainer::IsRawItemDefined(int i, int time)
{
	if (!IsRawDataAlloc() || (undefined_initialized && undefined[time][i])) {
		return false;
	}
	// need to determine if data is undefined based on raw string data
	const char* buf = (char*)(raw_data[time] + i*(field_len+1));
	if (type == GeoDaConst::string_type) return true;
	if (type == GeoDaConst::long64_type) return GenUtils::validInt(buf);
	// We need to come up with a better date check than the following
	if (type == GeoDaConst::date_type) return GenUtils::validInt(buf);
	// can assume that type is double_type
	if (GenUtils::isEmptyOrSpaces(buf)) return false;
	wxString temp(buf);
	temp.Trim(true);
	temp.Trim(false);
	double val;
	bool r = temp.ToCDouble(&val);
	return r && boost::math::isfinite<double>(val);
	
}

// Allow for filling of double from long64 field
void DbfColContainer::GetVec(std::vector<double>& vec, int time)
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	if (vec.size() != size) vec.resize(size);
	if (time < 0 || time >= time_steps) time = 0;
	if (type == GeoDaConst::double_type) {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = d_vec[time][i];
		} else {
			raw_data_to_vec(vec, time);
		}
	} else {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = (double) l_vec[time][i];
		} else {
			std::vector<wxInt64> t(size);
			raw_data_to_vec(t, time);
			for (int i=0; i<size; i++) vec[i] = (double) t[i];
		}
	}
}

// Allow for filling of long64 from double field
void DbfColContainer::GetVec(std::vector<wxInt64>& vec, int time)
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	if (vec.size() != size) vec.resize(size);
	if (time < 0 || time >= time_steps) time = 0;
	if (type == GeoDaConst::long64_type) {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = l_vec[time][i];
		} else {
			raw_data_to_vec(vec, time);
		}
	} else {
		if (vector_valid) {
			for (int i=0; i<size; i++) vec[i] = (wxInt64) d_vec[time][i];
		} else {
			std::vector<double> t(size);
			raw_data_to_vec(t, time);
			for (int i=0; i<size; i++) vec[i] = (wxInt64) t[i];
		}		
	}
}

void DbfColContainer::GetVec(std::vector<wxString>& vec, int time)
{
	if (vec.size() != size) vec.resize(size);
	if (time < 0 || time >= time_steps) time = 0;
	if (vector_valid) {
		for (int i=0; i<size; i++) vec[i] = s_vec[time][i];
	} else {
		raw_data_to_vec(vec, time);
	}
}

void DbfColContainer::GetVec(d_array_type& data)
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	if (data.shape()[0] != time_steps || data.shape()[1] != size) {
		data.resize(boost::extents[time_steps][size]);
	}
	if (type == GeoDaConst::long64_type) {
		if (vector_valid) {
			for (int t=0; t<time_steps; t++) {
				for (int i=0; i<size; i++) data[t][i] = (double) l_vec[t][i];
			}
		} else {
			raw_data_to_vec(data);
		}
	} else {
		if (vector_valid) {
			for (int t=0; t<time_steps; t++) {
				for (int i=0; i<size; i++) data[t][i] = d_vec[t][i];
			}
		} else {
			d_array_type temp;
			raw_data_to_vec(temp);
			for (int t=0; t<time_steps; t++) {
				for (int i=0; i<size; i++) data[t][i] = temp[t][i];
			}
		}		
	}
}

void DbfColContainer::GetVec(l_array_type& data)
{
	if (type != GeoDaConst::double_type &&
		type != GeoDaConst::long64_type) return;
	if (data.shape()[0] != time_steps || data.shape()[1] != size) {
		data.resize(boost::extents[time_steps][size]);
	}
	if (type == GeoDaConst::long64_type) {
		if (vector_valid) {
			for (int t=0; t<time_steps; t++) {
				for (int i=0; i<size; i++) data[t][i] = l_vec[t][i];
			}
		} else {
			raw_data_to_vec(data);
		}
	} else {
		if (vector_valid) {
			for (int t=0; t<time_steps; t++) {
				for (int i=0; i<size; i++) data[t][i] = (wxInt64) d_vec[t][i];
			}
		} else {
			l_array_type temp;
			raw_data_to_vec(temp);
			for (int t=0; t<time_steps; t++) {
				for (int i=0; i<size; i++) data[t][i] = (wxInt64) temp[t][i];
			}
		}
	}
}

// Note: we should check that every value written is within proper bounds
void DbfColContainer::SetFromVec(std::vector<double>& vec, int time)
{
	if (grid_base->IsSpaceTimeIdField(name)) return;
	if (vec.size() != size) return;
	if (time < 0 || time >= time_steps) time = 0;
	if (type == GeoDaConst::long64_type) {
		if (l_vec.shape()[1] != size) {
			l_vec.resize(boost::extents[time_steps][size]);
			std::fill(l_vec.origin(), l_vec.origin()+l_vec.num_elements(), 0);
		}
	} else if (type == GeoDaConst::double_type) {
		if (d_vec.shape()[1] != size) {
			d_vec.resize(boost::extents[time_steps][size]);
			std::fill(d_vec.origin(), d_vec.origin()+d_vec.num_elements(), 0);
		}
	} else {
		// only numeric types supported currently
		return;
	}
	if (IsRawDataAlloc()) FreeRawData();
 	
	CheckUndefined();
	for (int i=0; i<size; i++) {
		undefined[time][i] = !boost::math::isfinite<double>(vec[i]);
	}
	
	if (type == GeoDaConst::long64_type) {
		for (int i=0; i<size; i++) {
			l_vec[time][i] = undefined[time][i] ? 0 : (wxInt64) vec[i];
		}
	} else { // must be double_type
		for (int i=0; i<size; i++) {
			d_vec[time][i] = undefined[time][i] ? 0 : vec[i];
		}
	}
	stale_min_max_val[time] = true;
	vector_valid = true;
	UpdateMinMaxVals();
	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::SetFromVec(std::vector<wxInt64>& vec, int time)
{
	if (grid_base->IsSpaceTimeIdField(name)) return;
	if (vec.size() != size) return;
	if (time < 0 || time >= time_steps) time = 0;
	if (type == GeoDaConst::long64_type) {
		if (l_vec.shape()[1] != size) {
			l_vec.resize(boost::extents[time_steps][size]);
		}
	} else if (type == GeoDaConst::double_type) {
		if (d_vec.shape()[1] != size) {
			d_vec.resize(boost::extents[time_steps][size]);
		}
	} else {
		// only numeric types supported currently
		return;
	}
	if (IsRawDataAlloc()) FreeRawData();

	CheckUndefined();
	for (int i=0; i<size; i++) undefined[time][i] = false;
	
	if (type == GeoDaConst::long64_type) {
		for (int i=0; i<size; i++) l_vec[time][i] = vec[i];
	} else { // must be double_type
		for (int i=0; i<size; i++) d_vec[time][i] = (double) vec[i];
	}
	stale_min_max_val[time] = true;
	vector_valid = true;
	UpdateMinMaxVals();
	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::SetFromVec(std::vector<std::string>& vec, int time)
{
	if (grid_base->IsSpaceTimeIdField(name)) return;
	if (vec.size() != size) return;
	if (time < 0 || time >= time_steps) time = 0;
	if (s_vec.shape()[1] != size) {
		s_vec.resize(boost::extents[time_steps][size]);
	}
	if (IsRawDataAlloc()) FreeRawData();	
	for (int i=0; i<size; i++) undefined[time][i] = false;
	for (int i=0; i<size; i++) s_vec[time][i] = wxString(vec[i]);
	stale_min_max_val[time] = false;
	vector_valid = true;

	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::SetUndefined(const std::vector<bool>& undef_vec, int time)
{
	if (grid_base->IsSpaceTimeIdField(name)) return;
	if (undefined.shape()[1] != size) {
		undefined.resize(boost::extents[time_steps][size]);
	}
	if (time < 0 || time >= time_steps) time = 0;
	CheckUndefined();
	for (int i=0; i<size; i++) undefined[time][i] = undef_vec[i];
	grid_base->SetChangedSinceLastSave(true);
}

void DbfColContainer::GetUndefined(std::vector<bool>& undef_vec, int time)
{
	if (undef_vec.size() != size) undef_vec.resize(size);
	if (time < 0 || time >= time_steps) time = 0;
	if (!undefined_initialized) {
		CheckUndefined();
	}
	for (int i=0; i<size; i++) undef_vec[i] = undefined[time][i];
}

void DbfColContainer::GetUndefined(b_array_type& data)
{
	if (data.shape()[0] != time_steps || data.shape()[1] != size) {
		data.resize(boost::extents[time_steps][size]);
	}
	if (!undefined_initialized) {
		CheckUndefined();
	}
	for (int t=0; t<time_steps; t++) {
		for (int i=0; i<size; i++) data[t][i] = undefined[t][i];
	}
}

void DbfColContainer::CheckUndefined()
{
	if (undefined_initialized) return;
	undefined_initialized = true;
	if (undefined.shape()[1] != size) {
		undefined.resize(boost::extents[time_steps][size]);
	}
	const int inc = field_len+1;
	for (int t=0; t<time_steps; t++) {
		for (int i=0; i<size; i++) undefined[t][i] = false;
		if (type == GeoDaConst::double_type) {
			if (IsRawDataAlloc()) {
				char* buf=raw_data[t];
				for (int i=0; i<size; i++) {
					// we are not using atof since we it seems to be difficult
					// to choose a US locale on all systems so as to assume the
					// DBF-required use of '.' for the decimal character
					wxString temp(buf);
					temp.Trim(true);
					temp.Trim(false);
					double x;
					bool r = temp.ToCDouble(&x);
					undefined[t][i] = !(r && boost::math::isfinite<double>(x));
					buf += inc;
				}	
			} else {
				for (int i=0; i<size; i++) {
					undefined[t][i] = !boost::math::isfinite<double>(d_vec[t][i]);
				}
			}
		} else if (type == GeoDaConst::long64_type) {
			if (IsRawDataAlloc()) {
				char* buf=raw_data[t];
				for (int i=0; i<size; i++) {
					undefined[t][i] = !GenUtils::validInt(buf);
					buf += inc;
				}
			} else {
				// all integers are valid, there is no way to
				// represent infinity or NaN as in float and double
			}
		}
	}
}

void DbfColContainer::raw_data_to_vec(d_array_type& vec)
{
	if (vec.shape()[0] != time_steps || vec.shape()[1] != size) {
		vec.resize(boost::extents[time_steps][size]);
	}
	const int inc = field_len+1;
	for (int t=0; t<time_steps; t++) {
		char* buf=raw_data[t];
		for (int i=0; i<size; i++) {
			// we are not using atof since we it seems to be difficult
			// to choose a US locale on all systems so as to assume the
			// DBF-required use of '.' for the decimal character
			wxString temp(buf);
			temp.Trim(true);
			temp.Trim(false);
			bool r = temp.ToCDouble(&vec[t][i]);
			undefined[t][i] = !(r && boost::math::isfinite<double>(vec[t][i]));
			buf += inc;
		}
	}
	undefined_initialized = true;
}

void DbfColContainer::raw_data_to_vec(std::vector<double>& vec, int time)
{
	if (time < 0 || time >= time_steps) time = 0;
	if (vec.size() != size) vec.resize(size);
	CheckUndefined();
	const int inc = field_len+1;
	char* buf=raw_data[time];
	for (int i=0; i<size; i++) {
		// we are not using atof since we it seems to be difficult
		// to choose a US locale on all systems so as to assume the
		// DBF-required use of '.' for the decimal character
		wxString temp(buf);
		temp.Trim(true);
		temp.Trim(false);
		bool r = temp.ToCDouble(&vec[i]);
		undefined[time][i] = !(r && boost::math::isfinite<double>(vec[i]));
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(l_array_type& vec)
{
	if (vec.shape()[0] != time_steps || vec.shape()[1] != size) {
		vec.resize(boost::extents[time_steps][size]);
	}
	const int inc = field_len+1;
	for (int t=0; t<time_steps; t++) {
		char* buf=raw_data[t];
		for (int i=0; i<size; i++) {
			undefined[t][i] = !GenUtils::validInt(buf);
			GenUtils::strToInt64(buf, &vec[t][i]);  // will set to 0 if undefined
			buf += inc;
		}
	}
	undefined_initialized = true;
}

void DbfColContainer::raw_data_to_vec(std::vector<wxInt64>& vec, int time)
{
	if (time < 0 || time >= time_steps) time = 0;
	if (vec.size() != size) vec.resize(size);
	CheckUndefined();
	const int inc = field_len+1;
	char* buf=raw_data[time];
	for (int i=0; i<size; i++) {
		undefined[time][i] = !GenUtils::validInt(buf);
		GenUtils::strToInt64(buf, &vec[i]);  // will set to 0 if undefined
		buf += inc;
	}
}

void DbfColContainer::raw_data_to_vec(s_array_type& vec)
{
	if (vec.shape()[0] != time_steps || vec.shape()[1] != size) {
		vec.resize(boost::extents[time_steps][size]);
	}
	const int inc = field_len+1;
	for (int t=0; t<time_steps; t++) {
		char* buf=raw_data[t];
		for (int i=0; i<size; i++) {
			vec[t][i] = wxString(buf);
			undefined[t][i] = false;
			buf += inc;
		}
	}
	undefined_initialized = true;
}

void DbfColContainer::raw_data_to_vec(std::vector<wxString>& vec, int time)
{
	if (time < 0 || time >= time_steps) time = 0;
	if (vec.size() != size) vec.resize(size);
	CheckUndefined();
	const int inc = field_len+1;
	char* buf=raw_data[time];
	for (int i=0; i<size; i++) {
		vec[i] = wxString(buf);
		undefined[time][i] = false;
		buf += inc;
	}
}

void DbfColContainer::d_vec_to_raw_data()
{
	char temp[255];
	const int inc = field_len + 1;
	for (int t=0; t<time_steps; t++) {
		char* buf=raw_data[t];
		for (int i=0; i<size; i++) {
			if (!boost::math::isfinite<double>(d_vec[t][i])) {
				// the number is either NaN (not a number) or +/- infinity
				undefined[t][i] = true;
			}
			if (undefined[t][i]) {
				for (int j=0; j<field_len; j++) buf[j] = ' ';
			} else {
				sprintf(temp, "%#*.*f", field_len, decimals, d_vec[t][i]);
				for (int j=0; j<field_len; j++) buf[j] = temp[j];
			}
			buf[field_len] = '\0';
			buf += inc;
		}
		if (!sprintf_period_for_decimal()) {
			for (int i=0, iend=size*inc; i<iend; i++) {
				if (raw_data[t][i] == ',') raw_data[t][i] = '.';
			}
		}
	}
}

void DbfColContainer::l_vec_to_raw_data()
{
	char temp[255];
	const int inc = field_len + 1;
	for (int t=0; t<time_steps; t++) {
		char* buf=raw_data[t];
		for (int i=0; i<size; i++) {
			if (undefined[t][i]) {
				for (int j=0; j<field_len; j++) buf[j] = ' ';
			} else {
				sprintf(temp, "%*lld", field_len, l_vec[t][i]);
				for (int j=0; j<field_len; j++) buf[j] = temp[j];
			}
			buf[field_len] = '\0';
			buf += inc;
		}
	}
}

void DbfColContainer::s_vec_to_raw_data()
{
	char temp[255];
	const int inc = field_len + 1;
	for (int t=0; t<time_steps; t++) {
		char* buf=raw_data[t];
		for (int i=0; i<size; i++) {
			if (undefined[t][i] || s_vec[t][i].IsEmpty()) {
				for (int j=0; j<field_len; j++) buf[j] = ' ';
			} else {
				sprintf(temp, "%*s", field_len,
						(const_cast<char*>((const char*)s_vec[t][i].mb_str())));
				for (int j=0; j<field_len; j++) buf[j] = temp[j];
			}
			buf[field_len] = '\0';
			buf += inc;
		}
	}
}

void DbfColContainer::CopyRawDataToVector()
{
	if (!IsRawDataAlloc()) return;
	switch (type) {
		case GeoDaConst::date_type:
			raw_data_to_vec(l_vec);
			break;
		case GeoDaConst::long64_type:
			raw_data_to_vec(l_vec);
			break;
		case GeoDaConst::double_type:
			raw_data_to_vec(d_vec);
			break;
		case GeoDaConst::string_type:
			raw_data_to_vec(s_vec);
			break;
		default:
			break;
	}
	vector_valid = true;
}


void DbfColContainer::CopyVectorToRawData()
{
	switch (type) {
		case GeoDaConst::date_type:
		{
			if (l_vec.shape()[1] != size) return;
			if (!IsRawDataAlloc()) AllocRawData();
			l_vec_to_raw_data();
		}			
		case GeoDaConst::long64_type:
		{
			if (l_vec.shape()[1] != size) return;
			if (!IsRawDataAlloc()) AllocRawData();
			l_vec_to_raw_data();
		}
			break;
		case GeoDaConst::double_type:
		{
			if (d_vec.shape()[1] != size) return;
			if (!IsRawDataAlloc()) AllocRawData();
			d_vec_to_raw_data();
		}
			break;
		case GeoDaConst::string_type:
		{
			if (s_vec.shape()[1] != size) return;
			if (!IsRawDataAlloc()) AllocRawData();
			s_vec_to_raw_data();
		}
			break;
		default:
			break;
	}
}


DbfGridCellAttrProvider::DbfGridCellAttrProvider(std::vector<int>& row_order_s,
												 std::vector<bool>& selected_s)
: row_order(row_order_s), selected(selected_s)
{
    attrForSelectedRows = new wxGridCellAttr;
    attrForSelectedRows->SetBackgroundColour(*wxLIGHT_GREY);
}

DbfGridCellAttrProvider::~DbfGridCellAttrProvider()
{
    attrForSelectedRows->DecRef();
}

wxGridCellAttr *DbfGridCellAttrProvider::GetAttr(int row, int col,
									wxGridCellAttr::wxAttrKind kind ) const
{
	//LOG_MSG(wxString::Format("Calling DbfGridCellAttrProvider::GetAttr"
	//						 "(%d, %d, %d)", row, col, kind));
    wxGridCellAttr *attr = wxGridCellAttrProvider::GetAttr(row, col, kind);
	
	//if (row >= 0) LOG_MSG(wxString::Format("GetAttr: row=%d, "
	//									   "col=%d selected=%d",
	//									   row, col,
	//									   selected[row_order[row]] ? 1 : 0));
	
    if ( row >= 0 && selected[row_order[row]] ) {
        if ( !attr ) {
            attr = attrForSelectedRows;
            attr->IncRef();
        } else {
            if ( !attr->HasBackgroundColour() ) {
                wxGridCellAttr *attrNew = attr->Clone();
                attr->DecRef();
                attr = attrNew;
                attr->SetBackgroundColour(*wxLIGHT_GREY);
            }
        }
    }
	
    return attr;
}


DbfGridTableBase::DbfGridTableBase(int rows_s, int cols_s,
								   HighlightState* highlight_state_s,
								   TableState* table_state_s)
: rows(rows_s), time_steps(1), time_ids(1, 0), curr_time_step(0),
highlight_state(highlight_state_s), table_state(table_state_s),
hs(highlight_state_s->GetHighlight()),
sorting_col(-1),
sorting_ascending(false),
row_order(rows_s), col_data(cols_s),
changed_since_last_save(false), dbf_file_name_no_ext("")
{
	LOG_MSG("Entering DbfGridTableBase::DbfGridTableBase");
	int cols = cols_s;
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	orig_header.version = 3; // default
	orig_header.year = timeinfo->tm_year+1900;
	orig_header.month = timeinfo->tm_mon+1;
	orig_header.day = timeinfo->tm_mday;
	orig_header.num_records=0;
	orig_header.header_length=0;
	orig_header.length_each_record=0;
	orig_header.num_fields=0;
	
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		col_data[i] = new DbfColContainer(this);
		col_data[i]->Init(rows, time_steps, GeoDaConst::double_type,
						 wxString::Format("col_%d", i+1),
						 20, 4, 4, false, true, true);
	}
	SortByDefaultDecending();
	for (int j=0, jend=cols; j<jend; j++) {
		for (int i=0, iend=rows; i<iend; i++) {
			col_data[j]->undefined[0][i] = false;
			col_data[j]->d_vec[0][i] = (j*10000)+i + 0.1;
		}
	}
	highlight_state->registerObserver(this);
	table_state->registerObserver(this);
	//SetAttrProvider(new DbfGridCellAttrProvider(row_order, hs));
	LOG_MSG("Exiting DbfGridTableBase::DbfGridTableBase");
}

/** If field_names size != string_table width, then field_names will
 be automatically generated */
DbfGridTableBase::DbfGridTableBase(const std_str_array_type& string_table,
								   const std::vector<std::string>& field_names,
								   const std::string file_name,
								   HighlightState* highlight_state_s,
								   TableState* table_state_s)
: rows(string_table.shape()[0]), time_steps(1), time_ids(1, 0),
curr_time_step(0), highlight_state(highlight_state_s),
table_state(table_state_s),
hs(highlight_state_s->GetHighlight()),
sorting_col(-1), sorting_ascending(false),
row_order(string_table.shape()[0]), col_data(string_table.shape()[1]),
changed_since_last_save(false)
{
	LOG_MSG("Entering DbfGridTableBase::DbfGridTableBase");
	int cols = col_data.size();
	SortByDefaultDecending();
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	orig_header.version = 3; // default
	orig_header.year = timeinfo->tm_year+1900;
	orig_header.month = timeinfo->tm_mon+1;
	orig_header.day = timeinfo->tm_mday;
	orig_header.num_records=rows;
	orig_header.header_length=0; // To be calculated below
	orig_header.length_each_record=0; // To be calculated below
	orig_header.num_fields=cols;
	
	dbf_file_name = wxFileName(wxString(file_name));
	dbf_file_name_no_ext = wxFileName(wxString(file_name)).GetName();
	
	// Will try to convert each column of string table first into
	// valid doubles, then into valid ints, and default to leave as strings
	
	std::vector<wxString> col_names(cols);
	if (field_names.size() != cols) {
		for (int col=0; col<cols; col++) {
			col_names[col] << "VAR_" << col+1;
		}
	} else {
		std::set<wxString> names;
		std::vector<bool> is_valid_col_name(cols);
		std::vector<wxString> name_to_use(cols);
		for (int col=0; col<cols; col++) {
			wxString n = wxString(field_names[col]);
			n.Trim(true);
			n.Trim(false);
			n.MakeUpper();
			if (DbfFileUtils::isValidFieldName(n) &&
				(names.find(n) == names.end())) {
				is_valid_col_name[col] = true;
				names.insert(n);
				name_to_use[col] = n;
			}
		}
		
		int var_gen_cnt=0;
		for (int col=0; col<cols; col++) {
			bool valid_name_found = false;
			wxString next_col_name("VAR");
			if (is_valid_col_name[col]) {
				col_names[col] = name_to_use[col];
			} else {
				// keep trying VAR_XX until unused name encountered.
				// max out at VAR_99999
				while (!valid_name_found || var_gen_cnt > 99999) {
					var_gen_cnt++;
					next_col_name = wxEmptyString;
					next_col_name << "VAR_" << var_gen_cnt;
					if (names.find(next_col_name) == names.end()) {
						valid_name_found = true;
						names.insert(next_col_name);
					}
				}
				col_names[col] = next_col_name;
			}
		}
	}
	
	std::vector<wxInt64> v_longs;
	std::vector<double> v_doubles;
	std::vector<std::string> v_strings;
	std::vector<bool> undef;
	int failed_index;
	for (int col=0; col<cols; col++) {
		col_data[col] = new DbfColContainer(this);
		GeoDaConst::FieldType type;
		int length = 0;
		int decimals = 0;
		if (GeoDa::ConvertColToLongs(string_table, col, v_longs,
									 undef, failed_index)) {
			// column of integers
			type = GeoDaConst::long64_type;
			length = GeoDaConst::max_dbf_long_len;
			col_data[col]->Init(rows, time_steps, type, col_names[col],
								length, decimals, decimals,
								false, true, true);
			col_data[col]->SetFromVec(v_longs);
			col_data[col]->SetUndefined(undef);
		} else if (GeoDa::ConvertColToDoubles(string_table, col, v_doubles,
											  undef, failed_index)) {
			// column of reals
			type = GeoDaConst::double_type;
			//MMM: for some reason, if length is > 17, then values in
			// table get truncated to 1.0
			length = 17; //GeoDaConst::max_dbf_double_len;
			double max = 0;
			bool has_neg = false;
			for (int i=0; i<rows; i++) {
				if (fabs(v_doubles[i]) > max) max = fabs(v_doubles[i]);
				if (v_doubles[i] < 0) has_neg = true;
			}
			int digits = (int) ceil(log10(max));
			decimals = length - (digits + 2);
			if (decimals < 1) decimals = 1;
			if (decimals > GeoDaConst::max_dbf_double_decimals) {
				length = length - (decimals -
								   GeoDaConst::max_dbf_double_decimals);
				decimals = GeoDaConst::max_dbf_double_decimals;
			}
			int display_dec = GeoDaConst::default_display_decimals;
			if (display_dec > decimals) display_dec = decimals;
			col_data[col]->Init(rows, time_steps, type, col_names[col],
								length, decimals, display_dec,
								false, true, true);
			col_data[col]->SetFromVec(v_doubles);
			col_data[col]->SetUndefined(undef);
		} else {
			// column of strings by default
			v_strings.resize(rows);
			type = GeoDaConst::string_type;
			for (int i=0; i<rows; i++) {
				if (string_table[i][col].length() > length) {
					length = string_table[i][col].length();
				}
			}
			col_data[col]->Init(rows, time_steps, type, col_names[col],
								length, decimals, decimals,
								false, true, true);
			for (int i=0; i<rows; i++) v_strings[i] = string_table[i][col];
			col_data[col]->SetFromVec(v_strings);
		}
	}
	// Each field descriptor is 32 bits and begins at byte 32 and terminates
	// with an additional byte 0x0D.
	orig_header.header_length = 32 + orig_header.num_fields*32 + 1;
	orig_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	for (int i=0; i<orig_header.num_fields; i++) {
		orig_header.length_each_record += col_data[i]->field_len;
	}
	
	SetAttrProvider(new DbfGridCellAttrProvider(row_order, hs));
	highlight_state->registerObserver(this);
	table_state->registerObserver(this);
	LOG_MSG("Exiting DbfGridTableBase::DbfGridTableBase");
}

DbfGridTableBase::DbfGridTableBase(DbfFileReader& dbf,
								   HighlightState* highlight_state_s,
								   TableState* table_state_s)
: rows(dbf.getNumRecords()), time_steps(1), time_ids(1, 0), curr_time_step(0),
highlight_state(highlight_state_s), table_state(table_state_s),
hs(highlight_state_s->GetHighlight()),
sorting_col(-1), sorting_ascending(false),
row_order(dbf.getNumRecords()), col_data(dbf.getNumFields()),
orig_header(dbf.getFileHeader()),
changed_since_last_save(false)
{
	LOG_MSG("Entering DbfGridTableBase::DbfGridTableBase");
	int cols = dbf.getNumFields();
	SortByDefaultDecending();
	
	dbf_file_name = wxFileName(dbf.fname);
	dbf_file_name_no_ext = wxFileName(dbf.fname).GetName();
	
	std::vector<DbfFieldDesc> desc = dbf.getFieldDescs();
	for (int col=0, col_end=desc.size(); col<col_end; col++) {
		col_data[col] = new DbfColContainer(this);
		GeoDaConst::FieldType type = GeoDaConst::string_type;
		if (desc[col].type == 'N' || desc[col].type == 'F') {
			if (desc[col].decimals > 0) {
				type = GeoDaConst::double_type;
				col_data[col]->Init(rows, time_steps, type, desc[col].name,
									desc[col].length, desc[col].decimals,
									desc[col].decimals,
									true, false, false);	
			} else {
				type = GeoDaConst::long64_type;
				col_data[col]->Init(rows, time_steps, type, desc[col].name,
									desc[col].length, desc[col].decimals,
									desc[col].decimals,
									true, false, false);
			}
		} else if (desc[col].type == 'D') {
			if (desc[col].length != 8) {
				LOG_MSG("Error: Date field found with incorrect length!"
						" We recomend fixing this in your DBF before "
						"proceeding.");
			}
			type = GeoDaConst::date_type;
			col_data[col]->Init(rows, time_steps, type, desc[col].name,
								desc[col].length, 0, 0, true, false, false);
		} else {
			// We will assume (desc[i].type == 'C')
			type = GeoDaConst::string_type;
			col_data[col]->Init(rows, time_steps, type, desc[col].name,
								desc[col].length, desc[col].decimals,
								desc[col].decimals,
								true, false, false);
			// note that we consider all strings to be valid, so we chose
			// to say that all string values have been defined.
			//dbf.getFieldValsString(col, col_data[col]->s_vec);
		}
	}
	
	if (!dbf.file.is_open()) {
		dbf.file.open(dbf.fname.mb_str(wxConvUTF8),
					  std::ios::in | std::ios::binary);
	}
	if (!(dbf.file.is_open() && dbf.file.good())) return;
	
	// Note: first byte of every DBF row is the record deletion flag, so
	// we always skip this.
	int del_flag_len = 1;  // the record deletion flag
	dbf.file.seekg(dbf.header.header_length, std::ios::beg);
	for (int row=0; row<rows; row++) {
		dbf.file.seekg(del_flag_len, std::ios::cur);
		for (int col=0; col<cols; col++) {
			int field_len = desc[col].length;
			//LOG(dbf.file.tellg());
			dbf.file.read((char*)(col_data[col]->raw_data[0] + row*(field_len+1)),
						  field_len);
			col_data[col]->raw_data[0][row*(field_len+1)+field_len] = '\0';
			//LOG_MSG(wxString((char*)(col_data[col]->raw_data[0]
			//						 + row*(field_len+1))));
		}
	}
	SetAttrProvider(new DbfGridCellAttrProvider(row_order, hs));
	
	highlight_state->registerObserver(this);
	table_state->registerObserver(this);
	LOG_MSG("Exiting DbfGridTableBase::DbfGridTableBase");
}

DbfGridTableBase::DbfGridTableBase(DbfFileReader& dbf_sp,
								   DbfFileReader& dbf_tm,
								   int sp_tbl_sp_col,
								   int tm_tbl_sp_col,
								   int tm_tbl_tm_col,
								   HighlightState* highlight_state_s,
								   TableState* table_state_s)
: rows(dbf_sp.getNumRecords()), time_steps(1), time_ids(1, 0),
curr_time_step(0),
highlight_state(highlight_state_s), table_state(table_state_s),
hs(highlight_state_s->GetHighlight()),
sorting_col(-1), sorting_ascending(false),
row_order(dbf_sp.getNumRecords()),
col_data(dbf_sp.getNumFields() + dbf_tm.getNumFields() - 2),
orig_header(dbf_sp.getFileHeader()),
orig_header_tm(dbf_tm.getFileHeader()),
changed_since_last_save(false)
{
	using namespace std;
	LOG_MSG("Entering DbfGridTableBase::DbfGridTableBase");
	int cols_sp = dbf_sp.getNumFields();
	int cols_tm = dbf_tm.getNumFields();
	SortByDefaultDecending();
	sp_tbl_sp_col_name = dbf_sp.getFieldDesc(sp_tbl_sp_col).name;
	tm_tbl_sp_col_name = dbf_tm.getFieldDesc(tm_tbl_sp_col).name;
	tm_tbl_tm_col_name = dbf_tm.getFieldDesc(tm_tbl_tm_col).name;
	
	dbf_file_name = wxFileName(dbf_sp.fname);
	dbf_file_name_no_ext = wxFileName(dbf_sp.fname).GetName();
	dbf_tm_file_name = wxFileName(dbf_tm.fname);
	dbf_tm_file_name_no_ext = wxFileName(dbf_tm.fname).GetName();
		
	vector<wxInt64> tm_tbl_tm_ids(dbf_tm.getNumRecords());
	set<wxInt64> t_tm_ids;
	if (!dbf_tm.getFieldValsLong(tm_tbl_tm_col, tm_tbl_tm_ids)) {
		LOG_MSG("Error: unable to read time column data");
		return;
	}
	for (int i=0, iend=tm_tbl_tm_ids.size(); i<iend; i++) {
		t_tm_ids.insert(tm_tbl_tm_ids[i]);
	}
	if (dbf_tm.getNumRecords() % (int) t_tm_ids.size() != 0) {
		wxString msg;
		msg << "Error: the number of time DBF records must be a multiple of ";
		msg << " the number of time column values.\n";
		msg << "Number of time dbf records: " << dbf_tm.getNumRecords();
		msg << ", number of unique time values: " << t_tm_ids.size();
		LOG_MSG(msg);
		return;
	}
	time_steps = t_tm_ids.size();
	time_ids.clear();
	for (set<wxInt64>::iterator it=t_tm_ids.begin(); it!=t_tm_ids.end(); it++) {
		time_ids.push_back(*it);
	}
	sort(time_ids.begin(), time_ids.end());
	//for (int i=0; i<time_steps; i++) { LOG(time_ids[i]); }
	map<wxInt64,int> time_id_to_step;
	for (int i=0, iend=time_ids.size(); i<iend; i++) {
		time_id_to_step[time_ids[i]] = i;
	}
	
	vector<DbfFieldDesc> desc_sp = dbf_sp.getFieldDescs();
	for (int col=0; col<cols_sp; col++) {
		col_data[col] = new DbfColContainer(this);
		GeoDaConst::FieldType type = GeoDaConst::string_type;
		if (desc_sp[col].type == 'N' || desc_sp[col].type == 'F') {
			if (desc_sp[col].decimals > 0) {
				type = GeoDaConst::double_type;
				col_data[col]->Init(rows, 1, type, desc_sp[col].name,
									desc_sp[col].length, desc_sp[col].decimals,
									desc_sp[col].decimals,
									true, false, false);	
			} else {
				type = GeoDaConst::long64_type;
				col_data[col]->Init(rows, 1, type, desc_sp[col].name,
									desc_sp[col].length, desc_sp[col].decimals,
									desc_sp[col].decimals,
									true, false, false);
			}
		} else if (desc_sp[col].type == 'D') {
			if (desc_sp[col].length != 8) {
				LOG_MSG("Error: Date field found with incorrect length!"
						" We recomend fixing this in your DBF before "
						"proceeding.");
			}
			type = GeoDaConst::date_type;
			col_data[col]->Init(rows, 1, type, desc_sp[col].name,
								desc_sp[col].length, 0, 0, true, false, false);
		} else {
			// We will assume (desc_sp[i].type == 'C')
			type = GeoDaConst::string_type;
			col_data[col]->Init(rows, 1, type, desc_sp[col].name,
								desc_sp[col].length, desc_sp[col].decimals,
								desc_sp[col].decimals,
								true, false, false);
			// note that we consider all strings to be valid, so we chose
			// to say that all string values have been defined.
			//dbf.getFieldValsString(col, col_data[col]->s_vec);
		}
	}

	vector<DbfFieldDesc> desc_tm = dbf_tm.getFieldDescs();
	int col_ind = cols_sp;
	for (int col=0; col<cols_tm; col++) {
		if (col != tm_tbl_sp_col && col != tm_tbl_tm_col) {
			col_data[col_ind] = new DbfColContainer(this);
			GeoDaConst::FieldType type = GeoDaConst::string_type;
			if (desc_tm[col].type == 'N' || desc_tm[col].type == 'F') {
				if (desc_tm[col].decimals > 0) {
					type = GeoDaConst::double_type;
					col_data[col_ind]->Init(rows, time_steps, type,
											desc_tm[col].name,
											desc_tm[col].length,
											desc_tm[col].decimals,
											desc_tm[col].decimals,
											true, false, false);	
				} else {
					type = GeoDaConst::long64_type;
					col_data[col_ind]->Init(rows, time_steps, type,
											desc_tm[col].name,
											desc_tm[col].length,
											desc_tm[col].decimals,
											desc_tm[col].decimals,
											true, false, false);
				}
			} else if (desc_tm[col].type == 'D') {
				if (desc_tm[col].length != 8) {
					LOG_MSG("Error: Date field found with incorrect length!"
							" We recomend fixing this in your DBF before "
							"proceeding.");
				}
				type = GeoDaConst::date_type;
				col_data[col_ind]->Init(rows, time_steps, type,
										desc_tm[col].name,
										desc_tm[col].length, 0, 0,
										true, false, false);
			} else {
				// We will assume (desc_tm[i].type == 'C')
				type = GeoDaConst::string_type;
				col_data[col_ind]->Init(rows, time_steps, type,
										desc_tm[col].name,
										desc_tm[col].length,
										desc_tm[col].decimals,
										desc_tm[col].decimals,
										true, false, false);
				// note that we consider all strings to be valid, so we chose
				// to say that all string values have been defined.
				//dbf.getFieldValsString(col, col_data[col]->s_vec);
			}
			col_ind++;
		}
	}
		
	if (!dbf_sp.file.is_open()) {
		dbf_sp.file.open(dbf_sp.fname.mb_str(wxConvUTF8),
						 std::ios::in | std::ios::binary);
	}
	if (!(dbf_sp.file.is_open() && dbf_sp.file.good())) {
		LOG_MSG("Error: could not open space DBF for reading");
		return;
	}
	if (!dbf_tm.file.is_open()) {
		dbf_tm.file.open(dbf_tm.fname.mb_str(wxConvUTF8),
						 std::ios::in | std::ios::binary);
	}
	if (!(dbf_tm.file.is_open() && dbf_tm.file.good())) {
		LOG_MSG("Error: could not open time DBF for reading");
		return;
	}
	
	// Note: first byte of every DBF row is the record deletion flag, so
	// we always skip this.
	char buff[256];
	int del_flag_len = 1;  // the record deletion flag
	dbf_sp.file.seekg(dbf_sp.header.header_length, std::ios::beg);
	for (int row=0; row<rows; row++) {
		dbf_sp.file.read(buff, del_flag_len);
		for (int col=0; col<cols_sp; col++) {
			int field_len = desc_sp[col].length;
			dbf_sp.file.read((char*)(col_data[col]->raw_data[0]
									 + row*(field_len+1)),
							 field_len);
			col_data[col]->raw_data[0][row*(field_len+1)+field_len] = '\0';
		}
	}
	
	// we need to match up time dbf rows with space dbf rows.  Don't want
	// to assume these are in the same order.
	vector<int> tm_row_to_sp_row(dbf_tm.getNumRecords());
	vector<int> tm_row_to_time_step(dbf_tm.getNumRecords());
	vector<wxInt64> sp_ids_vec;
	col_data[sp_tbl_sp_col]->GetVec(sp_ids_vec);
	map<wxInt64,int> sp_id_to_row;
	for (int i=0, iend=sp_ids_vec.size(); i<iend; i++) {
		if (sp_id_to_row.find(sp_ids_vec[i]) != sp_id_to_row.end()) {
			LOG_MSG("Error: duplicate space id found");
			return;
		} else {
			sp_id_to_row[sp_ids_vec[i]] = i;
		}
	}
	
	vector<wxInt64> tm_tbl_sp_ids(dbf_tm.getNumRecords());
	dbf_tm.getFieldValsLong(tm_tbl_sp_col, tm_tbl_sp_ids);
	for (int i=0, iend=tm_tbl_sp_ids.size(); i<iend; i++) {
		int tt_sp_id = tm_tbl_sp_ids[i];
		int tt_tm_id = tm_tbl_tm_ids[i];
		if (sp_id_to_row.find(tt_sp_id) == sp_id_to_row.end()) {
			wxString msg;
			msg << "Error: Table space id " << tt_sp_id;
			msg << " not found in time table.";
			LOG_MSG(msg);
			return;
		} else {
			tm_row_to_sp_row[i] = sp_id_to_row[tt_sp_id];
			tm_row_to_time_step[i] = time_id_to_step[tt_tm_id];
		}
	}
	
	dbf_tm.file.seekg(dbf_tm.header.header_length, std::ios::beg);
	for (int row=0, tm_recs=dbf_tm.getNumRecords(); row<tm_recs; row++) {
		dbf_tm.file.read(buff, del_flag_len);
		int col_ind = cols_sp;
		for (int col=0; col<cols_tm; col++) {
			int mrow = tm_row_to_sp_row[row];
			int ts = tm_row_to_time_step[row];
			int field_len = desc_tm[col].length;
			if (col != tm_tbl_sp_col && col != tm_tbl_tm_col) {
				dbf_tm.file.read((char*)
								 (col_data[col_ind]->raw_data[ts]
								  + mrow*(field_len+1)),
								 field_len);
				col_data[col_ind]->
					raw_data[ts][mrow*(field_len+1)+field_len] = '\0';
				col_ind++;
			} else {
				dbf_tm.file.read(buff, field_len);
			}
		}
	}
	
	SetAttrProvider(new DbfGridCellAttrProvider(row_order, hs));
	
	highlight_state->registerObserver(this);
	table_state->registerObserver(this);
	LOG_MSG("Exiting DbfGridTableBase::DbfGridTableBase");
}

DbfGridTableBase::~DbfGridTableBase()
{
	LOG_MSG("Entering DbfGridTableBase::~DbfGridTableBase");
	for (std::vector<DbfColContainer*>::iterator it=col_data.begin();
		 it != col_data.end(); it++) {
		delete (*it);
	}
	highlight_state->removeObserver(this);
	table_state->removeObserver(this);
	LOG_MSG("Exiting DbfGridTableBase::~DbfGridTableBase");
}

/** Impelmentation of HighlightStateObserver interface function.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void DbfGridTableBase::update(HighlightState* o)
{
	LOG_MSG("Entering DbfGridTableBase::update(HighlightState* o)");
	if (GetView()) GetView()->Refresh();
	LOG_MSG("Exiting DbfGridTableBase::update(HighlightState* o)");
}

/** This is the implementation of the TableStateObserver interface
 update function. It is called whenever the table associated with
 TableState's state has changed. Since this is the table, the update
 method will do nothing. */
void DbfGridTableBase::update(TableState* o)
{
	LOG_MSG("In DbfGridTableBase::update(TableState* o)");
}

/** Grid Base does not need to be notifed of TableViewer column
 movements since moving a column in the TableViewer doesn not
 change the state of Grid Base.  This method is here as a workaround
 to noftify interested listeners that the visual column order has
 changed.  It is called by NewTableViewerFrame::OnColMoveEvent */
void DbfGridTableBase::notifyColMove()
{
	LOG_MSG("In DbfGridTableBase::notifyColMove()");
	table_state->notifyObservers();
}

int DbfGridTableBase::GetNumberColsSpace()
{
	int cnt = 0;
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (col_data[i]->time_steps == 1) cnt++;
	}
	return cnt;
}

int DbfGridTableBase::GetNumberColsTime()
{ 
	int cnt = 0;
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (col_data[i]->time_steps > 1) cnt++;
	}
	return cnt;
}

wxString DbfGridTableBase::GetTimeString(int time)
{
	if (time < 0 || time > time_steps - 1) return wxEmptyString;
	wxString s;
	s << time_ids[time];
	return s;
}

void DbfGridTableBase::GetTimeStrings(std::vector<wxString>& times)
{
	times.resize(time_steps);
	for (int i=0; i<time_steps; i++) times[i] = GetTimeString(i);
}

bool DbfGridTableBase::IsColTimeVariant(const wxString& name)
{
	if (!ColNameExists(name)) return false;
	return IsColTimeVariant(FindColId(name));
}

bool DbfGridTableBase::IsColTimeVariant(int col)
{
	return col_data[col]->time_steps > 1;
}

int DbfGridTableBase::GetColTimeSteps(int col)
{
	return col_data[col]->time_steps;
}

bool DbfGridTableBase::IsColNumeric(int col)
{
	return (col_data[col]->type == GeoDaConst::double_type ||
			col_data[col]->type == GeoDaConst::long64_type);
}

GeoDaConst::FieldType DbfGridTableBase::GetColType(int col)
{
	return col_data[col]->type;
}

wxString DbfGridTableBase::GetColName(int col)
{
	return col_data[col]->name;
}

int DbfGridTableBase::GetColLength(int col)
{
	return col_data[col]->field_len;
}

int DbfGridTableBase::GetColDecimals(int col)
{
	return col_data[col]->decimals;
}

void DbfGridTableBase::GetColData(int col, d_array_type& dbl_data)
{
	col_data[col]->CheckUndefined();
	if (!IsColNumeric(col)) return;
	DbfColContainer* cd = col_data[col];
	cd->GetVec(dbl_data);
	for (int t=0, t_end=cd->time_steps; t<t_end; t++) {
		for (int i=0, iend=cd->size; i<iend; i++) {
			if (cd->undefined[t][i]) dbl_data[t][i] = 0;
		}
	}
}

void DbfGridTableBase::GetColData(int col, int time, std::vector<double>& data)
{
	col_data[col]->CheckUndefined();
	if (!IsColNumeric(col)) return;
	DbfColContainer* cd = col_data[col];
	cd->GetVec(data, time);
	for (int i=0, iend=cd->size; i<iend; i++) {
		if (cd->undefined[time][i]) data[i] = 0;
	}
}

void DbfGridTableBase::GetColUndefined(int col, b_array_type& undefined)
{
	col_data[col]->GetUndefined(undefined);
}

void DbfGridTableBase::GetColUndefined(int col, int time,
									   std::vector<bool>& undefined)
{
	col_data[col]->GetUndefined(undefined, time);
}

void DbfGridTableBase::GetMinMaxVals(int col, std::vector<double>& min_vals,
									 std::vector<double>& max_vals)
{
	if (!IsColNumeric(col)) return;
	col_data[col]->GetMinMaxVals(min_vals, max_vals);
}

bool DbfGridTableBase::ConvertToSpTime(const wxString& sp_dbf_name,
									   const wxString& tm_dbf_name,
									   int space_col,
									   const wxString& tm_field_name,
									   const std::vector<wxInt64>& new_time_ids,
									   wxString& err_msg)
{
	// backup original values first in case something goes wrong
	wxFileName dbf_file_name_orig = dbf_file_name;
	wxString dbf_file_name_no_ext_orig = dbf_file_name_no_ext;
	wxFileName dbf_tm_file_name_orig = dbf_tm_file_name;
	wxString dbf_tm_file_name_no_ext_orig = dbf_tm_file_name_no_ext;
	DbfFileHeader orig_header_orig = orig_header;
	DbfFileHeader orig_header_tm_orig = orig_header_tm;
	wxString sp_tbl_sp_col_name_orig = sp_tbl_sp_col_name;
	wxString tm_tbl_sp_col_name_orig = tm_tbl_sp_col_name;
	wxString tm_tbl_tm_col_name_orig = tm_tbl_tm_col_name;
	int time_steps_orig = time_steps;
	std::vector<wxInt64> time_ids_orig(time_steps);
	for (int i=0; i<time_steps_orig; i++) time_ids_orig[i] = time_ids[i];
	
	// modify values
	std::vector<int> col_id_map;
	FillColIdMap(col_id_map);
	
	dbf_file_name = sp_dbf_name;
	dbf_file_name_no_ext = wxFileName(sp_dbf_name).GetName();
	dbf_tm_file_name = tm_dbf_name;
	dbf_tm_file_name_no_ext = wxFileName(tm_dbf_name).GetName();
	
	orig_header_tm.version = orig_header.version;
	orig_header_tm.year = orig_header.year;
	orig_header_tm.month = orig_header.month;
	orig_header_tm.day = orig_header.day;
	orig_header_tm.num_records = GetNumberRows() * time_steps;
	orig_header_tm.num_fields = GetNumberColsTime() + 2;
	orig_header_tm.header_length = 32 + orig_header_tm.num_fields*32 + 1;
	orig_header_tm.length_each_record = 1; // first byte is either 0x20 or 0x2A
	// add in length of time and space fields
	orig_header_tm.length_each_record += col_data[space_col]->field_len;
	orig_header_tm.length_each_record += GeoDaConst::default_dbf_long_len;
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		int mcol = col_id_map[i];
		if (mcol != space_col && col_data[mcol]->time_steps > 1) {
			orig_header_tm.length_each_record += col_data[mcol]->field_len;
		}
	}
	
	sp_tbl_sp_col_name = col_data[space_col]->name;
	tm_tbl_sp_col_name = sp_tbl_sp_col_name;
	tm_tbl_tm_col_name = tm_field_name;
	curr_time_step = 0;
	time_steps = new_time_ids.size();
	time_ids.resize(time_steps);
	for (int i=0; i<time_steps; i++) time_ids[i] = new_time_ids[i];
	
	bool success = WriteToSpaceTimeDbf(sp_dbf_name, tm_dbf_name, err_msg);
	
	if (!success) {
		// restore orginal values
		dbf_file_name = dbf_file_name_orig;
		dbf_file_name_no_ext = dbf_file_name_no_ext_orig;
		dbf_tm_file_name = dbf_tm_file_name_orig;
		dbf_tm_file_name_no_ext = dbf_tm_file_name_no_ext_orig;
		orig_header = orig_header_orig;
		orig_header_tm = orig_header_tm_orig;
		sp_tbl_sp_col_name = sp_tbl_sp_col_name_orig;
		tm_tbl_sp_col_name = tm_tbl_sp_col_name_orig;
		tm_tbl_tm_col_name = tm_tbl_tm_col_name_orig;
		time_steps = time_steps_orig;
		time_ids.resize(time_steps_orig);
		for (int i=0; i<time_steps_orig; i++) time_ids[i] = time_ids_orig[i];
	}
	
	return success;
}


bool DbfGridTableBase::IsDuplicateColNames(wxString& dup_ret_str)
{
	bool dup = false;
	std::set<wxString> names;
	wxString name;
	std::set<wxString>::iterator it;
	for (int col=0; col<GetNumberCols(); col++) {
		name = GetColName(col).Upper();
		if (names.find(name) != names.end()) {
			dup_ret_str = name;
			dup = true;
			break;
		}
		names.insert(name);
	}
	return dup;
}

/// Use this method with great caution!  This is used by the
// TimeVariantImportDlg to copy raw data from sp field to tm fields.
// All properties must match exactly, or else method will return false.
// Furthermore, raw data must already be allocated for both space
// and time fields.
bool DbfGridTableBase::CopySpColRawToTmColRaw(int sp_col, int tm_col, int time)
{
	DbfColContainer* sp_c = col_data[sp_col];
	DbfColContainer* tm_c = col_data[tm_col];
	if (sp_c->type != tm_c->type || sp_c->field_len != tm_c->field_len ||
		sp_c->decimals != tm_c->decimals ||
		!sp_c->IsRawDataAlloc() || !tm_c->IsRawDataAlloc()) return false;
	
	for (int i=0, its=((sp_c->field_len)+1)*(sp_c->size); i<its; i++) {
		tm_c->raw_data[time][i] = sp_c->raw_data[0][i];
	}
	return true;
}

// pure virtual method implementation for wxGridTableBase
int DbfGridTableBase::GetNumberRows()
{
	return rows;
}

int DbfGridTableBase::GetNumberCols()
{ 
	return col_data.size();
}

wxString DbfGridTableBase::GetValue(int row, int col)
{
	if (row<0 || row>=GetNumberRows()) return wxEmptyString;
	int field_len = col_data[col]->field_len;
	
	int curr_ts = col_data[col]->time_steps > 1 ? curr_time_step : 0; 
	if (col_data[col]->undefined_initialized &&
		col_data[col]->undefined[curr_ts][row_order[row]]) {
		return wxEmptyString;
	}

	switch (col_data[col]->type) {
		case GeoDaConst::date_type:
		{
			if (col_data[col]->vector_valid) {
				int x = col_data[col]->l_vec[curr_ts][row_order[row]];
				int day = x % 100; x /= 100;
				int month = x % 100; x /= 100;
				int year = x;
				return wxString::Format("%04d %02d %02d", year, month, day);
			}
			if (col_data[col]->IsRawDataAlloc()) {
				wxString temp((char*)(col_data[col]->raw_data[curr_ts]
									  + row_order[row]*(field_len+1)));
				long val;
				bool success = temp.ToCLong(&val);
				
				if (col_data[col]->undefined_initialized || success) {
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
		case GeoDaConst::long64_type:
		{
			if (col_data[col]->vector_valid) {
				return wxString::Format("%lld",
						col_data[col]->l_vec[curr_ts][row_order[row]]);
			}
			if (col_data[col]->IsRawDataAlloc()) {
				const char* str = (char*)(col_data[col]->raw_data[curr_ts]
										  + row_order[row]*(field_len+1));
				//LOG_MSG(wxString::Format("row: %d, col: %d, raw: %s",
				//						 row, col, str));
				
				if (col_data[col]->undefined_initialized ||
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
		case GeoDaConst::double_type:
		{
			// We have to be careful to return a formated string with digits
			// after the decimal place at most min(decimals, displayed_decimals)
			int decimals = col_data[col]->decimals + 1; // one extra decimal
			int disp_dec = GenUtils::min<int>(col_data[col]->decimals,
											col_data[col]->displayed_decimals);
			wxString d_char = DbfColContainer::sprintf_period_for_decimal()
								? "." : ",";
			if (col_data[col]->vector_valid) {
				double val = col_data[col]->d_vec[curr_ts][row_order[row]];
				// limit val to acceptable range
				int d = col_data[col]->decimals;
				int fl = col_data[col]->field_len;
				double max_val = DbfFileUtils::GetMaxDouble(fl, d);
				double min_val = DbfFileUtils::GetMinDouble(fl, d);
				if (max_val < val) {
					val = max_val;
				} else if (min_val > val) {
					val = min_val;
				}
				wxString s = wxString::Format("%.*f", disp_dec, val);
				return s.SubString(0, s.Find(d_char) + disp_dec);
			}
			if (col_data[col]->IsRawDataAlloc()) {
				wxString temp((char*)(col_data[col]->raw_data[curr_ts]
									  + row_order[row]*(field_len+1)));
				// trim any leading or trailing spaces.  For some reason
				// a trailing space causes ToCDouble to return false even
				// though it set val to the correct value.
				temp.Trim(true);
				temp.Trim(false);
				double val;
				bool success = temp.ToCDouble(&val);
				if (success) success = boost::math::isfinite<double>(val);
				
				if (col_data[col]->undefined_initialized || success) {
					wxString s = wxString::Format("%.*f", disp_dec, val);
					return s.SubString(0, s.Find(d_char) + disp_dec);
				} else {
					return wxEmptyString;
				}
			}
		}
			break;
		case GeoDaConst::string_type:
		{
			if (col_data[col]->vector_valid) {
				return col_data[col]->s_vec[curr_ts][row_order[row]];
			}
			if (col_data[col]->IsRawDataAlloc()) {
				return wxString((char*)(col_data[col]->raw_data[curr_ts]
										+ row_order[row]*(field_len+1)));
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
void DbfGridTableBase::SetValue(int row, int col, const wxString &value)
{
	LOG_MSG(wxString::Format("DbfGridTableBase::SetValue(%d, %d, %s)",
							 row, col,
							 (const_cast<char*>((const char*)value.mb_str()))));
	if (row<0 || row>=GetNumberRows()) return;
	if (IsSpaceTimeIdField(col_data[col]->name)) return;
	
	int curr_ts = col_data[col]->time_steps > 1 ? curr_time_step : 0;
	int field_len = col_data[col]->field_len;
	int rrow = row_order[row];
	char temp[1024];
	char* buf=0;
	if (col_data[col]->IsRawDataAlloc()) {
		buf = col_data[col]->raw_data[curr_ts] + rrow*(field_len+1);
		buf[field_len] = '\0';
	}
	
	col_data[col]->stale_min_max_val[curr_ts] = true;
	col_data[col]->UpdateMinMaxVals();
	
	// assume defined by default
	col_data[col]->undefined[curr_ts][rrow] = false;
	switch (col_data[col]->type) {
		case GeoDaConst::date_type: {
			// first, check that value is valid.  If invalid, we will
			// write some default value and will set undefined to true
			wxInt64 l_val;
			bool valid = GenUtils::validInt(
								const_cast<char*>((const char*)value.mb_str()));
			if (valid) {
				GenUtils::strToInt64(
					const_cast<char*>((const char*)value.mb_str()), &l_val);
			} else {
				col_data[col]->undefined[curr_ts][rrow] = true;
			}
			if (col_data[col]->vector_valid) {
				if (col_data[col]->undefined[curr_ts][rrow]) {
					col_data[col]->l_vec[curr_ts][rrow] = 0;
				} else {
					col_data[col]->l_vec[curr_ts][rrow] = l_val;
				}
			}
			if (buf) {
				if (col_data[col]->undefined[curr_ts][rrow]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%*lld", field_len, l_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
				}
			}
			break;
		}
		case GeoDaConst::long64_type: {
			// first, check that value is valid.  If invalid, we will
			// write some default value and will set undefined to true
			wxInt64 l_val;
			if (!GenUtils::validInt(value)) {
				col_data[col]->undefined[curr_ts][rrow] = true;
			} else {
				GenUtils::strToInt64(value, &l_val);
				// limit l_val to acceptable range
				int fl = col_data[col]->field_len;
				wxInt64 max_val = DbfFileUtils::GetMaxInt(fl);
				wxInt64 min_val = DbfFileUtils::GetMinInt(fl);
				if (max_val < l_val) {
					l_val = max_val;
				} else if (min_val > l_val) {
					l_val = min_val;
				}
			}

			if (col_data[col]->vector_valid) {
				if (col_data[col]->undefined[curr_ts][rrow]) {
					col_data[col]->l_vec[curr_ts][rrow] = 0;
				} else {
					col_data[col]->l_vec[curr_ts][rrow] = l_val;
				}
			}
			if (buf) {
				if (col_data[col]->undefined[curr_ts][rrow]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%*lld", field_len, l_val);
					for (int j=0; j<field_len; j++) buf[j] = temp[j];
				}
			}
			break;
		}
		case GeoDaConst::double_type: {
			double d_val;
			if (!value.ToDouble(&d_val)) {
				col_data[col]->undefined[curr_ts][rrow] = true;
			} else if (!boost::math::isfinite<double>(d_val)) {
				col_data[col]->undefined[curr_ts][rrow] = true;
			}
			if (!col_data[col]->undefined[curr_ts][rrow]) {
				// limit d_val to acceptable range
				int d = col_data[col]->decimals;
				int fl = col_data[col]->field_len; 
				double max_val = DbfFileUtils::GetMaxDouble(fl, d);
				double min_val = DbfFileUtils::GetMinDouble(fl, d);
				if (max_val < d_val) {
					d_val = max_val;
				} else if (min_val > d_val) {
					d_val = min_val;
				}
			}
			if (col_data[col]->vector_valid) {
				if (col_data[col]->undefined[curr_ts][rrow]) {
					col_data[col]->d_vec[curr_ts][rrow] = 0;
				} else {
					col_data[col]->d_vec[curr_ts][rrow] = d_val;
				}
			}
			if (buf) {
				if (col_data[col]->undefined[curr_ts][rrow]) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					sprintf(temp, "%#*.*f", field_len, col_data[col]->decimals,
							d_val);
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
		case GeoDaConst::string_type: {
			if (col_data[col]->vector_valid) {
				if (value.Length() > field_len) {
					col_data[col]->s_vec[curr_ts][rrow]
						= value.Mid(0, field_len);	
				} else {
					col_data[col]->s_vec[curr_ts][rrow] = value;
				}
			}
			if (col_data[col]->IsRawDataAlloc()) {
				if (value.IsEmpty()) {
					for (int j=0; j<field_len; j++) buf[j] = ' ';
				} else {
					strncpy(buf, (const char*)value.mb_str(wxConvUTF8),
							field_len);
					buf[field_len]='\0';
				}
			}
			break;
		}
		default:
			break;
	}
	changed_since_last_save = true;
}

bool DbfGridTableBase::ColNameExists(const wxString& name)
{
	return (FindColId(name) != wxNOT_FOUND);
}

/** Returns the column id in the underlying grid, not the visual grid
 displayed order.  wxNOT_FOUND is returned if not found.  Always
 returns the first result found. */
int DbfGridTableBase::FindColId(const wxString& name)
{
	wxString c_name = name;
	c_name.Trim(false);
	c_name.Trim(true);
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (c_name.CmpNoCase(col_data[i]->name) == 0) return i;
	}
	return wxNOT_FOUND;
}

void DbfGridTableBase::PrintTable()
{
	for (int row=0, row_end=GetNumberRows(); row<row_end; row++) {
		for (int t=0; t<time_steps; t++) {
			wxString msg = "   ";
			for (int col=0, col_end=col_data.size(); col<col_end; col++) {
				col_data[col]->CopyRawDataToVector();
				switch (col_data[col]->type) {
					case GeoDaConst::date_type:
						msg << col_data[col]->l_vec[t][row] << " ";
						break;	
					case GeoDaConst::long64_type:
						msg << col_data[col]->l_vec[t][row] << " ";
						break;
					case GeoDaConst::double_type:
						msg << col_data[col]->d_vec[t][row] << " ";
						break;
					case GeoDaConst::string_type:
						msg << col_data[col]->s_vec[t][row] << " ";
						break;
					default:
						break;
				}
			}
			LOG_MSG(msg);
		}
	}
}

bool DbfGridTableBase::WriteToDbf(const wxString& fname, wxString& err_msg)
{
	std::ofstream out_file;
	out_file.open(fname.mb_str(wxConvUTF8), std::ios::out | std::ios::binary);
	if (!(out_file.is_open() && out_file.good())) {
		err_msg += "Problem opening \"" + fname + "\"";
		return false;
	}

	dbf_file_name_no_ext = wxFileName(fname).GetName();
	
	// Ensure that raw_data exists.  If raw_data exists, then each item is
	// assumed to be ready for writing to disk.
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (!col_data[i]->IsRawDataAlloc()) col_data[i]->CopyVectorToRawData();
	}

	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	// We must write the DBF in the current displayed column order
	std::vector<int> col_id_map;
	FillColIdMap(col_id_map);
	
	// update orig_header
	orig_header.num_records = GetNumberRows();
	orig_header.num_fields = GetNumberCols();
	// Each field descriptor is 32 bits and begins at byte 32 and terminates
	// with an additional byte 0x0D.
	orig_header.header_length = 32 + orig_header.num_fields*32 + 1;
	orig_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	for (int i=0; i<orig_header.num_fields; i++) {
		orig_header.length_each_record += col_data[i]->field_len;
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
	for (int i=0; i<header.num_fields; i++) {
		int mi = col_id_map[i];
		for (int j=0; j<32; j++) byte32_buff[j] = 0;
		strncpy(byte32_buff,
				(const char*)col_data[mi]->name.mb_str(wxConvUTF8), 11);
		switch (col_data[mi]->type) {
			case GeoDaConst::date_type:
				byte32_buff[11] = 'D';
				break;
			case GeoDaConst::long64_type:
				byte32_buff[11] = 'N';
				break;
			case GeoDaConst::double_type:
				byte32_buff[11] = 'N';
				break;
			default:
				byte32_buff[11] = 'C';
				break;
		}
		byte32_buff[16] = (wxUint8) col_data[mi]->field_len;
		byte32_buff[17] = (wxUint8) col_data[mi]->decimals;
		out_file.write(byte32_buff, 32);
	}
	delete [] byte32_buff;
	// mark end of field descriptors with 0x0D
	out_file.put((char) 0x0D);
	
	// Write out each record
	for (int row=0; row<header.num_records; row++) {
		out_file.put((char) 0x20); // each record starts with a space character
		for (int col=0; col<header.num_fields; col++) {
			int mcol = col_id_map[col];
			int f_len = col_data[mcol]->field_len;
			out_file.write(col_data[mcol]->raw_data[0] + row*(f_len+1), f_len);
		}
	}
	// 0x1A is the EOF marker
	out_file.put((char) 0x1A);
	out_file.close();
	changed_since_last_save = false;
	
	return true;
}

bool DbfGridTableBase::WriteToSpaceTimeDbf(const wxString& space_fname,
										   const wxString& time_fname,
										   wxString& err_msg)
{
	std::ofstream out_file_sp;
	out_file_sp.open(space_fname.mb_str(wxConvUTF8),
					 std::ios::out | std::ios::binary);
	if (!(out_file_sp.is_open() && out_file_sp.good())) {
		err_msg += "Problem opening \"" + space_fname + "\"";
		return false;
	}
	
	std::ofstream out_file_tm;
	out_file_tm.open(time_fname.mb_str(wxConvUTF8),
					 std::ios::out | std::ios::binary);
	if (!(out_file_tm.is_open() && out_file_tm.good())) {
		err_msg += "Problem opening \"" + time_fname + "\"";
		return false;
	}
	
	dbf_file_name_no_ext = wxFileName(space_fname).GetName();
	dbf_tm_file_name_no_ext = wxFileName(time_fname).GetName();
	
	// Ensure that raw_data exists.  If raw_data exists, then each item is
	// assumed to be ready for writing to disk.
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		if (!col_data[i]->IsRawDataAlloc()) col_data[i]->CopyVectorToRawData();
	}
	
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	// We must write the DBF in the current displayed column order
	std::vector<int> col_id_map;
	FillColIdMap(col_id_map);
	
	int sp_col_id = FindColId(sp_tbl_sp_col_name);
	
	// update orig_header
	orig_header.num_records = GetNumberRows();
	orig_header.num_fields = GetNumberColsSpace();
	// Each field descriptor is 32 bits and begins at byte 32 and terminates
	// with an additional byte 0x0D.
	orig_header.header_length = 32 + orig_header.num_fields*32 + 1;
	orig_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	// add in length of space field
	orig_header.length_each_record += col_data[sp_col_id]->field_len;
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		int mcol = col_id_map[i];
		if (mcol != sp_col_id && col_data[mcol]->time_steps == 1) {
			orig_header.length_each_record += col_data[mcol]->field_len;
		}
	}
	
	// update orig_header_tm
	orig_header_tm.num_records = GetNumberRows() * time_steps;
	orig_header_tm.num_fields = GetNumberColsTime() + 2;
	// Each field descriptor is 32 bits and begins at byte 32 and terminates
	// with an additional byte 0x0D.
	orig_header_tm.header_length = 32 + orig_header_tm.num_fields*32 + 1;
	orig_header_tm.length_each_record = 1; // first byte is either 0x20 or 0x2A
	// add in length of time and space fields
	orig_header_tm.length_each_record += col_data[sp_col_id]->field_len;
	orig_header_tm.length_each_record += GeoDaConst::default_dbf_long_len;
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		int mcol = col_id_map[i];
		if (mcol != sp_col_id && col_data[mcol]->time_steps > 1) {
			orig_header_tm.length_each_record += col_data[mcol]->field_len;
		}
	}
	
	wxUint32 u_int32;
	wxUint32* u_int32p = &u_int32;
	wxUint16 u_int16;
	wxUint16* u_int16p = &u_int16;
	wxUint8 u_int8;
	wxUint8* u_int8p = &u_int8;
	char membyte;
	
	// byte 0
	membyte = orig_header.version;
	out_file_sp.put(membyte);
	membyte = orig_header_tm.version;
	out_file_tm.put(membyte);
	
	// byte 1
	membyte = (char) (orig_header.year - 1900);
	out_file_sp.put(membyte);
	membyte = (char) (orig_header_tm.year - 1900);
	out_file_tm.put(membyte);
	
	// byte 2
	membyte = (char) orig_header.month;
	out_file_sp.put(membyte);
	membyte = (char) orig_header_tm.month;
	out_file_tm.put(membyte);
	
	// byte 3
	membyte = (char) orig_header.day;
	out_file_sp.put(membyte);
	membyte = (char) orig_header_tm.day;
	out_file_tm.put(membyte);
	
	// byte 4-7
	u_int32 = orig_header.num_records;
	u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
	out_file_sp.write((char*) u_int32p, 4);
	u_int32 = orig_header_tm.num_records;
	u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
	out_file_tm.write((char*) u_int32p, 4);	
	
	// byte 8-9
	u_int16 = orig_header.header_length;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file_sp.write((char*) u_int16p, 2);
	u_int16 = orig_header_tm.header_length;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file_tm.write((char*) u_int16p, 2);
	
	// byte 10-11
	u_int16 = orig_header.length_each_record;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file_sp.write((char*) u_int16p, 2);
	u_int16 = orig_header_tm.length_each_record;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file_tm.write((char*) u_int16p, 2);
	
	// byte 12-13 (0x0000)
	u_int16 = 0x0;
	out_file_sp.write((char*) u_int16p, 2);
	out_file_tm.write((char*) u_int16p, 2);
	
	// bytes 14-31: write 0 
	membyte = 0;
	for (int i=0; i<(31-14)+1; i++) out_file_sp.put(membyte);
	for (int i=0; i<(31-14)+1; i++) out_file_tm.put(membyte);
	
	// out_file_sp/tm now points to byte 32, which is the beginning of the list
	// of fields.  There must be at least one field.  Each field descriptor
	// is 32 bytes long with byte 0xd following the last field descriptor.
	char* byte32_buff = new char[32];
	
	// write out space id field to space and time DBFs
	for (int j=0; j<32; j++) byte32_buff[j] = 0;
	strncpy(byte32_buff,
			(const char*)col_data[sp_col_id]->name.mb_str(wxConvUTF8), 11);	
	byte32_buff[11] = 'N';
	byte32_buff[16] = (wxUint8) col_data[sp_col_id]->field_len;
	byte32_buff[17] = (wxUint8) col_data[sp_col_id]->decimals;
	out_file_sp.write(byte32_buff, 32);
	out_file_tm.write(byte32_buff, 32);
	// write out time id field to time DBF
	for (int j=0; j<32; j++) byte32_buff[j] = 0;
	strncpy(byte32_buff, (const char*)tm_tbl_tm_col_name.mb_str(wxConvUTF8),
			11);
	byte32_buff[11] = 'N';
	byte32_buff[16] = (wxUint8) GeoDaConst::default_dbf_long_len;
	byte32_buff[17] = (wxUint8) 0;
	out_file_tm.write(byte32_buff, 32);
	
	for (int i=0, iend=col_data.size(); i<iend; i++) {
		int mcol = col_id_map[i];
		for (int j=0; j<32; j++) byte32_buff[j] = 0;
		strncpy(byte32_buff,
				(const char*)col_data[mcol]->name.mb_str(wxConvUTF8), 11);
		switch (col_data[mcol]->type) {
			case GeoDaConst::date_type:
				byte32_buff[11] = 'D';
				break;
			case GeoDaConst::long64_type:
				byte32_buff[11] = 'N';
				break;
			case GeoDaConst::double_type:
				byte32_buff[11] = 'N';
				break;
			default:
				byte32_buff[11] = 'C';
				break;
		}
		byte32_buff[16] = (wxUint8) col_data[mcol]->field_len;
		byte32_buff[17] = (wxUint8) col_data[mcol]->decimals;
		if (mcol != sp_col_id) {
			if (col_data[mcol]->time_steps == 1) {
				out_file_sp.write(byte32_buff, 32);
			} else {
				out_file_tm.write(byte32_buff, 32);
			}
		}
	}
	
	// mark end of field descriptors with 0x0D
	out_file_sp.put((char) 0x0D);
	out_file_tm.put((char) 0x0D);
	
	// Write out each record to space DBF
	for (int row=0, row_end=GetNumberRows(); row<row_end; row++) {
		out_file_sp.put((char) 0x20); // each record starts with a space character
		// write out space id
		int f_len = col_data[sp_col_id]->field_len;
		out_file_sp.write(col_data[sp_col_id]->raw_data[0] + row*(f_len+1),
						  f_len);
		for (int col=0, col_end=GetNumberCols(); col<col_end; col++) {
			int mcol = col_id_map[col];
			if (mcol != sp_col_id && col_data[mcol]->time_steps == 1) {
				f_len = col_data[mcol]->field_len;
				out_file_sp.write(col_data[mcol]->raw_data[0]
								  + row*(f_len+1), f_len);
			}
		}
	}
	
	// Write out each record to time DBF
	for (int row=0, row_end=GetNumberRows(); row<row_end; row++) {
		for (int t=0, t_end=time_steps; t<t_end; t++) {
			 // each record starts with a space character
			out_file_tm.put((char) 0x20);
			// write out space id
			int f_len = col_data[sp_col_id]->field_len;
			out_file_tm.write(col_data[sp_col_id]->raw_data[0]
							  + row*(f_len+1), f_len);
			// write out time id
			f_len = GeoDaConst::default_dbf_long_len;
			for (int j=0; j<32; j++) byte32_buff[j] = 0;
			wxString tm_str;
			tm_str << time_ids[t];
			strncpy(byte32_buff, (const char*)tm_str.mb_str(wxConvUTF8),
					tm_str.size());
			out_file_tm.write(byte32_buff, f_len);
			for (int col=0, col_end=GetNumberCols(); col<col_end; col++) {
				int mcol = col_id_map[col];
				if (mcol != sp_col_id && col_data[mcol]->time_steps > 1) {
					f_len = col_data[mcol]->field_len;
					out_file_tm.write(col_data[mcol]->raw_data[t]
									  + row*(f_len+1), f_len);
				}
			}
		}
	}
	
	// 0x1A is the EOF marker
	out_file_sp.put((char) 0x1A);
	out_file_tm.put((char) 0x1A);
	out_file_sp.close();
	out_file_tm.close();
	changed_since_last_save = false;
	delete [] byte32_buff;
	
	return true;
}

bool DbfGridTableBase::IsSelected(int row)
{
	return hs[row];
}

void DbfGridTableBase::Select(int row)
{
	//LOG_MSG(wxString::Format("selecting %d", (int) row));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyHighlighted(0, row);
	highlight_state->SetTotalNewlyHighlighted(1);
	highlight_state->SetTotalNewlyUnhighlighted(0);
	highlight_state->notifyObservers();	
}

void DbfGridTableBase::Deselect(int row)
{
	//LOG_MSG(wxString::Format("deselecting %d", (int) row));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyUnhighlighted(0, row);
	highlight_state->SetTotalNewlyHighlighted(0);
	highlight_state->SetTotalNewlyUnhighlighted(1);
	highlight_state->notifyObservers();	
}

/** Only wxGrid should call this, others should use Selected(int row) */
bool DbfGridTableBase::FromGridIsSelected(int row)
{
	return hs[row_order[row]];
}

/** Only wxGrid should call this, others should use Select(int row) */
void DbfGridTableBase::FromGridSelect(int row)
{
	//LOG_MSG(wxString::Format("selecting %d", (int) row_order[row]));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyHighlighted(0, row_order[row]);
	highlight_state->SetTotalNewlyHighlighted(1);
	highlight_state->SetTotalNewlyUnhighlighted(0);
	highlight_state->notifyObservers();
}

/** Only wxGrid should call this, others should use Deselect(int row) */
void DbfGridTableBase::FromGridDeselect(int row)
{
	//LOG_MSG(wxString::Format("deselecting %d", (int) row_order[row]));
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetNewlyUnhighlighted(0, row_order[row]);
	highlight_state->SetTotalNewlyHighlighted(0);
	highlight_state->SetTotalNewlyUnhighlighted(1);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::SelectAll()
{
	int total_newly_selected = 0;
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	for (int i=0; i<hl_size; i++) {
		if (!hs[i]) nh[total_newly_selected++] = i;
	}
	highlight_state->SetEventType(HighlightState::delta);
	highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
	highlight_state->SetTotalNewlyUnhighlighted(0);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::DeselectAll()
{
	highlight_state->SetEventType(HighlightState::unhighlight_all);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::InvertSelection()
{
	highlight_state->SetEventType(HighlightState::invert);
	highlight_state->notifyObservers();
}

void DbfGridTableBase::SortByDefaultDecending()
{
	LOG_MSG("Calling DbfGridTableBase::SortByDefaultDecending");
	for (int i=0; i<rows; i++) {
		row_order[i] = i;
	}
	sorting_ascending = false;
	sorting_col = -1;
}

void DbfGridTableBase::SortByDefaultAscending()
{
	LOG_MSG("Calling DbfGridTableBase::SortByDefaultAscending");
	int last_ind = rows-1;
	for (int i=0; i<rows; i++) {
		row_order[i] = last_ind - i;
	}
	sorting_ascending = true;
	sorting_col = -1;
}


template <class T>
class index_pair
{
public:
	int index;
	T val;
	static bool less_than(const index_pair& i,
						   const index_pair& j) {
		return (i.val<j.val);
	}
	static bool greater_than (const index_pair& i,
							  const index_pair& j) {
		return (i.val>j.val);
	}
};

void DbfGridTableBase::SortByCol(int col, bool ascending)
{
	if (col == -1) {
		if (ascending) {
			SortByDefaultAscending();
		} else {
			SortByDefaultDecending();
		}
		return;
	}
	sorting_ascending = ascending;
	sorting_col = col;
	int rows = GetNumberRows();
	
	switch (col_data[col]->type) {
		case GeoDaConst::date_type:
		case GeoDaConst::long64_type:
		{
			std::vector<wxInt64> temp;
			col_data[col]->GetVec(temp);
			std::vector< index_pair<wxInt64> > sort_col(rows);
			for (int i=0; i<rows; i++) {
				sort_col[i].index = i;
				sort_col[i].val = temp[i];
			}
			if (ascending) {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxInt64>::less_than);
			} else {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxInt64>::greater_than);
			}
			for (int i=0, iend=rows; i<iend; i++) {
				row_order[i] = sort_col[i].index;
			}
		}
			break;
		case GeoDaConst::double_type:
		{
			std::vector<double> temp;
			col_data[col]->GetVec(temp);
			std::vector< index_pair<double> > sort_col(rows);
			for (int i=0; i<rows; i++) {
				sort_col[i].index = i;
				sort_col[i].val = temp[i];
			}
			if (ascending) {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<double>::less_than);
			} else {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<double>::greater_than);
			}
			for (int i=0, iend=rows; i<iend; i++) {
				row_order[i] = sort_col[i].index;
			}
		}
			break;
		case GeoDaConst::string_type:
		{
			std::vector<wxString> temp;
			col_data[col]->GetVec(temp);
			std::vector< index_pair<wxString> > sort_col(rows);
			for (int i=0; i<rows; i++) {
				sort_col[i].index = i;
				sort_col[i].val = temp[i];
			}
			if (ascending) {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxString>::less_than);
			} else {
				sort(sort_col.begin(), sort_col.end(),
					 index_pair<wxString>::greater_than);
			}
			for (int i=0, iend=rows; i<iend; i++) {
				row_order[i] = sort_col[i].index;
			}
		}
			break;
		default:
			break;
	}
}

void DbfGridTableBase::MoveSelectedToTop()
{
	LOG_MSG("Entering DbfGridTableBase::MoveSelectedToTop");
	std::set<int> sel_set;
	for (int i=0, iend=rows; i<iend; i++) {
		if (hs[row_order[i]]) sel_set.insert(row_order[i]);
	}
	int sel_row_offset = 0;
	int unsel_row_offset = sel_set.size();
	for (int i=0; i<rows; i++) {
		if (sel_set.find(i) != sel_set.end()) {
			row_order[sel_row_offset++] = i;
		} else {
			row_order[unsel_row_offset++] = i;
		}
	}
	sorting_col = -1;
	LOG_MSG("Exiting DbfGridTableBase::MoveSelectedToTop");	
}

void DbfGridTableBase::ReorderCols(const std::vector<int>& col_order)
{
	LOG_MSG("Entering DbfGridTableBase::ReorderCols");
	for (int i=0, iend=col_order.size(); i<iend; i++) {
		LOG(col_order[i]);
	}
	if (col_order.size() != col_data.size()) return;
	if (sorting_col != -1) sorting_col = col_order[sorting_col];
	std::vector<DbfColContainer*> orig(col_data);
	for (int i=0, iend=col_order.size(); i<iend; i++) {
		LOG_MSG(wxString::Format("col_data[%d] = orig[%d]",
								 col_order[i], i));
		col_data[col_order[i]] = orig[i];
	}
	for (int i=0, iend=col_order.size(); i<iend; i++) {
		LOG(col_data[i]->name);
	}
	changed_since_last_save = true;
	LOG_MSG("Entering DbfGridTableBase::ReorderCols");
}

/** If there is an associated wxGrid, then return the column ids in the order
 they are displayed in the table.  Otherwise, just return 0, 1, 2, ... The
 vector is automatically resized to col_data.size() 
 A mapping from displayed col order to actual col ids in table
 Eg, in underlying table, we might have A, B, C, D, E, F,
 but because of user wxGrid col reorder operaions might see these
 as C, B, A, F, D, E.  In this case, the col_map would be
 0->2, 1->1, 2->0, 3->5, 4->3, 5->4  */
void DbfGridTableBase::FillColIdMap(std::vector<int>& col_map)
{
	col_map.resize(col_data.size());
	wxGrid* grid = GetView();
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
void DbfGridTableBase::FillNumericColIdMap(std::vector<int>& col_map)
{
	std::vector<int> t;
	FillColIdMap(t);
	int numeric_cnt = 0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (col_data[t[i]]->type == GeoDaConst::long64_type ||
			col_data[t[i]]->type == GeoDaConst::double_type) {
			numeric_cnt++;
		}
	}
	col_map.resize(numeric_cnt);
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (col_data[t[i]]->type == GeoDaConst::long64_type ||
			col_data[t[i]]->type == GeoDaConst::double_type) {
			col_map[cnt++] = t[i];
		}
	}
}

/** Similar to FillColIdMap except this is a map of long64 type columns
 only.  The size of the resulting corresponds to the number of numeric
 columns */
void DbfGridTableBase::FillIntegerColIdMap(std::vector<int>& col_map)
{
	std::vector<int> t;
	FillColIdMap(t);
	int numeric_cnt = 0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (col_data[t[i]]->type == GeoDaConst::long64_type) {
			numeric_cnt++;
		}
	}
	col_map.resize(numeric_cnt);
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		if (col_data[t[i]]->type == GeoDaConst::long64_type) {
			col_map[cnt++] = t[i];
		}
	}
}

void DbfGridTableBase::FillNumericNameList(std::vector<wxString>& num_names)
{
	std::vector<int> t;
	FillNumericColIdMap(t);
	num_names.resize(t.size());
	int cnt=0;
	for (int i=0, iend=t.size(); i<iend; i++) {
		num_names[i] = col_data[t[i]]->name;
	}
}

bool DbfGridTableBase::IsSpaceTimeIdField(const wxString& name)
{
	wxString nm = name;
	nm.Trim(false);
	nm.Trim(true);
	return (nm.CmpNoCase(sp_tbl_sp_col_name) == 0 ||
			nm.CmpNoCase(tm_tbl_sp_col_name) == 0 ||
			nm.CmpNoCase(tm_tbl_tm_col_name) == 0);
}

bool DbfGridTableBase::InsertCol(int pos,
								 int time_steps, GeoDaConst::FieldType type,
								 const wxString& name,
								 int field_len, int decimals,
								 int displayed_decimals,
								 bool alloc_raw_data,
								 bool mark_all_defined)
{
	LOG_MSG(wxString::Format("Inserting column into table at postion %d", pos));
	if (pos < 0 || pos > col_data.size()) return false;
	std::vector<DbfColContainer*> orig(col_data);
	col_data.resize(orig.size()+1);
	for (int i=0; i<pos; i++) col_data[i] = orig[i];
	col_data[pos] = new DbfColContainer(this);
	if (type == GeoDaConst::date_type) {
		// will leave unitialized
		col_data[pos]->Init(rows, time_steps, type, name, field_len,
							decimals, decimals,
							alloc_raw_data, !alloc_raw_data, false);
		col_data[pos]->undefined_initialized = true;
	} else {
		col_data[pos]->Init(rows, time_steps, type, name, field_len,
							decimals, decimals,
							alloc_raw_data, !alloc_raw_data, mark_all_defined);
	}
	for (int i=pos; i<orig.size(); i++) col_data[i+1] = orig[i];
	if (pos <= sorting_col) sorting_col++;
	LOG(col_data.size());
	
	if (GetView()) {
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_INSERTED, pos, 1 );
		GetView()->ProcessTableMessage( msg );
		if (type == GeoDaConst::long64_type) {
			GetView()->SetColFormatNumber(pos);
		} else if (type == GeoDaConst::double_type) {
			GetView()->SetColFormatFloat(pos, -1,
						GenUtils::min<int>(decimals, displayed_decimals));
		} else {
			// leave as a string
		}
	}
	
	changed_since_last_save = true;
	table_state->notifyObservers();
	return true;
}

bool DbfGridTableBase::DeleteCol(int pos)
{
	LOG_MSG(wxString::Format("Deleting column from table at postion %d", pos));
	if (pos < 0 || pos >= col_data.size() || col_data.size() == 0) return false;
	std::vector<DbfColContainer*> orig(col_data);
	col_data.resize(orig.size()-1);
	for (int i=0; i<pos; i++) col_data[i] = orig[i];
	delete col_data[pos];
	for (int i=pos+1; i<orig.size(); i++) col_data[i-1] = orig[i];
	if (pos == sorting_col) { sorting_col = -1; sorting_ascending = true; }
	if (pos < sorting_col) sorting_col--;
	
	if (GetView()) {
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED, pos, 1 );
		GetView()->ProcessTableMessage( msg );
	}
	changed_since_last_save = true;
	table_state->notifyObservers();
	return true;
}

wxString DbfGridTableBase::GetRowLabelValue(int row)
{
	if (row<0 || row>=GetNumberRows()) return wxEmptyString;
	return wxString::Format("%d", row_order[row]+1);
}

wxString DbfGridTableBase::GetColLabelValue(int col)
{
	// \uXXXX are unicode characters for up and down arrows
	// \u0668 Arabic-Indic digit eight (up pointing arrow)
	// \u0667 Arabic-Indic digit seven (down pointing arrow)
	// \u25B5 white small up-pointing triangle (too big on OSX)
	// \u25BF white small down-pointing triangle
	// \u25B4 black small up-pointing triangle  (too big on OSX)
	// \u25BE black small down-pointing triangle
	// \u02C4, \u02C5 are up and down-pointing arrows
	// \u27F0 upward quadruple arrow
	// \u27F1 downward quadruble arrow

	if (col<0 || col>col_data.size()) return wxEmptyString;
		
	wxString label(col_data[col]->name);
	
	if (col_data[col]->time_steps > 1) {
		label << " (" << time_ids[curr_time_step] << ")";
	}
	
	if (col == sorting_col) {
		if (GeneralWxUtils::isMac()) {
			label << (sorting_ascending ? " \u02C4" : " \u02C5");
		} else if (GeneralWxUtils::isUnix()) {
			label << (sorting_ascending ? " \u25B5" : " \u25BF");
		} else {
			label << (sorting_ascending ? " >" : " <");
		}
	}
	return label;
}

