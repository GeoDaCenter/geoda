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

#include <algorithm> // std::min_element, std::max_element
#include <limits>
#include "logger.h"
#include "VarTools.h"

using namespace GdaVarTools;

const double Manager::NaN = std::numeric_limits<double>::quiet_NaN();

Manager::Manager()
: global_time(0)
{
}

Manager::Manager(const std::vector<wxString>& tm_strs_)
: tm_strs(tm_strs_), global_time(0)
{
	if (tm_strs.size() == 0) tm_strs.push_back("");
}

Manager::Manager(const Manager& s)
: vars(s.vars), tm_strs(s.tm_strs), global_time(s.global_time)
{
}

const Manager& Manager::operator=(const Manager& s)
{
	vars = s.vars;
	tm_strs = s.tm_strs;
	global_time = s.global_time;
	return *this;
}

void Manager::ClearAndInit(const std::vector<wxString>& tm_strs_)
{
	vars.clear();
	tm_strs = tm_strs_;
	global_time = 0;
}

void Manager::AppendVar(const wxString& name,
						const std::vector<double>& min_vals,
						const std::vector<double>& max_vals,
						int time,
                        bool sync_with_global_time,
						bool fixed_scale)
{
	bool tm_variant = min_vals.size() > 1;
	Entry e(name, time, tm_variant, sync_with_global_time && tm_variant,
					fixed_scale, min_vals, max_vals);
	vars.push_back(e);
}

void Manager::RemoveVar(int var)
{
	if (var < 0 || var >= vars.size()) return;
	vars.erase(vars.begin()+var);
}

bool Manager::MoveVarForwardOne(int var)
{
	if (var <= 0 || var >= vars.size()) return false;
	Entry a = vars[var];
	Entry b = vars[var-1];
	vars[var] = b;
	vars[var-1] = a;
	return true;
}

bool Manager::MoveVarBackOne(int var)
{
	if (var < 0 || var+1 >= vars.size()) return false;
	Entry a = vars[var];
	Entry b = vars[var+1];
	vars[var] = b;
	vars[var+1] = a;
	return true;
}

int Manager::CurPossibleSynchedTmRange()
{
	int num_diff_tms = tm_strs.size() - 1;
	if (num_diff_tms < 0) num_diff_tms = 0;
	if (!IsAnySyncWithGlobalTm()) return num_diff_tms;
	int d = MaxTmForAllSynced() - MinTmForAllSynced();
	return num_diff_tms - d;
}

bool Manager::IsAnyTimeVariant()
{
	for (size_t i=0, sz=vars.size(); i<sz; ++i) {
		if (IsTimeVariant(i)) return true;
	}
	return false;
}

bool Manager::IsAnySyncWithGlobalTm()
{
	for (size_t i=0, sz=vars.size(); i<sz; ++i) {
		if (vars[i].sync_with_global_time) return true;
	}
	return false;	
}

wxString Manager::GetStrForTmId(int time)
{
	if (time < 0 || time >= tm_strs.size()) return "";
	return tm_strs[(size_t) time];
}

wxString Manager::GetName(int var)
{
	if (var < 0 || var >= vars.size()) return "";
	return vars[var].name;
}

int Manager::GetTime(int var)
{
	if (var < 0 || var >= vars.size()) return 0;
	return vars[var].time;
}

wxString Manager::GetNameWithTime(int var)
{
	if (var < 0 || var >= vars.size()) return "";
	wxString s(vars[var].name);
	if (!vars[var].is_time_variant) return s;
	s << " (" << GetStrForTmId(vars[var].time) << ")";
	return s;
}

bool Manager::IsTimeVariant(int var)
{
	if (var < 0 || var >= vars.size()) return false;
	return vars[var].is_time_variant;
}

bool Manager::IsSyncWithGlobalTm(int var)
{
	if (var < 0 || var >= vars.size() || !IsTimeVariant(var)) return false;
	return vars[var].sync_with_global_time;
}

void Manager::SetSyncWithGlobalTm(int var, bool sync)
{
	if (var < 0 || var >= vars.size()) return;
	vars[var].sync_with_global_time = sync;
}

bool Manager::IsFixedScale(int var)
{
	if (var < 0 || var >= vars.size()) return false;
	return vars[var].fixed_scale;
}

void Manager::SetFixedScale(int var, bool fixed)
{
	if (var < 0 || var >= vars.size()) return;
	vars[var].fixed_scale = fixed;
}

double Manager::GetMinOverAllTms(int var)
{
	using namespace std;
	if (var < 0 || var >= vars.size()) return NaN;
	vector<double>::iterator i = min_element(vars[var].min_vals.begin(),
                                             vars[var].min_vals.end());
	if (i == vars[var].min_vals.end()) return NaN;
	return (*i);
}

double Manager::GetMaxOverAllTms(int var)
{
	using namespace std;
	if (var < 0 || var >= vars.size()) return NaN;
	vector<double>::iterator i = max_element(vars[var].max_vals.begin(),
                                             vars[var].max_vals.end());
	if (i == vars[var].max_vals.end()) return NaN;
	return (*i);
}

double Manager::GetMinWithinPossibleTms(int var)
{
	using namespace std;
	if (var < 0 || var >= vars.size()) return NaN;
	if (!IsSyncWithGlobalTm(var)) return vars[var].min_vals[vars[var].time];
	int min_tm = OffsetFromMinSyncedTm(var);
	int max_tm = min_tm + CurPossibleSynchedTmRange();
	vector<double>::iterator i = min_element(vars[var].min_vals.begin()+min_tm,
                                             vars[var].min_vals.begin()+max_tm+1);
	if (i == vars[var].min_vals.end()) return NaN;
	return (*i);
}

double Manager::GetMaxWithinPossibleTms(int var)
{
	using namespace std;
	if (var < 0 || var >= vars.size()) return NaN;
	if (!IsSyncWithGlobalTm(var)) return vars[var].max_vals[vars[var].time];
	int min_tm = OffsetFromMinSyncedTm(var);
	int max_tm = min_tm + CurPossibleSynchedTmRange();
    
	vector<double>::iterator i = max_element(vars[var].max_vals.begin()+min_tm,
                                             vars[var].max_vals.begin()+max_tm+1);
	if (i == vars[var].max_vals.end()) return NaN;
	return (*i);
}

double Manager::GetMinCurTm(int var)
{
	using namespace std;
	if (var < 0 || var >= vars.size()) return NaN;
	return vars[var].min_vals[vars[var].time];
}

double Manager::GetMaxCurTm(int var)
{
	using namespace std;
	if (var < 0 || var >= vars.size()) return NaN;
	return vars[var].max_vals[vars[var].time];
}

void Manager::UpdateGlobalTime(int tm)
{
	global_time = tm;
	int delta = 0;
	if (global_time < MinTmForAllSynced()) {
		delta = global_time - MinTmForAllSynced();
	} else if (global_time > MaxTmForAllSynced()) {
		delta = global_time - MaxTmForAllSynced();
	}
	if (delta	!= 0) {
		for (size_t i=0, sz=vars.size(); i<sz; ++i) {
			if (vars[i].sync_with_global_time && vars[i].is_time_variant) {
				vars[i].time += delta;
			}
		}
	}
}

int Manager::GetLastGlobalTime()
{
	return global_time;
}

int Manager::OffsetFromMinSyncedTm(int var)
{
	if (var < 0 || var >= vars.size() || !IsSyncWithGlobalTm(var)) return 0;
	return vars[var].time - MinTmForAllSynced();
}

int Manager::MinTmForAllSynced()
{
	if (vars.size() == 0) return 0;
	int m = 0;
	bool found_any_synched = false;
	for (size_t i=0, sz=vars.size(); i<sz; ++i) {
		if (IsSyncWithGlobalTm(i) && (!found_any_synched || vars[i].time < m)) {
			m = vars[i].time;
			found_any_synched = true;
		}
	}
	return m;
}

int Manager::MaxTmForAllSynced()
{
	if (vars.size() == 0) return 0;
	int m = 0;
	bool found_any_synched = false;
	for (size_t i=0, sz=vars.size(); i<sz; ++i) {
		if (IsSyncWithGlobalTm(i) && (!found_any_synched || vars[i].time > m)) {
			m = vars[i].time;
			found_any_synched = true;
		}
	}
	return m;
}

Manager::Entry::Entry(const wxString& name_, int time_,
                      bool is_time_variant_,
                      bool sync_with_global_time_,
                      bool fixed_scale_,
                      const std::vector<double>& min_vals_,
                      const std::vector<double>& max_vals_)
: name(name_), time(time_), is_time_variant(is_time_variant_),
sync_with_global_time(sync_with_global_time_),
fixed_scale(fixed_scale_), min_vals(min_vals_), max_vals(max_vals_)
{
}

Manager::Entry::Entry(const Entry& s)
: name(s.name), time(s.time), is_time_variant(s.is_time_variant),
sync_with_global_time(s.sync_with_global_time),
fixed_scale(s.fixed_scale), min_vals(s.min_vals), max_vals(s.max_vals)
{
}

Manager::Entry& Manager::Entry::operator=(const Entry& s)
{
	name = s.name;
	time = s.time;
	is_time_variant = s.is_time_variant;
	sync_with_global_time = s.sync_with_global_time;
	fixed_scale = s.fixed_scale;
	min_vals = s.min_vals;
	max_vals = s.max_vals;
	return *this;
}	


VarInfo::VarInfo() : min(1, 0), max(1, 0)
{
	is_time_variant = false;
	time = 0;
	sync_with_global_time = true;
	fixed_scale = true;
    is_moran = false;
	is_ref_variable = false;
    
	time_min = 0;
	time_max = 0;
	min_over_time = 0;
	max_over_time = 0;
}


/* Sets all Secondary Attributes in GdaVarTools::VarInfo based
 on Primary Attributes.
 This method must be called whenever a Primary attribute of any item in the
 GdaVarTools::VarInfo vector changes. */
int GdaVarTools::UpdateVarInfoSecondaryAttribs(std::vector<VarInfo>& var_info)
{
	//PrintVarInfoVector(var_info);
	int num_vars = var_info.size();
	int ref_var = -1;
	for (int i=0; i<num_vars; i++) {
		if (ref_var == -1 && var_info[i].sync_with_global_time)
            ref_var = i;
		var_info[i].is_ref_variable = (i == ref_var);
		// The following parameters are set to default values here
		var_info[i].ref_time_offset = 0;
		var_info[i].time_min = var_info[i].time;
		var_info[i].time_max = var_info[i].time;
		var_info[i].min_over_time = var_info[i].min[var_info[i].time];
		var_info[i].max_over_time = var_info[i].max[var_info[i].time];
	}
	
	if (ref_var == -1)
        return ref_var;
	int ref_time = var_info[ref_var].time;
	int min_time = ref_time;
	int max_time = ref_time;
	for (int i=0; i<num_vars; i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].ref_time_offset = var_info[i].time - ref_time;
			if (var_info[i].time < min_time)
                min_time = var_info[i].time;
			if (var_info[i].time > max_time)
                max_time = var_info[i].time;
		}
	}
	int global_max_time = var_info[ref_var].max.size()-1;
	int min_ref_time = ref_time - min_time;
	int max_ref_time = global_max_time - (max_time - ref_time);
	for (int i=0; i<num_vars; i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time_min = min_ref_time + var_info[i].ref_time_offset;
			var_info[i].time_max = max_ref_time + var_info[i].ref_time_offset;
			for (int t=var_info[i].time_min; t<=var_info[i].time_max; t++) {
				if (var_info[i].min[t] < var_info[i].min_over_time) {
					var_info[i].min_over_time = var_info[i].min[t];
				}
				if (var_info[i].max[t] > var_info[i].max_over_time) {
					var_info[i].max_over_time = var_info[i].max[t];
				}
			}
		}
	}
	return ref_var;
}

void GdaVarTools::PrintVarInfoVector(std::vector<VarInfo>& var_info)
{
    /*
	LOG_MSG("Entering GdaVarTools::PrintVarInfoVector");
	LOG(var_info.size());
	for (int i=0; i<var_info.size(); i++) {
		LOG(var_info[i].sync_with_global_time);
		LOG(var_info[i].fixed_scale);
		
		LOG_MSG("Secondary Attributes:");
		LOG(var_info[i].is_ref_variable);
		LOG(var_info[i].ref_time_offset);
		LOG(var_info[i].time_min);
		LOG(var_info[i].time_max);
		LOG(var_info[i].min_over_time);
		LOG(var_info[i].max_over_time);
		LOG_MSG("\n");
	}
	LOG_MSG("Exiting GdaVarTools::PrintVarInfoVector");
     */
}
