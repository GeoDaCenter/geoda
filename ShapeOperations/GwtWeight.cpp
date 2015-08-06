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

#include <fstream>
#include <iomanip>
#include <wx/filename.h>
#include "GwtWeight.h"

// file name encodings
// in windows, wxString.fn_str() will return a wchar*, which take care of 
// international encodings
// in mac, wxString.mb_str() will return UTF8 char*
#ifdef __WIN32__
  #ifndef GET_ENCODED_FILENAME
    #define GET_ENCODED_FILENAME(a) a.fn_str() 
  #endif
#else
#ifndef GET_ENCODED_FILENAME
  #define GET_ENCODED_FILENAME(a) a.mb_str() 
  #endif
#endif

GwtElement::~GwtElement()
{
	if (data) delete [] data;
	nbrs = 0;
}

bool GwtElement::alloc(const int sz)
{
	if (data) delete [] data;
	if (sz > 0) {
		nbrs = 0;
		data = new GwtNeighbor[sz];
	}
	return !empty();
}

double GwtElement::SpatialLag(const std::vector<double>& x,
															const bool std) const
{
	double lag= 0;
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

bool GwtWeight::HasIsolates(GwtElement *gwt, int num_obs)
{
	if (!gwt) return false;
	for (int i=0; i<num_obs; i++) { if (gwt[i].Size() <= 0) return true; }
	return false;
}

bool Gda::SaveGwt(const GwtElement* g,
									const wxString& layer_name, 
									const wxString& ofname,
									const wxString& id_var_name,
									const std::vector<wxInt64>& id_vec)  
{
	using namespace std;
	if (g == NULL || layer_name.IsEmpty() || ofname.IsEmpty()
			|| id_vec.size() == 0) return false;
	
	wxFileName wx_fn(ofname);
	wx_fn.SetExt("gwt");
	wxString final_ofn(wx_fn.GetFullPath());
	ofstream out;
	out.open(GET_ENCODED_FILENAME(final_ofn));
	if (!(out.is_open() && out.good())) return false;
	
	size_t num_obs = (int) id_vec.size();
	out << "0 " << num_obs << " " << layer_name;
	out << " " << id_var_name << endl;
	
	for (int i=0; i<num_obs; ++i) {
		for (long nbr=0; nbr<g[i].Size(); ++nbr) {
			const GwtNeighbor& current = g[i].elt(nbr);
			out << id_vec[i] << ' ' << id_vec[current.nbx];
			out << ' ' << setprecision(9) << setw(18)
				<< current.weight << endl;
		}
	}
	return true;
}


bool Gda::SaveGwt(const GwtElement* g,
                  const wxString& layer_name,
                  const wxString& ofname,
                  const wxString& id_var_name,
                  const std::vector<wxString>& id_vec)
{
	using namespace std;
	if (g == NULL || layer_name.IsEmpty() || ofname.IsEmpty()
			|| id_vec.size() == 0) return false;
	
	wxFileName wx_fn(ofname);
	wx_fn.SetExt("gwt");
	wxString final_ofn(wx_fn.GetFullPath());
	ofstream out;
	out.open(GET_ENCODED_FILENAME(final_ofn));
	if (!(out.is_open() && out.good())) return false;
	
	size_t num_obs = (int) id_vec.size();
	out << "0 " << num_obs << " " << layer_name;
	out << " " << id_var_name << endl;
	
	for (int i=0; i<num_obs; ++i) {
		for (long nbr=0; nbr<g[i].Size(); ++nbr) {
			const GwtNeighbor& current = g[i].elt(nbr);
			out << id_vec[i] << ' ' << id_vec[current.nbx];
			out << ' ' << setprecision(9) << setw(18)
				<< current.weight << endl;
		}
	}
	return true;
}