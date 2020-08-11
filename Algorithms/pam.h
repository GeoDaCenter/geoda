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
using namespace boost;

/**
 * title = "xoroshiro+ / xorshift* / xorshift+ generators and the PRNG shootout", //
 * booktitle = "Online", //
 * url = "http://xoroshiro.di.unimi.it/", //
 */
class Xoroshiro128Random
{
    long long s0;
    long long s1;
public:
    Xoroshiro128Random(long long xor64 = 123456789) {
        // set seed
        // XorShift64* generator to seed:
        if (xor64 == 0)
            xor64 = 4101842887655102017L;
        xor64 ^= (unsigned long long)xor64 >> 12; // a
        xor64 ^= xor64 << 25; // b
        xor64 ^= (unsigned long long)xor64 >> 27; // c
        s0 = xor64 * 2685821657736338717L;
        xor64 ^= (unsigned long long)xor64 >> 12; // a
        xor64 ^= xor64 << 25; // b
        xor64 ^= (unsigned long long)xor64 >> 27; // c
        s1 = xor64 * 2685821657736338717L;
    }
    virtual ~Xoroshiro128Random() {}
    int nextInt(int n) {
        if (n <=0) return 0;
        int r =  (int)((n & -n) == n ? nextLong() & n - 1 // power of two
            : (unsigned long long)(((unsigned long long)nextLong() >> 32) * n) >> 32);
        return r;
    }
    long long nextLong() {
        long long t0 = s0, t1 = s1;
        long long result = t0 + t1;
        t1 ^= t0;
        // left rotate: (n << d)|(n >> (INT_BITS - d));
        s0 = (t0 << 55) | ((unsigned long long)t0 >> (64 - 55));
        s0 = s0 ^ t1 ^ (t1 << 14); // a, b
        s1 = (t1 << 36) | ((unsigned long long)t1 >> (64 -36));
        return result;
    }
    double nextDouble() {
#ifdef WIN32
		char tempStr[] = "0x1.0p-53";
		double nd = std::strtod(tempStr, NULL);
        return ((unsigned long long)nextLong() >> 11) * nd;
#else
		return ((unsigned long long)nextLong() >> 11) * 0x1.0p-53;
#endif
    }
    std::vector<int> randomSample(int samplesize, int n)
    {
        std::vector<int> samples(samplesize);
        int i=0;
        boost::unordered_map<int, bool> sample_dict;
        boost::unordered_map<int, bool>::iterator it;
        while (sample_dict.size() < samplesize) {
            int rnd = nextInt(n);
            if (sample_dict.find(rnd) == sample_dict.end()) {
                samples[i++] = rnd;
            }
            sample_dict[rnd] = true;
        }
        return samples;
    }
};

class PAMUtils
{
public:
    static std::vector<int> randomSample(Xoroshiro128Random& rand,
                                         int samplesize, int n,
                                         const std::vector<int>& previous = std::vector<int>());
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DistMatrix
{
protected:
    std::vector<int> ids;
    bool has_ids;
public:
    DistMatrix(const std::vector<int>& _ids=std::vector<int>())
    : ids(_ids), has_ids(!_ids.empty()) {}
    virtual ~DistMatrix() {}
    // Get distance between i-th and j-th object
    // if ids vector is provided, the distance (i,j) -> distance(ids[i], ids[j])
    virtual double getDistance(int i, int j) = 0;
    virtual void setIds(const std::vector<int>& _ids) {
        ids = _ids;
        has_ids = !ids.empty();
    }
};

class RawDistMatrix : public DistMatrix
{
    double** dist;
public:
    RawDistMatrix(double** dist, const std::vector<int>& _ids=std::vector<int>())
    : DistMatrix(_ids), dist(dist) {}
    virtual ~RawDistMatrix() {}
    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        if (has_ids) {
            i = ids[i];
            j = ids[j];
        }
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
    RDistMatrix(int num_obs, const std::vector<double>& dist, const std::vector<int>& _ids=std::vector<int>())
    : DistMatrix(_ids), num_obs(num_obs), dist(dist) {
        n = (num_obs - 1) * num_obs / 2;
    }
    virtual ~RDistMatrix() {}
    
    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        if (has_ids) {
            i = ids[i];
            j = ids[j];
        }
        // lower part triangle, store column wise
        int r = i > j ? i : j;
        int c = i < j ? i : j;
        int idx = n - (num_obs - c - 1) * (num_obs - c) / 2 + (r -c) -1 ;
        return dist[idx];
    }
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
