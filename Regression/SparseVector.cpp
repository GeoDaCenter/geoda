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
#include "SparseVector.h"

SparseVector::SparseVector(const int sz)

: size(0), nzEntries(0), val(NULL), nz(NULL), isNz(NULL)  {

    if (sz <= 0) error(" size of vector must be positive ");

    size = sz;

    nzEntries = 0;

    denseReady = true;

    val = new double[ sz ];

    nz = new int[ sz ];

    isNz = new bool[ sz ];

    if (!val || !nz || !isNz)

        error("fail to allocate");

    for (int cnt = 0; cnt < size; ++cnt)  {

        val[ cnt ] = 0;

        isNz[ cnt ] = false;

    };

}

SparseVector::~SparseVector()  {
    release(&val);
    release(&nz);
    release(&isNz);
}



// sets vector to zero

void SparseVector::checkout()  {

    for (int cnt = 0; cnt < nzEntries; ++cnt)  {

        isNz[ nz[cnt] ] = false;

        val[ nz[cnt] ] = 0;

    };

    nzEntries = 0;

}



void SparseVector::addTimes(const SparseVector &v, const double &w)  {

    for (int cnt = 0; cnt < v.getNzEntries(); ++cnt)  {

        int ix = v.getIx(cnt);

        this->plusAt( ix, v.getValue(ix) * w );

    }

}



void SparseVector::minus(const SparseVector &a, const SparseVector &b)  {

    reset();

    if (a.size > this->size || b.size > this->size)  {
        // vector size does not match
    };



    // process all indices that are non-zeros in a. some of them might have corresponding zeros in b.

    for (int ax = 0; ax < a.nzEntries; ++ax)  {

        int ix = a.nz[ax];

        this->setAt( ix, a.val[ix] - b.val[ix] );      

    };



    // process all indices for non-zeros in b. filter out those that have been already processed.

    for (int bx = 0; bx < b.nzEntries; ++bx)  {

        int ix = b.nz[bx];

        if (!a.isNz[ix])

            this->setAt( ix, -b.val[ix] );

    };



}





double SparseVector::norm()  const  {

    double scale = 0, ssq = 1, t;

    for (int cnt = 0; cnt < nzEntries; ++cnt)  {

        double absc = fabs(val[nz[cnt]]);

        if (scale < absc)  {

            t = scale / absc;

            ssq = 1.0 + ssq * t *t;

            scale = absc;

        }  else  if (absc > 0)  {

            t = absc / scale;

            ssq += t * t;

        };

    };

    return scale * scale * ssq;

}



double SparseVector::product(const SparseVector &a)  const  {

    double ss = 0;

    if (this->nzEntries > a.nzEntries)  return a.product(*this);

    for (int cnt = 0; cnt < this->nzEntries; ++cnt)  {

        int ix = this->nz[ cnt ];

        if ( a.isNz[ ix ] )

            ss += a.val[ ix ] * this->val[ ix ];

    };

    return ss;



}

void SparseVector::copy(const SparseVector &a)  {

    this->reset();

    for (int cnt = 0; cnt < a.nzEntries; ++cnt)  {
        int ix = a.nz[ cnt ];
        this->setAt( ix, a.val[ ix ] );
    };
}

// multiplies vector by a scalar s and adds vector a

void SparseVector::timesPlus(const SparseVector &a, const double &s)  {

    if (s != 1.0)

        for (int cnt = 0; cnt < this->nzEntries; ++cnt)  {

            this->val[ this->nz[ cnt ] ] *= s;

        };



    for (int cnt = 0; cnt < a.nzEntries; ++cnt)  {

        int ix = a.nz[ cnt ];

        this->plusAt( ix, a.val[ ix ] );

    };

}



void SparseVector::dropZeros()  {

    for (int cnt = 0; cnt < nzEntries; ++cnt)  {

        int ix = nz[cnt];

        if (fabs(val[ ix ]) < 1.0e-14)  {

            nz[cnt] = nz[--nzEntries];

            isNz[ix] = false;

            val[ix] = 0;

        };

    };

}

