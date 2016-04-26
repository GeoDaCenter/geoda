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

#include <boost/foreach.hpp>
#include "VarGroup.h"

VarGroup::VarGroup() : displayed_decimals(-1)
{
}

VarGroup::VarGroup(const VarGroup& e) : name(e.name), vars(e.vars),
displayed_decimals(e.displayed_decimals)
{
}

VarGroup::VarGroup(const wxString& _name, int _displayed_decimals)
: name(_name), displayed_decimals(_displayed_decimals)
{
}

VarGroup::VarGroup(const wxString& _name, std::vector<wxString> _vars,
				   int _displayed_decimals)
:  name(_name), vars(_vars), displayed_decimals(_displayed_decimals)
{
	if (vars.size() == 1) vars.clear();
}

bool VarGroup::operator<(const VarGroup& e) const
{
	return name < e.name;
}

VarGroup& VarGroup::operator=(const VarGroup& e)
{
	name = e.name;
	vars = e.vars;
	return *this;
}

void VarGroup::Append(const VarGroup& e)
{
	if (e.vars.size() == 0 && !e.name.IsEmpty()) {
		vars.push_back(e.name);
	} else {
		for (int i=0; i<e.vars.size(); ++i) vars.push_back(e.vars[i]);
	}
}

void VarGroup::AppendPlaceholder()
{
	vars.push_back("");
}

bool VarGroup::IsAllPlaceholders() const
{
	if (IsEmpty()) return true;
	if (IsSimple()) return false;
	BOOST_FOREACH(const wxString& v, vars) if (!v.IsEmpty()) return false;
	return true;
}

int VarGroup::GetNumTms() const
{
	if (IsEmpty()) return 0;
	return vars.size() == 0 ? 1 : vars.size();
}

/**
 * If IsSimple() true, then return just group name, otherwise
 * return vars.
 */
void VarGroup::GetVarNames(std::vector<wxString>& var_nms) const
{
	if (IsEmpty()) return;
	if (IsSimple()) {
		var_nms.resize(1);
		var_nms[0] = GetGroupName();
	} else {
		var_nms = vars;
	}
}

wxString VarGroup::GetNameByTime(int time) const
{
	if ( vars.size() == 0 ) return name;
	if ( time >= 0 && time < vars.size() ) return vars[time];
	return "";
}

wxString VarGroup::GetGroupName() const
{
	return name;
}

void VarGroup::SetGroupName(const wxString& new_name)
{
	name = new_name;
}

void VarGroup::SetVarName(const wxString& new_name, int time)
{
	if (time < 0 || vars.size() >= time) return;
	vars[time] = new_name;
}

bool VarGroup::IsSimple() const
{
	return vars.size() == 0;
}

/** If name is empty, then VarGroup is considered to be empty.  An empty
 VarGroup should only be used as a temporary return value to indicate
 that a VarGroup wasn't found. */
bool VarGroup::IsEmpty() const
{
	return name.IsEmpty();
}

wxString VarGroup::ToStr() const
{
	wxString ss;
	if (vars.size() == 0) {
		ss << "VarGroup name: " << name;
	} else {
		ss << "VarGroup name: " << name << "\n";
		ss << "      vars: ";
		BOOST_FOREACH(const wxString& v, vars) {
			if (v.empty()) { // a placeholder
				ss << "placeholder ";
			} else {
				ss << v << " ";
			}
		}
	}
	ss << "\n";
	return ss;
}

int VarGroup::GetDispDecs() const
{
	return displayed_decimals;
}

void VarGroup::SetDispDecs(int _displayed_decimals)
{
	displayed_decimals = _displayed_decimals;
}
