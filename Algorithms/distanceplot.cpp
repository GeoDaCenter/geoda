#include <math.h>
#include <cfloat>
#include <time.h>
#include <stdlib.h>

#include <boost/foreach.hpp>

#include "../GenGeomAlgs.h"
#include "distanceplot.h"


DistancePlot::DistancePlot(const std::vector<GdaPoint*>& points,
                           const std::vector<std::vector<double> >& data,
                           const std::vector<std::vector<bool> >& data_undefs,
                           char dist_method,
                           bool is_arc, bool is_mile,
                           uint64_t last_seed_used,
                           bool reuse_last_seed)
: points(points), data(data), data_undefs(data_undefs),
last_seed_used(last_seed_used), reuse_last_seed(reuse_last_seed),
is_arc(is_arc), is_mile(is_mile),
dist_method(dist_method), rand_count(0), num_pts(0)
{
    min_x = DBL_MAX;
    min_y = DBL_MAX;
    max_x = DBL_MIN;
    max_y = DBL_MIN;

    num_obs = points.size();
    num_vars = data.size();

    if (!reuse_last_seed) {
        unsigned int initseed = (unsigned int) time(0);
        srand(initseed);
        last_seed_used = rand();
    }
}


DistancePlot::~DistancePlot()
{

}

bool DistancePlot::compute_var_dist(size_t i, size_t j, double& var_dist)
{
    bool undef = false;
    double val = 0;
    if (dist_method == 'm') {
        for (size_t v=0; v < num_vars; ++v) {
            val += fabs(data[v][i] - data[v][j]);
            undef = undef || data_undefs[v][i];
            undef = undef || data_undefs[v][j];
        }
        var_dist = sqrt(val);
    } else {
        // euclidean as default 'e'
        for (size_t v=0; v < num_vars; ++v) {
            val = data[v][i] - data[v][j];
            var_dist += val * val;
            undef = undef || data_undefs[v][i];
            undef = undef || data_undefs[v][j];
        }
        var_dist = sqrt(var_dist);
    }

    return undef;
}

double DistancePlot::compute_geo_dist(size_t i, size_t j)
{
    double dist = 0;
    if (is_arc) {
        if (is_mile) {
            dist = GenGeomAlgs::ComputeArcDistMi(points[i]->GetY(),
                                                 points[i]->GetX(),
                                                 points[j]->GetY(),
                                                 points[j]->GetX());
        } else {
            dist = GenGeomAlgs::ComputeArcDistKm(points[i]->GetY(),
                                                 points[i]->GetX(),
                                                 points[j]->GetY(),
                                                 points[j]->GetX());
        }
    } else {
        dist = GenGeomAlgs::ComputeEucDist(points[i]->GetX(),
                                           points[i]->GetY(),
                                           points[j]->GetX(),
                                           points[j]->GetY());
    }
    return dist;
}

/**
 * mark which index is randomly selected for sampling approach
 */
void DistancePlot::gen_rand_flag(size_t i, size_t num_rand)
{
    if (rand_count >= num_rand)
        return;

    size_t rand_idx = Gda::ThomasWangHashDouble(last_seed_used+i) * num_pts;
    if (rand_flags[rand_idx] == false) {
        mutex.lock();
        rand_count ++;
        rand_flags[rand_idx] = true;
        mutex.unlock();
    }
}

void DistancePlot::pick_random_list(size_t num_rand)
{
    thread_pool pool;
    for (size_t i=0; i<num_pts; ++i) {
        if (rand_count > num_rand)
            break;
        pool.enqueue(boost::bind(&DistancePlot::gen_rand_flag, this, i, num_rand));
    }
}

void DistancePlot::run(bool rand_sample, size_t num_rand)
{
    // init return result
    num_pts = (num_obs * num_obs - num_obs) / 2;
    if (rand_sample) {
        if (num_rand < num_pts) {
            rand_flags.resize(num_pts, false);
            pick_random_list(num_rand);
            num_pts = num_rand;
        } else {
            rand_sample = false;
        }

    }

    if (!rand_sample){
        x.resize(num_pts);
        y.resize(num_pts);
        x_undefs.resize(num_pts);
        y_undefs.resize(num_pts);
    }

    // run
    thread_pool pool;
    for (size_t i=0; i<num_obs; ++i) {
        pool.enqueue(boost::bind(&DistancePlot::compute_dist, this, i, rand_sample));
    }
}

void DistancePlot::compute_dist(size_t row_idx, bool rand_sample)
{
    double geo_dist, var_dist, val;
    size_t scatter_pos, sum_to_m, sum_to_n = num_obs * (num_obs - 1) / 2;

    for (size_t j=row_idx + 1; j<num_obs; ++j) {
        sum_to_m = (num_obs - row_idx) * (num_obs - row_idx - 1) / 2;
        scatter_pos = (sum_to_n - sum_to_m) + (j - row_idx -  1);

        if (rand_sample) {
            if (rand_flags[scatter_pos] == false) {
                continue;
            }
        }

        // compute geo distance
        geo_dist = compute_geo_dist(row_idx, j);

        // compute variable distance
        bool undef = compute_var_dist(row_idx, j, var_dist);

        if (rand_sample) {
            mutex.lock();
            x.push_back(geo_dist);
            y.push_back(var_dist);
            x_undefs.push_back(undef);
            y_undefs.push_back(undef);
            mutex.unlock();
        } else {
            //mutex.lock();
            x[scatter_pos] = geo_dist;
            y[scatter_pos] = var_dist;
            x_undefs[scatter_pos] = undef;
            y_undefs[scatter_pos] = undef;
            //mutex.unlock();
        }
        if (geo_dist < min_x) min_x = geo_dist;
        if (geo_dist > max_x) max_x = geo_dist;
        if (var_dist < min_y) min_y = var_dist;
        if (var_dist > max_y) max_y = var_dist;
    }
}

void DistancePlot::run(const rtree_pt_2d_t& rtree, double thresh)
{
    thread_pool pool;
    for (size_t i=0; i<num_obs; ++i) {
        pool.enqueue(boost::bind(&DistancePlot::compute_dist_thres, this, i,
                                 rtree, thresh));
    }
}

void DistancePlot::compute_dist_thres(size_t row_idx, const rtree_pt_2d_t& rtree,
                                      double thresh)
{
    double geo_dist, var_dist = 0, val;

    double pt_x = points[row_idx]->GetX();
    double pt_y = points[row_idx]->GetY();
    box_2d b(pt_2d(pt_x-thresh, pt_y-thresh), pt_2d(pt_x+thresh, pt_y+thresh));

    // query points within threshold distance
    std::vector<pt_2d_val> q;
    rtree.query(bgi::intersects(b), std::back_inserter(q));

    BOOST_FOREACH(const pt_2d_val& w, q) {
        const size_t j = w.second;
        if (j > row_idx) {
            // compute geo distance
            geo_dist = compute_geo_dist(row_idx, j);
            if (geo_dist > thresh) continue;

            // compute variable distance
            bool undef = compute_var_dist(row_idx, j, var_dist);

            // save to results
            mutex.lock();
            x.push_back(geo_dist);
            y.push_back(var_dist);
            x_undefs.push_back(undef);
            y_undefs.push_back(undef);
            mutex.unlock();

            // update min/max
            if (geo_dist < min_x) min_x = geo_dist;
            if (geo_dist > max_x) max_x = geo_dist;
            if (var_dist < min_y) min_y = var_dist;
            if (var_dist > max_y) max_y = var_dist;
        }
    }
}

void DistancePlot::run(const rtree_pt_3d_t& rtree, double thresh)
{
    thread_pool pool;
    for (size_t i=0; i<num_obs; ++i) {
        pool.enqueue(boost::bind(&DistancePlot::compute_dist_thres3d, this, i,
                                 rtree, thresh));
    }
}

void DistancePlot::compute_dist_thres3d(size_t row_idx, const rtree_pt_3d_t& rtree,
                                        double thresh)
{
    double geo_dist, var_dist, val;

    double pt_x = points[row_idx]->GetX();
    double pt_y = points[row_idx]->GetY();
    double x_3d, y_3d, z_3d;
    GenGeomAlgs::LongLatDegToUnit(pt_x, pt_y, x_3d, y_3d, z_3d);

    // thresh is in radians.  Need to convert to unit sphere secant distance
    double sec_thresh = thresh;// GenGeomAlgs::RadToUnitDist(thresh);

    box_3d b(pt_3d(x_3d - sec_thresh, y_3d - sec_thresh, z_3d - sec_thresh),
             pt_3d(x_3d + sec_thresh, y_3d + sec_thresh, z_3d + sec_thresh));

    // query points within threshold distance
    std::vector<pt_3d_val> q;
    rtree.query(bgi::intersects(b), std::back_inserter(q));

    BOOST_FOREACH(const pt_3d_val& w, q) {
        const size_t j = w.second;
        if (j > row_idx) {
            // compute geo distance
            geo_dist = compute_geo_dist(row_idx, j);
            if (geo_dist > thresh) continue;
            // compute variable distance
            bool undef = compute_var_dist(row_idx, j, var_dist);

            // save to results
            mutex.lock();
            x.push_back(geo_dist);
            y.push_back(var_dist);
            x_undefs.push_back(undef);
            y_undefs.push_back(undef);
            mutex.unlock();

            // update min/max
            if (geo_dist < min_x) min_x = geo_dist;
            if (geo_dist > max_x) max_x = geo_dist;
            if (var_dist < min_y) min_y = var_dist;
            if (var_dist > max_y) max_y = var_dist;
        }
    }
}

double DistancePlot::GetMinX()
{
    return min_x;
}
double DistancePlot::GetMaxX()
{
    return max_x;
}
double DistancePlot::GetMinY()
{
    return min_y;
}
double DistancePlot::GetMaxY()
{
    return max_y;
}

const std::vector<double>& DistancePlot::GetX()
{
    return x;
}

const std::vector<double>& DistancePlot::GetY()
{
    return y;
}

const std::vector<bool>& DistancePlot::GetXUndefs()
{
    return x_undefs;
}

const std::vector<bool>& DistancePlot::GetYUndefs()
{
    return y_undefs;
}
