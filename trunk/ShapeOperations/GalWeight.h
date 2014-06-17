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

#ifndef __GEODA_CENTER_GAL_WEIGHT_H__
#define __GEODA_CENTER_GAL_WEIGHT_H__

#include <fstream>
#include <vector>
#include "../GdaConst.h"
#include "GeodaWeight.h"

class TableInterface;
struct DataPoint;

class GalElement {
public:
    long size; // number of neighbors in data array.
    long* data;

public:
    GalElement();
    virtual ~GalElement();
    int alloc(int sz);
    bool empty() const { return data == 0; }
    void Push(long val) { data[size++] = val; }
    long Pop() { return (size <= 0) ? GdaConst::EMPTY : data[--size]; }
    long Size() const { return size; }
	long elt(long where) const { return data[where]; }
    long* dt() const { return data; }
	double SpatialLag(const std::vector<double>& x, const bool std=true) const;
    double SpatialLag(const double* x, const bool std=true) const;
    double SpatialLag(const DataPoint* x, const bool std=true) const;
    double SpatialLag(const DataPoint* x, const int* perm,
					  const bool std=true) const;
    double SpatialLag(const double* x, const int* perm,
					  const bool std=true) const;
	double SpatialLag(const std::vector<double>& x, const int* perm,
					  const bool std=true) const;
};

class GalWeight : public GeoDaWeight {
public:
	GalWeight() : gal(0) { weight_type = gal_type; }
	virtual ~GalWeight() { if (gal) delete [] gal; gal = 0; }
	GalElement* gal;
	static bool HasIsolates(GalElement *gal, int num_obs) {
		if (!gal) return false;
		for (int i=0; i<num_obs; i++) { if (gal[i].Size() <= 0) return true; }
		return false; }
	virtual bool HasIsolates() { return HasIsolates(gal, num_obs); }
};

namespace WeightUtils {
	GalElement* ReadGal(const wxString& w_fname, TableInterface* table_int);
}

#endif
