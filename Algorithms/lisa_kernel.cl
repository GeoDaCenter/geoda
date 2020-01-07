#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define BUCKET_EMPTY -1

int hash_code(uint key, uint capacity)
{
    return key % capacity;
}

void init_bucket(int* bucket, uint capacity)
{
    for (size_t i=0; i<capacity; ++i) {
        bucket[i] = BUCKET_EMPTY;
    }
}

void insert_bucket(uint key, int *bucket, uint capacity)
{
    int hashIndex = hash_code(key, capacity);

    //find next free space
    while (bucket[hashIndex] != key && bucket[hashIndex] != BUCKET_EMPTY) {
        hashIndex++;
        hashIndex %= capacity;
    }

    // make sure that insert_bucket() will not overfill the bucket
    // todo:

    bucket[hashIndex] = key;
}

bool search_bucket(uint key, int *bucket, uint capacity)
{
    // Apply hash function to find index for given key
    int hashIndex = hash_code(key, capacity);
    int counter=0;

    //finding the node with given key
    while (counter++ < capacity) { //to avoid infinite loop

        if (bucket[hashIndex] == BUCKET_EMPTY)
            return false;

        //if node found return true
        if(bucket[hashIndex] == key)
            return true;

        hashIndex++;
        hashIndex %= capacity;
    }

    //If not found return false
    return false;
}

struct Dict {
    int key;
    int val;
};

typedef struct Dict Dict;

void init_dict(Dict *arr, uint capacity)
{
    for (size_t i=0; i<capacity; ++i) {
        arr[i].key = BUCKET_EMPTY;
    }
}

void insert_dict(uint key, uint val, Dict *arr, uint capacity)
{
    int hashIndex = hash_code(key, capacity);

    //find next free space
    while (arr[hashIndex].key != key && arr[hashIndex].key != BUCKET_EMPTY) {
        hashIndex++;
        hashIndex %= capacity;
    }

    // make sure that insert_dict() will not overfill the bucket
    // todo:

    arr[hashIndex].key = key;
    arr[hashIndex].val = val;
}

bool search_dict(uint key, Dict *arr, uint capacity)
{
    // Apply hash function to find index for given key
    int hashIndex = hash_code(key, capacity);
    int counter=0;

    //finding the node with given key
    while (counter++ < capacity) { //to avoid infinite loop

        //if node found return true
        if(arr[hashIndex].key == key)
            return arr[hashIndex].val;

        hashIndex++;
        hashIndex %= capacity;
    }

    //If not found return false
    return BUCKET_EMPTY;
}

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

double ThomasWangHashDouble(ulong key);
double ThomasWangHashDouble(ulong key)
{
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return 5.42101086242752217E-20 * key;
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
    size_t countLarger = 0;

    int rnd_dict[123];

    for (size_t perm=0; perm<permutations; perm++ ) {
        size_t randNeighbors = 0;
        double permutedLag = 0;
        init_bucket(rnd_dict, 123);

        while (randNeighbors < numNeighbors) {
            double rng_val = ThomasWangHashDouble(seed_start++) * max_rand;
            int newRandom = (int)rng_val;

            if (search_bucket(newRandom, rnd_dict, 123) == false) {
                permutedLag += values[newRandom];
                //rnd_numbers[rand] = newRandom;
                randNeighbors++;
                insert_bucket(newRandom, rnd_dict, 123);
            }
        }
        permutedLag /= numNeighbors;
        double localMoranPermuted = permutedLag * values[i];
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
