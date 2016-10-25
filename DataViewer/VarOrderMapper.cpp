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

#include <set>
#include <boost/foreach.hpp>
#include "../logger.h"
#include "VarOrderMapper.h"

VarOrderMapper::VarOrderMapper() : time_ids(1, "0")
{
}

VarOrderMapper::VarOrderMapper(const VarOrderMapper& vo)
: time_ids(vo.GetTimeIdsRef()), var_grps(vo.GetVarGroupsRef())
{
}

VarOrderMapper::VarOrderMapper(const VarOrderPtree& vo)
: time_ids(vo.GetTimeIdsRef()), var_grps(vo.GetVarGroupsRef())
{
}

VarOrderMapper::~VarOrderMapper()
{
}

void VarOrderMapper::Update(const VarOrderPtree& vo)
{
    time_ids = vo.GetTimeIdsRef();
    var_grps = vo.GetVarGroupsRef();
}

int VarOrderMapper::GetNumVarGroups() const
{
	return var_grps.size();
}

int VarOrderMapper::GetNumTms() const
{
	return time_ids.size();
}

const std::vector<wxString>& VarOrderMapper::GetTimeIdsRef() const
{
	return time_ids;
}

const VarGroup_container& VarOrderMapper::GetVarGroupsRef() const
{
	return var_grps;
}

/** Returns column number for given column.  Returns -1 if not found.  Note,
 does not search in Enteries.vars. */
int VarOrderMapper::GetColId(const wxString& name) const
{
	int cnt=0;
	BOOST_FOREACH(const VarGroup& e, var_grps) {
		if (e.name.CmpNoCase(name) == 0) return cnt;
        //else if (e.vars.size() > 0) {
        //    BOOST_FOREACH(const wxString& subname, e.vars)
        //        if (subname.CmpNoCase(name) == 0) return cnt;
        //}
        ++cnt;
	}
	return wxNOT_FOUND;
}

int VarOrderMapper::GetColIdx(const wxString& name) const
{
    int cnt=0;
    BOOST_FOREACH(const VarGroup& e, var_grps) {
        if (e.name.CmpNoCase(name) == 0) return cnt;
        else if (e.vars.size() > 0) {
            BOOST_FOREACH(const wxString& subname, e.vars)
                if (subname.CmpNoCase(name) == 0) return cnt;
        }
        ++cnt;
    }
    return wxNOT_FOUND;
}

/** Searches for name in VarGroup.  If VarGroup is simple, then returns
 true if name == VarGroup::name. If VarGroup not simple, then only returns
 true if name found in VarGroup vars.  col and tm are set to -1 when
 not found. */
bool VarOrderMapper::SimpleColNameToColAndTm(const wxString& name,
											 int& col, int& tm)
{
	col = -1;
	tm = -1;
	if (name.IsEmpty()) return false;
	int i=0;
	for (VarGroup_container::iterator vg_i=var_grps.begin();
		 vg_i != var_grps.end(); ++vg_i)
	{
		if (vg_i->IsSimple()) {
			if (vg_i->GetGroupName().CmpNoCase(name)==0) {
				col = i;
				tm = 0;
				return true;
			}
		} else {
			for (int j=0, sz=vg_i->vars.size(); j<sz; ++j) {
				if (vg_i->vars[j] == name) {
					col = i;
					tm = j;
					return true;
				}
			}
		}
		++i;
	}
	return false;
}

/** Returns a copy of VarGroup corresponding to name.  If not found, returns
 an empty VarGroup. */
VarGroup VarOrderMapper::FindVarGroup(const wxString& name) const
{
	/// MMM We should not be using CmpNoCase for no reason here.  Some
	/// DBs do support case-sensitive names and this logic would
	/// break that.
	BOOST_FOREACH(const VarGroup& e, var_grps) {
		if (e.name.CmpNoCase(name)==0) return e;
	}
	return VarGroup();
}

/** Returns a copy of VarGroup in corresponding position.  If out of range,
 returns an empty VarGroup. */
VarGroup VarOrderMapper::FindVarGroup(int i) const
{
	int cnt=0;
	BOOST_FOREACH(const VarGroup& e, var_grps) if (cnt++ == i) return e;
	return VarGroup();
}

VarGroup_container::iterator VarOrderMapper::FindVarGroupIt(
													const wxString& name)
{
	/// MMM We should not be using CmpNoCase for no reason here.  Some
	/// DBs do support case-sensitive names and this logic would
	/// break that.
	for (VarGroup_container::iterator i=var_grps.begin();
		 i!=var_grps.end(); ++i) {
		if (i->name.CmpNoCase(name)==0) return i;
	}
	return var_grps.end();
}

VarGroup_container::iterator VarOrderMapper::FindVarGroupIt(int j)
{
	int cnt=0;
	for (VarGroup_container::iterator i=var_grps.begin();
		 i!=var_grps.end(); ++i) {
		if (cnt++ == j) return i;
	}
	return var_grps.end();
}

/** Returns Database Column names, not group names.  If placeholder, then
 empty string returned.  Default time is 0.
 */
wxString VarOrderMapper::GetSimpleColName(int col, int time) const
{
	return FindVarGroup(col).GetNameByTime(time);
}

/** Returns group names, not underlying DB name (unless a simple group).
 If -1 or out of range, then empty string returned.
 */
wxString VarOrderMapper::GetGroupName(int col) const
{
	if (col < 0) return "";
	return FindVarGroup(col).GetGroupName();
}

/** Returns true if VarGroup fround with name for group or in simple vars */
bool VarOrderMapper::DoesNameExist(const wxString& name,
								   bool case_sensitive) const
{
	BOOST_FOREACH(const VarGroup& g, var_grps) {
		if (g.name.IsSameAs(name, case_sensitive)) return true;
		BOOST_FOREACH(const wxString& v, g.vars) {
			if (v.IsSameAs(name, case_sensitive)) return true;
		}
	}
	return false;
}

/** Works for groups or simple groups */
void VarOrderMapper::SetGroupName(int pos, const wxString& new_name)
{
	if (pos < 0 || pos >= var_grps.size()) return;
	VarGroup_container::iterator i = FindVarGroupIt(pos);
	i->SetGroupName(new_name);
}

void VarOrderMapper::SetSimpleColName(int pos, int time,
									  const wxString& new_name)
{
	if (time < 0 || pos < 0 || pos >= var_grps.size()) return;
	VarGroup_container::iterator i = FindVarGroupIt(pos);
	if (time >= i->GetNumTms()) return;
	i->SetVarName(new_name, time);
}

void VarOrderMapper::SetDisplayedDecimals(int pos, int disp_decs)
{
	if (pos < 0 || pos >= var_grps.size()) return;
	VarGroup_container::iterator i = FindVarGroupIt(pos);
	i->SetDispDecs(disp_decs);
}

/** Insert VarGroup at postion pos.  If pos >= var_grps.size(), insert
 at the end. */
void VarOrderMapper::InsertVarGroup(const VarGroup& e, int pos)
{
	LOG_MSG("Inside VarOrderMapper::InsertVarGroup");
	wxString msg;
	msg << "  inserting " << e.name << " at position " << pos;
	if (pos < 0) pos = 0;
	if (pos > var_grps.size()) pos = var_grps.size();
	VarGroup_container::iterator i = FindVarGroupIt(pos);
	var_grps.insert(i, e);
}

/** Remove VarGroup at postion pos. */
void VarOrderMapper::RemoveVarGroup(int pos)
{
	LOG_MSG("Inside VarOrderMapper::RemoveVarGroup");
	if (pos < 0 || pos >= var_grps.size()) return;
	VarGroup_container::iterator i = FindVarGroupIt(pos);
	wxString msg;
	msg << "  removing " << i->name << " at position " << pos;
	var_grps.erase(i);
}

/**
 * col_ids contains the positions of top-level columns that are to be
 * grouped into new column grp_name.  VarGroups refered to in cols will be
 * removed. Resulting grouped entry will be inserted into position grp_pos
 * in resulting table (with fewer columns).  Placeholders are indicated
 * with -1.
 * grp_pos is the desired position to insert the group into the table
 * after the grouping is finished (and there are therefore fewer columns)
 */
void VarOrderMapper::Group(const std::vector<int>& col_ids,
						  const wxString& grp_name, int grp_pos,
						  TableDeltaList_type& tdl)
{
	using namespace std;
	vector<wxString> cols;
	BOOST_FOREACH(int cid, col_ids) cols.push_back(GetGroupName(cid));
	// cols[i] with empty or not found names are assumed to refer to
	// placeholder var_grps.
	VarGroup new_e(grp_name);
	for (size_t c=0; c<cols.size(); c++) {
		if (cols[c] == "") {
			new_e.AppendPlaceholder();
		} else {
			VarGroup_container::iterator i = FindVarGroupIt(cols[c]);
			if (i == var_grps.end() || !i->IsSimple()) continue;
			TableDeltaEntry tde(i->name, false, GetColId(i->name));
			tdl.push_back(tde);
			new_e.Append(i->name);
			var_grps.erase(i);
		}
	}
	// insert new VarGroup at grp_pos
	if (grp_pos > var_grps.size()) grp_pos = var_grps.size();
	TableDeltaEntry tde(grp_name, true, grp_pos);
	tde.pos_final = grp_pos;
	tdl.push_back(tde);
	var_grps.insert(FindVarGroupIt(grp_pos), new_e);
}

/** Ungroup VarGroup at pos, deleting the VarGroup and inserting
 all non placeholder var_grps into the table at that position in
 order.
 */
void VarOrderMapper::Ungroup(int grp_pos, TableDeltaList_type& tdl)
{
	using namespace std;
	VarGroup e = FindVarGroup(grp_pos);
	if (e.IsSimple()) return;
	wxString grp_name = e.name;
	if (e.name == "") return;
	for (size_t i=0; i<e.vars.size(); ++i) {
		// insert in reverse order, one position after current grp_pos
		wxString name = e.vars[(e.vars.size()-1) - i];
		if (name == "") continue;
		VarGroup_container::iterator it = FindVarGroupIt(grp_name);
		++it;
		var_grps.insert(it, VarGroup(name));
		TableDeltaEntry tde(name, true, grp_pos+1);
		tde.pos_final = grp_pos+i;
		tdl.push_back(tde);
	}
	VarGroup_container::iterator it = FindVarGroupIt(grp_name);
	var_grps.erase(it);
	tdl.push_back(TableDeltaEntry(grp_name, false, grp_pos));
}

void VarOrderMapper::SwapTimes(int time1, int time2)
{
	if (time1 < 0 || time2 < 0 ||
		time1 >= time_ids.size() || time2 >= time_ids.size()) return;
	wxString tmp = time_ids[time1];
	time_ids[time1] = time_ids[time2];
	time_ids[time2] = tmp;

	for (VarGroup_container::iterator i=var_grps.begin();
		 i!=var_grps.end(); ++i) {
		if (i->IsSimple()) continue;
		if (i->vars.size() > time2 && i->vars.size() > time1) {
			wxString tmp = i->vars[time1];
			i->vars[time1] = i->vars[time2];
			i->vars[time2] = tmp;
		}
	}
}

/** First move through table in reverse and add simple columns from each
 * group. Secondly, do a cleanup pass where pure-placeholder groups
 * are removed from the table. */
void VarOrderMapper::RemoveTime(int time, TableDeltaList_type& tdl)
{
	if (time < 0 || time >= time_ids.size()) return;
	time_ids.erase(time_ids.begin() + time);
	
	
	// Add all all simple columns removed from groups
	for (int pos=GetNumVarGroups()-1; pos>=0; --pos) {
		VarGroup_container::iterator i = FindVarGroupIt(pos);
		if (i->IsSimple() || time >= i->GetNumTms()) continue;
		wxString vn = i->GetNameByTime(time);
		i->vars.erase(i->vars.begin() + time);
		if (vn == "") continue;
		// must insert as new simple variable into list
		InsertVarGroup(VarGroup(vn, i->GetDispDecs()), pos);
		TableDeltaEntry tde(vn, true, pos);
		tdl.push_back(tde);
	}
	
	// Remove all VarGroup with only placeholder entries remaining
	for (int pos=GetNumVarGroups()-1; pos>=0; --pos) {
		VarGroup_container::iterator i = FindVarGroupIt(pos);
		if (i->IsAllPlaceholders()) {
			TableDeltaEntry tde(i->GetGroupName(), false, pos);
			tdl.push_back(tde);
			wxString msg;
			msg << "  All placeholders group found: removing " << i->name;
			msg << " at position " << pos;
			var_grps.erase(i);
		}
	}
	
	// Must determine final positions of inserted columns and add this
	// info to tdl
	for (TableDeltaList_type::iterator i=tdl.begin(); i!=tdl.end(); ++i) {
		if (!i->insert) continue;
		i->pos_final = GetColId(i->group_name);
	}
}

void VarOrderMapper::InsertTime(int time, const wxString& new_time_id)
{
	if (time < 0 || time > time_ids.size()) return;
	time_ids.insert(time_ids.begin() + time, new_time_id);
	
	for (VarGroup_container::iterator i=var_grps.begin();
		 i!=var_grps.end(); ++i) {
		if (!i->IsSimple() && i->GetNumTms() >= time) {
			i->vars.insert(i->vars.begin() + time, "");
		}
	}
}

void VarOrderMapper::RenameTime(int time, const wxString& new_time_id)
{
	if (time < 0 || time >= time_ids.size()) return;
	time_ids[time] = new_time_id;
}

wxString VarOrderMapper::VarOrderToStr() const
{
	using namespace std;
	wxString ss;
	int col = 0;
	ss << "VarGroups_container:\n";
	BOOST_FOREACH(const VarGroup& e, var_grps) {
		if (e.vars.size() == 0) {
			ss << "col " << col << ": name: " << e.name;
		} else {
			ss << "col " << col << ": name: " << e.name << "\n";
			ss << "       vars: ";
			int t=0;
			BOOST_FOREACH(const wxString& v, e.vars) {
				if (v.empty()) { // a placeholder
					ss << "placeholder ";
				} else {
					ss << v << " ";
				}
				++t;
			}
		}
		ss << "\n";
		++col;
	}
	return ss;
}
