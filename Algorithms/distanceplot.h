#ifndef __GEODA_CENTER_DISTANCEPLOT_H___
#define __GEODA_CENTER_DISTANCEPLOT_H___

#include <vector>

#include "../GdaShape.h"

class DistancePlot
{

public:
    DistancePlot(const std::vector<GdaPoint*>& points,
                 const std::vector<std::vector<double> >& data,
                 const std::vector<std::vector<bool> >& data_undefs,
                 char dist_method = 'e',
                 bool rand_sample = false,
                 size_t num_rand = 0,
                 uint64_t last_seed_used = 1234567890,
                 bool reuse_last_seed = true);
    virtual ~DistancePlot();

    void run();

    const std::vector<double>& GetX();
    const std::vector<double>& GetY();
    const std::vector<bool>& GetXUndefs();
    const std::vector<bool>& GetYUndefs();

    double GetMinX();
    double GetMaxX();
    double GetMinY();
    double GetMaxY();

protected:
    bool rand_sample;
    size_t num_rand;
    size_t num_obs;
    size_t num_vars;
    size_t num_pts;
    size_t rand_count;
    
    char dist_method;
    uint64_t last_seed_used;
    bool reuse_last_seed;
    boost::mutex mutex;

    const std::vector<GdaPoint*>& points;
    const std::vector<std::vector<double> >& data;
    const std::vector<std::vector<bool> >& data_undefs;

    double min_x;
    double max_x;
    double min_y;
    double max_y;

    std::vector<double> x;
    std::vector<double> y;

    std::vector<bool> x_undefs;
    std::vector<bool> y_undefs;

    std::vector<bool> rand_flags;

    void compute_dist(size_t row_idx);

    void gen_rand_flag(size_t i);
};

#endif

