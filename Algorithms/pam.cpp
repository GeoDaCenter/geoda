#include <map>
#include <vector>
#include <set>
#include <math.h>
#include <float.h>
#include <algorithm>    // std::max

#include "pam.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PAM::PAM(int num_obs, DistMatrix* dist_matrix, int k, int maxiter)
: num_obs(num_obs), dist_matrix(dist_matrix), k(k), maxiter(100)
{
    ids.resize(num_obs);
    for (int i=0; i<num_obs; ++i) {
        ids[i] = i;
    }
    
    // set seed
    long long xor64 = 123456789;
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

PAM::~PAM() {
    
}

std::vector<int> PAM::run() {
    // Initialization
    std::vector<int> medoids = initialMedoids();
    
    // Setup cluster assignment store
    assignment.resize(num_obs, -1);
    nearest.resize(num_obs, -1);
    second.resize(num_obs, -1);
    
    // Run partition
    run(medoids, maxiter);
    
    // Get results
    std::vector<int> cluster_result(num_obs, 0);
    for (int i=0; i<ids.size(); ++i) {
        cluster_result[i] = assignment[ids[i]] + 1;
    }
    return cluster_result;
}

// Run the PAM optimization phase.
double PAM::run(std::vector<int> &medoids, int maxiter) {
    int k = (int)medoids.size();
    // Initial assignment to nearest medoids
    // TODO: reuse distance information, from the build phase, when possible?
    double tc = assignToNearestCluster(medoids);
    // Swap phase
    int bestid;
    int iteration = 0;
    while(iteration < maxiter || maxiter <= 0) {
        ++iteration;
        // Try to swap a non-medoid with a medoid member:
        double best = DBL_MAX;
        int bestcluster = -1;
        // Iterate over all non-medoids:
        for (int i=0; i<ids.size(); ++i) {
            int h = ids[i];
            // Compare object to its own medoid.
            if (medoids[assignment[h]] == h) {
                continue; // This is a medoid
            }
            double hdist = nearest[h]; // Current cost of h.
            if(hdist <= 0.) {
              continue; // Duplicate of a medoid.
            }
            // Find the best possible swap for h:
            for(int pi = 0; pi < k; pi++) {
                // hdist is the cost we get back by making the non-medoid h medoid.
                double cpi = computeReassignmentCost(h, pi) - hdist;
                if(cpi < best) {
                    best = cpi;
                    bestid  = h;
                    bestcluster = pi;
                }
            }
        }
        if(!(best < -1e-12 * tc)) {
          break;
        }
        medoids[bestcluster] =  bestid;
        // Reassign
        double nc = assignToNearestCluster(medoids);
        // ".iteration-" + iteration + ".cost", nc
        if(nc > tc) {
            if(nc - tc < 1e-7 * tc) {
                //LOG.warning("PAM failed to converge (numerical instability?)");
                break;
            }
            //LOG.warning("PAM failed to converge: costs increased by: " + (nc - tc) + " exepected a decrease by " + best);
            break;
        }
        tc = nc;
    }
    //".iterations", iteration
    // ".final-cost", tc
    return tc;
}

//Get the minimum distance to previous medoids.
double PAM::getMinDist(int j, std::vector<int>& medids, std::vector<double>& mindist)
{
    double prev = mindist[j];
    if (prev == DBL_MIN) {
        prev = DBL_MAX;
        for (int i=0; i<medids.size(); ++i) {
            double d = getDistance(j, medids[i]);
            prev = d < prev ? d : prev;
        }
        mindist[j] = prev;
    }
    return prev;
}

// Partial Fisher-Yates shuffle.
void PAM::shuffle(std::vector<int> &samples, int ssize, int end) {
    ssize = ssize < end ? ssize : end; // Guard for choosing from tiny sets
    
    for(int i = 1; i < ssize; i++) {
        int a = i-1, b = i+nextInt(end - i);
        int tmp = samples[b];
        samples[b] = samples[a];
        samples[a] = tmp;
    }
}


long long PAM::nextLong()
{
    // seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1)
    //  return (int)(seed >>> (48 - bits)).
    // Using Xoroshiro 128 Nonthread safe
    long long t0 = s0, t1 = s1;
    long long result = t0 + t1;
    t1 ^= t0;
    // left rotate: (n << d)|(n >> (INT_BITS - d));
    s0 = (t0 << 55) | ((unsigned long long)t0 >> (64 - 55));
    s0 = s0 ^ t1 ^ (t1 << 14); // a, b
    s1 = (t1 << 36) | ((unsigned long long)t1 >> (64 -36));
    
    return result;
}

int PAM::nextInt(int n)
{
    if (n <=0)
        return 0;
    
    int r =  (int)((n & -n) == n ? nextLong() & n - 1 // power of two
        : (unsigned long long)(((unsigned long long)nextLong() >> 32) * n) >> 32);
    return r;
}

double PAM::getDistance(int i, int j) {
    return sqrt(dist_matrix->getDistance(i, j));
}

// PAM initialization
std::vector<int> PAM::initialMedoids() {
    std::vector<int> medids;
    std::set<int> medids_set;
    
    // O(sqrt(n)) sample if k^2 < n.
    int ssize = 10 + (int)ceil(sqrt(ids.size()));
    if (ssize < ids.size()) ssize = (int)ids.size();
    
    // We need three temporary storage arrays:
    std::vector<double> mindist(num_obs), bestd(num_obs), tempd(num_obs), temp;
    
    int bestid;
    // First mean is chosen by having the smallest distance sum to all others.
    {
        double best = DBL_MAX;
        for (int i=0; i<ids.size(); ++i) {
            double sum = 0, d;
            for (int j=0; j<ids.size(); ++j) {
                d = getDistance(ids[i], ids[j]);
                sum += d;
                tempd[ids[j]] =  d;
            }
            if(sum < best) {
                best = sum;
                bestid = ids[i];
                // Swap mindist and newd:
                temp = mindist;
                mindist = tempd;
                tempd = temp;
            }
        }
        medids.push_back(bestid);
        medids_set.insert(bestid);
    }
    
    // Subsequent means optimize the full criterion.
    for(int i = 1; i < k; i++) {
        double best = DBL_MAX;
        bestid = -1; // bestid.unset();
        for (int j=0; j<ids.size(); ++j) {
            if(medids_set.find(ids[j]) != medids_set.end()) {
                continue;
            }
            double sum = 0., v;
            for (int k=0; k<ids.size(); ++k) {
                v = std::min(getDistance(ids[j], ids[k]), mindist[ids[k]]);
                sum += v;
                tempd[ids[k]]  =  v;
            }
            if(sum < best) {
                best = sum;
                bestid = ids[j];
                // Swap bestd and newd:
                temp = bestd;
                bestd = tempd;
                tempd = temp;
            }
        }
        if(bestid == -1) {
          //throw new AbortException("No medoid found that improves the criterion function?!? Too many infinite distances.");
            return std::vector<int>();
        }
        medids.push_back(bestid);
        medids_set.insert(bestid);
        // Swap bestd and mindist:
        temp = bestd;
        bestd = mindist;
        mindist = temp;
    }
    mindist.clear();
    bestd.clear();
    tempd.clear();
    return medids;
}

// Assign each object to the nearest cluster, return the cost.
double PAM::assignToNearestCluster(std::vector<int> &means) {
    double cost = 0.;
    for (int i=0; i<ids.size(); ++i) {
        double mindist = DBL_MAX, mindist2 = DBL_MAX;
        int minindx = -1;
        for (int j=0; j<means.size(); ++j) {
            double dist = getDistance(ids[i], means[j]);
            if(dist < mindist) {
                mindist2 = mindist;
                minindx = j;
                mindist = dist;
            }
            else if(dist < mindist2) {
              mindist2 = dist;
            }
        }
        if(minindx < 0) {
            // "Too many infinite distances. Cannot assign objects."
            return 0;
        }
        int iditer = ids[i];
        assignment[iditer]  = minindx;
        nearest[iditer] = mindist;
        second[iditer] = mindist2;
        cost += mindist;
    }
    return cost;
}

// Compute the reassignment cost of one swap.
double PAM::computeReassignmentCost(int h, int mnum) {
    double cost = 0.;
    // Compute costs of reassigning other objects j:
    for (int i=0; i<ids.size(); ++i) {
        int j = ids[i];
        if (h == j) {
            continue;
        }
        // distance(j, i) to nearest medoid
        double distcur = nearest[j];
        // distance(j, h) to new medoid
        double dist_h = getDistance(h, j);
        // Check if current medoid of j is removed:
        if(assignment[j] == mnum) {
            // Case 1b: j switches to new medoid, or to the second nearest:
            cost += std::min(dist_h, second[j]) - distcur;
        }
        else if(dist_h < distcur) {
            // Case 1c: j is closer to h than its current medoid
            cost += dist_h - distcur;
        } // else Case 1a): j is closer to i than h and m, so no change.
    }
    return cost;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FastPAM::FastPAM(int num_obs, DistMatrix* dist_matrix, int k, int maxiter, double fasttol)
: PAM(num_obs, dist_matrix, k, maxiter), fasttol(fasttol)
{
    //super(distQ, ids, assignment);
    fastswap = 1 - fasttol;
}

FastPAM::~FastPAM() {
    
}

// Run k-medoids
double FastPAM::run(std::vector<int>& medoids, int maxiter) {
    //  return final cost
    int k = (int)medoids.size();
    // Initial assignment to nearest medoids
    // TODO: reuse distance information, from the build phase, when possible?
    double tc = assignToNearestCluster(medoids);
    // iteration 0 cost = tc
    // start PAM iteration
    int fastswaps = 0; // For statistics
    // Swap phase
    std::vector<int> bestids(k);
    int bestid;
    
    std::vector<double> best(k), cost(k);
    
    int iteration = 0;
    while(iteration < maxiter || maxiter <= 0) {
        ++iteration;
        findBestSwaps(medoids, bestids, best, cost);
        // Convergence check
        int min = argmin(best);
        if  (!(best[min] < -1e-12 * tc)) {
            break; // Converged
        }
        // Update values for new medoid.
        while(min >= 0 && best[min] < -1e-12 * tc) {
            //updateAssignment(medoids, bestids.assignVar(min, bestid), min);
            bestid = bestids[min];
            updateAssignment(medoids, bestid, min);
            
            tc += best[min];
            best[min] = DBL_MAX; // Deactivate
            
            // Find next candidate:
            while((min = argmin(best)) >= 0 && best[min] < -1e-12 * tc) {
                bestid = bestids[min];
                // Compare object to its own medoid.
                if (medoids[assignment[bestid] & 0x7FFF]  == bestid) {
                    best[min] = DBL_MAX; // Deactivate
                    continue; // This is a medoid.
                }
                double hdist = nearest[bestid]; // Current cost
                // hdist is the cost we get back by making the non-medoid h medoid.
                double c = computeReassignmentCost(bestid, min) - hdist;
                if(c <= best[min] * fastswap) {
                    best[min] = c;
                    ++fastswaps;
                    break;
                }
                best[min] = DBL_MAX; // Deactivate
            }
        }
        // ".iteration-" + iteration + ".cost", tc
    }
    // // TODO: we may have accumulated some error on tc.
    // ".iterations", iteration
    // ".fast-swaps", fastswaps
    // ".final-cost", tc
    
    // Cleanup
    for(int i=0; i<ids.size(); ++i) {
        int j = ids[i];
        assignment[j]  = assignment[j] & 0x7FFF;
    }
    return tc;
}

// LAB initialization
std::vector<int> FastPAM::initialMedoids() {
    std::vector<int> medids;
    std::set<int> medids_set;
    // O(sqrt(n)) sample if k^2 < n.
    int ssize = 10 + (int)ceil(sqrt(ids.size()));
    if (ssize > ids.size()) ssize = (int)ids.size();
    
    // We need three temporary storage arrays:
    std::vector<double> mindist(num_obs, DBL_MIN), bestd(num_obs), tempd(num_obs), tmp;
    std::vector<int> sample = ids;
    int range = (int)sample.size();
    
    shuffle(sample, ssize, range);
    
    // First mean is chosen by having the smallest distance sum to all others.
    {
        double best = DBL_MAX;
        int bestoff = -1;
        
        for (int i=0; i<ssize; ++i) {
            double sum = 0, d;
            tempd.clear();
            for (int j=0; j<ssize; ++j) {
                d = getDistance(sample[i], sample[j]);
                sum += d;
                tempd[sample[j]] = d;
            }
            if(sum < best) {
                best = sum;
                bestoff = i;
                // Swap mindist and newd:
                tmp = mindist;
                mindist = tempd;
                tempd = tmp;
            }
        }
        medids.push_back(sample[bestoff]);
        medids_set.insert(sample[bestoff]);
        //sample.swap(bestoff, --range);
        int tmp = sample[--range];
        sample[range] = sample[bestoff];
        sample[bestoff] = tmp;
    }
    // First one was just chosen.
    // Subsequent means optimize the full criterion.
    // Choosing initial medoids", k,
    while(medids.size() < k) {
        // New sample
        ssize = range < ssize ? range : ssize;
        shuffle(sample, ssize, range);
        double best = DBL_MAX;
        int bestoff = -1;
        for (int i=0; i<ssize; ++i)  {
            if (medids_set.find(sample[i]) != medids_set.end()) {
                continue;
            }
            double sum = 0., v;
            tempd.clear();
            for (int j=0; j<ssize; ++j) {
                double prev = getMinDist(sample[j], medids, mindist);
                if(prev == 0) {
                    continue;
                }
                v = std::min(getDistance(sample[i], sample[j]), prev);
                sum += v;
                tempd[sample[j]] =  v;
            }
            if(sum < best) {
                best = sum;
                bestoff = i;
                // Swap bestd and newd:
                tmp = bestd;
                bestd = tempd;
                tempd = tmp;
            }
        }
        if(bestoff < 0) {
            //"No medoid found that improves the criterion function?!? Too many infinite distances."
            return medids;
        }
        medids.push_back(sample[bestoff]);
        medids_set.insert(sample[bestoff]);
        // sample.swap(bestoff, --range);
        int tmp_val = sample[--range];
        sample[range] = sample[bestoff];
        sample[bestoff] = tmp_val;
        // Swap bestd and mindist:
        tmp = bestd;
        bestd = mindist;
        mindist = tmp;
    }
    
    mindist.clear();
    bestd.clear();
    tempd.clear();
    return medids;
}


void FastPAM::findBestSwaps(std::vector<int> &medoids, std::vector<int> &bestids, std::vector<double> &best, std::vector<double> &cost)
{
    size_t n_medoids = medoids.size();
    
    best.resize(n_medoids, DBL_MAX);
    cost.resize(n_medoids, 0);
    
    // Iterate over all non-medoids:
    for (int k=0; k<ids.size(); ++k) {
        int h = ids[k];
        // Compare object to its own medoid.
        if (medoids[assignment[h]&0x7FFF] == h) {
            continue; // This is a medoid.
        }
        
        // The cost we get back by making the non-medoid h medoid.
        for (int j=0; j<n_medoids; ++j) cost[j] = -nearest[h];

        computeReassignmentCost(h, cost);
        
        // Find the best possible swap for each medoid:
        for(int i = 0; i < cost.size(); i++) {
            double costi = cost[i];
            if(costi < best[i]) {
                best[i] = costi;
                bestids[i] = h;
            }
        }
    }
}

bool FastPAM::isMedoid(int id) {
    return false;
}

void FastPAM::computeReassignmentCost(int h, std::vector<double> &cost) {
    // h: Current object to swap with any medoid.
    // cost: Cost aggregation array, must have size k
    
    // Compute costs of reassigning other objects j:
    for (int j=0; j<ids.size(); ++j) {
        if (h== j) {
            continue;
        }
        // distance(j, i) for pi == pj
        double distcur = nearest[j];
        //  distance(j, o) to second nearest / possible reassignment
        double distsec = second[j];
        // distance(j, h) to new medoid
        double dist_h = getDistance(h, j);
        // Case 1b: j switches to new medoid, or to the second nearest:
        int pj = assignment[j] & 0x7FFF;
        
        cost[pj] += fmin(dist_h, distsec) - distcur;
        if(dist_h < distcur) {
            double delta = dist_h - distcur;
            // Case 1c: j is closer to h than its current medoid
            for(int pi = 0; pi < pj; pi++) {
              cost[pi] += delta;
            }
            for(int pi = pj + 1; pi < cost.size(); pi++) {
              cost[pi] += delta;
            }
        } // else Case 1a): j is closer to i than h and m, so no change.
    }
}


double FastPAM::computeReassignmentCost(int h, int mnum)
{
    double cost = 0.;
    // Compute costs of reassigning other objects j:
    for (int i=0; i<ids.size(); ++i) {
        int j = ids[i];
        if (h == j)  {
            continue;
        }
        // distance(j, i) to nearest medoid
        double distcur = nearest[j];
        // distance(j, h) to new medoid
        double dist_h = getDistance(h, j);
        // Check if current medoid of j is removed:
        if((assignment[j] & 0x7FFF) == mnum) {
            // distance(j, o) to second nearest / possible reassignment
            // Case 1b: j switches to new medoid, or to the second nearest:
            cost += fmin(dist_h, second[j]) - distcur;
        }
        else if(dist_h < distcur) {
            // Case 1c: j is closer to h than its current medoid
            cost += dist_h - distcur;
        } // else Case 1a): j is closer to i than h and m, so no change.
    }
    return cost;
}

double FastPAM::assignToNearestCluster(std::vector<int> &means) {
    // means Object centroids
    // return Assignment cost
    double cost = 0.;
    for (int j=0; j<ids.size(); ++j) {
        int iditer = ids[j];
        double mindist = DBL_MAX, mindist2 = DBL_MAX;
        int minindx = -1, minindx2 = -1;
        for (int h=0; h< means.size(); ++h) {
            double dist = getDistance(iditer, means[h]);
            if(dist < mindist) {
                minindx2 = minindx;
                mindist2 = mindist;
                minindx = h;// miter.getOffset();
                mindist = dist;
            } else if(dist < mindist2) {
                minindx2 = h;//miter.getOffset();
                mindist2 = dist;
            }
        }
        if(minindx < 0) {
            //throw new AbortException("Too many infinite distances. Cannot assign objects.");
            return 0;
        }
        //assignment.put(iditer, minindx | (minindx2 << 16));
        assignment[iditer]  = minindx | (minindx2 << 16);
        //nearest.put(iditer, mindist);
        nearest[iditer] = mindist;
        //second.put(iditer, mindist2);
        second[iditer] = mindist2;
        
        cost += mindist;
    }
    return cost;
}

int FastPAM::argmin(const std::vector<double> &best) {
    double min = DBL_MAX;
    int ret = -1;
    for(int i = 0; i < best.size(); i++) {
        double v = best[i];
        if (v < min) {
            min = v;
            ret = i;
        }
    }
    return ret;
}

void FastPAM::updateAssignment(std::vector<int>& medoids, int h, int m) {
    // Medoids set;
    // miter Medoid iterator
    // h New medoid
    // m Position of replaced medoid
    
    // The new medoid itself.
    medoids[m] = h;
    
    double hdist = nearest[h];
    nearest[h] = 0;
    
    int olda = assignment[h];
    
    // In the high short, we store the second nearest center!
    if((olda & 0x7FFF) != m) {
        assignment[h]  =  m | ((olda & 0x7FFF) << 16);
        second[h] = hdist;
    } else {
        assignment[h] =  m | (olda & 0x7FFF0000);
    }
    // assert (DBIDUtil.equal(h, miter.seek(m)));
    // Compute costs of reassigning other objects j:
    for (int i=0; i<ids.size(); ++i) {
        int j = ids[i];
        if (h == j) {
            continue;
        }
        // distance(j, i) for pi == pj
        double distcur = nearest[j];
        // distance(j, o) to second nearest / possible reassignment
        double distsec = second[j];
        // distance(j, h) to new medoid
        double dist_h = getDistance(h, j);
        // Case 1b: j switches to new medoid, or to the second nearest:
        int pj = assignment[j];
        int po = (unsigned int)pj >> 16;
        pj &= 0x7FFF; // Low byte is the old nearest cluster.
        if(pj == m) { // Nearest medoid is gone.
            if(dist_h < distsec) { // Replace nearest.
                nearest[j] = dist_h;
                assignment[j] = m | (po << 16);
            } else {// Second is new nearest.
                 nearest[j] = distsec;
                // Find new second nearest.
                assignment[j] = po | (updateSecondNearest(j, medoids, m, dist_h, po) << 16);
            }
        }
        else { // Nearest medoid not replaced
            if(dist_h < distcur) {
                nearest[j] = dist_h;
                second[j] = distcur;
                assignment[j] = m | (pj << 16);
            } else if(po == m) { // Second was replaced.
              assignment[j] = pj | (updateSecondNearest(j, medoids, m, dist_h, pj) << 16);
            } else if(dist_h < distsec) {
              second[j] = dist_h;
              assignment[j] = pj | (m << 16);
            }
        }
    }
}

int FastPAM::updateSecondNearest(int j, std::vector<int> &medoids, int h, double dist_h, int n)
{
    double sdist = dist_h;
    int sbest = h;
    for (int i=0; i<medoids.size(); ++i) {
        if (i != h && i != n) {
            double d = getDistance(j, medoids[i]);
            if(d < sdist) {
                sdist = d;
                sbest = i;
            }
        }
    }
    second[j] =  sdist;
    return sbest;
}
