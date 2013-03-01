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
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/GwtWeight.h"

#include "mix.h"
#include "SparseMatrix.h"


void SparseMatrix::init(const int sz)  
{
    if (sz <= 0) {
		error("dimension of the matrix must be a positive number");
	}
    size = sz;
    row = new SparseRow[ size ];
    scale = new double [ size ];
    transpose = NULL;
    col = new DenseVector;
		col->alloc(size);

}

SparseMatrix::SparseMatrix(const SparseMatrix &W, const double rho)  
{
    init(W.size);
    for (int r = 0; r < size; ++r)  {
        this->row[ r ].alloc( W.row[ r ].getSize() + 1 );	// eserve an extra slot for the diagonal element
        int loc = 0;
        for (int cnt = 0; cnt < W.row[r].getSize(); ++cnt)  {
            int ix = W.row[r].getIx(cnt);
            double w = -rho * W.row[r].getWeight(cnt);
            if (ix >= r && loc == cnt)  {
                if (ix == r) w += 1.0;			// diagonal element
                else this->row[r].setNb(loc++, r, 1);
            };
            this->row[r].setNb(loc++, ix, w);
        };
        if (loc == W.row[ r ].getSize())
            this->row[r].setNb(loc++, r, 1);		// diagonal element
    };
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

SparseMatrix::SparseMatrix(const GwtElement *my_gwt, int obs)  
{
	createGWT(my_gwt, obs);
}

void SparseMatrix::createGWT(const GwtElement* my_gwt, int obs)  
{	// get the weights from GWT file
    int dim = obs;
    typedef pair<int, int>		Map;
    typedef pair<int, double>   Tramp;

    Map	* key = new Map [ dim ];

    this->init( dim );

		m_SumAllElement = 0;
    int cnt = 0;
    for (cnt = 0; cnt < dim; ++cnt)  
		{	// for each row in the matrix ...
        int row, nbs;
				row = cnt;
				long *neigh = my_gwt[cnt].GetData();
				nbs = my_gwt[cnt].Size();
        key[cnt] = Map(row, cnt);
        this->row[cnt].alloc( nbs );

				m_SumAllElement += nbs;
        for (int nb = 0; nb < nbs; nb++)  
				{	// process each neighbor
            long nbId = neigh[nb]; 
            this->row[cnt].setNb( nb, nbId, my_gwt[cnt].elt(nb).weight );
        };
    };


    
    HeapSort(key, dim);
    int szi = key[dim-1].first - key[0].first + 1;

    int * ik = new int[ szi ];

    for (cnt = 0; cnt < szi; ++cnt)
        ik[ cnt ] = -1;
    for (cnt = 0; cnt < dim; ++cnt)
        ik [ key[cnt].first - key[0].first ] = key[cnt].second;

    for (int r = 0; r < dim; ++r)
        for (cnt = 0; cnt < row[r].getSize(); ++cnt)  
				{
            int oix = row[r].getIx(cnt);
            if (oix < key[0].first || oix > key[dim-1].first)  
						{
							wxMessageBox("Error: value does not exist in the weights file");
							exit(0);

            };
            int lx = ik[ oix - key[0].first ];
            if (lx < 0)  
						{
							wxMessageBox("Error: value does not exist in the weights file");
							exit(0);
            };
            this->row[r].setIx(cnt, lx);
        };

    release(&ik);
    release(&key);
		
}

void SparseMatrix::createGAL(const GalElement* my_gal, int obs)  
{	// get the weights from GAL file
    int dim = obs;
    typedef pair<int, int>		Map;

    Map	* key = new Map [ dim ];

    this->init( dim );

		m_SumAllElement = 0;
    int cnt = 0;
    for (cnt = 0; cnt < dim; ++cnt)  
		{	// for each row in the matrix ...
        int row, nbs;
				row = cnt;
				long *neigh = my_gal[cnt].dt();
				nbs = my_gal[cnt].Size();
        key[cnt] = Map(row, cnt);
        this->row[cnt].alloc( nbs );

				m_SumAllElement += nbs;
        for (int nb = 0; nb < nbs; nb++)  
				{	// process each neighbor
            long nbId = neigh[nb]; 
            this->row[cnt].setNb( nb, nbId, 1.0 );
        };
    };


    
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
				exit(0);
            }
            int lx = ik[ oix - key[0].first ];
            if (lx < 0) {
				wxMessageBox("Error: value does not exist in the weights file");
				exit(0);
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
void SparseMatrix::rowMatrix(SparseVector &row1, const SparseVector &row2)  const  
{
    row1.reset();

    for (int cnt = 0; cnt < row2.getNzEntries(); cnt++)  
		{
        int loc = row2.getIx(cnt);
        row[ loc ].rowPlusSRow( row1, row2.getValue(loc) );
    };
}

void SparseMatrix::rowIminusRhoThis(const double rho, SparseVector &row1, const SparseVector &row2)  const  {
    // accomplish row1 = row2 * (I-rhoThis) in  two steps:
    rowMatrix(row1, row2);			// row1 = row2 * This
    row1.timesPlus( row2, -rho);		// row1 = row2 - rho * row1
}

void SparseMatrix::rowMatrix(DenseVector &r, const DenseVector &b)  const  
{
    r.reset();		// r = 0
    for (int cnt = 0; cnt < b.getSize(); cnt++)
        row[ cnt ].rowPlusSRow( r, b.getValue(cnt) );
}

void SparseMatrix::matrixColumn(DenseVector &c1, const DenseVector &c2)  const  
{
    for (int cnt = 0; cnt < size; cnt++)
        c1.setAt( cnt, row[cnt].timesColumn(c2) );
}

// premultiplies column by diagonal matrix of scale and premultiplies result by its inverse, i.e.
// elements of the column are multiplied by scale and the result divided by scale[ix]
double SparseMatrix::scaledMatrixColumn(const SparseVector &v, const int ix)  const  {
    double sum = 0;
    int loc;
    SparseRow & rix = row[ ix ];
    for (int cnt = 0; cnt < rix.getSize(); ++cnt)
        if (v.isNonZero( loc = rix.getIx(cnt) ))
            sum += v.getValue( loc ) * rix.getWeight(cnt) * scale[ loc ] ;
    return sum / scale[ ix ];
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

// create a symmetric matrix (from a row-standardized one) that has the same eigenvalues
void SparseMatrix::makeStdSymmetric()  {
    for (int r = 0; r < size; ++r)  {
        row[ r ].mRowDivColumn( scale, r );
    };
}

// reverse the changes of the previos step -- makes a row-standardized matrix from a symmetric one
void SparseMatrix::makeRowStd()  
{
    for (int r = 0; r < size; ++r)  {
        row[ r ].mColumnDivRow( scale, r );
    };
}

void SparseMatrix::symStd()  {
	int r = 0;
    for (r = 0; r < size; ++r)  {
        scale [ r ] = sqrt( row [ r ].sum() );        
    };
    for (r = 0; r < size; ++r)
        for (int cnt = 0; cnt < row[r].getSize(); ++cnt)  {
            int ix = row[r].getIx(cnt);
            row[r].setWeight( cnt , row[r].getWeight(cnt) / scale[r] / scale[ix]);
        };
}

void SparseMatrix::IminusRhoThis( const double rho, const DenseVector &column, DenseVector &result)  const  {
    for (int r = 0; r < size; ++r)  {
        double p = row[ r ].timesColumn(column);
        result.setAt( r, column.getValue(r) - rho * p );
    };
}

long SparseMatrix::GetSumAllElement()
{
	return 		m_SumAllElement;
}

double SparseMatrix::GetTraceWW()
{
    double sum = 0;
    int loc;

		for (int ix=0;ix < size; ix++)
		{
			SparseRow & rix = row[ ix ];
			for (int cnt = 0; cnt < rix.getSize(); ++cnt)
			{
        loc = rix.getIx(cnt);

				// find ix in row[loc]
				bool found = false;
				double w = 0.0;
				for (int j=0;j < row[loc].getSize() && !found;j++)
				{
					if (row[loc].getIx(j) == ix)
					{
						found = true;
						w = row[loc].getWeight(j);
					}
//				else if (row[loc].getIx(j) > ix+1)
//					break;
				}

        if (found) 
					sum += rix.getWeight(cnt) * w;
			}
		}
    return sum;
}

DenseVector* SparseMatrix::GetCol(const int c) 
{
		for (int ix=0;ix < size; ix++)
		{
			col->setAt(ix,0.0);
			SparseRow & rix = row[ ix ];
			bool found = false;
			double w = 0.0;
			for (int j=0;j < rix.getSize() && !found;j++)
			{
				if (rix.getIx(j) == c)
				{
					found = true;
				  col->setAt(ix,rix.getWeight(j));
				}
			}
		}
		return col;
}

/*
void SparseMatrix::WtTimesColumn(DenseVector &wtx, const DenseVector &x)
{
	for (int i =0;i < size; i++)
	{
		DenseVector* w = GetCol(i);
		wtx.setAt(i,w->product(x));
	}
}
*/
 
void SparseMatrix::WtTimesColumn(DenseVector &wtx, const DenseVector &x)
{
	for (int i =0;i < size; i++)
	{
		Link* L = getRow(i).getNb();

		double s = 0.0;
		for (int j=0; j< getRow(i).getSize();j++)
		{
			double const n = getRow(L[j].getIx()).getSize();
			double const w = 1.0 / n;
			s += x.getValue(L[j].getIx()) * w;
		}
		wtx.setAt(i,s);
	}
}

