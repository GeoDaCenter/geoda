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
std::vector<int> BUILD::run(const std::vector<int>& ids, int k)
{
    int nn = (int) ids.size();
    std::vector<int> medids;
    std::set<int> medids_set;
    
    // O(sqrt(n)) sample if k^2 < n.
    int ssize = 10 + (int)ceil(std::sqrt((double)nn));
    if (ssize < nn) ssize = nn;
    
    // We need three temporary storage arrays:
    std::vector<double> mindist(nn), bestd(nn), tempd(nn), temp;
    
    int bestid;
    // First mean is chosen by having the smallest distance sum to all others.
    {
        double best = DBL_MAX;
        for (int i=0; i< nn; ++i) {
            double sum = 0, d;
            for (int j=0; j<nn; ++j) {
                d = dist->getDistance(ids[i], ids[j]);
                sum += d;
                tempd[ ids[j] ] =  d;
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
        for (int j=0; j<nn; ++j) {
            if(medids_set.find( ids[j] ) != medids_set.end()) {
                continue;
            }
            double sum = 0., v;
            for (int k=0; k<nn; ++k) {
                v = std::min(dist->getDistance(ids[j], ids[k]), mindist[ids[k]]);
                sum += v;
                tempd[ ids[k] ]  =  v;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Partial Fisher-Yates shuffle.
void LAB::shuffle(std::vector<int> &samples, int ssize, int end) {
    ssize = ssize < end ? ssize : end; // Guard for choosing from tiny sets
    
    for(int i = 1; i < ssize; i++) {
        int a = i-1, b = i+random.nextInt(end - i);
        int tmp = samples[b];
        samples[b] = samples[a];
        samples[a] = tmp;
    }
}

//Get the minimum distance to previous medoids.
double LAB::getMinDist(int j, std::vector<int>& medids, std::vector<double>& mindist)
{
    double prev = mindist[j];
    if (prev == DBL_MIN) {
        prev = DBL_MAX;
        for (int i=0; i<medids.size(); ++i) {
            double d = dist->getDistance(j, medids[i]);
            prev = d < prev ? d : prev;
        }
        mindist[j] = prev;
    }
    return prev;
}

std::vector<int> LAB::run(const std::vector<int>& ids, int k)
{
    int nn = (int) ids.size();
    std::vector<int> medids;
    std::set<int> medids_set;
    
    // O(sqrt(n)) sample if k^2 < n.
    int ssize = 10 + (int)ceil(std::sqrt((double)nn));
    if (ssize > nn) ssize = nn;
    
    // We need three temporary storage arrays:
    std::vector<double> mindist(nn, DBL_MIN), bestd(nn), tempd(nn, DBL_MIN), tmp;
    std::vector<int> sample(nn);
    for (int i=0; i<nn; ++i) sample[i] = ids[i];
    int range = (int)sample.size();
    
    shuffle(sample, ssize, range);
    
    // First mean is chosen by having the smallest distance sum to all others.
    {
        double best = DBL_MAX;
        int bestoff = -1;
        
        for (int i=0; i<ssize; ++i) {
            double sum = 0, d;
            for (int j=0; j<tempd.size(); ++j) tempd[j] = DBL_MIN;
            for (int j=0; j<ssize; ++j) {
                d = dist->getDistance(sample[i], sample[j]);
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
            //tempd.clear();
            for (int j=0; j<tempd.size(); ++j) tempd[j] = DBL_MIN;
            for (int j=0; j<ssize; ++j) {
                double prev = getMinDist(sample[j], medids, mindist);
                if(prev == 0) {
                    continue;
                }
                v = std::min(dist->getDistance(sample[i], sample[j]), prev);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PAM::PAM(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init, int k, int maxiter, const std::vector<int>& _ids)
: num_obs(num_obs), dist_matrix(dist_matrix), initializer(init),
k(k), maxiter(maxiter), ids(_ids)
{
    if (initializer == NULL) {
        // set default to PAM classic initializer
        initializer = new BUILD(dist_matrix);
    }
    
    if (ids.empty()) {
        // using sequential enumeric numbers as ids
        ids.resize(num_obs);
        for (int i=0; i<num_obs; ++i) {
            ids[i] = i;
        }
    }
}

PAM::~PAM() {
    
}

double PAM::run() {
    // using sequential ids for PAM internally
    std::vector<int> seq_ids(num_obs);
    for (int i=0; i<num_obs; ++i) {
        seq_ids[i] = i;
    }
    
    // Initialization
    medoids = initializer->run(seq_ids, k);
    
    // Setup cluster assignment store
    assignment.resize(num_obs, -1);
    nearest.resize(num_obs, -1);
    second.resize(num_obs, -1);
    
    // Run partition
    double cost = run(medoids, maxiter);
    
    return cost;
}

std::vector<int> PAM::getMedoids()
{
    return medoids;
}

std::vector<int> PAM::getResults()
{
    // Get results, using ids instead of seq_ids
    // cluster starts counting from 1 instead of 0
    std::vector<int> cluster_result(num_obs, 0);
    for (int i=0; i<num_obs; ++i) {
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
        for (int h=0; h<num_obs; ++h) {
            // Compare object to its own medoid.
            if (medoids[assignment[h]] == h) {
                continue; // This is a medoid
            }
            double hdist = nearest[h]; // Current cost of h.
            if (hdist <= 0.) {
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


double PAM::getDistance(int i, int j) {
    return dist_matrix->getDistance(i, j);
}

// Assign each object to the nearest cluster, return the cost.
double PAM::assignToNearestCluster(std::vector<int> &means) {
    double cost = 0.;
    for (int i=0; i<num_obs; ++i) {
        double mindist = DBL_MAX, mindist2 = DBL_MAX;
        int minindx = -1;
        for (int j=0; j<means.size(); ++j) {
            double dist = getDistance(i, means[j]);
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
        int iditer = i;
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
    for (int j=0; j < num_obs; ++j) {
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

FastPAM::FastPAM(int num_obs, DistMatrix* dist_matrix, PAMInitializer* init,
                 int k, int maxiter, double fasttol,const std::vector<int>& _ids)
: PAM(num_obs, dist_matrix, init, k, maxiter, _ids), fasttol(fasttol)
{
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
    for(int i=0; i<num_obs; ++i) {
        assignment[i]  = assignment[i] & 0x7FFF;
    }
    return tc;
}


void FastPAM::findBestSwaps(std::vector<int> &medoids, std::vector<int> &bestids, std::vector<double> &best, std::vector<double> &cost)
{
    size_t n_medoids = medoids.size();
    
    best.resize(n_medoids, DBL_MAX);
    cost.resize(n_medoids, 0);
    
    // Iterate over all non-medoids:
    for (int h=0; h<num_obs; ++h) {
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
    for (int j=0; j<num_obs; ++j) {
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
        
        cost[pj] += std::min(dist_h, distsec) - distcur;
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
    for (int j=0; j<num_obs; ++j) {
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
            cost += std::min(dist_h, second[j]) - distcur;
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
    for (int j=0; j<num_obs; ++j) {
        int iditer = j;
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
    for (int i=0; i<num_obs; ++i) {
        int j = i;
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

std::vector<int> PAMUtils::randomSample(Xoroshiro128Random& rand,
                                        int samplesize, int n,
                                        const std::vector<int>& previous)
{
    if (previous.empty()) {
        return rand.randomSample(samplesize, n);
    }
    int cnt = 0;
    std::vector<int> sample(samplesize);
    unordered_map<int, bool> sample_dict;
    unordered_map<int, bool>::iterator it;
    for (int i=0; i<previous.size(); ++i) {
        if (sample_dict.find(previous[i]) == sample_dict.end()) {
            sample[cnt++] = previous[i];
            sample_dict[ previous[i] ] = true;
        }
    }
    
    std::vector<int> rest_rnds = rand.randomSample(samplesize - (int)previous.size(), n);
    for (int i=0; i<rest_rnds.size(); ++i) {
        if (sample_dict.find(rest_rnds[i]) == sample_dict.end()) {
            sample[cnt++] = rest_rnds[i];
            sample_dict[ rest_rnds[i] ] = true;
        }
    }
    
    // If these two were not disjoint, we can be short of the desired size!
    if (sample_dict.size() < samplesize) {
        // Draw a large enough sample to make sure to be able to fill it now.
        // This can be less random though, because the iterator may impose an
        // order; but this is a rare code path.
        std::vector<int> rnd_picks = rand.randomSample(samplesize, n);
        for (int i=0; i<samplesize && sample_dict.size() < samplesize; ++i) {
            if (sample_dict.find(rnd_picks[i]) == sample_dict.end()) {
                sample[cnt++] = rnd_picks[i];
                sample_dict[ rnd_picks[i] ] = true;
            }
        }
    }
    return sample;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CLARA::CLARA(int num_obs, DistMatrix *dist_matrix, PAMInitializer* init, int k, int maxiter, int numsamples, double sampling, bool independent, int seed)
:  num_obs(num_obs), dist_matrix(dist_matrix), initializer(init), k(k),
maxiter(maxiter), sampling(sampling), numsamples(numsamples), keepmed(!independent), random(seed)
{
    
}

CLARA::~CLARA() {
    
}

double CLARA::run()
{
    int samplesize = sampling <= 1 ? sampling * num_obs : sampling;
    if (samplesize > num_obs ) samplesize = num_obs;
    
    if(samplesize < 3 * k) {
        //"The sampling size is set to a very small value,
        //it should be much larger than k.");
    }
    
    double best = DBL_MAX;
    
    for(int j = 0; j < numsamples; j++) {
        std::vector<int> rids;
        
        if (keepmed)
            rids = PAMUtils::randomSample(random, samplesize, num_obs, bestmedoids);
        else
            rids = PAMUtils::randomSample(random, samplesize, num_obs);
        
        //  run PAM using rids
        //PAM pam(samplesize, dist_matrix,
        dist_matrix->setIds(rids);
        PAM pam(samplesize, dist_matrix, initializer, k, maxiter);
        double score = pam.run();
        
        // allow to work on full dist matrix
        dist_matrix->setIds(std::vector<int>());
        
        std::vector<int> assignment;
        std::vector<int> medoids = pam.getMedoids();
        std::vector<int> r_assignment = pam.getAssignement();
        score += assignRemainingToNearestCluster(medoids, rids, r_assignment, assignment);
        
        if(score < best) {
            best = score;
            bestclusters = assignment;
            bestmedoids = medoids;
            for (int k=0; k<bestmedoids.size(); ++k ) {
                bestmedoids[k] = rids[ bestmedoids[k] ];
            }
        }
    }
    return best;
}

std::vector<int> CLARA::getResults()
{
    // Get results
    std::vector<int> cluster_result(num_obs, 0);
    for (int i=0; i<num_obs; ++i) {
        cluster_result[i] = bestclusters[i] + 1;
    }
    return cluster_result;
}

double CLARA::assignRemainingToNearestCluster(std::vector<int>& means, std::vector<int>& rids, std::vector<int>& r_assignment, std::vector<int>& assignment)
{
    unordered_map<int, bool> rid_dict;
    assignment.resize(num_obs);
    for (int j=0; j<r_assignment.size(); ++j) {
        assignment[rids[j]] = r_assignment[j];
        rid_dict[rids[j]] = true;
    }
    
    double distsum = 0.;
    for (int j=0; j< num_obs; ++j) {
        if (rid_dict.find(j) != rid_dict.end()) {
            continue;
        }
        double mindist = DBL_MAX;
        int minIndex = 0;
        for (int i=0; i<means.size(); ++i) {
            double dist = dist_matrix->getDistance(rids[means[i]], j);
            if (dist < mindist)  {
                minIndex = i;
                mindist = dist;
            }
        }
        distsum += mindist;
        assignment[j] = minIndex;
    }
    return distsum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FastCLARA::FastCLARA(int num_obs, DistMatrix *dist_matrix, PAMInitializer* init, int k, int maxiter, double fasttol, int numsamples, double sampling, bool independent, int seed)
:  CLARA(num_obs, dist_matrix, init, k, maxiter, numsamples, sampling, independent, seed),
fasttol(fasttol)
{}

// Using LAB + PAM on random samples
double FastCLARA::run()
{
    int samplesize = sampling <= 1 ? sampling * num_obs : sampling;
    if (samplesize > num_obs ) samplesize = num_obs;
    
    if(samplesize < 3 * k) {
        //"The sampling size is set to a very small value,
        //it should be much larger than k.");
    }
    
    double best = DBL_MAX;
    
    for(int j = 0; j < numsamples; j++) {
        std::vector<int> rids;
        
        if (keepmed)
            rids = PAMUtils::randomSample(random, samplesize, num_obs, bestmedoids);
        else
            rids = PAMUtils::randomSample(random, samplesize, num_obs);
        
        //  run PAM using rids
        //PAM pam(samplesize, dist_matrix,
        dist_matrix->setIds(rids);
        FastPAM pam(samplesize, dist_matrix, initializer, k, maxiter, fasttol);
        double score = pam.run();
        
        // allow to work on full dist matrix
        dist_matrix->setIds(std::vector<int>());
        
        std::vector<int> assignment;
        std::vector<int> medoids = pam.getMedoids();
        std::vector<int> r_assignment = pam.getAssignement();
        score += assignRemainingToNearestCluster(medoids, rids, r_assignment, assignment);
        
        if(score < best) {
            best = score;
            bestclusters = assignment;
            bestmedoids = medoids;
            for (int k=0; k<bestmedoids.size(); ++k ) {
                bestmedoids[k] = rids[ bestmedoids[k] ];
            }
        }
    }
    return best;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CLARANS::CLARANS(int num_obs, DistMatrix *dist_matrix, int k, int numlocal, double maxneighbor, int seed)
: num_obs(num_obs), dist_matrix(dist_matrix), k(k),
numlocal(numlocal), maxneighbor(maxneighbor), random(seed)
{
    
}

CLARANS::~CLARANS() {
    
}

double CLARANS::run() {
    if (k >= num_obs / 2) {
        // "A very large k was chosen. This implementation is not optimized for this case."
    }
    // Number of retries, relative rate, or absolute count:
    int retries = (int)ceil(maxneighbor < 1 ? maxneighbor * k * (num_obs - k) : maxneighbor);
    
    int cand  = 0;
    
    // Setup cluster assignment store
    Assignment best(k, num_obs, dist_matrix), curr(k, num_obs, dist_matrix),
               scratch(k, num_obs, dist_matrix), tmp;
    
    // 1. initialize
    double bestscore = DBL_MAX;
    for(int i = 0; i < numlocal; i++) {
        // 2. choose random initial medoids
        curr.medoids = PAMUtils::randomSample(random, k, num_obs);
        
        //curr.medoids[0]=431;curr.medoids[1] = 211;curr.medoids[2]=293;curr.medoids[3]=10;
        
        // Cost of initial solution
        double total = curr.assignToNearestCluster();
        
        // 3. Set j to 1
        int j = 1;
        step: while (j < retries) {
            // 4 part a. choose a random non-medoid (~ neighbor in G):
            for (int r = 0;; r++) {
                // // Random point
                cand = random.nextInt(num_obs);
                if (curr.nearest[cand] > 0) {
                    break; // Good: not a medoid.
                }
                // We may have many duplicate points
                if (curr.second[cand] ==  0) {
                    ++j; // Cannot yield an improvement if we are metric.
                    goto step; // NOTE: this should go back to top while()
                } else if (!curr.hasMedoid(cand)) {
                    // Probably not a good candidate, but try nevertheless
                    break;
                }
                if (r >= 1000) {
                    // "Failed to choose a non-medoid in 1000 attempts. Choose k << N."
                    return 0;
                }
                // else: this must be the medoid.
            }
            // 4 part b. choose a random medoid to replace:
            int otherm = random.nextInt(k);
            // 5. check lower cost
            double cost = curr.computeCostDifferential(cand, otherm, scratch);
            if(!(cost < -1e-12 * total)) {
                ++j; // 6. try again
                continue;
            }
            total += cost; // cost is negative!
            // Swap:
            tmp = curr;
            curr = scratch;
            scratch = tmp;
            j = 1;
        }
        // New best:
        if(total < bestscore) {
            // Swap:
            Assignment tmp = curr;
            curr = best;
            best = tmp;
            bestscore = total;
        }
    }
    
    bestmedoids = best.medoids;
    bestclusters = best.assignment;
    return bestscore;
}

std::vector<int> CLARANS::getResults() {
    // Get results
    std::vector<int> cluster_result(num_obs, 0);
    for (int i=0; i<num_obs; ++i) {
        cluster_result[i] = bestclusters[i] + 1;
    }
    return cluster_result;
}


Assignment::Assignment(int k, int num_obs, DistMatrix* dist_matrix)
:  k(k), num_obs(num_obs), dist_matrix(dist_matrix), medoids(k),
assignment(num_obs), nearest(num_obs), secondid(num_obs), second(num_obs)
{
}

Assignment &Assignment::operator=(const Assignment &other) {
    num_obs = other.num_obs;
    dist_matrix = other.dist_matrix;
    medoids_dict = other.medoids_dict;
    medoids = other.medoids;
    assignment = other.assignment;
    nearest = other.nearest;
    secondid = other.secondid;
    second = other.second;
    return *this;
}

// h Current object to swap with any medoid.
// mnum Medoid number to swap with h.
// Scratch assignment to fill.
// return Cost change
double Assignment::computeCostDifferential(int h, int mnum, Assignment& scratch)
{
    // Update medoids of scratch copy.
    scratch.medoids = medoids;
    scratch.medoids[mnum] = h;
    double cost = 0;
    // Compute costs of reassigning other objects j:
    for (int j=0; j<num_obs; ++j) {
        if (h == j) {
            scratch.recompute(j, mnum, 0, -1, DBL_MAX);
            continue;
        }
        // distance(j, i) to nearest medoid
        double distcur = nearest[j];
        // distance(j, h) to new medoid
        double dist_h = dist_matrix->getDistance(h, j);
        // current assignment of j
        int jcur = assignment[j];
        // Check if current medoid of j is removed:
        if(jcur == mnum) {
            // distance(j, o) to second nearest / possible reassignment
            double distsec = second[j];
            // Case 1b: j switches to new medoid, or to the second nearest:
            if (dist_h < distsec) {
                cost += dist_h - distcur;
                scratch.assignment[j] = mnum;
                scratch.nearest[j] = dist_h;
                scratch.second[j] = distsec;
                scratch.secondid[j] = jcur;
            } else {
                // Second nearest is the new assignment.
                cost += distsec - distcur;
                // We have to recompute, because we do not know the true new second
                // nearest.
                scratch.recompute(j, mnum, dist_h, jcur, distsec);
            }
        }
        else if(dist_h < distcur) {
            // Case 1c: j is closer to h than its current medoid
            // and the current medoid is not removed (jcur != mnum).
            cost += dist_h - distcur;
            // Second nearest is the previous assignment
            scratch.assignment[j] = mnum;
            scratch.nearest[j] = dist_h;
            scratch.second[j] = distcur;
            scratch.secondid[j] = jcur;
        }
        else { // else Case 1a): j is closer to i than h and m, so no change.
            int jsec = secondid[j];
            double distsec = second[j];
            // Second nearest is still valid.
            if(jsec != mnum && distsec <= dist_h) {
                scratch.assignment[j] = jcur;
                scratch.nearest[j] = distcur;
                scratch.secondid[j] = jsec;
                scratch.second[j] = distsec;
            } else {
                scratch.recompute(j, jcur, distcur, mnum, dist_h);
            }
        }
    }
    return cost;
}

//Recompute the assignment of one point.
// id Point id
// mnum Medoid number for known distance
// known Known distance
// return cost
double Assignment::recompute(int id, int mnum, double known, int snum, double sknown) {
    double mindist = mnum >= 0 ? known : DBL_MAX, mindist2 = DBL_MAX;
    int minIndex = mnum, minIndex2 = -1;
    for(int i = 0; i < medoids.size(); i++) {
        if(i == mnum) {
            continue;
        }
        double dist = i == snum ? sknown : dist_matrix->getDistance(id, medoids[i]);
        if (id == medoids[i] || dist < mindist) {
            minIndex2 = minIndex;
            mindist2 = mindist;
            minIndex = i;
            mindist = dist;
        } else if(dist < mindist2) {
            minIndex2 = i;
            mindist2 = dist;
        }
    }
    if(minIndex < 0) {
        // "Too many infinite distances. Cannot assign objects.");
        return 0;
    }
    assignment[id] = minIndex;
    nearest[id] = mindist;
    secondid[id] = minIndex2;
    second[id] = mindist2;
    return mindist;
}

// Assign each point to the nearest medoid.
// return Assignment cost
double Assignment::assignToNearestCluster() { 
    double cost = 0.;
    for (int i=0; i< num_obs; ++i) {
        cost += recompute(i, -1, DBL_MAX, -1, DBL_MAX);
    }
    return cost;
}

bool Assignment::hasMedoid(int cand) { 
    if (medoids_dict.empty()) {
        for (int i=0; i<medoids.size(); ++i) {
            medoids_dict[medoids[i]] = true;
        }
    }
    return medoids_dict.find(cand) != medoids_dict.end();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FastCLARANS::FastCLARANS(int num_obs, DistMatrix *dist_matrix, int k, int numlocal, double maxneighbor, int seed)
: CLARANS(num_obs, dist_matrix, k, numlocal, maxneighbor, seed)
{
    
}

double FastCLARANS::run() {
    if(k * 2 >= num_obs) {
      // Random sampling of non-medoids will be slow for huge k
      //LOG.warning("A very large k was chosen. This implementation is not optimized for this case.");
    }
    // Number of retries, relative rate, or absolute count:
    int retries = (int)ceil(maxneighbor < 1 ? maxneighbor * (num_obs - k) : maxneighbor);
    // We will be using this to avoid sampling the same points twice.
    std::vector<int> subsampler(num_obs);
    for (int i=0; i<num_obs; ++i) subsampler[i] = i;
    int cand = 0, tmp, rnd;
    
    // Setup cluster assignment store
    FastAssignment best(k, num_obs, dist_matrix);
    FastAssignment curr(k, num_obs, dist_matrix);
    
    // 1. initialize
    double bestscore = DBL_MAX;
    for(int i = 0; i < numlocal; i++) {
        // 2. choose random initial medoids
        curr.medoids = PAMUtils::randomSample(random, k, num_obs);
        
        //curr.medoids[0]=431;curr.medoids[1] = 211;curr.medoids[2]=293;curr.medoids[3]=10;
        
        //  Cost of initial solution:
        double total = curr.assignToNearestCluster();
        
        // 3. Set j to 1.
        int j = 1;
        step: while(j < retries) {
            // 4 part a. choose a random non-medoid (~ neighbor in G):
            for (int r = 0; r < num_obs; r++) {
                // Random point
                rnd = random.nextInt(num_obs - r) + r;
                // Fisher-Yates shuffle to avoid sampling the same points twice!
                tmp = subsampler[r];
                subsampler[r] = subsampler[rnd];
                subsampler[rnd] = tmp;
                
                cand = subsampler[r]; // Random point
                if(curr.nearest[cand] > 0) {
                    break; // Good: not a medoid.
                }
                // We may have many duplicate points
                if(curr.second[cand] == 0) {
                    ++j; // Cannot yield an improvement if we are metric.
                    goto step;
                } else if( !curr.hasMedoid(cand)) {
                    // Probably not a good candidate, but try nevertheless
                    break;
                }
                if(r >= 1000) {
                    //throw new AbortException("Failed to choose a non-medoid in 1000 attempts. Choose k << N.");
                    return 0;
                }
                // else: this must be the medoid.
            }
            // 5. check lower cost
            double cost = curr.computeCostDifferential(cand);
            if(!(cost < -1e-12 * total)) {
                ++j; // 6. try again
                continue;
            }
            total += cost; // cost is negative!
            // Swap:
            curr.performLastSwap(cand);
            j = 1;
        }
        // New best:
        if(total < bestscore) {
            // Swap:
            FastAssignment tmp = curr;
            curr = best;
            best = tmp;
            bestscore = total;
        }
    }
    
    bestmedoids = best.medoids;
    bestclusters = best.assignment;
    return bestscore;
}

//  h Current object to swap with any medoid.
double FastAssignment::computeCostDifferential(int h)
{
    int k = (int)cost.size();
    for (int i=0; i<k; ++i)  cost[i] = 0;
    // Compute costs of reassigning other objects j:
    for (int j=0; j<num_obs; ++j) {
        if (h == j) {
            continue;
        }
        // distance(j, i) to nearest medoid
        double distcur = nearest[j];
        // distance(j, h) to new medoid
        double dist_h = dist_matrix->getDistance(h, j);
        // current assignment of j
        int jcur = assignment[j];
        // Check if current medoid of j is removed:
        cost[jcur] += std::min(dist_h, second[j]) - distcur;
        double change = dist_h - distcur;
        if (change < 0) {
            for(int mnum = 0; mnum < jcur; mnum++) {
                cost[mnum] += change;
            }
            for(int mnum = jcur + 1; mnum < k; mnum++) {
                cost[mnum] += change;
            }
        }
    }
    double min = cost[0];
    lastbest = 0;
    for(int i = 1; i < k; i++) {
        if(cost[i] < min) {
            min = cost[i];
            lastbest = i;
        }
    }
    return min;
}

// Compute the reassignment cost, for one swap.
// h Current object to swap with the best medoid
void FastAssignment::performLastSwap(int h)
{
    // Update medoids of scratch copy.
    medoids[lastbest] = h;
    
    // Compute costs of reassigning other objects j:
    for (int j=0; j<num_obs; ++j) {
        if (h == j) {
            recompute(j, lastbest, 0., -1, DBL_MAX);
            continue;
        }
        // distance(j, i) to nearest medoid
        double distcur = nearest[j];
        // distance(j, h) to new medoid
        double dist_h = dist_matrix->getDistance(h, j);
        // current assignment of j
        int jcur = assignment[j];
        // Check if current medoid of j is removed:
        if(jcur == lastbest) {
            // distance(j, o) to second nearest / possible reassignment
            double distsec = second[j];
            // Case 1b: j switches to new medoid, or to the second nearest:
            if(dist_h < distsec) {
                assignment[j] = lastbest;
                nearest[j] = dist_h;
                second[j] = distsec;
                secondid[j] = jcur;
            } else {
                // We have to recompute, because we do not know the true new second
                // nearest.
                recompute(j, lastbest, dist_h, jcur, distsec);
            }
        }
        else if(dist_h < distcur) {
            // Case 1c: j is closer to h than its current medoid
            // and the current medoid is not removed (jcur != mnum).
            // Second nearest is the previous assignment
            assignment[j] = lastbest;
            nearest[j] = dist_h;
            second[j] = distcur;
            secondid[j] = jcur;
        }
        else { // else Case 1a): j is closer to i than h and m, so no change.
            int jsec = secondid[j];
            double distsec = second[j];
            // Second nearest is still valid.
            if(jsec != lastbest && distsec <= dist_h) {
                assignment[j] = jcur;
                nearest[j] = distcur;
                secondid[j] = jsec;
                second[j] = distsec;
            }
            else {
                recompute(j, jcur, distcur, lastbest, dist_h);
            }
        }
    }
}

