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

#ifndef __GEODA_CENTER_POWER_SYM_LAG_H__
#define __GEODA_CENTER_POWER_SYM_LAG_H__

class PowerSymLag  {
public :
    PowerSymLag(const Iterator<WMap> matrix, const INDEX vsize);
    VALUE Init();
    void AdvanceLag();

    VALUE SparseRowLag();
    VALUE SparseColumnLag();
    VALUE DenseRowLag()  {
        LastWasRow= 1;
        RowMultiply(RowLag(), mt, Row);
        return Product(Row(), RowLag());
    };

    VALUE DenseColumnLag()  {
        LastWasRow= 0;
        AdvanceLag();
        return Product(RowLag(), RowLag());
    };

    VALUE ComputeLag()  {
        if (NonZero.count() < LongLength)    // do sparse lag
            return LastWasRow ? SparseColumnLag() : SparseRowLag();
        if (!LongInit)  {
            if (LastWasRow) return SparseColumnLag();  // last time do sparse
            for (INDEX cnt= 0; cnt < Dim; ++cnt)
                if (OrderLag[cnt] != LastOrder)
                    RowLag[cnt]= 0;
            LongInit= 1;
        };
        return LastWasRow ? DenseColumnLag() : DenseRowLag();
    };

private :
        WVector           RowLag, Row;
    Vector<INDEX>     NonZero;
    Vector<unsigned char>      OrderLag, Order;
    bool              LastWasRow, LongInit;
    INDEX             LastOrder, Dim, LongLength;
    Iterator<WMap>    mt;
};

#endif


