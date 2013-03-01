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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "mix.h"
#include "DenseVector.h"

DenseVector::DenseVector(const int sz) : size(0), val(NULL)  
{
    if (sz < 0) error("  size of vector must be positive");
    if (sz > 0) alloc( sz );
}

void DenseVector::alloc(const int sz)  
{
    if (size > 0 && size != sz) release(&val);
    size = sz;
    val = new double[ sz ];
    if (!val)  
		{
			error("Error: fail to allocate memory");
		}
		rlse = false;

    reset();
}    

DenseVector::~DenseVector()  
{
    if (rlse)  release(&val);
//    rlse = true;
    size = 0;
}


void DenseVector::addTimes(const DenseVector &v, const double &w)  {

    for (int cnt = 0; cnt < size; ++cnt)
        val[ cnt ] += v.val[ cnt ] * w;
}



void DenseVector::minus(const DenseVector &a, const DenseVector &b)  
{
    if (a.size != this->size || b.size != this->size)  
		{
        error("  vector size don't match");
    };
    for (int cnt = 0; cnt < size; ++cnt)
        val[ cnt ] = a.val[ cnt ] - b.val[ cnt ];
}



double DenseVector::norm()  const  
{
	return ::norm(val, size);
}



double DenseVector::product(const DenseVector &a)  const  
{
    double s = ::product(this->val, a.val, size);
    return s;
}



void DenseVector::timesMatrix(DenseVector &v, const DenseVector *w)  const  {

    const int dim = w[0].getSize();
    v.alloc( dim );
    for (int cnt = 0; cnt < dim; ++cnt)  
		{
        double s = 0;
        for (int cp = 0; cp < this->getSize(); ++cp)
            s += this->getValue(cp) * w[cp].getValue(cnt);
        v.setAt( cnt, s );
    }
}



void DenseVector::timesSquareMatrix(DenseVector &v, double **cov)  const 
 {
    v.alloc( this->getSize() );
    for (int cnt = 0; cnt < getSize(); ++cnt)  
		{
        double s = 0;
        for (int cp = 0; cp < getSize(); ++cp)  
            s += getValue(cp) * cov[cp][cnt];
        v.setAt( cnt, s );
    }
}



void DenseVector::squareTimesColumn(DenseVector &v, double **cov)  const  
{
    v.alloc( this->getSize() );
    for (int cnt = 0; cnt < getSize(); ++cnt)  
		{
        double s = 0;
        for (int cp = 0; cp < getSize(); ++cp)
            s += getValue(cp) * cov[cnt][cp];
        v.setAt( cnt, s );
    }
}

void DenseVector::copy(const DenseVector &a)  {

    for (int cnt = 0; cnt < size; ++cnt)
        val[ cnt ] = a.val[ cnt ];
}


void DenseVector::timesPlus(const DenseVector &a, const double &s)  {

    for (int cnt = 0; cnt < size; ++cnt)
        val[ cnt ] = val[ cnt] * s + a.val[ cnt ];

}

double DenseVector::sum()  const  {

    double sum = 0;

    for (int cnt = 0; cnt < size; ++cnt) sum += val[ cnt ];

    return sum;

}
