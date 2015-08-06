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

#include "DistancesCalc.h"

UnOrdIntPair::UnOrdIntPair()
{
	i = -1;
	j = -1;
}

UnOrdIntPair::UnOrdIntPair(int i_, int j_)
{
	if (i_ < j_) {
		i=i_;
		j=j_;
	} else {
		i=j_;
		j=i_;
	}
}

bool UnOrdIntPair::operator<(const UnOrdIntPair& s) const
{
	int a_i = (i < j ? i : j);
	int a_j = (i < j ? j : i);
	int b_i = (s.i < s.j ? s.i : s.j);
	int b_j = (s.i < s.j ? s.j : s.i);
	
	if (a_i != b_i) return a_i < b_i;
	return a_j < b_j;
}

bool UnOrdIntPair::operator==(const UnOrdIntPair& s) const
{
	return (((i==s.i) && (j==s.j)) ||
					((i==s.j) && (j==s.i)));
}

wxString UnOrdIntPair::toStr()
{
	wxString s;
	s << "(" << i << "," << j << ")";
	return s;
}
