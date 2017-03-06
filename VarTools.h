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

#ifndef __GEODA_CENTER_VAR_TOOLS_H__
#define __GEODA_CENTER_VAR_TOOLS_H__

#include <limits>
#include <vector>
#include <wx/string.h>
#include "VarTools.h"

/**
 Design Notes for Variable Times
 
 Overview:
 Variables are either time-variant or invariant.  Each time-variant variable
 has an Boolean attribute sync_with_global that indicates if the time for that
 variable can change with global time.  If false, that variable is temporarily
 treated as a time-invariant variable.
 
 Assume that all variables are time-invariant and have sync_with_global set
 to true.  Each variable has a time attribute.  If all variables initially
 have the same time attribute, then they should all have the same value as
 global_time as global_time changes.  If they have different values, then
 the time range is non-zero and this range is maintained as global_time
 changes.
 
 We don't even want to have a global_time saved.  Rather, there will just
 be calls such as "SyncToGlobalTime(int global_time)" that will result
 in all variable times being updated.
 
 */

namespace GdaVarTools {
	
/*
Examples:
 
 First principle: keep minimal state info!
 Actual state info:
 global_time
 for each variable: time & is synch to global (only possible for tm-variant vars)
 
 Invariant: min over var times <= global_time <= max over times
 
 Calculating current possible time ranges is simply max-min
 Calculating a particular variable's lowest possible time is a matter of knowing
 it's offset from lowest.
 
 Private: GetOffSetFromLowest(): returns time offset from lowest.
 
 
 Tms: 50, 60, 70, 80, 90
 
 Say initally 
 v1 60, v2 80
 
 I think we should a "pull along" time concept.  So, in this case, if global
 time is 60, 70, or 80, v1 and v2 stay as they are.
 
 If global is 50, then v1 and v2 shift to 50, 70.
 If global is 90, then v1 and v2 shift to 70, 90.
 
 If v1 becomes time invariant, then v2 is now only variable it will syncronize
 to global exactly.
 
 So, ref_var should simply be the lowest time.
 All other variables should maintain a positive offset from that var.
 There can be more than one reference var if there is more than one
 lowest.
 
 In fact, no need to call any the "ref var", instead, just use convention
 that all "lowest" have 0 for offset.
 
 We can get a new "lowest" in two ways:
 1) the previous lowest, is now set to not synch and therefore the next lowest
 now becomes the lowest
 
 2) A variable with a current lower number is now told to synch again and
 is the new lowest.
 
*/

class Manager {
public:
	/** Create a new variable manager with given vector of global time strings.
	 If empty, or only one empty string given, then assumed to be only
	 one time period. */
	Manager();
	Manager(const std::vector<wxString>& tm_strs);
	Manager(const Manager& s);
	const Manager& operator=(const Manager& s);
	
	/** clears out current variables and reinits with new time strings.  If
	 default constructer was used, this must be called to initialize Manager. */
	void ClearAndInit(const std::vector<wxString>& tm_strs);
	
	/** Variables are added to the end of current list with this method.
	 Min/max values for all times must be given. At the end of all AppendVar
	 operations, should call UpdateGlobalTime() */
	void AppendVar(const wxString& name,
								 const std::vector<double>& min_vals,
								 const std::vector<double>& max_vals,
								 int time = 0, bool sync_with_global_time = true,
								 bool fixed_scale = true);
	
	/** Remove given var. If var is not at the end of the list, the
	 other variable ids will change.  Should call UpdateGlobalTime()
	 after removing a synched variables. */
	void RemoveVar(int var);
	/** Move var position forward by one (smaller value).
	 Returns true if success */
	bool MoveVarForwardOne(int var);
	/** Move var position backwards by one (larger value).
	 Returns true if success */
	bool MoveVarBackOne(int var);
	
	/** Given current constraints, what is the range of different
	 time ensembles for synched variables.  */
	int CurPossibleSynchedTmRange();
	bool IsAnyTimeVariant();
	bool IsAnySyncWithGlobalTm();
	wxString GetStrForTmId(int time);
	int GetVarsCount() { return vars.size(); }
	
	/** Get variable name string */
	wxString GetName(int var);
	int GetTime(int var);
	wxString GetNameWithTime(int var);
	bool IsTimeVariant(int var);
	bool IsSyncWithGlobalTm(int var);
	/** Should call UpdateGlobalTime() after changing synched state of
	 a variable, especially if sync is set to false. */
	void SetSyncWithGlobalTm(int var, bool sync);
	bool IsFixedScale(int var);
	void SetFixedScale(int var, bool fixed);
	
	/** Get minimum value over full range of global times */
	double GetMinOverAllTms(int var);
	/** Get maximum value over full range of global times */
	double GetMaxOverAllTms(int var);
	/** Get minimum value over full range of global times */
	double GetMinWithinPossibleTms(int var);
	double GetMaxWithinPossibleTms(int var);
	double GetMinCurTm(int var);	
	double GetMaxCurTm(int var);
	
	/** All variable's time attributes are upated according to constraints. */
	void UpdateGlobalTime(int tm);
	/** Get last value global time was set to.  Initially 0. */
	int GetLastGlobalTime();
	
private:
	int MinTmForAllSynced();
	int MaxTmForAllSynced();
	int OffsetFromMinSyncedTm(int var);
	
	struct Entry {
        Entry(const wxString& name, int time,
              bool is_time_variant, bool sync_with_global_time,
              bool fixed_scale,
              const std::vector<double>& min_vals,
              const std::vector<double>& max_vals);
		Entry(const Entry& e);
		virtual Entry& operator=(const Entry& s);
		
		wxString name;
		int time;
		bool is_time_variant;
		bool sync_with_global_time;
		bool fixed_scale;
		std::vector<double> min_vals; // min values for each time
		std::vector<double> max_vals; // max values for each time
	};
	static const double NaN;
	
	std::vector<Entry> vars;
	std::vector<wxString> tm_strs;
	int global_time; // according to last time it was set
};
	
struct VarInfo {
	VarInfo();
	
	// Primary Attributes
	wxString name;
	bool is_time_variant;
	int time; // current time, always between time_min and time_max
	std::vector<double> min; // min values for each time
	std::vector<double> max; // max values for each time
	std::vector<bool> has_undef; // max values for each time
	/* Keep synchronized with reference time.
	 * This only applies to time-variant variables.
	 * If false for a time-variant variable, then that variable
	 * is temporarily treated as a non-time-variant variable. */
	bool sync_with_global_time; 
	// change scale with each time-step if false, otherwise
	// scale is set according to min/max values over all possible times
	// for this particular variable combination.
	bool fixed_scale;
    
    bool is_moran; // moran requires |min| == |max|
	
	// Secondary Attributes
	
	// if true, then this variable time tries to match the the
	// global time and other variables time offsets are with respect
	// to this variable's time.  If true, then ref_time_offset = 0
	// Only one variable can be the reference variable for time
	bool is_ref_variable;
	// time offset from the reference variable time
	int ref_time_offset; // offset from ref_time
	int time_min;
	int time_max;
	double min_over_time; // within time min/max range
	double max_over_time; // within time min/max range
};

struct VarState {
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	int ref_var_index;
	int num_time_vals;
};
	

	
	
/** Returns new reference variable index, or else -1 if no reference
 variable */
int UpdateVarInfoSecondaryAttribs(std::vector<VarInfo>& var_info);

/** print info to log in debug mode */
void PrintVarInfoVector(std::vector<VarInfo>& var_info);

}

#endif
