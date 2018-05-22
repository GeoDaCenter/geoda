#ifndef __GEODA_CENTER_GPU_LISA_H___
#define __GEODA_CENTER_GPU_LISA_H___


bool gpu_lisa(const char* cl_path, int rows, int permutations, unsigned long long last_seed_used, double* values, double* local_moran, GalElement* w, double* p);

#endif
