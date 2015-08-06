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

#ifndef __GEODA_CENTER_VAR_GROUP_H__
#define __GEODA_CENTER_VAR_GROUP_H__

#include <list>
#include <vector>
#include <wx/string.h>

struct VarGroup;
typedef std::list<VarGroup> VarGroup_container;

struct VarGroup {
	VarGroup();
	VarGroup(const VarGroup& e);
	VarGroup(const wxString &name, int displayed_decimals=-1);
	VarGroup(const wxString &name, std::vector<wxString> vars,
			 int displayed_decimals=-1);
	bool operator<(const VarGroup& e) const;
	VarGroup& operator=(const VarGroup& e);
	void Append(const VarGroup& e);
	void AppendPlaceholder();
	bool IsAllPlaceholders() const;
	int GetNumTms() const;
	void GetVarNames(std::vector<wxString>& var_nms) const;
	wxString GetNameByTime(int time) const;
	wxString GetGroupName() const;
	/** renames the group */
	void SetGroupName(const wxString& new_name);
	/** rename simple var at given time */
	void SetVarName(const wxString& new_name, int time);
	bool IsSimple() const;
	bool IsEmpty() const;
	wxString ToStr() const;
	int GetDispDecs() const;
	void SetDispDecs(int displayed_decimals);
	
	/** simple name if only one time period, otherwise group name */
	wxString name;
	/** var: empty "" indicates placeholder
	 * if vars.size() == 0, then a simple variable */
	std::vector<wxString> vars;
	
	/** Number of displayed decimals for the entire group.  Only
	 used for non-integer numeric types.  -1 indicates not used
	 or use default for type. */
	int displayed_decimals;
};

#endif
