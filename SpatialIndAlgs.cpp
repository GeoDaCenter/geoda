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
#include <wx/wx.h>

#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/stopwatch.h>

#include "PointSetAlgs.h"
#include "GenGeomAlgs.h"
#include "SpatialIndAlgs.h"
#include "VarCalc/NumericTests.h"
#include "GdaException.h"
#include "logger.h"

using namespace std;



void SpatialIndAlgs::to_3d_centroids(const vector<pt_2d>& pt2d,
                                     vector<pt_3d>& pt3d)
{
	size_t obs = pt2d.size();
	pt3d.resize(obs);
	for (size_t i=0; i<obs; ++i) {
		pt3d[i] = pt_3d(bg::get<0>(pt2d[i]), bg::get<1>(pt2d[i]), 0);
	}
}

void SpatialIndAlgs::to_3d_centroids(const vector<pt_lonlat>& ptll,
                                     vector<pt_3d>& pt3d)
{
	size_t obs = ptll.size();
	pt3d.resize(obs);
	for (size_t i=0; i<obs; ++i) {
		double x, y, z;
		GenGeomAlgs::LongLatDegToUnit(bg::get<0>(ptll[i]),
                                      bg::get<1>(ptll[i]),
									  x, y, z);
		pt3d[i] = pt_3d(x, y, z);
	}
}




void SpatialIndAlgs::default_test()
{
    // create the rtree using default constructor
    rtree_box_2d_t rtree;

    // create some values
    for ( unsigned i = 0 ; i < 10 ; ++i ) {
        // create a box
        box_2d b(pt_2d(i + 0.0f, i + 0.0f), pt_2d(i + 0.5f, i + 0.5f));
        // insert new value
        rtree.insert(std::make_pair(b, i));
    }

    // find values intersecting some area defined by a box
    box_2d query_box(pt_2d(0, 0), pt_2d(5, 5));
    std::vector<box_2d_val> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

	const int k=3;
    // find k nearest values to a point
    std::vector<box_2d_val> result_n;
    rtree.query(bgi::nearest(pt_2d(0, 0), k), std::back_inserter(result_n));

    // note: in Boost.Geometry WKT representation of a box is polygon

    // display results
	stringstream ss;
    ss << "spatial query box:" << std::endl;
    ss << bg::wkt<box_2d>(query_box) << std::endl;
    ss << "spatial query result:" << std::endl;
    BOOST_FOREACH(box_2d_val const& v, result_s) {
        ss << bg::wkt<box_2d>(v.first) << " - " << v.second << std::endl;
	}

    ss << k << "-nn query point:" << std::endl;
    ss << bg::wkt<pt_2d>(pt_2d(0, 0)) << std::endl;
    ss << k << "-nn query result:" << std::endl;
    BOOST_FOREACH(box_2d_val const& v, result_n) {
        ss << bg::wkt<box_2d>(v.first) << " - " << v.second << std::endl;
	}

	pt_lonlat sp(0, 45);
	ss << "Spherical pt get<0>: " << bg::get<0>(sp) << std::endl;
	ss << "Spherical pt get<1>: " << bg::get<1>(sp) << std::endl;
	ss << "Spherical pt: " << bg::wkt<pt_lonlat>(sp) << std::endl;
	
	ss << "default_test() END";
}

void SpatialIndAlgs::print_rtree_stats(rtree_box_2d_t& rtree)
{
	stringstream ss;
	ss << "Rtree stats:" << endl;
	ss << "  size: " << rtree.size() << endl;
	ss << "  empty?: " << rtree.empty() << endl;
	box_2d bnds = rtree.bounds();
	ss << "  bounds: " << bg::wkt<box_2d>(bnds);
}

void SpatialIndAlgs::query_all_boxes(rtree_box_2d_t& rtree)
{
	int dzero = 0;
	int dpos = 0;
	int cnt=0;
	box_2d bnds = rtree.bounds();
	
	for (rtree_box_2d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it) { ++cnt; }

	cnt = 0;
	
    rtree_box_2d_t::const_query_iterator it;
	for (it=rtree.qbegin(bgi::intersects(rtree.bounds())); it != rtree.qend(); ++it)
	{
		const box_2d_val& v = *it;
		pt_2d c;
		boost::geometry::centroid(v.first, c);
		vector<box_2d_val> q;
		rtree.query(bgi::intersects(v.first), std::back_inserter(q));
		int qcnt=0;
		BOOST_FOREACH(box_2d_val const& w, q) {
			if (w.second == v.second)
                continue;
			++cnt;
			++qcnt;
		}
	}
}

void SpatialIndAlgs::knn_query(const rtree_pt_2d_t& rtree, int nn)
{
	int cnt=0;
	box_2d bnds = rtree.bounds();

    rtree_pt_2d_t::const_query_iterator it;
	for (it= rtree.qbegin(bgi::intersects(rtree.bounds())); it != rtree.qend(); ++it)
    {
        ++cnt;
    }

	cnt = 0;

	const int k=nn+1;
	for (it= rtree.qbegin(bgi::intersects(rtree.bounds())); it != rtree.qend(); ++it)
	{
		const pt_2d_val& v = *it;
		vector<pt_2d_val> q;
		rtree.query(bgi::nearest(v.first, k), std::back_inserter(q));
		BOOST_FOREACH(pt_2d_val const& w, q) {
            if (w.second == v.second) {
                continue;
            }
			++cnt;
		}
	}
}

GwtWeight* SpatialIndAlgs::knn_build(const vector<double>& x,
                                     const vector<double>& y,
                                     int nn,
                                     bool is_arc, bool is_mi,
                                     bool is_inverse, double power,
                                     const wxString& kernel,
                                     double bandwidth,
                                     bool adaptive_bandwidth,
                                     bool use_kernel_diagnals)
{
	size_t nobs = x.size();
	GwtWeight* gwt = 0;
	if (is_arc) {
		rtree_pt_3d_t rtree;
		{
			vector<pt_3d> pts;
			{
				vector<pt_lonlat> ptll(nobs);
				for (int i=0; i<nobs; ++i) ptll[i] = pt_lonlat(x[i], y[i]);
				to_3d_centroids(ptll, pts);
			}
			fill_pt_rtree(rtree, pts);
		}
		gwt = knn_build(rtree, nn, true, is_mi, is_inverse, power, kernel, bandwidth, adaptive_bandwidth, use_kernel_diagnals);
        
	} else {
		rtree_pt_2d_t rtree;
		{
			vector<pt_2d> pts(nobs);
			for (int i=0; i<nobs; ++i) pts[i] = pt_2d(x[i], y[i]);
			fill_pt_rtree(rtree, pts);
		}
		gwt = knn_build(rtree, nn, is_inverse, power, kernel, bandwidth, adaptive_bandwidth, use_kernel_diagnals);
        
	}
	return gwt;
}

void SpatialIndAlgs::apply_kernel(const GwtWeight* Wp, const wxString& kernel, bool use_kernel_diagnals)
{
    // apply kernel
    double gaussian_const = pow(M_PI * 2.0, -0.5);
    
    for (int i=0; i<Wp->num_obs; i++) {
        GwtElement& e = Wp->gwt[i];
        GwtNeighbor* nbrs = e.dt();
        for (int j=0; j<e.Size(); j++) {
            if (!use_kernel_diagnals && i==nbrs[j].nbx) {
                nbrs[j].weight = 1.0;
                continue;
            }
            // functions follow Anselin and Rey (2010) table 5.4
            if (kernel.IsSameAs("triangular",false)) {
                nbrs[j].weight = 1 - nbrs[j].weight;
            } else if (kernel.IsSameAs("uniform", false)) {
                nbrs[j].weight = 0.5;
            } else if (kernel.IsSameAs("epanechnikov", false)) {
                nbrs[j].weight = (3.0 / 4.0) * (1.0 - pow(nbrs[j].weight,2.0));
            } else if (kernel.IsSameAs("quartic", false)) {
                nbrs[j].weight = (15.0 / 16.0) * pow((1.0 - pow(nbrs[j].weight,2.0)), 2.0);
            } else if (kernel.IsSameAs("gaussian", false)) {
                nbrs[j].weight = gaussian_const * exp( -pow(nbrs[j].weight, 2.0) / 2.0 );
            }
        }
    }
}

GwtWeight* SpatialIndAlgs::knn_build(const rtree_pt_2d_t& rtree, int nn, bool is_inverse, double power, const wxString& kernel, double bandwidth_, bool adaptive_bandwidth_, bool use_kernel_diagnals)
{
	GwtWeight* Wp = new GwtWeight;
	Wp->num_obs = rtree.size();
	Wp->is_symmetric = false;
	Wp->symmetry_checked = true;
	Wp->gwt = new GwtElement[Wp->num_obs];
	
	int cnt=0;
	const int k=nn+1;
    double bandwidth = bandwidth_;
    bool adaptive_bandwidth = adaptive_bandwidth_;

	for (rtree_pt_2d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_2d_val& v = *it;
		size_t obs = v.second;
        // each point "v" with index "obs"
		vector<pt_2d_val> q;
		rtree.query(bgi::nearest(v.first, k), std::back_inserter(q));
		GwtElement& e = Wp->gwt[obs];
		e.alloc(q.size());
        double local_bandwidth = 0;
		BOOST_FOREACH(pt_2d_val const& w, q) {
			if ( (kernel.IsEmpty() && w.second == v.second) ||
                 (!kernel.IsEmpty() && !use_kernel_diagnals && w.second == v.second) )
                continue;
			GwtNeighbor neigh;
			neigh.nbx = w.second;
            double d = bg::distance(v.first, w.first);
            if (bandwidth_ ==0 && d > bandwidth) bandwidth = d;
            if (d > local_bandwidth) local_bandwidth = d;
            if (is_inverse) d = pow(d, power);
            neigh.weight =  d;
			e.Push(neigh);
			++cnt;
		}
        if (adaptive_bandwidth && local_bandwidth > 0 && !kernel.IsEmpty()) {
            GwtNeighbor* nbrs = e.dt();
            for (int j=0; j<e.Size(); j++) {
                nbrs[j].weight = nbrs[j].weight / local_bandwidth;
            }
        }
	}

    if (!adaptive_bandwidth && bandwidth > 0 && !kernel.IsEmpty()) {
        // use max knn distance as bandwidth
        for (int i=0; i<Wp->num_obs; i++) {
            GwtElement& e = Wp->gwt[i];
            GwtNeighbor* nbrs = e.dt();
            for (int j=0; j<e.Size(); j++) {
                nbrs[j].weight = nbrs[j].weight / bandwidth;
            }
        }
    }
    if (!kernel.IsEmpty()) {
        
        apply_kernel(Wp, kernel, use_kernel_diagnals);
    }
    
	return Wp;
}

GwtWeight* SpatialIndAlgs::knn_build(const rtree_pt_3d_t& rtree, int nn,
					 bool is_arc, bool is_mi,  bool is_inverse, double power, const wxString& kernel, double bandwidth_, bool adaptive_bandwidth_, bool use_kernel_diagnals)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;

	GwtWeight* Wp = new GwtWeight;
	Wp->num_obs = rtree.size();
	Wp->is_symmetric = false;
	Wp->symmetry_checked = true;
	Wp->gwt = new GwtElement[Wp->num_obs];
	
	int cnt=0;
	const int k=nn+1;
    double bandwidth = bandwidth_;
    bool adaptive_bandwidth = adaptive_bandwidth_;
    // if not set,  use max knn distance as bandwidth
    
	for (rtree_pt_3d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_3d_val& v = *it;
		size_t obs = v.second;		
		vector<pt_3d_val> q;
		rtree.query(bgi::nearest(v.first, k), std::back_inserter(q));
		GwtElement& e = Wp->gwt[obs];
		e.alloc(q.size());
		double lon_v, lat_v;
		double x_v, y_v;
		if (is_arc) {
			UnitToLongLatDeg(bg::get<0>(v.first), bg::get<1>(v.first),
							 bg::get<2>(v.first), lon_v, lat_v);
		} else {
			x_v = bg::get<0>(v.first);
			y_v = bg::get<1>(v.first);
		}
        double local_bandwidth = 0;
		BOOST_FOREACH(pt_3d_val const& w, q) {
			if (kernel.IsEmpty() && w.second == v.second)
                continue;
			GwtNeighbor neigh;
			neigh.nbx = w.second;
			if (is_arc) {
				double lon_w, lat_w;
				UnitToLongLatDeg(bg::get<0>(w.first), bg::get<1>(w.first),
								 bg::get<2>(w.first), lon_w, lat_w);
				if (is_mi) {
					neigh.weight = ComputeArcDistMi(lon_v, lat_v, lon_w, lat_w);
				} else {
					neigh.weight = ComputeArcDistKm(lon_v, lat_v, lon_w, lat_w);
				}
			} else {
				//neigh.weight = bg::distance(v.first, w.first);
				neigh.weight = ComputeEucDist(x_v, y_v,
											  bg::get<0>(w.first),
                                              bg::get<1>(w.first));
			}
            if (is_inverse) neigh.weight = pow(neigh.weight, power);
            
            if (bandwidth_ == 0 && neigh.weight > bandwidth)
                bandwidth = neigh.weight;
            if (neigh.weight > local_bandwidth)
                local_bandwidth = neigh.weight;
           
			e.Push(neigh);
			++cnt;
		}
        if (adaptive_bandwidth && local_bandwidth > 0 && !kernel.IsEmpty()) {
            GwtNeighbor* nbrs = e.dt();
            for (int j=0; j<e.Size(); j++) {
                nbrs[j].weight = nbrs[j].weight / local_bandwidth;
            }
        }
	}

    if (!adaptive_bandwidth && bandwidth > 0 && !kernel.IsEmpty()) {
        // use max knn distance as bandwidth
        for (int i=0; i<Wp->num_obs; i++) {
            GwtElement& e = Wp->gwt[i];
            GwtNeighbor* nbrs = e.dt();
            for (int j=0; j<e.Size(); j++) {
                nbrs[j].weight = nbrs[j].weight / bandwidth;
            }
        }
    }
    if (!kernel.IsEmpty()) {
        apply_kernel(Wp, kernel, use_kernel_diagnals);
    }
    
	stringstream ss;
	ss << "Time to create 3D " << (is_arc ? " arc " : "")
	   << nn << "-NN GwtWeight "
	   << "with " << cnt << " total neighbors in ms : " << sw.Time();
	return Wp;
}


double SpatialIndAlgs::est_thresh_for_num_pairs(const rtree_pt_2d_t& rtree,
												double num_pairs)
{
	double nobs_d = (double) rtree.size();
	if (num_pairs >= (nobs_d*(nobs_d-1.0))/2.0) {
		return bg::distance(rtree.bounds().min_corner(), rtree.bounds().max_corner());
	}
	// Need roughly double since pairs are visited twice.
	double avg_n = (num_pairs / nobs_d)*2.0;
	// To avoid the use of a hash table, we just allow pairs
	// to be counted twice each bin is just an average. Although
	// distances will be calculated twice, the cost should be offset
	// by the faster performance of no hash table inserts / lookups.
	double thresh = est_thresh_for_avg_num_neigh(rtree, avg_n);
	return thresh;
}

double SpatialIndAlgs::est_thresh_for_avg_num_neigh(const rtree_pt_2d_t& rtree,
													double avg_n)
{
	// Use a binary search to estimate threshold to acheive average num neighbors
	wxStopWatch sw;
	using namespace GenGeomAlgs;
	int max_iters = 20;
	int iters = 0;
	double lower = 0;
	double lower_avg = 0;
	box_2d bnds(rtree.bounds());
	double upper = bg::distance(bnds.min_corner(), bnds.max_corner());
	double upper_avg = (double) rtree.size();
	double guess = upper;
	double guess_avg = upper_avg;
	double th = guess;
	
	bool was_improvement = true;
	for (iters=0; iters<max_iters && was_improvement; ++iters) {
		guess = lower + (upper-lower)/2.0;
		guess_avg = est_avg_num_neigh_thresh(rtree, guess);
		{
			stringstream ss;
			ss << "\niter: " << iters << "   target avg: " << avg_n << endl;
			ss << "  lower: " << lower << ", lower_avg: " << lower_avg << endl;
			ss << "  guess: " << guess << ", guess_avg: " << guess_avg << endl;
			ss << "  upper: " << upper << ", upper_avg: " << upper_avg;
		}
		if (guess_avg == avg_n) {
			//LOG_MSG("new guess was exact!");
			// this will never happen, but put case here for completeness
			th = guess;
			was_improvement = false;
		} else if (guess_avg <= lower_avg) {
			//LOG_MSG("new guess below lower bound");
			was_improvement = false;
		} else if (guess_avg >= upper_avg) {
			//LOG_MSG("new guess above lower bound");
			was_improvement = false;
		} else if (guess_avg < avg_n) {
			//LOG_MSG("increase lower bound");
			lower = guess;
			lower_avg = guess_avg;
		} else { // guess_avg > avg_n
			//LOG_MSG("decrease upper bound");
			upper = guess;
			upper_avg = guess_avg;
		}
		if (was_improvement) {
			th = guess;
		}
	}

	stringstream ss;
	ss << "Estimated " << th << " threshold for average "
	   << "number neighbors " << avg_n << "." << endl;
	ss << "Calculation time to peform " << iters << " iterations: "
	   << sw.Time() << " ms.";
	LOG_MSG("Exiting est_thresh_for_avg_num_neigh");
	return th;
}

double SpatialIndAlgs::est_avg_num_neigh_thresh(const rtree_pt_2d_t& rtree,
																								double th, size_t trials)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;

	vector<pt_2d_val> query_pts;
	rtree.query(bgi::intersects(rtree.bounds()), back_inserter(query_pts));
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::random::uniform_int_distribution<> X(0, query_pts.size()-1);
	size_t tot_neigh = 0;
	for (size_t i=0; i<trials; ++i) {
		const pt_2d_val& v = query_pts[X(rng)];
		double x = v.first.get<0>();
		double y = v.first.get<1>();
		box_2d b(pt_2d(x-th, y-th), pt_2d(x+th, y+th));
		vector<pt_2d_val> q;
		rtree.query(bgi::intersects(b), std::back_inserter(q));
		BOOST_FOREACH(const pt_2d_val& w, q) {
			if (w.second != v.second && bg::distance(v.first, w.first) <= th)
			{
				++tot_neigh;
			}
		}
	}

	double avg = ((double) tot_neigh) / ((double) trials);

	stringstream ss;
	ss << "Estimated " << avg << " neighbors on average for "
	   << "threshold " << th << "." << endl;
	ss << "Time to perform " << trials << " random trials: "
	   << sw.Time() << " ms.";
	//LOG_MSG(ss.str());
	return avg;
}

double SpatialIndAlgs::est_mean_distance(const std::vector<double>& x,
										 const std::vector<double>& y,
										 bool is_arc, size_t max_iters)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;
	const size_t pts_sz = x.size();
	const size_t all_pairs_sz = (pts_sz*(pts_sz-1))/2;
	if (x.size() != y.size() || x.size() == 0 || y.size() == 0) { return -1; }
	double sum = 0;
	double smp_cnt = 0;
	
	if (all_pairs_sz <= max_iters) {
		for (size_t i=0; i<pts_sz; ++i) {
			for (size_t j=i+1; j<pts_sz; ++j) {
				sum += (is_arc ? ComputeArcDistRad(x[i], y[i], x[j], y[j]) :
						ComputeEucDist(x[i], y[i], x[j], y[j]));
			}
		}
		smp_cnt = (double) all_pairs_sz;
	} else {
		// Mersenne Twister random number generator, randomly seeded
		// with current time in seconds since Jan 1 1970.
		static boost::mt19937 rng(std::time(0));
		static boost::random::uniform_int_distribution<> X(0, pts_sz-1);
		for (size_t t=0; t<max_iters; ++t) {
			size_t i=X(rng);
			size_t j=X(rng);
			sum += (is_arc ? ComputeArcDistRad(x[i], y[i], x[j], y[j]) :
					ComputeEucDist(x[i], y[i], x[j], y[j]));
		}
		smp_cnt = max_iters;
	}
	stringstream ss;
	ss << "est_mean_distance finished in " << sw.Time() << " ms.";
	return sum/smp_cnt;
}

double SpatialIndAlgs::est_median_distance(const std::vector<double>& x,
										   const std::vector<double>& y,
										   bool is_arc, size_t max_iters)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;
	if (x.size() != y.size() || x.size() == 0 || y.size() == 0) { return -1; }
	const size_t pts_sz = x.size();
	const size_t all_pairs_sz = (pts_sz*(pts_sz-1))/2;
	vector<double> v;

	if (all_pairs_sz <= max_iters) {
		v.resize((pts_sz*(pts_sz-1))/2);
		size_t t=0;
		for (size_t i=0; i<pts_sz; ++i) {
			for (size_t j=i+1; j<pts_sz; ++j) {
				v[t] = (is_arc ? ComputeArcDistRad(x[i], y[i], x[j], y[j]) :
						ComputeEucDist(x[i], y[i], x[j], y[j]));
				++t;
			}
		}
	} else {
		v.resize(max_iters);
		// Mersenne Twister random number generator, randomly seeded
		// with current time in seconds since Jan 1 1970.
		static boost::mt19937 rng(std::time(0));
		static boost::random::uniform_int_distribution<> X(0, pts_sz-1);
		size_t cnt=0;
		for (size_t t=0; t<max_iters; ++t) {
			size_t i=X(rng);
			size_t j=X(rng);
			//if (i==j) continue;
			v[t] = (is_arc ? ComputeArcDistRad(x[i], y[i], x[j], y[j]) :
					ComputeEucDist(x[i], y[i], x[j], y[j]));
			if (!Gda::is_finite(v[t]) || Gda::is_nan(v[t])) {
				stringstream ss;
				ss << "d(i="<<i<<",j="<<j<<"): "<<v[t];
			}
			
		}
	}
	sort(v.begin(), v.end());
	stringstream ss;
	ss << "est_median_distance finished in " << sw.Time() << " ms.";
	return v[v.size()/2];
}

GwtWeight* SpatialIndAlgs::thresh_build(const std::vector<double>& x,
                                        const std::vector<double>& y,
                                        double th, double power,
                                        bool is_arc, bool is_mi)
{
	using namespace GenGeomAlgs;
	size_t nobs = x.size();
	GwtWeight* gwt = 0;
	if (is_arc) {
		double r_th = is_mi ? EarthMiToRad(th) : EarthKmToRad(th);
		double u_th = RadToUnitDist(r_th);
		rtree_pt_3d_t rtree;
		{
			vector<pt_3d> pts;
			{
				vector<pt_lonlat> ptll(nobs);
				for (int i=0; i<nobs; ++i) ptll[i] = pt_lonlat(x[i], y[i]);
				to_3d_centroids(ptll, pts);
			}
			fill_pt_rtree(rtree, pts);
		}
		gwt = thresh_build(rtree, u_th, power, is_mi);
	} else {
		rtree_pt_2d_t rtree;
		{
			vector<pt_2d> pts(nobs);
            for (int i=0; i<nobs; ++i) {
                pts[i] = pt_2d(x[i], y[i]);
            }
			fill_pt_rtree(rtree, pts);
		}
		gwt = thresh_build(rtree, th, power);
	}
	return gwt;
}

GwtWeight* SpatialIndAlgs::thresh_build(const rtree_pt_2d_t& rtree, double th, double power)
{
	wxStopWatch sw;
    
	GwtWeight* Wp = new GwtWeight;
	Wp->num_obs = rtree.size();
	Wp->is_symmetric = false;
	Wp->symmetry_checked = true;
    
    int num_obs = Wp->num_obs;
	Wp->gwt = new GwtElement[num_obs];
	
	int cnt=0;
	bool ignore_too_large_compute = false;
    rtree_pt_2d_t::const_query_iterator it;
	for (it = rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_2d_val& v = *it;
		double x = v.first.get<0>();
		double y = v.first.get<1>();
		box_2d b(pt_2d(x-th, y-th), pt_2d(x+th, y+th));
		size_t obs = v.second;		
		vector<pt_2d_val> q;
		rtree.query(bgi::intersects(b), std::back_inserter(q));
		size_t lcnt = 0;
		list<pt_2d_val> l;
		BOOST_FOREACH(pt_2d_val const& w, q) {
			if (w.second != v.second &&
				bg::distance(v.first, w.first) <= th)
			{
				l.push_front(w);
				++lcnt;
			}
		}
        if (lcnt > 200 && ignore_too_large_compute == false) {
            
            wxString msg = _("You can try to proceed but the current threshold distance value might be too large to compute. If it fails, please input a smaller distance band (which might leave some observations neighborless) or use other weights (e.g. KNN).");
			wxMessageDialog dlg(NULL, msg, "Do you want to continue?", wxYES_NO | wxYES_DEFAULT);
			if (dlg.ShowModal() != wxID_YES) {
				// clean up memory
				delete Wp;
				throw GdaException(msg.mb_str());
			}
			else {
				ignore_too_large_compute = true;
			}
            
        }
		GwtElement& e = Wp->gwt[obs];
		e.alloc(lcnt);
		BOOST_FOREACH(pt_2d_val const& w, l) {
			GwtNeighbor neigh;
			neigh.nbx = w.second;
			double w_val = bg::distance(v.first, w.first);
            if (power != 1) w_val = pow(w_val, power);
            neigh.weight = w_val;
			e.Push(neigh);
			++cnt;
		}
	}

	stringstream ss;
	ss << "Time to create " << th << " threshold GwtWeight,"
	   << endl << "  with " << cnt << " total neighbors in ms : "
	   << sw.Time();
	return Wp;
}

double SpatialIndAlgs::est_avg_num_neigh_thresh(const rtree_pt_3d_t& rtree,
					 double th,	size_t trials)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;

	vector<pt_3d_val> query_pts;
	rtree.query(bgi::intersects(rtree.bounds()), back_inserter(query_pts));
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::random::uniform_int_distribution<> X(0, query_pts.size()-1);
	size_t tot_neigh = 0;
	for (size_t i=0; i<trials; ++i) {
		const pt_3d_val& v = query_pts[X(rng)];
		double x = v.first.get<0>();
		double y = v.first.get<1>();
		double z = v.first.get<2>();
		box_3d b(pt_3d(x-th, y-th, z-th), pt_3d(x+th, y+th, z+th));
		vector<pt_3d_val> q;
		rtree.query(bgi::intersects(b), std::back_inserter(q));
		BOOST_FOREACH(const pt_3d_val& w, q) {
			if (w.second != v.second && bg::distance(v.first, w.first) <= th)
			{
				++tot_neigh;
			}
		}
	}
	double avg = ((double) tot_neigh) / ((double) trials);
	stringstream ss;
	ss << "Estimated " << avg << " neighbors on average for "
	   << "threshold " << th << "." << endl;
	ss << "Time to perform " << trials << " random trials: "
	   << sw.Time() << " ms.";
	return avg;
}

/** threshold th is the radius of intersection sphere with
  respect to the unit shpere of the 3d point rtree */
GwtWeight* SpatialIndAlgs::thresh_build(const rtree_pt_3d_t& rtree, double th, double power, bool is_mi)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;
	
	GwtWeight* Wp = new GwtWeight;
	Wp->num_obs = rtree.size();
	Wp->is_symmetric = false;
	Wp->symmetry_checked = true;
	Wp->gwt = new GwtElement[Wp->num_obs];
	
	{
		stringstream ss;
		ss << "In thresh_build for unit sphere" << endl;
		ss << "th : " << th << endl;
		ss << "Input th (unit sphere secant distance): " << th << endl;
		double r = UnitDistToRad(th);
		ss << "Input th (unit sphere rad): " << r << endl;
		ss << "Input th (earth km): " << EarthRadToKm(r) << endl;
		ss << "Input th (earth mi): " << EarthRadToMi(r);	
	}
	int cnt=0;
	for (rtree_pt_3d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_3d_val& v = *it;
		double vx = v.first.get<0>();
		double vy = v.first.get<1>();
		double vz = v.first.get<2>();
		double lon_v, lat_v;
		UnitToLongLatDeg(vx, vy, vz, lon_v, lat_v);
		box_3d b(pt_3d(vx-th, vy-th, vz-th), pt_3d(vx+th, vy+th, vz+th));
		size_t obs = v.second;
		vector<pt_3d_val> q;
		rtree.query(bgi::intersects(b), std::back_inserter(q));
		size_t lcnt = 0;
		list<pt_3d_val> l;
		BOOST_FOREACH(pt_3d_val const& w, q) {
			if (w.second != v.second &&
				bg::distance(v.first, w.first) <= th)
			{
				l.push_front(w);
				++lcnt;
			}
		}
		GwtElement& e = Wp->gwt[obs];
		e.alloc(lcnt);
		BOOST_FOREACH(pt_3d_val const& w, l) {
			GwtNeighbor neigh;
			neigh.nbx = w.second;
			double wx = w.first.get<0>();
			double wy = w.first.get<1>();
			double wz = w.first.get<2>();
			double lon_w, lat_w;
			double d;
			UnitToLongLatDeg(wx, wy, wz, lon_w, lat_w);
			if (is_mi) {
				d = ComputeArcDistMi(lon_v, lat_v, lon_w, lat_w);
			} else {
				d = ComputeArcDistKm(lon_v, lat_v, lon_w, lat_w);
			}
            if (power!=1) d = pow(d, power);
			neigh.weight = d;
			e.Push(neigh);
			++cnt;
		}
	}

	stringstream ss;
	ss << "Time to create arc " << th << " threshold GwtWeight,"
	   << endl << "  with " << cnt << " total neighbors in ms : "
	   << sw.Time();
	return Wp;
}

double SpatialIndAlgs::find_max_1nn_dist(const std::vector<double>& x,
                                         const std::vector<double>& y,
                                         bool is_arc, bool is_mi)
{
	using namespace GenGeomAlgs;
	size_t nobs = x.size();
	double min_d_1nn, max_d_1nn, mean_d_1nn, median_d_1nn, d;
	if (is_arc) {
		rtree_pt_3d_t rtree;
		{
			vector<pt_3d> pts;
			{
				vector<pt_lonlat> ptll(nobs);
				for (int i=0; i<nobs; ++i) ptll[i] = pt_lonlat(x[i], y[i]);
				to_3d_centroids(ptll, pts);
			}
			fill_pt_rtree(rtree, pts);
		}
		get_pt_rtree_stats(rtree, min_d_1nn, max_d_1nn, mean_d_1nn, median_d_1nn);
		d = is_mi ? EarthRadToMi(max_d_1nn) : EarthRadToKm(max_d_1nn);
	} else {
		rtree_pt_2d_t rtree;
		{
			vector<pt_2d> pts(nobs);
			for (int i=0; i<nobs; ++i) pts[i] = pt_2d(x[i], y[i]);
			fill_pt_rtree(rtree, pts);
		}
		get_pt_rtree_stats(rtree, min_d_1nn, max_d_1nn, mean_d_1nn, median_d_1nn);
		d = max_d_1nn;
	}
	return d;
}

void SpatialIndAlgs::get_pt_rtree_stats(const rtree_pt_2d_t& rtree,
                                        double& min_d_1nn, double& max_d_1nn,
                                        double& mean_d_1nn, double& median_d_1nn)
{
	wxStopWatch sw;
	const int k=2;
	size_t obs = rtree.size();
	vector<double> d(obs);
	for (rtree_pt_2d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_2d_val& v = *it;
		vector<pt_2d_val> q;
		rtree.query(bgi::nearest(v.first, k), std::back_inserter(q));
		BOOST_FOREACH(pt_2d_val const& w, q) {
			if (w.second == v.second) continue;
			d[v.second] = bg::distance(v.first, w.first);
		}
	}
	sort(d.begin(), d.end());
	min_d_1nn = d[0];
	max_d_1nn = d[d.size()-1];
	median_d_1nn = d[(d.size()-1)/2];
	double s=0;
	for (size_t i=0; i<obs; ++i) s += d[i];
	mean_d_1nn = s / (double) obs;

	stringstream ss;
	ss << "Euclidean points stats:" << endl;
	ss << "  min_d_1nn: " << min_d_1nn << endl;
	ss << "  max_d_1nn: " << max_d_1nn << endl;
	ss << "  median_d_1nn: " << median_d_1nn << endl;
	ss << "  mean_d_1nn: " << mean_d_1nn << endl;
	ss << "  running time in ms: " << sw.Time();
}

/** results returned in radians */
void SpatialIndAlgs::get_pt_rtree_stats(const rtree_pt_3d_t& rtree,
					 double& min_d_1nn, double& max_d_1nn,
					 double& mean_d_1nn, double& median_d_1nn)
{
	wxStopWatch sw;
	using namespace GenGeomAlgs;
	size_t obs = rtree.size();
	vector<double> d(obs);
	for (rtree_pt_3d_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_3d_val& v = *it;
		vector<pt_3d_val> q;
		rtree.query(bgi::nearest(v.first, 2), std::back_inserter(q));
		BOOST_FOREACH(pt_3d_val const& w, q) {
			if (w.second == v.second) continue;
			double lonv, latv, lonw, latw;
			UnitToLongLatRad(v.first.get<0>(), v.first.get<1>(),
							 v.first.get<2>(), lonv, latv);
			UnitToLongLatRad(w.first.get<0>(), w.first.get<1>(),
							 w.first.get<2>(), lonw, latw);
			d[v.second] = LonLatRadDistRad(lonv, latv, lonw, latw);
		}
	}
	sort(d.begin(), d.end());
	min_d_1nn = d[0];
	max_d_1nn = d[d.size()-1];
	median_d_1nn = d[(d.size()-1)/2];
	double s=0;
	for (size_t i=0; i<obs; ++i) s += d[i];
	mean_d_1nn = s / (double) obs;

	stringstream ss;
	ss << "Long / Lat points stats:" << endl;
	ss << "  min_d_1nn: " << min_d_1nn << " rad, "
	   << RadToDeg(min_d_1nn) << " deg, "
	   << EarthRadToKm(min_d_1nn) << " km, "
	   << EarthRadToMi(min_d_1nn) << " mi" << endl;
	ss << "  max_d_1nn: " << max_d_1nn << " rad, "
	   << RadToDeg(max_d_1nn) << " deg, "
	   << EarthRadToKm(max_d_1nn) << " km, "
	   << EarthRadToMi(max_d_1nn) << " mi" << endl;
	ss << "  median_d_1nn: " << median_d_1nn << " rad, "
	   << RadToDeg(median_d_1nn) << " deg, "
	   << EarthRadToKm(median_d_1nn) << " km, "
	   << EarthRadToMi(median_d_1nn) << " mi" << endl;
	ss << "  mean_d_1nn: " << mean_d_1nn << " rad, "
	   << RadToDeg(mean_d_1nn) << " deg, "
	   << EarthRadToKm(mean_d_1nn) << " km, "
	   << EarthRadToMi(mean_d_1nn) << " mi" << endl;
	ss << "  running time in ms: " << sw.Time();
}

GwtWeight* SpatialIndAlgs::knn_build(const rtree_pt_lonlat_t& rtree, int nn)
{
	GwtWeight* Wp = new GwtWeight;
	Wp->num_obs = rtree.size();
	Wp->is_symmetric = false;
	Wp->symmetry_checked = true;
	Wp->gwt = new GwtElement[Wp->num_obs];
	
	int cnt=0;
	const int k=nn+1;
	for (rtree_pt_lonlat_t::const_query_iterator it =
			 rtree.qbegin(bgi::intersects(rtree.bounds()));
		 it != rtree.qend() ; ++it)
	{
		const pt_lonlat_val& v = *it;
		size_t obs = v.second;		
		vector<pt_lonlat_val> q;
		rtree.query(bgi::nearest(v.first, k), std::back_inserter(q));
		GwtElement& e = Wp->gwt[obs];
		e.alloc(q.size());
		BOOST_FOREACH(const pt_lonlat_val& w, q) {
			if (w.second == v.second) continue;
			GwtNeighbor neigh;
			neigh.nbx = w.second;
			neigh.weight = bg::distance(v.first, w.first);
			e.Push(neigh);
			++cnt;
		}
	}

	return Wp;
}

bool SpatialIndAlgs::write_gwt(const GwtWeight* W,
							   const wxString& _layer_name,
							   const wxString& ofname,
							   const wxString& vname,
							   const std::vector<wxInt64>& id_vec)  
{
    if (!W) {
        return false;
    }
	const GwtElement* g = W->gwt;
	size_t num_obs = W->num_obs;
    
    if (!g ||
        _layer_name.IsEmpty() ||
        ofname.IsEmpty() ||
        id_vec.size() == 0 ||
        num_obs != id_vec.size())
    {
        return false;
    }

    wxFileName gwtfn(ofname);
    wxString gwt_ofn(gwtfn.GetFullPath());

#ifdef __WIN32__
	std:ofstream out(gwt_ofn.wc_str());
#else
	std:ofstream out;
	out.open(GET_ENCODED_FILENAME(gwt_ofn));
#endif
    
    if (!(out.is_open() && out.good())) {
        return false;
    }

    wxString layer_name(_layer_name);
    // if layer_name contains an empty space, the layer name should be
    // braced with quotes "layer name"
    if (layer_name.Contains(" ")) {
        layer_name = "\"" + layer_name + "\"";
    }
    
    out << "0" << " " << num_obs << " " << layer_name;
    out << " " << vname.mb_str() << endl;
    
    for (size_t i=0; i<num_obs; ++i) {
        for (long nbr=0, sz=g[i].Size(); nbr<sz; ++nbr) {
            GwtNeighbor current=g[i].elt(nbr);
            double w = current.weight;
            out << id_vec[i] << ' ' << id_vec[current.nbx];
			out << ' ' << setprecision(9) << w << endl;
        }
    }
    return true;
}

void SpatialIndAlgs::fill_pt_rtree(rtree_pt_2d_t& rtree,
								   const std::vector<pt_2d>& pts)
{
	size_t obs = pts.size();
	for (size_t i=0; i<obs; ++i) {
		rtree.insert(make_pair(pts[i], i));
	}
}

void SpatialIndAlgs::fill_pt_rtree(rtree_pt_lonlat_t& rtree,
								   const std::vector<pt_lonlat>& pts)
{
	size_t obs = pts.size();
	for (size_t i=0; i<obs; ++i) {
		rtree.insert(make_pair(pts[i], i));
	}
}

void SpatialIndAlgs::fill_pt_rtree(rtree_pt_3d_t& rtree,
								   const std::vector<pt_3d>& pts)
{
	size_t obs = pts.size();
	for (size_t i=0; i<obs; ++i) {
		rtree.insert(make_pair(pts[i], i));
	}
}

std::ostream& SpatialIndAlgs::operator<< (std::ostream &out,
						  const LonLatPt& pt) {
	out << "(" << pt.lon << "," << pt.lat << ")";
    return out;
}

std::ostream& SpatialIndAlgs::operator<< (std::ostream &out,
										  const wxRealPoint& pt) {
	out << "(" << pt.x << "," << pt.y << ")";
    return out;
}

std::ostream& SpatialIndAlgs::operator<< (std::ostream &out, const XyzPt& pt) {
	out << "(" << pt.x << "," << pt.y << "," << pt.z << ")";
    return out;
}


