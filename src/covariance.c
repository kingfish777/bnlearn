#include "include/rcore.h"
#include "include/matrix.h"
#include "include/covariance.h"

/* fill a covariance matrix. */
void c_covmat(double **data, double *mean, int nrow, int ncol,
    double *mat, int first) {

int i = 0, j = 0, k = 0;
long double temp = 0;

  /* special case: with zero and one observations, the covariance
   * matrix is all zeroes. */
  if (nrow <= 1) {

    memset(mat, '\0', ncol * ncol * sizeof(double));

    return ;

  }/*THEN*/

  /* compute the actual covariance. */
  for (i = first; i < ncol; i++) {

    for (j = i; j < ncol; j++) {

      for (k = 0, temp = 0; k < nrow; k++)
        temp += (data[i][k] - mean[i]) * (data[j][k] - mean[j]);

      /* fill in the symmetric element of the matrix. */
      mat[CMC(j, i, ncol)] = mat[CMC(i, j, ncol)] =
        (double)(temp / (nrow - 1));

    }/*FOR*/

  }/*FOR*/

}/*C_COVMAT*/

/* update only a single row/column in a covariance matrix. */
void c_update_covmat(double **data, double *mean, int update, int nrow,
    int ncol, double *mat) {

int j = 0, k = 0;
long double temp = 0;

  /* special case: with zero and one observations, the covariance
   * matrix is all zeroes. */
  if (nrow <= 1) {

    for (j = 0; j < ncol; j++)
      mat[CMC(j, update, ncol)] = mat[CMC(update, j, ncol)] = 0;

    return ;

  }/*THEN*/

  /* compute the actual covariance. */
  for (j = 0; j < ncol; j++) {

    for (k = 0, temp = 0; k < nrow; k++)
      temp += (data[update][k] - mean[update]) * (data[j][k] - mean[j]);

    /* fill the symmetric elements of the matrix. */
    mat[CMC(j, update, ncol)] = mat[CMC(update, j, ncol)] =
      (double)(temp / (nrow - 1));

  }/*FOR*/

}/*C_UPDATE_COVMAT*/

/* compute the sum of the squared errors. */
double c_sse(double *data, double mean, int nrow) {

int i = 0;
long double nvar = 0;

  for (i = 0; i < nrow; i++)
    nvar += (data[i] - mean) * (data[i] - mean);

  return (double)nvar;

}/*C_SSE*/

/* compute the mean. */
double c_mean(double *data, int nrow) {

int i = 0;
long double mean = 0;

  for (i = 0; i < nrow; i++)
   mean += data[i];
  mean /= nrow;

  return (double)mean;

}/*C_MEAN*/

/* compute a vector of means from a set of variables. */
void c_meanvec(double **data, double *mean, int nrow, int ncol, int first) {

int i = 0, j = 0;
long double sum = 0;

  for (i = first; i < ncol; i++) {

    for (j = 0, sum = 0; j < nrow; j++)
      sum += data[i][j];

    mean[i] = sum / nrow;

  }/*FOR*/

}/*C_MEANVEC*/

/* compute a vector of variances from a set of variables. */
void c_ssevec(double **data, double *sse, double *means, int nrow, int ncol,
    int first) {

int i = 0, j = 0;
long double sum = 0;

  for (i = first; i < ncol; i++) {

    for (sum = 0, j = 0 ; j < nrow; j++)
      sum += (data[i][j] - means[i]) * (data[i][j] - means[i]);

    sse[i] = sum;

  }/*FOR*/

}/*C_SSEVEC*/

/* update one elemenet in a vector of means. */
void c_update_meanvec(double **data, double *mean, int update, int nrow) {

int j = 0;
long double sum = 0;

  for (j = 0; j < nrow; j++)
    sum += data[update][j];
  mean[update] = sum / nrow;

}/*C_UPDATE_MEANVEC*/

/* compute a single standard deviation. */
void c_sd(double *xx, int nobs, int p, double mean, int compute, double *sd) {

  SD_GUARD(nobs, p, *sd,
     if (compute)
       mean = c_mean(xx, nobs);

    *sd = sqrt(c_sse(xx, mean, nobs) / (nobs - p));
  )

}/*C_SD*/

/* compute one standard deviation per stratum. */
void c_cgsd(double *xx, int *z, int *nz, int nobs, int nstrata, int p,
  long double *means, double *sd) {

int i = 0, *nnz = NULL;
long double *ssr = NULL, *mm = NULL;

  /* allocate the accumulators. */
  ssr = Calloc1D(nstrata, sizeof(long double));

  /* allocate and compute the per-stratum sample sizes, if needed. */
  if (!nz) {

    nnz = Calloc1D(nstrata, sizeof(int));
    for (i = 0; i < nobs; i++)
      nnz[z[i] - 1]++;

  }/*THEN*/
  else {

    nnz = nz;

  }/*ELSE*/

  /* allocate and compute the per-stratum means, if needed. */
  if (!means) {

    mm = Calloc1D(nstrata, sizeof(long double));
    for (i = 0; i < nobs; i++)
      mm[z[i] - 1] += xx[i];
    for (i = 0; i < nstrata; i++)
      if (nnz[i] != 0)
        mm[i] /= nnz[i];

  }/*THEN*/
  else {

    mm = means;

  }/*ELSE*/

  /* compute the per-stratum sum of the squares residuals. */
  for (i = 0; i < nobs; i++)
    ssr[z[i] - 1] += (xx[i] - mm[z[i] - 1]) * (xx[i] - mm[z[i] - 1]);
  /* compute the standard deviations appropriately. */
  for (i = 0; i < nstrata; i++) {

    SD_GUARD(nnz[i], p, sd[i],
      sd[i] = sqrt(ssr[i] / (nnz[i] - p));
    )

  }/*FOR*/

  if (!nz)
    Free1D(nnz);
  if (!means)
    Free1D(mm);
  Free1D(ssr);

}/*C_CGSD*/

/* compute one or more standard deviations, possibly with strata. */
SEXP cgsd(SEXP x, SEXP strata, SEXP nparams) {

int nstrata = 0, nobs = length(x);
int *z = NULL;
SEXP sd;

  if (strata == R_NilValue) {

    PROTECT(sd = allocVector(REALSXP, 1));
    c_sd(REAL(x), nobs, INT(nparams), NA_REAL, TRUE, REAL(sd));

  }/*THEN*/
  else {

    nstrata = NLEVELS(strata);
    z = INTEGER(strata);
    PROTECT(sd = allocVector(REALSXP, nstrata));

    c_cgsd(REAL(x), z, NULL, nobs, nstrata, INT(nparams), NULL, REAL(sd));

  }/*ELSE*/

  UNPROTECT(1);

  return sd;

}/*CGSD*/
