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
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <wx/msgdlg.h>
#include "GalWeight.h"
#include "GwtWeight.h"
#include "../DataViewer/TableInterface.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "WeightUtils.h"

wxString WeightUtils::ReadIdField(const wxString& fname)
{
	using namespace std;
	wxString ext = GenUtils::GetFileExt(fname).Lower();
	if (ext != "gal" && ext != "gwt") return "";
	
	ifstream file;
	file.open(GET_ENCODED_FILENAME(fname), ios::in);  // a text file
	if (!(file.is_open() && file.good())) return "";
	
	// Header line is identical for GWT and GAL
	// First determine if header line is correct
	// Can be either: int int string string  (type n_obs filename field)
	// or : int (n_obs)
	string str;
	getline(file, str);
	stringstream ss (str, stringstream::in | stringstream::out);
		
	wxInt64 num1 = 0;
	wxInt64 num2 = 0;
	wxInt64 num_obs = 0;
	string dbf_name, t_key_field;
    
    
    string line;
    std::getline(ss, line);
    wxString header(line);
    
    // detect if header contains string with empty space, which should be quoted
    if (header.Contains("\"")) {
        int start_quote = header.find("\"");
        int end_quote = header.find("\"", start_quote + 1);
        dbf_name = header.SubString(start_quote + 1, end_quote - 1);
        t_key_field = header.SubString(end_quote + 1 + 1 /*1 for blank space */,
                                       header.length()-1);
        wxString nums = header.SubString(0, start_quote-1);
        int break_pos = nums.find(" ");
        wxString num1_str = nums.SubString(0, break_pos-1);
        wxString num2_str = nums.SubString(break_pos+1, nums.length()-1);
        num1_str.ToLongLong(&num1);
        num2_str.ToLongLong(&num2);
        
    } else {
    
        ss.clear();
        ss.seekg(0, ios::beg); // reset to beginning
        ss >> num1 >> num2 >> dbf_name >> t_key_field;
    }
    
    
	wxString key_field(t_key_field);
	if (num2 == 0) {
		key_field = "";
		num_obs = num1;
	} else {
		num_obs = num2;
	}
	
	file.clear();
	if (file.is_open()) file.close();
	
	return key_field;
}

GalElement* WeightUtils::ReadGal(const wxString& fname,
								 TableInterface* table_int)
{
	using namespace std;
	ifstream file;
	file.open(GET_ENCODED_FILENAME(fname), ios::in);  // a text file
	if (!(file.is_open() && file.good())) {
		return 0;
	}
	
	// First determine if header line is correct
	// Can be either: int int string string  (type n_obs filename field)
	// or : int (n_obs)
	
	int line_cnt = 0;
	bool use_rec_order = false;
	string str;
	getline(file, str);
	line_cnt++;
	stringstream ss (str, stringstream::in | stringstream::out);
	
	wxInt64 num1 = 0;
	wxInt64 num2 = 0;
	wxInt64 num_obs = 0;
	string dbf_name, t_key_field;
  
    string line;
    std::getline(ss, line);
    wxString header(line);
    
    // detect if header contains string with empty space, which should be quoted
    if (header.Contains("\"")) {
        int start_quote = header.find("\"");
        int end_quote = header.find("\"", start_quote + 1);
        dbf_name = header.SubString(start_quote + 1, end_quote - 1);
        t_key_field = header.SubString(end_quote + 1 + 1 /*1 for blank space */,
                                       header.length()-1);
        wxString nums = header.SubString(0, start_quote-1);
        int break_pos = nums.find(" ");
        wxString num1_str = nums.SubString(0, break_pos-1);
        wxString num2_str = nums.SubString(break_pos+1, nums.length()-1);
        num1_str.ToLongLong(&num1);
        num2_str.ToLongLong(&num2);
        
    } else {
        
        ss.clear();
        ss.seekg(0, ios::beg); // reset to beginning
        ss >> num1 >> num2 >> dbf_name >> t_key_field;
    }
    
    
	wxString key_field(t_key_field);
	if (num2 == 0) {
		use_rec_order = true;
		num_obs = num1;
	} else {
		num_obs = num2;
		if (key_field.IsEmpty() || key_field == "ogc_fid") {
			use_rec_order = true;
		}
	}
	
	if (table_int != NULL && num_obs != table_int->GetNumberRows()) {
		wxString msg = "The number of observations specified in chosen ";
		msg << "weights file is " << num_obs << ", but the number in the ";
		msg << "current Table is " << table_int->GetNumberRows();
		msg << ", which is incompatible.";
		wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return 0;
	}
	
	// Note: we want to be able to support blank lines.  If an observation
	// has no neighbors, then we'd like to be able to not include the
	// observation, or, if it is recorded, then the following line can
	// either be empty or blank.
    // note: we use wxString as key (convert int to string) for the case of any
    // string type numbers (e.g. the FIPS)
	map<wxString, int> id_map;
    
	if (use_rec_order) {
		// we need to traverse through every second line of the file and
		// record the max and min values.  So long as the max and min
		// values are such that num_obs = (max - min) + 1, we will assume
		// record order is valid.
		wxInt64 min_val = LLONG_MAX;
		wxInt64 max_val = LLONG_MIN;
		while (!file.eof()) {
			wxInt64 obs=0, num_neigh=0;
			// get next non-blank line
			str = "";
			while (str.empty() && !file.eof()) {
				getline(file, str);
				line_cnt++;
			}
			if (file.eof()) continue;
			{
				stringstream ss (str, stringstream::in | stringstream::out);
				ss >> obs >> num_neigh;
				if (obs < min_val) {
					min_val = obs;
				} else if (obs > max_val) {
					max_val = obs;
				}
			}
			if (num_neigh > 0) { // ignore the list of neighbors
				// get next non-blank line
				str = "";
				while (str.empty() && !file.eof()) {
					getline(file, str);
					line_cnt++;
				}
				if (file.eof()) continue;
			}
		}
		if (max_val - min_val != num_obs - 1) {
			wxString msg = "Record order specified, but found minimum";
			msg << " and maximum observation values of " << min_val;
			msg << " and " << max_val << " which is incompatible with";
			msg << " number of observations specified in first line of";
			msg << " weights file: " << num_obs << ".";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
        for (int i=0; i<num_obs; i++) {
            wxString iid;
            iid << i+min_val;
            id_map[ iid ] = i;
        }
        
	} else if ( table_int != NULL) {
		int col=0, tm=0;
        
		table_int->DbColNmToColAndTm(key_field, col, tm);
		if (col == wxNOT_FOUND) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file not found ";
			msg << "in currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		if (table_int->GetColType(col) != GdaConst::long64_type &&
            table_int->GetColType(col) != GdaConst::string_type ) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file is";
			msg << " not a number type in the currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		// get mapping from key_field to record ids (which always start
		// from 0 internally, but are displayed to the user from 1)
        if (table_int->GetColType(col) == GdaConst::long64_type) {
    		vector<wxInt64> vec;
    		table_int->GetColData(col, 0, vec);
    		for (int i=0; i<num_obs; i++) {
                wxString str_id;
                str_id << vec[i];
                id_map[ str_id ] = i;
            }
        }
        if (table_int->GetColType(col) == GdaConst::string_type) {
    		vector<wxString> vec;
    		table_int->GetColData(col, 0, vec);
    		for (int i=0; i<num_obs; i++) {
                id_map[ vec[i] ] = i;
            }
        }
		if (id_map.size() != num_obs) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" in weights file contains duplicate ";
			msg << "values in the currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
	}
	
	GalElement* gal = new GalElement[num_obs];
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	line_cnt = 0;
	getline(file, str); // skip header line
	line_cnt++;
	map<wxString, int>::iterator it;
	while (!file.eof()) {
		int gal_obs;
        std::string obs;
        wxInt64 num_neigh;
		// get next non-blank line
		str = "";
		while (str.empty() && !file.eof()) {
			getline(file, str);
			line_cnt++;
		}
		if (file.eof()) continue;
		{
			stringstream ss (str, stringstream::in | stringstream::out);
			ss >> obs >> num_neigh;
			it = id_map.find(wxString(obs));
			if (it == id_map.end()) {
				wxString msg = "On line ";
				msg << line_cnt << " of weights file, observation id " << obs;
				if (use_rec_order) {
					msg << " encountered which is out of allowed observation ";
					msg << "range of 1 through " << num_obs << ".";
				} else {
					msg << " encountered which does not exist in field \"";
					msg << key_field << "\" of the Table.";
				}
				wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				delete [] gal;
				return 0;
			}
			gal_obs = (*it).second; // value
			gal[gal_obs].SetSizeNbrs(num_neigh);
		}
		if (num_neigh > 0) { // skip next if no neighbors
			// get next non-blank line
			str = "";
			while (str.empty() && !file.eof()) {
				getline(file, str);
				line_cnt++;
			}
			if (file.eof()) continue;
			{
				stringstream ss (str, stringstream::in | stringstream::out);
				for (int j=0; j<num_neigh; j++) {
					string neigh;
					ss >> neigh;
					it = id_map.find(wxString(neigh));
					if (it == id_map.end()) {
						wxString msg = "On line ";
						msg << line_cnt << " of weights file, observation id ";
						msg << neigh;
						if (use_rec_order) {
							msg << " encountered which is out of allowed ";
							msg << "observation ";
							msg << "range of 1 through " << num_obs << ".";
						} else {
							msg << " encountered which does not exist ";
							msg << "in field \"" << key_field;
							msg << "\" of the Table.";
						}
						wxMessageDialog dlg(NULL, msg, "Error",
											wxOK|wxICON_ERROR);
						dlg.ShowModal();
						delete [] gal;
						return 0;
					}
					long n_id = (*it).second; // value of id_map[neigh];
					gal[gal_obs].SetNbr(j, n_id);
				}
			}
		}
	}	
	
	file.clear();
	if (file.is_open()) file.close();
	
	return gal;
}



GalElement* WeightUtils::ReadGwtAsGal(const wxString& fname,
									  TableInterface* table_int)
{
	using namespace std;
	ifstream file;
	file.open(GET_ENCODED_FILENAME(fname), ios::in);  // a text file
	if (!(file.is_open() && file.good())) {
		return 0;
	}
	
	// First determine if header line is correct
	// Can be either: int int string string  (type n_obs filename field)
	// or : int (n_obs)
	
	bool use_rec_order = false;
	string str;
	getline(file, str);
	cout << str << endl;
	stringstream ss(str, stringstream::in | stringstream::out);
	
	wxInt64 num1 = 0;
	wxInt64 num2 = 0;
	wxInt64 num_obs = 0;	
	string dbf_name, t_key_field;
    
    string line;
    std::getline(ss, line);
    wxString header(line);
    
    // detect if header contains string with empty space, which should be quoted
    if (header.Contains("\"")) {
        int start_quote = header.find("\"");
        int end_quote = header.find("\"", start_quote + 1);
        dbf_name = header.SubString(start_quote + 1, end_quote - 1);
        t_key_field = header.SubString(end_quote + 1 + 1 /*1 for blank space */,
                                       header.length()-1);
        wxString nums = header.SubString(0, start_quote-1);
        int break_pos = nums.find(" ");
        wxString num1_str = nums.SubString(0, break_pos-1);
        wxString num2_str = nums.SubString(break_pos+1, nums.length()-1);
        num1_str.ToLongLong(&num1);
        num2_str.ToLongLong(&num2);
        
    } else {
        
        ss.clear();
        ss.seekg(0, ios::beg); // reset to beginning
        ss >> num1 >> num2 >> dbf_name >> t_key_field;
    }
    
	wxString key_field(t_key_field);
	if (num2 == 0) {
		use_rec_order = true;
		num_obs = num1;
	} else {
		num_obs = num2;
		if (key_field.IsEmpty() || key_field == "ogc_fid") {
			use_rec_order = true;
		}
	}
	
	if (table_int != NULL && num_obs != table_int->GetNumberRows()) {
		wxString msg = "The number of observations specified in chosen ";
		msg << "weights file is " << num_obs << ", but the number in the ";
		msg << "current Table is " << table_int->GetNumberRows();
		msg << ", which is incompatible.";
		wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return 0;
	}
	
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxString, int> id_map;
	if (use_rec_order) {
		// we need to traverse through every line of the file and
		// record the max and min values.  So long as the max and min
		// values are such that num_obs = (max - min) + 1, we will assume
		// record order is valid.
		wxInt64 min_val = LLONG_MAX;
		wxInt64 max_val = LLONG_MIN;
		while (!file.eof()) {
			wxInt64 obs1=0, obs2=0;
			getline(file, str);
			if (!str.empty()) {
				stringstream ss (str, stringstream::in | stringstream::out);
				ss >> obs1 >> obs2;
				if (obs1 < min_val) {
					min_val = obs1;
				} else if (obs1 > max_val) {
					max_val = obs1;
				}
				if (obs2 < min_val) {
					min_val = obs2;
				} else if (obs2 > max_val) {
					max_val = obs2;
				}
			}
		}
		if (max_val - min_val != num_obs - 1) {
			wxString msg = "Record order specified, but found minimum";
			msg << " and maximum observation values of " << min_val;
			msg << " and " << max_val << " which is incompatible with";
			msg << " number of observations specified in first line of";
			msg << " weights file: " << num_obs << ".";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		for (int i=0; i<num_obs; i++) {
                    wxString iid;
                    iid << i+min_val;
                    id_map[ iid ] = i;
                }
        
	} else if (table_int != NULL) {
		int col, tm;
		table_int->DbColNmToColAndTm(key_field, col, tm);
		if (col == wxNOT_FOUND) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file not found ";
			msg << "in currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		if (table_int->GetColType(col) != GdaConst::long64_type &&
            table_int->GetColType(col) != GdaConst::string_type) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file is";
			msg << " not an integer type in the currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		// get mapping from key_field to record ids (which always start
		// from 0 internally, but are displayed to the user from 1)
        if (table_int->GetColType(col) == GdaConst::long64_type) {
    		vector<wxInt64> vec;
    		table_int->GetColData(col, 0, vec);
    		for (int i=0; i<num_obs; i++) {
                wxString str_id;
                str_id << vec[i];
                id_map[ str_id ] = i;
            }
        }
        if (table_int->GetColType(col) == GdaConst::string_type) {
    		vector<wxString> vec;
    		table_int->GetColData(col, 0, vec);
    		for (int i=0; i<num_obs; i++) {
                id_map[ vec[i] ] = i;
            }
        }

		if (id_map.size() != num_obs) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" in weights file contains duplicate ";
			msg << "values in the currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
	}
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	// we need to traverse through every line of the file and
	// record the number of neighbors for each observation.
	map<wxString, set<wxString> >::iterator it;
	map<wxString, set<wxString> > nbr_histogram;
	while (!file.eof()) {
		string obs1, obs2;
		getline(file, str);
		if (!str.empty()) {
			stringstream ss (str, stringstream::in | stringstream::out);
            ss >> obs1 >> obs2;
			it = nbr_histogram.find(wxString(obs1));
			if (it == nbr_histogram.end()) {
                set<wxString> s;
				nbr_histogram[obs1] = s;
			}
            if (obs2 != obs1)
                nbr_histogram[obs1].insert(obs2);
		}
	}
	
	vector<size_t> gal_cnt(num_obs, 0);
	GalElement* gal = new GalElement[num_obs];
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxString, int>::iterator it1;
	map<wxString, int>::iterator it2;
	int line_num=1;
	while (!file.eof()) {
		int gwt_obs1, gwt_obs2;
		//wxInt64 obs1, obs2;
        string obs1, obs2;
        double wVal;
		getline(file, str);
		if (!str.empty()) {
			stringstream ss(str, stringstream::in | stringstream::out);
			ss >> obs1 >> obs2 >> wVal;
			it1 = id_map.find(obs1);
			it2 = id_map.find(obs2);
			if (it1 == id_map.end() || it2 == id_map.end()) {
				string obs;
				if (it1 == id_map.end())
                    obs = obs1;
				if (it2 == id_map.end())
                    obs = obs2;
				wxString msg = "On line ";
				msg << line_num+1 << " of weights file, observation id " << obs;
				if (use_rec_order) {
					msg << " encountered which is out of allowed observation ";
					msg << "range of 1 through " << num_obs << ".";
				} else {
					msg << " encountered which does not exist in field \"";
					msg << key_field << "\" of the Table.";
				}
				wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				delete [] gal;
				return 0;
			}
			
			gwt_obs1 = (*it1).second; // value
			gwt_obs2 = (*it2).second; // value
			if (gal[gwt_obs1].Size() == 0) {
				gal[gwt_obs1].SetSizeNbrs(nbr_histogram[obs1].size());
			}
            if (obs2 != obs1) {
                gal[gwt_obs1].SetNbr(gal_cnt[gwt_obs1]++, gwt_obs2, wVal);
            }
		}
		line_num++;
	}	
	
	file.clear();
	if (file.is_open()) file.close();
	
	return gal;
}

/** This function should not be used unless an actual GWT object is needed
 internally.  In most cases, the ReadGwtAsGal function should be used */
GwtElement* WeightUtils::ReadGwt(const wxString& fname,
								 TableInterface* table_int)
{
	using namespace std;
	ifstream file;
	file.open(GET_ENCODED_FILENAME(fname), ios::in);  // a text file
	if (!(file.is_open() && file.good())) {
		return 0;
	}
	
	// First determine if header line is correct
	// Can be either: int int string string  (type n_obs filename field)
	// or : int (n_obs)
	
	bool use_rec_order = false;
	string str;
	getline(file, str);
	cout << str << endl;
	stringstream ss(str, stringstream::in | stringstream::out);
	
	wxInt64 num1 = 0;
	wxInt64 num2 = 0;
	wxInt64 num_obs = 0;	
	string dbf_name, t_key_field;
    
    string line;
    std::getline(ss, line);
    wxString header(line);
    
    // detect if header contains string with empty space, which should be quoted
    if (header.Contains("\"")) {
        int start_quote = header.find("\"");
        int end_quote = header.find("\"", start_quote + 1);
        dbf_name = header.SubString(start_quote + 1, end_quote - 1);
        t_key_field = header.SubString(end_quote + 1 + 1 /*1 for blank space */,
                                       header.length()-1);
        wxString nums = header.SubString(0, start_quote-1);
        int break_pos = nums.find(" ");
        wxString num1_str = nums.SubString(0, break_pos-1);
        wxString num2_str = nums.SubString(break_pos+1, nums.length()-1);
        num1_str.ToLongLong(&num1);
        num2_str.ToLongLong(&num2);
        
    } else {
        
        ss.clear();
        ss.seekg(0, ios::beg); // reset to beginning
        ss >> num1 >> num2 >> dbf_name >> t_key_field;
    }
    
	wxString key_field(t_key_field);
	if (num2 == 0) {
		use_rec_order = true;
		num_obs = num1;
	} else {
		num_obs = num2;
		if (key_field.IsEmpty()) {
			use_rec_order = true;
		}
	}
	
	if (num_obs != table_int->GetNumberRows()) {
		wxString msg = "The number of observations specified in chosen ";
		msg << "weights file is " << num_obs << ", but the number in the ";
		msg << "current Table is " << table_int->GetNumberRows();
		msg << ", which is incompatible.";
		wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return 0;
	}
	
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxInt64, int> id_map;
	if (use_rec_order) {
		// we need to traverse through every line of the file and
		// record the max and min values.  So long as the max and min
		// values are such that num_obs = (max - min) + 1, we will assume
		// record order is valid.
		wxInt64 min_val = LLONG_MAX;
		wxInt64 max_val = LLONG_MIN;
		while (!file.eof()) {
			wxInt64 obs1=0, obs2=0;
			getline(file, str);
			if (!str.empty()) {
				stringstream ss (str, stringstream::in | stringstream::out);
				ss >> obs1 >> obs2;
				if (obs1 < min_val) {
					min_val = obs1;
				} else if (obs1 > max_val) {
					max_val = obs1;
				}
				if (obs2 < min_val) {
					min_val = obs2;
				} else if (obs2 > max_val) {
					max_val = obs2;
				}
			}
		}
		if (max_val - min_val != num_obs - 1) {
			wxString msg = "Record order specified, but found minimum ";
			msg << " and maximum observation values of " << min_val;
			msg << " and " << max_val << " which is incompatible with ";
			msg << " number of observations specified in first line of ";
			msg << " weights file: " << num_obs << ".";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		for (int i=0; i<num_obs; i++) id_map[i+min_val] = i;
	} else {
		int col, tm;
		table_int->DbColNmToColAndTm(key_field, col, tm);
		if (col == wxNOT_FOUND) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file not found ";
			msg << "in currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		if (table_int->GetColType(col) != GdaConst::long64_type) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file is ";
			msg << " not an integer type in the currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		// get mapping from key_field to record ids (which always start
		// from 0 internally, but are displayed to the user from 1)
		vector<wxInt64> vec;
		table_int->GetColData(col, 0, vec);
		for (int i=0; i<num_obs; i++) id_map[vec[i]] = i;
		if (id_map.size() != num_obs) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" in weights file contains duplicate ";
			msg << "values in the currently loaded Table.";
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
	}
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	// we need to traverse through every line of the file and
	// record the number of neighbors for each observation.
	map<wxInt64, int>::iterator it;
	map<wxInt64, int> nbr_histogram;
	while (!file.eof()) {
		wxInt64 obs1=0;
		getline(file, str);
		if (!str.empty()) {
			stringstream ss (str, stringstream::in | stringstream::out);
			ss >> obs1;
			
			it = nbr_histogram.find(obs1);
			if (it == nbr_histogram.end()) {
				nbr_histogram[obs1] = 1;
			} else {
				nbr_histogram[obs1] = (*it).second + 1;
			}
		}
	}
	
	GwtElement* gwt = new GwtElement[num_obs];
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxInt64, int>::iterator it1;
	map<wxInt64, int>::iterator it2;
	int line_num = 1;
	while (!file.eof()) {
		int gwt_obs1, gwt_obs2;
		wxInt64 obs1, obs2;
        double w_val;
		getline(file, str);
		if (!str.empty()) {
			stringstream ss(str, stringstream::in | stringstream::out);
			ss >> obs1 >> obs2 >> w_val;
			it1 = id_map.find(obs1);
			it2 = id_map.find(obs2);
			if (it1 == id_map.end() || it2 == id_map.end()) {
				int obs;
				if (it1 == id_map.end()) obs = obs1;
				if (it2 == id_map.end()) obs = obs2;
				wxString msg = "On line ";
				msg << line_num+1 << " of weights file, observation id " << obs;
				if (use_rec_order) {
					msg << " encountered which out allowed observation ";
					msg << "range of 1 through " << num_obs << ".";
				} else {
					msg << " encountered which does not exist in field \"";
					msg << key_field << "\" of the Table.";
				}
				wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				delete [] gwt;
				return 0;
			}
			gwt_obs1 = (*it1).second; // value
			gwt_obs2 = (*it2).second; // value
            
			if (gwt[gwt_obs1].empty())
                gwt[gwt_obs1].alloc(nbr_histogram[obs1]);
            
			gwt[gwt_obs1].Push(GwtNeighbor(gwt_obs2, w_val));
		}
		line_num++;
	}	
	
	if (file.is_open()) file.close();
	
	return gwt;
}

GalElement* WeightUtils::Gwt2Gal(GwtElement* Gwt, long obs) 
{
	if (Gwt == NULL) return NULL;
	GalElement* Gal = new GalElement[obs];
	
	for (int i=0; i<obs; i++) {
		Gal[i].SetSizeNbrs(Gwt[i].Size());
		for (int j=0, sz=Gwt[i].Size(); j<sz; j++) {
			Gal[i].SetNbr(j, Gwt[i].data[j].nbx);
		}
	}
	return Gal;
}



