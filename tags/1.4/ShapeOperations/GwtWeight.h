/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include <fstream>
#include <vector>
#include "../GeoDaConst.h"
#include "GeodaWeight.h"

class GalElement;
class DbfGridTableBase;
struct DataPoint;

struct GwtNeighbor
{
    long    nbx;
    double  weight;
    GwtNeighbor(const long nb=0, const double w=0) : nbx(nb), weight(w) {}
    void assign(const long nb=0, const double w=0) { nbx=nb;  weight=w; }
};

class GwtElement {
public:
    long nbrs; // current number of neighbors
    GwtNeighbor* data; // list neighborhood
	
public:
    GwtElement(const long sz=0) : data(0), nbrs(0) {
        if (sz > 0) data = new GwtNeighbor[sz]; }
    virtual ~GwtElement() {
        if (data) delete [] data;
        nbrs = 0; }
    bool alloc(const int sz) {
		if (data) delete [] data;
        if (sz > 0) {
			nbrs = 0;
			data = new GwtNeighbor[sz];
		}
        return !empty(); }
    bool empty() const { return data == 0; }
    void Push(const GwtNeighbor &elt) { data[nbrs++] = elt; }
    GwtNeighbor Pop() {
        if (!nbrs) return GwtNeighbor(GeoDaConst::EMPTY);
        return data[--nbrs]; }
    long Size() const { return nbrs; }
    GwtNeighbor elt(const long where) const { return data[where]; }
	GwtNeighbor* dt() const { return data; }
	double SpatialLag(const std::vector<double>& x, const bool std=true) const;
    double SpatialLag(const double* x, const bool std=true) const;
    double SpatialLag(const DataPoint* x, const bool std=true) const;
    double SpatialLag(const DataPoint* x, const int *perm,
					  const bool std=true) const;

	long* GetData() const; // this allocates an array and should be removed
};


class GwtWeight : public GeoDaWeight {
public:
	GwtWeight() : gwt(0) { weight_type = gwt_type; }
	virtual ~GwtWeight() { if (gwt) delete [] gwt; gwt = 0; }
	GwtElement* gwt;
	static bool HasIsolates(GwtElement *gwt, int num_obs) {
		if (!gwt) return false;
		for (int i=0; i<num_obs; i++) { if (gwt[i].Size() <= 0) return true; }
		return false; }
	virtual bool HasIsolates() { return HasIsolates(gwt, num_obs); }
};

namespace WeightUtils {
	GalElement* ReadGwtAsGal(const wxString& w_fname,
							DbfGridTableBase* grid_base);
	GwtElement* ReadGwt(const wxString& w_fname, DbfGridTableBase* grid_base);
	GalElement* Gwt2Gal(GwtElement* Gwt, long obs);
}

#endif
