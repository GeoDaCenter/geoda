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

#ifndef __GEODA_CENTER_DENSE_MATRIX_H__
#define __GEODA_CENTER_DENSE_MATRIX_H__

/*  ---  DenseMatrix  ---  */
class  DenseMatrix 
{
public:
    DenseMatrix(const int r, const int c);
    DenseMatrix(double **array, const int r, const int c);
    void timesColumn(DenseVector &r, const DenseVector &v)  const;
private :
    void init(const int r, const int c);
    void clear();
    int rows, columns;
    DenseVector * row;
};
#endif

