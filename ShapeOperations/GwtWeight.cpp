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

#include "../DataViewer/TableInterface.h"
#include "../GenUtils.h"
#include "../Project.h"
#include "GwtWeight.h"


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
    // NOTE: not used, please see GalElement::SpatialLag
	double lag= 0;
	int cnt = 0;
	for (cnt= (int)Size() - 1; cnt >= 0; cnt--) {
		//lag += data[cnt].weight * x[ data[cnt].nbx ];
		lag += x[ data[cnt].nbx ];
	}
	if (std && Size() > 1) lag /= Size();
	return lag;
}

double GwtElement::SpatialLag(const double *x, const bool std) const  {
    // NOTE: not used, please see GalElement::SpatialLag
	double    lag= 0;
	int cnt = 0;
	for (cnt= (int)Size() - 1; cnt >= 0; cnt--) {
		//lag += data[cnt].weight * x[ data[cnt].nbx ];
		lag += x[ data[cnt].nbx ];
	}
	if (std && Size() > 1) lag /= Size();
	return lag;
}

bool GwtElement::Check(long nbr_idx)
{
    for (size_t i=0; i<nbrs; ++i) {
        if (data[i].nbx == nbr_idx) return true;
    }
    return false;
}

std::vector<long> GwtElement::GetNbrs()
{
    std::vector<long> nbr_ids;
    for (size_t i=0; i<nbrs; ++i) {
        nbr_ids.push_back(data[i].nbx);
    }
    return nbr_ids;
}

////////////////////////////////////////////////////////////////////////////////
//

void GwtWeight::Update(const std::vector<bool>& undefs)
{
    
}

const std::vector<long> GwtWeight::GetNeighbors(int obs_idx) const
{
    return gwt[obs_idx].GetNbrs();
}

bool GwtWeight::CheckNeighbor(int obs_idx, int nbr_idx)
{
    return gwt[obs_idx].Check(nbr_idx);
}

bool GwtWeight::HasIsolates(GwtElement *gwt, int num_obs)
{
	if (!gwt) return false;
	for (int i=0; i<num_obs; i++) {
        if (gwt[i].Size() <= 0)
            return true;
    }
	return false;
}

void GwtWeight::GetNbrStats()
{
    double empties = 0;
    for (int i=0; i<num_obs; i++) {
        if (gwt[i].Size() == 0)
        empties += 1;
    }
    sparsity = empties / (double)num_obs;
    // others
    int sum_nnbrs = 0;
    vector<int> nnbrs_array;
    std::map<int, int> e_dict;
    for (int i=0; i<num_obs; i++) {
        GwtNeighbor* nbrs = gwt[i].dt();
        int n_nbrs = 0;
        for (int j=0; j<gwt[i].Size();j++) {
            int nbr = nbrs[j].nbx;
            if (i != nbr) {
                n_nbrs++;
                e_dict[i] = nbr;
                e_dict[nbr] = i;
            }
        }
        sum_nnbrs += n_nbrs;
        if (i==0 || n_nbrs < min_nbrs) min_nbrs = n_nbrs;
        if (i==0 || n_nbrs > max_nbrs) max_nbrs = n_nbrs;
        nnbrs_array.push_back(n_nbrs);
    }
    double n_edges = e_dict.size() / 2.0;
    density = 100.0* sum_nnbrs / (double)(num_obs * num_obs);
    
    if (num_obs > 0) mean_nbrs = sum_nnbrs / (double)num_obs;
    std::sort(nnbrs_array.begin(), nnbrs_array.end());
    if (num_obs % 2 ==0) {
        median_nbrs = (nnbrs_array[num_obs/2-1] + nnbrs_array[num_obs/2]) / 2.0;
    } else {
        median_nbrs = nnbrs_array[num_obs/2];
    }
}

bool GwtWeight::SaveDIDWeights(Project* project, int num_obs, std::vector<wxInt64>& newids, std::vector<wxInt64>& stack_ids, const wxString& ofname)
{
    using namespace std;
    if (!project || ofname.empty()) return false;
    
    WeightsManInterface* wmi = project->GetWManInt();
    if (!wmi) return false;
    
    wxString layer_name = GenUtils::GetFileNameNoExt(ofname);
    
    if (!gwt) return false;
    
    int n = newids.size();
    
#ifdef __WIN32__
	ofstream out(ofname.wc_str());
#else
	ofstream out;
	out.open(GET_ENCODED_FILENAME(ofname));
#endif

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
        
        for (long nbr=0; nbr<gwt[orig_id].Size(); ++nbr) {
            const GwtNeighbor& current = gwt[orig_id].elt(nbr);
            
            int n_id = current.nbx + offset + 1; // current.nbx starts from 0, so add 1
            
            out << newids[i] << ' ' << n_id << ' ' << setprecision(9) << setw(18) << current.weight << endl;
        }
    }
    return true;
}

bool GwtWeight::SaveSpaceTimeWeights(const wxString& ofname, WeightsManInterface* wmi, TableInterface* table_int)
{
    using namespace std;
    
    if (ofname.empty() || !wmi || !table_int)
        return false;
    
    wxString layer_name = GenUtils::GetFileNameNoExt(ofname);
    if (!gwt) return false;
    
    vector<wxString> id_vec;
    int c_id = table_int->FindColId(this->id_field);
    if (c_id < 0) return false;
    
    table_int->GetColData(c_id, 1, id_vec);
    
    std::vector<wxString> time_ids;
    table_int->GetTimeStrings(time_ids);
    
    size_t num_obs = id_vec.size();
    size_t num_t = time_ids.size();
    size_t n = num_obs * num_t;

    // (id, time) : stid
    typedef std::pair<wxString, wxString> STID_KEY;
    std::map<STID_KEY, int> stid_dict;
    
    int id=1;
    for (size_t i=0; i<num_t; ++i) {
        for (size_t j=0; j<num_obs; ++j) {
            STID_KEY k(id_vec[j], time_ids[i]);
            stid_dict[k] = id++;
        }
    }
    
#ifdef __WIN32__
	ofstream out(ofname.wc_str());
#else
	ofstream out;
	out.open(GET_ENCODED_FILENAME(ofname));
#endif

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
            
            for (long nbr=0; nbr<gwt[j].Size(); ++nbr) {
                const GwtNeighbor& current = gwt[j].elt(nbr);
                
                STID_KEY k1(id_vec[current.nbx], time_ids[i]);
                int n_id = stid_dict[k1];
                
                out << m_id << ' ' << n_id << ' ' << setprecision(9) << setw(18) << current.weight << endl;
            }
        }
    }
    
    return true;
}
////////////////////////////////////////////////////////////////////////////////
//
bool Gda::SaveGwt(const GwtElement* g,
									const wxString& _layer_name,
									const wxString& ofname,
									const wxString& id_var_name,
									const std::vector<wxInt64>& id_vec)  
{
	using namespace std;
	if (g == NULL || _layer_name.IsEmpty() || ofname.IsEmpty()
			|| id_vec.size() == 0) return false;
	
	wxFileName wx_fn(ofname);
	wxString final_ofn(wx_fn.GetFullPath());

#ifdef __WIN32__
	ofstream out(final_ofn.wc_str());
#else
	ofstream out;
	out.open(GET_ENCODED_FILENAME(final_ofn));
#endif

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
                  const wxString& _layer_name,
                  const wxString& ofname,
                  const wxString& id_var_name,
                  const std::vector<wxString>& id_vec)
{
	using namespace std;
	if (g == NULL || _layer_name.IsEmpty() || ofname.IsEmpty()
			|| id_vec.size() == 0) return false;
	
	wxFileName wx_fn(ofname);
	wxString final_ofn(wx_fn.GetFullPath());

#ifdef __WIN32__
	ofstream out(final_ofn.wc_str());
#else
	ofstream out;
	out.open(GET_ENCODED_FILENAME(final_ofn));
#endif

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
