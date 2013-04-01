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
#include "DenseMatrix.h"

DenseMatrix::DenseMatrix(const int r, const int c)  
{
    init(r, c);
}

DenseMatrix::DenseMatrix(double ** array, const int r, const int c)  
{
    if (rows > 0) clear();
    row = new DenseVector[ r ];
    if (row)  
		
    {
        rows = r;
        columns =c;
    };
    for (int cnt = 0; cnt < rows; ++cnt)
        row[cnt].absorb( array[cnt], c );
}


void DenseMatrix::timesColumn(DenseVector &r1, const DenseVector &v)  const  
{
    for (int r = 0; r < rows; ++r)
        row[r].setAt( r, row[r].product(v) );
}

void DenseMatrix::init(const int r, const int c)  
{
    if ( (rows > 0 && rows != r) || (columns > 0 && columns != c) )
        clear();
    row = new DenseVector[ r ];
    if (row)  
		{
        rows = r;
        columns = c;
    };
    for (int cnt = 0; cnt < rows; ++cnt)
        row[ cnt ].alloc( columns );
}

void DenseMatrix::clear()  
{
    for (int r = 0; r < rows; ++r)
        release(&row);
    rows = 0;
    columns = 0;

}

