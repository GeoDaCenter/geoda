#include "smacof.h"
#include "../Regression/f2c.h"

int dposv_(char *uplo, integer *n, integer *nrhs, doublereal *a, integer *lda, doublereal *b, integer *ldb, integer *info);

int dsyevd_(char *jobz, char *uplo, integer *n, doublereal *
            a, integer *lda, doublereal *w, doublereal *work, integer *lwork,
            integer *iwork, integer *liwork, integer *info);

int dgeqrf_(integer *m, integer *n, doublereal *a, integer *
            lda, doublereal *tau, doublereal *work, integer *lwork, integer *info);

int dorgqr_(integer *m, integer *n, integer *k, doublereal *
            a, integer *lda, doublereal *tau, doublereal *work, integer *lwork,
            integer *info);

lapack_int to_lapack_int(const int *n)
{
    int nn = (int)*n;
    lapack_int ln = 0;
    ln = nn;
    return ln;
}


void dposv(const int *n, const int *m, double *a, double *b) {
    lapack_int nn = to_lapack_int(n), mm = to_lapack_int(m);

    //(void)LAPACKE_dposv(LAPACK_COL_MAJOR, 'U', nn, mm, a, nn, b, nn);
    // ref: lapacke_dposv.c
    // LAPACKE_dposv_work(matrix_layout, uplo, n, nrhs, a, lda, b, ldb)

    lapack_int info = 0;
    lapack_int lda = nn;
    lapack_int ldb = nn;
    dposv_('U', &nn, &mm, a, &lda, b, &ldb, &info);
    if( info < 0 ) {
        info = info - 1;
    }
    return;
}

void dsyevd(const int *n, double *a, double *x) {
    lapack_int nn = to_lapack_int(n);
    //(void)LAPACKE_dsyevd(LAPACK_COL_MAJOR, 'V', 'U', nn, a, nn, x);
    // ref: lapacke_dstevd.c
    // LAPACKE_dstevd( int matrix_layout, char jobz, lapack_int n, double* d, double* e, double* z, lapack_int ldz )

    lapack_int lda = (lapack_int)*n;
    lapack_int info = 0;
    lapack_int iwork_query;
    lapack_int lwork = -1;
    double work_query;
    lapack_int liwork = -1;

    /* Query optimal working array(s) size */
    dsyevd_('V', 'U', &nn, a, &lda, x, &work_query, &lwork, &iwork_query, &liwork, &info);

    liwork = (lapack_int)iwork_query;
    lwork = (lapack_int)work_query;

    /* Allocate memory for work arrays */
    lapack_int* iwork = NULL;
    double* work = NULL;

    iwork = (lapack_int*)LAPACKE_malloc( sizeof(lapack_int) * liwork );
    work = (double*)LAPACKE_malloc( sizeof(double) * lwork );

    /* Call middle-level interface */
    info = 0;
    lda = (lapack_int)*n;
    nn = (lapack_int)*n;
    dsyevd_('V', 'U', &nn, a, &lda, x, work, &lwork, iwork, &liwork, &info);

    LAPACKE_free( work );
    LAPACKE_free( iwork );
    return;
}

void dgeqrf(const int *n, const int *m, double *a, double *tau) {
    //lapack_int nn = (lapack_int)*n, mm = (lapack_int)*m;
    //(void)LAPACKE_dgeqrf(LAPACK_COL_MAJOR, nn, mm, a, nn, tau);
    // ref: lapacke_dgeqrf.c
    lapack_int mm = to_lapack_int(n);
    lapack_int nn = to_lapack_int(m);
    lapack_int lda = to_lapack_int(n);

    lapack_int info = 0;
    lapack_int lwork = -1;
    double* work = NULL;
    double work_query;

    /* Query optimal working array(s) size */
    // mm, nn, a, lda, tau, &work_query, lwork
    dgeqrf_( &mm, &nn, a, &lda, tau, &work_query, &lwork, &info );

    lwork = (lapack_int)work_query;
    /* Allocate memory for work arrays */
    work = (double*)LAPACKE_malloc( sizeof(double) * lwork );

    /* Call middle-level interface */
    // mm, nn, a, lda, tau, work, lwork
    lda = to_lapack_int(n);
    mm = to_lapack_int(n);
    nn = to_lapack_int(m);
    info = 0;
    dgeqrf_( &mm, &nn, a, &lda, tau, work, &lwork, &info );

    /* Release memory and exit */
    LAPACKE_free( work );
    return;
}

void dorgqr(const int *n, const int *m, double *a, double *tau) {
    //lapack_int nn = (lapack_int)*n, mm = (lapack_int)*m;
    //(void)LAPACKE_dorgqr(LAPACK_COL_MAJOR, nn, mm, mm, a, nn, tau);
    // ref: lapacke_dorgqr.c

    lapack_int mm = to_lapack_int(n);
    lapack_int nn = to_lapack_int(m);
    lapack_int k = to_lapack_int(m);
    lapack_int lda = to_lapack_int(n);

    lapack_int info = 0;
    lapack_int lwork = -1;
    double* work = NULL;
    double work_query;

    /* Query optimal working array(s) size */
    // mm, nn, k, a, lda, tau, &work_query, lwork
    dorgqr_(&mm, &nn, &k, a, &lda, tau, &work_query, &lwork, &info);

    lwork = (lapack_int)work_query;
    /* Allocate memory for work arrays */
    work = (double*)LAPACKE_malloc( sizeof(double) * lwork );

    /* Call middle-level interface */
    mm = to_lapack_int(n);
    nn = to_lapack_int(m);
    k = to_lapack_int(m);
    lda = to_lapack_int(n);
    info = 0;
    // mm, nn, k, a, lda, tau, work, lwork
    dorgqr_(&mm, &nn, &k, a, &lda, tau, work, &lwork, &info);

    /* Release memory and exit */
    LAPACKE_free( work );

    return;
}

void dortho(const int *n, const int *m, double *a) {
    int nn = (int)*n, mm = (int)*m;
    double *tau = calloc((size_t)mm, sizeof(double));

    //(void)LAPACKE_dgeqrf(LAPACK_COL_MAJOR, nn, mm, a, nn, tau);
    //(void)LAPACKE_dorgqr(LAPACK_COL_MAJOR, nn, mm, mm, a, nn, tau);
    dgeqrf(&nn, &mm, a, tau);

    nn = (int)*n;
    mm = (int)*m;
    dorgqr(&nn, &mm, a, tau);

    free(tau);
    return;
}
