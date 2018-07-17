#ifndef __GEODA_CENTER_GPU_LISA_H___
#define __GEODA_CENTER_GPU_LISA_H___

#include "../ShapeOperations/GalWeight.h"

bool gpu_lisa(const char* cl_path, int rows, int permutations, unsigned long long last_seed_used, double* values, double* local_moran, GalElement* w, double* p);


bool gpu_localjoincount(const char* cl_path, int rows, int permutations, unsigned long long last_seed_used, int num_vars, int* values, int* local_jc, GalElement* w, double* p);

#endif
