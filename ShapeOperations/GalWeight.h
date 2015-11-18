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

#ifndef __GEODA_CENTER_GAL_WEIGHT_H__
#define __GEODA_CENTER_GAL_WEIGHT_H__

#include <vector>
#include <map>
#include "GeodaWeight.h"

class TableInterface;

class GalElement {
public:
	GalElement();
	void SetSizeNbrs(size_t sz);
	void SetNbr(size_t pos, long n);
	void SetNbr(size_t pos, long n, double w);
	void SetNbrs(const std::vector<long>& nbrs);
	const std::vector<long>& GetNbrs() const;
	void SortNbrs();
	long Size() const { return nbr.size(); }
	long operator[](size_t n) const { return nbr[n]; }
	double SpatialLag(const std::vector<double>& x) const;
	double SpatialLag(const double* x) const;
	double SpatialLag(const std::vector<double>& x, const int* perm) const;
    double GetRW(int idx);
    bool Check(long nbrIdx);
    
private:
    std::map<long, int> nbrLookup; // nbr_id, idx_in_nbrWeight
	std::vector<long> nbr;
	std::vector<double> nbrWeight;
    std::vector<double> nbrAvgW;
};

class GalWeight : public GeoDaWeight {
public:
	GalWeight() : gal(0) { weight_type = gal_type; }
	GalWeight(const GalWeight& gw);
	virtual GalWeight& operator=(const GalWeight& gw);
	virtual ~GalWeight() { if (gal) delete [] gal; gal = 0; }
	GalElement* gal;
	static bool HasIsolates(GalElement *gal, int num_obs);
	virtual bool HasIsolates() { return HasIsolates(gal, num_obs); }
};

namespace Gda {
	bool SaveGal(const GalElement* g,
							 const wxString& layer_name, 
							 const wxString& ofname,
							 const wxString& id_var_name,
							 const std::vector<wxInt64>& id_vec);
	bool SaveGal(const GalElement* g,
							 const wxString& layer_name, 
							 const wxString& ofname,
							 const wxString& id_var_name,
							 const std::vector<wxString>& id_vec);
	
	void MakeHigherOrdContiguity(size_t distance, size_t obs,
															 GalElement* W,
															 bool cummulative);
}

#endif
