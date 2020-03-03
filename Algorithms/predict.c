#include "S.h"
#include "loess.h"
#include "loessc.h"

void
predict(eval, m, lo, pre, se)
double  *eval;
long int	m, se;
struct	loess_struct	*lo;
struct	pred_struct	*pre;
{
	long int	size_info[3];
	void    pred_();

        pre->fit = (double *) malloc(m * sizeof(double));
        pre->se_fit = (double *) malloc(m * sizeof(double));
	pre->residual_scale = lo->out.s;
	pre->df = (lo->out.one_delta * lo->out.one_delta) / lo->out.two_delta;

	size_info[0] = lo->in.p;
	size_info[1] = lo->in.n;
	size_info[2] = m;
    
	pred_(lo->in.y, lo->in.x, eval, size_info, &lo->out.s, 
		lo->in.weights, 
		lo->out.robust,
		&lo->model.span,
		&lo->model.degree,
		&lo->model.normalize,
		lo->model.parametric,
		lo->model.drop_square,
		&lo->control.surface,
		&lo->control.cell,
		&lo->model.family,
		lo->kd_tree.parameter,
		lo->kd_tree.a,
		lo->kd_tree.xi,
		lo->kd_tree.vert,
		lo->kd_tree.vval,
		lo->out.divisor,
		&se,
		pre->fit,
		pre->se_fit);
}

void
pred_(y, x_, new_x, size_info, s, weights, robust, span, degree, 
	normalize, parametric, drop_square, surface, cell, family, 
	parameter, a, xi, vert, vval, divisor, se, fit, se_fit)
double  *y, *x_, *new_x, *weights, *robust, *span, *cell, *fit, *s,
        *xi, *vert, *vval, *divisor, *se_fit;
long int    *size_info, *degree, *normalize, *parametric, *drop_square, 
        *parameter, *a, *se;
char    **surface, **family;
{     
        double  *x, *x_tmp, *x_evaluate, *L, new_cell, z, tmp, *fit_tmp, 
	        *temp, sum, mean;
        long int    N, D, M, sum_drop_sqr = 0, sum_parametric = 0,
	        nonparametric = 0, *order_parametric, *order_drop_sqr;
	int     i, j, k, p, cut, comp();
    long int family_id;

        D = size_info[0];
        N = size_info[1];
	M = size_info[2];

	x = (double *) malloc(N * D * sizeof(double));
	x_tmp = (double *) malloc(N * D * sizeof(double));
	x_evaluate = (double *) malloc(M * D * sizeof(double));
	L = (double *) malloc(N * M * sizeof(double));
        order_parametric = (long int *) malloc(D * sizeof(long int));
        order_drop_sqr = (long int *) malloc(D * sizeof(long int));
	temp = (double *) malloc(N * D * sizeof(double));

	for(i = 0; i < (N * D); i++)
		x_tmp[i] = x_[i];
	for(i = 0; i < D; i++) {
		k = i * M;
		for(j = 0; j < M; j++) {
			p = k + j;
			new_x[p] = new_x[p] / divisor[i];
		}
	}
	if(!strcmp(*surface, "direct") || se) {
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

	if(!strcmp(*surface, "direct")) {
	        if(*se) {
                        family_id = (!strcmp(*family, "gaussian")) ? GAUSSIAN : SYMMETRIC;
		        loess_dfitse(y, x, x_evaluate, weights, robust,
				&family_id, span, degree,
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
           	        fit_tmp = (double *) malloc(M * sizeof(double));
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
			se_fit[i] = (*s) * sqrt(tmp);
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
pred_free_mem(pre)
struct	pred_struct	*pre;
{
	free(pre->fit);
	free(pre->se_fit);
}






