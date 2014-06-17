/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <wx/msgdlg.h>
#include "../DataViewer/TableInterface.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "GalWeight.h"
#include "GwtWeight.h"

double GwtElement::SpatialLag(const std::vector<double>& x,
							  const bool std) const
{
	double    lag= 0;
	int cnt = 0;
	for (cnt= Size() - 1; cnt >= 0; cnt--) {
		//lag += data[cnt].weight * x[ data[cnt].nbx ];
		lag += x[ data[cnt].nbx ];
	}
	if (std && Size() > 1) lag /= Size();
	return lag;
}

double GwtElement::SpatialLag(const double *x, const bool std) const  {
	double    lag= 0;
	int cnt = 0;
	for (cnt= Size() - 1; cnt >= 0; cnt--) {
		//lag += data[cnt].weight * x[ data[cnt].nbx ];
		lag += x[ data[cnt].nbx ];
	}
	if (std && Size() > 1) lag /= Size();
	return lag;
}

double GwtElement::SpatialLag(const DataPoint *x, const bool std) const  {
	double    lag= 0;
	int cnt = 0;
	for (cnt= Size() - 1; cnt >= 0; cnt--) {
		//lag += data[cnt].weight * x[ data[cnt].nbx ].horizontal; 
		lag += x[ data[cnt].nbx ].horizontal;
	}
	if (std && Size() > 1) lag /= Size();
	return lag;
}

double GwtElement::SpatialLag(const DataPoint *x, 
							  const int * perm, const bool std) const  {
	double    lag = 0;
	int cnt = 0;
	for (cnt = Size() - 1; cnt >= 0; cnt--) {
		//lag += data[cnt].weight * x[ perm[ data[cnt].nbx ] ].horizontal;
		lag += x[ perm[ data[cnt].nbx ] ].horizontal;
	}
	if (std && Size() > 1) lag /= Size();
	return lag;
}

long* GwtElement::GetData() const
{
	long* dt = new long[nbrs];
	for (int i=0;i<nbrs;i++) dt[i] = data[i].nbx;
	return dt;
}

GalElement* WeightUtils::ReadGwtAsGal(const wxString& fname,
									 TableInterface* table_int)
{
	LOG_MSG("Entering WeightUtils::ReadGwtAsGal");
	using namespace std;
	ifstream file;
	//file.open(fname.mb_str(wxConvUTF8), ios::in);  // a text file
	file.open(fname.fn_str(), ios::in);  // a text file
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
	ss >> num1 >> num2 >> dbf_name >> t_key_field;
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
		LOG_MSG(msg);
		wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return 0;
	}
	
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxInt64, int> id_map;
	if (use_rec_order) {
		LOG_MSG("using record order");
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
			LOG_MSG(msg);
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
			LOG_MSG(msg);
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		if (table_int->GetColType(col) != GdaConst::long64_type) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file is";
			msg << " not an integer type in the currently loaded Table.";
			LOG_MSG(msg);
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		// get mapping from key_field to record ids (which always start
		// from 0 internally, but are displayed to the user from 1)
		vector<wxInt64> vec;
		table_int->GetColData(col, 0, vec);
		for (int i=0; i<num_obs; i++) id_map[vec[i]] = i;
		//for (int i=0; i<num_obs; i++) {
		//	LOG_MSG(wxString::Format("id_map[vec[%d]]=%d", (int) vec[i],
		//							 (int) id_map[vec[i]]));
		//}
		if (id_map.size() != num_obs) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" in weights file contains duplicate ";
			msg << "values in the currently loaded Table.";
			LOG_MSG(msg);
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
	
	GalElement* gal = new GalElement[num_obs];
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxInt64, int>::iterator it1;
	map<wxInt64, int>::iterator it2;
	int line_num=1;
	while (!file.eof()) {
		int gwt_obs1, gwt_obs2;
		wxInt64 obs1, obs2;
		getline(file, str);
		if (!str.empty()) {
			stringstream ss(str, stringstream::in | stringstream::out);
			ss >> obs1 >> obs2;
			it1 = id_map.find(obs1);
			it2 = id_map.find(obs2);
			if (it1 == id_map.end() || it2 == id_map.end()) {
				int obs;
				if (it1 == id_map.end()) obs = obs1;
				if (it2 == id_map.end()) obs = obs2;
				wxString msg = "On line ";
				msg << line_num+1 << " of weights file, observation id " << obs;
				if (use_rec_order) {
					msg << " encountered which is out of allowed observation ";
					msg << "range of 1 through " << num_obs << ".";
				} else {
					msg << " encountered which does not exist in field \"";
					msg << key_field << " of the Table.";
				}
				LOG_MSG(msg);
				wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				delete [] gal;
				return 0;
			}
			//LOG_MSG(wxString::Format("nbr_histogram[%d]=%d", (int) obs1,
			//						 (int) nbr_histogram[obs1]));
			gwt_obs1 = (*it1).second; // value
			gwt_obs2 = (*it2).second; // value
			if (gal[gwt_obs1].empty()) {
				gal[gwt_obs1].alloc(nbr_histogram[obs1]);
			}
			gal[gwt_obs1].Push(gwt_obs2);
		}
		line_num++;
	}	
	
	file.clear();
	if (file.is_open()) file.close();
	
	LOG_MSG("Exiting WeightUtils::ReadGwtAsGal");
	return gal;
}

/** This function should not be used unless an actual GWT object is needed
 internally.  In most cases, the ReadGwtAsGal function should be used */
GwtElement* WeightUtils::ReadGwt(const wxString& fname,
								 TableInterface* table_int)
{
	LOG_MSG("Entering WeightUtils::ReadGwt");
	using namespace std;
	ifstream file;
	//file.open(fname.mb_str(wxConvUTF8), ios::in);  // a text file
	file.open(fname.fn_str(), ios::in);  // a text file
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
	ss >> num1 >> num2 >> dbf_name >> t_key_field;
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
		LOG_MSG(msg);
		wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return 0;
	}
	
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	getline(file, str); // skip header line
	map<wxInt64, int> id_map;
	if (use_rec_order) {
		LOG_MSG("using record order");
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
			LOG_MSG(msg);
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
			LOG_MSG(msg);
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		if (table_int->GetColType(col) != GdaConst::long64_type) {
			wxString msg = "Specified key value field \"";
			msg << key_field << "\" on first line of weights file is ";
			msg << " not an integer type in the currently loaded Table.";
			LOG_MSG(msg);
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
			LOG_MSG(msg);
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
		getline(file, str);
		if (!str.empty()) {
			stringstream ss(str, stringstream::in | stringstream::out);
			ss >> obs1 >> obs2;
			it1 = id_map.find(obs1);
			it2 = id_map.find(obs1);
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
					msg << key_field << " of the Table.";
				}
				LOG_MSG(msg);
				wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				delete [] gwt;
				return 0;
			}
			gwt_obs1 = (*it1).second; // value
			gwt_obs2 = (*it2).second; // value
			if (gwt[gwt_obs1].empty()) gwt[gwt_obs1].alloc(nbr_histogram[obs1]);
			gwt[gwt_obs1].Push(gwt_obs2);
		}
		line_num++;
	}	
	
	if (file.is_open()) file.close();
	
	LOG_MSG("Exiting WeightUtils::ReadGwt");
	return gwt;
}

GalElement* WeightUtils::Gwt2Gal(GwtElement* Gwt, long obs) 
{
	if (Gwt == NULL) return NULL;
	GalElement* Gal = new GalElement[obs];
	
	for (int i=0; i<obs; i++) {
		Gal[i].alloc(Gwt[i].Size());
		for (int j=0; j < Gwt[i].Size(); j++) {
			Gal[i].Push(Gwt[i].data[j].nbx);
		}
	}
	return Gal;
}
