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

#ifndef __GEODA_CENTER_GDA_JSON_H__
#define __GEODA_CENTER_GDA_JSON_H__

#include <vector>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_value.h>
#include <wx/string.h>
#include "DataViewer/TableInterface.h"

namespace GdaJson
{
	bool getBoolValFromObj(const json_spirit::Object& obj,
							   const wxString& name);
	wxString getStrValFromObj(const json_spirit::Object& obj,
							  const wxString& name);
	wxString getStrValFromObj(const json_spirit::Value& val,
							  const wxString& name);
	bool hasName(const json_spirit::Object& obj, const wxString& name);	
	bool hasName(const json_spirit::Value& val, const wxString& name);
	bool findValue(const json_spirit::Object& input,
				   json_spirit::Value& output,
				   const wxString& name);
	bool findValue(const json_spirit::Value& input,
				   json_spirit::Value& output,
				   const wxString& name);
	bool arrayToVec(const json_spirit::Array& js_array,
					std::vector<int>& output);
	void toValue(json_spirit::Value& v, const d_array_type& data);
	void toValue(json_spirit::Value& v, const d_array_type& data,
				 const b_array_type& undefined,
				 const std::vector<GdaConst::FieldType> types);
	void toValue(json_spirit::Value& v, const l_array_type& data);
	void toValue(json_spirit::Value& v, const l_array_type& data,
				 const b_array_type& undefined,
				 const std::vector<GdaConst::FieldType> types);
	void toValue(json_spirit::Value& v, const s_array_type& data);
	void toValue(json_spirit::Value& v, const s_array_type& data,
				 const b_array_type& undefined,
				 const std::vector<GdaConst::FieldType> types);
	void toValue(json_spirit::Value& v, const b_array_type& undefined);
	void toValue(json_spirit::Value& v, const std::vector<wxString>& strs);
	void toValue(json_spirit::Value& v, const std::vector<int>& vec);
	json_spirit::Pair toPair(const wxString& name, const wxString& val);
	json_spirit::Pair toPair(const wxString& name, int val);
	json_spirit::Pair toPair(const wxString& name, double val);
	json_spirit::Pair toPair(const wxString& name, bool val);
}

#endif
