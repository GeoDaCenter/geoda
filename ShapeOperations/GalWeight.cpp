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
#include <iomanip>
#include <fstream>
#include <set>
#include <wx/filename.h>
#include "GalWeight.h"

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

GalElement::GalElement()
{
}

void GalElement::SetSizeNbrs(size_t	sz)
{
	nbr.resize(sz);
}

void GalElement::SetNbr(size_t pos, long n)
{
	if (pos < nbr.size()) nbr[pos] = n;
}

void GalElement::SetNbrs(const std::vector<long>& nbrs)
{
	nbr = nbrs;
}

const std::vector<long> & GalElement::GetNbrs() const
{
	return nbr;
}

void GalElement::SortNbrs()
{
	std::sort(nbr.begin(), nbr.end(), std::greater<long>());
}

/** Compute spatial lag for a contiguity weights matrix.
 Automatically performs standardization of the result. */
double GalElement::SpatialLag(const std::vector<double>& x) const
{
	double lag = 0;
	size_t sz = Size();
	for (size_t i=0; i<sz; ++i) lag += x[nbr[i]];
	if (sz>1) lag /= (double) sz;
	return lag;
}

/** Compute spatial lag for a contiguity weights matrix.
 Automatically performs standardization of the result. */
double GalElement::SpatialLag(const double *x) const
{
	double lag = 0;
	size_t sz = Size();
	for (size_t i=0; i<sz; ++i) lag += x[nbr[i]];
	if (sz>1) lag /= (double) sz;
	return lag;
}

double GalElement::SpatialLag(const std::vector<double>& x,
							  const int* perm) const  
{
	double lag = 0;
	size_t sz = Size();
	for (size_t i=0; i<sz; ++i) lag += x[perm[nbr[i]]];
	if (sz>1) lag /= (double) sz;
	return lag;
}

GalWeight::GalWeight(const GalWeight& gw)
: GeoDaWeight(gw)
{
	GalWeight::operator=(gw);
}

GalWeight& GalWeight::operator=(const GalWeight& gw)
{
	GeoDaWeight::operator=(gw);
	gal = new GalElement[num_obs];
	for (int i=0; i<num_obs; ++i) gal[i].SetNbrs(gw.gal[i].GetNbrs());
	return *this;
}

bool GalWeight::HasIsolates(GalElement *gal, int num_obs)
{
	if (!gal) return false;
	for (int i=0; i<num_obs; i++) { if (gal[i].Size() <= 0) return true; }
	return false;
}

bool Gda::SaveGal(const GalElement* g, 
									const wxString& layer_name, 
									const wxString& ofname, 
									const wxString& id_var_name,
									const std::vector<wxInt64>& id_vec)
{
	using namespace std;
	if (g == NULL || ofname.empty() ||
			id_var_name.empty() || id_vec.size() == 0) return false;
	
	wxFileName wx_fn(ofname);
	wx_fn.SetExt("gal");
	wxString final_fon(wx_fn.GetFullPath());
	ofstream out;
	out.open(GET_ENCODED_FILENAME(final_fon));
	if (!(out.is_open() && out.good())) return false;
	
	size_t num_obs = (int) id_vec.size();
	out << "0 " << num_obs << " " << layer_name;
	out << " " << id_var_name << endl;
	
	for (size_t i=0; i<num_obs; ++i) {
		out << id_vec[i];
		out << " " << g[i].Size() << endl;
		for (int cp=g[i].Size(); --cp >= 0;) {
			out << id_vec[g[i][cp]];
			if (cp > 0) out << " ";
		}
		out << endl;
	}
	return true;
}

bool Gda::SaveGal(const GalElement* g,
                  const wxString& layer_name,
                  const wxString& ofname,
                  const wxString& id_var_name,
                  const std::vector<wxString>& id_vec)
{
	using namespace std;
	if (g == NULL || ofname.empty() ||
        id_var_name.empty() || id_vec.size() == 0) return false;
	
	wxFileName wx_fn(ofname);
	wx_fn.SetExt("gal");
	wxString final_fon(wx_fn.GetFullPath());
	ofstream out;
	out.open(GET_ENCODED_FILENAME(final_fon));
	if (!(out.is_open() && out.good())) return false;
	
	size_t num_obs = (int) id_vec.size();
	out << "0 " << num_obs << " " << layer_name;
	out << " " << id_var_name << endl;
	
	for (size_t i=0; i<num_obs; ++i) {
		out << id_vec[i];
		out << " " << g[i].Size() << endl;
		for (int cp=g[i].Size(); --cp >= 0;) {
			out << id_vec[g[i][cp]];
			if (cp > 0) out << " ";
		}
		out << endl;
	}
	return true;
}

/** Add higher order neighbors up to (and including) distance. 
 If cummulative true, then include lower orders as well.  Otherwise,
 only include elements on frontier. */
void Gda::MakeHigherOrdContiguity(size_t distance, size_t obs, GalElement* W,
																	bool cummulative)
{	
	using namespace std;
	if (obs < 1 || distance <=1) return;
	vector<vector<long> > X(obs);
	for (size_t i=0; i<obs; ++i) {
		vector<set<long> > n_at_d(distance+1);
		n_at_d[0].insert(i);
		for (size_t j=0, sz=W[i].Size(); j<sz; ++j) {
			n_at_d[1].insert(W[i][j]);
		}
		for (size_t d=2; d<=distance; ++d) {
			for (set<long>::const_iterator it=n_at_d[d-1].begin();
					 it!=n_at_d[d-1].end(); ++it)
			{
				for (size_t j=0, sz=W[*it].Size(); j<sz; ++j) {
					long nbr = W[*it][j];
					if (n_at_d[d-1].find(nbr) == n_at_d[d-1].end() &&
							n_at_d[d-2].find(nbr) == n_at_d[d-2].end()) {
						n_at_d[d].insert(nbr);
					}
				}
			}
		}
		size_t sz_Xi = 0;
		for (size_t d=(cummulative ? 1 : distance); d<=distance; ++d) {
			sz_Xi += n_at_d[d].size();
		}
		X[i].resize(sz_Xi);
		size_t cnt=0;
		for (size_t d=(cummulative ? 1 : distance); d<=distance; ++d) {
			for (set<long>::const_iterator it=n_at_d[d].begin();
					 it!=n_at_d[d].end(); ++it) { X[i][cnt++] = *it; }
		}
		sort(X[i].begin(), X[i].end(), greater<long>());
	}
	for (size_t i=0; i<obs; ++i) {
		W[i].SetSizeNbrs(X[i].size());
		for (size_t j=0, sz=X[i].size(); j<sz; ++j) W[i].SetNbr(j, X[i][j]);
	}
}

