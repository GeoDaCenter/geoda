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

#ifndef __GEODA_CENTER_SMILE_H__
#define __GEODA_CENTER_SMILE_H__

/*
*   OLS computes Ordinary Least Squares estimates and places output in result
*/
bool OLS(double * Y, int dim, double ** X, int expl, double * result,
		 bool InclConstant);

/*
        ML estimation of spatial lag model.
 Returns pairs of coefficient and their std errors -- total (expl + 1) pairs.
 The order of pairs corresponds to the order of explanatory variables.
 The spatial autoregressive coefficient and its std. error are reported last.
 */
int SpatialLag(GalElement *g, double * Y, int dim, double ** X, int expl,
			   double * result, bool InclConstant);

/*
ML estimation of spatial error model.
 Returns pairs of coefficient and their std errors -- total (expl + 1) pairs.
 The order of pairs corresponds to the order of explanatory variables.
 The spatial autoregressive coefficient and its std. error are reported last.
 */
int SpatialError(GalElement *g, double * Y, int dim, double ** X, int expl,
				 double * result, bool InclConstant);

#endif

