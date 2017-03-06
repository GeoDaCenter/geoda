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
#include "GdaException.h"
#include "logger.h"
#include "DefaultVarsPtree.h"

wxString DefaultVar::ToStr() const
{
	wxString s;
	s << "Default Var Info:\n";
	s << "  name: " << name << "\n";
	if (!time_id.IsEmpty()) s << "  time_id: " << time_id << "\n";
	return s;
}

DefaultVarsPtree::DefaultVarsPtree()
{
}

DefaultVarsPtree::DefaultVarsPtree(const DefaultVarsPtree& o)
{
    std::list<DefaultVar>::const_iterator it;
    for (it = o.default_vars_list.begin(); it != o.default_vars_list.end(); ++it) {
        default_vars_list.push_back( *it );
    }
}

DefaultVarsPtree::DefaultVarsPtree(const boost::property_tree::ptree& pt,
								   const wxString& proj_path)
{
    ReadPtree(pt, proj_path);
}

DefaultVarsPtree::~DefaultVarsPtree()
{
}

DefaultVarsPtree* DefaultVarsPtree::Clone()
{
    return new DefaultVarsPtree(*this);
}

void DefaultVarsPtree::ReadPtree(const boost::property_tree::ptree& pt,
								 const wxString& proj_path)
{
	LOG_MSG("Entering DefaultVarsPtree::ReadPtree");
	using boost::property_tree::ptree;
	using namespace std;
	default_vars_list.clear();
	try {
		try {
			pt.get_child("default_vars");
		}
		catch (boost::property_tree::ptree_bad_path& e) {
			// spatial_weights is optional
			return;
		}
		
		// iterate over each child of default_vars
		BOOST_FOREACH(const ptree::value_type &v,
					  pt.get_child("default_vars")) {
			wxString key = v.first.data();
			if (key == "default_var") {
				DefaultVar dv;
				BOOST_FOREACH(const ptree::value_type &v, v.second) {
					wxString key = v.first.data();
					if (key == "name") {
						wxString s = v.second.data();
						dv.name = s;
					} else if (key == "time_id") {
						wxString s = v.second.data();
						dv.time_id = s;
					} else {
						wxString msg("Warning: unrecognized key: ");
						msg << key;
					}
				}
				default_vars_list.push_back(dv);
			} else {
				wxString msg("Warning: unrecognized key: ");
				msg << key;
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
	
	LOG_MSG("Exiting DefaultVarsPtree::ReadPtree");
}

void DefaultVarsPtree::WritePtree(boost::property_tree::ptree& pt,
								  const wxString& proj_path)
{
	using boost::property_tree::ptree;
	using namespace std;
	try {
		ptree& sub = pt.put("default_vars", "");

		// Write each spatial weights meta info definition
		BOOST_FOREACH(const DefaultVar& v, default_vars_list) {
			ptree& ssub = sub.add("default_var", "");
			ssub.put("name", v.name);
			if (!v.time_id.IsEmpty()) ssub.put("time_id", v.time_id);
		}	
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

const std::list<DefaultVar>& DefaultVarsPtree::GetDefaultVarList() const
{
	return default_vars_list;
}

void DefaultVarsPtree::SetDefaultVarList(const std::vector<wxString>& names,
										const std::vector<wxString>& time_ids)
{
	default_vars_list.clear();
	if (names.size() != time_ids.size()) return;
	for (int i=0, sz=names.size(); i<sz; ++i) {
		DefaultVar v;
		v.name = names[i];
		v.time_id = time_ids[i];
		default_vars_list.push_back(v);
	}
}

wxString DefaultVarsPtree::ToStr() const
{
	using namespace std;
	wxString s;
	BOOST_FOREACH(const DefaultVar& w, default_vars_list) s << w.ToStr();
	return s;
}
