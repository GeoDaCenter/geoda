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
#include <boost/property_tree/ptree.hpp>
#include <wx/wx.h>

#include "../GdaException.h"
#include "../logger.h"
#include "VarOrderPtree.h"
#include "VarOrderPtree.h"

VarOrderPtree::VarOrderPtree() : time_ids(1, "time 0")
{
}

VarOrderPtree::VarOrderPtree(const VarOrderPtree& vo)
: time_ids(vo.time_ids), var_grps(vo.var_grps)
{
}

VarOrderPtree::VarOrderPtree(const boost::property_tree::ptree& pt,
							 const wxString& proj_path)
: time_ids(1, "time 0")
{
    ReadPtree(pt, proj_path);
}

VarOrderPtree::~VarOrderPtree()
{
}

VarOrderPtree* VarOrderPtree::Clone()
{
    return new VarOrderPtree(*this);
}

void VarOrderPtree::ReadPtree(const boost::property_tree::ptree& pt,
							  const wxString& proj_path)
{
	LOG_MSG("Entering VarOrderPtree::ReadPtree");
	using boost::property_tree::ptree;
    std::set<wxString> grp_set;
	try {
		try {
			pt.get_child("variable_order");
		}
		catch (boost::property_tree::ptree_bad_path& e) {
			// variable_order is optional
			return;
		}
		
		// iterate over each child of variable_order
		time_ids.clear();
		BOOST_FOREACH(const ptree::value_type &v,
					  pt.get_child("variable_order")) {
			wxString key = v.first.data();
			if (key == "var") {
				VarGroup ent;
				wxString tmp(v.second.data().c_str(), wxConvUTF8);
				ent.name = tmp.Trim().Trim(false);
                BOOST_FOREACH(const ptree::value_type &v, v.second) {
                    wxString tmp(v.first.data(), wxConvUTF8);
                    wxString key = tmp;
                    if (key == "displayed_decimals") {
                        wxString vs(v.second.data().c_str(), wxConvUTF8);
                        long dd;
                        if (!vs.ToLong(&dd)) dd = -1;
                        ent.displayed_decimals = dd;
                    }
                    if (key == "meta_data") {
                        BOOST_FOREACH(const ptree::value_type &mv, v.second) {
                            wxString mk(mv.first.data(), wxConvUTF8);
                            wxString ms(mv.second.data().c_str(), wxConvUTF8);
                            if (mk == "original_variable" ||
                                mk == "number_of_categories" ||
                                mk == "classification_type" ||
                                mk == "selection_range") {
                                ent.meta_data[mk] = ms;
                            } else if (mk == "categories") {
                                BOOST_FOREACH(const ptree::value_type &mcv, mv.second) {
                                    wxString mck(mcv.first.data(), wxConvUTF8);
                                    wxString mcs(mcv.second.data().c_str(), wxConvUTF8);
                                    mck = "categories." + mck;
                                    ent.meta_data[mck] = mcs;
                                }
                            }
                        }
                    }
                }
				var_grps.push_back(ent);
			} else if (key == "time_ids") {
				BOOST_FOREACH(const ptree::value_type &v, v.second) {
					wxString tmp(v.first.data(), wxConvUTF8);
					wxString key = tmp;
					wxString val(v.second.data().c_str(), wxConvUTF8);
					time_ids.push_back(val);
				}
			} else if (key == "group") {
                bool valid_group = true;
				VarGroup ent;
				BOOST_FOREACH(const ptree::value_type &v, v.second) {
					wxString tmp(v.first.data(), wxConvUTF8);
					wxString key = tmp;
					if (key == "name") {
						wxString tmp1(v.second.data().c_str(), wxConvUTF8);
						ent.name = tmp1;
					} else if (key == "var") {
						wxString tmp1(v.second.data().c_str(), wxConvUTF8);
						ent.vars.push_back(tmp1);
					} else if (key == "placeholder") {
						ent.vars.push_back("");
                        valid_group = false;
					} else if (key == "displayed_decimals") {
						wxString vs(v.second.data().c_str(), wxConvUTF8);
						long dd;
						if (!vs.ToLong(&dd)) dd = -1;
						ent.displayed_decimals = dd;
                    } else {
                        valid_group = false;
                    }
				}
				if (ent.name.empty()) {
                    wxString msg = _("space-time variable found with no name");
					throw GdaException(msg.mb_str());
				}
				if (grp_set.find(ent.name) != grp_set.end()) {
					wxString ss = _("Space-time variables with duplicate name \"%s\" found.");
                    ss = wxString::Format(ss, ent.name);
					throw GdaException(ss.mb_str());
				}
                if (valid_group) {
    				var_grps.push_back(ent);
    				grp_set.insert(ent.name);
                }
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
	if (time_ids.size() == 0) {
		time_ids.push_back("time 0"); // Insert a single default time
	}
}

void VarOrderPtree::WritePtree(boost::property_tree::ptree& pt,
							   const wxString& proj_path)
{
	using boost::property_tree::ptree;
	try {
		ptree& subtree = pt.put("variable_order", "");
		
		// Write time_ids
		BOOST_FOREACH(const wxString& s, time_ids) {
			subtree.add("time_ids.id", s);
		}
		
		// Write variables and groups
		BOOST_FOREACH(const VarGroup& e, var_grps) {
			if (e.vars.size() == 0) {				
				ptree& sstree = subtree.add("var", e.name);
                if (e.displayed_decimals != -1) {
                    wxString vs;
                    vs << e.displayed_decimals;
                    sstree.put("displayed_decimals", vs);
                }
                if (e.meta_data.empty() == false) {
                    std::map<wxString, wxString> meta_data = e.meta_data;
                    std::map<wxString, wxString>::iterator iter;
                    for (iter = meta_data.begin(); iter != meta_data.end();
                         ++iter) {
                        wxString meta_name = "meta_data.";
                        meta_name << iter->first;
                        wxString meta_value = iter->second;
                        sstree.add(meta_name.ToStdString(), meta_value);
                    }
                }
			} else {
				ptree& sstree = subtree.add("group", "");
				sstree.put("name", e.name);
            
				if (e.displayed_decimals != -1) {
					wxString vs;
					vs << e.displayed_decimals;
					sstree.put("displayed_decimals", vs);
				}
				BOOST_FOREACH(const wxString& v, e.vars) {
					if (v == "") {
						sstree.add("placeholder", "");
					} else {
						sstree.add("var", v);
					}
				}
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

const std::vector<wxString>& VarOrderPtree::GetTimeIdsRef() const
{
	return time_ids;
}

const VarGroup_container& VarOrderPtree::GetVarGroupsRef() const
{
	return var_grps;
}

bool VarOrderPtree::CorrectVarGroups(const std::vector<wxString>& ds_var_list,
                                     const std::vector<GdaConst::FieldType>& ds_var_type,
                                     bool case_sensitive)
{
	LOG_MSG("Entering VarOrderPtree::CorrectVarGroups");
	bool changed = false;

    std::set<wxString> ds_var_set;
	BOOST_FOREACH(const wxString &v, ds_var_list) {
        if (case_sensitive) ds_var_set.insert(v);
        else ds_var_set.insert(v.Lower());
    }
	
    std::set<wxString> var_set;
    std::set<wxString> group_nm_set;
	BOOST_FOREACH(const VarGroup& e, var_grps) {
		if (e.vars.size() == 0) {
			if (case_sensitive) var_set.insert(e.name);
            else var_set.insert(e.name.Lower());
		} else {
			if (case_sensitive)  group_nm_set.insert(e.name);
            else group_nm_set.insert(e.name.Lower());
			BOOST_FOREACH(const wxString& v, e.vars) {
                if (!v.empty()) {
                    if (case_sensitive) var_set.insert(v);
                    else var_set.insert(v.Lower());
                }
			}
		}
	}
	
	// Remove all items in var_set not in ds_var_set
	BOOST_FOREACH(const wxString& v, var_set) {
        if (ds_var_set.find(v) == ds_var_set.end()) {
			RemoveFromVarGroups(v, case_sensitive);
			changed = true;
		}
	}
	
	// Ensure all vars in each group have compatible types.  If not
	// compatible, ungroup and append to end.
    std::list<wxString> ungroup;
	for (VarGroup_container::iterator i=var_grps.begin(); i!=var_grps.end();) {
		if (!IsTypeCompatible(i->vars, ds_var_list, ds_var_type)) {
			BOOST_FOREACH(const wxString& v, i->vars) {
				ungroup.push_back(v);
			}
			VarGroup_container::iterator tmp = i;
			++i;
			var_grps.erase(tmp);
			changed = true;
		} else { ++i; }
	}
	BOOST_FOREACH(const wxString& v, ungroup) {
		VarGroup ent;
		ent.name = v;
		var_grps.push_back(ent);
	}
	
	// Search for any groups with only placeholders and remove them
    // todo: should add removed items to var_grps
	for (VarGroup_container::iterator i=var_grps.begin(); i!=var_grps.end();) {
		if (i->IsAllPlaceholders()) {
			VarGroup_container::iterator tmp = i;
			++i;
			var_grps.erase(tmp);
			changed = true;
		} else { ++i; }
	}
	
	// Append all items in ds_var_list not in var_set
    for (int i=0; i<ds_var_list.size(); ++i) {
        wxString v = case_sensitive ? ds_var_list[i] : ds_var_list[i].Lower();
		if (var_set.find(v) == var_set.end()) {
			VarGroup ent;
			ent.name = v;
			var_grps.push_back(ent);
		}
	}	
	
	LOG_MSG("Exiting VarOrderPtree::CorrectVarGroups");
	return changed;
}

/** Update VarOrderPtree based on meta-data from TableInterface
 */
void VarOrderPtree::ReInitFromTableInt(TableInterface* table)
{
    if (!table) return;
	
	var_grps.clear();
	time_ids.clear();
	
    std::vector<wxString> tm_strs;
	table->GetTimeStrings(tm_strs);
	int times = table->GetTimeSteps();
	int cols = table->GetNumberCols();
	this->time_ids.resize(times);
	for (int t=0; t<times; ++t) {
		this->time_ids[t] = tm_strs[t].ToStdString();
	}
    std::vector<int> col_map;
	table->FillColIdMap(col_map);
	for (int i=0; i<cols; ++i) {
		int col = col_map[i];
		VarGroup e;
		e.name = table->GetColName(col);
		if (table->IsColTimeVariant(col)) {
			e.vars.resize(times);
			for (int t=0; t<times; ++t) {
				if (table->GetColType(col, t) == GdaConst::placeholder_type) {
					e.vars[t] = "";
				} else {
					e.vars[t] = table->GetColName(col, t);
				}
			}
		}
		e.displayed_decimals = table->GetColDispDecimals(col);
        e.meta_data = table->GetMetaData(col);
		this->var_grps.push_back(e);
	}
}

wxString VarOrderPtree::VarOrderToStr() const
{
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


/// Remove var from var_grps if found.  Replace with placeholder
bool VarOrderPtree::RemoveFromVarGroups(const wxString& v,
                                        bool case_sensitive)
{
	if (v == "") return false;
	
	for (VarGroup_container::iterator i=var_grps.begin();
		 i!=var_grps.end(); ++i) {
		if (i->name.IsSameAs(v)) {
			var_grps.erase(i);
			return true;
		}
		for (std::vector<wxString>::iterator ii = i->vars.begin();
			 ii != i->vars.end(); ++ii) {
			if ((*ii).IsSameAs(v)) {
				i->vars.erase(ii);
				return true;
			}
		}
	}
	return false;
}

bool VarOrderPtree::IsTypeCompatible(const std::vector<wxString>& vars,
                                     const std::vector<wxString>& ds_var_list,
                                     const std::vector<GdaConst::FieldType>& ds_var_type)
{
	if (vars.size() == 0) return true;
    std::set<GdaConst::FieldType> type_set;
    
	BOOST_FOREACH(const wxString& v, vars) {
		if (!v.empty()) {
            GdaConst::FieldType type = GdaConst::unknown_type;
            for (size_t i=0; i<ds_var_list.size(); ++i) {
                if (ds_var_list[i].Cmp(v) == 0) {
                    type = ds_var_type[i];
                    break;
                }
            }
            if ( GdaConst::unknown_type == type) {
				wxString ss;
				ss << "Error: could not find type for var: " << v;
				return false;
			}
            if (type != GdaConst::placeholder_type) {
                type_set.insert(type);
            }
		}
	}
    return type_set.size() <= 1;
}

