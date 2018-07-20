#pragma OPENCL EXTENSION cl_khr_fp64 : enable
float wang_rnd(uint seed);
float wang_rnd(uint seed)
{
    uint maxint=0;
    maxint--; // not ok but works
    
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    
    return ((float)seed)/(float)maxint;
}

__kernel void lisa(const int n, const int permutations, const unsigned long last_seed, __global double *values,  __global double *local_moran,  __global int *num_nbrs, __global int *nbr_idx, __global double *p) {
    
    // Get the index of the current element
    size_t i = get_global_id(0);

    if (i >= n) {
        return;
    }
   
    size_t j = 0;
    size_t seed_start = i + last_seed;
    
    size_t numNeighbors = num_nbrs[i];
    if (numNeighbors == 0) {
        return;
    }
    
    size_t nbr_start = 0;
    
    for (j=0; j <i; j++) {
        nbr_start += num_nbrs[j];
    }
    
    size_t max_rand = n-1;
    int newRandom;
    
    size_t perm=0;
    size_t rand = 0;
    
    bool is_valid;
    double rng_val;
    double permutedLag =0;
    double localMoranPermuted=0;
    size_t countLarger = 0;
    
    size_t rnd_numbers[123]; // 1234 can be replaced with max #nbr
    
    for (perm=0; perm<permutations; perm++ ) {
        rand=0;
        permutedLag =0;
        while (rand < numNeighbors) {
            is_valid = true;
            rng_val = wang_rnd(seed_start++) * max_rand;
            newRandom = (int)rng_val;
          
            if (newRandom != i ) {
                for (j=0; j<rand; j++) {
                    if (newRandom == rnd_numbers[j]) {
                        is_valid = false;
                        break;
                    }
                }
                if (is_valid) {
                    permutedLag += values[newRandom];
                    rnd_numbers[rand] = newRandom;
                    rand++;
                }
            }
        
        }
        permutedLag /= numNeighbors;
        localMoranPermuted = permutedLag * values[i];
        if (localMoranPermuted > local_moran[i]) {
            countLarger++;
        }
    }
    
    // pick the smallest
    if (permutations-countLarger <= countLarger) {
        countLarger = permutations-countLarger;
    }
    
    double sigLocal = (countLarger+1.0)/(permutations+1);
    p[i] = sigLocal;
}
