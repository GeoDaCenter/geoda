#include <stdio.h>
#include <iostream>
#include <math.h>

#ifdef __linux__
//do nothing
float* gpu_distmatrix(const char* cl_path, int rows, int columns, double** data, int start, int end)
{
    return NULL;
}
#else

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE (0x100000)

using namespace std;

float* gpu_distmatrix(const char* cl_path, int rows, int columns, double** data, int start, int end)
{
    unsigned long long _rows = rows;
    unsigned long long _columns = columns;
    unsigned long long sz_float = sizeof(float);
    unsigned long long _block = end-start+1;
    unsigned long long i;
    
    unsigned long long indata_size = _rows * _columns * sz_float;
    unsigned long long outdata_size = _rows * _block * sz_float;
    float* a = (float *) malloc (indata_size);
    float* r = (float *) malloc (outdata_size);
    
    unsigned long long idx;
    
    for (i=0; i<rows; i++) {
        for (unsigned long long j=0; j<columns; j++) {
            idx = i*_columns+j;
            a[idx] = data[i][j];
        }
    }
    
    unsigned long long sum_rows = _rows * _block;
    for (i=0; i<sum_rows; i++) {
        r[i] = 0;
    }
    
    
    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;
    
    fp = fopen(cl_path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        return 0;
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );
    
    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    
    cl_uint maxDevices = 100;
    cl_device_id* devices = new cl_device_id[maxDevices];
    cl_uint nrDevices;
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, maxDevices, devices, &ret_num_devices);
    cl_device_id device_id = devices[0];
    if (ret_num_devices==2) {
        device_id = devices[1];
    }
    //ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &ret_num_devices);
    
    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, ret_num_devices, devices, NULL, NULL, &ret);
    
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    
    size_t test = (size_t)outdata_size;
    // Create memory buffers on the device for each vector
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                      indata_size, NULL, &ret);
    cl_mem r_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                      outdata_size, NULL, &ret);
    
    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
                               indata_size, a, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, r_mem_obj, CL_TRUE, 0,
                               outdata_size, r, 0, NULL, NULL);
    
    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
                                                   (const char **)&source_str, (const size_t *)&source_size, &ret);
    
    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "euclidean_dist", &ret);
    
    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_ulong), (void *)&_rows);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_ulong), (void *)&_columns);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&start);
    ret = clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&end);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&a_mem_obj);
    ret = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&r_mem_obj);
    
    // Execute the OpenCL kernel on the list
    size_t global_item_size = 64 * ceil(_block/64.0); // Process the entire lists
    size_t local_item_size = 64; // Process in groups of 64
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                 &global_item_size, &local_item_size, 0, NULL, NULL);
    
    // Read the memory buffer C on the device to the local variable C
    ret = clEnqueueReadBuffer(command_queue, r_mem_obj, CL_TRUE, 0,
                              outdata_size, r, 0, NULL, NULL);
    
    // Display the result to the screen
    for(i = 0; i < 20; i++)
        printf("%f\n", r[i]);
    
    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
    ret = clReleaseMemObject(r_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    free(a);
    return r;
}

#endif
