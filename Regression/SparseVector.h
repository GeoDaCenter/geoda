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

#ifndef __GEODA_CENTER_SPARSE_VECTOR_H__
#define __GEODA_CENTER_SPARSE_VECTOR_H__

/*  ---  SparseVector  ---  */
class SparseVector  {
public :
    SparseVector(const int sz);
    virtual ~SparseVector();
    int getNzEntries()  const  {  return nzEntries;  }
    bool isNonZero(const int ix)  const  {  return isNz[ ix ];  }
    int getIx(const int elt)  const  {  return nz[ elt ];  }
    double getValue(const int ix)  const  {  return val[ ix ];  }
    void setAt(const int ix, const double &v)  {
        checkin(ix);
        val[ ix ] = v;
    }
    void plusAt(const int ix, const double &v)  {
        checkin(ix);
        val[ ix ] += v;
    }
    void addTimes(const SparseVector &v, const double &w);
    void reset()  {
        checkout();
    }
    void makeDenseReady();
    void minus(const SparseVector &a, const SparseVector &b);
    double norm()  const;

    double product(const SparseVector &a)  const;
    void copy(const SparseVector &a);
    void timesPlus(const SparseVector &a, const double &s);
    void dropZeros();
private:
	int 	size;		// vector size (== allocation of the arrays)
    int		nzEntries;	// number of non-zero entries;
    bool	denseReady;
    double 	* val;		// values of all elements
    int		* nz;		// list of indices of non-zero elements;
    bool	* isNz;		// indicator of non-zero values
    void checkin(const int ix)  {
        if (!isNz[ix])  {
            isNz[ ix ] = true;
            nz [ nzEntries++ ] = ix;
        };
    }
    void checkout();
};
#endif

