#ifndef __GEODA_CENTER_DISTANCEPLOT_H___
#define __GEODA_CENTER_DISTANCEPLOT_H___

#include <vector>

#include "../SpatialIndAlgs.h"
#include "../GdaShape.h"

class DistancePlot
{

public:
    DistancePlot(const std::vector<GdaPoint*>& points,
                 const std::vector<std::vector<double> >& data,
                 const std::vector<std::vector<bool> >& data_undefs,
                 char dist_method = 'e',
                 bool is_arc = false, bool is_mile = false,
                 uint64_t last_seed_used = 1234567890,
                 bool reuse_last_seed = true);
    
    virtual ~DistancePlot();

    void run(bool is_rand_sample=false, size_t num_rand=0);
    void run(const rtree_pt_2d_t& rtree, double thresh);
    void run(const rtree_pt_3d_t& rtree, double thresh);

    const std::vector<double>& GetX();
    const std::vector<double>& GetY();
    const std::vector<bool>& GetXUndefs();
    const std::vector<bool>& GetYUndefs();

    double GetMinX();
    double GetMaxX();
    double GetMinY();
    double GetMaxY();

    bool IsArc() { return is_arc;}
    bool IsMile() { return is_mile;}
    char DistMethod() { return dist_method;}

    size_t GetNumPoints() { return num_pts;}

protected:
    size_t num_obs;
    size_t num_vars;
    size_t num_pts;
    size_t rand_count;

    bool is_mile;
    bool is_arc;

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

    double compute_geo_dist(size_t i, size_t j);

    bool compute_var_dist(size_t i, size_t j, double& var_dist);

    void gen_rand_flag(size_t i, size_t num_rand);
    
    void pick_random_list(size_t num_rand);
    
    void compute_dist(size_t row_idx, bool rand_sample);

    void compute_dist_thres(size_t row_idx, const rtree_pt_2d_t& rtree,
                            double thresh);

    void compute_dist_thres3d(size_t row_idx, const rtree_pt_3d_t& rtree,
                              double thresh);
};

#endif

