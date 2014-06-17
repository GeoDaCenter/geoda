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

GalElement::GalElement() : data(0), size(0)
{
}

GalElement::~GalElement()
{
	LOG_MSG("In GalElement::~GalElement");
	if (data) delete [] data;
	size = 0;
}

int GalElement::alloc(int sz)
{
	if (data) delete [] data;
	if (sz > 0) {
		size = 0;
		data = new long[sz];
	}
	return !empty();
}

//*** compute spatial lag for a contiguity weights matrix
//*** optionally (default) performs standardization of the result
double GalElement::SpatialLag(const std::vector<double>& x,
							  const bool std) const  {
	double    lag= 0;
	for (int  cnt= Size(); cnt > 0; )
		lag += x[ data[--cnt] ];
	if (std && Size() > 1)
		lag /= Size();
	return lag;
}

//*** compute spatial lag for a contiguity weights matrix
//*** optionally (default) performs standardization of the result
double GalElement::SpatialLag(const double *x, const bool std) const  {
	double    lag= 0;
	for (int  cnt= Size(); cnt > 0; )
		lag += x[ data[--cnt] ];
	if (std && Size() > 1)
		lag /= Size();
	return lag;
}

//*** compute spatial lag for a contiguity weights matrix
//*** optionally (default) performs standardization of the result
double GalElement::SpatialLag(const DataPoint *x, const bool std) const  {
	double    lag= 0;
	for (int cnt= Size(); cnt > 0; )
		lag += x[ data[--cnt] ].horizontal;
	if (std && Size() > 1)
		lag /= Size();
	return lag;
}

//*** compute spatial lag for a contiguity matrix, with a given permutation
//*** optionally (default) performs standardization
double GalElement::SpatialLag(const DataPoint *x, const int * perm,
							  const bool std) const  {
	double    lag = 0;
	for (int cnt = Size(); cnt > 0; )
		lag += x[ perm[ data[--cnt] ] ].horizontal;
	if (std && Size() > 1)
		lag /= Size();
	return lag;
}

double GalElement::SpatialLag(const double *x, const int * perm,
							  const bool std) const  
{
	double    lag = 0;
	for (int cnt = Size(); cnt > 0; )
		lag += x[ perm[ data[--cnt]]];
	if (std && Size() > 1)
		lag /= Size();
	return lag;
}

double GalElement::SpatialLag(const std::vector<double>& x, const int * perm,
							  const bool std) const  
{
	double    lag = 0;
	for (int cnt = Size(); cnt > 0; )
		lag += x[ perm[ data[--cnt]]];
	if (std && Size() > 1)
		lag /= Size();
	return lag;
}

GalElement* WeightUtils::ReadGal(const wxString& fname,
								 TableInterface* table_int)
{
	LOG_MSG("Entering WeightUtils::ReadGal");
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
	
	// Note: we want to be able to support blank lines.  If an observation
	// has no neighbors, then we'd like to be able to not include the
	// observation, or, if it is recorded, then the following line can
	// either be empty or blank.
	map<wxInt64, int> id_map;
	if (use_rec_order) {
		LOG_MSG("using record order");
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
			LOG_MSG(msg);
			wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return 0;
		}
		for (int i=0; i<num_obs; i++) id_map[i+min_val] = i;
	} else {
		int col=0, tm=0;
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
	
	GalElement* gal = new GalElement[num_obs];
	file.clear();
	file.seekg(0, ios::beg); // reset to beginning
	line_cnt = 0;
	getline(file, str); // skip header line
	line_cnt++;
	map<wxInt64, int>::iterator it;
	while (!file.eof()) {
		int gal_obs;
		wxInt64 obs, num_neigh;
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
			it = id_map.find(obs);
			if (it == id_map.end()) {
				wxString msg = "On line ";
				msg << line_cnt << " of weights file, observation id " << obs;
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
				delete [] gal;
				return 0;
			}
			gal_obs = (*it).second; // value
			gal[gal_obs].alloc(num_neigh);
		}
		if (num_neigh > 0) { // skip next of no neighbors
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
					long long neigh = 0;
					ss >> neigh;
					it = id_map.find(neigh);
					if (it == id_map.end()) {
						wxString msg = "On line ";
						msg << line_cnt << " of weights file, observation id ";
						msg << obs;
							if (use_rec_order) {
								msg << " encountered which out allowed ";
								msg << "observation ";
								msg << "range of 1 through " << num_obs << ".";
							} else {
								msg << " encountered which does not exist ";
								msg << "in field \"" << key_field;
								msg << " of the Table.";
							}
						LOG_MSG(msg);
						wxMessageDialog dlg(NULL, msg, "Error",
											wxOK|wxICON_ERROR);
						dlg.ShowModal();
						delete [] gal;
						return 0;
					}
					gal[gal_obs].Push((*it).second); // value of id_map[neigh];
				}
			}
		}
	}	
	
	file.clear();
	if (file.is_open()) file.close();
	
	LOG_MSG("Exiting WeightUtils::ReadGal");
	return gal;
}
