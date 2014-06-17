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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "mix.h"
#include "Lite2.h"

#include "PowerLag.h"


/*
PowerLag
 */
//template <class R>
PowerLag::PowerLag(const Iterator<WMap> matrix, const INDEX vsize) : 
RowLag(vsize), Row(vsize), ColumnLag(vsize), Column(vsize),
OrderLag(vsize, 0), Order(vsize, 0), NonZero(vsize),
LastWasRow(false), LongInit(0), LastOrder(1), Dim(vsize), LongLength(vsize/2), 
mt(matrix)  {};


/*
PowerLag
 */
//template <class R>
VALUE PowerLag::Init()  {
    VALUE TmpResult;
    VALUE   Product= 0;
    Iterator<WPair>  it;

    for (it= mt[Dim](); it; ++it)  {			// sic! mt[ Dim ]
        WPair      Neighbor= *it;
        OrderLag[Neighbor.first]= 1;
        Order[Neighbor.first]= 1;
        NonZero << Neighbor.first;
        RowLag[Neighbor.first]= Neighbor.second;
        TmpResult=  (*mt[Neighbor.first]).second;
        ColumnLag[Neighbor.first]= TmpResult;
        Product += TmpResult * Neighbor.second;
    };
    return Product;
};

/*
PowerLag
 */
//template <class R>
void PowerLag::AdvanceLag()  {
    RowLag.Swap(Row);  ColumnLag.Swap(Column);
    OrderLag.Swap(Order);
    ++LastOrder;
    return;
};

/*
PowerLag
 */
//template <class R>
VALUE PowerLag::SparseRowLag()  {
    LastWasRow= 1;
    INDEX NewLag= LastOrder + 1, Current;
    Iterator<INDEX> SaveLast= NonZero();
    Iterator<WPair>	it;
    Iterator<INDEX>	Inz;
    for (Inz= SaveLast; Inz; ++Inz)
        if (OrderLag[Current= *Inz] == LastOrder)  {
            VALUE     Nbr= RowLag[Current];
            for (it= mt[Current](); it; ++it)  {
                INDEX  IxNbr= (*it).first, OrderIxNbr= Order[IxNbr];
                if (OrderIxNbr == NewLag) Row[IxNbr] += (*it).second * Nbr;
                else  {
                    Row[IxNbr] = (*it).second * Nbr;
                    if (OrderIxNbr == 0 && OrderLag[IxNbr] == 0)
                        NonZero << IxNbr;
                    Order[IxNbr]= NewLag;
                };
            };
        };
    VALUE Product= 0;
    for (Inz= SaveLast; Inz; ++Inz)  {
        Current= *Inz;
        if (OrderLag[Current] == LastOrder && Order[Current] == NewLag)
            Product += ColumnLag[Current] * Row[Current];
    };
    return Product;
};

/*
PowerLag
 */
//template <class R>
VALUE PowerLag::SparseColumnLag()  {
    LastWasRow= 0;
    INDEX NewLag= LastOrder + 1, Current;
    VALUE   Product= 0;
    Iterator<WPair>       it;
    for (Iterator<INDEX> Inz= NonZero(); Inz; ++Inz)
        if (Order[Current= *Inz] == NewLag)  {
            VALUE   TmpResult= 0;
            for (it= mt[Current](); it; ++it)  {
                INDEX    IxNbr= (*it).first;
                if (OrderLag[IxNbr] == LastOrder)
                    TmpResult += ColumnLag[IxNbr] * (*it).second;
            };
            Column[Current] = TmpResult;
            Product += TmpResult * Row[Current];
        };
    AdvanceLag();
    return Product;
};


