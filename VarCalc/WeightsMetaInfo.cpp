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

#include "WeightsMetaInfo.h"

WeightsMetaInfo::WeightsMetaInfo()
{
	SetToDefaults();
}

void WeightsMetaInfo::SetToDefaults()
{
	id_var = "";
	filename = "";
	sym_type = SYM_unknown;
	order = 1;
	inc_lower_orders = false;
	dist_metric = DM_unspecified;
	dist_units = DU_unspecified;
	dist_values = DV_unspecified;
	num_neighbors = 0;
	threshold_val = 0;
	dist_var1 = "";
	dist_var2 = "";
	dist_tm1 = -1;
	dist_tm2 = -1;
}

void WeightsMetaInfo::SetToCustom(const wxString& idv)
{
	SetToDefaults();
	id_var = idv;
}

void WeightsMetaInfo::SetToRook(const wxString& idv,
								long order_, bool inc_lower_orders_)
{
	SetToDefaults();
	id_var = idv;
	weights_type = WT_rook;
	sym_type = SYM_symmetric;
	order = order_;
	inc_lower_orders = inc_lower_orders_;
	//if (order < 2)
    //    inc_lower_orders = true;
}

void WeightsMetaInfo::SetToQueen(const wxString& idv,
								 long order_, bool inc_lower_orders_)
{
	SetToDefaults();
	id_var = idv;
	weights_type = WT_queen;
	sym_type = SYM_symmetric;
	order = order_;
	inc_lower_orders = inc_lower_orders_;
	//if (order < 2)
    //    inc_lower_orders = true;
}

void WeightsMetaInfo::SetToThres(const wxString& idv,
								 DistanceMetricEnum dist_metric_,
								 DistanceUnitsEnum dist_units_,
                                 wxString dist_units_str_,
								 DistanceValuesEnum dist_values_,
								 double threshold_val_,
								 wxString dist_var1_, long dist_tm1_,
								 wxString dist_var2_, long dist_tm2_)
{
	SetToDefaults();
	id_var = idv;
	weights_type = WT_threshold;
	sym_type = SYM_symmetric;
	dist_metric = dist_metric_;
	dist_units = dist_units_;
    dist_units_str = dist_units_str_;
	dist_values = dist_values_;
	threshold_val = threshold_val_;
	if (!dist_var1_.IsEmpty()) {
		dist_var1 = dist_var1_;
		if (dist_tm1_ >= 0) dist_tm1 = dist_tm1_;
	}
	if (!dist_var2_.IsEmpty()) {
		dist_var2 = dist_var2_;
		if (dist_tm2_ >= 0) dist_tm2 = dist_tm2_;
	}
}

void WeightsMetaInfo::SetToKnn(const wxString& idv,
							   DistanceMetricEnum dist_metric_,
							   DistanceUnitsEnum dist_units_,
                               wxString dist_units_str_,
							   DistanceValuesEnum dist_values_,
							   long k,
							   wxString dist_var1_, long dist_tm1_,
							   wxString dist_var2_, long dist_tm2_)
{
	SetToDefaults();
	id_var = idv;
	weights_type = WT_knn;
	sym_type = SYM_asymmetric;
	dist_metric = dist_metric_;
	dist_units = dist_units_;
    dist_units_str = dist_units_str_;
	dist_values = dist_values_;
	num_neighbors = k;
	if (!dist_var1_.IsEmpty()) {
		dist_var1 = dist_var1_;
		if (dist_tm1_ >= 0) dist_tm1 = dist_tm1_;
	}
	if (!dist_var2_.IsEmpty()) {
		dist_var2 = dist_var2_;
		if (dist_tm2_ >= 0) dist_tm2 = dist_tm2_;
	}
}

wxString WeightsMetaInfo::ToStr() const
{
	wxString s;
	s << "Weights Meta Info:\n";
	s << "  filename: " << filename << "\n";
	s << "  id_var: " << id_var << "\n";
	if (weights_type == WT_custom) {
		s << "  weights_type: WT_custom\n";
	} else if (weights_type == WT_rook || weights_type == WT_queen) {
		if (weights_type == WT_rook) {
			s << "  weights_type: WT_rook\n";
		} else {
			s << "  weights_type: WT_queen\n";
		}
		s << "  order: " << order << "\n";
		s << "  inc_lower_orders: " << inc_lower_orders << "\n";
	} else {
		if (weights_type == WT_threshold) {
			s << "  weights_type: WT_threshold\n";
		} else {
			s << "  weights_type: WT_knn\n";
		}
		s << "  dist_metric: " << dist_metric << "\n";
		s << "  dist_units: " << dist_units << "\n";
		s << "  dist_values: " << dist_values << "\n";
		s << "  dist_var1: " << dist_var1 << "\n";
		if (dist_tm1 >= 0) s << "  dist_tm1: " << dist_tm1 << "\n";
		s << "  dist_var2: " << dist_var2 << "\n";
		if (dist_tm2 >= 0) s << "  dist_tm2: " << dist_tm2 << "\n";
		if (weights_type == WT_threshold) {
			s << "  threshold_val: " << threshold_val << "\n";
		} else {
			s << "  num_neighbors: " << num_neighbors << "\n";
		}
	}
	return s;
}

wxString WeightsMetaInfo::TypeToStr() const
{
	if (weights_type == WT_rook) {
		return "rook";
	} else if (weights_type == WT_queen) {
		return "queen";
	} else if (weights_type == WT_threshold) {
		return "threshold";
	} else if (weights_type == WT_knn) {
		return "k-NN";
	}
	return "custom";
}

wxString WeightsMetaInfo::SymToStr() const
{
	if (sym_type == SYM_symmetric) {
		return "symmetric";
	} else if  (sym_type == SYM_asymmetric) {
		return "asymmetric";
	}
	return "unknown";
}

wxString WeightsMetaInfo::DistValsToStr() const
{
	if (dist_values == DV_centroids) {
		return "centroids";
	} else if (dist_values == DV_mean_centers) {
		return "mean centers";
	} else if (dist_values == DV_vars) {
		wxString s;
		if (!dist_var1.IsEmpty()) {
			s << dist_var1;
			if (dist_tm1 >= 0) s << " (t=" << dist_tm1 << ")";
		}
		if (!dist_var2.IsEmpty()) {
			s << ", ";
			s << dist_var2;
			if (dist_tm2 >= 0) s << " (t=" << dist_tm2 << ")";
		}
		return s;
	}
	return "unspecified";
}

wxString WeightsMetaInfo::DistMetricToStr() const
{
	if (dist_metric == DM_euclidean) {
		return "Euclidean";
	} else if (dist_metric == DM_arc) {
		return "arc";
	}
	return "unspecified";
}

wxString WeightsMetaInfo::DistUnitsToStr() const
{
	if (dist_units == DU_km) {
		return "km";
	} else if (dist_units == DU_mile) {
		return "mile";
    } else  {
        // dist_units == DU_unspecified
        if (!dist_units_str.IsEmpty()) {
            return dist_units_str;
        } else {
            return "unspecified";
        }
    }
}


bool operator<(const WeightsMetaInfo& lh, const WeightsMetaInfo& rh)
{
	if (lh.weights_type != rh.weights_type) {
		return lh.weights_type < rh.weights_type;
	}
	// we know the weight types are identical
	if (lh.weights_type == WeightsMetaInfo::WT_custom) {
		return lh.filename < rh.filename;
	}
	if (lh.weights_type == WeightsMetaInfo::WT_rook ||
		lh.weights_type == WeightsMetaInfo::WT_queen) {
		if (lh.order != rh.order) return lh.order < rh.order;
		if (lh.inc_lower_orders != rh.inc_lower_orders) {
			return lh.inc_lower_orders == false;
		} else {
			return true;
		}
	}
	// distance-based weights
	if (lh.dist_metric != rh.dist_metric) {
		return lh.dist_metric < rh.dist_metric;
	}
	if (lh.dist_units != rh.dist_units) {
		return lh.dist_units < rh.dist_units;
	}
	if (lh.dist_values != rh.dist_values) {
		return lh.dist_values < rh.dist_values;
	}
	if (lh.dist_var1 != rh.dist_var1) {
		return lh.dist_var1 < rh.dist_var1;
	}
	if (lh.dist_tm1 != rh.dist_tm1) {
		return lh.dist_tm1 < rh.dist_tm1;
	}
	if (lh.dist_var2 != rh.dist_var2) {
		return lh.dist_var2 < rh.dist_var2;
	}
	if (lh.dist_tm2 != rh.dist_tm2) {
		return lh.dist_tm2 < rh.dist_tm2;
	}
	if (lh.weights_type == WeightsMetaInfo::WT_threshold) {
		return lh.threshold_val < rh.threshold_val;
	}
	// must be knn
	return lh.num_neighbors < rh.num_neighbors;
}


bool operator>(const WeightsMetaInfo& lh, const WeightsMetaInfo& rh)
{
	return operator<(rh,lh);
}

bool operator==(const WeightsMetaInfo& lh, const WeightsMetaInfo& rh)
{
	return !(operator<(lh,rh)) && !(operator<(rh,lh));
}

bool operator!=(const WeightsMetaInfo& lh, const WeightsMetaInfo& rh)
{
	return operator<(lh,rh) || operator<(rh,lh);
}


