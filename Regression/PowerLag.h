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

#ifndef __GEODA_CENTER_POWER_LAG_H__
#define __GEODA_CENTER_POWER_LAG_H__

/*
RowMultiply
product is a row-vector, that results from vector (row) and
sparse matrix (mt) multiplication.
Assumes all objects already exist.
 */
//template <class R>
inline void RowMultiply(Iterator<VALUE> row, Iterator<WMap> mt,
						Vector<VALUE> &product)  {
    INDEX dim= mt.count();
    product.reset();
    for (INDEX cp= 0; cp < dim; ++cp) product << 0;
    for( ; mt; ++mt, ++row)  {
        VALUE RowValue= *row;
        if (RowValue)
            for (WMap::input_iterator it= (*mt)(); it; ++it)
                product[(*it).first] += (*it).second * RowValue;
    };
    return;
}

/*
PowerLag
 */
//template <class R>
class PowerLag  
{
public :
    PowerLag(const Iterator<WMap> matrix, const INDEX vsize);
    VALUE Init();
    void AdvanceLag();

    VALUE SparseRowLag();
    VALUE SparseColumnLag();
    VALUE DenseRowLag()  {
        LastWasRow= 1;
        RowMultiply(RowLag(), mt, Row);
        return Product(Row(), ColumnLag());
    };

    VALUE DenseColumnLag()  {
        LastWasRow= 0;
        ColumnMultiply(ColumnLag(), mt, Column);
        AdvanceLag();
        return Product(RowLag(), ColumnLag());
    };

    VALUE ComputeLag()  {
        if (NonZero.count() < LongLength)    // do sparse lag
            return LastWasRow ? SparseColumnLag() : SparseRowLag();
        if (!LongInit)  {
            if (LastWasRow) return SparseColumnLag();  // last time do sparse
            for (INDEX cnt= 0; cnt < Dim; ++cnt)
                if (OrderLag[cnt] != LastOrder)
                    RowLag[cnt]= ColumnLag[cnt]= 0;
            LongInit= 1;
        };
        return LastWasRow ? DenseColumnLag() : DenseRowLag();
    };

private :
        WVector           RowLag, Row, ColumnLag, Column;
    Vector<INDEX>     OrderLag, Order, NonZero;
    bool              LastWasRow, LongInit;
    INDEX             LastOrder, Dim, LongLength;
    Iterator<WMap>    mt;
};

#endif



