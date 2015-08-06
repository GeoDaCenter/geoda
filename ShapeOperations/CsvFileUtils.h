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

#ifndef __GEODA_CENTER_CSV_FILE_UTILS_H__
#define __GEODA_CENTER_CSV_FILE_UTILS_H__

#include <iostream>
#include <string>
#include <vector>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/multi_array.hpp>
#include <wx/string.h>


typedef boost::multi_array<std::string, 2> std_str_array_type;

namespace Gda
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
	namespace phoenix = boost::phoenix;
	
    /// CSV field grammar definition
    
    template <typename Iterator>
	struct csv_record_grammar : qi::grammar<Iterator, std::vector<std::string>(),
	ascii::space_type>
    {
        csv_record_grammar() : csv_record_grammar::base_type(record)
        {
            using qi::lit;
            using qi::lexeme;
            using ascii::char_;
            using ascii::string;
			namespace ql = qi::labels;
			
			using phoenix::push_back;
			
			text = lexeme[*(char_ - (char_('\"') | char_(','))) [ql::_val += ql::_1]];
			non_escaped %= text;
			escaped = "\"" >> *( text [ql::_val += ql::_1]
								| char_(',') [ql::_val += ql::_1] 
								| string("\"\"") [ql::_val += "\""]
								) >> "\"";
			field %= (escaped | non_escaped);
			
			// Both of the following expression work, but we've chosen
			// to use the more explicit version for readability.
			//record = field >> *(',' >> field);
			record = field [push_back(ql::_val, ql::_1)] >> 
			*(char_(',') >> field [push_back(ql::_val, ql::_1)]);
        }
		
		qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> record;
        qi::rule<Iterator, std::string(), ascii::space_type> field;
        qi::rule<Iterator, std::string(), ascii::space_type> text;
        qi::rule<Iterator, std::string(), ascii::space_type> non_escaped;
		qi::rule<Iterator, std::string(), ascii::space_type> escaped;
    };
}

namespace Gda {
	void StringsToCsvRecord(const std::vector<std::string>& strings,
							std::string& record);
	std::istream& safeGetline(std::istream& is, std::string& t);
	bool GetCsvStats(const std::string& csv_fname, int& num_rows,
					 int& num_cols, std::vector<std::string>& first_row,
					 wxString& err_msg);
	bool FillStringTableFromCsv(const std::string& csv_fname,
								std_str_array_type& string_table,
								bool first_row_field_names,
								wxString& err_msg);
	bool ConvertColToLongs(const std_str_array_type& string_table,
						   int col, std::vector<wxInt64>& v,
						   std::vector<bool>& undef, int& failed_index);
	bool ConvertColToDoubles(const std_str_array_type& string_table,
							 int col, std::vector<double>& v,
							 std::vector<bool>& undef, int& failed_index);	
}

#endif
