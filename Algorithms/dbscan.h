#ifndef __GEODA_CENTER_DBSCAN_H___
#define __GEODA_CENTER_DBSCAN_H___

#include <vector>

class ANNkd_tree;

/**
 * DBSCAN: Density-Based Spatial Clustering of Applications with Noise
 *
 * see: scikit-learn/sklearn/cluster/_dbscan.py
 */
class DBSCAN {
public:
    /**
     * Perform DBSCAN clustering from *raw 2d array data
     */
    DBSCAN(unsigned int min_samples, float eps, const double** input_data,
           unsigned int num_rows, unsigned int num_cols, int distance_metric);

    virtual ~DBSCAN();

    virtual std::vector<int> getResults();

protected:
    void run();

    void createNearestNeighbors(const double** input_data);

    // eps : float, The maximum distance between two samples for one to be considered
    // as in the neighborhood of the other. This is not a maximum bound
    // on the distances of points within a cluster. This is the most
    // important DBSCAN parameter to choose appropriately for your data set
    // and distance function.
    float eps;

    // The number of samples (or total weight) in a neighborhood for a point
    // to be considered as a core point. This includes the point itself.
    unsigned int min_samples;

    // Number of rows (observations)
    unsigned int num_rows;

    // Number of columns (variables)
    unsigned int num_cols;

    // ANN kd-tree
    ANNkd_tree* kd_tree;


    // nearest neighbors
    std::vector<std::vector<std::pair<int, double> > > nn;

    // labels
    std::vector<int> labels;

    // core flags
    std::vector<bool> is_core;

};

#endif
