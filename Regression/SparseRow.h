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

#ifndef __GEODA_CENTER_SPARSE_ROW_H__
#define __GEODA_CENTER_SPARSE_ROW_H__

#include "SparseVector.h"
#include "DenseVector.h"
#include "Link.h"

class SparseRow  {
public :
    SparseRow() : size(0), nb(NULL), Status_flag(false)  {};
    SparseRow(int nz);
    virtual ~SparseRow();

    void		alloc(int nz);
    double	timesColumn(const SparseVector &v)  const;
    double	timesColumn(const DenseVector &v)  const;
		double  timesColumn(const SparseRow &v) const;  

    void		rowPlusSRow(SparseVector &v, const double w)  const;
    void		rowPlusSRow(DenseVector &v, const double w)  const;

    int			getSize()  const  {  return size;  }
		Link*   getNb() const { return nb; }

    void		setNb(const int loc, const int nbr, const double &w)  
		{
        nb[ loc ].setIx( nbr );
        nb[ loc ].setWeight( w );
    }

    int getIx(const int loc)  const {  return nb[ loc ].getIx();  }
    double getWeight(const int loc)  const  {  return nb[ loc ].getWeight();  }
    void setWeight(const int loc, const double &w) { nb[ loc ].setWeight( w );}
    void setIx(const int loc, const int nbr)  {  nb[ loc ].setIx( nbr );  }
    Link & getLink(int loc)  {  return nb[ loc ];  }
    double sum()  const;
    void multiply(const double &v);
    void mRowDivColumn(const double * scale, const int row);
    void mColumnDivRow(const double * scale, const int row);
		bool Status_flag;

private:
    int		size;
    Link	* nb;

};

#endif


