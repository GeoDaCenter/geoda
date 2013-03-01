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

/* This is an interface file for the random generator class.
Class Randik designed to generate float psudo-random numbers 0 <= x < 1.
Algorithm taken from Knuth, Donald E. 1981. The Art.., vol 2, 3.2-3.3.
*/

#ifndef __GEODA_CENTER_RANDIK_H__
#define __GEODA_CENTER_RANDIK_H__

class Randik
{
public:
    Randik();
    virtual ~Randik();
    float fValue() { // return float random value from [0, 1)
		Iterate();
		const float FC = (float) 1.0/MBIG;
		return cohort[current] * FC;
    }
    long lValue() { // return int random from 0 to MBIG-1
		Iterate();
		return cohort[current];
    }
	// return random permutation of 1...size
    int* Perm(const int size);
	// same as above, except for permutation vector is returned in
	// thePermutation and theRands is passed in as scratch.  Both
	// vectors must be fully allocated unlike the above which
	// allocates memory and must be deleted by caller.
	bool Perm(const int size, int* thePermutation, long* theRands);
	void PermG(const int size, int* thePermutation);  
private:
    enum {
        cohortStep = 21,
        cohortSize = 55,
        MBIG  = 1000000000,
        MSEED = 161803398,
    } constants;
    int    current;
    long*  cohort;
    void Initialize(const long Seed);   // seed the RNG
    void Iterate();                     // next number
};

#endif
