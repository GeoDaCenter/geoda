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
#include <map>
#include <utility>
#include <boost/uuid/uuid.hpp>
#include <wx/filename.h>

#include "../GenUtils.h"
#include "../Project.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../DataViewer/TableInterface.h"
#include "GalWeight.h"


////////////////////////////////////////////////////////////////////////////////
//
// GalElement
//
////////////////////////////////////////////////////////////////////////////////
GalElement::GalElement()
{
    is_nbrAvgW_empty = true;
}

bool GalElement::Check(long nbrIdx)
{
    if (nbrLookup.find(nbrIdx) != nbrLookup.end())
        return true;
    return false;
}

// return row standardized weights value
double GalElement::GetRW(int idx)
{
    if (is_nbrAvgW_empty) {
        size_t sz = nbr.size();
        nbrAvgW.resize(sz);
        double sumW = 0.0;
        
        for (size_t i=0; i<sz; i++)
            sumW += nbrWeight[i];
        
        for (size_t i=0; i<sz; i++) {
            nbrAvgW[i] = nbrWeight[i] / sumW;
        }
        is_nbrAvgW_empty = false;
    }
    
    if (nbrLookup.find(idx) != nbrLookup.end())
        return nbrAvgW[nbrLookup[idx]];
    return 0;
}

void GalElement::SetSizeNbrs(size_t	sz)
{
	nbr.resize(sz);
    nbrWeight.resize(sz);
    for(size_t i=0; i<sz; i++) {
        nbrWeight[i] = 1.0;
    }
}

// (which neighbor, what ID)
void GalElement::SetNbr(size_t pos, long n)
{
    if (pos < nbr.size()) {
        nbr[pos] = n;
        nbrLookup[n] = pos;
    }
    // this should be called by GAL created only
    if (pos < nbrWeight.size()) {
        nbrWeight[pos] = 1.0;
    }
}

// (which neighbor, what ID, what value)
void GalElement::SetNbr(size_t pos, long n, double w)
{
    if (pos < nbr.size()) {
        nbr[pos] = n;
        nbrLookup[n] = pos;
    } else {
        nbr.push_back(n);
        nbrLookup[n] = pos;
    }
    
    // this should be called by GWT-GAL 
    if (pos < nbrWeight.size()) {
        nbrWeight[pos] = w;
    } else {
        nbrWeight.push_back(w);
    }
}

// Update neighbor information on the fly using undefs information
// NOTE: this has to be used with a copy of weights (keep the original weights!)
void GalElement::Update(const std::vector<bool>& undefs)
{
    std::vector<int> undef_obj_positions;
   
    for (int i=0; i<nbr.size(); i++) {
        int obj_id = nbr[i];
        if (undefs[obj_id]) {
            int pos = nbrLookup[obj_id];
            undef_obj_positions.push_back(pos);
        }
    }
   
    if (undef_obj_positions.empty())
        return;
    
    // sort the positions in descending order, for removing from std::vector
	std::sort(undef_obj_positions.begin(),
              undef_obj_positions.end(), std::greater<int>());
   
    for (int i=0; i<undef_obj_positions.size(); i++) {
        int pos = undef_obj_positions[i];
        if (pos < nbr.size()) {
            nbrLookup.erase( nbr[pos] );
            nbr.erase( nbr.begin() + pos);
        }
        if (pos < nbrWeight.size()) {
            nbrWeight.erase( nbrWeight.begin() + pos);
        }
    }
}

/*
void GalElement::SetNbrs(const std::vector<long>& nbrs)
{
	nbr = nbrs;
    if (nbrWeight.empty()) {
        size_t sz = nbr.size();
        nbrWeight.resize(sz);
        for(size_t i=0; i<sz; i++) {
            nbrLookup[nbrs[i]] = i;
            nbrWeight[i] = 1.0;
        }
    }
}
 */

void GalElement::SetNbrs(const GalElement& gal)
{
    size_t sz = gal.Size();
    nbr.resize(sz);
    nbrWeight.resize(sz);
    
    nbr = gal.GetNbrs();
    nbrLookup = gal.nbrLookup;
    nbrWeight = gal.GetNbrWeights();
    nbrLookup = gal.nbrLookup;
    nbrAvgW = gal.nbrAvgW;
}

const std::vector<long> & GalElement::GetNbrs() const
{
	return nbr;
}

const std::vector<double> & GalElement::GetNbrWeights() const
{
	return nbrWeight;
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
   
    double sumW = 0;
	for (size_t i=0; i<sz; ++i) {
        sumW += nbrWeight[i];
    }
    
    if (sumW == 0)
        lag = 0;
    else {
        for (size_t i=0; i<sz; ++i) {
            
            lag += x[nbr[i]] * nbrWeight[i] / sumW;
        }
    }
	
	return lag;
}

/** Compute spatial lag for a contiguity weights matrix.
 Automatically performs standardization of the result. */
double GalElement::SpatialLag(const double *x) const
{
	double lag = 0;
	size_t sz = Size();
    
    double sumW = 0;
	for (size_t i=0; i<sz; ++i) {
        sumW += nbrWeight[i];
    }
    
    if (sumW == 0)
        lag = 0;
    else {
        for (size_t i=0; i<sz; ++i) {
            lag += x[nbr[i]] * nbrWeight[i] / sumW;
        }
    }
	
	//for (size_t i=0; i<sz; ++i) lag += x[nbr[i]];
	//if (sz>1) lag /= (double) sz;
	return lag;
}

double GalElement::SpatialLag(const std::vector<double>& x,
							  const int* perm) const  
{
    // todo: this should also handle ReadGWtAsGAL like previous 2 functions
	double lag = 0;
	size_t sz = Size();
	for (size_t i=0; i<sz; ++i) lag += x[perm[nbr[i]]];
	if (sz>1) lag /= (double) sz;
	return lag;
}

////////////////////////////////////////////////////////////////////////////////
//
// GalWeight
//
////////////////////////////////////////////////////////////////////////////////
GalWeight::GalWeight(const GalWeight& gw)
: GeoDaWeight(gw)
{
	GalWeight::operator=(gw);
}

GalWeight& GalWeight::operator=(const GalWeight& gw)
{
	GeoDaWeight::operator=(gw);
	gal = new GalElement[num_obs];
    
    for (int i=0; i<num_obs; ++i) {
        gal[i].SetNbrs(gw.gal[i]);
    }
    
    this->num_obs = gw.num_obs;
    this->wflnm = gw.wflnm;
    this->id_field = gw.id_field;
    
	return *this;
}

void GalWeight::Update(const std::vector<bool>& undefs)
{
    for (int i=0; i<num_obs; ++i) {
        gal[i].Update(undefs);
    }

}

bool GalWeight::HasIsolates(GalElement *gal, int num_obs)
{
    if (!gal) {
        return false;
    }
	for (int i=0; i<num_obs; i++) {
        if (gal[i].Size() <= 0) {
            return true;
        }
    }
	return false;
}

bool GalWeight::SaveDIDWeights(Project* project, int num_obs,
                               std::vector<wxInt64>& newids,
                               std::vector<wxInt64>& stack_ids,
                               const wxString& ofname)
{
    using namespace std;
    if (!project || ofname.empty()) return false;
    
    WeightsManInterface* wmi = project->GetWManInt();
    if (!wmi) return false;
    
    wxString layer_name = GenUtils::GetFileNameNoExt(ofname);
    
    GalElement* gal = this->gal;
    if (!gal) return false;
    
    int n = newids.size();
    
    ofstream out;
    out.open(GET_ENCODED_FILENAME(ofname));
    if (!(out.is_open() && out.good())) return false;
  
    // if layer_name contains an empty space, the layer name should be
    // braced with quotes "layer name"
    if (layer_name.Contains(" ")) {
        layer_name = "\"" + layer_name + "\"";
    }
    
    wxString id_var_name("STID");
    out << "0 " << n << " " << layer_name;
    out << " " << id_var_name << endl;
   
    int offset = 0;
    
    for (size_t i=0; i<n; ++i) {
        int orig_id = stack_ids[i];
        if (i == num_obs) {
            offset = num_obs;
            num_obs += num_obs;
        }
        
        out << newids[i];
        out << " " << gal[orig_id].Size() << endl;
        
        for (int cp=gal[orig_id].Size(); --cp >= 0;) {
			int n_id = gal[orig_id][cp];
            out << n_id + offset + 1; // n_id starts from 0, so add 1
            if (cp > 0) out << " ";
        }
        out << endl;
    }
    return true;
}

bool GalWeight::SaveSpaceTimeWeights(const wxString& ofname, WeightsManInterface* wmi, TableInterface* table_int)
{
    using namespace std;
    
    if (ofname.empty() || !wmi || !table_int)
        return false;
    
    wxString layer_name = GenUtils::GetFileNameNoExt(ofname);
    GalElement* gal = this->gal;
    if (!gal) return false;

    vector<wxString> id_vec;
    int c_id = table_int->FindColId(this->id_field);
    if (c_id < 0) return false;

    table_int->GetColData(c_id, 1, id_vec);
    
    std::vector<wxString> time_ids;
    table_int->GetTimeStrings(time_ids);

    size_t num_obs = id_vec.size();
    size_t num_t = time_ids.size();
    size_t n = num_obs * num_t;

    
    typedef std::pair<wxString, wxString> STID_KEY;
    std::map<STID_KEY, int> stid_dict;
    
    int id=1;
    for (size_t i=0; i<num_t; ++i) {
        for (size_t j=0; j<num_obs; ++j) {
            STID_KEY k(id_vec[j], time_ids[i]);
            stid_dict[k] = id++;
        }
    }

    ofstream out;
    out.open(GET_ENCODED_FILENAME(ofname));
    if (!(out.is_open() && out.good())) return false;
    
    // if layer_name contains an empty space, the layer name should be
    // braced with quotes "layer name"
    if (layer_name.Contains(" ")) {
        layer_name = "\"" + layer_name + "\"";
    }
    
    wxString id_var_name("STID");
    out << "0 " << n << " " << layer_name;
    out << " " << id_var_name << endl;

    for (size_t i=0; i<num_t; ++i) {
        for (size_t j=0; j<num_obs; ++j) {
            STID_KEY k(id_vec[j], time_ids[i]);
            int m_id = stid_dict[k];
            out << m_id;
            out << " " << gal[j].Size() << endl;
            
            for (int cp=gal[j].Size(); --cp >= 0;) {
                STID_KEY k(id_vec[gal[j][cp]], time_ids[i]);
                int n_id = stid_dict[k];
                out << n_id;
                if (cp > 0) out << " ";
            }
            out << endl;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// TODO: following old style functions should be moved into GalWeight class
bool Gda::SaveGal(const GalElement* g,
                  const wxString& _layer_name,
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

    wxString layer_name(_layer_name);
    // if layer_name contains an empty space, the layer name should be
    // braced with quotes "layer name"
    if (layer_name.Contains(" ")) {
        layer_name = "\"" + layer_name + "\"";
    }
    
	size_t num_obs = (int) id_vec.size();
	out << "0 " << num_obs << " " << layer_name;
	out << " " << id_var_name << endl;
	
	for (size_t i=0; i<num_obs; ++i) {
		out << id_vec[i];
		out << " " << g[i].Size() << endl;
		for (int cp=g[i].Size(); --cp >= 0;) {
			out << id_vec[g[i][cp]];
			if (cp > 0)
                out << " ";
		}
		out << endl;
	}
	return true;
}

bool Gda::SaveGal(const GalElement* g,
                  const wxString& _layer_name,
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
	if (!(out.is_open() && out.good()))
        return false;

    wxString layer_name(_layer_name);
    
    // if layer_name contains an empty space, the layer name should be
    // braced with quotes "layer name"
    if (layer_name.Contains(" ")) {
        layer_name = "\"" + layer_name + "\"";
    }
	size_t num_obs = (int) id_vec.size();
	out << "0 " << num_obs << " " << layer_name;
	out << " " << id_var_name << endl;
	
	for (size_t i=0; i<num_obs; ++i) {
		out << id_vec[i];
		out << " " << g[i].Size() << endl;
		for (int cp=g[i].Size(); --cp >= 0;) {
			out << id_vec[g[i][cp]];
			if (cp > 0)
                out << " ";
		}
		out << endl;
	}
	return true;
}

bool Gda::SaveSpaceTimeGal(const GalElement* g,
                  const std::vector<wxString>& time_ids,
                  const wxString& _layer_name,
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
	
	size_t num_obs = id_vec.size();
    size_t num_t = time_ids.size();
    size_t n = num_obs * num_t;
   
    wxString layer_name(_layer_name);
    // if layer_name contains an empty space, the layer name should be
    // braced with quotes "layer name"
    if (layer_name.Contains(" ")) {
        layer_name = "\"" + layer_name + "\"";
    }
    
	out << "0 " << n << " " << layer_name;
	out << " " << id_var_name << endl;

    for (size_t i=0; i<num_t; ++i) {
    	for (size_t j=0; j<num_obs; ++j) {
            out << id_vec[i] << "_t" << time_ids[i];
    		out << " " << g[i].Size() << endl;
            
    		for (int cp=g[i].Size(); --cp >= 0;) {
    			out << id_vec[g[i][cp]] << "_t" << time_ids[i];
    			if (cp > 0) out << " ";
    		}
    		out << endl;
    	}
    }
	return true;
}

/** Add higher order neighbors up to (and including) distance.
 If cummulative true, then include lower orders as well.  Otherwise,
 only include elements on frontier. */
void Gda::MakeHigherOrdContiguity(size_t distance, size_t obs,
                                  GalElement* W,
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

