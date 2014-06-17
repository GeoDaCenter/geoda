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

#ifndef __GEODA_CENTER_RATE_SMOOTHING_H__
#define __GEODA_CENTER_RATE_SMOOTHING_H__

#include <vector>
class GalElement;

namespace GdaAlgs {
	bool RateStandardizeEB(const int nObs, const double* P, const double* E,
						   double* m_results, std::vector<bool>& undefined);
	void RateSmoother_RawRate(int obs, double *P, double *E,
							  double *m_results, std::vector<bool>& undefined);
	void RateSmoother_ExcessRisk(int obs, double *P, double *E,
								 double *m_results,
								 std::vector<bool>& undefined);
	void RateSmoother_EBS(int obs, double *P, double *E,
						  double *m_results, std::vector<bool>& undefined);
	bool RateSmoother_SEBS(int obs, GalElement* m_gal, double *P, double *E,
						   double *m_results, std::vector<bool>& undefined);
	bool RateSmoother_SRS(int obs, GalElement* m_gal, double *P, double *E,
						  double *m_results, std::vector<bool>& undefined);
}

#endif
