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

#include <time.h>
#include "Randik.h"

//*** Constructor for class Randik
//*** Critical issue: a method generating a seed for the
//                    random numbers generator.
//*** Initialize member values
Randik::Randik() : current(0), cohort(NULL) 
{
	cohort = new long [ cohortSize ];
	if (cohort == NULL)  return;
	int  aPerfectNumber = 0;
	do {
		time_t     timer= time( NULL );        // get a unique number
		timer &= 0xffffff;                     // preserve a portion of it
		aPerfectNumber = -timer;               // it is a negative number now
	}
	while (aPerfectNumber == 0);             // sort out pathological zeros
	Initialize( aPerfectNumber );            // initialize RNG with the seed
}

//** member function Initialize()
//***  the seed has to be negative
void Randik::Initialize(const long Seed)  {
	long mj, mk;
	mj = MSEED + Seed;
	mj %= MBIG;
	cohort[0] = mj;
	mk= 1;
	int  i = cohortStep;
	do {
		cohort[i] = mk;
		mk = mj - mk;
		if (mk < 0) mk += MBIG;
		mj = cohort[i];
		i += cohortStep;
		if (i >= cohortSize) i -= cohortSize;
	}
	while (i);
	int  k = 31;
	for (int cnt = 4; cnt > 0; --cnt)
		do  {
			if (++i == cohortSize) i= 0;
			if (++k == cohortSize) k= 0;
			cohort[i] -= cohort[k];
			if (cohort[i] < 0) cohort[i] += MBIG;
		}
	while (i);
	current = 0;
}

//** destructor
Randik::~Randik()
{
	if (cohort) delete [] cohort; cohort = NULL;
}

//** Randik::Iterate()  Sign value to current and cohort[current] 
void Randik::Iterate()  {
	if (++current == cohortSize) current = 0;  // next element in the cohort
	int k = current - (cohortSize - cohortStep);
	if (k < 0) k += cohortSize;
	long      mj = cohort[ current ] - cohort[ k ];
	if (mj < 0) mj += MBIG;
	cohort[ current ] = mj;
}

inline void swapint(int &x, int &y)  
{
	int z = x;
	x = y;
	y = z;
}


//** IndexSort(): Sort array value. Algorithm: QuickSort
void IndexSort(const long * value, int * index,
			   const int lower, const int upper)  {
	int   i= lower, j= upper;
	const long    pivot= value[index[(lower+upper) >> 1]];
	do  {
		while (value[index[i]] < pivot) ++i;
		while (value[index[j]] > pivot) --j;
		if (i < j)  swapint(index[i], index[j]);
		if (i <= j)  { ++i;  --j;  };
	}
	while (j >= i);
	if (j > lower) IndexSort(value, index, lower, j);
	if (i < upper) IndexSort(value, index, i, upper);
}

//***  Generate a random permutation of 1...size
//***
int* Randik::Perm(const int size)  
{
	int* thePermutation = new int [ size ];
	long* theRands = new long [ size ];
	if (thePermutation == NULL || theRands == NULL)  {
		if (thePermutation) delete [] thePermutation;
		if (theRands) delete [] theRands;
		return NULL;
	}
	bool    permOk = true;
	do  {
		int cnt;
		for (cnt= 0; cnt < size; ++cnt)   // original permutation -- 0 permuts
			thePermutation[ cnt ] = cnt;
		for (cnt= 0; cnt < size; ++cnt)   // generate size random numbers
			theRands[ cnt ] = lValue();
		IndexSort(theRands, thePermutation, 0, size-1);
		int     thePrevious= thePermutation[0];
		for (cnt = 1; cnt < size; ++cnt)  {  // ascending order is IMPORTANT
			const int theNext = thePermutation[cnt];
			if (thePrevious == theNext)  {
				permOk = false;       // bad, bad permutation
				break;
			}
			thePrevious= theNext;
		}
	}
	while (!permOk);                  // loop while the permutation is not good
	delete [] theRands;
	return thePermutation;
}

//***  Generate a random permutation of 1...size
//***
bool Randik::Perm(const int size, int* thePermutation, long* theRands)
{
	if (!thePermutation || !theRands) return false;
	bool permOk = true;
	do  {
		int cnt;
		for (cnt= 0; cnt < size; ++cnt)   // original permutation -- 0 permuts
			thePermutation[ cnt ] = cnt;
		for (cnt= 0; cnt < size; ++cnt)   // generate size random numbers
			theRands[ cnt ] = lValue();
		IndexSort(theRands, thePermutation, 0, size-1);
		int     thePrevious= thePermutation[0];
		for (cnt = 1; cnt < size; ++cnt)  {  // ascending order is IMPORTANT
			const int theNext = thePermutation[cnt];
			if (thePrevious == theNext)  {
				permOk = false;       // bad, bad permutation
				break;
			}
			thePrevious= theNext;
		}
	}
	while (!permOk);  // loop while the permutation is not good
	return true;
}

void Randik::PermG(const int size, int* thePermutation)  
{
	long * theRands = new long [ size ];
	
	bool    permOk = true;
	do {
		int cnt;
		for (cnt= 0; cnt < size; ++cnt)   // original permutation -- 0 permuts
			thePermutation[ cnt ] = cnt;
		for (cnt= 0; cnt < size; ++cnt)   // generate size random numbers
			theRands[ cnt ] = lValue();
		IndexSort(theRands, thePermutation, 0, size-1);
		int     thePrevious= thePermutation[0];
		for (cnt = 1; cnt < size; ++cnt)  {  // ascending order is IMPORTANT
			const int theNext = thePermutation[cnt];
			if (thePrevious == theNext) {
				permOk = false;       // bad, bad permutation
				break;
			}
			thePrevious= theNext;
		}
	}
	while (!permOk);                  // loop while the permutation is not good
	delete [] theRands;
}
