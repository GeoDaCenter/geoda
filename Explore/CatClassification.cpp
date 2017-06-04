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

#include <iostream>
#include <iomanip>
#include <float.h>
#include <set>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <wx/msgdlg.h>
#include "../DataViewer/TableInterface.h"
#include "../DialogTools/NumCategoriesDlg.h"
#include "../logger.h"
#include "../GdaConst.h"
#include "CatClassification.h"

using namespace std;

struct UniqueValElem {
	UniqueValElem(double v, int f, int l): val(v), first(f), last(l) {}
	double val; // value
	int first; // index of first occurrance
	int last; // index of last occurrance
};

/** clears uv_mapping and resizes as needed */
void create_unique_val_mapping(std::vector<UniqueValElem>& uv_mapping,
							   const std::vector<double>& v,
                               const std::vector<bool>& v_undef)
{
	uv_mapping.clear();
	//uv_mapping.push_back(UniqueValElem(v[0], 0, 0));
	int cur_ind = -1;
   
	for (int i=0, iend=v.size(); i<iend; i++) {
        if (v_undef[i])
            continue;
        if (uv_mapping.empty()) {
			cur_ind++;
			uv_mapping.push_back(UniqueValElem(v[i], i, i));
        } else {
    		if (uv_mapping[cur_ind].val != v[i]) {
    			uv_mapping[cur_ind].last = i-1;
    			cur_ind++;
    			uv_mapping.push_back(UniqueValElem(v[i], i, i));
    		}
        }
	}
}

/** Assume that b.size() <= N-1 */
void pick_rand_breaks(std::vector<int>& b, int N)
{
	int num_breaks = b.size();
	if (num_breaks > N-1) return;
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::uniform_01<boost::mt19937> X(rng);
	
	std::set<int> s;
	while (s.size() != num_breaks) s.insert(1 + (N-1)*X());
	int cnt=0;
	for (std::set<int>::iterator it=s.begin(); it != s.end(); it++) {
		b[cnt++] = *it;
	}
	std::sort(b.begin(), b.end());
}

// translate unique value breaks into normal breaks given unique value mapping
void unique_to_normal_breaks(const std::vector<int>& u_val_breaks,
							 const std::vector<UniqueValElem>& u_val_mapping,
							 std::vector<int>& n_breaks)
{
	if (n_breaks.size() != u_val_breaks.size()) {
		n_breaks.resize(u_val_breaks.size());
	}
	for (int i=0, iend=u_val_breaks.size(); i<iend; i++) {
		n_breaks[i] = u_val_mapping[u_val_breaks[i]].first;
	}	
}

/** Assume input b and v is sorted.  If not, can sort
 with std::sort(v.begin(), v.end())
 We assume that b and v are sorted in ascending order and are
 valid (ie, no break indicies out of range and all categories
 have at least one value.
 gssd is the global sum of squared differences from the mean */
double calc_gvf(const std::vector<int>& b, const std::vector<double>& v,
				double gssd)
{
	int N = v.size();
	int num_cats = b.size()+1;
	double tssd=0; // total sum of local sums of squared differences
	for (int i=0; i<num_cats; i++) {
		int s = (i == 0) ? 0 : b[i-1];
		int t = (i == num_cats-1) ? N : b[i];
		
		double m=0; // local mean
		double ssd=0; // local sum of squared differences (variance)
		for (int j=s; j<t; j++) m += v[j];
		m /= ((double) t-s);
		for (int j=s; j<t; j++) ssd += (v[j]-m)*(v[j]-m);
		tssd += ssd;
	}
	
	return 1-(tssd/gssd);
}

void CatClassification::CatLabelsFromBreaks(const std::vector<double>& breaks,
											std::vector<wxString>& cat_labels,
											const CatClassifType theme,
                                            bool useScientificNotation)
{
    stringstream s;
    if (useScientificNotation)
        s << std::setprecision(2) << std::scientific;
    else
        s << std::setprecision(3);
    
	int num_breaks = breaks.size();
	int cur_intervals = num_breaks+1;
	cat_labels.resize(cur_intervals);
	for (int ival=0; ival<cur_intervals; ++ival) {
        s.str("");
		if (cur_intervals <= 1) {
			s << "";
		} else if (ival == 0) {
			s << "< " << breaks[ival];
			//s << "(-inf, " << breaks[ival] << ")";
		} else if (ival == cur_intervals-1 && cur_intervals != 2) {
			s << "> " << breaks[ival-1];
			//s << "(" << breaks[ival-1] << ", inf)";
		} else if (ival == cur_intervals-1 && cur_intervals == 2) {
			if (theme == CatClassification::unique_values)
				s << "=" << breaks[ival - 1];
			else
				s << ">= " << breaks[ival-1];
			//s << "[" << breaks[ival-1] << ", inf)";
		} else {
			int num_breaks = cur_intervals-1;
			int num_breaks_lower = (num_breaks+1)/2;
			wxString a;
			wxString b;
			if (ival < num_breaks_lower) {
				a = "[";
				b = ")";
			} else if (ival == num_breaks_lower) {
				a = "[";
				b = "]";
			} else {
				a = "(";
				b = "]";
			}
			s << a << breaks[ival-1] << ", ";
			s << breaks[ival] << b;
		}
		cat_labels[ival] = s.str();
	}	
}

/** It is assumed that var is sorted in ascending order */
void CatClassification::SetBreakPoints(std::vector<double>& breaks,
									   std::vector<wxString>& cat_labels,
									   const Gda::dbl_int_pair_vec_type& var,
                                       const std::vector<bool>& var_undef,
									   const CatClassifType theme,
                                       int num_cats,
                                       bool useScientificNotation)
{
	int num_obs = var.size();
    
	if (num_cats < 1)
        num_cats = 1;
    
	if (num_cats > 10)
        num_cats = 10;
    
	breaks.resize(num_cats-1);
	cat_labels.resize(num_cats);
    
	if (theme == CatClassification::percentile ||
		theme == CatClassification::hinge_15 ||
		theme == CatClassification::hinge_30 ||
		theme == CatClassification::stddev ||
		theme == CatClassification::excess_risk_theme) {
		num_cats = 6;
	} else if (theme == no_theme) {
		num_cats = 1;
		cat_labels[0] = "";
	}
    
	// no_theme handled by default
	if (theme == hinge_15 || theme == hinge_30) {
		HingeStats hinge_stats;
		hinge_stats.CalculateHingeStats(var);
		breaks[0] = (theme == hinge_15 ? hinge_stats.extreme_lower_val_15 :
							hinge_stats.extreme_lower_val_30);
		breaks[1] = hinge_stats.Q1;
		breaks[2] = hinge_stats.Q2;
		breaks[3] = hinge_stats.Q3;
		breaks[4] = (theme == hinge_15 ? hinge_stats.extreme_upper_val_15 :
						   hinge_stats.extreme_upper_val_30);
		cat_labels[0] = "Lower outlier";
		cat_labels[1] = "< 25%";
		cat_labels[2] = "25% - 50%";
		cat_labels[3] = "50% - 75%";
		cat_labels[4] = "> 75%";
		cat_labels[5] = "Upper outlier";
        
	} else if (theme == quantile) {
		if (num_cats == 1) {
			// already handled
		} else {
			for (int i=0, iend=breaks.size(); i<iend; i++) {
				breaks[i] =
					Gda::percentile(((i+1.0)*100.0)/((double) num_cats), var);
			}
		}
		CatLabelsFromBreaks(breaks, cat_labels, theme, useScientificNotation);
        
	} else if (theme == percentile) {
		breaks[0] = Gda::percentile(1, var);
		breaks[1] = Gda::percentile(10, var);
		breaks[2] = Gda::percentile(50, var);
		breaks[3] = Gda::percentile(90, var);
		breaks[4] = Gda::percentile(99, var);
		cat_labels[0] = "< 1%";       // < 1%
		cat_labels[1] = "1% - 10%";   // >= 1% && < 10%
		cat_labels[2] = "10% - 50%";  // >= 10% && < 50%
		cat_labels[3] = "50% - 90%";  // >= 50% && <= 90%
		cat_labels[4] = "90% - 99%";  // > 90% && <= 99%
		cat_labels[5] = "> 99%";      // > 99%
        
	} else if (theme == stddev) {
		std::vector<double> v(num_obs);
		SampleStatistics stats;
		for (int i=0; i<num_obs; i++) v[i] = var[i].first;
		stats.CalculateFromSample(v);
		breaks[0] = stats.mean - 2.0 * stats.sd_with_bessel;
		breaks[1] = stats.mean - 1.0 * stats.sd_with_bessel;
		breaks[2] = stats.mean;
		breaks[3] = stats.mean + 1.0 * stats.sd_with_bessel;
		breaks[4] = stats.mean + 2.0 * stats.sd_with_bessel;
		
		CatLabelsFromBreaks(breaks, cat_labels, theme, useScientificNotation);
        
	} else if (theme == unique_values) {
		std::vector<double> v(num_obs);
		std::vector<bool> v_undef(num_obs);
        for (int i=0; i<num_obs; i++) {
            v[i] = var[i].first;
            int ind = var[i].second;
            v_undef[i] = var_undef[ind];
        }
		std::vector<UniqueValElem> uv_mapping;
		create_unique_val_mapping(uv_mapping, v, v_undef);
        
		int num_unique_vals = uv_mapping.size();
		num_cats = num_unique_vals;
		if (num_unique_vals > 10) {
			num_cats = 10;
			//FindNaturalBreaks(num_cats, var, var_undef, breaks);
		} 
		breaks.resize(num_cats - 1);
		for (int i = 0; i<num_cats - 1; i++) {
			breaks[i] = (uv_mapping[i].val + uv_mapping[i + 1].val) / 2.0;
		}

		cat_labels.resize(num_cats);

		for (int i=0; i<num_cats; ++i) {
			cat_labels[i] = "";
			cat_labels[i] << uv_mapping[i].val;
		}
		if (num_unique_vals > 10) {
			cat_labels[9] = "Others";
		}

		// don't need to correct for
		//CatLabelsFromBreaks(breaks, cat_labels, theme, useScientificNotation);
        
	} else if (theme == natural_breaks) {
		FindNaturalBreaks(num_cats, var, var_undef, breaks);
		CatLabelsFromBreaks(breaks, cat_labels, theme, useScientificNotation);
        
	} else if (theme == equal_intervals) {
		double min_val = var[0].first;
		double max_val = var[0].first;
		for (int i=0; i<num_obs; i++) {
			double val = var[i].first;
			if (val < min_val) {
				min_val = val;
			} else if (val > max_val) {
				max_val = val;
			}
		}
		if (min_val == max_val || num_cats == 1) {
			num_cats = 1;
			breaks.resize(0);
		} else {
			std::vector<double> cat_min(num_cats);
			std::vector<double> cat_max(num_cats);
			double delta = (max_val - min_val) / (double) num_cats;
			for (int i=0; i<breaks.size(); i++) {
				breaks[i] = min_val + (((double) i) + 1.0)*delta;
			}
		}
		CatLabelsFromBreaks(breaks, cat_labels, theme, useScientificNotation);
	}
}

/** Update Categories based on num_cats and number time periods

 var is assumed to be sorted.

 num_cats is only used by themes where the user enters the number of categories.
 
 Note: LISA and Getis-Ord map themes are not supported by this function.
 */
void
CatClassification::
PopulateCatClassifData(const CatClassifDef& cat_def,
                       const std::vector<Gda::dbl_int_pair_vec_type>& var,
                       const std::vector<std::vector<bool> >& var_undef,
                       CatClassifData& cat_data,
                       std::vector<bool>& cats_valid,
                       std::vector<wxString>& cats_error_message,
                       bool useSciNotation,
                       bool useUndefinedCategory)
{
	int num_cats = cat_def.num_cats;
	CatClassifType theme = cat_def.cat_classif_type;
	int num_time_vals = var.size();
	int num_obs = var[0].size();
   
	if (theme == CatClassification::no_theme) {
		// 1 = #cats
		cat_data.CreateCategoriesAllCanvasTms(1, num_time_vals, num_obs);
		for (int t=0; t<num_time_vals; t++) {
			cat_data.SetCategoryColor(t, 0,
									  GdaConst::map_default_fill_colour);
		}
	} else if (theme == CatClassification::quantile) {
		// user supplied number of categories
		cat_data.CreateCategoriesAllCanvasTms(num_cats, num_time_vals, num_obs);
		cat_data.SetCategoryBrushesAllCanvasTms(
				CatClassification::sequential_color_scheme, num_cats, false);
        
	} else if (theme == CatClassification::unique_values) {
		// number of categories based on number of unique values in data
		cat_data.CreateEmptyCategories(num_time_vals, num_obs);
        
	} else if (theme == CatClassification::natural_breaks) {
		// user supplied number of categories
		cat_data.CreateEmptyCategories(num_time_vals, num_obs);
		// if there are fewer unique values than number of categories,
		// we will automatically reduce the number of categories to the
		// number of unique values.
        
	} else if (theme == CatClassification::equal_intervals) {
		// user supplied number of categories
		cat_data.CreateEmptyCategories(num_time_vals, num_obs);
		// if there is only one value, then we automatically reduce
		// the number of categories down to one.
        
	} else if (theme == CatClassification::percentile ||
			   theme == CatClassification::hinge_15 ||
			   theme == CatClassification::hinge_30 ||
			   theme == CatClassification::stddev ||
			   theme == CatClassification::excess_risk_theme) {
		num_cats = 6;
		cat_data.CreateCategoriesAllCanvasTms(num_cats, num_time_vals, num_obs);
		cat_data.SetCategoryBrushesAllCanvasTms(
				CatClassification::diverging_color_scheme, num_cats, false);
        
	} else if (theme == CatClassification::custom) {
		cat_data.CreateCategoriesAllCanvasTms(num_cats, num_time_vals, num_obs);
		cat_data.SetCategoryBrushesAllCanvasTms(cat_def.colors);
        
	}

    // detect if undefined category
    std::vector<int> undef_cnts_tms(num_time_vals, 0);
    for (int t=0; t<num_time_vals; t++) {
        for (int i=0; i<var_undef[t].size(); i++) {
            if (var_undef[t][i]) {
                undef_cnts_tms[t] += 1;
            }
        }
    }
    
	if (theme == CatClassification::no_theme) {
		for (int t=0; t<num_time_vals; t++) {
            
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
            
			cat_data.SetCategoryLabel(t, 0, "");
			cat_data.SetCategoryCount(t, 0, num_obs);
            for (int i=0; i<num_obs; i++) {
                int ind = var[t][i].second;
                if (!useUndefinedCategory && var_undef[t][ind]) {
                    continue;
                }
                int c = var_undef[t][ind] ? 1 : 0;
                cat_data.AppendIdToCategory(t, c, ind);
            }
			cat_data.SetCategoryMinMax(t, 0, var[t][0].first,
									   var[t][var[t].size()-1].first);
		}
		return;
	}
	
    stringstream ss;
    if (useSciNotation)
        ss << std::setprecision(2) << std::scientific;
    else
        ss << std::setprecision(3);
    
	if (num_cats > num_obs) {
		for (int t=0; t<num_time_vals; t++) {
			cats_valid[t] = false;
			cats_error_message[t] << "Error: Chosen theme requires more ";
			cats_error_message[t] << "cateogries than observations.";
		}
        
	} else if (theme == hinge_15 || theme == hinge_30) {
		std::vector<HingeStats> hinge_stats(num_time_vals);
		cat_data.SetCategoryBrushesAllCanvasTms(
				CatClassification::diverging_color_scheme, num_cats, false);
		cat_data.ResetAllCategoryMinMax();
        
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
            
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
			
			hinge_stats[t].CalculateHingeStats(var[t], var_undef[t]);
			double extreme_lower = hinge_stats[t].extreme_lower_val_15;
			double extreme_upper = hinge_stats[t].extreme_upper_val_15;
			if (theme == hinge_30) {
				extreme_lower = hinge_stats[t].extreme_lower_val_30;
				extreme_upper = hinge_stats[t].extreme_upper_val_30;	
			}
            
            double p_min = DBL_MAX;
            double p_max = -DBL_MAX;
        
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
                
                if (var_undef[t][ind] ) {
                    if (useUndefinedCategory) {
                        cat_data.AppendIdToCategory(t, 6, ind); //0-5 hinge
                    }
                    continue;
                }
                
                if (val < p_min)
                    p_min = val;
                if (val > p_max)
                    p_max = val;
				int cat_num = 0;
				if (val < extreme_lower) {
					cat_num = 0;
				} else if (val < hinge_stats[t].Q1) {
					cat_num = 1;
				} else if (val < hinge_stats[t].Q2) {
					cat_num = 2;
				} else if (val <= hinge_stats[t].Q3) {
					cat_num = 3;
				} else if (val <= extreme_upper) {
					cat_num = 4;
				} else { // val > extreme_upper
					cat_num = 5;
				}
				cat_data.AppendIdToCategory(t, cat_num, ind);
				cat_data.UpdateCategoryMinMax(t, cat_num, val);
			}
			std::vector<wxString> labels(num_cats);
			labels[0] << "Lower outlier";
			labels[1] << "< 25%";
			labels[2] << "25% - 50%";
			labels[3] << "50% - 75%";
			labels[4] << "> 75%";
			labels[5] << "Upper outlier";
            
			std::vector<wxString> labels_ext(num_cats);

            ss.str("");
            if (cat_data.GetNumObsInCategory(t, 0) == 0) {
                ss << " [-inf : " << extreme_lower << "]";
            } else {
                ss << " [" << p_min << " : " << extreme_lower << "]";
            }
            labels_ext[0] = ss.str();
            ss.str("");
            cat_data.SetCategoryMinMax(t, 0, p_min, extreme_lower);
                                       
            ss << " [" << extreme_lower << " : " << hinge_stats[t].Q1 << "]";
            labels_ext[1] = ss.str();
            ss.str("");
            cat_data.SetCategoryMinMax(t, 1, extreme_lower, hinge_stats[t].Q1);
            
            ss << " [" << hinge_stats[t].Q1 << " : " << hinge_stats[t].Q2 << "]";
            labels_ext[2] = ss.str();
            ss.str("");
            cat_data.SetCategoryMinMax(t, 2, hinge_stats[t].Q1, hinge_stats[t].Q2);
            
            ss << " [" << hinge_stats[t].Q2 << " : " << hinge_stats[t].Q3 << "]";
            labels_ext[3] = ss.str();
            ss.str("");
            cat_data.SetCategoryMinMax(t, 3, hinge_stats[t].Q2, hinge_stats[t].Q3);
            
            ss << " [" << hinge_stats[t].Q3 << " : " << extreme_upper << "]";
            labels_ext[4] = ss.str();
            ss.str("");
            cat_data.SetCategoryMinMax(t, 4, hinge_stats[t].Q3, extreme_upper);
            
            if (cat_data.GetNumObsInCategory(t, num_cats-1) == 0 &&
                p_max > extreme_upper)
                ss << " [" << extreme_upper << " : " << p_max << "]";
            else
                ss << " [" << extreme_upper << " : inf]";
            labels_ext[5] = ss.str();
            cat_data.SetCategoryMinMax(t, 5, extreme_upper, p_max);
       
            
			for (int cat=0; cat<num_cats; cat++) {
				cat_data.SetCategoryLabel(t, cat, labels[cat]);
				cat_data.SetCategoryLabelExt(t, cat, labels_ext[cat]);
				cat_data.SetCategoryCount(t, cat,
										  cat_data.GetNumObsInCategory(t, cat));
                
			}
		}
	} else if (theme == custom) {
		if (num_cats == 1) {
			for (int t=0; t<num_time_vals; t++) {
				if (!cats_valid[t])
                    continue;
                
                if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                    cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
                
				for (int i=0, iend=var[t].size(); i<iend; i++) {
                    int ind = var[t][i].second;
                    if (!useUndefinedCategory && var_undef[t][ind])
                        continue;
                    int c = var_undef[t][ind] ? 1 : 0;
                    cat_data.AppendIdToCategory(t, c, ind);
				}
				double low_v = var[t][0].first;
				double high_v = var[t][num_obs - 1].first;
				wxString s = wxString::Format("(-inf, inf) (%d)", num_obs);
				cat_data.SetCategoryLabel(t, 0, s);
				cat_data.SetCategoryCount(t, 0, num_obs);
				cat_data.SetCategoryMinMax(t, 0, low_v, high_v);
			}
		} else {
            
			std::vector<double> cat_min(num_cats);
			std::vector<double> cat_max(num_cats);
			std::vector<double> breaks(num_cats-1);
			int num_breaks = breaks.size();
			int num_breaks_lower = (num_breaks+1)/2;
		
			for (int t=0; t<num_time_vals; t++) {
                
				if (!cats_valid[t])
                    continue;
                
                if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                    cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
                
				// Set default cat_min / cat_max values for when
				// category size is 0
				cat_min[0] = var[t][0].first;
				cat_max[0] = cat_def.breaks[0];
				for (int i=1; i<num_breaks; i++) {
					cat_min[i] = cat_def.breaks[i-1];
					cat_max[i] = cat_def.breaks[i];
				}
				cat_min[num_breaks] = cat_def.breaks[num_breaks-1];
				cat_max[num_breaks] = var[t][num_obs-1].first;
				double val;
				int ind;
				for (int i=0, iend=var[t].size(); i<iend; i++) {
					val = var[t][i].first;
					ind = var[t][i].second;
                    if (var_undef[t][ind]) {
                        if (useUndefinedCategory) {
                            cat_data.AppendIdToCategory(t, num_breaks+1, ind);
                        }
                        continue;
                    }
					bool found = false;
					int cat = num_breaks; // last cat by default
					for (int j=0; j<num_breaks_lower; j++) {
						if (val < cat_def.breaks[j]) {
							found = true;
							cat = j;
							break;
						}
					}
					if (!found) {
						for (int j=num_breaks_lower; j<num_breaks; j++) {
							if (val <= cat_def.breaks[j]) {
								cat = j;
								break;
							}
						}
					}
					if (cat_data.GetNumObsInCategory(t, cat) == 0) {
						cat_min[cat] = val;
						cat_max[cat] = val;
					} else {
						if (val < cat_min[cat]) {
							cat_min[cat] = val;
						} else if (val > cat_max[cat]) {
							cat_max[cat] = val;
						}
					}
					cat_data.AppendIdToCategory(t, cat, ind);
				}
				
				for (int ival=0; ival<num_cats; ival++) {
                    ss.str("");
					if (num_cats <= 1) {
                        ss << "";
						cat_data.SetCategoryCount(t, ival, num_obs);
					} else if (ival == 0) {
                        ss << "< " << cat_def.breaks[ival];
						cat_data.SetCategoryCount(t, ival,
									cat_data.GetNumObsInCategory(t, ival));
					} else if (ival == num_cats-1 && num_cats != 2) {
                        ss << "> " << cat_def.breaks[ival-1];
						cat_data.SetCategoryCount(t, ival,
									cat_data.GetNumObsInCategory(t, ival));
					} else if (ival == num_cats-1 && num_cats == 2) {
                        ss << ">= " << cat_def.breaks[ival-1];
						cat_data.SetCategoryCount(t, ival,
												  cat_data.GetNumObsInCategory(t, ival));
					} else {
						int num_breaks = num_cats-1;
						int num_breaks_lower = (num_breaks+1)/2;
						wxString a;
						wxString b;
						if (ival < num_breaks_lower) {
							a = "[";
							b = ")";
						} else if (ival == num_breaks_lower) {
							a = "[";
							b = "]";
						} else {
							a = "(";
							b = "]";
						}
                        ss << a << cat_def.breaks[ival-1] << ", ";
                        ss << cat_def.breaks[ival] << b;
						cat_data.SetCategoryCount(t, ival,
									cat_data.GetNumObsInCategory(t, ival));
					}
                    if (cat_def.names[ival].IsEmpty()) {
                        cat_data.SetCategoryLabel(t, ival, wxString(ss.str()));
                    } else {
                        cat_data.SetCategoryLabel(t, ival, cat_def.names[ival]);
                    }
					cat_data.SetCategoryMinMax(t, ival,
											   cat_min[ival], cat_max[ival]);
				}
			}
		}
	} else if (theme == quantile) {
		if (num_cats == 1) {
			for (int t=0; t<num_time_vals; t++) {
                
				if (!cats_valid[t])
                    continue;
                if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                    cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
                
				for (int i=0, iend=var[t].size(); i<iend; i++) {
                    int ind = var[t][i].second;
                    if (!useUndefinedCategory && var_undef[t][ind])
                        continue;
                    int c = var_undef[t][ind] ? 1 : 0;
					cat_data.AppendIdToCategory(t, c, ind);
				}
				double low_v = var[t][0].first;
				double high_v = var[t][num_obs - 1].first;
                ss.str("");
				ss << "[" << low_v << " : " << high_v << "]";
				cat_data.SetCategoryLabel(t, 0, wxString(ss.str()));
				cat_data.SetCategoryCount(t, 0, num_obs);
				cat_data.SetCategoryMinMax(t, 0, low_v, high_v);
			}
		} else {
			std::vector<double> cat_min(num_cats);
			std::vector<double> cat_max(num_cats);
			std::vector<double> breaks(num_cats-1);
			int num_breaks = breaks.size();
			int num_breaks_lower = (num_breaks+1)/2;
			
			for (int t=0; t<num_time_vals; t++) {
				if (!cats_valid[t])
                    continue;
                if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                    cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
                
				for (int i=0; i<num_breaks; i++) {
					breaks[i] = Gda::percentile( ((i+1.0)*100.0)/((double) num_cats),
												  var[t], var_undef[t]);
				}
				// Set default cat_min / cat_max values for when
				// category size is 0
				cat_min[0] = var[t][0].first;
				cat_max[0] = breaks[0];
				for (int i=1; i<num_breaks; i++) {
					cat_min[i] = breaks[i-1];
					cat_max[i] = breaks[i];
				}
				cat_min[num_breaks] = breaks[num_breaks-1];
				cat_max[num_breaks] = var[t][num_obs-1].first;
				double val;
				int ind;
				for (int i=0, iend=var[t].size(); i<iend; i++) {
					val = var[t][i].first;
					ind = var[t][i].second;
                    if (var_undef[t][ind]) {
                        if (useUndefinedCategory) {
                            cat_data.AppendIdToCategory(t, num_breaks+1, ind);
                        }
                        continue;
                    }
					bool found = false;
					int cat = num_breaks; // last cat by default
					for (int j=0; j<num_breaks_lower; j++) {
						if (val < breaks[j]) {
							found = true;
							cat = j;
							break;
						}
					}
					if (!found) {
						for (int j=num_breaks_lower; j<num_breaks; j++) {
							if (val <= breaks[j]) {
								cat = j;
								break;
							}
						}
					}
					if (cat_data.GetNumObsInCategory(t, cat) == 0) {
						cat_min[cat] = val;
						cat_max[cat] = val;
					} else {
						if (val < cat_min[cat]) {
							cat_min[cat] = val;
						} else if (val > cat_max[cat]) {
							cat_max[cat] = val;
						}
					}
					cat_data.AppendIdToCategory(t, cat, ind);
				}
                ss.str("");
				ss << "[" << cat_min[0] << " : " << cat_max[0] << "]";
				cat_data.SetCategoryLabel(t, 0, wxString(ss.str()));
				cat_data.SetCategoryCount(t, 0,
										  cat_data.GetNumObsInCategory(t, 0));
				cat_data.SetCategoryMinMax(t, 0, cat_min[0], cat_max[0]);
				for (int i=1, iend=breaks.size(); i<iend; i++) {
                    ss.str("");
					ss << "[" << cat_min[i] << " : " << cat_max[i] << "]";
					cat_data.SetCategoryLabel(t, i, wxString(ss.str()));
					cat_data.SetCategoryCount(t, i,
										cat_data.GetNumObsInCategory(t, i));
					cat_data.SetCategoryMinMax(t, i, cat_min[i], cat_max[i]);
				}
                ss.str("");
				ss << "[" << cat_min[num_breaks] << " : " << cat_max[num_breaks] << "]";
				cat_data.SetCategoryLabel(t, num_breaks, wxString(ss.str()));
				cat_data.SetCategoryCount(t, num_breaks,
					cat_data.GetNumObsInCategory(t, num_breaks));
				cat_data.SetCategoryMinMax(t, num_breaks, cat_min[num_breaks],
										   cat_max[num_breaks]);
			}
		}
	} else if (theme == percentile) {
		cat_data.ResetAllCategoryMinMax();
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
            
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
		
            double p_min = DBL_MAX;
            double p_max = -DBL_MAX;
			double p_1 = Gda::percentile(1, var[t], var_undef[t]);
			double p_10 = Gda::percentile(10, var[t], var_undef[t]);
			double p_50 = Gda::percentile(50, var[t], var_undef[t]);
			double p_90 = Gda::percentile(90, var[t], var_undef[t]);
			double p_99 = Gda::percentile(99, var[t], var_undef[t]);
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
                if (var_undef[t][ind]) {
                    if (useUndefinedCategory) {
                        cat_data.AppendIdToCategory(t, 6, ind); // 0-5 for percentiles
                    }
                    continue;
                }
                if (val < p_min)
                    p_min = val;
                if (val > p_max)
                    p_max = val;
				int cat_num = 0;
				if (val < p_1) {
					cat_num = 0;
				} else if (val < p_10) {
					cat_num = 1;
				} else if (val < p_50) {
					cat_num = 2;
				} else if (val <= p_90) {
					cat_num = 3;
				} else if (val <= p_99) {
					cat_num = 4;
				} else { // val > p_99
					cat_num = 5;
				}
				cat_data.AppendIdToCategory(t, cat_num, ind);
				cat_data.UpdateCategoryMinMax(t, cat_num, val);
			}
			std::vector<wxString> labels(num_cats);
			labels[0] << "< 1%";       // < 1%
			labels[1] << "1% - 10%";   // >= 1% && < 10%
			labels[2] << "10% - 50%";  // >= 10% && < 50%
			labels[3] << "50% - 90%";  // => 50% && <= 90%
			labels[4] << "90% - 99%";  // > 90% && <= 99%
			labels[5] << "> 99%";      // > 99%
			std::vector<wxString> labels_ext(num_cats);
           
            ss.str("");
            ss << " [" << p_min << " : " << p_1 << "]";
            labels_ext[0] = ss.str();
            ss.str("");
            ss << " [" << p_1 << " : " << p_10 << "]";
            labels_ext[1] = ss.str();
            ss.str("");
            ss << " [" << p_10 << " : " << p_50 << "]";
            labels_ext[2] = ss.str();
            ss.str("");
            ss << " [" << p_50 << " : " << p_90 << "]";
            labels_ext[3] = ss.str();
            ss.str("");
            ss << " [" << p_90 << " : " << p_99 << "]";
            labels_ext[4] = ss.str();
            ss.str("");
            ss << " [" << p_99 << " : " << p_max << "]";
            labels_ext[5] = ss.str();
            
			for (int cat=0; cat<num_cats; cat++) {
				cat_data.SetCategoryLabel(t, cat, labels[cat]);
				cat_data.SetCategoryLabelExt(t, cat, labels_ext[cat]);
				cat_data.SetCategoryCount(t, cat,
										  cat_data.GetNumObsInCategory(t, cat));
			}
		}
	} else if (theme == stddev) {
		std::vector<double> v;
		SampleStatistics stats;
		cat_data.ResetAllCategoryMinMax();
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
			
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
            
            for (int i=0; i<num_obs; i++) {
                if ( var_undef[t][ var[t][i].second ] )
                    continue;
                v.push_back( var[t][i].first );
            }
            
            stats.CalculateFromSample(v);
			
			double SDm2 = stats.mean - 2.0 * stats.sd_with_bessel;
			double SDm1 = stats.mean - 1.0 * stats.sd_with_bessel;
			double mean = stats.mean;
			double SDp1 = stats.mean + 1.0 * stats.sd_with_bessel;
			double SDp2 = stats.mean + 2.0 * stats.sd_with_bessel;
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
                if (var_undef[t][ind]) {
                    if (useUndefinedCategory) {
                        cat_data.AppendIdToCategory(t, 6, ind); // 0-5 for percentiles
                    }
                    continue;
                }
				int cat_num = 0;
				if (val < SDm2) {
					cat_num = 0;
				} else if (val < SDm1) {
					cat_num = 1;
				} else if (val < mean) {
					cat_num = 2;
				} else if (val <= SDp1) {
					cat_num = 3;
				} else if (val <= SDp2) {
					cat_num = 4;
				} else { // val > SDp2
					cat_num = 5;
				}
				cat_data.AppendIdToCategory(t, cat_num, ind);
				cat_data.UpdateCategoryMinMax(t, cat_num, val);
			}
			std::vector<wxString> labels(num_cats);
			// < -2 sd
			// >= -2 sd && < -1 sd
			// >= -1 sd && < mean
			// => mean && <= 1 sd
			// > 1 sd && <= 2 sd
			// > 2 sd
            
            ss.str("");
            ss << "< " << SDm2;
            labels[0] = ss.str();
            ss.str("");
            ss << SDm2 << " - " << SDm1;
            labels[1] = ss.str();
            ss.str("");
            ss << SDm1 << " - " << mean;
            labels[2] = ss.str();
            ss.str("");
            ss << mean << " - " << SDp1;
            labels[3] = ss.str();
            ss.str("");
            ss << SDp1 << " - " << SDp2;
            labels[4] = ss.str();
            ss.str("");
            ss << "> " << SDp2;
            labels[5] = ss.str();
            
			for (int cat=0; cat<num_cats; cat++) {
				cat_data.SetCategoryLabel(t, cat, labels[cat]);
				cat_data.SetCategoryCount(t, cat,
										  cat_data.GetNumObsInCategory(t, cat));
			}
		}
	} else if (theme == unique_values) {
		// The Unique Values theme is somewhat different from the other themes
		// in that we calculate the number from the data itself.  We support
		// at most 10 unique values.
		
		std::vector< std::vector<double> > u_vals_map(num_time_vals);
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
            
			//u_vals_map[t].push_back(var[t][0].first);
            
			for (int i=0; i<num_obs; i++) {
                double val = var[t][i].first;
                int ind = var[t][i].second;
                
                if (var_undef[t][ind]) {
                    continue;
                }
                
				if (u_vals_map[t].empty() ||
                    u_vals_map[t][u_vals_map[t].size()-1] != val)
				{
					u_vals_map[t].push_back(val);
				}
			}
		}
		
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
           
            int n_cat = u_vals_map[t].size();
            if (n_cat > max_num_categories)
                n_cat = max_num_categories;
           
            bool reversed = false;
			cat_data.SetCategoryBrushesAtCanvasTm(CatClassification::unique_color_scheme,
                                                  n_cat, reversed, t);
		}
		
		cat_data.ResetAllCategoryMinMax();
        
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
            
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
            
			int cur_cat = 0;
            
			for (int i=0; i<num_obs; i++) {
                double val = var[t][i].first;
                int ind = var[t][i].second;
                
                if (var_undef[t][ind]) {
                    continue;
                }
                
				if (u_vals_map[t][cur_cat]  != val &&
                    cur_cat < max_num_categories-1)
                {
                    cur_cat++;
                }
                
				cat_data.AppendIdToCategory(t, cur_cat, var[t][i].second);
				cat_data.UpdateCategoryMinMax(t, cur_cat, var[t][i].first);
			}
            
            // for undefined category
            for (int i=0; i<num_obs; i++) {
                double val = var[t][i].first;
                int ind = var[t][i].second;
                
                if (var_undef[t][ind] && useUndefinedCategory) {
                    cat_data.AppendIdToCategory(t, cur_cat+1, var[t][i].second);
                }
            }
            
            // for labels
            int n_cat = u_vals_map[t].size();
            if (n_cat > max_num_categories)
                n_cat = max_num_categories;
            
			std::vector<wxString> labels(n_cat);
			for (int cat=0; cat<n_cat; cat++) {
                int n_obs_in_cat = cat_data.GetNumObsInCategory(t, cat);
                
                ss.str("");
                if (cat < max_num_categories - 1) {
                    ss << u_vals_map[t][cat];
                } else {
                    if (n_obs_in_cat == 1) {
                        ss << u_vals_map[t][cat];
                    } else {
                        ss << "Others";
                    }
                }
                labels[cat] << ss.str();
				cat_data.SetCategoryLabel(t, cat, labels[cat]);
				cat_data.SetCategoryCount(t, cat, n_obs_in_cat);
			}
		}
	} else if (theme == natural_breaks) {
		SetNaturalBreaksCats(num_cats, var, var_undef, cat_data, cats_valid);
        
	} else if (theme == equal_intervals) {
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
            
            double min_val =  0;
            double max_val = 0;
            bool has_init = false;
            
			for (int i=0; i<num_obs; i++) {
				double val = var[t][i].first;
                int ind = var[t][i].second;
                
                if (var_undef[t][ind])
                    continue;
               
                if (!has_init) {
                    min_val = val;
                    max_val = val;
                    has_init = true;
                } else if (val < min_val) {
					min_val = val;
				} else if (val > max_val) {
					max_val = val;
				}
			}
			
			if (min_val == max_val || num_cats == 1) {
				// Create just one category and continue
				cat_data.SetCategoryBrushesAtCanvasTm(
					CatClassification::sequential_color_scheme, 1, false, t);
                
                if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                    cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
                
				for (int i=0; i<num_obs; i++) {
                    double val = var[t][i].first;
                    int ind = var[t][i].second;
                    if (!useUndefinedCategory && var_undef[t][ind])
                        continue;
                    int c = var_undef[t][ind] ? 1 : 0;
					cat_data.AppendIdToCategory(t, c, ind);
				}
                ss.str("");
				ss << "[" << min_val << " : " << max_val << "]";
				cat_data.SetCategoryLabel(t, 0, wxString(ss.str()));
				cat_data.SetCategoryCount(t, 0, num_obs);
				cat_data.SetCategoryMinMax(t, 0, min_val, max_val);
				continue;
			}
			
			// we know that num_cats >= 2 and <= 10
			cat_data.SetCategoryBrushesAtCanvasTm(
				CatClassification::sequential_color_scheme, num_cats, false, t);
		
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
            
			std::vector<double> cat_min(num_cats);
			std::vector<double> cat_max(num_cats);
			double range = max_val - min_val;
			double delta = range / (double) num_cats;
			for (int i=0; i<num_cats; i++) {
				cat_min[i] = min_val + ((double) i)*delta;
				cat_max[i] = min_val + (((double) i) + 1.0)*delta;
			}
			
			int last_cat = num_cats - 1;
			for (int i=0; i<num_obs; i++) {
				double val = var[t][i].first;
				int ind = var[t][i].second;
               
                if (var_undef[t][ind]) {
                    if (useUndefinedCategory) {
                        cat_data.AppendIdToCategory(t, last_cat+1, ind);
                    }
                    continue;
                }
                
				int cat_num = last_cat; // last cat by default
				
				for (int j=0; j<num_cats && cat_num == last_cat; j++) {
					if (val >= cat_min[j] && val < cat_max[j])
                        cat_num = j;
				}
                cat_data.AppendIdToCategory(t, cat_num, ind);
			}
			
			for (int i=0; i<num_cats; i++) {
                ss.str("");
				ss << "[" << cat_min[i] << " : " << cat_max[i] << "]";
				cat_data.SetCategoryLabel(t, i, wxString(ss.str()));
				cat_data.SetCategoryCount(t, i,
										  cat_data.GetNumObsInCategory(t, i));
				cat_data.SetCategoryMinMax(t, i, cat_min[i], cat_max[i]);
			}
		}
	} else if (theme == excess_risk_theme) {
		cat_data.ResetAllCategoryMinMax();
		for (int t=0; t<num_time_vals; t++) {
			if (!cats_valid[t])
                continue;
			
            if (undef_cnts_tms[t]>0 && useUndefinedCategory)
                cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
            
			double val;
			int ind;
			for (int i=0, iend=var[t].size(); i<iend; i++) {
				val = var[t][i].first;
				ind = var[t][i].second;
                if (var_undef[t][ind]) {
                    if (useUndefinedCategory) {
                        cat_data.AppendIdToCategory(t, 6, ind); // 0-5 for percentiles
                    }
                    continue;
                }
				int cat_num = 0;
				if (val < 0.25) {
					cat_num = 0;
				} else if (val < 0.50) {
					cat_num = 1;
				} else if (val < 1.00) {
					cat_num = 2;
				} else if (val <= 2.00) {
					cat_num = 3;
				} else if (val <= 4.00) {
					cat_num = 4;
				} else { // val > 4.00
					cat_num = 5;
				}
				cat_data.AppendIdToCategory(t, cat_num, ind);
				cat_data.UpdateCategoryMinMax(t, cat_num, val);
			}
			std::vector<wxString> labels(num_cats);
			labels[0] << "< 0.25";       // < 0.25
			labels[1] << "0.25 - 0.50";  // >= 0.25 && < 0.50
			labels[2] << "0.50 - 1.00";  // >= 0.50 && < 1.00
			labels[3] << "1.00 - 2.00";  // >= 1.00 && <= 2.00
			labels[4] << "2.00 - 4.00";  // > 2.00 && <= 4.00
			labels[5] << "> 4.00";       // > 4.00
			for (int cat=0; cat<num_cats; cat++) {
				cat_data.SetCategoryLabel(t, cat, labels[cat]);
				cat_data.SetCategoryCount(t, cat,
										  cat_data.GetNumObsInCategory(t, cat));
			}
		}
	} else {
		for (int t=0; t<num_time_vals; t++) {
			cats_valid[t] = false;
			cats_error_message[t] = "Theme Not Implemented";
		}
	}
}

/**
 Modify CatClassifDef so that it is consistent with data in the TableInterface.
 Some things that might require changes to CatClassifDef data:
 - associated variable no longer exists
 - values and range of associated variable has changed
 Additionally, in case the user has manually edited the meta-data in the
 project configuration file, we attempt to make data self-consistent.
 If any changes made, then true is returned.  Otherwise, return false.
 
 If the the classification type is custom, then the breaks, colors, etc
 will be left as they were.
 */
bool CatClassification::CorrectCatClassifFromTable(CatClassifDef& _cc,
												   TableInterface* table_int)
{
	if (!table_int)
        return false;
    
	int num_obs = table_int->GetNumberRows();
	CatClassifDef cc;
	cc = _cc;
	
	std::sort(cc.breaks.begin(), cc.breaks.end());
	if (cc.uniform_dist_min > cc.uniform_dist_max) {
		double t = cc.uniform_dist_min;
		cc.uniform_dist_min = cc.uniform_dist_max;
		cc.uniform_dist_max = t;
	}
	if (cc.breaks.size() > 0) {
		if (cc.uniform_dist_min > cc.breaks[0]) {
			cc.uniform_dist_min = cc.breaks[0];
		}
		if (cc.uniform_dist_max < cc.breaks[cc.breaks.size()-1]) {
			cc.uniform_dist_max = cc.breaks[cc.breaks.size()-1];
		}
	}
	// At this point, uniform_dist min/max and breaks consistent with each
	// other
	
	int col = -1, tm = 0;
	// first ensure that assoc_db_fld_name exists in table
	bool field_removed = (cc.assoc_db_fld_name != "" &&
						  (!table_int->DbColNmToColAndTm(cc.assoc_db_fld_name,
														 col, tm) ||
						   !table_int->IsColNumeric(col)));
	
	if (field_removed) {
		// use min/max breaks as min/max for uniform dist
		if (cc.breaks.size() > 0) {
			cc.uniform_dist_min = cc.breaks[0];
			cc.uniform_dist_max = cc.breaks[cc.breaks.size()-1];
		}
		cc.assoc_db_fld_name = "";
	}
	
	bool uni_dist_mode = cc.assoc_db_fld_name.IsEmpty();
	Gda::dbl_int_pair_vec_type data(num_obs);
    std::vector<bool> data_undef(num_obs, false);
    
	if (uni_dist_mode) {
		// fill data with uniform distribution
		double delta = ((cc.uniform_dist_max-cc.uniform_dist_min) /
						(double) num_obs);
		for (int i=0; i<num_obs; ++i) {
			double di = (double) i;
			data[i].first = cc.uniform_dist_min + di*delta;
			data[i].second = i;
		}
        
	} else {
		std::vector<double> v;
		table_int->GetColData(col, tm, v);
        table_int->GetColUndefined(col, tm, data_undef);
		for (int i=0; i<num_obs; ++i) {
			data[i].first = v[i];
			data[i].second = i;
		}
		std::sort(data.begin(), data.end(), Gda::dbl_int_pair_cmp_less);
	}
	
	// ensure that CatClassifType and BreakValsType are consistent
	if (cc.cat_classif_type != CatClassification::custom) {
		cc.break_vals_type = CatClassification::by_cat_classif_type;
	}
	if (cc.cat_classif_type == CatClassification::custom &&
		cc.break_vals_type == CatClassification::by_cat_classif_type) {
		// this is an illegal combination, so default to quantile_break_vals 
		cc.break_vals_type = CatClassification::quantile_break_vals;
	}
	
	// ensure that num_cats is correct
	if (cc.num_cats < 1)
        cc.num_cats = 1;
    
	if (cc.num_cats > 10)
        cc.num_cats = 10;
    
	if (cc.cat_classif_type == CatClassification::no_theme ||
		cc.break_vals_type == CatClassification::no_theme_break_vals)
    {
		cc.num_cats = 1;
        
	} else if (cc.cat_classif_type == CatClassification::hinge_15 ||
			   cc.break_vals_type == CatClassification::hinge_15_break_vals ||
			   cc.cat_classif_type == CatClassification::hinge_30 ||
			   cc.break_vals_type == CatClassification::hinge_30_break_vals ||
			   cc.cat_classif_type == CatClassification::percentile ||
			   cc.break_vals_type == CatClassification::percentile_break_vals ||
			   cc.cat_classif_type == CatClassification::stddev ||
			   cc.break_vals_type == CatClassification::stddev_break_vals)
    {
		cc.num_cats = 6;
        
	} else if (cc.cat_classif_type == CatClassification::unique_values ||
			   cc.break_vals_type == CatClassification::unique_values_break_vals)
    {
		// need to determine number of unique values
		std::vector<double> v(num_obs);
		std::vector<bool> v_undef(num_obs);
        for (int i=0; i<num_obs; i++) {
            int ind = data[i].second;
            v[i] = data[i].first;
            v_undef[i] = data_undef[ind];
        }
		std::vector<UniqueValElem> uv_mapping;
		create_unique_val_mapping(uv_mapping, v, v_undef);
        
		int num_unique_vals = uv_mapping.size();
        if (num_unique_vals > 10) {
            num_unique_vals = 10;
        }
		cc.num_cats = num_unique_vals;
	}
	// otherwise the user can choose the number of categories
	
	// ensure that size of breaks, colors and names matches num_cats
	if (cc.breaks.size() != cc.num_cats-1) cc.breaks.resize(cc.num_cats-1);
	if (cc.colors.size() != cc.num_cats) cc.colors.resize(cc.num_cats);
	if (cc.names.size() != cc.num_cats) cc.names.resize(cc.num_cats);

	if (!uni_dist_mode) {
		// ensure that min/max and breaks are consistent with actual min/max
		double col_min = 0, col_max = 0;
		table_int->GetMinMaxVals(col, tm, col_min, col_max);
		cc.uniform_dist_min = col_min;
		cc.uniform_dist_max = col_max;
		for (int i=0, sz=cc.breaks.size(); i<sz; ++i) {
			if (cc.breaks[i] < col_min)
                cc.breaks[i] = col_min;
			if (cc.breaks[i] > col_max)
                cc.breaks[i] = col_max;
		}
	}
	
	if (cc.cat_classif_type == CatClassification::hinge_15 ||
		cc.break_vals_type == CatClassification::hinge_15_break_vals ||
		cc.cat_classif_type == CatClassification::hinge_30 ||
		cc.break_vals_type == CatClassification::hinge_30_break_vals ||
		cc.cat_classif_type == CatClassification::percentile ||
		cc.break_vals_type == CatClassification::percentile_break_vals ||
		cc.cat_classif_type == CatClassification::stddev ||
		cc.break_vals_type == CatClassification::stddev_break_vals ||
		cc.cat_classif_type == CatClassification::quantile ||
		cc.break_vals_type == CatClassification::quantile_break_vals ||
		cc.cat_classif_type == CatClassification::unique_values ||
		cc.break_vals_type == CatClassification::unique_values_break_vals ||
		cc.cat_classif_type == CatClassification::natural_breaks ||
		cc.break_vals_type == CatClassification::natural_breaks_break_vals ||
		cc.cat_classif_type == CatClassification::equal_intervals ||
		cc.break_vals_type == CatClassification::equal_intervals_break_vals)
	{
		// Calculate breaks from data
		CatClassification::CatClassifType cct = cc.cat_classif_type;
		if (cc.break_vals_type != CatClassification::by_cat_classif_type) {
			cct = BreakValsTypeToCatClassifType(cc.break_vals_type);
		}
		CatClassification::SetBreakPoints(cc.breaks, cc.names, data, data_undef, cct, cc.num_cats);
	}
	
	if (cc.color_scheme != CatClassification::custom_color_scheme)
	{
		// Calculate colors
		CatClassification::PickColorSet(cc.colors, cc.color_scheme,
										cc.num_cats);
	}
	
	bool changed = false;
	if (cc != _cc) {
		_cc = cc;
		changed = true;
	}
	return changed;
}

void CatClassification::FindNaturalBreaks(int num_cats,
                                          const Gda::dbl_int_pair_vec_type& var,
                                          const std::vector<bool>& var_undef,
                                          std::vector<double>& nat_breaks)
{
	int num_obs = var.size();
	std::vector<double> v(num_obs);
	std::vector<bool> v_undef(num_obs);
    
    for (int i=0; i<num_obs; i++) {
        v[i] = var[i].first;
        int ind = var[i].second;
        v_undef[i] = var_undef[ind];
    }
	// if there are fewer unique values than number of categories,
	// we will automatically reduce the number of categories to the
	// number of unique values.
	
	std::vector<UniqueValElem> uv_mapping;
	create_unique_val_mapping(uv_mapping, v, v_undef);
	int num_unique_vals = uv_mapping.size();
	int t_cats = GenUtils::min<int>(num_unique_vals, num_cats);
	
	double mean = 0;
    int valid_obs = 0;
    for (int i=0; i<num_obs; i++) {
        if (v_undef[i]) {
            continue;
        }
        valid_obs += 1;
        mean += v[i];
    }
	mean /= (double) valid_obs;
    
	double gssd = 0;
    for (int i=0; i<num_obs; i++) {
        if (v_undef[i]) {
            continue;
        }
        gssd += (v[i]-mean)*(v[i]-mean);
    }
	
	std::vector<int> rand_b(t_cats-1);
	std::vector<int> best_breaks(t_cats-1);
	std::vector<int> uv_rand_b(t_cats-1);
	double max_gvf_found = 0;
	int max_gvf_ind = 0;
    
	// for 5000 permutations, 2200 obs, and 4 time periods, slow enough
	// make sure permutations is such that this total is not exceeded.
	double c = 5000*2200*4;
	int perms = c / ((double) num_obs);
	if (perms < 10) perms = 10;
	if (perms > 10000) perms = 10000;
	
	for (int i=0; i<perms; i++) {
		pick_rand_breaks(uv_rand_b, num_unique_vals);
		// translate uv_rand_b into normal breaks
		unique_to_normal_breaks(uv_rand_b, uv_mapping, rand_b);
		double new_gvf = calc_gvf(rand_b, v, gssd);
		if (new_gvf > max_gvf_found) {
			max_gvf_found = new_gvf;
			max_gvf_ind = i;
			best_breaks = rand_b;
		}
	}
    
	nat_breaks.resize(best_breaks.size());
	for (int i=0, iend=best_breaks.size(); i<iend; i++) {
		nat_breaks[i] = var[best_breaks[i]].first;
	}
}
	
void
CatClassification::
SetNaturalBreaksCats(int num_cats,
                     const std::vector<Gda::dbl_int_pair_vec_type>& var,
                     const std::vector<std::vector<bool> >& var_undef,
                     CatClassifData& cat_data, std::vector<bool>& cats_valid,
                     CatClassification::ColorScheme coltype)
{
	int num_time_vals = var.size();
	int num_obs = var[0].size();
    
	// user supplied number of categories
	cat_data.CreateEmptyCategories(num_time_vals, num_obs);
    
	// if there are fewer unique values than number of categories,
	// we will automatically reduce the number of categories to the
	// number of unique values.

    std::vector<int> undef_cnts_tms(num_time_vals, 0);
    for (int t=0; t<num_time_vals; t++) {
        for (int i=0; i<var_undef[t].size(); i++) {
            if (var_undef[t][i]) {
                undef_cnts_tms[t] += 1;
            }
        }
    }
    
	for (int t=0; t<num_time_vals; t++) {
        std::vector<double> v(num_obs);
        
        for (int i=0; i<num_obs; i++) {
            double val = var[t][i].first;
            int ind = var[t][i].second;
            v[i] = val;
        }
        
		if (!cats_valid[t])
            continue;
        
		std::vector<UniqueValElem> uv_mapping;
		create_unique_val_mapping(uv_mapping, v, var_undef[t]);
        
		int num_unique_vals = uv_mapping.size();
		int t_cats = GenUtils::min<int>(num_unique_vals, num_cats);
		
		double mean = 0;
        int valid_obs = 0;
        for (int i=0; i<num_obs; i++) {
            double val = var[t][i].first;
            int ind = var[t][i].second;
            if (var_undef[t][ind])
                continue;
            mean += val;
            valid_obs += 1;
        }
		mean /= (double) valid_obs;
        
		double gssd = 0;
        for (int i=0; i<num_obs; i++) {
            double val = var[t][i].first;
            int ind = var[t][i].second;
            if (var_undef[t][ind])
                continue;
            gssd += (val-mean)*(val-mean);
        }
		
		std::vector<int> rand_b(t_cats-1);
		std::vector<int> best_breaks(t_cats-1);
		std::vector<int> uv_rand_b(t_cats-1);
        
		double max_gvf_found = 0;
		int max_gvf_ind = 0;
        
		// for 5000 permutations, 2200 obs, and 4 time periods, slow enough
		// make sure permutations is such that this total is not exceeded.
		double c = 5000*2200*4;
		int perms = c / ((double) num_time_vals * (double) valid_obs);
		if (perms < 10) perms = 10;
		if (perms > 10000) perms = 10000;
		
		for (int i=0; i<perms; i++) {
			pick_rand_breaks(uv_rand_b, num_unique_vals);
			// translate uv_rand_b into normal breaks
			unique_to_normal_breaks(uv_rand_b, uv_mapping, rand_b);
			double new_gvf = calc_gvf(rand_b, v, gssd);
			if (new_gvf > max_gvf_found) {
				max_gvf_found = new_gvf;
				max_gvf_ind = i;
				best_breaks = rand_b;
			}
		}
		
		cat_data.SetCategoryBrushesAtCanvasTm(coltype, t_cats, false, t);
        
        if (undef_cnts_tms[t]>0)
            cat_data.AppendUndefCategory(t, undef_cnts_tms[t]);
		
		for (int i=0, nb=best_breaks.size(); i<=nb; i++) {
			int ss = (i == 0) ? 0 : best_breaks[i-1];
			int tt = (i == nb) ? v.size() : best_breaks[i];
			for (int j=ss; j<tt; j++) {
                double val = var[t][j].first;
                int ind = var[t][j].second;
                int c = var_undef[t][ind] ? t_cats : i;
				cat_data.AppendIdToCategory(t, c, ind);
			}
			wxString l;
			l << "[" << GenUtils::DblToStr(var[t][ss].first);
			l << ":" << GenUtils::DblToStr(var[t][tt-1].first) << "]";
			cat_data.SetCategoryLabel(t, i, l);
			cat_data.SetCategoryCount(t, i, cat_data.GetNumObsInCategory(t, i));
			cat_data.SetCategoryMinMax(t, i, var[t][ss].first,
									   var[t][tt-1].first);
		}
	}
}

CatClassification::ColorScheme
CatClassification::GetColSchmForType(CatClassifType theme)
{
	if (theme == CatClassification::no_theme ||
		theme == CatClassification::custom) {
		return sequential_color_scheme;
	} else if (theme == CatClassification::unique_values) {
		return qualitative_color_scheme;
	} else if (theme == CatClassification::quantile ||
			   theme == CatClassification::natural_breaks ||
			   theme == CatClassification::equal_intervals) {
		return sequential_color_scheme;
	} else if (theme == CatClassification::percentile ||
			   theme == CatClassification::hinge_15 ||
			   theme == CatClassification::hinge_30 ||
			   theme == CatClassification::stddev ||
			   theme == CatClassification::excess_risk_theme) {
		return diverging_color_scheme;
	}
	return custom_color_scheme;
}

wxString CatClassification::CatClassifTypeToString(CatClassifType theme_type)
{
	if (theme_type == CatClassification::no_theme) {
		return "Themeless";
	} else if (theme_type == CatClassification::quantile) {
		return "Quantile";
	} else if (theme_type == CatClassification::unique_values) {
		return "Unique Values";
	} else if (theme_type == CatClassification::natural_breaks) {
		return "Natural Breaks";
	} else if (theme_type == CatClassification::equal_intervals) {
		return "Equal Intervals";
	} else if (theme_type == CatClassification::percentile) {
		return "Percentile";
	} else if (theme_type == CatClassification::hinge_15) {
		return "Hinge=1.5";
	} else if (theme_type == CatClassification::hinge_30) {
		return "Hinge=3.0";
	} else if (theme_type == CatClassification::stddev) {
		return "Standard Deviation";
	} else if (theme_type == CatClassification::excess_risk_theme) {
		return "Excess Risk";
	} else if (theme_type == CatClassification::lisa_categories ||
			   theme_type == CatClassification::lisa_significance) {
		return "LISA";
	} else if (theme_type == CatClassification::getis_ord_categories ||
			   theme_type == CatClassification::getis_ord_significance) {
		return "Getis-Ord";
	} else if (theme_type == CatClassification::local_geary_categories ||
			   theme_type == CatClassification::local_geary_significance) {
		return "Local Geary";
	} else if (theme_type == CatClassification::custom) {
		return "Custom";
	}
	return wxEmptyString;
}

/** The following color schemes come from Color Brewer 2.0 web application:
 http://colorbrewer2.org/ */
void CatClassification::PickColorSet(std::vector<wxColour>& color_vec,
								  ColorScheme coltype, int num_color,
								  bool reversed)
{
    
    
    if (coltype == unique_color_scheme) {
        color_vec.resize(num_color, *wxBLUE);
        wxColour unique_colors[20] = {
            wxColour(166,206,227),
            wxColour(31,120,180),
            wxColour(178,223,138),
            wxColour(51,160,44),
            wxColour(251,154,153),
            wxColour(227,26,28),
            wxColour(253,191,111),
            wxColour(255,127,0),
            wxColour(106,61,154),
            wxColour(255,255,153),
            wxColour(177,89,40),
            wxColour(255,255,179),
            wxColour(190,186,218),
            wxColour(251,128,114),
            wxColour(128,177,211),
            wxColour(179,222,105),
            wxColour(252,205,229),
            wxColour(217,217,217),
            wxColour(188,128,189),
            wxColour(204,235,197)
        };
        
        for (int i = 0; i < num_color; i++) {
            color_vec[i] = unique_colors[i];
        }
        return;
    }
    
	if (num_color < 1) num_color = 1;
	if (num_color > 10) num_color = 10;
	short colpos[11] = {0, 0, 1, 3, 6, 10, 15, 21, 28, 36, 45};
	
    if (color_vec.size() != num_color) {
        color_vec.resize(num_color, *wxBLUE);
    }
    
	wxColour Color1[56] = { //Sequential (colorblind safe)
		wxColour(217, 95, 14),
		
		wxColour(254, 196, 79), wxColour(217, 95, 14),
		
        wxColour(255, 247, 188), wxColour(254, 196, 79),
		wxColour(217, 95, 14),
		
        wxColour(255, 255, 212), wxColour(254, 217, 142),
		wxColour(254, 153, 41), wxColour(204, 76, 2),
		
        wxColour(255, 255, 212), wxColour(254, 217, 142),
		wxColour(254, 153, 41), wxColour(217, 95, 14),
		wxColour(153, 52, 4),
		
        wxColour(255, 255, 212), wxColour(254, 227, 145),
		wxColour(254, 196, 79), wxColour(254, 153, 41),
		wxColour(217, 95, 14), wxColour(153, 52, 4),
		
        wxColour(255, 255, 212), wxColour(254, 227, 145),
		wxColour(254, 196, 79), wxColour(254, 153, 41),
		wxColour(236, 112, 20), wxColour(204, 76, 2),
        wxColour(140, 45, 4),
		
        wxColour(255, 255, 229), wxColour(255, 247, 188),
		wxColour(254, 227, 145), wxColour(254, 196, 79),
		wxColour(254, 153, 41), wxColour(236, 112, 20),
        wxColour(204, 76, 2), wxColour(140, 45, 4),
		
        wxColour(255, 255, 229), wxColour(255, 247, 188),
		wxColour(254, 227, 145), wxColour(254, 196, 79),
		wxColour(254, 153, 41), wxColour(236, 112, 20),
        wxColour(204, 76, 2), wxColour(153, 52, 4),
		wxColour(102, 37, 6),
		
        wxColour(255, 255, 229), wxColour(255, 247, 188),
		wxColour(254, 227, 145), wxColour(254, 196, 79),
		wxColour(254, 153, 41), wxColour(236, 112, 20),
        wxColour(204, 76, 2), wxColour(153, 52, 4),
		wxColour(102, 37, 6), wxColour(80, 17, 5)
    };
	
    wxColour Color2[56] = { // Diverging (colorblind safe)
		wxColour(103, 169, 207),
		
		wxColour(239, 138, 98), wxColour(103, 169, 207),
		
		wxColour(239, 138, 98), wxColour(247, 247, 247),
		wxColour(103, 169, 207),
		
        wxColour(202, 0, 32), wxColour(244, 165, 130),
		wxColour(146, 197, 222), wxColour(5, 113, 176),
		
        wxColour(202, 0, 32), wxColour(244, 165, 130),
		wxColour(247, 247, 247), wxColour(146, 197, 222),
		wxColour(5, 113, 176),
		
        wxColour(178, 24, 43), wxColour(239, 138, 98),
		wxColour(253, 219, 199), wxColour(209, 229, 240),
		wxColour(103, 169, 207), wxColour(33, 102, 172),
		
        wxColour(178, 24, 43), wxColour(239, 138, 98),
		wxColour(253, 219, 199), wxColour(247, 247, 247),
        wxColour(209, 229, 240), wxColour(103, 169, 207),
		wxColour(33, 102, 172),
		
        wxColour(178, 24, 43), wxColour(214, 96, 77),
		wxColour(244, 165, 130), wxColour(253, 219, 199),
		wxColour(209, 229, 240), wxColour(146, 197, 222),
		wxColour(67, 147, 195), wxColour(33, 102, 172),
		
        wxColour(178, 24, 43), wxColour(214, 96, 77),
		wxColour(244, 165, 130), wxColour(253, 219, 199),
		wxColour(247, 247, 247), wxColour(209, 229, 240),
		wxColour(146, 197, 222), wxColour(67, 147, 195),
		wxColour(33, 102, 172),
		
        wxColour(103, 0, 31), wxColour(178, 24, 43),
		wxColour(214, 96, 77), wxColour(244, 165, 130),
		wxColour(253, 219, 199), wxColour(209, 229, 240),
		wxColour(146, 197, 222), wxColour(67, 147, 195),
		wxColour(33, 102, 172), wxColour(5, 48, 97)
    };
	
	wxColour Color3[56] = { // Qualitative (colorblind safe up to 4)
		wxColour(31, 120, 180),
		
		wxColour(31, 120, 180), wxColour(51, 160, 44),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138),
		
        wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		wxColour(251, 154, 153),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		wxColour(251, 154, 153), wxColour(227, 26, 28),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		wxColour(251, 154, 153), wxColour(227, 26, 28),
		wxColour(253, 191, 111),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		wxColour(251, 154, 153), wxColour(227, 26, 28),
		wxColour(253, 191, 111), wxColour(255, 127, 0),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		wxColour(251, 154, 153), wxColour(227, 26, 28),
		wxColour(253, 191, 111), wxColour(255, 127, 0),
		wxColour(202, 178, 214),
		
		wxColour(166, 206, 227), wxColour(31, 120, 180),
		wxColour(178, 223, 138), wxColour(51, 160, 44),
		wxColour(251, 154, 153), wxColour(227, 26, 28),
		wxColour(253, 191, 111), wxColour(255, 127, 0),
		wxColour(202, 178, 214), wxColour(106, 61, 154)
    };
	
    
    if (!reversed) {
        switch (coltype) {
            case sequential_color_scheme:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color1[colpos[num_color] + i];
                }
                break;
            case diverging_color_scheme:
				for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color2[colpos[num_color] + num_color - i-1];
                }
                break;
			case qualitative_color_scheme:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color3[colpos[num_color] + i];
                }
                if (num_color == 2) {
                    // hard code to unique values: blue and orange
                    color_vec[0] = Color3[1];
                    color_vec[1] = Color3[19];
                }
                
                break;
            default:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color1[colpos[num_color] + num_color - i-1];
                }
                break;
        }
    } else {
        switch (coltype) {
            case sequential_color_scheme:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color1[colpos[num_color] + num_color - i-1];
                }
                break;
            case diverging_color_scheme:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color2[colpos[num_color] + i];
                }
                break;
			case qualitative_color_scheme:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color3[colpos[num_color] + num_color - i-1];
                }
                break;
            default:
                for (int i = 0; i < num_color; i++) {
                    color_vec[i] = Color1[colpos[num_color] + num_color - i-1];
                }
                break;
        }
    }
}

/** When increasing or decreasing number of cats, will preserve existing data.
 Additionally, when a non custom-color-scheme is in use,  ApplyColorScheme
 is automatically called.
 */
void CatClassification::ChangeNumCats(int num_cats, CatClassifDef& cc)
{
	if (num_cats < 1) num_cats = 1;
	if (num_cats > 10) num_cats = 10;
	CatClassifDef t_cc;
	t_cc = cc;
	cc.num_cats = num_cats;
	cc.breaks.resize(num_cats-1);
	cc.names.resize(num_cats);
	cc.colors.resize(num_cats);
	for (int i=0; i<num_cats-1; i++) cc.breaks[i] = 0;
	for (int i=0; i<num_cats; i++) {
		cc.names[i] = "";
		cc.colors[i] = *wxBLUE;
	}
	for (int i=0; i<t_cc.breaks.size() && i<cc.breaks.size(); i++) {
		cc.breaks[i] = t_cc.breaks[i];
	}
	if (t_cc.num_cats < cc.num_cats) {
		int tnb = t_cc.breaks.size();
		for (int i=t_cc.num_cats-1; i<cc.num_cats-1; i++) {
			double last = 0;
			if (tnb > 0) last = t_cc.breaks[tnb-1];
			if (tnb > 1) {
				double d = t_cc.breaks[tnb-1]-t_cc.breaks[tnb-2];
				last += d*((double) ((i-tnb)+1));
			}
			cc.breaks[i] = last;
		}
	} 
	
	{
		int nn = cc.num_cats < t_cc.num_cats ? cc.num_cats : t_cc.num_cats;
		for (int i=0; i<nn; i++) {
			cc.names[i] = t_cc.names[i];
			cc.colors[i] = t_cc.colors[i];
		}
	}

	if (t_cc.num_cats < cc.num_cats) {
		for (int i=t_cc.num_cats; i<cc.num_cats; i++) {
			if (cc.names[i] == "") cc.names[i] << "category " << i+1;
		}
	}
	if (cc.color_scheme != custom_color_scheme) {
		ApplyColorScheme(cc.color_scheme, cc);
	}
	BOOST_FOREACH(double v, cc.breaks) {
		if (v < cc.uniform_dist_min) cc.uniform_dist_min = v;
		if (v > cc.uniform_dist_max) cc.uniform_dist_max = v;
	}
}

/** Change the given break value and automatically sort.
	Return new position of brk. */
int CatClassification::ChangeBreakValue(int brk, double new_val,
										CatClassifDef& cc)
{
	int num_brks = cc.breaks.size();
	if (brk < 0 || brk > num_brks-1) return brk;
	cc.breaks[brk] = new_val;
	std::sort(cc.breaks.begin(), cc.breaks.end());
	if (cc.breaks[brk] == new_val) return brk;
	for (int i=0; i<num_brks; i++) {
		if (cc.breaks[i] == new_val) return i;
	}
	BOOST_FOREACH(double v, cc.breaks) {
		if (v < cc.uniform_dist_min) cc.uniform_dist_min = v;
		if (v > cc.uniform_dist_max) cc.uniform_dist_max = v;
	}
	return brk;
}

void CatClassification::ChangeUnifDistMin(double new_unif_dist_min,
										 CatClassifDef& cc)
{
	cc.uniform_dist_min = new_unif_dist_min;
	if (cc.uniform_dist_min > cc.uniform_dist_max) {
		double t = cc.uniform_dist_max;
		cc.uniform_dist_max = cc.uniform_dist_min;
		cc.uniform_dist_min = t;
	}
	for (int i=0, sz=cc.breaks.size(); i<sz; ++i) {
		if (cc.breaks[i] > 
			cc.uniform_dist_min) cc.breaks[i] = cc.uniform_dist_min;
		if (cc.breaks[i] <
			cc.uniform_dist_max) cc.breaks[i] = cc.uniform_dist_max;
	}
}

void CatClassification::ChangeUnifDistMax(double new_unif_dist_max,
										  CatClassifDef& cc)
{
	cc.uniform_dist_max = new_unif_dist_max;
	if (cc.uniform_dist_min > cc.uniform_dist_max) {
		double t = cc.uniform_dist_max;
		cc.uniform_dist_max = cc.uniform_dist_min;
		cc.uniform_dist_min = t;
	}
	for (int i=0, sz=cc.breaks.size(); i<sz; ++i) {
		if (cc.breaks[i] > 
			cc.uniform_dist_min) cc.breaks[i] = cc.uniform_dist_min;
		if (cc.breaks[i] <
			cc.uniform_dist_max) cc.breaks[i] = cc.uniform_dist_max;
	}
}

void CatClassification::ApplyColorScheme(ColorScheme scheme, CatClassifDef& cc)
{
	if (cc.num_cats < 1 || cc.num_cats > 10) return;
	cc.color_scheme = scheme;
	if (cc.num_cats != cc.colors.size()) cc.colors.resize(cc.num_cats);
	if (scheme == custom_color_scheme) return;
	PickColorSet(cc.colors, scheme, cc.num_cats, false);
}

void CatClassification::PrintCatClassifDef(const CatClassifDef& cc,
										   wxString& str)
{
	str << "CatClassifDef details:\n";
	str << "  title: " << cc.title << "\n";
	str << "  num_cats: " << cc.num_cats << "\n";
	str << "  breaks: ";
	for (int i=0; i<cc.num_cats-1; i++) str << cc.breaks[i] << " ";
	str << "\nnames: ";
	for (int i=0; i<cc.num_cats; i++) str << cc.names[i] << " ";
	str << "\ncolors: ";
	for (int i=0; i<cc.num_cats; i++) {
		str << ColorToString(cc.colors[i]) << " ";
	}
	str << "\ncolor_scheme: ";
	if (cc.color_scheme == sequential_color_scheme) {
		str << "sequential_color_scheme";
	} else if (cc.color_scheme == diverging_color_scheme) {
		str << "diverging_color_scheme";
	} else if (cc.color_scheme == qualitative_color_scheme) {
		str << "qualitative_color_scheme";	
	} else if (cc.color_scheme == custom_color_scheme) {
		str << "custom_color_scheme";
	}
	str << "\n";
	str << "cat_classif_type: " << CatClassifTypeToString(cc.cat_classif_type);
	str << "\n";
}

wxString CatClassification::ColorToString(const wxColour& c)
{
	wxString s;
	s << "(" << (int) c.Red() << "," << (int) c.Blue();
	s << "," << (int) c.Green() << ")";
	return s;
}


CatClassification::BreakValsType
CatClassification::CatClassifTypeToBreakValsType(
									CatClassification::CatClassifType cct)
{
	if (cct == no_theme) return no_theme_break_vals;
	if (cct == quantile) return quantile_break_vals;
	if (cct == unique_values) return unique_values_break_vals;
	if (cct == natural_breaks) return natural_breaks_break_vals;
	if (cct == equal_intervals) return equal_intervals_break_vals;

	if (cct == percentile) return custom_break_vals; //percentile_break_vals
	if (cct == hinge_15) return custom_break_vals; //hinge_15_break_vals
	if (cct == hinge_30) return custom_break_vals; //hinge_30_break_vals
	if (cct == stddev) return custom_break_vals; //stddev_break_vals
	
	return custom_break_vals;
}

CatClassification::CatClassifType
CatClassification::BreakValsTypeToCatClassifType(
									CatClassification::BreakValsType bvt)
{
	if (bvt == no_theme_break_vals) return no_theme;
	if (bvt == quantile_break_vals) return quantile;
	if (bvt == unique_values_break_vals) return unique_values;
	if (bvt == natural_breaks_break_vals) return natural_breaks;
	if (bvt == equal_intervals_break_vals) return equal_intervals;

	if (bvt == percentile_break_vals) return custom; //percentile
	if (bvt == hinge_15_break_vals) return custom; //hinge_15
	if (bvt == hinge_30_break_vals) return custom; //hinge_30
	if (bvt == stddev_break_vals) return custom; //stddev
	
	return custom;	
}


CatClassifDef::CatClassifDef()
: cat_classif_type(CatClassification::custom),
break_vals_type(CatClassification::quantile_break_vals),
num_cats(5), automatic_labels(true),
color_scheme(CatClassification::sequential_color_scheme),
names(5), colors(5), uniform_dist_min(0), uniform_dist_max(1)
{
	names[0] = "category 1";
	CatClassification::PickColorSet(colors, color_scheme, num_cats, false);
}

CatClassifDef& CatClassifDef::operator=(const CatClassifDef& s)
{
	cat_classif_type = s.cat_classif_type;
	break_vals_type = s.break_vals_type;
	num_cats = s.num_cats;
	automatic_labels = s.automatic_labels;
	breaks.resize(s.breaks.size());
	for (int i=0; i<s.breaks.size(); i++) breaks[i] = s.breaks[i];
	names.resize(s.names.size());
	for (int i=0; i<s.names.size(); i++) names[i] = s.names[i];
	colors.resize(s.colors.size());
	for (int i=0; i<s.colors.size(); i++) colors[i] = s.colors[i];
	color_scheme = s.color_scheme;
	title = s.title;
	assoc_db_fld_name = s.assoc_db_fld_name;
	uniform_dist_min = s.uniform_dist_min;
	uniform_dist_max = s.uniform_dist_max;
	return *this;
}

bool CatClassifDef::operator==(const CatClassifDef& s) const
{
	if (cat_classif_type == s.cat_classif_type &&
		num_cats == s.num_cats &&
		cat_classif_type != CatClassification::custom) return true;
	if (cat_classif_type != s.cat_classif_type) return false;
	if (break_vals_type != s.break_vals_type) return false;
	if (num_cats != s.num_cats) return false;
	if (automatic_labels != s.automatic_labels) return false;
	if (breaks.size() != s.breaks.size()) return false;
	for (int i=0, sz=breaks.size(); i<sz; ++i) {
		if (breaks[i] != s.breaks[i]) return false;
	}
	if (names.size() != s.names.size()) return false;
	for (int i=0, sz=names.size(); i<sz; ++i) {
		if (names[i] != s.names[i]) return false;
	}
	if (colors.size() != s.colors.size()) return false;
	for (int i=0, sz=colors.size(); i<sz; ++i) {
		if (colors[i] != s.colors[i]) return false;
	}
	if (color_scheme != s.color_scheme) return false;
	if (title != s.title) return false;
	if (assoc_db_fld_name != s.assoc_db_fld_name) return false;
	if (uniform_dist_min != s.uniform_dist_min) return false;
	if (uniform_dist_max != s.uniform_dist_max) return false;
	return true;
}

bool CatClassifDef::operator!=(const CatClassifDef& s) const
{
	return !(operator==(s));
}

wxString CatClassifDef::ToStr() const
{
	wxString s;
	s << "Categories Definition:\n";
	s << "  title: " << title << "\n";
	s << "  cat_classif_type: " << cat_classif_type << "\n";
	s << "  automatic_labels: " << automatic_labels << "\n";
	BOOST_FOREACH(double n, breaks) {
		s << "  break: " << n << "\n";
	}
	s << "  break_vals_type: " << break_vals_type << "\n";
	BOOST_FOREACH(const wxString& n, names) {
		s << "  name: " << n << "\n";
	}
	BOOST_FOREACH(const wxColour& n, colors) {
		s << "  color: " << "(" << (int) n.Red() << ",";
		s << (int) n.Green() << "," << (int) n.Blue() << ")" << "\n";
	}
	s << "  color_scheme: ";
	if (color_scheme == CatClassification::sequential_color_scheme) {
		s << "sequential_color_scheme";
	} else if (color_scheme == CatClassification::diverging_color_scheme) {
		s << "diverging_color_scheme";
	} else if (color_scheme == CatClassification::qualitative_color_scheme) {
		s << "qualitative_color_scheme";
	} else { // CatClassification::custom_color_scheme 
		s << "custom_color_scheme";
	}
	s << "\n";
	s << "  num_cats: " << num_cats << "\n";
	s << "  assoc_db_fld_name: " << assoc_db_fld_name << "\n";
	s << "  uniform_dist_min: " << uniform_dist_min << "\n";
	s << "  uniform_dist_max: " << uniform_dist_max << "\n";
	return s;
}

void CatClassifData::CreateEmptyCategories(int num_canvas_tms, int num_obs)
{
	canvas_tm_steps = num_canvas_tms;
	categories.clear();
	categories.resize(num_canvas_tms);
	for (int t=0; t<num_canvas_tms; t++) {
		categories[t].id_to_cat.resize(num_obs);
	}
	curr_canvas_tm_step = 0;
}

void CatClassifData::ExchangeLabels(int from, int to)
{
    
    if (from < 0 || to < 0 || from == to)
        return;
    
    int tms = categories.size();
    
    for (int t = 0; t < tms; t++) {
        int sz = categories[t].cat_vec.size();
        if( from > sz || to > sz) {
            return;
        }
        wxBrush from_brush = categories[t].cat_vec[from].brush;
        wxBrush to_brush = categories[t].cat_vec[to].brush;
        wxPen from_pen = categories[t].cat_vec[from].pen;
        wxPen to_pen = categories[t].cat_vec[to].pen;
    
        Category tmp = categories[t].cat_vec[from];
        categories[t].cat_vec[from] = categories[t].cat_vec[to];
        categories[t].cat_vec[to] = tmp;
    
        categories[t].cat_vec[from].brush = from_brush;
        categories[t].cat_vec[to].brush = to_brush;
        categories[t].cat_vec[from].pen = from_pen;
        categories[t].cat_vec[to].pen = to_pen;

    }
}

void CatClassifData::AppendUndefCategory(int t, int count)
{
    Category c_undef;
    c_undef.brush.SetColour(wxColour(70, 70, 70));
    //c_undef.pen.SetColour(wxColour(0,0,0));
    c_undef.label = "undefined";
    c_undef.min_val = 0;
    c_undef.max_val = 0;
    c_undef.count = count;
    c_undef.min_max_defined = false;
    
    categories[t].cat_vec.push_back(c_undef);
}

void CatClassifData::CreateCategoriesAllCanvasTms(int num_cats,
                                                  int num_canvas_tms,
                                                  int num_obs)
{
	canvas_tm_steps = num_canvas_tms;
	categories.clear();
	categories.resize(num_canvas_tms);
	for (int t=0; t<num_canvas_tms; t++) {
		categories[t].id_to_cat.resize(num_obs);
        categories[t].cat_vec.resize(num_cats);
	}
	curr_canvas_tm_step = 0;
}

void CatClassifData::CreateCategoriesAtCanvasTm(int num_cats, int canvas_tm)
{
	categories[canvas_tm].cat_vec.resize(num_cats);
}

void CatClassifData::SetCategoryBrushesAllCanvasTms(
											std::vector<wxColour> colors)
{
	for (int t=0; t<categories.size(); t++) {
		for (int i=0; i<colors.size(); i++) {
			categories[t].cat_vec[i].brush.SetColour(colors[i]);
			categories[t].cat_vec[i].pen.SetColour(
				GdaColorUtils::ChangeBrightness(colors[i]));
		}
	}
}

void CatClassifData::SetCategoryBrushesAllCanvasTms(
										CatClassification::ColorScheme coltype,
												  int ncolor, bool reversed)
{
	std::vector<wxColour> colors;
	CatClassification::PickColorSet(colors, coltype, ncolor, reversed);
	for (int t=0; t<categories.size(); t++) {
		for (int i=0; i<colors.size(); i++) {
			categories[t].cat_vec[i].brush.SetColour(colors[i]);
			categories[t].cat_vec[i].pen.SetColour(
							GdaColorUtils::ChangeBrightness(colors[i]));
		}
	}
}

void CatClassifData::SetCategoryBrushesAtCanvasTm(
									CatClassification::ColorScheme coltype,
									int ncolor, bool reversed, int canvas_tm)
{
	categories[canvas_tm].cat_vec.resize(ncolor);
	std::vector<wxColour> colors;
	CatClassification::PickColorSet(colors, coltype, ncolor, reversed);
	for (int i=0; i<colors.size(); i++) {
		categories[canvas_tm].cat_vec[i].brush.SetColour(colors[i]);
		categories[canvas_tm].cat_vec[i].pen.SetColour(
							   GdaColorUtils::ChangeBrightness(colors[i]));
	}
}

int CatClassifData::GetNumCategories(int canvas_tm)
{
	return categories[canvas_tm].cat_vec.size();
}

int CatClassifData::GetNumObsInCategory(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return 0;
	return categories[canvas_tm].cat_vec[cat].ids.size();
}

std::vector<int>& CatClassifData::GetIdsRef(int canvas_tm, int cat)
{
	return categories[canvas_tm].cat_vec[cat].ids;
}

void CatClassifData::SetCategoryColor(int canvas_tm, int cat, wxColour color)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size())
        return;
	categories[canvas_tm].cat_vec[cat].brush.SetColour(color);
	categories[canvas_tm].cat_vec[cat].pen.SetColour(
								 GdaColorUtils::ChangeBrightness(color));
}

wxColour CatClassifData::GetCategoryColor(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size())
        return *wxBLACK;
	return
        categories[canvas_tm].cat_vec[cat].brush.GetColour();
}

wxBrush CatClassifData::GetCategoryBrush(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) {
		return *wxBLACK_BRUSH;
	}
	wxBrush br = categories[canvas_tm].cat_vec[cat].brush;
    if (br.IsOk() && br.GetColour().IsOk()) return br;
    categories[canvas_tm].cat_vec[cat].brush.SetColour(*wxBLACK);
    return *wxBLACK_BRUSH;
}

wxPen CatClassifData::GetCategoryPen(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) {
		return *wxBLACK_PEN;
	}
	wxPen pen = categories[canvas_tm].cat_vec[cat].pen;
    if (pen.IsOk() && pen.GetColour().IsOk()) return pen;
    categories[canvas_tm].cat_vec[cat].pen.SetColour(*wxBLACK);
    categories[canvas_tm].cat_vec[cat].pen.SetWidth(1);
    return *wxBLACK_PEN;
}

void CatClassifData::AppendIdToCategory(int canvas_tm, int cat, int id)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size())
        return;
	categories[canvas_tm].cat_vec[cat].ids.push_back(id);
	categories[canvas_tm].id_to_cat[id] = cat;
}

void CatClassifData::ClearAllCategoryIds()
{
	for (int t=0; t<categories.size(); t++) {
		for (int cat=0; cat<categories[t].cat_vec.size(); cat++) {
			categories[t].cat_vec[cat].ids.clear();
		}
	}
}

wxString CatClassifData::GetCatLblWithCnt(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) {
		return wxEmptyString;
	}
	wxString s;
	s << categories[canvas_tm].cat_vec[cat].label;
	s << " (" << categories[canvas_tm].cat_vec[cat].count << ")";
	s << " " << categories[canvas_tm].cat_vec[cat].label_ext;
	return s;
}


wxString CatClassifData::GetCategoryLabel(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) {
		return wxEmptyString;
	}
	return categories[canvas_tm].cat_vec[cat].label;
}

void CatClassifData::SetCategoryLabel(int canvas_tm, int cat,
									const wxString& label)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return;
	categories[canvas_tm].cat_vec[cat].label = label;
}

void CatClassifData::SetCategoryLabelExt(int canvas_tm, int cat,
									const wxString& label)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return;
	categories[canvas_tm].cat_vec[cat].label_ext = label;
}

int CatClassifData::GetCategoryCount(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return 0;
	return categories[canvas_tm].cat_vec[cat].count;
}

void CatClassifData::SetCategoryCount(int canvas_tm, int cat, int count)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return;
	categories[canvas_tm].cat_vec[cat].count = count;
}

void CatClassifData::ResetCategoryMinMax(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return;
	categories[canvas_tm].cat_vec[cat].min_val =
	std::numeric_limits<double>::max();
	categories[canvas_tm].cat_vec[cat].max_val =
	-std::numeric_limits<double>::max();
	categories[canvas_tm].cat_vec[cat].min_max_defined = false;
}

void CatClassifData::ResetAllCategoryMinMax(int canvas_tm)
{
	for (int cat = 0; cat < categories[canvas_tm].cat_vec.size(); cat++) {
		ResetCategoryMinMax(canvas_tm, cat);
	}
}

void CatClassifData::ResetAllCategoryMinMax()
{
	for (int t=0; t<categories.size(); t++) {
		ResetAllCategoryMinMax(t);
	}
}

void CatClassifData::UpdateCategoryMinMax(int canvas_tm, int cat,
										const double& val)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return;
	if (val < categories[canvas_tm].cat_vec[cat].min_val) {
		categories[canvas_tm].cat_vec[cat].min_val = val;
	}
	if (val > categories[canvas_tm].cat_vec[cat].max_val) {
		categories[canvas_tm].cat_vec[cat].max_val = val;
	}
	categories[canvas_tm].cat_vec[cat].min_max_defined = true;
}

void CatClassifData::SetCategoryMinMax(int canvas_tm, int cat,
									 const double& min, const double& max)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return;
	categories[canvas_tm].cat_vec[cat].min_val = min;
	categories[canvas_tm].cat_vec[cat].max_val = max;
	categories[canvas_tm].cat_vec[cat].min_max_defined = true;
}

bool CatClassifData::IsMinMaxDefined(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return false;
	return categories[canvas_tm].cat_vec[cat].min_max_defined;
}

bool CatClassifData::IsCategoryEmpty(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return true;
	return (categories[canvas_tm].cat_vec[cat].ids.size() == 0);
}

double CatClassifData::GetCategoryMin(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return 0;
	return categories[canvas_tm].cat_vec[cat].min_val;
}

double CatClassifData::GetCategoryMax(int canvas_tm, int cat)
{
	if (cat <0 || cat >= categories[canvas_tm].cat_vec.size()) return 0;
	return categories[canvas_tm].cat_vec[cat].max_val;
}

/// cat_brk: from 0 to num_cats-1
bool CatClassifData::HasBreakVal(int canvas_tm, int cat_brk)
{
	if (cat_brk <0 ||
		cat_brk >= categories[canvas_tm].cat_vec.size()-1) return false;
	return (IsMinMaxDefined(canvas_tm, cat_brk) ||
			IsMinMaxDefined(canvas_tm, cat_brk+1));
}

/// cat_brk: from 0 to num_cats-1
double CatClassifData::GetBreakVal(int canvas_tm, int cat_brk)
{
	if (cat_brk <0 ||
		cat_brk >= categories[canvas_tm].cat_vec.size()-1) return 0;
	if (IsMinMaxDefined(canvas_tm, cat_brk) &&
		IsMinMaxDefined(canvas_tm, cat_brk+1)) {
		return (GetCategoryMax(canvas_tm, cat_brk) +
				GetCategoryMin(canvas_tm, cat_brk+1))/2.0;
	}
	if (IsMinMaxDefined(canvas_tm, cat_brk) &&
		!IsMinMaxDefined(canvas_tm, cat_brk+1)) {
		return GetCategoryMax(canvas_tm, cat_brk);
	}
	if (!IsMinMaxDefined(canvas_tm, cat_brk) &&
		IsMinMaxDefined(canvas_tm, cat_brk+1)) {
		return GetCategoryMin(canvas_tm, cat_brk+1);
	}
	return 0;
}

int CatClassifData::GetCurrentCanvasTmStep()
{
	return curr_canvas_tm_step;
}

void CatClassifData::SetCurrentCanvasTmStep(int canvas_tm)
{
	if (canvas_tm >= 0 || canvas_tm < canvas_tm_steps) {
		curr_canvas_tm_step = canvas_tm;
	}
}

int CatClassifData::GetCanvasTmSteps()
{
	return canvas_tm_steps;
}

