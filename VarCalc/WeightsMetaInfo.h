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

#ifndef __GEODA_CENTER_WEIGHTS_META_INFO_H__
#define __GEODA_CENTER_WEIGHTS_META_INFO_H__

#include <vector>
#include <wx/string.h>

struct WeightsMetaInfo
{
	enum WeightTypeEnum {
		WT_custom, WT_rook, WT_queen, WT_threshold, WT_knn
	};
	enum SymmetryEnum {
		SYM_unknown, SYM_symmetric, SYM_asymmetric
	};
	enum DistanceValuesEnum {
		DV_unspecified, DV_centroids, DV_mean_centers, DV_vars
	};
	enum DistanceMetricEnum {
		DM_unspecified, DM_euclidean, DM_arc
	};
	enum DistanceUnitsEnum {
		DU_unspecified, DU_km, DU_mile 
	};
	
	WeightsMetaInfo();
	void SetToDefaults();
	void SetToCustom(const wxString& id_var);
	void SetToRook(const wxString& id_var, long order=1,
				   bool inc_lower_orders=false);
	void SetToQueen(const wxString& id_var, long order=1,
					bool inc_lower_orders=false);
	void SetToThres(const wxString& id_var,
					DistanceMetricEnum dist_metric,
					DistanceUnitsEnum dist_units,
                    wxString dist_units_str,
					DistanceValuesEnum dist_values,
					double threshold_val,
					wxString dist_var_1 = "", long dist_tm_1 = -1,
					wxString dist_var_2 = "", long dist_tm_2 = -1);
	void SetToKnn(const wxString& id_var,
				  DistanceMetricEnum dist_metric,
				  DistanceUnitsEnum dist_units,
                  wxString dist_units_str,
				  DistanceValuesEnum dist_values,
				  long k,
				  wxString dist_var_1 = "", long dist_tm_1 = -1,
				  wxString dist_var_2 = "", long dist_tm_2 = -1);

	wxString filename; // weights file filename if exists
	wxString id_var; // if empty, then record order assumed
	SymmetryEnum sym_type;
	
	WeightTypeEnum weights_type;
	
	// Used by contiguity weights
	long order;
	bool inc_lower_orders;
	
	// Used by distance weights
	DistanceMetricEnum dist_metric;
	DistanceUnitsEnum dist_units;
	DistanceValuesEnum dist_values;
    
    wxString dist_units_str;
	
	wxString dist_var1; // x-coord
	wxString dist_var2; // y-coord
	// optional time period for x and y.  -1 indicates that variable is
	// not time-variant.  In this case dist_tm1(2) should not be
	// written to gda file.
	long dist_tm1; 
	long dist_tm2;
	
	// Used by knn
	long num_neighbors; // 1 or more.  If knn_make_sym then this will be boosted
	
	// Used by threshold distance
	double threshold_val; // any real
	
	wxString ToStr() const;
	wxString TypeToStr() const;
	wxString SymToStr() const;
	wxString DistValsToStr() const;
	wxString DistMetricToStr() const;
	wxString DistUnitsToStr() const;

	friend bool operator<(const WeightsMetaInfo& lh,
						  const WeightsMetaInfo& rh);
	friend bool operator>(const WeightsMetaInfo& lh,
						  const WeightsMetaInfo& rh);
	friend bool operator==(const WeightsMetaInfo& lh,
						   const WeightsMetaInfo& rh);
	friend bool operator!=(const WeightsMetaInfo& lh,
						   const WeightsMetaInfo& rh);
};

#endif

