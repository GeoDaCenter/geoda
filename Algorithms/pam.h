#ifndef __GEODA_CENTER_PAM_H
#define __GEODA_CENTER_PAM_H

#include <vector>

class DistMatrix
{
public:
    DistMatrix() {}
    virtual ~DistMatrix() {}
    // Get distance between i-th and j-th object
    virtual double getDistance(int i, int j) = 0;
};

class RawDistMatrix : public DistMatrix
{
    double** dist;
public:
    RawDistMatrix(double** dist) : dist(dist) {}
    virtual ~RawDistMatrix() {}
    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        // lower part triangle
        int r = i > j ? i : j;
        int c = i < j ? i : j;
        return dist[r][c];
    }
};

class RDistMatrix : public DistMatrix
{
    int num_obs;
    int n;
    const std::vector<double>& dist;
public:
    RDistMatrix(int num_obs, const std::vector<double>& dist)
    : num_obs(num_obs), dist(dist), DistMatrix() {
        n = (num_obs - 1) * num_obs / 2;
    }
    virtual ~RDistMatrix() {}
    
    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        // lower part triangle, store column wise
        int r = i > j ? i : j;
        int c = i < j ? i : j;
        int idx = n - (num_obs - c + 1) * (num_obs - c) / 2 + r ;
        return idx;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PAM
{
public:
    PAM(int num_obs, DistMatrix* dist_matrix, int k, int maxiter);
    virtual ~PAM();
    
    virtual std::vector<int> run();
    
protected:
    // Partial Fisher-Yates shuffle.
    // ids IDs to shuffle
    // ssize sample size to generate
    //  end Valid range
    void shuffle(std::vector<int>& samples, int ssize, int end);
    
    // Get the minimum distance to previous medoids.
    // j j current object
    // mindist distance storage
    // return minimum distance
    double getMinDist(int j, std::vector<int>& medids, std::vector<double>& mindist);
    
    // Get distance between i-th and j-th object
    double getDistance(int i, int j);
    
    // Choose the initial medoids.
    // return Initial medoids
    virtual std::vector<int> initialMedoids();
    
    // Run the PAM optimization phase.
    virtual double run(std::vector<int>& medoids, int maxiter);
    
    // PAM Assign each object to the nearest cluster, return the cost.
    //  means Object centroids
    // return Assignment cost
    virtual double assignToNearestCluster(std::vector<int>& means);
    
    // Compute the reassignment cost of one swap.
    // h Current object to swap with the medoid
    // mnum Medoid number to be replaced
    virtual double computeReassignmentCost(int h, int mnum);
    
    // random number generator
    int nextInt(int bound);
    
    long long nextLong();
    
protected:
    // Number of observations
    int num_obs;
    
    // Distance matrix
    DistMatrix* dist_matrix;
    
    // Number of clusters
    int k;
    
    // ids
    std::vector<int> ids;
    
    // id : value
    std::vector<int> assignment;
    
    // distance to nearest
    std::vector<double> nearest;
    
    // distance to second nearest
    std::vector<double> second;
    
    // Max iteration
    int maxiter;
    
    // seed
    long long s0;
    long long s1;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastPAM : public PAM
{
public:
    FastPAM(int num_obs, DistMatrix* dist_matrix, int k, int maxiter,
            double fasttol=1.0);
    virtual ~FastPAM();
    
protected:
    // LAB initialization
    virtual std::vector<int> initialMedoids();
    
    // Run the PAM optimization phase.
    virtual double run(std::vector<int>& medoids, int maxiter);
    
    // FastPAM1
    // Compute the reassignment cost, for all medoids in one pass.
    virtual void computeReassignmentCost(int h, std::vector<double>& cost);
    
    // FastPAM1
    // Returns a list of clusters. The k<sup>th</sup> cluster contains the ids
    // of those objects, that are nearest to the k<sup>th</sup> mean.
    virtual double assignToNearestCluster(std::vector<int>& means);
    
    // Compute the reassignment cost of one swap.
    // h Current object to swap with the medoid
    // mnum Medoid number to be replaced
    virtual double computeReassignmentCost(int h, int mnum);
    
    void findBestSwaps(std::vector<int>& medoids,
                       std::vector<int>& bestids,
                       std::vector<double>& best,
                       std::vector<double>& cost);
    
    bool isMedoid(int id);
    
    int argmin(const std::vector<double>& best);
    
    // FastPAM1
    // Update an existing cluster assignment.
    // medoids Medoids set;
    // h New medoid
    // m Position of replaced medoid
    void updateAssignment(std::vector<int>& medoids, int h, int m);
    
    // FastPAM1
    // j Query point
    // medoids Medoids
    // h Known medoid
    // dist_h Distance to h
    // n Known nearest
    // return Index of second nearest medoid, {@link #second} is updated.
    int updateSecondNearest(int j, std::vector<int>& medoids,
                            int h, double dist_h, int n);
    
    
protected:
    // Tolerance for fast swapping behavior (may perform worse swaps).
    double fastswap = 0.;
    
    // Tolerance for fast swapping behavior (may perform worse swaps).
    double fasttol = 0;
};


#endif
