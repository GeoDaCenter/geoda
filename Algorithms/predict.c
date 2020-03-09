#include "S.h"
#include "loess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char *error_message;
extern int error_status;

static void
pred_(double *y, double *x_, double *new_x, long int *size_info, double *residual_scale,
      double *weights, double *robust, double *span, long int *degree,
      long int *normalize, long int *parametric, long int *drop_square, char **surface,
      double *cell, char **family, long int *parameter, long int *a, double *xi,
      double *vert, double *vval, double *divisor, long int *se, double *fit,
      double *se_fit)
{
    double *x, *x_tmp, *x_evaluate, *L, new_cell, tmp, *fit_tmp,
           *temp;
    long int N, D, M, sum_drop_sqr = 0, sum_parametric = 0,
        nonparametric = 0, *order_parametric, *order_drop_sqr;
    long int i, j, k, p;
    long int gaussian_family = !strcmp(*family, "gaussian");
    long int direct_surface = !strcmp(*surface, "direct");

    D = size_info[0];
    N = size_info[1];
    M = size_info[2];

    x = MALLOC(N * D * sizeof(double));
    x_tmp = MALLOC(N * D * sizeof(double));
    x_evaluate = MALLOC(M * D * sizeof(double));
    L = MALLOC(N * M * sizeof(double));
    order_parametric = MALLOC(D * sizeof(long int));
    order_drop_sqr = MALLOC(D * sizeof(long int));
    temp = MALLOC(N * D * sizeof(double));

    for(i = 0; i < (N * D); i++)
        x_tmp[i] = x_[i];
    for(i = 0; i < D; i++) {
        k = i * M;
        for(j = 0; j < M; j++) {
            p = k + j;
            new_x[p] = new_x[p] / divisor[i];
        }
    }
    if(direct_surface || se) {
        for(i = 0; i < D; i++) {
            k = i * N;
            for(j = 0; j < N; j++) {
                p = k + j;
                x_tmp[p] = x_[p] / divisor[i];
            }
        }
    }
    j = D - 1;
    for(i = 0; i < D; i++) {
        sum_drop_sqr = sum_drop_sqr + drop_square[i];
        sum_parametric = sum_parametric + parametric[i];
        if(parametric[i])
            order_parametric[j--] = i;
        else
            order_parametric[nonparametric++] = i;
    }
    for(i = 0; i < D; i++) {
        order_drop_sqr[i] = 2 - drop_square[order_parametric[i]];
        k = i * M;
        p = order_parametric[i] * M;
        for(j = 0; j < M; j++)
            x_evaluate[k + j] = new_x[p + j];
        k = i * N;
        p = order_parametric[i] * N;
        for(j = 0; j < N; j++)
            x[k + j] = x_tmp[p + j];
    }
    for(i = 0; i < N; i++)
        robust[i] = weights[i] * robust[i];

    if(direct_surface) {
        if(*se) {
            loess_dfitse(y, x, x_evaluate, weights, robust,
                         &gaussian_family, span, degree,
                         &nonparametric, order_drop_sqr, &sum_drop_sqr,
                         &D, &N, &M, fit, L);
        }
        else {
            loess_dfit(y, x, x_evaluate, robust, span, degree,
                       &nonparametric, order_drop_sqr, &sum_drop_sqr,
                       &D, &N, &M, fit);
        }
    }
    else {
        loess_ifit(parameter, a, xi, vert, vval, &M, x_evaluate, fit);
        if(*se) {
            new_cell = (*span) * (*cell);
            fit_tmp = MALLOC(M * sizeof(double));
            loess_ise(y, x, x_evaluate, weights, span, degree,
                      &nonparametric, order_drop_sqr, &sum_drop_sqr,
                      &new_cell, &D, &N, &M, fit_tmp, L);
            free(fit_tmp);
        }
    }
    if(*se) {
        for(i = 0; i < N; i++) {
            k = i * M;
            for(j = 0; j < M; j++) {
                p = k + j;
                L[p] = L[p] / weights[i];
                L[p] = L[p] * L[p];
            }
        }
        for(i = 0; i < M; i++) {
            tmp = 0;
            for(j = 0; j < N; j++)
                tmp = tmp + L[i + j * M];
            se_fit[i] = (*residual_scale) * sqrt(tmp);
        }
    }
    free(x);
    free(x_tmp);
    free(x_evaluate);
    free(L);
    free(order_parametric);
    free(order_drop_sqr);
    free(temp);
}

void
predict(double *eval, loess *lo, prediction *pre)
{
    long int size_info[3];

    pre->fit = MALLOC(pre->m * sizeof(double));
    if (pre->se) {
        pre->se_fit = MALLOC(pre->m * sizeof(double));
    }
    pre->residual_scale = lo->outputs->residual_scale;
    pre->df = (lo->outputs->one_delta * lo->outputs->one_delta) /
              lo->outputs->two_delta;

    size_info[0] = lo->inputs->p;
    size_info[1] = lo->inputs->n;
    size_info[2] = pre->m;

    error_status = 0;
    lo->status.err_status = 0;
    lo->status.err_msg = NULL;

    pred_(lo->inputs->y,
          lo->inputs->x, eval,
          size_info,
          &lo->outputs->residual_scale,
          lo->inputs->weights,
          lo->outputs->robust,
          &lo->model->span,
          &lo->model->degree,
          &lo->model->normalize,
          lo->model->parametric,
          lo->model->drop_square,
          &lo->control->surface,
          &lo->control->cell,
          &lo->model->family,
          lo->kd_tree->parameter,
          lo->kd_tree->a,
          lo->kd_tree->xi,
          lo->kd_tree->vert,
          lo->kd_tree->vval,
          lo->outputs->divisor,
          &pre->se,
          pre->fit,
          pre->se_fit);

    if(error_status){
        lo->status.err_status = error_status;
        lo->status.err_msg = error_message;
    }
}


void
pred_free_mem(prediction *pre)
{
    free(pre->fit);
    if(pre->se) {
        free(pre->se_fit);
    }
}
