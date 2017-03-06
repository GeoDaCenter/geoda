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

#ifndef __GEODA_CENTER_CORRELOGRAM_ALGS_H__
#define __GEODA_CENTER_CORRELOGRAM_ALGS_H__

#include <vector>
#include <wx/gdicmn.h> // for wxRealPoint

namespace CorrelogramAlgs {
	
	struct CorreloBin {
		CorreloBin():
		dist_min(0), dist_max(0), corr_avg(0),
		corr_avg_valid(false), num_pairs(0) {}
		double dist_min; // >
		double dist_max; // <=
		double corr_avg;
		bool corr_avg_valid; // If corr_avg is valid, then true.  If false,
		// only num_pairs count is valid.  Can be useful for displaying just
		// the histogram of number of pairs in each distance band bin.
		int num_pairs; // number of pairs sampled
	};
	
	void GetSampMeanAndVar(const std::vector<double>& Z,
                           const std::vector<bool>& Z_undef,
                           double& mean, double& var);
	
	/**
	 Input:
	   pts: vector of input points
	   is_arc: if true than input points are earth lat/long coordinates
	     and distances are returned in radians, otherwise points are treated
	     as points on a Cartesian plane.
		 dist_cutoff: if <= 0, then compute max distance otherwise
									throw away results for distances > dist_cutoff
	   num_bins: number of distance band categories
	   iters: number of random trials
	 Output:
	   out: vector of CorreloBin output objects of size num_cats
		 true if success, false if sample variance <= 0
	 */
    bool MakeCorrRandSamp(const std::vector<wxRealPoint>& pts,
                          const std::vector<double>& Z,
                          const std::vector<bool>& Z_undef,
                          bool is_arc, double dist_cutoff,
                          int num_bins, int iters,
                          std::vector<CorreloBin>& out);

	bool MakeCorrAllPairs(const std::vector<wxRealPoint>& pts,
						  const std::vector<double>& Z,
                          const std::vector<bool>& Z_undef,
						  bool is_arc, int num_bins,
						  std::vector<CorreloBin>& out);
	
	/** Compute Correlogram for all pairs within thresh distaance
	 cuttoff.  The resulting number of pairs can be very large,
	 so it is recommended to use SpatialIndAlgs::est_thresh_for_avg_num_neigh
	 to choose the threshold.  */
	bool MakeCorrThresh(const rtree_pt_2d_t& rtree,
						const std::vector<double>& Z,
                        const std::vector<bool>& Z_undef,
						double thresh, int num_bins,
						std::vector<CorreloBin>& out);
	
    bool MakeCorrThresh(const rtree_pt_3d_t& rtree,
                        const std::vector<double>& Z,
                        const std::vector<bool>& Z_undef,
                        double thresh, int num_bins,
                        std::vector<CorreloBin>& out);
}

#endif
