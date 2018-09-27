__kernel void euclidean_dist(const unsigned long rows, const unsigned long columns, __global float *a, __global float *r) {
//__kernel void vector_add(const int rows, const int columns, __global float *A, __global float *B, __global float *C) {
//__kernel void vector_add(__global int *A, __global int *B, __global int *C) {
//__kernel void vector_add(__global float *A, __global float *B, __global float *C) {
    
    // Get the index of the current element
    int rid = get_global_id(0);

    // Do the operation
    //C[i] = A[i] + B[i];
    //r[0] = 1.23;

    if (rid >= rows) 
        return;
   
    unsigned long id_1;
    unsigned long id_2;
    unsigned long idx;
    float tmp=0;
    float dist=0;
    
    for (unsigned long i=0; i<rows; i++ ) {
        if (i!= rid) {
            tmp = 0;
            dist = 0;
            for (unsigned long j=0; j<columns; j++) {
                id_1 = i*columns + j;
                id_2 = rid*columns + j;
                tmp = a[id_1] - a[id_2];
                dist += tmp*tmp;
            }
            idx = rid*rows + i;
            r[idx] = dist;
        }
    } 
}
