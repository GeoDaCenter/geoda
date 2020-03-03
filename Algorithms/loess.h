#ifndef DLOESS_LOESS_H_
#define DLOESS_LOESS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* for the meaning of these fields, see struct.m */
/* long int-s are used here so that the codes can be called from S */

#include <stdint.h>

#define TRUE  1
#define FALSE 0

struct loess_struct {
	struct {
		long int    n;
	        long int    p;
	        double  *y;
	        double  *x;
		double	*weights;
	} in;
	struct {
	        double  span;
	        long int    degree;
	        long int    normalize;
	        long int    parametric[8];
	        long int    drop_square[8];
	        char    *family;
	} model;
	struct {
	        char    *surface;
	        char    *statistics;
	        double  cell;
	        char    *trace_hat;
	        long int    iterations;
	} control;
	struct {
		long int	*parameter;
		long int	*a;
		double	*xi;
		double	*vert;
		double	*vval;
	} kd_tree;
	struct {
		double	*fitted_values;
	        double  *fitted_residuals;
		double  enp;
		double	s;
		double  one_delta;
		double	two_delta;
		double	*pseudovalues;
		double	trace_hat;
		double	*diagonal;
		double	*robust;
		double  *divisor;
	} out;
};

struct pred_struct {
	double	*fit;
	double	*se_fit;
	double  residual_scale;
	double  df;
};

struct anova_struct {
	double	dfn;
	double	dfd;
	double  F_value;
	double  Pr_F;
};

struct ci_struct {
	double	*fit;
	double	*upper;
	double  *lower;
};

// loess

void
loess_setup(double *x, double *y, long int n, long int p,
            struct loess_struct *lo);

void
loess(struct loess_struct *lo);

void
loess_free_mem(struct loess_struct *lo);

void
loess_summary(struct loess_struct *lo);

// predict

void
predict(double *eval, long int m, struct loess_struct *lo,
        struct pred_struct *pre, long int se);

void
pred_free_mem(struct pred_struct *pre);

// misc

void
anova(struct loess_struct *one, struct loess_struct *two,
      struct anova_struct *out);

void
pointwise(struct pred_struct *pre, long int m, double coverage,
          struct ci_struct *ci);

void
pw_free_mem(struct ci_struct *ci);

#ifdef __cplusplus
}
#endif

#endif // DLOESS_LOESS_H_
