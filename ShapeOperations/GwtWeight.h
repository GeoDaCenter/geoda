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

#ifndef __GEODA_CENTER_GWT_WEIGHT_H__
#define __GEODA_CENTER_GWT_WEIGHT_H__

#include <vector>
#include "GeodaWeight.h"

class Project;
class WeightsManInterface;
class TableInterface;

struct GwtNeighbor {
	long nbx;
	double weight;
	GwtNeighbor(const long nb=0, const double w=0) : nbx(nb), weight(w) {}
	void assign(const long nb=0, const double w=0) { nbx=nb;  weight=w; }
};

class GwtElement {
public:
	long nbrs; // current number of neighbors
	GwtNeighbor* data; // list neighborhood
	
public:
	GwtElement() : data(0), nbrs(0) {}
	virtual ~GwtElement();
    
	bool alloc(const int sz);
    
	bool empty() const { return data == 0; }
    
	void Push(const GwtNeighbor &elt) { data[nbrs++] = elt; }
    
	GwtNeighbor Pop() { return !nbrs ? GwtNeighbor(-1) : data[--nbrs]; }
    
	long Size() const { return nbrs; }
    
	GwtNeighbor elt(const long where) const { return data[where]; }
    
	GwtNeighbor* dt() const { return data; }
    
	double SpatialLag(const std::vector<double>& x, const bool std=true) const;
    
	double SpatialLag(const double* x, const bool std=true) const;
};


class GwtWeight : public GeoDaWeight {
public:
	GwtWeight() : gwt(0) { weight_type = gwt_type; }
	virtual ~GwtWeight() { if (gwt) delete [] gwt; gwt = 0; }
	GwtElement* gwt;
	static bool HasIsolates(GwtElement *gwt, int num_obs);
    
	virtual bool HasIsolates() { return HasIsolates(gwt, num_obs); }
    virtual bool SaveDIDWeights(Project* project,
                        int num_obs,
                        std::vector<wxInt64>& newids,
                        std::vector<wxInt64>& stack_ids,
                        const wxString& ofname);
    virtual bool SaveSpaceTimeWeights(const wxString& ofname, WeightsManInterface* wmi, TableInterface* table_int);
    
    virtual void Update(const std::vector<bool>& undefs);
};

namespace Gda {
	bool SaveGwt(const GwtElement* g,
							 const wxString& layer_name, 
							 const wxString& ofname,
							 const wxString& id_var_name,
							 const std::vector<wxInt64>& id_vec);
	bool SaveGwt(const GwtElement* g,
							 const wxString& layer_name, 
							 const wxString& ofname,
							 const wxString& id_var_name,
							 const std::vector<wxString>& id_vec);
}

#endif
