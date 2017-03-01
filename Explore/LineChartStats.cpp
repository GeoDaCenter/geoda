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

#include <cmath>
#include <limits>
#include <math.h>
#include <boost/math/distributions/students_t.hpp>
#include <boost/math/distributions/fisher_f.hpp>
#include "../logger.h"
#include "LineChartStats.h"

LineChartStats::LineChartStats(const vec_vec_dbl_type& Y_,
                               const std::vector<bool>& Y_undef_,
                               const wxString& Yname_,
                               const std::vector<bool>& tms_subset0_,
                               const std::vector<bool>& tms_subset1_,
                               const bool& compare_regimes_,
                               const bool& compare_time_periods_,
                               const bool& compare_r_and_t_)
: Y(Y_),
Y_undef(Y_undef_),
Yname(Yname_), 
Y_avg_min(0), 
Y_avg_max(0),
Y_sel_avg_min(0),
Y_excl_avg_min(0),
Y_sel_avg_max(0),
Y_excl_avg_max(0),
tms_subset0(tms_subset0_), 
tms_subset1(tms_subset1_),
compare_regimes(compare_regimes_), 
compare_time_periods(compare_time_periods_),
compare_r_and_t(compare_r_and_t_),
test_stat_valid(false), 
time_invariant(tms_subset0_.size() > 1)
{
	test_stat_c.resize(6);
	test_stat_valid_c.resize(6);
	deg_free_c.resize(6);
	p_val_c.resize(6);
	for (size_t i=0; i<6; ++i) {
		test_stat_c[i] = 0;
		test_stat_valid_c[i] = false;
		deg_free_c[i] = 0;
		p_val_c[i] = 1;
	}
	
	ss_ptrs.push_back(&s0);
	ss_ptrs.push_back(&s1);
	ss_ptrs.push_back(&s2);
	ss_ptrs.push_back(&s3);
}

LineChartStats::SampStats::SampStats()
{
	mean = 0; // sample mean
	mean_v = false; // is sample mean valid
	var = 0; // sample variance
	sd = 0; // sample standard deviation
	sem = 0; // standard error of the mean
	var_v = false; // is sample variance valid
	sz_i = 0; // sample size as integer
	sz_d = 0; // sample size as double
}

void LineChartStats::UpdateNonRegimesNonTmsStats()
{
	Y_avg_valid = false;
	size_t tms=Y.size();
    
	size_t valid_num_obs = 0;
    
    for (size_t i=0; i<Y_undef.size(); i++) {
        if ( !Y_undef[i]) {
            valid_num_obs += 1;
        }
    }
    
	double num_obs_d = (double) valid_num_obs;
	obs_sz_i = valid_num_obs;
	obs_sz_d = obs_sz_i;
	Y_avg.resize(tms);
	Y_ss.resize(tms);
	
	for (size_t t=0; t<tms; ++t) {
		Y_avg[t] = 0;
		Y_ss[t] = 0;
       
        if (Y[t].size() > 0)  {
    		for (size_t i=0; i<Y_undef.size(); ++i) {
                if (Y_undef[i])
                    continue;
    			Y_avg[t] += Y[t][i];
    			Y_ss[t] += Y[t][i] * Y[t][i];
    		}
    		if (valid_num_obs > 0) {
    			Y_avg[t] /= num_obs_d;
    			Y_avg_valid = true;
    			if (t==0) {
    				Y_avg_min = Y_avg[0];
    				Y_avg_max = Y_avg[0];
    			} else {
    				if (Y_avg[t] < Y_avg_min) {
    					Y_avg_min = Y_avg[t];
    				} else if (Y_avg[t] > Y_avg_max) {
    					Y_avg_max = Y_avg[t];
    				}
    			}
    		}
        }
	}
}

void LineChartStats::UpdateRegimesStats(const std::vector<bool>& hs,
                                        int default_Y_sel_avg_valid,
                                        int default_Y_excl_avg_valid)
{
    Y_sel_avg_min = 0;
    Y_sel_avg_max = 0;
    Y_excl_avg_min = 0;
    Y_excl_avg_max = 0;
    
	Y_sel_avg_valid = false;
	Y_excl_avg_valid = false;
	size_t tms=Y.size();
    size_t valid_num_obs = 0;
    
    for (size_t i=0; i<Y_undef.size(); i++) {
        if ( !Y_undef[i]) {
            valid_num_obs += 1;
        }
    }
    
    
	double num_obs_d = (double) valid_num_obs;
	Y_sel_avg.resize(tms);
	Y_sel_ss.resize(tms);
	Y_excl_avg.resize(tms);
	Y_excl_ss.resize(tms);

	double num_sel = 0;
    double num_excl = 0;
    bool num_sel_valid = false;
    bool num_unsel_valid = false;
    
    for (size_t i=0; i<hs.size(); ++i) {
        if ( hs[i] ) {
            if ( Y_undef[i] == false ) {
                num_sel_valid = true;
			    num_sel += 1.0;
            }
        } else {
            if ( Y_undef[i] == false ) {
                num_unsel_valid = true;
                num_excl += 1.0;
            }
        }
    }
	
    
	sel_sz_i = (int) num_sel;
	sel_sz_d = num_sel;
	excl_sz_i = (int) num_excl;
	excl_sz_d = num_excl;
	
	for (size_t t=0; t<tms; ++t) {
		Y_sel_avg[t] = 0;
		Y_sel_ss[t] = 0;
		Y_excl_avg[t] = 0;
		Y_excl_ss[t] = 0;
        if (Y[t].size() == 0)
            continue;
        
		for (size_t i=0; i<Y_undef.size(); ++i) {
            if (Y_undef[i])
                continue;
			if (hs[i]) {
                if (num_sel_valid) {
                    Y_sel_avg[t] += Y[t][i];
                    Y_sel_ss[t] += Y[t][i] * Y[t][i];
                }
			} else {
                if (num_unsel_valid) {
                    Y_excl_avg[t] += Y[t][i];
                    Y_excl_ss[t] += Y[t][i] * Y[t][i];
                }
			}
		}
		if (num_sel > 0 && num_sel_valid) {
            if (num_sel > 0)
                Y_sel_avg[t] /= num_sel;
            else
                Y_sel_avg[t] = 0.0;
                
			Y_sel_avg_valid = true;
			if (t==0) {
				Y_sel_avg_min = Y_sel_avg[0];
				Y_sel_avg_max = Y_sel_avg[0];
			}
			if (Y_sel_avg[t] < Y_sel_avg_min) {
				Y_sel_avg_min = Y_sel_avg[t];
			} else if (Y_sel_avg[t] > Y_sel_avg_max) {
				Y_sel_avg_max = Y_sel_avg[t];
			}
		}
		if (num_excl > 0 && num_unsel_valid) {
            if (num_excl > 0)
                Y_excl_avg[t] /= num_excl;
            else
                Y_excl_avg[t] = 0.0;
            
			Y_excl_avg_valid = true;
			if (t==0) {
				Y_excl_avg_min = Y_excl_avg[0];
				Y_excl_avg_max = Y_excl_avg[0];
			}
			if (Y_excl_avg[t] < Y_excl_avg_min) {
				Y_excl_avg_min = Y_excl_avg[t];
			} else if (Y_excl_avg[t] > Y_excl_avg_max) {
				Y_excl_avg_max = Y_excl_avg[t];
			}
		}
        
	}
    
    if (!num_sel_valid)
        Y_sel_avg_valid = num_sel_valid;
    if (!num_unsel_valid)
        Y_excl_avg_valid = num_unsel_valid;
    
    // override Y_sel_avg_valid if user selected in UI
    if (default_Y_excl_avg_valid >=0) {
        Y_excl_avg_valid = default_Y_excl_avg_valid;
    }
    if (default_Y_sel_avg_valid >= 0 ) {
        Y_sel_avg_valid = default_Y_sel_avg_valid;
    }
}

void LineChartStats::UpdateOtherStats()
{
	if (compare_time_periods)
        UpdateCompareTmsStats();
    
	//if (compare_regimes) UpdateCompareRegimesStats();
    
	if (compare_regimes || compare_r_and_t)
        UpdateCompareRegAndTmStats();
    
	UpdateTtest();
}

void LineChartStats::UpdateCompareTmsStats()
{
	// Instead of comparing highlight/non-highlight regimes, time subsets
	// 0 and 1 are used for regimes.
	
	Y_avg_tm0_valid = false;
	Y_avg_tm1_valid = false;
	Y_avg_tm0 = 0;
	Y_avg_tm1 = 0;
	
	size_t tms = tms_subset0.size();
	size_t tsub0_sz = 0;
	for (size_t t=0; t<tms; ++t) 
		if (tms_subset0[t]) 
			tsub0_sz++;
	size_t tsub1_sz = 0;
	for (size_t t=0; t<tms; ++t) 
		if (tms_subset1[t]) 
			tsub1_sz++;

	if (tsub0_sz > 0 && Y_avg_valid) {
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				Y_avg_tm0 += Y_avg[t];
		Y_avg_tm0 /= (double) tsub0_sz;
		Y_avg_tm0_valid = true;
	}
	
	if (tsub1_sz > 0 && Y_avg_valid) {
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset1[t]) 
				Y_avg_tm1 += Y_avg[t];
		Y_avg_tm1 /= (double) tsub1_sz;
		Y_avg_tm1_valid = true;
	}
	
    size_t valid_num_obs = 0;
    
    for (size_t i=0; i<Y_undef.size(); i++) {
        if ( !Y_undef[i]) {
            valid_num_obs += 1;
        }
    }
    
    double obs_d = (double) valid_num_obs;
    
	SampStats blank;
	s0 = blank;
	s1 = blank;
	s2 = blank;
	s3 = blank;
	s0.sz_i = tsub0_sz * valid_num_obs;
	s1.sz_i = tsub1_sz * valid_num_obs;
	s0.sz_d = (double) s0.sz_i;
	s1.sz_d = (double) s1.sz_i;
	s0.mean = Y_avg_tm0;
	s0.mean_v = Y_avg_tm0_valid;
	s1.mean = Y_avg_tm1;
	s1.mean_v = Y_avg_tm1_valid;
	if (s0.mean_v && s0.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				ss += Y_ss[t];
		s0.var = (ss - s0.sz_d*s0.mean*s0.mean)/(s0.sz_d-1);
		if (s0.var > 16*DBL_MIN) {
			s0.var_v = true;
			s0.sd = sqrt(s0.var);
			s0.sem = s0.sd/sqrt(s0.sz_d);
		}
	}
	if (s1.mean_v && s1.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset1[t]) 
				ss += Y_ss[t];
		s1.var = (ss - s1.sz_d*s1.mean*s1.mean)/(s1.sz_d-1);
		if (s1.var > 16*DBL_MIN) {
			s1.var_v = true;
			s1.sd = sqrt(s1.var);
			s1.sem = s1.sd/sqrt(s1.sz_d);
		}
	}
}

void LineChartStats::UpdateCompareRegimesStats()
{
	Y_sel_tm0_avg_valid = false;
	Y_excl_tm0_avg_valid = false;
	Y_sel_tm0_avg = 0;
	Y_excl_tm0_avg = 0;
	
	size_t tms = tms_subset0.size();
	size_t tsub0_sz = 0;
	if (Y.size() > 1) {
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				tsub0_sz++;
	} else {
		tsub0_sz = 1;
	}
		
	if (tsub0_sz > 0 && Y_sel_avg_valid) {
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset0[t]) 
				Y_sel_tm0_avg += Y_sel_avg[t];
		}
		Y_sel_tm0_avg /= (double) tsub0_sz;
		Y_sel_tm0_avg_valid = true;
	}

	if (tsub0_sz > 0 && Y_excl_avg_valid) {
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset0[t]) 
				Y_excl_tm0_avg += Y_excl_avg[t];
		}
		Y_excl_tm0_avg /= (double) tsub0_sz;
		Y_excl_tm0_avg_valid = true;
	}

    size_t valid_num_obs = 0;
    
    for (size_t i=0; i<Y_undef.size(); i++) {
        if ( !Y_undef[i]) {
            valid_num_obs += 1;
        }
    }
    
	double obs_d = (double) valid_num_obs;
	SampStats blank;
	s0 = blank;
	s1 = blank;
	s2 = blank;
	s3 = blank;
	s0.sz_i = tsub0_sz * sel_sz_i;
	s1.sz_i = tsub0_sz * excl_sz_i;
	s0.sz_d = (double) s0.sz_i;
	s1.sz_d = (double) s1.sz_i;
	s0.mean = Y_sel_tm0_avg;
	s0.mean_v = Y_sel_tm0_avg_valid;
	s1.mean = Y_excl_tm0_avg;
	s1.mean_v = Y_excl_tm0_avg_valid;
	if (s0.mean_v && s0.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				ss += Y_sel_ss[t];
		s0.var = (ss - s0.sz_d*s0.mean*s0.mean)/(s0.sz_d-1);
		if (s0.var > 16*DBL_MIN) {
			s0.var_v = true;
			s0.sd = sqrt(s0.var);
			s0.sem = s0.sd/sqrt(s0.sz_d);
		}
	}
	if (s1.mean_v && s1.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				ss += Y_excl_ss[t];
		s1.var = (ss - s1.sz_d*s1.mean*s1.mean)/(s1.sz_d-1);
		if (s1.var > 16*DBL_MIN) {
			s1.var_v = true;
			s1.sd = sqrt(s1.var);
			s1.sem = s1.sd/sqrt(s1.sz_d);
		}
	}
}

void LineChartStats::UpdateCompareRegAndTmStats()
{
	Y_sel_tm0_avg_valid = false;
	Y_excl_tm0_avg_valid = false;
	Y_sel_tm1_avg_valid = false;
	Y_excl_tm1_avg_valid = false;
	Y_sel_tm0_avg = 0;
	Y_excl_tm0_avg = 0;
	Y_sel_tm1_avg = 0;
	Y_excl_tm1_avg = 0;
    
	size_t tms = tms_subset0.size();
	size_t tsub0_sz = 0;
	if (Y.size() > 1) {
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				tsub0_sz++;
	} else {
		tsub0_sz = 1;
	}
	size_t tsub1_sz = 0;
	if (Y.size() > 1) {
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset1[t]) 
				tsub1_sz++;
	} else {
		tsub1_sz = 1;
	}
	
	if (tsub0_sz > 0 && Y_sel_avg_valid) {
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset0[t]) 
				Y_sel_tm0_avg += Y_sel_avg[t];
		}
		Y_sel_tm0_avg /= (double) tsub0_sz;
        if (Y_sel_avg_valid == 1) {
            Y_sel_tm0_avg_valid = true;
        }
	}
	
	if (tsub0_sz > 0 && Y_excl_avg_valid) {
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset0[t]) 
				Y_excl_tm0_avg += Y_excl_avg[t];
		}
		Y_excl_tm0_avg /= (double) tsub0_sz;
        if (Y_excl_avg_valid == 1) {
            Y_excl_tm0_avg_valid = true;
        }
	}
	
	if (tsub1_sz > 0 && Y_sel_avg_valid) {
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset1[t]) 
				Y_sel_tm1_avg += Y_sel_avg[t];
		}
		Y_sel_tm1_avg /= (double) tsub1_sz;
        if (Y_sel_avg_valid == 2 ||
            (Y_sel_avg_valid == 1 && Y_excl_avg_valid == 0)) {
            Y_sel_tm1_avg_valid = true;
        }
	}
	
	if (tsub1_sz > 0 && Y_excl_avg_valid) {
		for (size_t t=0; t<tms; ++t) {
			if (tms_subset1[t]) 
				Y_excl_tm1_avg += Y_excl_avg[t];
		}
		Y_excl_tm1_avg /= (double) tsub1_sz;
        if (Y_excl_avg_valid == 2 ||
            (Y_excl_avg_valid == 1 && Y_sel_avg_valid == 0)) {
            Y_excl_tm1_avg_valid = true;
        }
	}
	
    size_t valid_num_obs = 0;
    
    for (size_t i=0; i<Y_undef.size(); i++) {
        if ( !Y_undef[i]) {
            valid_num_obs += 1;
        }
    }
	double obs_d = (double) valid_num_obs;
	SampStats blank;
	s0 = blank;
	s1 = blank;
	s2 = blank;
	s3 = blank;
	s0.sz_i = tsub0_sz * sel_sz_i;
	s1.sz_i = tsub0_sz * excl_sz_i;
	s0.sz_d = (double) s0.sz_i;
	s1.sz_d = (double) s1.sz_i;
	s0.mean = Y_sel_tm0_avg;
	s0.mean_v = Y_sel_tm0_avg_valid;
	s1.mean = Y_excl_tm0_avg;
	s1.mean_v = Y_excl_tm0_avg_valid;
	
	s2.sz_i = tsub1_sz * sel_sz_i;
	s3.sz_i = tsub1_sz * excl_sz_i;
	s2.sz_d = (double) s2.sz_i;
	s3.sz_d = (double) s3.sz_i;
	s2.mean = Y_sel_tm1_avg;
	s2.mean_v = Y_sel_tm1_avg_valid;
	s3.mean = Y_excl_tm1_avg;
	s3.mean_v = Y_excl_tm1_avg_valid;
	
	if (s0.mean_v && s0.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				ss += Y_sel_ss[t];
		s0.var = (ss - s0.sz_d*s0.mean*s0.mean)/(s0.sz_d-1);
		if (s0.var > 16*DBL_MIN) {
			s0.var_v = true;
			s0.sd = sqrt(s0.var);
			s0.sem = s0.sd/sqrt(s0.sz_d);
		}
	}
	if (s1.mean_v && s1.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset0[t]) 
				ss += Y_excl_ss[t];
		s1.var = (ss - s1.sz_d*s1.mean*s1.mean)/(s1.sz_d-1);
		if (s1.var > 16*DBL_MIN) {
			s1.var_v = true;
			s1.sd = sqrt(s1.var);
			s1.sem = s1.sd/sqrt(s1.sz_d);
		}
	}
	
	if (s2.mean_v && s2.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset1[t]) 
				ss += Y_sel_ss[t];
		s2.var = (ss - s2.sz_d*s2.mean*s2.mean)/(s2.sz_d-1);
		if (s2.var > 16*DBL_MIN) {
			s2.var_v = true;
			s2.sd = sqrt(s2.var);
			s2.sem = s2.sd/sqrt(s2.sz_d);
		}
	}
	if (s3.mean_v && s3.sz_i >= 2) {
		double ss = 0;
		for (size_t t=0; t<tms; ++t) 
			if (tms_subset1[t]) 
				ss += Y_excl_ss[t];
		s3.var = (ss - s3.sz_d*s3.mean*s3.mean)/(s3.sz_d-1);
		if (s3.var > 16*DBL_MIN) {
			s3.var_v = true;
			s3.sd = sqrt(s3.var);
			s3.sem = s3.sd/sqrt(s3.sz_d);
		}
	}
}

void LineChartStats::UpdateTtest()
{
	using namespace boost::math;
	test_stat_valid = false;
	if (s0.var_v && s1.var_v) {
        
        double mu = (s0.mean * s0.sz_d + s1.mean * s1.sz_d ) /
        (s0.sz_d + s1.sz_d);
        
        double est_eff_i = s0.mean - mu;
        double est_eff_j = s1.mean - mu;
        
        double ss_treat = est_eff_i * est_eff_i * s0.sz_d +
        est_eff_j * est_eff_j * s1.sz_d;
        
        double ss_res = s0.var * (s0.sz_d -1) + s1.var * (s1.sz_d -1);
        
        double ss_tot = ss_treat + ss_res;
        
        double df_treat = 2 - 1;
        double df_tot = s0.sz_d + s1.sz_d -1;
        double df_res = df_tot - df_treat;
        
        double MS_treat = ss_treat / df_treat;
        
        double MS_res = ss_res / df_res;
        
        double f_val = MS_treat / MS_res;
        
        fisher_f dist(df_treat, df_res);
        double q = cdf(complement(dist, fabs(f_val)));
        
        deg_free = obs_sz_i-1;
        test_stat = f_val;
        p_val = q;
        
        /*
		test_stat = (s0.mean-s1.mean) / sqrt(s0.var/s0.sz_d + s1.var/s1.sz_d);
	
		// Estimating the combined degrees of freedom we using the
		// Welch-Satterthwaite approximation:
		deg_free = pow(s0.var/s0.sz_d + s1.var/s1.sz_d, 2) /
		(pow(s0.var/s0.sz_d,2)/(s0.sz_d-1) + 
		 pow(s1.var/s1.sz_d,2)/(s1.sz_d-1) );
	
		// T-score to 2-sided p-val
		students_t dist(deg_free);
		double q = cdf(complement(dist, fabs(test_stat)));
		p_val = 2 * q;
		wxString msg;
		msg << "Probability that difference is due to chance" << " =  ";
		msg << p_val;
		LOG_MSG(msg);
         */
		test_stat_valid = true;
	}
	
	size_t c = 0;
	for (size_t i=0; i<ss_ptrs.size(); ++i) {
		const SampStats& si = *ss_ptrs[i];
		for (size_t j=i+1; j<ss_ptrs.size(); ++j) {
			const SampStats& sj = *ss_ptrs[j];
			test_stat_valid_c[c] = false;
			if (si.var_v && sj.var_v) {
                
                double mu = (si.mean * si.sz_d + sj.mean * sj.sz_d ) /
                            (si.sz_d + sj.sz_d);
                
                double est_eff_i = si.mean - mu;
                double est_eff_j = sj.mean - mu;
                
                double ss_treat = est_eff_i * est_eff_i * si.sz_d +
                                    est_eff_j * est_eff_j * sj.sz_d;
                
                double ss_res = si.var * (si.sz_d -1) + sj.var * (sj.sz_d -1);
                
                double ss_tot = ss_treat + ss_res;
                
                double df_treat = 2 - 1;
                double df_tot = si.sz_d + sj.sz_d -1;
                double df_res = df_tot - df_treat;
                
                double MS_treat = ss_treat / df_treat;
                
                double MS_res = ss_res / df_res;
                
                double f_val = MS_treat / MS_res;
                
                fisher_f dist(df_treat, df_res);
                double q = cdf(complement(dist, fabs(f_val)));
                if (si.sz_d == sj.sz_d) {
                    if (i == 0 || i == 2)
                        deg_free_c[c] = (sel_sz_i - 1 );
                    else
                        deg_free_c[c] = (excl_sz_i -1);
                    
                } else {
                    deg_free_c[c] = (obs_sz_i - 1);
                }
                //deg_free_c[c] = (obs_sz_i - 1);
                test_stat_c[c] = f_val;
                p_val_c[c] = q;
                /*
                // following is t-test
				test_stat_c[c] = ((si.mean-sj.mean) /
								  sqrt(si.var/si.sz_d + sj.var/sj.sz_d));
				
				// Estimating the combined degrees of freedom we using the
				// Welch-Satterthwaite approximation: 
				deg_free_c[c] = (pow(si.var/si.sz_d + sj.var/sj.sz_d, 2) /
								 (pow(si.var/si.sz_d,2)/(si.sz_d-1) + 
								  pow(sj.var/sj.sz_d,2)/(sj.sz_d-1)) );
				
				// T-score to 2-sided p-val
				students_t dist(deg_free_c[c]);
				double q = cdf(complement(dist, fabs(test_stat_c[c])));
				p_val_c[c] = 2 * q;
				wxString msg;
				msg << "Probability avg s"<<i<<", s"<<j<<" diff ";
				msg << "due to chance:  " << p_val_c[c];
				LOG_MSG(msg);
                 */
				test_stat_valid_c[c] = true;
                
			}
			++c;
		}
	}
}

