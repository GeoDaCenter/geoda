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

#include "GalWeight.h"
#include "RateSmoothing.h"
#include "../logger.h"

bool GeoDaAlgs::RateStandardizeEB(const int obs, const double* P,
								  const double* E, double* m_results,
								  std::vector<bool>& undefined)
{
	if (undefined.size() != obs) undefined.resize(obs);
	for (int i=0; i<obs; i++) undefined[i] = false;
	
	double	sP=0.0, sE=0.0;
	double* p = new double[obs];
	int i = 0;
	// compute pi, the rate i, and the pop. rate b_hat
	for (i=0; i<obs; i++) {
		sP += P[i];
		sE += E[i];
		p[i] = (P[i] != 0.0) ? E[i] / P[i] : -9999;
	}
	
	if (sP == 0.0) {
		delete [] p;
		return false;
	}
	
	const double b_hat = sE / sP;
	
	// compute a_hat, the variance
	double gamma=0.0;
	for (i=0; i< obs; i++) 
		if (p[i] != -9999)
			gamma += P[i] * ((p[i] - b_hat) * (p[i] - b_hat));
	
	double a = (gamma / sP) - (b_hat / (sP / obs));
	const double a_hat = a > 0 ? a : 0.0;
	
	for (i=0; i< obs; i++) {
		const double se = P[i] > 0 ? sqrt(a_hat + b_hat/P[i]) : 0.0;
		m_results[i] = 0.0;
		if (p[i] != -9999)
			m_results[i] = se > 0 ? (p[i] - b_hat) / se : 0.0;
	}
	delete [] p;
	return true;
}

void GeoDaAlgs::RateSmoother_RawRate(int obs, double *P, double *E,
									 double *m_results,
									 std::vector<bool>& undefined)
{
	if (undefined.size() != obs) undefined.resize(obs);
	for (int i=0; i<obs; i++) undefined[i] = false;
	
	double SP=0, SE=0;
	for (int i=0;i<obs;i++) {
		SP += P[i];
		SE += E[i];
		m_results[i] = 0;
		if (P[i]>0) m_results[i] = E[i]/P[i];
	}
}

void GeoDaAlgs::RateSmoother_ExcessRisk(int obs, double *P, double *E,
										double *m_results,
										std::vector<bool>& undefined)
{
	if (undefined.size() != obs) undefined.resize(obs);
	for (int i=0; i<obs; i++) undefined[i] = false;

	double SP=0, SE=0;
	for (int i=0; i<obs; i++) {
		SP += P[i];
		SE += E[i];
	}
	double lambda=1;
	if (SP>0) lambda = SE/SP;
	for (int i=0; i<obs; i++) 
	{
		double E_hat = P[i] * lambda;
		if (E_hat > 0) {
			m_results[i] = E[i] / E_hat;
		} else {
			m_results[i] = -1;
			undefined[i] = true;
		}
	}
}

void GeoDaAlgs::RateSmoother_EBS(int obs, double *P, double *E,
								 double *m_results,
								 std::vector<bool>& undefined)
{
	if (undefined.size() != obs) undefined.resize(obs);
	for (int i=0; i<obs; i++) undefined[i] = false;
	
	double* pi_raw = new double[obs];
	double SP=0, SE=0;
	int i = 0;
	for (i=0; i<obs; i++) {
		SP += P[i];
		SE += E[i];
		pi_raw[i] = 0;
		if (P[i]>0) pi_raw[i] = E[i]/P[i];
	}
	
	double theta1=1, theta2;
	if (SP>0) theta1 = SE/SP;
	double pbar = SP / obs;
	double q1=0, w;
	for (i=0; i<obs; i++) { 
		q1 += P[i]*(pi_raw[i]-theta1)*(pi_raw[i]-theta1);
	}
	theta2 = (q1/SP) - (theta1/pbar);
	
	if (theta2 < 0) theta2 = 0.0;
	// MMM: we should display a warning dialog when the
	// estimate for the variance, thata2, is negative
	for (i=0; i<obs; i++) {
		q1 = (theta2 + (theta1/P[i]));
		w = (q1 > 0) ? theta2 / q1 : 1;
		m_results[i] = (w * pi_raw[i]) + ((1-w) * theta1);
	}
	delete [] pi_raw;
}


bool GeoDaAlgs::RateSmoother_SEBS(int obs, GalElement* m_gal, double *P,
								  double *E, double *m_results,
								  std::vector<bool>& undefined)
{
	if (undefined.size() != obs) undefined.resize(obs);
	for (int i=0; i<obs; i++) undefined[i] = false;
	bool has_undefined = false;
	
	double* pi_raw = new double[obs];
	for (int i=0; i<obs; i++) {
		pi_raw[i]=1;
		if (P[i]>0) pi_raw[i] = E[i]/P[i];
	}
	
	for (int i=0; i<obs; i++) {
		int  nbr = m_gal[i].Size();
		long* dt = m_gal[i].dt();
		
		double SP=P[i], SE=E[i];
		
		for (int j=0; j<nbr; j++) {
			SP += P[dt[j]];
			SE += E[dt[j]];
		}
		
		double theta1=1, theta2;
		if (SP>0) theta1 = SE/SP;
		
		if (nbr > 0) {
			double pbar = SP / (nbr + 1);
			
			double q1 = P[i] *  (pi_raw[i] - theta1)* (pi_raw[i] - theta1);
			double w;
			for (int j=0; j<nbr; j++) {
				q1 += P[dt[j]] * 
				(pi_raw[dt[j]] - theta1)*
				(pi_raw[dt[j]] - theta1);
			}
			
			theta2 = (q1/SP) - (theta1/pbar);
			if (theta2 < 0) theta2 = 0.0;
			q1 = (theta2 + (theta1/P[i]));
			w = (q1 > 0) ? theta2 / q1 : 1;
			m_results[i] = (w * pi_raw[i]) + ((1-w) * theta1);
		} else {
			has_undefined = true;
			undefined[i] = 0;
			m_results[i] = 0;
		}
	}
	delete [] pi_raw;
	return has_undefined;
}

bool GeoDaAlgs::RateSmoother_SRS(int obs, GalElement* m_gal, double *P,
								 double *E, double *m_results,
								 std::vector<bool>& undefined)
{
	if (undefined.size() != obs) undefined.resize(obs);
	for (int i=0; i<obs; i++) undefined[i] = false;
	bool has_undefined = false;
	
	double SE = 0, SP=0;
	for (int i=0; i<obs; i++) {
		SE = 0; SP=0;
		long* dt = m_gal[i].dt();
		for (int j=0; j<m_gal[i].size; j++) {
			SE += E[dt[j]];
			SP += P[dt[j]];
		}
		m_results[i] = -9999;
		if ((P[i] + SP)>0) {
			m_results[i] = (E[i] + SE) / (P[i] + SP);
		}
		if (m_gal[i].size <= 0) {
			has_undefined = true;
			undefined[i] = true;
			m_results[i] = 0;
		}
	}
	return has_undefined;
}
