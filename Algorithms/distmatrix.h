#ifndef __GEODA_CENTER_DISTMATRIX_H___
#define __GEODA_CENTER_DISTMATRIX_H___


float* gpu_distmatrix(const char* cl_path, int rows, int columns, double** data, int start, int end);


#endif
