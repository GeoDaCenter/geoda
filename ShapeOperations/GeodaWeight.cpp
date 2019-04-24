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

#include <wx/filename.h>
#include "GeodaWeight.h"

GeoDaWeight::GeoDaWeight(const GeoDaWeight& gw)
{
	GeoDaWeight::operator=(gw);
}

const GeoDaWeight& GeoDaWeight::operator=(const GeoDaWeight& gw)
{
	weight_type = gw.weight_type;
	wflnm = gw.wflnm;
	title = gw.title;
	symmetry_checked = gw.symmetry_checked;
	is_symmetric = gw.is_symmetric;
	num_obs = gw.num_obs;
	
	return *this;
}

wxString GeoDaWeight::GetTitle()  const
{
	if (!title.IsEmpty()) return title;
	return wxFileName(wflnm).GetName();
}

double GeoDaWeight::GetSparsity() const
{
    return sparsity;
}
double GeoDaWeight::GetDensity() const
{
    return density;
}
int GeoDaWeight::GetNumObs() const
{
    return num_obs;
}

int GeoDaWeight::GetMinNumNbrs() const
{
    return min_nbrs;
}
int GeoDaWeight::GetMaxNumNbrs() const
{
    return max_nbrs;
}
double GeoDaWeight::GetMeanNumNbrs() const
{
    return mean_nbrs;
}
double GeoDaWeight::GetMedianNumNbrs() const
{
    return median_nbrs;
}
