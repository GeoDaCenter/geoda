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

#include <math.h>
#include "GalWeight.h"
#include "RateSmoothing.h"

bool GdaAlgs::RateStandardizeEB(const int obs, const double* P,
								  const double* E, double* results,
								  std::vector<bool>& undefined)
{
    bool has_undef = false;
	double	sP=0.0, sE=0.0;
	double* p = new double[obs];
	int i = 0;
    
	// compute pi, the rate i, and the pop. rate b_hat
	for (i=0; i<obs; i++) {
        if (undefined[i]) {
            p[i] = 0;
            continue;
        }
        
		if (P[i] == 0.0) {
			undefined[i] = true;
			p[i] = 0;
		} else {
			sP += P[i];
			sE += E[i];
			p[i] = E[i] / P[i];
		}
	}
	
	if (sP == 0.0) {
		delete [] p;
		for (int i=0; i<obs; i++) {
			undefined[i] = true;
			results[i] = 0;
		}
	}
	
	const double b_hat = sE / sP;
	
	// compute a_hat, the variance
    double obs_valid = 0.0;
	double gamma=0.0;
	for (i=0; i< obs; i++) {
		if (!undefined[i]) {
			gamma += P[i] * ((p[i] - b_hat) * (p[i] - b_hat));
            obs_valid += 1;
		}
        has_undef = true;
	}
	
	double a = (gamma / sP) - (b_hat / (sP / obs_valid));
	const double a_hat = a > 0 ? a : 0.0;
	
	for (i=0; i<obs; i++) {
		results[i] = 0.0;
		if (!undefined[i]) {
            const double se = P[i] > 0 ? sqrt(a_hat + b_hat/P[i]) : 0.0;
			results[i] = se > 0 ? (p[i] - b_hat) / se : 0.0;
		}
	}
	delete [] p;
	return has_undef;
}

bool GdaAlgs::RateSmoother_RawRate(int obs, double *P, double *E,
									 double *results,
									 std::vector<bool>& undefined)
{
    bool has_undef = false;
	double SP=0, SE=0;
	for (int i=0;i<obs;i++) {
        if (undefined[i]) {
            results[i] = 0;
            has_undef = true;
            continue;
        }
        
		SP += P[i];
		SE += E[i];
		results[i] = 0;
		if (P[i]>0) {
			results[i] = E[i]/P[i];
		} else {
            results[i] = 0;
			undefined[i] = true;
            has_undef = true;
		}
	}
    return has_undef;
}


bool GdaAlgs::RateSmoother_ExcessRisk(int obs, double *P, double *E,
										double *results,
										std::vector<bool>& undefined)
{
    bool has_undef = false;
    
	double SP=0, SE=0;
	for (int i=0; i<obs; i++) {
        if (undefined[i])
            continue;
		SP += P[i];
		SE += E[i];
	}
	double lambda=1;
	if (SP>0)
        lambda = SE/SP;
    
	for (int i=0; i<obs; i++) {
        if (undefined[i]) {
            results[i] = 0;
            has_undef = true;
            continue;
        }
        double E_hat = P[i] * lambda;
		if (E_hat > 0) {
			results[i] = E[i] / E_hat;
		} else {
			results[i] = 0;
            has_undef = true;
			undefined[i] = true;
		}
	}
    return has_undef;
}

bool GdaAlgs::RateSmoother_EBS(int obs, double *P, double *E,
								 double *results,
								 std::vector<bool>& undefined)
{
    bool has_undef = false;
    
	double* pi_raw = new double[obs];
	double SP=0, SE=0;
	int i = 0;
    int valid_obs = 0;
	for (i=0; i<obs; i++) {
        if (undefined[i]) {
            results[i] = 0;
            pi_raw[i] = 0;
            has_undef = true;
            continue;
        }
        valid_obs += 1;
		SP += P[i];
		SE += E[i];
		pi_raw[i] = 0;
		if (P[i]>0) {
			pi_raw[i] = E[i]/P[i];
		} else {
			undefined[i] = true;
			results[i] = 0;
            has_undef = true;
		}
	}
	
	double theta1=1, theta2;
	if (SP>0)
        theta1 = SE/SP;
    
	double pbar = SP / valid_obs;
	double q1=0, w;
	for (i=0; i<obs; i++) {
		if (!undefined[i]) {
			q1 += P[i]*(pi_raw[i]-theta1)*(pi_raw[i]-theta1);
		}
	}
	theta2 = (q1/SP) - (theta1/pbar);
	
	if (theta2 < 0) theta2 = 0.0;
	// MMM: we should display a warning dialog when the
	// estimate for the variance, thata2, is negative
	for (i=0; i<obs; i++) {
		if (!undefined[i]) {
			q1 = (theta2 + (theta1/P[i]));
			w = (q1 > 0) ? theta2 / q1 : 1;
			results[i] = (w * pi_raw[i]) + ((1-w) * theta1);
		}
	}
	delete [] pi_raw;
    return has_undef;
}


bool GdaAlgs::RateSmoother_SEBS(int obs, WeightsManInterface* w_man_int,
								boost::uuids::uuid weights_id,
								double *P, double *E,
								double *results, std::vector<bool>& undefined)
{
	//if (undefined.size() != obs) undefined.resize(obs);
	//for (int i=0; i<obs; i++) undefined[i] = false;
    
	bool has_undefined = false;
    for (int i=0; i<obs; i++) {
        if (undefined[i]) {
            has_undefined = true;
            break;
        }
    }
    GalElement* gal = NULL;
    GalWeight* gw  = NULL;
    
    if (has_undefined) {
        gw = new GalWeight(*w_man_int->GetGal(weights_id));
        gw->Update(undefined);
        gal = gw->gal;
    } else {
        gal = w_man_int->GetGal(weights_id)->gal;
    }

	
	double* pi_raw = new double[obs];
	for (int i=0; i<obs; i++) {
		pi_raw[i]=1;
        if (undefined[i]) {
            results[i] = 0;
            continue;
        }
		if (P[i]>0) {
			pi_raw[i] = E[i]/P[i];
		} else {
            results[i] = 0;
			undefined[i] = true;
		}
	}
	
	for (int i=0; i<obs; i++) {
		if (undefined[i])
            continue;
        
		int  nbrs = gal[i].Size();
		GalElement& elt_i = gal[i];
		
		double SP=P[i], SE=E[i];
		
		for (int j=0; j<nbrs; j++) {
			SP += P[elt_i[j]];
			SE += E[elt_i[j]];
		}
		
		double theta1=1, theta2;
		if (SP>0) theta1 = SE/SP;
		
		if (nbrs > 0) {
			double pbar = SP / (nbrs + 1);
			
			double q1 = P[i] *  (pi_raw[i] - theta1)* (pi_raw[i] - theta1);
			double w;
			for (int j=0; j<nbrs; j++) {
				if (undefined[elt_i[j]]) {
					undefined[i] = true;
				} else {
					q1 += P[elt_i[j]] * 
					(pi_raw[elt_i[j]] - theta1)*
					(pi_raw[elt_i[j]] - theta1);
				}
			}
			if (!undefined[i]) {
				theta2 = (q1/SP) - (theta1/pbar);
				if (theta2 < 0) theta2 = 0.0;
				q1 = (theta2 + (theta1/P[i]));
				w = (q1 > 0) ? theta2 / q1 : 1;
				results[i] = (w * pi_raw[i]) + ((1-w) * theta1);
			}
		} else {
			undefined[i] = true;
			results[i] = 0;
		}
	}
	delete [] pi_raw;
   
    if (has_undefined) {
        if (gw) {
            delete gw;
        }
    }
    
    for (int i=0; i<obs; ++i) {
        if (undefined[i]) {
            has_undefined = true;
        }
    }
    
	return has_undefined;
}

bool GdaAlgs::RateSmoother_SRS(int obs, WeightsManInterface* w_man_int,
							   boost::uuids::uuid weights_id,
							   double *P, double *E,
							   double *results, std::vector<bool>& undefined)
{
	//if (undefined.size() != obs) undefined.resize(obs);
	//for (int i=0; i<obs; i++) undefined[i] = false;
    
	bool has_undefined = false;
    for (int i=0; i<obs; i++) {
        if (undefined[i]) {
            has_undefined = true;
            break;
        } 
    }
    GalElement* gal = NULL;
    GalWeight* gw  = NULL;
    
    if (has_undefined) {
        gw = new GalWeight(*w_man_int->GetGal(weights_id));
        gw->Update(undefined);
        gal = gw->gal;
    } else {
        gal = w_man_int->GetGal(weights_id)->gal;
    }
	
	double SE = 0, SP=0;
	for (int i=0; i<obs; i++) {
		SE = 0; SP=0;
		results[i] = 0;
       
        if (undefined[i])
            continue;
        
		const GalElement& elm_i = gal[i];
		for (int j=0, sz=elm_i.Size(); j<sz; j++) {
			SE += E[elm_i[j]];
			SP += P[elm_i[j]];
		}
		if ((P[i] + SP)>0) {
			results[i] = (E[i] + SE) / (P[i] + SP);
		} else {
			undefined[i] = true;
            results[i] = 0;
		}
		if (gal[i].Size() <= 0) {
			undefined[i] = true;
			results[i] = 0;
		}
	}
  
    if (has_undefined) {
        if (gw) {
            delete gw;
        }
    }
    for (int i=0; i<obs; ++i) {
        if (undefined[i]) {
            has_undefined = true;
            break;
        }
    }
	return has_undefined;
}
