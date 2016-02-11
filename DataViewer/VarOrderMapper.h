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

#ifndef __GEODA_CENTER_VAR_ORDER_MAPPER_H__
#define __GEODA_CENTER_VAR_ORDER_MAPPER_H__

#include <vector>
#include "VarGroup.h"
#include "VarOrderPtree.h"

/**
 * Variables: simple or group. A group consists of simple variables.
 */
class VarOrderMapper {
public:
    VarOrderMapper();
	VarOrderMapper(const VarOrderMapper& vo);
	VarOrderMapper(const VarOrderPtree& vo);
    virtual ~VarOrderMapper();
	
    void Update(const VarOrderPtree& vo);
	int GetNumVarGroups() const;
	int GetNumTms() const;
	const std::vector<wxString>& GetTimeIdsRef() const;
	const VarGroup_container& GetVarGroupsRef() const;
	int GetColId(const wxString& name) const;
    int GetColIdx(const wxString& name) const;
	VarGroup FindVarGroup(const wxString& name) const;
	VarGroup FindVarGroup(int i) const;
	bool SimpleColNameToColAndTm(const wxString& name, int& col, int& tm);
	VarGroup_container::iterator FindVarGroupIt(const wxString& name);
	VarGroup_container::iterator FindVarGroupIt(int i);
	wxString GetSimpleColName(int col, int time=0) const;
	wxString GetGroupName(int col) const;
	bool DoesNameExist(const wxString& name, bool case_sensitive=false) const;
	void SetGroupName(int pos, const wxString& new_name);
	void SetSimpleColName(int pos, int time, const wxString& new_name);
	void SetDisplayedDecimals(int pos, int disp_decs);
	void InsertVarGroup(const VarGroup& e, int pos);
	void RemoveVarGroup(int pos);
	void Group(const std::vector<int>& col_ids,
			   const wxString& grp_name, int grp_pos,
			   TableDeltaList_type& tdl);
	void Ungroup(int grp_pos, TableDeltaList_type& tdl);
	void SwapTimes(int time1, int time2);
	void RemoveTime(int time, TableDeltaList_type& tdl);
	void InsertTime(int time, const wxString& new_time_id);
	void RenameTime(int time, const wxString& new_time_id);
	
	wxString VarOrderToStr() const;
	
private:
    std::vector<wxString> time_ids;
	VarGroup_container var_grps;
};

#endif
