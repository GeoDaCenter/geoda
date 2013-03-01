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

#ifndef __GEODA_CENTER_SPARSE_MATRIX_H__
#define __GEODA_CENTER_SPARSE_MATRIX_H__

#include "SparseVector.h"
#include "DenseVector.h"
#include "SparseRow.h"

class GalElement;
class GwtElement;

/*  ---  SparseMatrix  ---  */
class SparseMatrix  {

public :
	void WtTimesColumn(DenseVector &wtx, DenseVector const &x);
	DenseVector* GetCol(const int c);
	double GetTraceWW();
	long GetSumAllElement();
    SparseMatrix(const int sz)  {
        init(sz);
    }
	SparseMatrix(const GalElement *my_gal, int obs); 
	SparseMatrix(const GwtElement *my_gwt, int obs); 
	// create sparse matrix I-rho*W
    SparseMatrix(const SparseMatrix &W, const double rho);
    virtual ~SparseMatrix();

    int dim()  const  {  return size;  }

    void setTranspose(SparseMatrix &m);

    SparseMatrix * getTranspose() {
        return (transpose) ? transpose : (transpose = makeTranspose());
    }

    void rowMatrix(SparseVector &row1, const SparseVector &row2)  const;
    void rowMatrix(DenseVector &r, const DenseVector &b)  const;

    void matrixColumn(DenseVector &c1, const DenseVector &c2)  const;
    double scaledMatrixColumn(const SparseVector &v, const int ix)  const;

    void rowStandardize();

    void symStd();
    void alloc(const int ns) {
        if (ns != size) {
            release(&row);
            if (transpose) delete transpose;
            release(&scale);
        }
        init(ns);
    }

    void setRow(const int loc, SparseRow &r)  {  row[loc] = r;  }
    SparseRow & getRow(const int r)  const  {  return row[ r ];  }

    double * getScale()  const  {  return scale;  }

    void makeStdSymmetric();
    void makeRowStd();

    void rowIminusRhoThis(const double rho, SparseVector &row1,
						  const SparseVector &row2)  const;
    void IminusRhoThis( const double rho, const DenseVector &column,
					   DenseVector &result)  const;

    void scaleUp(DenseVector &v) const  {
        for (int cnt = 0; cnt < size; ++cnt)
            v.setAt( cnt, v.getValue(cnt) * scale[cnt] );
    }

    void scaleUp(DenseVector &v, const DenseVector &src) const  {
        for (int cnt = 0; cnt < size; ++cnt)
            v.setAt( cnt, src.getValue(cnt) * scale[cnt] );
    }

    void scaleDown(DenseVector &v)  const  {
        for (int cnt = 0; cnt < size; ++cnt)
            v.setAt( cnt, v.getValue(cnt) / scale[cnt] );
    }

private :
    int		size;		// dimension of the square matrix
    SparseRow	* row;
    DenseVector	* col;

    SparseMatrix	* transpose;
    double	* scale;
		long		m_SumAllElement;


    SparseMatrix * makeTranspose();
    void init(const int sz);
    void createGAL(const GalElement * my_gal, int obs);
    void createGWT(const GwtElement * my_gwt, int obs);
};
#endif

