#include <math.h>
#include <cfloat>
#include <time.h>
#include <stdlib.h>

#include "distanceplot.h"


DistancePlot::DistancePlot(const std::vector<GdaPoint*>& points,
                           const std::vector<std::vector<double> >& data,
                           const std::vector<std::vector<bool> >& data_undefs,
                           char dist_method,
                           bool rand_sample,
                           size_t num_rand,
                           uint64_t last_seed_used,
                           bool reuse_last_seed)
: points(points), data(data), data_undefs(data_undefs),
last_seed_used(last_seed_used), reuse_last_seed(reuse_last_seed),
dist_method(dist_method), rand_sample(rand_sample), num_rand(num_rand),
rand_count(0)
{
    min_x = DBL_MAX;
    min_y = DBL_MAX;
    max_x = DBL_MIN;
    max_y = DBL_MIN;

    num_obs = points.size();
    num_vars = data.size();

    num_pts = (num_obs * num_obs - num_obs) / 2;
    rand_flags.resize(num_pts, false);

    if (!reuse_last_seed) {
        unsigned int initseed = (unsigned int) time(0);
        srand(initseed);
        last_seed_used = rand();
    }

    if (rand_sample) {
        thread_pool pool;
        for (size_t i=0; i<num_pts; ++i) {
            pool.enqueue(boost::bind(&DistancePlot::gen_rand_flag, this, i));
        }
        num_pts = num_rand;

    } else {
        // init return result
        x.resize(num_pts);
        y.resize(num_pts);
        x_undefs.resize(num_pts);
        y_undefs.resize(num_pts);
    }
}

DistancePlot::~DistancePlot()
{

}

void DistancePlot::gen_rand_flag(size_t i)
{
    if (rand_count > num_rand) return;

    size_t rand_idx = Gda::ThomasWangHashDouble(last_seed_used+i) * num_pts;
    if (rand_flags[rand_idx] == false) {
        mutex.lock();
        rand_count ++;
        rand_flags[rand_idx] = true;
        mutex.unlock();
    }
}

void DistancePlot::run()
{

    thread_pool pool;
    for (size_t i=0; i<num_obs; ++i) {
        pool.enqueue(boost::bind(&DistancePlot::compute_dist, this, i));
    }
    //for (size_t i=0; i<num_obs; ++i) {
    //    compute_dist(i);
    //}
}

void DistancePlot::compute_dist(size_t row_idx)
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

        bool undef = false;
        if (dist_method == 'e') {
            // compute geo distance
            val = points[row_idx]->GetX() - points[j]->GetX();
            geo_dist = val * val;
            val = points[row_idx]->GetY() - points[j]->GetY();
            geo_dist = geo_dist + val * val;
            geo_dist = sqrt(geo_dist);

            // compute variable distance
            var_dist = 0;
            for (size_t v=0; v < num_vars; ++v) {
                val = data[v][row_idx] - data[v][j];
                var_dist += val * val;
                undef = undef || data_undefs[v][row_idx];
                undef = undef || data_undefs[v][j];
            }
            var_dist = sqrt(var_dist);
        }

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
