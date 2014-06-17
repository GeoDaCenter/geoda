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

#ifndef __GEODA_CENTER_STATISTICS_H__
#define __GEODA_CENTER_STATISTICS_H__

class DynStatistics  
{
	double *data;
	double *sdata;
	long	size;
	double	mean;
	double	sdev;
	double	max;
	double	min;
	double	mode;
	double	median;
protected:
public:
	DynStatistics(double *dt, long obs);
	virtual ~DynStatistics();
	double Mean() { return mean; };
	double StDev() { return sdev; };
	double* SortedData() { return sdata; };
	double Max() { return max; };
	double Min() { return min; };
	double Mode() { return mode; };
	double Median() { return median; };
	void ComputeStats();
private:
	void QSortQ(double* dt, int lower, int upper);
	void Sort(double &i, double &j, double &k);
	void Swap( double &x, double &y);


};

#endif
 
