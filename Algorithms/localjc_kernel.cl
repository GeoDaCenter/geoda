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

__kernel void localjc(const int n, const int permutations, const unsigned long last_seed, const unsigned long num_vars, __global unsigned short *zz,  __global unsigned short *local_jc,  __global unsigned short *num_nbrs, __global unsigned short *nbr_idx, __global float *p) {
    // Get the index of the current element
    int i = get_global_id(0);
    if (i >= n) {
        return;
    }
    if (local_jc[i] == 0) {
        p[i] = 0;
        return;
    }
    int j = 0;
    size_t seed_start = i + last_seed;
    size_t rnd_numbers[888];
    unsigned char dict[999];
    for (j=0; j<999; j++) dict[j] = 0;
    
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
    
    int perm=0;
    int rand = 0;
    
    bool is_valid;
    double rng_val;
    double permutedLag = 0;
    size_t countLarger = 0;
    
    for (perm=0; perm<permutations; perm++ ) {
        rand=0;
        permutedLag =0;
        while (rand < numNeighbors) {
            is_valid = true;
            rng_val = wang_rnd(seed_start++) * max_rand;
            newRandom = (int)rng_val;
          
            if (newRandom != i ) {
                if (dict[newRandom] == 0) {
                    dict[newRandom] = 1;
                    rnd_numbers[rand] = newRandom;
                    rand++;
                    permutedLag += zz[newRandom];
                }
            }
        }
        for (j=0; j<rand; j++) {
            dict[rnd_numbers[j]] = 0;
        }
        if (permutedLag >= local_jc[i]) {
            countLarger++;
        }
    }

    if (permutations-countLarger < countLarger) {
        countLarger = permutations-countLarger;
    }
    p[i] = permutations + 1;
    countLarger = countLarger + 1.0;
    p[i] = countLarger/p[i];
}


