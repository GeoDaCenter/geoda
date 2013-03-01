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
#include "Statistics.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DynStatistics::DynStatistics(double* dt, long obs)
{
	data = dt;
	size = obs;
	sdata = new double[obs];
}

DynStatistics::~DynStatistics()
{
	if (sdata) {
		delete [] sdata;
		sdata = NULL;
	}
}


void DynStatistics::ComputeStats()
{
	double sum = 0.0,x0,x2;
	for (int i=0; i<size; i++) {
		sdata[i] = data[i];
		sum += data[i];
	}
	mean = sum / size;
	sum =0.0;
	for (int i=0; i<size; i++) {
		x0 = data[i]-mean;
		x2 = x0 * x0;
		sum +=x2;
	}
	sdev = sqrt(sum / (size-1));

	max = sdata[0];
	min = sdata[0];
	for (int i=1;i <size;i++) {
		if (max < sdata[i])
			max = sdata[i];
		if (min > sdata[i])
			min = sdata[i];
	}
}

void DynStatistics::QSortQ(double* array, int low, int high)
{
	int i, j;
	Sort(array[low], array[(low+high)/2], array[high]);
	if ((high - low) > 2) {
		Swap(array[low+1], array[(low+high)/2]);
		i = low + 1; j = high;
		while (i < j) {
			i++; while(array[i] < array[low+1]) i++;
			j--; while(array[j] > array[low+1])j--;
			Swap(array[i],array[j]);
		}
		Swap(array[i], array[j]);
		Swap(array[low+1], array[j]);
		QSortQ(array,low, j-1);
		QSortQ(array, j+1, high);
	}

}

void DynStatistics::Sort(double &i, double &j, double &k)
{
	if (i > j) {
		Swap (i, j);
		if (j > k) Swap (j, k);
		if (i > j) Swap (i, j);
	}
	else {
		if (j > k) Swap (j, k);
		if (i > j) Swap(i, j);
	}

}

void DynStatistics::Swap( double &x, double &y)
{
	double temp = x;
	x = y;
	y = temp;
}
