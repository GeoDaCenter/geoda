// Author: Xun Li <lixun910@gmail.com>
// May 27, 2020
//
// Code ported from elki project: http://elki-project.github.io/
// Copyright follows elki project: http://elki-project.github.io/
// AGPLv3: https://github.com/elki-project/elki/blob/master/LICENSE.md
//
// 5-27-2020
// Xoroshiro128Random random number generator
// PAM, CLARA, CLARANS
// Initializer: BUILD and LAB
// FastPAM, FastCLARA, FastCLARANS
#ifndef __XL_PAM_H
#define __XL_PAM_H

#include <vector>
#include <boost/unordered_map.hpp>

#include "DataUtils.h"
#include "rng.h"

using namespace boost;


class PAMUtils
{
public:
    static std::vector<int> randomSample(Xoroshiro128Random& rand,
                                         int samplesize, int n,
                                         const std::vector<int>& previous = std::vector<int>());
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Initializer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PAMInitializer
{
protected:
    DistMatrix* dist;
public:
    PAMInitializer(DistMatrix* dist) : dist(dist) {}
    virtual ~PAMInitializer() {}
    virtual std::vector<int> run(const std::vector<int>& ids, int k) = 0;
};

class BUILD : public PAMInitializer
{
public:
    BUILD(DistMatrix* dist) : PAMInitializer(dist) {}
    virtual ~BUILD(){}
    virtual std::vector<int> run(const std::vector<int>& ids, int k);
};

// LAB (linear approximative BUILD)
class LAB : public PAMInitializer
{
    
public:
    LAB(DistMatrix* dist, int seed=123456789) : PAMInitializer(dist), random(seed) {}
    virtual ~LAB(){}
    virtual std::vector<int> run(const std::vector<int>& ids, int k);
    
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
    
    // seed
    Xoroshiro128Random random;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PAM
{
public:
    PAM(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
        int k, int maxiter, const std::vector<int>& ids=std::vector<int>());
    virtual ~PAM();
    
    virtual double run();
    
    virtual std::vector<int> getResults();
    
    virtual std::vector<int> getMedoids();
    
    virtual std::vector<int> getAssignement() { return assignment;}
protected:
    
    // Get distance between i-th and j-th object
    double getDistance(int i, int j);
    
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
    
protected:
    // Number of observations
    int num_obs;
    
    // Distance matrix
    DistMatrix* dist_matrix;
    
    // PAM Initializer
    PAMInitializer* initializer;
    
    // Number of clusters
    int k;
    
    // Max iteration
    int maxiter;
    
    // ids: could be non-enumeric numbers
    std::vector<int> ids;

    // cluster assignment
    std::vector<int> assignment;
    
    // distance to nearest
    std::vector<double> nearest;
    
    // distance to second nearest
    std::vector<double> second;
    
    // medoids
    std::vector<int> medoids;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastPAM : public PAM
{
public:
    // fasttol: controls how many additional swaps are performed.
    //   When set to 0, it will only execute an additional swap if it
    //   appears to be independent (i.e., the improvements resulting from the
    //   swap have not decreased when the first swap was executed).
    //   Default: 1 which means to perform any additional swap that gives an improvement.
    //   We could not observe a tendency to find worse results when doing these
    //   additional swaps, but a reduced runtime.
    FastPAM(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
            int k, int maxiter, double fasttol, const std::vector<int>& ids=std::vector<int>());
    virtual ~FastPAM();
    
    virtual double run() { return PAM::run(); }
protected:
    
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
    double fastswap;
    
    // Tolerance for fast swapping behavior (may perform worse swaps).
    double fasttol;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CLARA
{
public:
    // k Number of clusters to produce
    // maxiter Maximum number of iterations
    // numsamples Number of samples (sampling iterations)
    //    default: 5
    // sampling Sampling rate (absolute or relative)
    //    Default sample size suggested by Kaufman and Rousseeuw
    //    default: 40 + 2. * k
    // independent NOT Keep the previous medoids in the next sample
    //    false: using previous medoids in next sample
    CLARA(int num_obs, DistMatrix* dist_matrix,  PAMInitializer* init,
          int k, int maxiter, int numsamples, double sampling, bool independent, int seed=123456789);
    
    virtual ~CLARA();
    
    virtual double run();
    
    virtual std::vector<int> getResults();
    
    virtual std::vector<int> getMedoids() { return bestmedoids;}
    
protected:
    
    // Assign remining to nearest cluster
    double assignRemainingToNearestCluster(std::vector<int>& means,
                                           std::vector<int>& rids,
                                           std::vector<int>& r_assignment,
                                           std::vector<int>& assignment);
protected:
    // Number of observations
    int num_obs;
    
    // Distance matrix
    DistMatrix* dist_matrix;
    
    // PAM Initializer
    PAMInitializer* initializer;
    
    // Number of clusters
    int k;
    
    // Max iteration
    int maxiter;
    
    // Sampling rate. If less than 1, it is considered to be a relative value.
    double sampling;
    
    //  Number of samples to draw (i.e. iterations).
    int numsamples;
    
    // Keep the previous medoids in the sample (see page 145).
    bool keepmed;
    
    // Random factory for initialization.
    Xoroshiro128Random random;
    
    std::vector<int> bestmedoids;
    
    std::vector<int> bestclusters;
};

class FastCLARA : public CLARA
{
public:
    // k Number of clusters to produce
    // maxiter Maximum number of iterations
    // fasttol: FastPAM
    // numsamples Number of samples (sampling iterations)
    //    default: 5
    // sampling Sampling rate (absolute or relative)
    //    Larger sample size, used by Schubert and Rousseeuw, 2019
    //    default: 80 + 4. * k (has to > 3*k)
    // keepmed Keep the previous medoids in the next sample
    //    true if numsamples > 1
    FastCLARA(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
              int k, int maxiter, double fasttol,
              int numsamples, double sampling, bool independent, int seed=123456789);
    
    virtual ~FastCLARA() {}
    
    virtual double run();
    
protected:
    double fasttol;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Assignment
{
public:
    int k;
    int num_obs;
    // Distance matrix
    DistMatrix* dist_matrix;
    boost::unordered_map<int, bool> medoids_dict;
    std::vector<int> medoids;
    std::vector<int> assignment;
    std::vector<double> nearest;
    std::vector<int> secondid;
    std::vector<double> second;
    
public:
    Assignment() {}
    Assignment(int k, int num_obs, DistMatrix* dist_matrix);
    virtual ~Assignment() {}
    
    // Overload = operator
    virtual Assignment& operator=(const Assignment& other);
    
    // Compute the reassignment cost, for one swap.
    double computeCostDifferential(int h, int mnum, Assignment& scratch);
    
    //Recompute the assignment of one point.
    virtual double recompute(int id, int mnum, double known, int snum, double sknown);
    
    // Assign each point to the nearest medoid.
    virtual double assignToNearestCluster();
    
    // Check if medoid is already assigned
    virtual bool hasMedoid(int cand);
};

class CLARANS
{
public:
    // k Number of clusters to produce
    // numlocal  Number of samples to draw (i.e. restarts).
    //    default: 2
    // maxneighbor Sampling rate. If less than 1, it is considered to be a relative value.
    //    default:  0.0125
    CLARANS(int num_obs, DistMatrix* dist_matrix,
            int k, int numlocal, double maxneighbor, int seed=123456789);
    
    virtual ~CLARANS();
    
    virtual double run();
    
    virtual std::vector<int> getResults();
    
    virtual std::vector<int> getMedoids() { return bestmedoids;}
    
protected:
    
    // Assign remining to nearest cluster
    double assignRemainingToNearestCluster(std::vector<int>& means,
                                           std::vector<int>& rids,
                                           std::vector<int>& r_assignment,
                                           std::vector<int>& assignment);
protected:
    // Number of observations
    int num_obs;
    
    // Distance matrix
    DistMatrix* dist_matrix;
    
    // Number of clusters
    int k;
    
    // Number of samples to draw (i.e. restarts).
    int numlocal;
    
    //  Sampling rate. If less than 1, it is considered to be a relative value.
    double maxneighbor;
    
    // Random factory for initialization.
    Xoroshiro128Random random;
    
    std::vector<int> bestmedoids;
    
    std::vector<int> bestclusters;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FastAssignment : public Assignment
{
public:
    // Array for storing the per-medoid costs.
    std::vector<double> cost;
    
    // Last best medoid number
    int lastbest;
    
public:
    FastAssignment() : Assignment() {}
    FastAssignment(int k, int num_obs, DistMatrix* dist_matrix)
    : Assignment(k, num_obs, dist_matrix), cost(k) {}
    
    virtual ~FastAssignment()  {}
    
    // Compute the reassignment cost, for one swap.
    double computeCostDifferential(int h);
    
    // Perform last swap
    void performLastSwap(int h);
};

// A faster variation of CLARANS, that can explore O(k) as many swaps at a
// similar cost by considering all medoids for each candidate non-medoid. Since
// this means sampling fewer non-medoids, we suggest to increase the subsampling
// rate slightly to get higher quality than CLARANS, at better runtime.
//
class FastCLARANS : public CLARANS
{
public:
    // k Number of clusters to produce
    // numlocal  Number of samples to draw (i.e. restarts).
    //    default: 2
    // maxneighbor Sampling rate. If less than 1, it is considered to be a relative value.
    //    default:  2 * 0.0125, larger sampling rate
    FastCLARANS(int num_obs, DistMatrix* dist_matrix,
                int k, int numlocal, double maxneighbor,  int seed=123456789);
    virtual ~FastCLARANS() {}
    
    virtual double run();
};

#endif
