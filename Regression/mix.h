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

#ifndef __GEODA_CENTER_MIX_H__
#define __GEODA_CENTER_MIX_H__

#include <iostream>
#include <math.h>
#include <fstream>
#include <utility>

using namespace std;

const double ML_SMALL = 1.0e-14;
const double GoldenRatio = (sqrt((double) 5)-1)/2, GoldenToo = 1 - GoldenRatio;
const double TOLERANCE = 1.0e-14;

// standard normal density function
double ndf(double t);
// standard normal cumulative distribution function
double nc(double x);

/*
template <class T>
inline void swap(T & a, T & b)  {
    T c = a;
    a = b;
    b = c;
}
*/

void error(const char *s, const char *s2 = NULL);

template <class X>
inline void release(X** p)  
{
	if ((*p)) delete [] (*p);
	(*p) = NULL;
}

template <class X>
inline void alloc(X * (&p), const int sz)  
{
	p = new X[sz];
	for (int cnt = 0; cnt < sz; ++cnt) p[cnt] = 0;
}

template <class X>
inline void alloc(X * (&p), const int sz, X init)  {
	p = new X[sz];
	for (int cnt = 0; cnt < sz; ++cnt) p[cnt] = init;
}


double product(const double * v1, const double * v2, const int &sz);

double norm(const double *v, const int size);

// compute standard normal deviate using Box-Muller  transformation
double gauss();

double normW(const double *v, const double *w, const int size);

// compute incomplete gamma function -- numerical resipies, p. 172-4
double gammp(const double a, const double x);


/*
HeapSort
Sorts vector in the ascending order, using HeapSort.
Assumes operator < is defined and operator = is implemented.
For basic idea see Numerical Recipes in C., p.247.
 */
template <class T>
void HeapSort(T * start, const int size)  
{
    const int  half= size >> 1;
    if (half == 0) return;        // nothing to sort;
                                  //    T * start = v.first();
    T * hire= start + half, * boss, * empl;
    // hire will be decremented down to begin during the 'hiring' (heap creation).
    T * ir= &start[size-1];		// the last element in the sequence
    T rra;
    while (ir > start)  {
        if (hire > start)  rra= *(--hire);  // still hiring
        else  {                    // retirement and promotion
            rra= *ir;               // clear space at end of vector
            *ir-- = *start;         // retire the top of the heap into it
        };
        boss= hire;            // set up to sift down rra to its proper level
        empl= boss + (boss - start);
        while (++empl <= ir)  {
            if (empl < ir && *empl < *(empl+1)) ++empl;
            if (rra < *empl)  {         // demote rra
                *boss= *empl;
                boss= empl;
                empl= boss + (boss - start);
            }
            else  empl= ir;   // rra is already in place
        };
        *boss= rra;            // i is right place for rra
    }
}

// uses dictionary (array of pairs sorted in the in creasing order of key) to find
// value for the matching key
template <class K, class W>
W Find(pair<K,W> *start, const int size, const K &key)  {
    int mid = size / 2;
    int lower = 0, upper = size-1;
    while (start[mid].first != key)  {
        if (start[mid].first < key)  lower = mid + 1;
        else if (start[mid].first > key)  upper = mid - 1;
        else return start[mid].second;
        mid = (lower + upper) / 2;
    };
    return start[mid].second;
}
#endif


