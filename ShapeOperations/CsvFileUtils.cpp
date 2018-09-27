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

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <wx/stopwatch.h>
#include "../logger.h"
#include "CsvFileUtils.h"

/** This method makes a row in the Excel CSV format.
 The following rules are followed:
 1. If the string contanis no , or " chars, then leave as is
 2. Otherwise, wrap " chars around the whole string and
    escape out " chars with a second ".
 Eg.  6" -> "6"""  or 1,2,3 -> "1,2,3"
 3. Each string chunk is seperated by , chars. */
void Gda::StringsToCsvRecord(const std::vector<std::string>& strings,
							   std::string& record)
{
	using namespace std;
	vector<string> escaped_strs(strings.size());
	
	for (int i=0, iend=strings.size(); i<iend; i++) {
		string item(strings[i]);
		if (item.find('\"') != string::npos || item.find(',') != string::npos) {
			ostringstream ss;
			for (int j=0, jend=item.size(); j<jend; j++) {
				if (item[j] == '"') {
					ss << "\"\"";
				} else {
					ss << item[j];
				}
			}
			escaped_strs[i] = "\"" + ss.str() + "\"";
		} else {
			escaped_strs[i] = item;
		}
	}
	
	ostringstream ss;
	for (int i=0, iend=escaped_strs.size(); i<iend; i++) {
		ss << escaped_strs[i];
		if (i < iend-1) ss << ",";
	}
	
	record = ss.str();
}

std::istream& Gda::safeGetline(std::istream& is, std::string& t)
{
    t.clear();
	
    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.
	
    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();
	
    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
			case '\r':
				c = sb->sgetc();
				if(c == '\n')
					sb->sbumpc();
				return is;
			case '\n':
			case EOF:
				return is;
			default:
				t += (char)c;
        }
    }
}

bool Gda::GetCsvStats(const std::string& csv_fname,
						int& num_rows, int& num_cols,
						std::vector<std::string>& first_row,
						wxString& err_msg)
{
	using namespace std;
	
	ifstream file(csv_fname.c_str());
	if (!file.is_open()) {
		err_msg << "Unable to open CSV file.";
		return false;
	}
	
	typedef Gda::csv_record_grammar<string::const_iterator> csv_rec_gram;
	csv_rec_gram csv_record; // CSV grammar instance
	using boost::spirit::ascii::space;
	
	string line;
	num_rows = 0;
	num_cols = 0;
	first_row.clear();
	bool done = false;
	bool blank_line_seen_once = false;
	
	// Parse the first line
	Gda::safeGetline(file, line);
	if (line.empty()) {
		err_msg << "First line of CSV is empty";
		file.close();
		return false;
	} else {
		string::const_iterator iter = line.begin();
		string::const_iterator end = line.end();
		bool r = phrase_parse(iter, end, csv_record, space, first_row);
		if (!r || iter != end) {
			err_msg << "Problem parsing first line of CSV.";
			file.close();
			return false;
		}
		num_cols = first_row.size();
		num_rows++;
	}
	
	// count remaining number of non-blank lines in file
	while ( !file.eof() && file.good() && !done ) {
		int pos = file.tellg();
		Gda::safeGetline(file, line);
		if (!line.empty()) num_rows++;
		if (pos == file.tellg()) done = true;
	}
	
	file.close();
	return true;
}

/** If first_row_field_names is true, then first row of data will be ignored
 */
bool Gda::FillStringTableFromCsv(const std::string& csv_fname,
								   std_str_array_type& string_table,
								   bool first_row_field_names,
								   wxString& err_msg)
{
	using namespace std;
	wxStopWatch sw;
	
	int num_rows = 0;
	int num_cols = 0;
	std::vector<std::string> first_row;
	wxString stats_err_msg;
	bool success = Gda::GetCsvStats(csv_fname, num_rows, num_cols, first_row,
									  stats_err_msg);
	if (!success) {
		err_msg = stats_err_msg;
		return false;
	}
	if (first_row_field_names) num_rows--;
	
	string_table.resize(boost::extents[num_rows][num_cols]);
	
	ifstream file(csv_fname.c_str());
	if (!file.is_open()) {
		cout << "Error: unable to open CSV file." << endl;
		return false;
	}
	
	vector<string> v;
	typedef Gda::csv_record_grammar<string::const_iterator> csv_rec_gram;
	csv_rec_gram csv_record; // CSV grammar instance
	using boost::spirit::ascii::space;
	
	int row = 0;
	string line;
	// skip first row if these are field names
	if (first_row_field_names) Gda::safeGetline(file, line);
	bool done = false;
	while ( !file.eof() && file.good() && !done && row < num_rows ) {
		int pos = file.tellg();
		Gda::safeGetline(file, line);
		if (!line.empty()) {
			v.clear();
			string::const_iterator iter = line.begin();
			string::const_iterator end = line.end();
			
			bool r = phrase_parse(iter, end, csv_record, space, v);
			if (!r || iter != end) {
				int line_no = row+1;
				if (first_row_field_names) line_no++;
				err_msg << "Problem parsing CSV file line " << line_no << ".";
				file.close();
				return false;
			}
			if (v.size() != num_cols) {
				err_msg << "First line of CSV file line has " << num_cols;
				err_msg << " fields, but line " << row << " has ";
				err_msg << v.size() << " fields.  This is not valid in ";
				err_msg << "a CSV file.";
				file.close();
				return false;
			}
			for (int col=0; col<num_cols; col++) {
				string_table[row][col] = v[col];
			}
			row++;
		}
		if (pos == file.tellg()) done = true;
	}
	file.close();
	if (row != num_rows) {
		err_msg << "CSV file was specified as having " << num_rows;
		err_msg << " records, but " << row << " records were parsed.";
		return false;
	}
	
	return true;
}

bool Gda::ConvertColToLongs(const std_str_array_type& string_table,
							  int col, std::vector<wxInt64>& v,
							  std::vector<bool>& undef,
							  int& failed_index)
{
	using namespace std;
	using boost::lexical_cast;
    using boost::bad_lexical_cast;
	
	int num_rows = string_table.shape()[0];
	int num_cols = string_table.shape()[1];
	if (col < 0 || col >= num_cols) return false;	
	v.resize(num_rows);
	undef.resize(num_rows);
	
	for (int i=0; i<num_rows; i++) {
		string s(string_table[i][col]);
		boost::trim(s);
		undef[i] = true;
		if (!s.empty()) {
			try {
				v[i] = lexical_cast<wxInt64>(s);
			} catch (bad_lexical_cast &) {
				failed_index = i;
				return false;
			}
			undef[i] = false;
		} else {
			v[i] = 0;
		}
	}
	return true;
}

bool Gda::ConvertColToDoubles(const std_str_array_type& string_table,
								int col, std::vector<double>& v,
								std::vector<bool>& undef,
								int& failed_index)
{
	using namespace std;
	using boost::lexical_cast;
    using boost::bad_lexical_cast;
	
	int num_rows = string_table.shape()[0];
	int num_cols = string_table.shape()[1];
	if (col < 0 || col >= num_cols) return false;	
	v.resize(num_rows);
	undef.resize(num_rows);
	
	for (int i=0; i<num_rows; i++) {
		string s(string_table[i][col]);
		boost::trim(s);
		undef[i] = s.empty();
		if (!s.empty()) {
			try {
				v[i] = lexical_cast<double>(s);
			} catch (bad_lexical_cast &) {
				failed_index = i;
				return false;
			}
		} else {
			v[i] = 0;
		}
	}
	return true;
}

