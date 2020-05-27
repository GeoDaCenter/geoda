#ifndef __GEODA_CENTER_PAM_H
#define __GEODA_CENTER_PAM_H

#include <vector>
#if __cplusplus >= 201103
#include <unordered_map>
using namespace std;
#else
#include <boost/unordered_map.hpp>
using namespace boost;
#endif
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
        return ((unsigned long long)nextLong() >> 11) * 0x1.0p-53;
    }
    std::vector<int> randomSample(int samplesize, int n)
    {
        unordered_map<int, bool> sample_dict;
        unordered_map<int, bool>::iterator it;
        while (sample_dict.size() < samplesize) {
            sample_dict[nextInt(n)] = true;
        }
        std::vector<int>  samples(samplesize);
        int i=0;
        for (it = sample_dict.begin(); it != sample_dict.end(); ++it) {
            samples[i++] = it->first;
        }
        return samples;
    }
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
        int idx = n - (num_obs - c + 1) * (num_obs - c) / 2 + r ;
        return idx;
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

class LAB : public PAMInitializer
{
    
public:
    LAB(DistMatrix* dist) : PAMInitializer(dist) {}
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
    FastPAM(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
            int k, int maxiter, double fasttol=1.0, const std::vector<int>& ids=std::vector<int>());
    virtual ~FastPAM();
    
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
    double fastswap = 0.;
    
    // Tolerance for fast swapping behavior (may perform worse swaps).
    double fasttol = 0;
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
    // sampling Sampling rate (absolute or relative)
    // keepmed Keep the previous medoids in the next sample
    CLARA(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
    int k, int maxiter, int numsamples, double sampling, bool keepmed);
    
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Assignment
{
public:
    Assignment();
    virtual ~Assignment();
    
    std::vector<int> medoids;
    std::vector<int> assignment;
    std::vector<double> nearest;
    std::vector<int> secondid;
    std::vector<double> second;
    
    // Compute the reassignment cost, for one swap.
    double computeCostDifferential(int h, int mnum, Assignment& scratch);
    
    //Recompute the assignment of one point.
    double recompute(int i, int mnumn, double known, int snum, double sknown);
    
    // Assign each point to the nearest medoid.
    double assignToNearestCluster();
    
    bool hasMedoid(int cand);
};

class CLARANS
{
public:
    // k Number of clusters to produce
    // maxiter Maximum number of iterations
    // numsamples Number of samples (sampling iterations)
    // sampling Sampling rate (absolute or relative)
    CLARANS(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
    int k, int numlocal, double maxneighbor);
    
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
    
    // PAM Initializer
    PAMInitializer* initializer;
    
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


#endif
