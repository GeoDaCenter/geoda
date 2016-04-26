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

#include "mix.h"
#include "SparseRow.h"


SparseRow::SparseRow(int sz) : size(0), nb(NULL), Status_flag(false)
{
	alloc(sz);
}

void SparseRow::alloc(int sz)  
{
	if (sz <= 0) {
		//			error("Size must be positive\nCheck your weight matrix");
		//			return;
	}
	if (size != sz) {
		size = sz;
		release(&nb);
		nb = new Link[sz];
	}
	Status_flag = true;
}

SparseRow::~SparseRow()  
{
	release(&nb);
}

double SparseRow::sum() const
{
	double s = 0;
	for (int cnt = 0; cnt < size; ++cnt) s += nb[cnt].getWeight();
	return s;
}

void SparseRow::multiply(const double &v)  
{
	for (int cnt = 0; cnt < size; ++cnt) {
		nb[cnt].setWeight( nb[ cnt ].getWeight() * v );
	}
}

double SparseRow::timesColumn(const SparseVector &v) const  
{
	double sum = 0;
	int loc;
	for (int cnt = 0; cnt < this->size; ++cnt) {
		if (v.isNonZero( loc = nb[cnt].getIx() )) {
			sum += v.getValue( loc ) * nb[cnt].getWeight();
		}
	}
	return sum;
}

double SparseRow::timesColumn(const SparseRow &v) const  
{
	double sum = 0;
	Link *vnb = v.getNb();
	for (int cnt = 0; cnt < this->size; ++cnt) {
		sum += vnb[cnt].getWeight() * nb[cnt].getWeight();
	}
	return sum;
}

double SparseRow::timesColumn(const DenseVector &v) const
{
	double sum = 0;
	for (int cnt = 0; cnt < this->size; ++cnt) {
		sum += v.getValue(nb[cnt].getIx()) * nb[cnt].getWeight();
	}
	return sum;
}

void SparseRow::rowPlusSRow(SparseVector &v1, const double w)  const  
{
	for (int cnt = 0; cnt < size; ++cnt) {
		v1.plusAt( nb[cnt].getIx(), w * nb[cnt].getWeight() );
	}
}

void SparseRow::rowPlusSRow(DenseVector &v, const double w)  const  
{
	for (int cnt = 0; cnt < size; ++cnt) {
		v.plusAt( nb[cnt].getIx(), w * nb[cnt].getWeight() );
	}
}

void SparseRow::mRowDivColumn(const double * scale, const int row)
{
	const double m = scale[ row ];
	for (int cnt = 0; cnt < size; ++cnt)  {
    int ix = nb[cnt].getIx();
		nb[cnt].setWeight( nb[cnt].getWeight() * m / scale[ix] );
	}
}

void SparseRow::mColumnDivRow(const double * scale, const int row) {
	const double m = scale[ row ];
	for (int cnt = 0; cnt < size; ++cnt)  {
		int ix = nb[cnt].getIx();
		nb[cnt].setWeight( nb[cnt].getWeight() * scale[ix] / m );
	}
}
