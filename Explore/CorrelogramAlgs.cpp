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

#include <list>
#include <map>
#include <set>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <wx/stopwatch.h>
#include "../SpatialIndAlgs.h"
#include "../GenGeomAlgs.h"
#include "../PointSetAlgs.h"
#include "../logger.h"
#include "CorrelogramAlgs.h"


void CorrelogramAlgs::GetSampMeanAndVar(const std::vector<double>& Z_,
                                        const std::vector<bool>& Z_undef,
										double& mean, double& var)
{
    // get valid Z using Z_undef
    std::vector<double> Z;
    for (size_t i=0; i<Z_.size(); i++) {
        if (Z_undef[i] == false) {
            Z.push_back(Z_[i]);
        }
    }
    // get mean & var
	size_t Z_sz = Z.size();
	double N = (double) Z_sz;
	double sum = 0.0;
    for (size_t i=0; i<Z_sz; ++i) {
        sum += Z[i];
    }
	double smpl_mn = sum/N;
	double ssd = 0.0;
	double diff = 0;
	for (size_t i=0; i<Z_sz; ++i) {
		diff = Z[i] - smpl_mn;
		ssd += diff*diff;
	}
	double smpl_var = ssd/N;
    
	mean = smpl_mn;
	var = smpl_var;
}

bool CorrelogramAlgs::MakeCorrRandSamp(const std::vector<wxRealPoint>& pts,
									   const std::vector<double>& Z,
									   const std::vector<bool>& Z_undef,
									   bool is_arc,
									   double dist_cutoff,
									   int num_bins, int iters,
									   std::vector<CorreloBin>& out)
{
	using namespace std;
	using namespace GenGeomAlgs;
	LOG_MSG("Entering CorrelogramAlgs::MakeCorrRandSamp");
	wxStopWatch sw;
	
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::random::uniform_int_distribution<> X(0, pts.size()-1);
	
	double nbins_d = (double) num_bins;
	if (dist_cutoff <= 0) {
		wxRealPoint a,b;
		dist_cutoff = PointSetAlgs::EstDiameter(pts, is_arc, a, b);
		if (is_arc) dist_cutoff = DegToRad(dist_cutoff);
	}
	
	bool calc_prods = (Z.size() == pts.size());
	double mean = 0;
	double var = 0;
	if (calc_prods) {
		GetSampMeanAndVar(Z, Z_undef, mean, var);
		if (var <= 0) {
			return false;
		}
	}
	if (num_bins <= 0)
        num_bins = 1;
	out.clear();
	out.resize(num_bins);
	double binw = dist_cutoff/nbins_d; // bin width
	out[0].dist_min = 0;
	out[0].dist_max = binw;
	for (int i=1; i<num_bins; ++i) {
		out[i].dist_min = binw*((double) i);
		out[i].dist_max = binw*((double) (i+1));
		out[i].corr_avg_valid = calc_prods;
	}
	
	int ta_cnt = 0; // throw away count
    int t = 0;
    int t_max = 0;
    int t_max_const = iters + 9999999;
    while (t < iters ) {
		size_t i=X(rng);
		size_t j=X(rng);
        
        // potential hangs here
        if (Z_undef.size() > 0 && (Z_undef[i] || Z_undef[j])) {
            if (t_max < t_max_const) {
                continue;
            } else {
                // ERROR: too many undefined values, computing hangs and exits.
                return false;
            }
            t_max += 1;
        }
        
        t += 1;
        
		const wxRealPoint& p1(pts[i]);
		const wxRealPoint& p2(pts[j]);
		double d;
        
        // NOTE: performance boost here: cache the computed distance
        // the cache could have a limited size e.g. 10000
		if (is_arc) {
			d = ComputeArcDistRad(p1.x, p1.y, p2.x, p2.y);
		} else {
			d = ComputeEucDist(p1.x, p1.y, p2.x, p2.y);
		}
		int b = (int) (d/binw);
		if (b >= num_bins || b<0) {
			ta_cnt++;
			continue;
		}
		out[b].num_pairs++;
		if (calc_prods && var != 0) {
			double prod = (Z[i]-mean)*(Z[j]-mean)/var;
			out[b].corr_avg += prod;
		}
	}
	for (size_t b=0; b<num_bins; ++b) {
		if (out[b].num_pairs >0 && calc_prods) {
			out[b].corr_avg /= ((double) out[b].num_pairs);
		}
	}

	LOG_MSG("Exiting CorrelogramAlgs::MakeCorrRandSamp");
	return true;
}

bool CorrelogramAlgs::MakeCorrAllPairs(const std::vector<wxRealPoint>& pts,
									   const std::vector<double>& Z,
                                       const std::vector<bool>& Z_undef,
									   bool is_arc, int num_bins,
									   std::vector<CorreloBin>& out)

{
	using namespace std;
	using namespace GenGeomAlgs;
	LOG_MSG("Entering CorrelogramAlgs::MakeCorrAllPairs");
	wxStopWatch sw;

	size_t nobs = pts.size();

	bool calc_prods = (Z.size() == nobs);
	double mean = 0;
	double var = 0;
    
	if (calc_prods) {
		GetSampMeanAndVar(Z, Z_undef, mean, var);
		if (var <= 0) {
			return false;
		}
	}

	double min_d, max_d;
	min_d = (is_arc ?
             ComputeArcDistRad(pts[0].x, pts[0].y, pts[1].x, pts[1].y) :
			 ComputeEucDist(pts[0].x, pts[0].y, pts[1].x, pts[1].y));
	max_d = min_d;

	size_t pairs = ((nobs-1)*nobs)/2;
	vector<double> Zdist(pairs);
	vector<double> Zprod(calc_prods ? pairs : 0);
	vector<double> Zprod_undef(calc_prods ? pairs : 0);
	size_t pc = 0;
	for (size_t i=0; i<nobs; ++i) {
		for (size_t j=i+1; j<nobs; ++j) {
            
            if (Z_undef.size() > 0 && (Z_undef[i] || Z_undef[j])) {
                Zprod_undef[pc] = true;
                Zdist[pc] = 0;
                Zprod[pc] = 0;
                continue;
            }
            
            Zprod_undef[pc] = false;
			double d = (is_arc ?
                        ComputeArcDistRad(pts[i].x, pts[i].y,pts[j].x, pts[j].y) :
						ComputeEucDist(pts[i].x, pts[i].y, pts[j].x, pts[j].y));
			if (d < min_d) {
				min_d = d;
			} else if (d > max_d) {
				max_d = d;
			}
			Zdist[pc] = d;
            if (calc_prods && var != 0) {
                Zprod[pc] = (Z[i]-mean)*(Z[j]-mean)/var;
            }
			++pc;
		}
	}
	
    if (num_bins <= 0) {
        num_bins = 1;
    }
	out.clear();
	out.resize(num_bins);
	double nbins_d = (double) num_bins;
	double binw = max_d/nbins_d; // bin width
	out[0].dist_min = 0;
	out[0].dist_max = binw;
	for (int i=1; i<num_bins; ++i) {
		out[i].dist_min = binw*((double) i);
		out[i].dist_max = binw*((double) (i+1));
		out[i].corr_avg_valid = calc_prods;
	}

	size_t ta_cnt = 0;
	for (size_t i=0, sz=Zdist.size(); i<sz; ++i) {
        if (wxIsNaN(Zdist[i]) || Zprod_undef[i]) {
            // todo something wrong here, Zdist value is NaN
            continue;
        }
        
		int b = (int) (Zdist[i]/binw);
		if (b >= num_bins) {
			b=num_bins-1;
			ta_cnt++;
		}
		out[b].num_pairs++;
        if (calc_prods) {
            out[b].corr_avg += Zprod[i];
        }
	}
	LOG(ta_cnt); // should be at most 1
	
	for (size_t b=0; b<num_bins; ++b) {
		if (out[b].num_pairs != 0 && calc_prods) {
            out[b].corr_avg /= ((double) out[b].num_pairs);
        }
	}
    
	/*
		stringstream ss;
		ss << "MakeCorrMakeCorrAllPairs with " << pairs
		   << " pairs finished in " << sw.Time() << " ms.";
		LOG_MSG(ss.str());
	*/
	LOG_MSG("Exiting CorrelogramAlgs::MakeCorrAllPairs");
	return true;
}

bool CorrelogramAlgs::MakeCorrThresh(const rtree_pt_2d_t& rtree,
									 const std::vector<double>& Z,
                                     const std::vector<bool>& Z_undef,
									 double thresh, int num_bins,
									 std::vector<CorreloBin>& out)
{
	using namespace std;
	using namespace GenGeomAlgs;
	using namespace SpatialIndAlgs;
	LOG_MSG("Entering CorrelogramAlgs::MakeCorrThresh (plane)");
	wxStopWatch sw;

	if (thresh <= 0) return false;
	bool calc_prods = (Z.size() == rtree.size());
	double nbins_d = (double) num_bins;
    
	if (num_bins <= 0) num_bins = 1;
	out.clear();
	out.resize(num_bins);
	double binw = thresh/nbins_d; // bin width
	out[0].dist_min = 0;
	out[0].dist_max = binw;
	for (int i=1; i<num_bins; ++i) {
		out[i].dist_min = binw*((double) i);
		out[i].dist_max = binw*((double) (i+1));
		out[i].corr_avg_valid = calc_prods;
	}

	double mean= 0;
	double var=0;
	if (calc_prods) {
		GetSampMeanAndVar(Z, Z_undef, mean, var);
		if (var <= 0) {
			//LOG_MSG("Error: non-positive variance calculated");
			return false;
		}
	}
	
	int ta_cnt = 0; // throw away count
	int pairs_cnt = 0; // pairs count
	for (rtree_pt_2d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_2d_val& v = *it;
		const size_t i = v.second;
		double x = v.first.get<0>();
		double y = v.first.get<1>();
		box_2d b(pt_2d(x-thresh, y-thresh), pt_2d(x+thresh, y+thresh));
		vector<pt_2d_val> q;
		rtree.query(bgi::intersects(b), std::back_inserter(q));
		BOOST_FOREACH(const pt_2d_val& w, q) {
			const size_t j = w.second;
			if (i >= j) continue;
            if (Z_undef.size() > 0 && (Z_undef[i] || Z_undef[j])) 
                continue;
            
			double d = bg::distance(v.first, w.first);
			int b = (int) (d/binw);
			if (b >= num_bins || b<0) {
				++ta_cnt;
				continue;
			}
            
			++pairs_cnt;
			out[b].num_pairs++;
			if (calc_prods && var !=0) {
                double prod = (Z[i]-mean)*(Z[j]-mean)/var;
                out[b].corr_avg += prod;
			}
		}
	}
	
	for (size_t b=0; b<num_bins; ++b) {
		if (out[b].num_pairs != 0 && calc_prods) {
            out[b].corr_avg /= ((double) out[b].num_pairs);
		}
	}

	/*
		stringstream ss;
		ss << "MakeCorrThresh with threshold " << thresh
		   << " finished in " << sw.Time() << " ms.";
		LOG_MSG(ss.str());
	*/
	LOG_MSG("Exiting CorrelogramAlgs::MakeCorrThresh (plane)");
	return true;
}

/** thresh is assumed to be in radians.  Output is also in radians. */
bool CorrelogramAlgs::MakeCorrThresh(const rtree_pt_3d_t& rtree,
                                     const std::vector<double>& Z,
                                     const std::vector<bool>& Z_undef,
                                     double thresh, int num_bins,
                                     std::vector<CorreloBin>& out)
{
	using namespace std;
	using namespace GenGeomAlgs;
	using namespace SpatialIndAlgs;
	LOG_MSG("Entering CorrelogramAlgs::MakeCorrThresh (sphere)");
	wxStopWatch sw;
	
	if (thresh <= 0) return false;
	bool calc_prods = (Z.size() == rtree.size());
	double nbins_d = (double) num_bins;
	if (num_bins <= 0) num_bins = 1;
	out.clear();
	out.resize(num_bins);
	double binw = thresh/nbins_d; // bin width
	out[0].dist_min = 0;
	out[0].dist_max = binw;
	for (int i=1; i<num_bins; ++i) {
		out[i].dist_min = binw*((double) i);
		out[i].dist_max = binw*((double) (i+1));
		out[i].corr_avg_valid = calc_prods;
	}
	
	double mean= 0;
	double var=0;
	if (calc_prods) {
		GetSampMeanAndVar(Z, Z_undef, mean, var);
		if (var <= 0) {
			//LOG_MSG("Error: non-positive variance calculated");
			return false;
		}
	}
	
	// thresh is in radians.  Need to convert to unit sphere secant distance
	double sec_thresh = RadToUnitDist(thresh);
	
	int ta_cnt = 0; // throw away count
	int pairs_cnt = 0; // pairs count
	for (rtree_pt_3d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
			 it != rtree.qend() ; ++it)
	{
		const pt_3d_val& v = *it;
		const size_t i = v.second;
		double x = v.first.get<0>();
		double y = v.first.get<1>();
		double z = v.first.get<2>();
		box_3d b(pt_3d(x-sec_thresh, y-sec_thresh, z-sec_thresh),
						 pt_3d(x+sec_thresh, y+sec_thresh, z+sec_thresh));
		vector<pt_3d_val> q;
		rtree.query(bgi::intersects(b), std::back_inserter(q));
		BOOST_FOREACH(const pt_3d_val& w, q) {
			const size_t j = w.second;
			if (i >= j) continue;
            if (Z_undef.size() > 0 && (Z_undef[i] || Z_undef[j]))
                continue;
			double d = bg::distance(v.first, w.first);
			double rad_d = UnitDistToRad(d);
			
			int b = (int) (rad_d/binw);
			if (b >= num_bins || b<0) {
				++ta_cnt;
				continue;
			}
			++pairs_cnt;
			out[b].num_pairs++;
			if (calc_prods && var != 0) {
				double prod = (Z[i]-mean)*(Z[j]-mean)/var;
				out[b].corr_avg += prod;
			}
		}
	}
	
	for (size_t b=0; b<num_bins; ++b) {
		if (out[b].num_pairs != 0 && calc_prods) {
			out[b].corr_avg /= ((double) out[b].num_pairs);
		}
	}
	
	/*
		stringstream ss;
		ss << "MakeCorrThresh with threshold " << thresh
		<< " finished in " << sw.Time() << " ms.";
		LOG_MSG(ss.str());
	*/
	LOG_MSG("Exiting CorrelogramAlgs::MakeCorrThresh (sphere)");
	return true;
}
