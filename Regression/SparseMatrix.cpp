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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/GalWeight.h"

#include "mix.h"
#include "SparseMatrix.h"


void SparseMatrix::init(const int sz)  
{
    size = sz;
    row = new SparseRow[ size ];
    scale = new double [ size ];
    
    col = new DenseVector;
	col->alloc(size);
}

 
SparseMatrix::~SparseMatrix()  
{
//    if (row)
//        for (int r = 0; r < size; ++r)
//            release( &row[r] );
    release(&row);
    release(&scale);
    size = 0;
}

SparseMatrix::SparseMatrix(const GalElement *my_gal, int obs)  
{
	createGAL(my_gal, obs);
}

void SparseMatrix::createGAL(const GalElement* my_gal, int obs)  
{	// get the weights from GAL file
    int dim = obs;
    typedef std::pair<int, int>	Map;

    Map	* key = new Map [ dim ];

    this->init( dim );

    int cnt = 0;
    for (cnt = 0; cnt < dim; ++cnt) { // for each row in the matrix ...
        int row, nbs;
		row = cnt;
		nbs = my_gal[cnt].Size();
        key[cnt] = Map(row, cnt);
        this->row[cnt].alloc( nbs );

        for (int nb = 0; nb < nbs; nb++) {	// process each neighbor
            this->row[cnt].setNb( nb, my_gal[cnt][nb], 1.0 );
        }
    }
    
    HeapSort(key, dim);
    int szi = key[dim-1].first - key[0].first + 1;

    int * ik = new int[ szi ];

    for (cnt = 0; cnt < szi; ++cnt)
        ik[ cnt ] = -1;
    for (cnt = 0; cnt < dim; ++cnt)
        ik [ key[cnt].first - key[0].first ] = key[cnt].second;

    for (int r = 0; r < dim; ++r) {
        for (cnt = 0; cnt < row[r].getSize(); ++cnt) {
            int oix = row[r].getIx(cnt);
            if (oix < key[0].first || oix > key[dim-1].first)  {
				wxMessageBox("Error: value does not exist in the weights file");
                return;
            }
            int lx = ik[ oix - key[0].first ];
            if (lx < 0) {
				wxMessageBox("Error: value does not exist in the weights file");
                return;
            }
            this->row[r].setIx(cnt, lx);
        }
	}
    release(&ik);
    release(&key);
}


/*  compute row-matrix product: r = b A.
*   implemented as the sum of sparse rows: r = sum(bi * ai), where
* bi is i-th component of the row; ai is i-th row of matrix A.
*/
void SparseMatrix::rowMatrix(SparseVector &row1, const SparseVector &row2) const
{
    row1.reset();

    for (int cnt = 0; cnt < row2.getNzEntries(); cnt++) {
        int loc = row2.getIx(cnt);
        row[ loc ].rowPlusSRow( row1, row2.getValue(loc) );
    }
}

void SparseMatrix::matrixColumn(DenseVector &c1, const DenseVector &c2) const
{
    for (int cnt = 0; cnt < size; cnt++) {
        c1.setAt(cnt, row[cnt].timesColumn(c2) );
	}
}

void SparseMatrix::rowIminusRhoThis(const double rho, SparseVector &row1,
									const SparseVector &row2)  const  {
    // accomplish row1 = row2 * (I-rhoThis) in  two steps:
    rowMatrix(row1, row2);			// row1 = row2 * This
    row1.timesPlus( row2, -rho);		// row1 = row2 - rho * row1
}

// rowsatndardization -- initial operation with a symmetric matrix
// should be performed only once
void SparseMatrix::rowStandardize()  {
    for (int r = 0; r < size; ++r)  {
        double sum = row[ r ].sum();
        if (sum > 0)
            row[ r ].multiply( 1.0/sum );
        else sum = 1;
        scale[ r ] = sqrt( sum );		// save square root of the sum of rows
    };
}

// create a symmetric matrix (from a row-standardized one) that
// has the same eigenvalues
void SparseMatrix::makeStdSymmetric()  {
    for (int r = 0; r < size; ++r)  {
        row[ r ].mRowDivColumn( scale, r );
    };
}

// reverse the changes of the previos step -- makes a
// row-standardized matrix from a symmetric one
void SparseMatrix::makeRowStd()  
{
    for (int r = 0; r < size; ++r)  {
        row[ r ].mColumnDivRow( scale, r );
    };
}

void SparseMatrix::IminusRhoThis( const double rho, const DenseVector &column,
								 DenseVector &result)  const  {
    for (int r = 0; r < size; ++r)  {
        double p = row[ r ].timesColumn(column);
        result.setAt( r, column.getValue(r) - rho * p );
    };
}

//
//void SparseMatrix::WtTimesColumn(DenseVector &wtx, const DenseVector &x)
//{
//	for (int i =0;i < size; i++)
//	{
//		DenseVector* w = GetCol(i);
//		wtx.setAt(i,w->product(x));
//	}
//}
//

// Incorrect for non-symmetric matrix
//void SparseMatrix::WtTimesColumn(DenseVector &wtx, const DenseVector &x)
//{
//	for (int i=0; i<size; i++) {
//		Link* L = getRow(i).getNb();
//		double s = 0.0;
//		for (int j=0, jsz=getRow(i).getSize(); j<jsz; j++) {
//			double const n = getRow(L[j].getIx()).getSize();
//			double const w = 1.0 / n;
//			s += x.getValue(L[j].getIx()) * w;
//		}
//		wtx.setAt(i,s);
//	}
//}

void SparseMatrix::WtTimesColumn(DenseVector &wtx, const DenseVector &x)
{
	MakeTranspose();
	typedef list< std::pair<int,double> >::iterator l_itr_t;
	
	for (int i=0; i<size; i++) {
		double s = 0.0;
		for (l_itr_t itr=transpose[i].begin(); itr != transpose[i].end(); ++itr)
		{
			int j = itr->first;
			double w = itr->second;
			s += x.getValue(j) * w;
		}
		wtx.setAt(i, s);
	}
}

/* Transpose is the transpose of SparseMatrix including weights.
 transpose data structure is a vector of lists of pairs where each pair is
 the index of the non-zero element and the value (weight) */
void SparseMatrix::MakeTranspose()
{
	if (transpose.size() != 0) return;
	transpose.resize(size);
	for (int i=0; i<size; i++) {
		Link* L = getRow(i).getNb();
		for (int nz_cnt=0, nz_sz=getRow(i).getSize(); nz_cnt<nz_sz; nz_cnt++) {
			int j = L[nz_cnt].getIx();
			double w_ij = L[nz_cnt].getWeight();
			transpose[j].push_back(std::make_pair(i, w_ij));
		}
	}
}
