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

#ifndef __GEODA_CENTER_ML_IM_H__
#define __GEODA_CENTER_ML_IM_H__

#include "DenseVector.h"
#include "SparseMatrix.h"

const int SMALL_DIM = 500;
const int ASYM_DIM = 1000;

double SimulationLag(const GalElement* weight,
					 int num_obs,
					 int	Precision, 
					 const double			rho, 
					 double*		my_Y,
					 double**		my_X,
					 const int				deps,
					 bool InclConstant,
					 double* Lik,
					 wxGauge* p_bar,
					 double p_bar_min_fraction,
					 double p_bar_max_fraction);  

double SimulationError(const GalElement* weight,
					   int num_obs,
					   int Precision, 
					   const double rho, 
					   const double* my_Y,
					   double** my_X,
					   const int deps,
					   double * &beta, 
					   bool InclConstant,
					   double* Lik,
					   wxGauge* p_bar,
					   double p_bar_min_fraction,
					   double p_bar_max_fraction);

bool OLS(DenseVector &y, DenseVector * X, const bool IncludeConst,
		 double ** &cov, double *resid, DenseVector &ols);

bool OLSS(DenseVector &y, DenseVector * X, const bool IncludeConst,
		  double ** &cov, double ** &ocov, double *resid, DenseVector &ols);

bool OLS(Iterator<VALUE> y, Iterator<WVector> X, const bool IncludeConst, 
				 Vector<WVector> &cov, WVector &resid, WVector &ols);

bool ordinaryLS(DenseVector &y, DenseVector * X, double ** &cov, 
				 double * resid, DenseVector &ols);

#endif

