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

#include "logger.h"
#include "GdaJson.h"

bool GdaJson::getBoolValFromObj(const json_spirit::Object& obj,
								const wxString& name)
{
	std::string std_name(name.ToStdString());
	for (json_spirit::Object::const_iterator i=obj.begin(); i!=obj.end(); ++i)
	{
		if (i->name_ == std_name) {
			if (i->value_.type() == json_spirit::bool_type) {
				return i->value_.get_bool();
			}
			return false;
		}
	}
	return false;
}


wxString GdaJson::getStrValFromObj(const json_spirit::Object& obj,
								   const wxString& name)
{
	std::string std_name(name.ToStdString());
	for (json_spirit::Object::const_iterator i=obj.begin(); i!=obj.end(); ++i)
	{
		if (i->name_ == std_name) {
			if (i->value_.type() == json_spirit::str_type) {
				return wxString(i->value_.get_str());
			}
			return "";
		}
	}
	return "";
}

wxString GdaJson::getStrValFromObj(const json_spirit::Value& val,
								   const wxString& name)
{
	if (val.type() != json_spirit::obj_type) return "";
	return getStrValFromObj(val.get_obj(), name);
}

bool GdaJson::hasName(const json_spirit::Object& obj, const wxString& name)
{
	std::string std_name(name.ToStdString());
	for (json_spirit::Object::const_iterator i=obj.begin(); i!=obj.end(); ++i)
	{
		if (i->name_ == std_name) return true;
	}
	return false;
}

bool GdaJson::hasName(const json_spirit::Value& val, const wxString& name)
{
	if (val.type() != json_spirit::obj_type) return false;
	return GdaJson::hasName(val.get_obj(), name);
}

bool GdaJson::findValue(const json_spirit::Object& input,
						json_spirit::Value& output,
						const wxString& name)
{
	std::string std_name(name.ToStdString());
	const json_spirit::Object o=input;
	for (json_spirit::Object::const_iterator i=o.begin(); i!=o.end(); ++i)
	{
		if (i->name_ == std_name) {
			output = i->value_;
			return true;
		}
	}
	return false;
}

bool GdaJson::findValue(const json_spirit::Value& input,
						json_spirit::Value& output,
						const wxString& name)
{
	if (input.type() != json_spirit::obj_type) return false;
	return GdaJson::findValue(input.get_obj(), output, name);
}

bool GdaJson::arrayToVec(const json_spirit::Array& js_array,
						 std::vector<int>& output)
{
	output.clear();
	try {
		for (size_t i=0, sz=js_array.size(); i<sz; ++i) {
			output.push_back(js_array[i].get_int());
		}
		return true;
	}
	catch (std::runtime_error e) {
		LOG_MSG("GdaJson::jsonArrayToVec (int): std::runtime_error");
	}
	return false;
}

void GdaJson::toValue(json_spirit::Value& v, const d_array_type& data)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		for (size_t i=0; i<num_obs; ++i) {
			a[i] = Value(data[t][i]);
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}

void GdaJson::toValue(json_spirit::Value& v, const d_array_type& data,
					  const b_array_type& undefined,
					  const std::vector<GdaConst::FieldType> types)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		if (types[t] == GdaConst::unknown_type || GdaConst::placeholder_type) {
			for (size_t i=0; i<num_obs; ++i) a[i] = Value();
		} else {
			for (size_t i=0; i<num_obs; ++i) {
				if (undefined[t][i]) {
					a[i] = Value(data[t][i]);
				} else {
					a[i] = Value();
				}
			}
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}

void GdaJson::toValue(json_spirit::Value& v, const l_array_type& data)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		for (size_t i=0; i<num_obs; ++i) {
			a[i] = Value((int64_t) data[t][i]);
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}

void GdaJson::toValue(json_spirit::Value& v, const l_array_type& data,
					  const b_array_type& undefined,
					  const std::vector<GdaConst::FieldType> types)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		if (types[t] == GdaConst::unknown_type || GdaConst::placeholder_type) {
			for (size_t i=0; i<num_obs; ++i) a[i] = Value();
		} else {
			for (size_t i=0; i<num_obs; ++i) {
				if (undefined[t][i]) {
					a[i] = Value((int64_t) data[t][i]);
				} else {
					a[i] = Value();
				}
			}
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}

void GdaJson::toValue(json_spirit::Value& v, const s_array_type& data)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		for (size_t i=0; i<num_obs; ++i) {
			a[i] = Value(data[t][i].ToStdString());
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}

void GdaJson::toValue(json_spirit::Value& v, const s_array_type& data,
					  const b_array_type& undefined,
					  const std::vector<GdaConst::FieldType> types)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		if (types[t] == GdaConst::unknown_type || GdaConst::placeholder_type) {
			for (size_t i=0; i<num_obs; ++i) a[i] = Value();
		} else {
			for (size_t i=0; i<num_obs; ++i) {
				if (undefined[t][i]) {
					a[i] = Value(data[t][i].ToStdString());
				} else {
					a[i] = Value();
				}
			}
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}


void GdaJson::toValue(json_spirit::Value& v, const b_array_type& data)
{
	using namespace json_spirit;
	size_t tms = data.shape()[0];
	size_t num_obs = data.shape()[1];
	Array top_array(tms);
	for (size_t t=0; t<tms; ++t) {
		Array a(num_obs);
		for (size_t i=0; i<num_obs; ++i) {
			a[i] = Value(data[t][i]);
		}
		top_array[t] = Value(a);
	}
	v = Value(top_array);
}

void GdaJson::toValue(json_spirit::Value& v, const std::vector<wxString>& strs)
{
	using namespace json_spirit;
	Array a;
	for (size_t i=0, sz=strs.size(); i<sz; ++i) {
		a.push_back(Value(strs[i].ToStdString()));
	}
	v = Value(a);
}

void GdaJson::toValue(json_spirit::Value& v, const std::vector<int>& vec)
{
	using namespace json_spirit;
	Array a;
	for (size_t i=0, sz=vec.size(); i<sz; ++i) {
		a.push_back(Value(vec[i]));
	}
	v = Value(a);
}

json_spirit::Pair GdaJson::toPair(const wxString& name, const wxString& val)
{
	return json_spirit::Pair(name.ToStdString(),
							 json_spirit::Value(val.ToStdString()));
}

json_spirit::Pair GdaJson::toPair(const wxString& name, int val)
{
	return json_spirit::Pair(name.ToStdString(),
							 json_spirit::Value(val));
}

json_spirit::Pair GdaJson::toPair(const wxString& name, double val)
{
	return json_spirit::Pair(name.ToStdString(),
							 json_spirit::Value(val));
}

json_spirit::Pair GdaJson::toPair(const wxString& name, bool val)
{
	return json_spirit::Pair(name.ToStdString(),
							 json_spirit::Value(val));
}
