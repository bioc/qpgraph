/*
  qpgraph package - this C code implements functions to learn qp-graphs from
                    data and utilities for GGM model inference and simulation
 
  Copyright (C) 2009 R. Castelo and A. Roverato
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, you can obtain one via WWW at
  http://www.gnu.org/copyleft/gpl.html, or by writing to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/



#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Lapack.h>
#include <R_ext/RS.h>
#include "cliquer.h"

/* constants */

#define E2I(v,w) (v > w ? ((int) (((double) ( v * (v - 1))) / 2.0)) + w : ((int) (((double) (w * (w - 1))) / 2.0)) + v) 
#define UTE2I(v,w) (v > w ? ((int) (((double) ( v * (v - 1))) / 2.0)) + w + v : ((int) (((double) (w * (w - 1))) / 2.0)) + v + w) 

/* datatype definitions */

typedef struct tag_clique_t {
  union {
    set_t vts; /* vertex set - vertices stored using cliquer sets */
    int * vta; /* vertex array - vertices stored as an array of integers */
  } u;
  int n;

  struct tag_clique_t* next;
} clique_t;
                                                                                                
typedef struct {
  clique_t* first;
  clique_t* last;
  int       n;
} clique_set_t;

/* function prototypes */

extern void R_FlushConsole(void);
extern void R_CheckUserInterrupt(void);
#ifdef Win32
extern void R_ProcessEvents(void);
#endif
#ifdef HAVE_AQUA
extern void R_ProcessEvents(void);
#endif

static SEXP
qp_fast_nrr(SEXP XR, SEXP qR, SEXP nTests, SEXP alpha, SEXP pairup_i_noint,
            SEXP pairup_j_noint, SEXP pairup_ij_int, SEXP verbose,
            SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP env);

static SEXP
qp_fast_nrr_identicalQs(SEXP XR, SEXP qR, SEXP nTests, SEXP alpha, SEXP pairup_i_noint,
                        SEXP pairup_j_noint, SEXP pairup_ij_int, SEXP verbose,
                        SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP env);

static SEXP
qp_fast_nrr_par(SEXP XR, SEXP qR, SEXP nTestsR, SEXP alphaR, SEXP pairup_i_nointR,
                SEXP pairup_j_nointR, SEXP pairup_ij_intR, SEXP verboseR, SEXP startTimeR,
                SEXP nAdj2estimateTimeR, SEXP myRankR, SEXP clSzeR, SEXP masterNode, SEXP env);
static SEXP
qp_fast_nrr_identicalQs_par(SEXP XR, SEXP qR, SEXP nTestsR, SEXP alphaR, SEXP pairup_i_nointR,
                            SEXP pairup_j_nointR, SEXP pairup_ij_intR, SEXP verboseR,
                            SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP myRankR, SEXP clSzeR,
                            SEXP masterNode, SEXP env);
static SEXP
qp_fast_edge_nrr(SEXP S, SEXP n_varR, SEXP NR, SEXP iR, SEXP jR, SEXP qR, SEXP TR, SEXP sigR);

static double
qp_edge_nrr(double* S, int n_var, int N, int i, int j, int q, int nTests, double alpha);

static double
qp_edge_nrr_identicalQs(double* S, int n_var, int* Qs, double* Qinvs, int N, int i,
                        int j, int q, int nTests, double alpha);
static void
sampleQs(int T, int q, int v_i, int v_j, int n, int* y);

static SEXP
qp_fast_ci_test(SEXP SR, SEXP n_varR, SEXP NR, SEXP iR, SEXP jR, SEXP C);
static SEXP
qp_fast_ci_test2(SEXP SR, SEXP n_varR, SEXP NR, SEXP iR, SEXP jR, SEXP C);

static double
qp_ci_test(double* S, int n_var, int N, int i, int j, int* C, int q);
static double
qp_ci_test2(double* S, int n_var, int N, int i, int j, int* C, int q, double*);


boolean
cliquer_cb_add_clique_to_list(set_t clique, graph_t* g, clique_options* opts);

void
add_clique_vts(clique_set_t* cset, set_t clique);

void
add_clique_vta(clique_set_t* cset, int* clique, int n);

void
destroy_cliques_vts(clique_set_t* cset);

void
destroy_cliques_vta(clique_set_t* cset);

void
init_cliques_list(clique_set_t* cset);

static SEXP
qp_fast_cliquer_get_cliques(SEXP I, SEXP clqspervtx, SEXP verbose);

Rboolean
is_maximal_clique(int* I, int n, int* clq, int cs, set_t noclq);

static SEXP
qp_fast_update_cliques_removing(SEXP I, SEXP clqlstR, SEXP vR, SEXP wR, SEXP verbose);

static SEXP
qp_clique_number_lb(SEXP I, SEXP return_vertices, SEXP approx_iter, SEXP verbose);

static SEXP
qp_clique_number_os(SEXP I, SEXP return_vertices, SEXP verbose);

int
clique_number_os(int n, int* I, int verbose);

static SEXP
qp_fast_pac_se(SEXP Shat, SEXP I);

static SEXP
qp_fast_ipf(SEXP vv, SEXP cliq, SEXP tol, SEXP verbose);

static void
fast_ipf_step(int n, double* Vf, double* Vn, int* a, int csze);

static void
cov2cor(double* R, double* V, int n);

static void
matprod(double *x, int nrx, int ncx, double *y, int nry, int ncy, double *z);
 
static void
matinv(double* inv, double* M, int n);

static void
matsumf(double* R, int nrow, int ncol, double* M, double* N, double factor);

static void
matscalarprod(double* R, int nrow, int ncol, double* M, double* N);

static void
mattran(double* T, double* M, int nrow, int ncol);

static void
matsubm(double* subM, double* M, int n, int* subrows,
        int nsubrows, int* subcols, int nsubcols);

static double
matmxab(double* M, int nrow, int ncol);

static void
matrepm(double* M, int n, int* subrows, int nsubrows,
        int* subcols, int nsubcols, double* N);

static void
setdiff(int n, int m, int* a, int* b);

void
i2e(int i, int* e_i, int* e_j);

int
e2i(int e_i, int e_j, int* i);

void
calculate_means(double* X, int n_var, int N, double* meanv);

static SEXP
qp_cov_upper_triangular(SEXP XR);

/* R-function register */

static R_CallMethodDef
callMethods[] = {
  {"qp_fast_nrr", (DL_FUNC) &qp_fast_nrr, 11},
  {"qp_fast_nrr_identicalQs", (DL_FUNC) &qp_fast_nrr_identicalQs, 11},
  {"qp_fast_nrr_par", (DL_FUNC) &qp_fast_nrr_par, 14},
  {"qp_fast_nrr_identicalQs_par", (DL_FUNC) &qp_fast_nrr_identicalQs_par, 14},
  {"qp_fast_edge_nrr", (DL_FUNC) &qp_fast_edge_nrr, 8},
  {"qp_fast_ci_test", (DL_FUNC) &qp_fast_ci_test,6},
  {"qp_fast_ci_test2", (DL_FUNC) &qp_fast_ci_test2,6},
  {"qp_fast_cliquer_get_cliques", (DL_FUNC) &qp_fast_cliquer_get_cliques, 3},
  {"qp_fast_update_cliques_removing", (DL_FUNC) &qp_fast_update_cliques_removing, 5},
  {"qp_clique_number_lb", (DL_FUNC) &qp_clique_number_lb, 4},
  {"qp_clique_number_os", (DL_FUNC) &qp_clique_number_os, 3},
  {"qp_fast_pac_se", (DL_FUNC) &qp_fast_pac_se, 2},
  {"qp_fast_ipf", (DL_FUNC) &qp_fast_ipf, 4},
  {"qp_cov_upper_triangular", (DL_FUNC) &qp_cov_upper_triangular, 1},
  {NULL}
};

void
R_init_qpgraph(DllInfo* info) {

  R_registerRoutines(info,NULL,callMethods,NULL,0);

  GetRNGstate(); /* initialize the R-builtin RNG */

}



/*
  FUNCTION: qp_fast_nrr
  PURPOSE: compute for each pair of vertices indexed by the columns of the
           matrix X the non-rejection rate. Vertex pairs may be restricted
           by using the pairup_* arguments
  RETURNS: matrix of non-rejection rate values in terms of number of non-rejected
           (accepted) tests for each pair of vertices
*/

static SEXP
qp_fast_nrr(SEXP XR, SEXP qR, SEXP nTestsR, SEXP alphaR, SEXP pairup_i_nointR,
            SEXP pairup_j_nointR, SEXP pairup_ij_intR, SEXP verboseR,
            SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP env) {
  int     N;
  int     n_var;
  int     q;
  int     nTests;
  double  alpha;
  SEXP    SR;
  double* S;
  int     l_ini = length(pairup_i_nointR);
  int     l_jni = length(pairup_j_nointR);
  int     l_int = length(pairup_ij_intR);
  int*    pairup_i_noint = INTEGER(pairup_i_nointR);
  int*    pairup_j_noint = INTEGER(pairup_j_nointR);
  int*    pairup_ij_int = INTEGER(pairup_ij_intR);
  int*    pairup_ij_noint = NULL;
  int     i,j,k,n_adj,pct,ppct;
  SEXP    nrrR;
  double* nrr;
  int     verbose;
  double  startTime, elapsedTime;
  int     nAdjEtime;

  PROTECT_INDEX Spi;

  N         = INTEGER(getAttrib(XR, R_DimSymbol))[0];
  n_var     = INTEGER(getAttrib(XR, R_DimSymbol))[1];
  q         = INTEGER(qR)[0];
  nTests    = INTEGER(nTestsR)[0];
  alpha     = REAL(alphaR)[0];
  verbose   = INTEGER(verboseR)[0];
  startTime = REAL(startTimeR)[0];
  nAdjEtime = INTEGER(nAdj2estimateTimeR)[0];

  if (q > n_var-2)
    error("q=%d > n.var-2=%d",q,n_var-2);

  if (q < 0)
    error("q=%d < 0",q);

  if (q > N-3)
    error("q=%d > N-3=%d", q, N-3);

  SR = qp_cov_upper_triangular(XR);
  PROTECT_WITH_INDEX(SR, &Spi);
  S = REAL(SR);

  if (l_ini + l_jni > 0) {
    pairup_ij_noint = Calloc(l_ini + l_jni, int);
    Memcpy(pairup_ij_noint, pairup_i_noint, (size_t) l_ini);
    Memcpy(pairup_ij_noint + l_ini, pairup_j_noint, (size_t) l_jni);
  }

  PROTECT(nrrR = allocVector(REALSXP, (n_var*(n_var-1))/2+n_var));
  nrr = REAL(nrrR);

  for (i=0;i < (n_var*(n_var-1))/2+n_var;i++)
    nrr[i] = NA_REAL;

  n_adj = l_int * (l_jni + l_ini) + l_ini * l_jni + l_int * (l_int - 1) / 2;

  elapsedTime = 0.0;
  if (startTime > 0.0) {
    SEXP procTimeR;
    double* procTime;
    SEXP call;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = procTime[2] - startTime;
    startTime = procTime[2];
    UNPROTECT(2); /* call procTimeR */
  }

  ppct = -1;
  k = 0;

  /* intersection variables against ij-exclusive variables */
  for (i=0; i < l_int; i++) {
    int i2 = pairup_ij_int[i] - 1;

    for (j=0; j < l_ini + l_jni; j++) {
      int j2 = pairup_ij_noint[j] - 1;

      nrr[UTE2I(i2, j2)] = qp_edge_nrr(S, n_var, N, i2, j2, q, nTests, alpha);
      k++;
      if (startTime > 0 && k == nAdjEtime)
        break;
      pct = (int) ((k * 100) / n_adj);
      if (pct != ppct) {
        if (verbose && startTime == 0) {
          if (pct % 10 == 0)
            Rprintf("%d",pct);
          else
            Rprintf(".",pct);
          R_FlushConsole();
        }
        R_CheckUserInterrupt();
#ifdef Win32
        R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
        R_ProcessEvents();
#endif
        ppct = pct;
      }
    }
    if (startTime > 0 && k == nAdjEtime)
      break;
  }

  if (l_ini + l_jni > 0)
    Free(pairup_ij_noint);

  /* i-exclusive variables against j-exclusive variables */
  if (startTime == 0 || k < 10) {
    for (i=0; i < l_ini; i++) {
      int i2 = pairup_i_noint[i] - 1;

      for (j=0; j < l_jni; j++) {
        int j2 = pairup_j_noint[j] - 1;

        nrr[UTE2I(i2, j2)] = qp_edge_nrr(S, n_var, N, i2, j2, q, nTests, alpha);
        k++;
        if (startTime > 0 && k == nAdjEtime)
          break;
        pct = (int) ((k * 100) / n_adj);
        if (pct != ppct) {
          if (verbose && startTime == 0) {
            if (pct % 10 == 0)
              Rprintf("%d",pct);
            else
              Rprintf(".",pct);
            R_FlushConsole();
          }
          R_CheckUserInterrupt();
#ifdef Win32
          R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
          R_ProcessEvents();
#endif
          ppct = pct;
        }
      }
      if (startTime > 0 && k == nAdjEtime)
        break;
    }
  }

  /* intersection variables against themselves (avoiding pairing the same) */
  if (startTime == 0 || k < 10) {
    for (i = 0; i < l_int-1; i++) {
      int i2 = pairup_ij_int[i] - 1;

      for (j = i+1; j < l_int; j++) {
        int j2 = pairup_ij_int[j] - 1;

        nrr[UTE2I(i2, j2)] = qp_edge_nrr(S, n_var, N, i2, j2, q, nTests, alpha);
        k++;
        if (startTime > 0 && k == nAdjEtime)
          break;
        pct = (int) ((k * 100) / n_adj);
        if (pct != ppct) {
          if (verbose && startTime == 0) {
            if (pct % 10 == 0)
              Rprintf("%d",pct);
            else
              Rprintf(".",pct);
            R_FlushConsole();
          }
          R_CheckUserInterrupt();
#ifdef Win32
          R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
          R_ProcessEvents();
#endif
          ppct = pct;
        }
      }
      if (startTime > 0 && k == nAdjEtime)
        break;
    }
  }

  if (verbose && startTime == 0)
    Rprintf("\n");

  UNPROTECT(2);   /* SR nrrR */

  if (startTime > 0) {
    SEXP procTimeR;
    double* procTime;
    SEXP nm;
    int* estimatedTime;
    SEXP call;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = elapsedTime + ((procTime[2] - startTime) / (double) k) * (double) n_adj;
    UNPROTECT(2); /* call procTimeR */

    PROTECT(nrrR = allocVector(INTSXP, 4));
    PROTECT(nm = allocVector(STRSXP, 4));
    estimatedTime = INTEGER(nrrR);
    estimatedTime[0] = (int) (elapsedTime / (24.0*3600.0));
    estimatedTime[1] = (int) ((elapsedTime - estimatedTime[0]*24.0*3600.0) / 3600.0);
    estimatedTime[2] = (int) ((elapsedTime - estimatedTime[0]*24.0*3600.0 -
                               estimatedTime[1]*3600.0) / 60.0);
    estimatedTime[3] = (int) (elapsedTime - estimatedTime[0]*24.0*3600.0 -
                              estimatedTime[1]*3600.0 - estimatedTime[2]*60.0 + 1.0);
    SET_STRING_ELT(nm, 0, mkChar("days"));
    SET_STRING_ELT(nm, 1, mkChar("hours"));
    SET_STRING_ELT(nm, 2, mkChar("minutes"));
    SET_STRING_ELT(nm, 3, mkChar("seconds"));
    setAttrib(nrrR, R_NamesSymbol, nm);

    UNPROTECT(2); /* nrrR nm */
  }

  return nrrR;
}



/*
  FUNCTION: qp_fast_nrr_identicalQs
  PURPOSE: compute for each pair of vertices indexed by the rows (columns)
           of the matrix S the non-rejection rate using a common set of Q sets
           for all vertex pairs considered. Vertex pairs may be restricted
           by using the pairup_* arguments
  RETURNS: matrix of non-rejection rate values in terms of number of non-rejected
           (accepted) tests for each pair of vertices
*/

static SEXP
qp_fast_nrr_identicalQs(SEXP XR, SEXP qR, SEXP nTestsR, SEXP alphaR, SEXP pairup_i_nointR,
                        SEXP pairup_j_nointR, SEXP pairup_ij_intR, SEXP verboseR,
                        SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP env) {
  int     N;
  int     n_var;
  int     q;
  int     nTests;
  double  alpha;
  SEXP    SR;
  double* S;
  int     l_ini = length(pairup_i_nointR);
  int     l_jni = length(pairup_j_nointR);
  int     l_int = length(pairup_ij_intR);
  int*    pairup_i_noint = INTEGER(pairup_i_nointR);
  int*    pairup_j_noint = INTEGER(pairup_j_nointR);
  int*    pairup_ij_int = INTEGER(pairup_ij_intR);
  int*    pairup_ij_noint = NULL;
  int     i,j,k,n_adj,pct,ppct;
  int*    q_by_T_samples;
  int*    Q;
  double* Qmat;
  double* Qinv;
  SEXP    nrrR;
  double* nrr;
  int     verbose;
  double  startTime, elapsedTime;
  int     nAdjEtime;

  PROTECT_INDEX Spi;

  N       = INTEGER(getAttrib(XR, R_DimSymbol))[0];
  n_var   = INTEGER(getAttrib(XR, R_DimSymbol))[1];
  q       = INTEGER(qR)[0];
  nTests  = INTEGER(nTestsR)[0];
  alpha   = REAL(alphaR)[0];
  verbose = INTEGER(verboseR)[0];
  startTime = REAL(startTimeR)[0];
  nAdjEtime = INTEGER(nAdj2estimateTimeR)[0];

  if (q > n_var-2)
    error("q=%d > n.var-2=%d",q,n_var-2);

  if (q < 0)
    error("q=%d < 0",q);

  if (q > N-3)
    error("q=%d > N-3=%d", q, N-3);

  SR = qp_cov_upper_triangular(XR);
  PROTECT_WITH_INDEX(SR, &Spi);
  S = REAL(SR);

  /* sample the Q sets and pre-calculate the inverse matrices */

  q_by_T_samples = Calloc(q * nTests, int);

  sampleQs(nTests, q, -1, -1, n_var, q_by_T_samples);

  Qmat = Calloc(q*q, double);
  Qinv = Calloc(q*q*nTests, double);

  /*
  for (i=0; i < nTests; i++) {
    Q = (int*) (q_by_T_samples+i*q);
    for (j=0; j < q; j++) {
      for (k=0; k < j; k++)
        Qmat[j + k*q] = Qmat[k + j*q] = S[Q[j] + Q[k] * n_var];
      Qmat[j + j*q] = S[Q[j] + Q[j] * n_var];
    }
    matinv((double*) (Qinv+i*q*q), Qmat, q);
  }
  */
  for (i=0; i < nTests; i++) {
    Q = (int*) (q_by_T_samples+i*q);
    for (j=0; j < q; j++) {
      for (k=0; k < j; k++)
        Qmat[j + k*q] = Qmat[k + j*q] = S[UTE2I(Q[j], Q[k])];
      Qmat[j + j*q] = S[UTE2I(Q[j], Q[j])];
    }
    matinv((double*) (Qinv+i*q*q), Qmat, q);
  }
  Free(Qmat);
  
  if (l_ini + l_jni > 0) {
    pairup_ij_noint = Calloc(l_ini + l_jni, int);
    Memcpy(pairup_ij_noint, pairup_i_noint, (size_t) l_ini);
    Memcpy(pairup_ij_noint + l_ini, pairup_j_noint, (size_t) l_jni);
  }

  PROTECT(nrrR = allocVector(REALSXP, (n_var*(n_var-1))/2+n_var));
  nrr = REAL(nrrR);

  for (i=0;i < (n_var*(n_var-1))/2+n_var;i++)
    nrr[i] = NA_REAL;

  n_adj = l_int * (l_jni + l_ini) + l_ini * l_jni + l_int * (l_int - 1) / 2;

  elapsedTime = 0.0;
  if (startTime > 0.0) {
    SEXP procTimeR;
    double* procTime;
    SEXP call;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = procTime[2] - startTime;
    startTime = procTime[2];
    UNPROTECT(2); /* call procTimeR */
  }

  ppct = -1;
  k = 0;

  /* intersection variables against ij-exclusive variables */
  for (i=0; i < l_int; i++) {
    int i2 = pairup_ij_int[i] - 1;

    for (j=0; j < l_ini + l_jni; j++) {
      int j2 = pairup_ij_noint[j] - 1;

      nrr[UTE2I(i2, j2)] = qp_edge_nrr_identicalQs(S, n_var, q_by_T_samples, Qinv,
                                                   N, i2, j2, q, nTests, alpha);
      k++;
      if (startTime > 0 && k == nAdjEtime)
        break;
      pct = (int) ((k * 100) / n_adj);
      if (pct != ppct) {
        if (verbose && startTime == 0) {
          if (pct % 10 == 0)
            Rprintf("%d",pct);
          else
            Rprintf(".",pct);
          R_FlushConsole();
        }
        R_CheckUserInterrupt();
#ifdef Win32
        R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
        R_ProcessEvents();
#endif
        ppct = pct;
      }
    }
    if (startTime > 0 && k == nAdjEtime)
      break;
  }

  if (l_ini + l_jni > 0)
    Free(pairup_ij_noint);

  /* i-exclusive variables against j-exclusive variables */
  if (startTime == 0 || k < 10) {
    for (i=0; i < l_ini; i++) {
      int i2 = pairup_i_noint[i] - 1;

      for (j=0; j < l_jni; j++) {
        int j2 = pairup_j_noint[j] - 1;

        nrr[UTE2I(i2, j2)] = qp_edge_nrr_identicalQs(S, n_var, q_by_T_samples, Qinv,
                                                     N, i2, j2, q, nTests, alpha);
        k++;
        if (startTime > 0 && k == nAdjEtime)
          break;
        pct = (int) ((k * 100) / n_adj);
        if (pct != ppct) {
          if (verbose && startTime == 0) {
            if (pct % 10 == 0)
              Rprintf("%d",pct);
            else
              Rprintf(".",pct);
            R_FlushConsole();
          }
          R_CheckUserInterrupt();
#ifdef Win32
          R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
          R_ProcessEvents();
#endif
          ppct = pct;
        }
      }
      if (startTime > 0 && k == nAdjEtime)
        break;
    }
  }

  /* intersection variables against themselves (avoiding pairing the same) */
  if (startTime == 0 || k < 10) {
    for (i = 0; i < l_int-1; i++) {
      int i2 = pairup_ij_int[i] - 1;

      for (j = i+1; j < l_int; j++) {
        int j2 = pairup_ij_int[j] - 1;

        nrr[UTE2I(i2, j2)] = qp_edge_nrr_identicalQs(S, n_var, q_by_T_samples, Qinv,
                                                     N, i2, j2, q, nTests, alpha);
        k++;
        if (startTime > 0 && k == nAdjEtime)
          break;
        pct = (int) ((k * 100) / n_adj);
        if (pct != ppct) {
          if (verbose && startTime == 0) {
            if (pct % 10 == 0)
              Rprintf("%d",pct);
            else
              Rprintf(".",pct);
            R_FlushConsole();
          }
          R_CheckUserInterrupt();
#ifdef Win32
          R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
          R_ProcessEvents();
#endif
          ppct = pct;
        }
      }
      if (startTime > 0 && k == nAdjEtime)
        break;
    }
  }

  Free(Qinv);

  if (verbose && startTime == 0)
    Rprintf("\n");

  UNPROTECT(2);   /* SR nrrR */

  if (startTime > 0) {
    SEXP procTimeR;
    double* procTime;
    SEXP nm;
    int* estimatedTime;
    SEXP call;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = elapsedTime + ((procTime[2] - startTime) / (double) k) * (double) n_adj;
    UNPROTECT(2); /* call procTimeR */

    PROTECT(nrrR = allocVector(INTSXP, 4));
    PROTECT(nm = allocVector(STRSXP, 4));
    estimatedTime = INTEGER(nrrR);
    estimatedTime[0] = (int) (elapsedTime / (24.0*3600.0));
    estimatedTime[1] = (int) ((elapsedTime - estimatedTime[0]*24.0*3600.0) / 3600.0);
    estimatedTime[2] = (int) ((elapsedTime - estimatedTime[0]*24.0*3600.0 -
                               estimatedTime[1]*3600.0) / 60.0);
    estimatedTime[3] = (int) (elapsedTime - estimatedTime[0]*24.0*3600.0 -
                              estimatedTime[1]*3600.0 - estimatedTime[2]*60.0 + 1.0);
    SET_STRING_ELT(nm, 0, mkChar("days"));
    SET_STRING_ELT(nm, 1, mkChar("hours"));
    SET_STRING_ELT(nm, 2, mkChar("minutes"));
    SET_STRING_ELT(nm, 3, mkChar("seconds"));
    setAttrib(nrrR, R_NamesSymbol, nm);

    UNPROTECT(2); /* nrrR nm */
  }

  return nrrR;
}



/*
  FUNCTION: qp_fast_nrr_par
  PURPOSE: compute for each pair of vertices indexed by the rows (columns)
           of the matrix S the non-rejection rate. Vertex pairs may be restricted
           by using the pairup_* arguments. This function should be called only
           within a parallel environment running in a cluster where arguments
           myRankR and clSzeR tell how many nodes form the cluster (clSzeR) and
           which is the node running the function (myRankR)
  RETURNS: matrix of non-rejection rate values in terms of number of non-rejected
           (accepted) tests for each pair of vertices
*/

static SEXP
qp_fast_nrr_par(SEXP XR, SEXP qR, SEXP nTestsR, SEXP alphaR, SEXP pairup_i_nointR,
                SEXP pairup_j_nointR, SEXP pairup_ij_intR, SEXP verboseR,
                SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP myRankR, SEXP clSzeR,
                SEXP masterNode, SEXP env) {
  int     N;
  int     n_var;
  int     q;
  int     nTests;
  double  alpha;
  SEXP    SR;
  double* S;
  int     l_ini = length(pairup_i_nointR);
  int     l_jni = length(pairup_j_nointR);
  int     l_int = length(pairup_ij_intR);
  int*    pairup_i_noint = INTEGER(pairup_i_nointR);
  int*    pairup_j_noint = INTEGER(pairup_j_nointR);
  int*    pairup_ij_int = INTEGER(pairup_ij_intR);
  int*    pairup_ij_noint = NULL;
  int     i,j,k,n_adj,n_adj_this_proc,pct,ppct;
  SEXP    nrrR, idxR;
  SEXP    result, result_names;
  double* nrr;
  int*    idx;
  int     verbose;
  int     myrank;
  int     clsze;
  int     firstAdj, lastAdj;
  double  startTime, elapsedTime;
  int     nAdjEtime;
  SEXP    progressReport,progressReportType,
          progressReportValue,progressReportSuccess,
          progressReportTag,progressReport_names;

  PROTECT(progressReport = allocVector(VECSXP,4));
  SET_VECTOR_ELT(progressReport,0,progressReportType = allocVector(STRSXP,1));
  SET_VECTOR_ELT(progressReport,1,progressReportValue = allocVector(INTSXP,1));
  SET_VECTOR_ELT(progressReport,2,progressReportSuccess = allocVector(LGLSXP,1));
  SET_VECTOR_ELT(progressReport,3,progressReportTag = allocVector(STRSXP,1));
  PROTECT(progressReport_names = allocVector(STRSXP,4));
  SET_STRING_ELT(progressReport_names,0,mkChar("type"));
  SET_STRING_ELT(progressReport_names,1,mkChar("value"));
  SET_STRING_ELT(progressReport_names,2,mkChar("success"));
  SET_STRING_ELT(progressReport_names,3,mkChar("tag"));
  setAttrib(progressReport,R_NamesSymbol,progressReport_names);
  SET_STRING_ELT(VECTOR_ELT(progressReport,0), 0, mkChar("VALUE"));
  INTEGER(VECTOR_ELT(progressReport,1))[0] = 0;
  LOGICAL(VECTOR_ELT(progressReport,2))[0] = TRUE;
  SET_STRING_ELT(VECTOR_ELT(progressReport,3), 0, mkChar("UPDATE"));

  PROTECT_INDEX Spi;

  N         = INTEGER(getAttrib(XR, R_DimSymbol))[0];
  n_var     = INTEGER(getAttrib(XR, R_DimSymbol))[1];
  q         = INTEGER(qR)[0];
  nTests    = INTEGER(nTestsR)[0];
  alpha     = REAL(alphaR)[0];
  verbose   = INTEGER(verboseR)[0];
  startTime = REAL(startTimeR)[0];
  nAdjEtime = INTEGER(nAdj2estimateTimeR)[0];
  myrank    = INTEGER(myRankR)[0];
  clsze     = INTEGER(clSzeR)[0];

  if (q > n_var-2)
    error("q=%d > n.var-2=%d",q,n_var-2);

  if (q < 0)
    error("q=%d < 0",q);

  if (q > N-3)
    error("q=%d > N-3=%d", q, N-3);

  SR = qp_cov_upper_triangular(XR);
  PROTECT_WITH_INDEX(SR, &Spi);
  S = REAL(SR);

  n_adj = l_int * (l_jni + l_ini) + l_ini * l_jni + l_int * (l_int - 1) / 2;

  if (l_ini + l_jni > 0) {
    pairup_ij_noint = Calloc(l_ini + l_jni, int);
    Memcpy(pairup_ij_noint, pairup_i_noint, (size_t) l_ini);
    Memcpy(pairup_ij_noint + l_ini, pairup_j_noint, (size_t) l_jni);
  }

  firstAdj = (myrank-1) * (n_adj / clsze);
  lastAdj  = myrank * (n_adj / clsze);

  if (myrank == clsze)
    lastAdj += n_adj - lastAdj;

  lastAdj--;

  n_adj_this_proc = lastAdj - firstAdj + 1;

  PROTECT(result = allocVector(VECSXP,2));
  SET_VECTOR_ELT(result, 0, nrrR = allocVector(REALSXP, lastAdj-firstAdj+1));
  SET_VECTOR_ELT(result, 1, idxR = allocVector(INTSXP, lastAdj-firstAdj+1));
  PROTECT(result_names = allocVector(STRSXP, 2));
  SET_STRING_ELT(result_names, 0, mkChar("nrr"));
  SET_STRING_ELT(result_names, 1, mkChar("idx"));
  setAttrib(result, R_NamesSymbol, result_names);
  nrr = REAL(VECTOR_ELT(result, 0));
  idx = INTEGER(VECTOR_ELT(result, 1));

  elapsedTime = 0.0;
  if (startTime > 0.0) {
    SEXP procTimeR;
    double* procTime;
    SEXP call;

    /* initialize 'idx' so that the R code copying the result works as
     * in a normal execution */
    for (k=0; k < n_adj_this_proc; k++)
      idx[k] = firstAdj + k + 1;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = procTime[2] - startTime;
    startTime = procTime[2];
    UNPROTECT(2); /* call procTimeR */
  }

  k = firstAdj;
  ppct = -1;

  if (k < l_int * (l_ini + l_jni)) {
    int j_first = k % (l_ini + l_jni);

    /* intersection variables against ij-exclusive variables */
    for (i=((int) (k/(l_ini + l_jni))); i < l_int && k <= lastAdj; i++) {
      int i2 = pairup_ij_int[i] - 1;

      for (j=j_first; j < l_ini + l_jni && k <= lastAdj; j++) {
        int j2 = pairup_ij_noint[j] - 1;

        nrr[k-firstAdj] = qp_edge_nrr(S, n_var, N, i2, j2, q, nTests, alpha);
        idx[k-firstAdj] = UTE2I(i2, j2) + 1;
        k++;
        if (startTime > 0 && k-firstAdj == 10)
          break;
        if (verbose && startTime == 0) {
          pct = (int) (((k-firstAdj) * 100) / n_adj_this_proc);
          if (pct != ppct) {
            SEXP s, t;
            PROTECT(t = s = allocList(3));
            SET_TYPEOF(s, LANGSXP);
            SETCAR(t, install("sendData")); t=CDR(t);
            SETCAR(t, masterNode);
            SET_TAG(t, install("node")); t=CDR(t);
            INTEGER(VECTOR_ELT(progressReport,1))[0] = k-firstAdj;
            SETCAR(t, progressReport);
            SET_TAG(t, install("data"));
            eval(s, env);
            UNPROTECT(1); /* t s */
          }
          ppct = pct;
        }
      }
      if (startTime > 0 && k-firstAdj == 10)
        break;
      j_first = 0;
    }
  }

  if (l_ini + l_jni > 0)
    Free(pairup_ij_noint);

  if (k <= lastAdj && k < l_int * (l_ini + l_jni) + l_ini * l_jni &&
      (startTime == 0 || k-firstAdj < 10)) {
    int i_first = ((int) ((k - l_int * (l_ini + l_jni)) / l_jni));
    int j_first = (k - l_int * (l_ini + l_jni)) % l_jni;

    /* i-exclusive variables against j-exclusive variables */
    for (i=i_first; i < l_ini && k <= lastAdj; i++) {
      int i2 = pairup_i_noint[i] - 1;

      for (j=j_first; j < l_jni && k <= lastAdj; j++) {
        int j2 = pairup_j_noint[j] - 1;

        nrr[k-firstAdj] = qp_edge_nrr(S, n_var, N, i2, j2, q, nTests, alpha);
        idx[k-firstAdj] = UTE2I(i2, j2) + 1;
        k++;
        if (startTime > 0 && k-firstAdj == 10)
          break;
        if (verbose && startTime == 0) {
          pct = (int) (((k-firstAdj) * 100) / n_adj_this_proc);
          if (pct != ppct) {
            SEXP s, t;
            PROTECT(t = s = allocList(3));
            SET_TYPEOF(s, LANGSXP);
            SETCAR(t, install("sendData")); t=CDR(t);
            SETCAR(t, masterNode);
            SET_TAG(t, install("node")); t=CDR(t);
            INTEGER(VECTOR_ELT(progressReport,1))[0] = k-firstAdj;
            SETCAR(t, progressReport);
            SET_TAG(t, install("data"));
            eval(s, env);
            UNPROTECT(1); /* t s */
          }
          ppct = pct;
        }
      }
      if (startTime > 0 && k-firstAdj == 10)
        break;
      j_first = 0;
    }
  }

  if (k <= lastAdj && (startTime == 0 || k-firstAdj < 10)) {
    int i_first = k - l_int * (l_ini + l_jni) - l_ini * l_jni;
    int l;

    /* intersection variables against themselves (avoiding pairing the same) */
    for (l = i_first; l < (l_int * (l_int - 1)) / 2 && k <= lastAdj; l++) {
      int i,j,i2,j2;
      i2e(l, &i, &j);

      i2 = pairup_ij_int[i] - 1;
      j2 = pairup_ij_int[j] - 1;

      nrr[k-firstAdj] = qp_edge_nrr(S, n_var, N, i2, j2, q, nTests, alpha);
      idx[k-firstAdj] = UTE2I(i2, j2) + 1;
      k++;
      if (startTime > 0 && k-firstAdj == 10)
        break;
      if (verbose && startTime == 0) {
        pct = (int) (((k-firstAdj) * 100) / n_adj_this_proc);
        if (pct != ppct) {
          SEXP s, t;
          PROTECT(t = s = allocList(3));
          SET_TYPEOF(s, LANGSXP);
          SETCAR(t, install("sendData")); t=CDR(t);
          SETCAR(t, masterNode);
          SET_TAG(t, install("node")); t=CDR(t);
          INTEGER(VECTOR_ELT(progressReport,1))[0] = k-firstAdj;
          SETCAR(t, progressReport);
          SET_TAG(t, install("data"));
          eval(s, env);
          UNPROTECT(1); /* t s */
        }
        ppct = pct;
      }
    }
  }

  if (startTime > 0) {
    SEXP procTimeR;
    double* procTime;
    SEXP call;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = elapsedTime + ((procTime[2] - startTime) / (double) (k-firstAdj)) * (double) n_adj_this_proc;
    UNPROTECT(2); /* call procTimeR */

    nrr[0] = elapsedTime; /* store in the first position of the nrr vector the estimated time */
  }

  UNPROTECT(5);   /* SR result result_names progressReport progressReport_names */

  return result;
}



/*
  FUNCTION: qp_fast_nrr_identicalQs_par
  PURPOSE: compute for each pair of vertices indexed by the rows (columns)
           of the matrix S the non-rejection rate using a common set of Q sets
           for all vertex pairs considered. Vertex pairs may be restricted
           by using the pairup_* arguments
  RETURNS: matrix of non-rejection rate values in terms of number of non-rejected
           (accepted) tests for each pair of vertices
*/

static SEXP
qp_fast_nrr_identicalQs_par(SEXP XR, SEXP qR, SEXP nTestsR, SEXP alphaR, SEXP pairup_i_nointR,
                            SEXP pairup_j_nointR, SEXP pairup_ij_intR, SEXP verboseR,
                            SEXP startTimeR, SEXP nAdj2estimateTimeR, SEXP myRankR, SEXP clSzeR,
                            SEXP masterNode, SEXP env) {
  int     N;
  int     n_var;
  int     q;
  int     nTests;
  double  alpha;
  SEXP    SR;
  double* S;
  int     l_ini = length(pairup_i_nointR);
  int     l_jni = length(pairup_j_nointR);
  int     l_int = length(pairup_ij_intR);
  int*    pairup_i_noint = INTEGER(pairup_i_nointR);
  int*    pairup_j_noint = INTEGER(pairup_j_nointR);
  int*    pairup_ij_int = INTEGER(pairup_ij_intR);
  int*    pairup_ij_noint = NULL;
  int     i,j,k,n_adj,n_adj_this_proc,pct,ppct;
  int*    q_by_T_samples;
  int*    Q;
  double* Qmat;
  double* Qinv;
  SEXP    nrrR, idxR;
  SEXP    result, result_names;
  double* nrr;
  int*    idx;
  int     verbose;
  int     myrank;
  int     clsze;
  int     firstAdj, lastAdj;
  double  startTime, elapsedTime;
  int     nAdjEtime;
  SEXP    progressReport,progressReportType,
          progressReportValue,progressReportSuccess,
          progressReportTag,progressReport_names;

  PROTECT(progressReport = allocVector(VECSXP,4));
  SET_VECTOR_ELT(progressReport,0,progressReportType = allocVector(STRSXP,1));
  SET_VECTOR_ELT(progressReport,1,progressReportValue = allocVector(INTSXP,1));
  SET_VECTOR_ELT(progressReport,2,progressReportSuccess = allocVector(LGLSXP,1));
  SET_VECTOR_ELT(progressReport,3,progressReportTag = allocVector(STRSXP,1));
  PROTECT(progressReport_names = allocVector(STRSXP,4));
  SET_STRING_ELT(progressReport_names,0,mkChar("type"));
  SET_STRING_ELT(progressReport_names,1,mkChar("value"));
  SET_STRING_ELT(progressReport_names,2,mkChar("success"));
  SET_STRING_ELT(progressReport_names,3,mkChar("tag"));
  setAttrib(progressReport,R_NamesSymbol,progressReport_names);
  SET_STRING_ELT(VECTOR_ELT(progressReport,0), 0, mkChar("VALUE"));
  INTEGER(VECTOR_ELT(progressReport,1))[0] = 0;
  LOGICAL(VECTOR_ELT(progressReport,2))[0] = TRUE;
  SET_STRING_ELT(VECTOR_ELT(progressReport,3), 0, mkChar("UPDATE"));

  PROTECT_INDEX Spi;

  N         = INTEGER(getAttrib(XR, R_DimSymbol))[0];
  n_var     = INTEGER(getAttrib(XR, R_DimSymbol))[1];
  q         = INTEGER(qR)[0];
  nTests    = INTEGER(nTestsR)[0];
  alpha     = REAL(alphaR)[0];
  verbose   = INTEGER(verboseR)[0];
  startTime = REAL(startTimeR)[0];
  nAdjEtime = INTEGER(nAdj2estimateTimeR)[0];
  myrank    = INTEGER(myRankR)[0];
  clsze     = INTEGER(clSzeR)[0];

  if (q > n_var-2)
    error("q=%d > n.var-2=%d",q,n_var-2);

  if (q < 0)
    error("q=%d < 0",q);

  if (q > N-3)
    error("q=%d > N-3=%d", q, N-3);

  SR = qp_cov_upper_triangular(XR);
  PROTECT_WITH_INDEX(SR, &Spi);
  S = REAL(SR);

  n_adj = l_int * (l_jni + l_ini) + l_ini * l_jni + l_int * (l_int - 1) / 2;

  /* sample the Q sets and pre-calculate the inverse matrices */

  q_by_T_samples = Calloc(q * nTests, int);

  sampleQs(nTests, q, -1, -1, n_var, q_by_T_samples);

  Qmat = Calloc(q*q, double);
  Qinv = Calloc(q*q*nTests, double);

  /*
  for (i=0; i < nTests; i++) {
    Q = (int*) (q_by_T_samples+i*q);
    for (j=0; j < q; j++) {
      for (k=0; k < j; k++)
        Qmat[j + k*q] = Qmat[k + j*q] = S[Q[j] + Q[k] * n_var];
      Qmat[j + j*q] = S[Q[j] + Q[j] * n_var];
    }
    matinv((double*) (Qinv+i*q*q), Qmat, q);
  }
  */
  for (i=0; i < nTests; i++) {
    Q = (int*) (q_by_T_samples+i*q);
    for (j=0; j < q; j++) {
      for (k=0; k < j; k++)
        Qmat[j + k*q] = Qmat[k + j*q] = S[UTE2I(Q[j], Q[k])];
      Qmat[j + j*q] = S[UTE2I(Q[j], Q[j])];
    }
    matinv((double*) (Qinv+i*q*q), Qmat, q);
  }
  Free(Qmat);
  
  if (l_ini + l_jni > 0) {
    pairup_ij_noint = Calloc(l_ini + l_jni, int);
    Memcpy(pairup_ij_noint, pairup_i_noint, (size_t) l_ini);
    Memcpy(pairup_ij_noint + l_ini, pairup_j_noint, (size_t) l_jni);
  }

  firstAdj = (myrank-1) * (n_adj / clsze);
  lastAdj  = myrank * (n_adj / clsze);

  if (myrank == clsze)
    lastAdj += n_adj - lastAdj;

  lastAdj--;

  n_adj_this_proc = lastAdj - firstAdj + 1;

  PROTECT(result = allocVector(VECSXP,2));
  SET_VECTOR_ELT(result, 0, nrrR = allocVector(REALSXP, lastAdj-firstAdj+1));
  SET_VECTOR_ELT(result, 1, idxR = allocVector(INTSXP, lastAdj-firstAdj+1));
  PROTECT(result_names = allocVector(STRSXP, 2));
  SET_STRING_ELT(result_names, 0, mkChar("nrr"));
  SET_STRING_ELT(result_names, 1, mkChar("idx"));
  setAttrib(result, R_NamesSymbol, result_names);
  nrr = REAL(VECTOR_ELT(result, 0));
  idx = INTEGER(VECTOR_ELT(result, 1));

  elapsedTime = 0.0;
  if (startTime > 0.0) {
    SEXP procTimeR;
    double* procTime;
    SEXP call;

    /* initialize 'idx' so that the R code copying the result works as
     * in a normal execution */
    for (k=0; k < n_adj_this_proc; k++)
      idx[k] = firstAdj + k + 1;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = procTime[2] - startTime;
    startTime = procTime[2];
    UNPROTECT(2); /* call procTimeR */
  }

  k = firstAdj;
  ppct = -1;

  if (k < l_int * (l_ini + l_jni)) {
    int j_first = k % (l_ini + l_jni);

    /* intersection variables against ij-exclusive variables */
    for (i=((int) (k/(l_ini + l_jni))); i < l_int && k <= lastAdj; i++) {
      int i2 = pairup_ij_int[i] - 1;

      for (j=j_first; j < l_ini + l_jni && k <= lastAdj; j++) {
        int j2 = pairup_ij_noint[j] - 1;

        nrr[k-firstAdj] = qp_edge_nrr_identicalQs(S, n_var, q_by_T_samples, Qinv,
                                                  N, i2, j2, q, nTests, alpha);
        idx[k-firstAdj] = UTE2I(i2, j2) + 1;
        k++;
        if (startTime > 0 && k-firstAdj == 10)
          break;
        if (verbose && startTime == 0) {
          pct = (int) (((k-firstAdj) * 100) / n_adj_this_proc);
          if (pct != ppct) {
            SEXP s, t;
            PROTECT(t = s = allocList(3));
            SET_TYPEOF(s, LANGSXP);
            SETCAR(t, install("sendData")); t=CDR(t);
            SETCAR(t, masterNode);
            SET_TAG(t, install("node")); t=CDR(t);
            INTEGER(VECTOR_ELT(progressReport,1))[0] = k-firstAdj;
            SETCAR(t, progressReport);
            SET_TAG(t, install("data"));
            eval(s, env);
            UNPROTECT(1);
          }
          ppct = pct;
        }
      }
      if (startTime > 0 && k-firstAdj == 10)
        break;
      j_first = 0;
    }
  }

  if (l_ini + l_jni > 0)
    Free(pairup_ij_noint);

  if (k <= lastAdj && k < l_int * (l_ini + l_jni) + l_ini * l_jni &&
      (startTime == 0 || k-firstAdj < 10)) {
    int i_first = ((int) ((k - l_int * (l_ini + l_jni)) / l_jni));
    int j_first = (k - l_int * (l_ini + l_jni)) % l_jni;

    /* i-exclusive variables against j-exclusive variables */
    for (i=i_first; i < l_ini && k <= lastAdj; i++) {
      int i2 = pairup_i_noint[i] - 1;

      for (j=j_first; j < l_jni && k <= lastAdj; j++) {
        int j2 = pairup_j_noint[j] - 1;

        nrr[k-firstAdj] = qp_edge_nrr_identicalQs(S, n_var, q_by_T_samples, Qinv,
                                                  N, i2, j2, q, nTests, alpha);
        idx[k-firstAdj] = UTE2I(i2, j2) + 1;
        k++;
        if (startTime > 0 && k-firstAdj == 10)
          break;
        if (verbose && startTime == 0) {
          pct = (int) (((k-firstAdj) * 100) / n_adj_this_proc);
          if (pct != ppct) {
            SEXP s, t;
            PROTECT(t = s = allocList(3));
            SET_TYPEOF(s, LANGSXP);
            SETCAR(t, install("sendData")); t=CDR(t);
            SETCAR(t, masterNode);
            SET_TAG(t, install("node")); t=CDR(t);
            INTEGER(VECTOR_ELT(progressReport,1))[0] = k-firstAdj;
            SETCAR(t, progressReport);
            SET_TAG(t, install("data"));
            eval(s, env);
            UNPROTECT(1);
          }
          ppct = pct;
        }
      }
      if (startTime > 0 && k-firstAdj == 10)
        break;
      j_first = 0;
    }
  }

  if (k <= lastAdj && (startTime == 0 || k-firstAdj < 10)) {
    int i_first = k - l_int * (l_ini + l_jni) - l_ini * l_jni;
    int l;

    /* intersection variables against themselves (avoiding pairing the same) */
    for (l = i_first; l < (l_int * (l_int - 1)) / 2 && k <= lastAdj; l++) {
      int i,j,i2,j2;
      i2e(l, &i, &j);

      i2 = pairup_ij_int[i] - 1;
      j2 = pairup_ij_int[j] - 1;

      nrr[k-firstAdj] = qp_edge_nrr_identicalQs(S, n_var, q_by_T_samples, Qinv,
                                                N, i2, j2, q, nTests, alpha);
      idx[k-firstAdj] = UTE2I(i2, j2) + 1;
      k++;
      if (startTime > 0 && k-firstAdj == 10)
        break;
      if (verbose && startTime == 0) {
        pct = (int) (((k-firstAdj) * 100) / n_adj_this_proc);
        if (pct != ppct) {
          SEXP s, t;
          PROTECT(t = s = allocList(3));
          SET_TYPEOF(s, LANGSXP);
          SETCAR(t, install("sendData")); t=CDR(t);
          SETCAR(t, masterNode);
          SET_TAG(t, install("node")); t=CDR(t);
          INTEGER(VECTOR_ELT(progressReport,1))[0] = k-firstAdj;
          SETCAR(t, progressReport);
          SET_TAG(t, install("data"));
          eval(s, env);
          UNPROTECT(1);
        }
        ppct = pct;
      }
    }
  }

  Free(Qinv);

  if (startTime > 0) {
    SEXP procTimeR;
    double* procTime;
    SEXP call;

    PROTECT(call = lang1(install("proc.time")));
    PROTECT(procTimeR = eval(call, env));
    procTime = REAL(procTimeR);
    elapsedTime = elapsedTime + ((procTime[2] - startTime) / (double) (k-firstAdj)) * (double) n_adj_this_proc;
    UNPROTECT(2); /* call procTimeR */

    nrr[0] = elapsedTime; /* store in the first position of the nrr vector the estimated time */
  }

  UNPROTECT(5);   /* SR result result_names progressReport progressReport_names */

  return result;
}



/*
  FUNCTION: qp_fast_ci_test
  PURPOSE: wrapper of the R-C interface for calling the function that performs
           a test for conditional independence between variables i and j
           given de conditioning set C
  RETURNS: a list with two members, the t-statistic value and the p-value
           on rejecting the null hypothesis of independence
*/

static SEXP
qp_fast_ci_test(SEXP SR, SEXP n_varR, SEXP NR, SEXP iR, SEXP jR, SEXP C) {
  int    N = INTEGER(NR)[0];
  int    n_var = INTEGER(n_varR)[0];
  int    q;
  int*   cond;
  int    i,j,k;
  double p_value;
  double t_value;
  SEXP   result;
  SEXP   result_names;
  SEXP   result_t_val;
  SEXP   result_p_val;

  PROTECT_INDEX Spi,Cpi;

  PROTECT_WITH_INDEX(SR,&Spi);
  PROTECT_WITH_INDEX(C,&Cpi);

  REPROTECT(SR = coerceVector(SR,REALSXP),Spi);
  REPROTECT(C = coerceVector(C,INTSXP),Cpi);

  i = INTEGER(iR)[0] - 1;
  j = INTEGER(jR)[0] - 1;
  q = length(C);

  cond = Calloc(q, int);
  for (k=0;k<q;k++)
    cond[k] = INTEGER(C)[k]-1;

  t_value = qp_ci_test(REAL(SR),n_var,N,i,j,cond,q);
  p_value = 2.0 * (1.0 - pt(fabs(t_value),N-q-2,1,0));

  PROTECT(result = allocVector(VECSXP,2));
  SET_VECTOR_ELT(result,0,result_t_val = allocVector(REALSXP,1));
  SET_VECTOR_ELT(result,1,result_p_val = allocVector(REALSXP,1));
  PROTECT(result_names = allocVector(STRSXP,2));
  SET_STRING_ELT(result_names,0,mkChar("t.value"));
  SET_STRING_ELT(result_names,1,mkChar("p.value"));
  setAttrib(result,R_NamesSymbol,result_names);
  REAL(VECTOR_ELT(result,0))[0] = t_value;
  REAL(VECTOR_ELT(result,1))[0] = p_value;

  UNPROTECT(4); /* S C result result_names */

  Free(cond);

  return result;
}
static SEXP
qp_fast_ci_test2(SEXP SR, SEXP n_varR, SEXP NR, SEXP iR, SEXP jR, SEXP C) {
  int    N = INTEGER(NR)[0];
  int    n_var = INTEGER(n_varR)[0];
  int    q;
  int*   cond;
  int    i,j,k;
  double p_value;
  double t_value;
  SEXP   result;
  SEXP   result_names;
  SEXP   result_t_val;
  SEXP   result_p_val;

  PROTECT_INDEX Spi,Cpi;

  PROTECT_WITH_INDEX(SR,&Spi);
  PROTECT_WITH_INDEX(C,&Cpi);

  REPROTECT(SR = coerceVector(SR,REALSXP),Spi);
  REPROTECT(C = coerceVector(C,INTSXP),Cpi);

  i = INTEGER(iR)[0] - 1;
  j = INTEGER(jR)[0] - 1;
  q = length(C);

  cond = Calloc(q, int);
  for (k=0;k<q;k++)
    cond[k] = INTEGER(C)[k]-1;

  t_value = qp_ci_test2(REAL(SR),n_var,N,i,j,cond,q,NULL);
  p_value = 2.0 * (1.0 - pt(fabs(t_value),N-q-2,1,0));

  PROTECT(result = allocVector(VECSXP,2));
  SET_VECTOR_ELT(result,0,result_t_val = allocVector(REALSXP,1));
  SET_VECTOR_ELT(result,1,result_p_val = allocVector(REALSXP,1));
  PROTECT(result_names = allocVector(STRSXP,2));
  SET_STRING_ELT(result_names,0,mkChar("t.value"));
  SET_STRING_ELT(result_names,1,mkChar("p.value"));
  setAttrib(result,R_NamesSymbol,result_names);
  REAL(VECTOR_ELT(result,0))[0] = t_value;
  REAL(VECTOR_ELT(result,1))[0] = p_value;

  UNPROTECT(4); /* S C result result_names */

  Free(cond);

  return result;
}



/*
  FUNCTION: qp_ci_test
  PURPOSE: perform a test for conditional independence between variables
           indexed by i and j given the conditioning set Q
  RETURNS: a list with two members, the t-statistic value and the p-value
           on rejecting the null hypothesis of independence
*/

static double
qp_ci_test(double* S, int n_var, int N, int i, int j, int* Q, int q) {
  int*    subvars;
  int     subn = q + 2;
  int     k,l;
  double* Mmar;
  double  S11;
  double* S12;
  double* S21;
  double* S22;
  double* S22inv;
  double* S22inv1col;
  double* tmpmat;
  double  tmpval;
  double  betahat;
  double  sigma;
  double  se;
  double  t_value;

  subvars     = Calloc(subn,int);
  Mmar        = Calloc(subn*subn,double);
  S12         = Calloc(subn,double);
  S21         = Calloc(subn,double);
  S22         = Calloc((subn-1)*(subn-1),double);
  S22inv      = Calloc((subn-1)*(subn-1),double);
  S22inv1col  = Calloc(subn-1,double);

  subvars[0] = i; /* order here is important, first variable i */
  subvars[1] = j; /* then variable j then the conditioning set */
  for (k=2;k<subn;k++)
    subvars[k] = Q[k-2];

  /* Mmar <- S[c(i, j, sp), c(i, j, sp)] 
     S11     <- Mmar[1,1]
     S12     <- Mmar[1,-1]
     S21     <- Mmar[-1,1]
     S22     <- Mmar[-1,-1] */
/*
  for (k=0;k<subn;k++)
    for (l=0;l<subn;l++) {
      Mmar[k+l*subn] = S[subvars[k]+subvars[l]*n_var];
      if (k == 0 && l > 0)
        S12[l-1] = Mmar[k+l*subn];
      if (k > 0 && l == 0)
        S21[k-1] = Mmar[k+l*subn];
      if (k > 0 && l > 0)
        S22[k-1+(l-1)*(subn-1)] = Mmar[k+l*subn];
    }
  S11 = Mmar[0];
*/
  for (k=0;k<subn;k++)
    for (l=0;l<subn;l++) {
      Mmar[k+l*subn] = S[UTE2I(subvars[k],subvars[l])]; /* S is a vector storing the upper */
      if (k == 0 && l > 0)                              /* triangle of the sample covariance */
        S12[l-1] = Mmar[k+l*subn];                      /* matrix in column-major order */
      if (k > 0 && l == 0)
        S21[k-1] = Mmar[k+l*subn];
      if (k > 0 && l > 0)
        S22[k-1+(l-1)*(subn-1)] = Mmar[k+l*subn];
    }
  S11 = Mmar[0];

  /* S22inv  <- solve(S22) */
  matinv(S22inv,S22,subn-1);

  /* betahat <- S12 %*% S22inv[,1] */
  Memcpy(S22inv1col,S22inv,(size_t) (subn-1));
  matprod(S12,1,subn-1,S22inv1col,subn-1,1,&betahat);

  /* sigma   <- sqrt((S11 - S12 %*% S22inv %*% S21) * (N - 1) / (N - q - 2)) */
  tmpmat = Calloc(subn-1,double);
  matprod(S22inv,subn-1,subn-1,S21,subn-1,1,tmpmat);
  matprod(S12,1,subn-1,tmpmat,subn-1,1,&tmpval);
  Free(tmpmat);
  sigma = sqrt( (S11 - tmpval) * (N - 1) / (N - subn) );
  /* se      <- sigma * sqrt(S22inv[1,1] / (N - 1)) */
  se = sigma * sqrt(S22inv[0] / (N - 1));
  /* t.value <- betahat / se */
  t_value = betahat / se;

  Free(S22inv1col);
  Free(S22inv);
  Free(S22);
  Free(S21);
  Free(S12);
  Free(Mmar);
  Free(subvars);

  return t_value;
}



/*
  FUNCTION: qp_ci_test2
  PURPOSE: perform a test for conditional independence between variables
           indexed by i and j given the conditioning set Q. this version
           is more efficient than the original one.
  RETURNS: a list with two members, the t-statistic value and the p-value
           on rejecting the null hypothesis of independence
*/

static double
qp_ci_test2(double* S, int n_var, int N, int i, int j, int* Q, int q, double* Qinv) {
  int*    subvars;
  int     subn = q + 2;
  int     k,l;
  double* Qmat;
  double* Sij;
  double* Sijbyq;
  double* Sqbyij;
  double* tmpmat1;
  double* tmpmat2;
  double* par_cov;
  double* par_cor;
  double  betahat;
  double  se;
  double  t_value;
  int     flagNoQinv=0;

  subvars     = Calloc(subn,int);
  Sij         = Calloc(4,double);
  Sijbyq      = Calloc(2*q, double);
  Sqbyij      = Calloc(q*2, double);
  par_cov     = Calloc(4,double);
  par_cor     = Calloc(4,double);

  subvars[0] = i; /* order here is important, first variable i */
  subvars[1] = j; /* then variable j then the conditioning set */
  for (k=2;k<subn;k++)
    subvars[k] = Q[k-2];
/*
  for (k=0;k<subn;k++)
    for (l=0;l<subn;l++) {
      if (k < 2 && l < 2)
        Sij[k+l*2] = S[subvars[k]+subvars[l]*n_var];
      if (k < 2 && l > 1) {
        Sijbyq[k+(l-2)*2] = S[subvars[k]+subvars[l]*n_var];
        Sqbyij[l-2+k*q] = S[subvars[l]+subvars[k]*n_var];
      }
    }
*/
  for (k=0;k<subn;k++)
    for (l=0;l<subn;l++) {
      double x = S[UTE2I(subvars[k], subvars[l])];

      if (k < 2 && l < 2)
        Sij[k+l*2] = x;
      if (k < 2 && l > 1) {
        Sijbyq[k+(l-2)*2] = x;
        Sqbyij[l-2+k*q] = x;
      }
    }

  if (Qinv == NULL) {
    Qmat = Calloc(q*q, double);
    Qinv = Calloc(q*q, double);
    /*
    for (i=0; i < q; i++) {
      for (j=0; j < i; j++)
        Qmat[i + j*q] = Qmat[j + i*q] = S[Q[i] + Q[j] * n_var];
      Qmat[i + i*q] = S[Q[i] + Q[i] * n_var];
    }
    */
    for (i=0; i < q; i++) {
      for (j=0; j < i; j++)
        Qmat[i + j*q] = Qmat[j + i*q] = S[UTE2I(Q[i], Q[j])];
      Qmat[i + i*q] = S[UTE2I(Q[i], Q[i])];
    }
    if (q > 1)
      matinv(Qinv,Qmat,q);
    else
      Qinv[0] = 1.0 / Qmat[0];
    Free(Qmat);
    flagNoQinv=1;
  }

  tmpmat1 = Calloc(q*2,double);
  tmpmat2 = Calloc(4,double);
  matprod(Qinv,q,q,Sqbyij,q,2,tmpmat1);
  matprod(Sijbyq,2,q,tmpmat1,q,2,tmpmat2);
  Free(tmpmat1);
  matsumf(par_cov, 2, 2, Sij, tmpmat2, -1.0);
  Free(tmpmat2);
  Free(Sij);
  Free(Sijbyq);
  Free(Sqbyij);
  cov2cor(par_cor, par_cov, 2);
  Free(par_cov);
  betahat = sqrt(N - q -2) * par_cor[2];
  se = sqrt(1.0 - par_cor[2] * par_cor[2]);

  /* t.value <- betahat / se */
  t_value = betahat / se;

  Free(par_cor);
  Free(subvars);

  if (flagNoQinv)
    Free(Qinv);

  return t_value;
}



/*
  FUNCTION: qp_edge_nrr
  PURPOSE: estimate the non-rejection rate of the edge as the number of tests
           that accept the null hypothesis of independence given the
           q-order conditionals
  RETURNS: the estimate of the non-rejection rate for the particular given edge
*/

static double
qp_edge_nrr(double* S, int n_var, int N, int i, int j, int q, int nTests,
             double alpha) {
  double thr;
  int*   q_by_T_samples;
  int    k;
  int    nAcceptedTests = 0;

  thr = qt(1.0-(alpha/2.0), N-q-2, 1, 0);

  q_by_T_samples = Calloc(q * nTests, int);

  sampleQs(nTests, q, i, j, n_var, q_by_T_samples);

  for (k = 0; k < nTests; k++) {
    double t_value;

    t_value = qp_ci_test(S, n_var, N, i, j, (int*) (q_by_T_samples+k*q), q);

    if (fabs(t_value) < thr)
      nAcceptedTests++;
  }

  Free(q_by_T_samples);

  return (double) ( ((double) nAcceptedTests) / ((double) nTests) );
}



/*
  FUNCTION: qp_edge_nrr_identicalQs
  PURPOSE: estimate the non-rejection rate of the edge as the number of tests
           that accept the null hypothesis of independence given the
           q-order conditionals
  RETURNS: the estimate of the non-rejection rate for the particular given edge
*/

static double
qp_edge_nrr_identicalQs(double* S, int n_var, int* Qs, double* Qinvs, int N, int i,
                        int j, int q, int nTests, double alpha) {
  double thr;
  int    k;
  int    nAcceptedTests = 0;
  int    nActualTests = 0;
  double avgpr = 0;

  thr = qt(1.0-(alpha/2.0), N-q-2, 1, 0);

  for (k = 0; k < nTests; k++) {
    double t_value;
    int    l=0;

    while (Qs[k*q+l] != i && Qs[k*q+l] !=j && l < q)
      l++;

    if (l >= q) {
      t_value = qp_ci_test2(S, n_var, N, i, j, (int*) (Qs+k*q), q, (double*) (Qinvs+k*q*q));

      if (fabs(t_value) < thr)
        nAcceptedTests++;

      nActualTests++;
    } else
      avgpr++;
  }

  return (double) ( ((double) nAcceptedTests) / ((double) nActualTests) );
}



/*
  FUNCTION: qp_fast_edge_nrr
  PURPOSE: wrapper to the C function that estimates the non-rejection
           rate of the edge as the number of tests that accept the null
           hypothesis of independence given the q-order conditionals
  RETURNS: the estimate of the non-rejection rate for the particular given edge
*/

static SEXP
qp_fast_edge_nrr(SEXP S, SEXP n_varR, SEXP NR, SEXP iR, SEXP jR, SEXP qR,
                 SEXP nTestsR, SEXP alphaR) {
  int    i,j;
  int    n_var = INTEGER(n_varR)[0];
  int    N;
  int    q;
  int    nTests;
  double alpha;
  SEXP   nrr;

  PROTECT_INDEX Spi;

  PROTECT_WITH_INDEX(S,&Spi);

  REPROTECT(S = coerceVector(S,REALSXP),Spi);

  i = INTEGER(iR)[0] - 1;
  j = INTEGER(jR)[0] - 1;

  N = INTEGER(NR)[0];
  q = INTEGER(qR)[0];

  nTests = INTEGER(nTestsR)[0];

  alpha = REAL(alphaR)[0];

  if (i < 0 || i > n_var-1 || j < 0 || j > n_var-1)
    error("vertices of the selected edge (i,j) should lie in the range [1,n.var=%d]", n_var);

  if (q > n_var-2)
    error("q=%d > n.var-2=%d", q, n_var-2);

  if (q < 0)
    error("q=%d < 0",q);

  if (q > N-3)
    error("q=%d > N-3=%d",q,N-3);

  PROTECT(nrr = allocVector(REALSXP,1));

  REAL(nrr)[0] = qp_edge_nrr(REAL(S),n_var,N,i,j,q,nTests,alpha);

  UNPROTECT(2); /* S nrr */

  return nrr;
}



/*
  FUNCTION: sampleQs
  PURPOSE: sample without replacement q elements from p, T times. this
           is a re-make of the SampleNoReplace function of random.c specifically
           tailored to sample in one shot everything we need
  RETURN: a vector with of the T samples of q elements one after each other
*/

int
int_cmp(const void* a, const void* b);

static void
sampleQs(int T, int q, int v_i, int v_j, int p, int* y) {
  int  i;
  int  total_j = 0;
  int* x;
  int* z;

  x = Calloc(p,int);
  z = Calloc(p,int);

  for (i = 0; i < p; i++) {             /* x is a working-only vector */
    x[i] = i;
    z[i] = i;                           /* maps each vertex into a proper place */
  }

  if (v_i >=0 && v_j >=0) {               /* when sampling Qs outside v_i and v_j     */
    if (v_i < v_j) {                      /* we should take care that the mapping z   */
      z[v_i] = v_j != p-2 ? p-2 : p-1;    /* re-maps the v_i and v_j vertices to the  */
      z[v_j] = z[v_i] != p-1 ? p-1 : p-2; /* p-1 and p-2 properly when any of the two */
    } else {                              /* is smaller than p-2                      */
      z[v_j] = v_i != p-2 ? p-2 : p-1;
      z[v_i] = z[v_j] != p-1 ? p-1 : p-2;
    }
  }

  for (i = 0; i < T; i++) {
    int j;
    int m = p-2;                              /* we sample from p-2 elements */

    for (j = 0; j < q ; j++) {
      int r;

      r = (int) (((double) m) * unif_rand()); /* sample using R-builtin RNG */
      y[total_j + j] = x[r];
      x[r] = x[--m];                          /* sample without replacement */

    }

    for (j = total_j; j < total_j+q; j++) { /* replace again the sampled elements */
      x[y[j]] = y[j];                       /* for the next round of T       */
      y[j] = z[y[j]];                       /* use the mapping z to avoid choosing v_i or v_j */
    }

    total_j += q;
  }

  Free(x);
  Free(z);
}



  typedef struct {
    int x;
    int ix;
  } IntWithIdx;

  int
  int_cmp_desc_idx(const void *a, const void *b) {
    const IntWithIdx* ia = (const IntWithIdx *) a;
    const IntWithIdx* ib = (const IntWithIdx *) b;

    return ib->x - ia->x;
  }

/*
  FUNCTION: qp_clique_number_lb
  PURPOSE: calculate a lower bound on the clique number from the input graph
  RETURNS: a lower bound of the clique number from the input graph
*/

static SEXP
qp_clique_number_lb(SEXP A, SEXP return_vertices, SEXP approx_iter, SEXP verbose) {
  int         n = INTEGER(getAttrib(A,R_DimSymbol))[0];
  IntWithIdx* deg;
  int*        pdeg;
  int*        ivec;
  int*        sset;
  int*        ssetelem;
  int         cliqueNumber=0;
  int*        cliqueVertices;
  int*        clq;
  int         i;
  int         ppct=-1;
  SEXP        return_value;

  PROTECT_INDEX Api;

  PROTECT_WITH_INDEX(A,&Api);

  REPROTECT(A = coerceVector(A,INTSXP),Api);

  deg = Calloc(n, IntWithIdx);
  pdeg = Calloc(n, int);
  ivec = Calloc(n, int);
  sset = Calloc(n, int);
  ssetelem = Calloc(n, int);
  cliqueVertices = Calloc(n, int);
  clq = Calloc(n, int);

  for (i=0; i < n; i++) {
    int j;

    deg[i].x = 0;
    for (j=0; j < n; j++)
      if (INTEGER(A)[j*n+i]) {
        deg[i].x++;
      }
    ivec[i] = i;
    deg[i].ix = i;
  } 
  qsort(deg, n, sizeof(IntWithIdx), int_cmp_desc_idx);

  if (INTEGER(verbose)[0])
    Rprintf("calculating lower bound on the maximum clique size\n");

  for (i=0; i < INTEGER(approx_iter)[0]; i++) {
    int pct;
    int clqsze;
    int j;

    for (j=0; j < n; j++)
      pdeg[j] = deg[j].ix;

    if (i % n + 1 > 1) {
      int m;

      m = n;
      /* sample (i % n + 1) elements from n without replacement */
      for (j=0; j < i % n + 1; j++) {
        int r;

        r = (int) (((double) m) * unif_rand()); /* sample using R-builtin RNG */
        sset[j] = ivec[r];
        ivec[r] = ivec[--m];                    /* sample without replacement */
      }

      /* store the indices to the degree ranking using the sampled elements */
      for (j = 0; j < i % n + 1; j++) {  /* replace again the sampled elements */
        ssetelem[j] = pdeg[sset[j]];
        ivec[sset[j]] = sset[j];         /* so that ivec remains intact */
      }

      /* shuffle the stored indices using the Fisher-Yates algorithm */
      j = i % n;
      while (j >= 0) {
        int k;

        k = (int) (((double) j) * unif_rand());
        if (j != k) {
          int tmp;

          tmp = ssetelem[j];
          ssetelem[j] = ssetelem[k];
          ssetelem[k] = tmp;
        }
        j = j - 1;
      }

      /* shuffle the corresponding elements in the degree ranking */
      for (j=0; j < i % n + 1;j++)
        pdeg[sset[j]] = ssetelem[j];
    }

    /* go through the degree ranking building a clique */
    clq[0] = pdeg[0];
    clqsze = 1;
    for (j=1;j < n;j++) {
      int k;
      int isClique;

      clq[clqsze] = pdeg[j];

      isClique = 1;
      k = 0;
      while (k < (int) (((double) ((clqsze+1)*(clqsze)))/2.0) && isClique) {
        int u, v;

        i2e(k, &u, &v);
        if (!INTEGER(A)[clq[u] * n + clq[v]])
          isClique = 0;

        k++;
      }

      if (isClique)
        clqsze++; 
    }

    if (clqsze > cliqueNumber) {
      cliqueNumber = clqsze;
      Memcpy(cliqueVertices, clq, (size_t) clqsze);
    }

    pct = (int) ((i * 100) / INTEGER(approx_iter)[0]);
    if (pct != ppct) {
      if (INTEGER(verbose)[0]) {
        if (pct % 10 == 0)
          Rprintf("%d",pct);
        else
          Rprintf(".",pct);
        R_FlushConsole();
      }

      R_CheckUserInterrupt();
#ifdef Win32
      R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
      R_ProcessEvents();
#endif
      ppct = pct;
    }
  }

  UNPROTECT(1); /* A */

  if (INTEGER(verbose)[0])
    Rprintf("\n");

  if (INTEGER(return_vertices)[0]) {
    SEXP names;

    PROTECT(return_value = allocVector(VECSXP,2));

    SET_VECTOR_ELT(return_value,0,allocVector(INTSXP, 1));
    SET_VECTOR_ELT(return_value,1,allocVector(INTSXP, cliqueNumber));

    INTEGER(VECTOR_ELT(return_value,0))[0] = cliqueNumber;

    for (i=0;i < cliqueNumber;i++)
      INTEGER(VECTOR_ELT(return_value,1))[i] = cliqueVertices[i] + 1; /* in R vertices are 1-based */

    PROTECT(names = allocVector(VECSXP,2));
    SET_VECTOR_ELT(names, 0, mkChar("size"));
    SET_VECTOR_ELT(names, 1, mkChar("vertices"));
    setAttrib(return_value, R_NamesSymbol, names);
    UNPROTECT(1); /* names */

  } else {
    PROTECT(return_value = allocVector(INTSXP,1));

    INTEGER(return_value)[0] = cliqueNumber;
  }
  Free(deg);
  Free(pdeg);
  Free(ivec);
  Free(sset);
  Free(ssetelem);
  Free(cliqueVertices);
  Free(clq);

  UNPROTECT(1);  /* return_value */

  return return_value;
}



/*
  FUNCTION: qp_clique_number_os
  PURPOSE: wrapper of the function clique_unweighted_max_weight from the
           cliquer library that implements the algorithms by P. Ostergard
           to search for the maximal clique of maximum size
  RETURNS: the size of the largest clique
*/

static SEXP
qp_clique_number_os(SEXP I, SEXP return_vertices, SEXP verbose) {
  graph_t*       g;
  int            n = INTEGER(getAttrib(I,R_DimSymbol))[0];
  clique_options clq_opts;
  int            i;
  SEXP           return_value;

  if (INTEGER(verbose)[0])
    Rprintf("Niskanen and Ostergard algorithm for maximum clique running\n");

  PROTECT_INDEX Ipi;

  PROTECT_WITH_INDEX(I,&Ipi);

  REPROTECT(I = coerceVector(I,INTSXP),Ipi);

  g = graph_new(n);

  /* copy the incidence matrix 'I' into a graph_t structure
     from the cliquer library */

  for (i=0;i<n;i++) {
    int j;

    for (j=0;j<i;j++)
      if (INTEGER(I)[j*n+i])
        GRAPH_ADD_EDGE(g,i,j);
  }

  UNPROTECT(1); /* I */

  clq_opts.reorder_function   = reorder_by_default;
  clq_opts.reorder_map        = NULL;
  clq_opts.time_function      = INTEGER(verbose)[0] ? clique_print_time : NULL;
  clq_opts.output             = NULL;
  clq_opts.user_function      = NULL;
  clq_opts.user_data          = NULL;
  clq_opts.clique_list        = NULL;
  clq_opts.clique_list_length = 0;
  
  if (INTEGER(return_vertices)[0]) {
    int   i,j;
    set_t maxclq;
    SEXP  names;

    maxclq = clique_find_single(g,0,0,TRUE,&clq_opts);

    PROTECT(return_value = allocVector(VECSXP,2));

    SET_VECTOR_ELT(return_value,0,allocVector(INTSXP,1));
    SET_VECTOR_ELT(return_value,1,allocVector(INTSXP,set_size(maxclq)));

    INTEGER(VECTOR_ELT(return_value,0))[0] = set_size(maxclq);

    i=-1; j=0;
    while ((i=set_return_next(maxclq,i)) >= 0)
      INTEGER(VECTOR_ELT(return_value,1))[j++] = i + 1; /* in R vertices are 1-based */

    set_free(maxclq);

    PROTECT(names = allocVector(VECSXP,2));
    SET_VECTOR_ELT(names,0,mkChar("size"));
    SET_VECTOR_ELT(names,1,mkChar("vertices"));
    setAttrib(return_value,R_NamesSymbol,names);
    UNPROTECT(1); /* names */

  } else {
    PROTECT(return_value = allocVector(INTSXP,1));

    INTEGER(return_value)[0] = clique_unweighted_max_weight(g,&clq_opts);
  }

  UNPROTECT(1); /* return_value */

  graph_free(g);

  return return_value;
}



/*
  FUNCTION: cliquer_cb_add_clique_to_list
  PURPOSE: callback function for cliquer library function clique_unweighted_find_all
           it adds a clique to a linked list
  RETURNS: TRUE always to let continue the search till the end
*/

boolean
cliquer_cb_add_clique_to_list(set_t clique, graph_t* g, clique_options* opts) {
  clique_set_t* cset;
  clique_t*     c;

  cset = (clique_set_t *) opts->user_data;
  c = Calloc(1,clique_t);
  c->next = NULL;
                                                                                                
  if (cset->n == 0)
    cset->first = cset->last = c;
  else {
    clique_t* p;

    p = cset->last;
    p->next = cset->last = c;
  }

  c->u.vts = set_duplicate(clique);
  c->n     = set_size(clique);
  cset->n++;

  return TRUE;
}



/*
  FUNCTION: add_clique_vts
  PURPOSE: adds a clique to a linked list assuming that vertices
           are given and stored as cliquer sets
  RETURNS: nothing
*/

void
add_clique_vts(clique_set_t* cset, set_t clique) {
  clique_t* c;

  c = Calloc(1,clique_t);
  c->next = NULL;

  if (cset->n == 0)
    cset->first = cset->last = c;
  else {
    clique_t* p;

    p = cset->last;
    p->next = cset->last = c;
  }

  c->u.vts = set_duplicate(clique);
  c->n     = set_size(clique);
  cset->n++;
}



/*
  FUNCTION: add_clique_vta
  PURPOSE: adds a clique to a linked list assuming that vertices
           are given and stored as an array of integers
  RETURNS: nothing
*/

void
add_clique_vta(clique_set_t* cset, int* clique, int n) {
  clique_t* c;

  c = Calloc(1,clique_t);
  c->next = NULL;

  if (cset->n == 0)
    cset->first = cset->last = c;
  else {
    clique_t* p;

    p = cset->last;
    p->next = cset->last = c;
  }

  c->n     = n;
  c->u.vta = Calloc(n,int);
  Memcpy(c->u.vta,clique,(size_t) n);

  cset->n++;
}



/*
  FUNCTION: destroy_cliques_vts
  PURPOSE: destroys an object of the type clique_set_t which
           consists of going through a dynamically linked list
           and freeing the memory allocated for each of its elements
           this version assumes vertices were stored as cliquer sets
  RETURN: none
*/

void
destroy_cliques_vts(clique_set_t* cset) {
  clique_t* p;
                                                                                                
  if (cset->n == 0)
    return;
                                                                                                
  p = cset->first;
  while (p != NULL) {
    clique_t* tmp;
                                                                                                
    tmp = p->next;
    set_free(p->u.vts);
    Free(p);
    p = tmp;
  }

  cset->first = cset->last = NULL;
  cset->n = 0;
}



/*
  FUNCTION: destroy_cliques_vta
  PURPOSE: destroys an object of the type clique_set_t which
           consists of going through a dynamically linked list
           and freeing the memory allocated for each of its elements
           this version assumes vertices were stored as arrays of integers
  RETURN: none
*/

void
destroy_cliques_vta(clique_set_t* cset) {
  clique_t* p;
                                                                                                
  if (cset->n == 0)
    return;
                                                                                                
  p = cset->first;
  while (p != NULL) {
    clique_t* tmp;
                                                                                                
    tmp = p->next;
    Free(p->u.vta);
    Free(p);
    p = tmp;
  }

  cset->first = cset->last = NULL;
  cset->n = 0;
}



/*
  FUNCTION: init_cliques_list
  PURPOSE: initialize an object of the type clique_set_t
  RETURN: none
*/

void
init_cliques_list(clique_set_t* cset) {
  cset->first = cset->last = NULL;
  cset->n = 0;
}



int
int_cmp(const void *a, const void *b) {
  const int *ia = (const int *) a;
  const int *ib = (const int *) b;

  return *ia - *ib;
}



/*
  FUNCTION: qp_fast_cliquer_get_cliques
  PURPOSE: finds the (maximal) cliques of an undirected graph, it uses the
           library 'cliquer' from:

           Ostergard, PRJ. A fast algorithm for the maximum clique problem
           Discrete Appl. Math., 120:195-205, 2002
           http://users.tkk.fi/~pat/cliquer.html
  RETURNS: a list of (maximal) cliques
*/

static SEXP
qp_fast_cliquer_get_cliques(SEXP I, SEXP clqspervtx, SEXP verbose) {
  graph_t*       g;
  int            n = INTEGER(getAttrib(I,R_DimSymbol))[0];
  int            i;
  int            nclqs;
  clique_set_t   clqlst;
  SEXP           clqlstR;
  clique_options clq_opts;

  if (!isMatrix(I)) {
    error("qpGetCliques() expects an incidence matrix");
  }

  PROTECT_INDEX Ipi;

  PROTECT_WITH_INDEX(I,&Ipi);

  REPROTECT(I = coerceVector(I,INTSXP),Ipi);

  g = graph_new(n);

  /* copy the incidence matrix 'I' into a graph_t structure
     from the cliquer library */

  for (i=0;i<n;i++) {
    int j;

    for (j=0;j<i;j++)
      if (INTEGER(I)[j*n+i])
        GRAPH_ADD_EDGE(g,i,j);
  }

  UNPROTECT(1); /* I */

  init_cliques_list(&clqlst);
  clq_opts.reorder_function   = reorder_by_default;
  clq_opts.reorder_map        = NULL;
  clq_opts.time_function      = INTEGER(verbose)[0] ? clique_print_time : NULL;
  clq_opts.output             = NULL;
  clq_opts.user_function      = cliquer_cb_add_clique_to_list;
  clq_opts.user_data          = (void *) &clqlst;
  clq_opts.clique_list        = NULL;
  clq_opts.clique_list_length = 0;
  
  nclqs = clique_unweighted_find_all(g,1,n,TRUE,&clq_opts);

  graph_free(g);

  if (nclqs != clqlst.n)
    error("qpGetCliques: nclqs is different from clqlst.n!!!");

  /* allocate n components more where to put the cliques to which each vertex belongs to
     afterwards the intersection operation should quickly give the set of cliques including
     a given edge */

  if (INTEGER(clqspervtx)[0])
    PROTECT(clqlstR = allocVector(VECSXP,clqlst.n + n));
  else
    PROTECT(clqlstR = allocVector(VECSXP,clqlst.n));

  if (clqlst.n > 0) {
    clique_t* p;
    int       iclq;
    int**     idxclqs = NULL;
    int*      nidxclqs = NULL;
    int*      sidxclqs = NULL;

    if (INTEGER(clqspervtx)[0]) {
      int i;

      idxclqs = (int **) Calloc(n,int *);
      nidxclqs = (int *) Calloc(n,int);
      sidxclqs = (int *) Calloc(n,int);

      for (i=0;i<n;i++)
        nidxclqs[i]=0;
    }

    iclq = INTEGER(clqspervtx)[0] ? n : 0;
    p = clqlst.first;
    while (p != NULL) {
      clique_t* tmpp;
      int  v,i;
      SEXP clq;

      SET_VECTOR_ELT(clqlstR,iclq,clq = allocVector(INTSXP,p->n));

      v=-1; i=0;
      while ((v=set_return_next(p->u.vts,v)) >= 0) {
        INTEGER(VECTOR_ELT(clqlstR,iclq))[i] = v + 1; /* in R vertices are 1-based */

        if (INTEGER(clqspervtx)[0]) {
          if (nidxclqs[v] == 0) {
            sidxclqs[v] = 1;
            idxclqs[v] = (int *) Calloc(sidxclqs[v],int);
            idxclqs[v][nidxclqs[v]] = iclq + 1;
            nidxclqs[v]++;
          } else {
            if (sidxclqs[v] > nidxclqs[v]) {
              idxclqs[v][nidxclqs[v]] = iclq + 1;
              nidxclqs[v]++;
            } else {
              sidxclqs[v] = sidxclqs[v] * 2;
              idxclqs[v] = (int *) Realloc(idxclqs[v],sidxclqs[v],int);
              idxclqs[v][nidxclqs[v]] = iclq + 1;
              nidxclqs[v]++;
            }
          }
        }

        i++;
      }

      iclq++;

      /* free the elements of the linked list at the same time that the new R list
         structure is created to store the cliques in order to use as little memory as possible */

      tmpp = p->next;
      set_free(p->u.vts);
      Free(p);
      p = tmpp;
    }

    if (INTEGER(clqspervtx)[0]) {
      int i;

      for (i=0;i<n;i++) {
        qsort(idxclqs[i],nidxclqs[i],sizeof(int),int_cmp);
        SET_VECTOR_ELT(clqlstR,i,allocVector(INTSXP,nidxclqs[i]));
        Memcpy(INTEGER(VECTOR_ELT(clqlstR,i)),idxclqs[i],(size_t) nidxclqs[i]);
        Free(idxclqs[i]);
      }

      Free(sidxclqs);
      Free(nidxclqs);
      Free(idxclqs);
    }

  }

  UNPROTECT(1); /* clqlstR */

  return clqlstR;
}



/*
  FUNCTION: qp_fast_update_cliques_removing
  PURPOSE: modifies an input list of maximal cliques by removing one edge of the graph
  RETURNS: a list of maximal cliques
*/

static SEXP
qp_fast_update_cliques_removing(SEXP I, SEXP clqlstR, SEXP vR, SEXP wR, SEXP verbose) {
  SEXP new_clqlstR;
  int n = INTEGER(getAttrib(I,R_DimSymbol))[0];
  int v = INTEGER(coerceVector(vR,INTSXP))[0] - 1; /* internally we work with 0-based vertices */
  int w = INTEGER(coerceVector(wR,INTSXP))[0] - 1; /* internally we work with 0-based vertices */
  int nclqlstR = length(clqlstR);
  int nnew_clqlstR;
  int* clqs_v_w;
  int  nclqs_v,nclqs_w;
  int  nclqs_v_w;
  clique_set_t clqlst;
  int* new_clq_v;
  int* new_clq_w;
  set_t allvtc;
  clique_t* p;
  int removed_so_far;
  int i,inewclq;
  int** idxclqs;
  int*  nidxclqs;
  int*  sidxclqs;
  int ppct;

  if (!isMatrix(I)) {
    error("qpUpdateCliquesRemoving() expects an incidence matrix");
  }

  PROTECT_INDEX Ipi;

  PROTECT_WITH_INDEX(I,&Ipi);

  REPROTECT(I = coerceVector(I,INTSXP),Ipi);

  nclqs_v = length(VECTOR_ELT(clqlstR,v));
  nclqs_w = length(VECTOR_ELT(clqlstR,w));
  clqs_v_w = (int *) Calloc(nclqs_v+nclqs_w,int);

  if (INTEGER(verbose)[0]) {
    Rprintf("qpUpdateCliquesRemoving: initially there are %d maximal clique(s)\n",nclqlstR-n);
    Rprintf("qpUpdateCliquesRemoving: searching cliques to which the edge v-w belongs to (v belongs to %d, w belongs to %d)\n",nclqs_v,nclqs_w);
  }

  /* the cliques where the edge v-w is are the common ones to those where v and w belong to */
  nclqs_v_w = 0;
  for (i=0;i<nclqs_v;i++)
    if (bsearch(INTEGER(VECTOR_ELT(clqlstR,v))+i,INTEGER(VECTOR_ELT(clqlstR,w)),nclqs_w,sizeof(int),int_cmp))
        clqs_v_w[nclqs_v_w++] = INTEGER(VECTOR_ELT(clqlstR,v))[i] - 1; /* internally the clique index
                                                                          in the R list is 0-based */

  /* sort the cliques to be removed in ascending clique index order */
  qsort(clqs_v_w,nclqs_v_w,sizeof(int),int_cmp);

  INTEGER(I)[v*n+w] = INTEGER(I)[w*n+v] = 0; /* make sure the edge is removed */

  allvtc = set_new(n);
  for (i=0;i<n;i++)
    SET_ADD_ELEMENT(allvtc,i);

  new_clq_v = Calloc(n,int); /* store here each new clique that formerly contained vertex v */
  new_clq_w = Calloc(n,int); /* store here each new clique that formerly contained vertex w */

  if (INTEGER(verbose)[0])
    Rprintf("qpUpdateCliquesRemoving: going through the %d v-w-containing clique(s) and decide which one(s) to add\n",nclqs_v_w);

  /* go through all the cliques where the edge v-w forms part */

  init_cliques_list(&clqlst);
  for (i=0;i<nclqs_v_w;i++) {
    int clq_v_w_size = length(VECTOR_ELT(clqlstR,clqs_v_w[i]));
    set_t restvtc_v = set_duplicate(allvtc);
    set_t restvtc_w = set_duplicate(allvtc);
    int iclq_v = 0;
    int iclq_w = 0;
    int j;

    for (j=0;j<clq_v_w_size;j++) {
      int vtx = INTEGER(VECTOR_ELT(clqlstR,clqs_v_w[i]))[j] - 1; /* internally we work with 0-based vertices */

      if (vtx != v) {
        new_clq_v[iclq_v++] = vtx + 1;  /* in the R object vertices are 1-based */
        SET_DEL_ELEMENT(restvtc_v,vtx);
      }

      if (vtx != w) {
        new_clq_w[iclq_w++] = vtx + 1;  /* in the R object vertices are 1-based */
        SET_DEL_ELEMENT(restvtc_w,vtx);
      }
    }

    if (is_maximal_clique(INTEGER(I),n,new_clq_v,clq_v_w_size-1,restvtc_v))
      add_clique_vta(&clqlst,new_clq_v,clq_v_w_size-1);

    if (is_maximal_clique(INTEGER(I),n,new_clq_w,clq_v_w_size-1,restvtc_w))
      add_clique_vta(&clqlst,new_clq_w,clq_v_w_size-1);
  }

  Free(new_clq_v);
  Free(new_clq_w);

  UNPROTECT(1); /* I */

  /* remove the cliques that contain the edge v-w and add the new ones */

  nnew_clqlstR = nclqlstR + clqlst.n - nclqs_v_w;
  PROTECT(new_clqlstR=allocVector(VECSXP,nnew_clqlstR));

  if (INTEGER(verbose)[0])
    Rprintf("qpUpdateCliquesRemoving: going to remove %d clique(s) and add %d clique(s) ending with %d clique(s)\n",nclqs_v_w,clqlst.n,nnew_clqlstR-n);

  inewclq = n;
  removed_so_far = 0;
  for (i=n;i<nclqlstR;i++)
    if (removed_so_far >= nclqs_v_w || clqs_v_w[removed_so_far] != i)
      SET_VECTOR_ELT(new_clqlstR,inewclq++, Rf_duplicate(VECTOR_ELT(clqlstR,i)));
    else
      removed_so_far++;

  p = clqlst.first;
  while (p != NULL) {
    SEXP newclq;
    clique_t* tmpp;

    PROTECT(newclq = allocVector(INTSXP,p->n));
    Memcpy(INTEGER(newclq),p->u.vta,(size_t) p->n);
    SET_VECTOR_ELT(new_clqlstR,inewclq++,newclq);
    UNPROTECT(1); /* newclq */

    /* free the elements of the linked list at the same time that the new R list
       structure is created to store the cliques in order to use a little memory as possible */

    tmpp = p->next;
    Free(p->u.vta);
    Free(p);
    p = tmpp;
  }

  Free(clqs_v_w);

  if (INTEGER(verbose)[0])
    Rprintf("qpUpdateCliquesRemoving: rebuilding references to cliques\n");

  idxclqs = (int **) Calloc(n,int *);
  nidxclqs = (int *) Calloc(n,int);
  sidxclqs = (int *) Calloc(n,int);

  for (i=0;i<n;i++)
    nidxclqs[i]=0;

  ppct = -1;
  for (i=n;i<nnew_clqlstR;i++) {
    int j;

    for (j=0;j<length(VECTOR_ELT(new_clqlstR,i));j++) {
      v = INTEGER(VECTOR_ELT(new_clqlstR,i))[j] - 1;
      if (nidxclqs[v] == 0) {
        sidxclqs[v] = 1;
        idxclqs[v] = (int *) Calloc(sidxclqs[v],int);
        idxclqs[v][nidxclqs[v]] = i + 1;
        nidxclqs[v]++;
      } else {
        if (sidxclqs[v] > nidxclqs[v]) {
          idxclqs[v][nidxclqs[v]] = i + 1;
          nidxclqs[v]++;
        } else {
          sidxclqs[v] = sidxclqs[v] * 2;
          idxclqs[v] = (int *) Realloc(idxclqs[v],sidxclqs[v],int);
          idxclqs[v][nidxclqs[v]] = i + 1;
          nidxclqs[v]++;
        }
      }
    }

    if (INTEGER(verbose)[0]) {
      int pct = (int) ((i*100)/nnew_clqlstR);

      if (pct != ppct) {
        if (pct % 10 == 0)
          Rprintf("%d",pct);
        else
          Rprintf(".",pct);
        R_FlushConsole();
#ifdef Win32
        R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
        R_ProcessEvents();
#endif
        ppct = pct;
      }
    }
  }
  if (INTEGER(verbose)[0])
    Rprintf("\n");

  for (i=0;i<n;i++) {
    SEXP clqrefs;

    qsort(idxclqs[i],nidxclqs[i],sizeof(int),int_cmp);
    PROTECT(clqrefs = allocVector(INTSXP,nidxclqs[i]));
    Memcpy(INTEGER(clqrefs),idxclqs[i],(size_t) nidxclqs[i]);
    Free(idxclqs[i]);

    SET_VECTOR_ELT(new_clqlstR,i,clqrefs);
    UNPROTECT(1); /* clqrefs */
  }

  Free(sidxclqs);
  Free(nidxclqs);
  Free(idxclqs);

  UNPROTECT(1); /* new_clqlstR */

  return new_clqlstR;
}



/*
  FUNCTION: is_maximal_clique
  PURPOSE: returns whether the clique in 'clq' is maximal. note that the
           vertices in clq come 1-based and vertices in noclq come 0-based
  RETURNS: TRUE if the clique is maximal; FALSE otherwise
*/
/* vertices in clq come 1-based vertices in noclq come 0-based */

Rboolean
is_maximal_clique(int* I, int n, int* clq, int cs, set_t noclq) {
  int i;
  Rboolean maximal = TRUE;

  i=-1;
  while ((i=set_return_next(noclq,i))>=0 && maximal) {
    int j=0;
    int allconnected = TRUE;

    while (j < cs && allconnected) {
      allconnected = allconnected && I[(clq[j]-1)*n+i];
      j++;
    }

    maximal = !allconnected;
  }

  return maximal;
}



/*
  FUNCTION: qp_fast_pac_se
  PURPOSE: calculate the standard errors for the edges of an undirected graphical
           Gaussian Markov model according to the method by:

           Roverato and Whittaker. Standard errors for the parameters of
           graphical Gaussian models, STATISTICS AND COMPUTING, 6:297-302, 1996

  PARAMETERS: S - estimate of the sample covariance matrix
              I - incidence matrix of the graph and thus it is assumed that the
                  diagonal is set to either 0s or FALSE truth values since there
                  should be no loops
  RETURNS: a matrix with the standard errors of the edges
*/

static SEXP
qp_fast_pac_se(SEXP Shat, SEXP I) {
  int  n_var = INTEGER(getAttrib(I,R_DimSymbol))[0];
  int  n_edges;
  int* r_nz;
  int* c_nz;
  int  i,j;
  int  rnz,cnz;
  double* tmp1;
  double* tmp2;
  double* tmp3;
  double* tmp4;
  double* Iss1;
  double* Iss2;
  double* Iss;
  double* Issinv;
  SEXP    SER;
  double* SE;
  PROTECT_INDEX Spi,Ipi;

  if (!isMatrix(Shat) || !isMatrix(I)) {
    error("qpPACSE: Shat or I is not a matrix!");
  }

  n_edges = 0;
  for (i=0;i<n_var;i++)
    for (j=0;j<=i;j++)
      if (INTEGER(I)[i+j*n_var] != 0)
        n_edges++;

  PROTECT_WITH_INDEX(Shat,&Spi);
  PROTECT_WITH_INDEX(I,&Ipi);

  REPROTECT(Shat = coerceVector(Shat,REALSXP),Spi);
  REPROTECT(I = coerceVector(I,INTSXP),Ipi);

  r_nz = Calloc(n_edges+n_var,int);
  c_nz = Calloc(n_edges+n_var,int);

  /* selection of row and column indices according to whether
     the cells in I have value 0 or not. indices of the diagonal
     are also selected */

  rnz = cnz = 0;
  for (i=0;i<n_var;i++) {
    for (j=0;j<=i;j++) {
      if (INTEGER(I)[i+j*n_var] != 0 || i==j) {
        r_nz[rnz++] = i;
        c_nz[cnz++] = j;
      }
    }
  }

  UNPROTECT(1); /* I */

  tmp1 = Calloc(cnz*cnz,double);
  tmp2 = Calloc(rnz*rnz,double);
  tmp3 = Calloc(cnz*rnz,double);
  tmp4 = Calloc(rnz*cnz,double);
  matsubm(tmp1,REAL(Shat),n_var,c_nz,cnz,c_nz,cnz);
  matsubm(tmp2,REAL(Shat),n_var,r_nz,rnz,r_nz,rnz);
  matsubm(tmp3,REAL(Shat),n_var,c_nz,cnz,r_nz,rnz);
  matsubm(tmp4,REAL(Shat),n_var,r_nz,rnz,c_nz,cnz);

  UNPROTECT(1); /* Shat */

  Iss1 = Calloc(cnz*cnz,double);
  Iss2 = Calloc(rnz*rnz,double);
  Iss  = Calloc(rnz*cnz,double);
  matscalarprod(Iss1,cnz,cnz,tmp1,tmp2);
  matscalarprod(Iss2,rnz,rnz,tmp3,tmp4);
  Free(tmp1); Free(tmp2); Free(tmp3); Free(tmp4);
  matsumf(Iss,rnz,cnz,Iss1,Iss2,1.0);
  Free(Iss1); Free(Iss2);

  Issinv = Calloc(rnz*cnz,double);
  matinv(Issinv,Iss,rnz);
  Free(Iss);

  PROTECT(SER = allocMatrix(REALSXP,n_var,n_var));
  SE = REAL(SER);

  for (i=0;i<n_var;i++) {
    for (j=i;j<n_var;j++) {
      SE[i+n_var*j] = SE[j+n_var*i] = NA_REAL;
    }
  }

  for (i=0;i<rnz;i++)
    if (r_nz[i] != c_nz[i])
      SE[r_nz[i]+n_var*c_nz[i]] = SE[c_nz[i]+n_var*r_nz[i]] = Issinv[i*rnz+i];

  Free(Issinv);

  for (i=0;i<n_var;i++)
    SE[i+n_var*i] = NA_REAL;

  Free(r_nz); Free(c_nz);

  UNPROTECT(1); /* SER */

  return SER;
}



/*
  FUNCTION: qp_fast_ipf
  PURPOSE: implement the Iterative Proportional Fitting (IPF) algorithm. Part of the
           R code below has been borrowed from an implementation by Graham Wills in
           June of 1992
  PARAMETERS: vvR - input matrix (usually the sample variance-covariance matrix)
              clqlstR - list of (maximal) cliques
              tolR - tolerance under which the main loop stops
              binary - when set to TRUE the faster C code is used instead
              verbose - when set to TRUE the algorithm shows the successive precision
                        achieve at each iteration
  RETURNS: the input matrix adjusted to the constraints of the list of cliques
*/

static SEXP
qp_fast_ipf(SEXP vvR, SEXP clqlstR, SEXP tolR, SEXP verbose) {
  int           n = INTEGER(getAttrib(vvR,R_DimSymbol))[0];
  int           nclqlst = length(clqlstR);
  double*       V;
  double*       Vold;
  double*       vv;
  double*       tmp;
  double        tol;
  double        diff;
  SEXP          VR;
  Rboolean      clqspervtx;
  int           fstclq = 0;
  PROTECT_INDEX vvpi,tolpi;
  int           pct,ppct;
  int           i,j;

  if (!isMatrix(vvR) || !isNewList(clqlstR)) {
    error("fast.ipf expects a matrix and a list of cliques");
  }

  clqspervtx = INTEGER(VECTOR_ELT(clqlstR,0))[0] > n ? TRUE : FALSE;
  if (clqspervtx && nclqlst <= n+1)
    error("qpIPF: the clique list seem to have vertex-clique indexes but then shows too few cliques\n");
  else if (clqspervtx)
    fstclq = n;

  PROTECT_WITH_INDEX(vvR,&vvpi);
  PROTECT_WITH_INDEX(tolR,&tolpi);

  REPROTECT(vvR  = coerceVector(vvR,REALSXP),vvpi);
  REPROTECT(tolR = coerceVector(tolR,REALSXP),tolpi);
  
  V    = Calloc(n*n,double);
  Vold = Calloc(n*n,double);
  vv   = Calloc(n*n,double);
  tmp  = Calloc(n*n,double);
  tol  = REAL(tolR)[0];

  UNPROTECT(1); /* tol */

  /* V <- diag(length(vv[, 1])) */
  for (i=0;i<n;i++)
    for (j=0;j<n;j++) {
      vv[i+j*n] = REAL(vvR)[i+j*n];
      V[i+j*n] = i == j ? 1.0 : 0.0;
    }
  UNPROTECT(1); /* vv */

  if (INTEGER(verbose)[0])
    Rprintf("qpIPF: %d cliques\n",nclqlst-fstclq);

  diff = 1.0;
  while (diff > tol) {
    /* Vold <- V */
    for (i=0;i<n;i++)
      for (j=0;j<n;j++)
        Vold[i+n*j] = V[i+n*j];

    if (INTEGER(verbose)[0])
      Rprintf("Iterating through cliques (%): ");

    ppct = -1;
    for (i=fstclq;i<nclqlst;i++) {
      SEXP cR;
      int  j;
      int* a;
      int  csze;

      PROTECT(cR = coerceVector(VECTOR_ELT(clqlstR,i),INTSXP));
      csze = length(cR);
      a = Calloc(csze,int);
      for (j=0;j<csze;j++)
        a[j] = INTEGER(cR)[j]-1;
      UNPROTECT(1); /* cR */

      fast_ipf_step(n,vv,V,a,csze);
      Free(a);

      pct = (int) (((i-fstclq)*100)/(nclqlst-fstclq));
      if (pct != ppct) {
        if (INTEGER(verbose)[0]) {
          if (pct % 10 == 0)
            Rprintf("%d",pct);
          else
            Rprintf(".",pct);
          R_FlushConsole();
        }

        R_CheckUserInterrupt();
#ifdef Win32
        R_ProcessEvents();
#endif
#ifdef HAVE_AQUA
        R_ProcessEvents();
#endif
        ppct = pct;
      }
    }
    if (INTEGER(verbose)[0])
      Rprintf("\n");

    matsumf(tmp,n,n,V,Vold,-1.0);
    diff = matmxab(tmp,n,n);
    if (INTEGER(verbose)[0])
      Rprintf("Precision: %.10f\n",diff);
  }

  PROTECT(VR = allocMatrix(REALSXP,n,n));
  Memcpy(REAL(VR),V,(size_t) (n * n));
  UNPROTECT(1); /* VR */

  Free(V);
  Free(Vold);
  Free(vv);
  Free(tmp);

  return VR;
}



/*
  FUNCTION: qp_fast_ipf_step
  PURPOSE: implement the Iterative Proportional Fitting (IPF) algorithm. Part of the
           R code below has been borrowed from an implementation by Graham Wills in
           June of 1992
  PARAMETERS: n - number of rows/columns of the input matrix
              Vf -
              Vn - (Vf and Vn have the same dimensions)
              a - clique
              csze - clique size
  RETURNS: the input matrix adjusted to the constraints of the clique
*/

static void
fast_ipf_step(int n, double* Vf, double* Vn, int* a, int csze) {
  int*    b;     /* vertices the clique a */
  double* Vfaa;
  double* Vnaa;
  double* Vni;
  double* Vnab;
  double* Vnba;
  double* Vnbb;
  double* Bnba;
  double* Vnbba;
  double* tmp1,*tmp2,*tmp3,*tmp4;
  int     i,j;

  b = Calloc(n-csze,int);

  /* b <- (1:length(Vf[, 1]))[ - a] */
  setdiff(n,csze,a,b);

  Vfaa  = Calloc(csze*csze,double);
  Vnaa  = Calloc(csze*csze,double);
  Vni   = Calloc(csze*csze,double);

  /* Vfaa <- Vf[a, a] */
  for (i=0;i<csze;i++)
    for (j=0;j<csze;j++) {
      Vfaa[i+j*csze] = Vf[a[i]+a[j]*n];
      Vni[i+j*csze] = i == j ? 1.0 : 0.0;
    }

  /* Vni <- solve(Vn[a, a]) */
  matsubm(Vnaa,Vn,n,a,csze,a,csze);
  matinv(Vni,Vnaa,csze);

  /* Bnba <- Vn[b, a] %*% Vni */
  Vnba = Calloc((n-csze)*csze,double);
  matsubm(Vnba,Vn,n,b,n-csze,a,csze);
  Bnba = Calloc((n-csze)*csze,double);
  matprod(Vnba,n-csze,csze,Vni,csze,csze,Bnba);

  /* Vnbba <- Vn[b, b] - Vn[b, a] %*% Vni %*% Vn[a, b] */
  Vnbb = Calloc((n-csze)*(n-csze),double);
  matsubm(Vnbb,Vn,n,b,n-csze,b,n-csze);
  Vnab = Calloc(csze*(n-csze),double);
  matsubm(Vnab,Vn,n,a,csze,b,n-csze);
  tmp1 = Calloc(csze*(n-csze),double);
  matprod(Vni,csze,csze,Vnab,csze,n-csze,tmp1);
  tmp2 = Calloc((n-csze)*(n-csze),double);
  matprod(Vnba,n-csze,csze,tmp1,csze,n-csze,tmp2);
  Vnbba = Calloc((n-csze)*(n-csze),double);
  matsumf(Vnbba,n-csze,n-csze,Vnbb,tmp2,-1.0);
  Free(tmp1);
  Free(tmp2);

  /* V <- Vf
  for (i=0;i<n;i++)
    for (j=0;j<ncol;j++)
      Vn[i+n*j] = Vf[i+n*j]; */
  Memcpy(Vn,Vf,(size_t) n*n);

  /* V[b, a] <- Bnba %*% Vfaa */
  tmp1 = Calloc((n-csze)*csze,double);
  matprod(Bnba,n-csze,csze,Vfaa,csze,csze,tmp1); 
  matrepm(Vn,n,b,n-csze,a,csze,tmp1);
  Free(tmp1);

  /* V[a, b] <- t(V[b, a]) */
  matsubm(Vnba,Vn,n,b,n-csze,a,csze);
  mattran(Vnab,Vnba,n-csze,csze);
  matrepm(Vn,n,a,csze,b,n-csze,Vnab);

  /* V[b, b] <- Vnbba + Bnba %*% Vfaa %*% t(Bnba) */
  tmp1 = Calloc(csze*(n-csze),double);
  mattran(tmp1,Bnba,n-csze,csze);
  tmp2 = Calloc(csze*(n-csze),double);
  matprod(Vfaa,csze,csze,tmp1,csze,n-csze,tmp2);
  Free(tmp1);
  tmp3 = Calloc((n-csze)*(n-csze),double);
  matprod(Bnba,n-csze,csze,tmp2,csze,n-csze,tmp3);
  Free(tmp2);
  tmp4 = Calloc((n-csze)*(n-csze),double);
  matsumf(tmp4,n-csze,n-csze,Vnbba,tmp3,1.0);
  Free(tmp3);
  matrepm(Vn,n,b,n-csze,b,n-csze,tmp4);
  Free(tmp4);

  Free(Vfaa);
  Free(Vnaa);
  Free(Vni);
  Free(Vnab);
  Free(Vnba);
  Free(Vnbb);
  Free(Bnba);
  Free(Vnbba);
  Free(b);
}



/*
  FUNCTION: cov2cor
  PURPOSE: this is a C implementation of the cov2cor function from the stats package
  RETURNS: a scaled covariance matrix so that the diagonal equals unity
*/

static void
cov2cor(double* R, double* V, int n) {
  double* Is;
  int     i,j;

  Is = Calloc(n, double);
  for (i=0; i < n; i++)
    Is[i] = sqrt(1.0 / V[i + i*n]);

  for (i=0; i < n; i++) {
    for (j=0; j < i; j++)
      R[i + j*n] = R[j + i*n] = Is[i] * V[i + j*n] * Is[j];      
    R[i + i*n] = 1.0;
  }

  Free(Is);
}



/*
  FUNCTION: matprod
  PURPOSE: multiply two matrices by using the LaPACK library that
           comes along with the R distribution, this code is taken from

           R-2.2.0/src/main/array.c
  RETURNS: none
*/

static void
matprod(double *x, int nrx, int ncx, double *y, int nry, int ncy, double *z) {
    char *transa = "N", *transb = "N";
    int i,  j, k;
    double one = 1.0, zero = 0.0, sum;
    Rboolean have_na = FALSE;

    if (nrx > 0 && ncx > 0 && nry > 0 && ncy > 0) {
        /* Don't trust the BLAS to handle NA/NaNs correctly: PR#4582
         * The test is only O(n) here
         */
        for (i = 0; i < nrx*ncx; i++)
            if (ISNAN(x[i])) {have_na = TRUE; break;}
        if (!have_na)
            for (i = 0; i < nry*ncy; i++)
                if (ISNAN(y[i])) {have_na = TRUE; break;}
        if (have_na) {
            for (i = 0; i < nrx; i++)
                for (k = 0; k < ncy; k++) {
                    sum = 0.0;
                    for (j = 0; j < ncx; j++)
                        sum += x[i + j * nrx] * y[j + k * nry];
                    z[i + k * nrx] = sum;
                }
        } else
            F77_CALL(dgemm)(transa, transb, &nrx, &ncy, &ncx, &one,
                            x, &nrx, y, &nry, &zero, z, &nrx);
    } else /* zero-extent operations should return zeroes */
        for(i = 0; i < nrx*ncy; i++) z[i] = 0;
}



/*
  FUNCTION: matinv
  PURPOSE: calculates de inverse of a matrix by using the LaPACK library
           that comes along with the R distribution, this code is taken from
           the function modLa_dgesv in file

           R-2.2.0/src/modules/lapack/Lapack.c
  RETURNS: none
*/

static void
matinv(double* inv, double* M, int n) {
  int     i,j;
  int     info;
  int*    ipiv;
  double* avals;
  double* work;
  double  anorm;
  double  rcond;
  double  tol = DBL_MIN;

  for (i=0;i<n;i++)
    for (j=0;j<n;j++)
      inv[i+j*n] = i == j ? 1.0 : 0.0; 

  ipiv = (int *) Calloc(n,double);
  avals = (double *) Calloc(n*n,double);
  Memcpy(avals,M,(size_t) (n*n));

  F77_CALL(dgesv)(&n,&n,avals,&n,ipiv,inv,&n,&info);
  if (info < 0)
    error("argument %d of Lapack routine %s had invalid value",-info, "dgesv");
  if (info > 0)
    error("Lapack routine dgesv: system is exactly singular");

  anorm = F77_CALL(dlange)("1", &n, &n, M, &n, (double*) NULL);

  work = (double *) Calloc(4*n,double);

  F77_CALL(dgecon)("1", &n, avals, &n, &anorm, &rcond, work, ipiv, &info);
  if (rcond < tol)
    error("system is computationally singular: reciprocal condition number = %g",rcond);

  Free(ipiv);
  Free(avals);
  Free(work);
}



/*
  FUNCTION: matsumf
  PURPOSE: calculates the sum of two matrices multiplying the second one by a
           scalar factor
  RETURNS: none
*/

static void
matsumf(double* R, int nrow, int ncol, double* M, double* N, double factor) {
  int  i,j;

  for (i=0;i<nrow;i++)
    for (j=0;j<ncol;j++)
      R[i+nrow*j] = M[i+nrow*j] + factor * N[i+nrow*j];
}



/*
  FUNCTION: matscalarprod
  PURPOSE: calculates the scalar product of two matrices
  RETURNS: none
*/

static void
matscalarprod(double* R, int nrow, int ncol, double* M, double* N) {
  int  i,j;

  for (i=0;i<nrow;i++)
    for (j=0;j<ncol;j++)
      R[i+nrow*j] = M[i+nrow*j] * N[i+nrow*j];
}



/*
  FUNCTION: mattran
  PURPOSE: performs the transposition of a matrix
  RETURNS: none
*/

static void
mattran(double* T, double* M, int nrow, int ncol) {
  int  i,j;

  for (i=0;i<nrow;i++)
    for (j=0;j<ncol;j++)
      T[j+ncol*i] = M[i+nrow*j];
}



/*
  FUNCTION: matsubm
  PURPOSE: extracts a submatrix of a matrix
  RETURNS: none
*/

static void
matsubm(double* subM, double* M, int n, int* subrows, int nsubrows,
        int* subcols, int nsubcols) {
  int  i,j;

  for (i=0;i<nsubrows;i++)
    for (j=0;j<nsubcols;j++)
      subM[i+nsubrows*j] = M[subrows[i]+n*subcols[j]];
}



/*
  FUNCTION: matmxab
  PURPOSE: find the absolut maximum value in a matrix of doubles
  RETURNS: none
*/

static double
matmxab(double* M, int nrow, int ncol) {
  int    i,j;
  double maxabs = 0.0;

  for (i=0;i<nrow;i++)
    for (j=0;j<ncol;j++) {
      double abs = M[i+nrow*j] > 0 ? M[i+nrow*j] : -1.0*M[i+nrow*j];
      maxabs = maxabs < abs ? abs : maxabs;
    }

  return maxabs;
}



/*
  FUNCTION: matrepm
  PURPOSE: replace a submatrix with another matrix
  RETURNS: none
*/

static void
matrepm(double* M, int n, int* subrows, int nsubrows,
        int* subcols, int nsubcols, double* N) {
  int  i,j;

  for (i=0;i<nsubrows;i++)
    for (j=0;j<nsubcols;j++)
      M[subrows[i]+n*subcols[j]] = N[i+nsubrows*j];
}



/*
  FUNCTION: matrepm
  PURPOSE: puts in b all integers from 0 till n-1 that are not
           in a where a is a vector of integers of size m
  RETURNS: none
*/

static void
setdiff(int n, int m, int* a, int* b) {
  int i;
  int k = 0;

  for (i=0;i<n;i++) {
    int j=0;
    while (j < m && a[j] != i)
      j++;

    if (j == m && k<n-m)
      b[k++] = i;
  }

}



/*
  FUNCTION: i2e
  PURPOSE: transform a non-negative integer representing an edge into two
           non-negative integers representing the vertices joined by this edge
  PARAMETERS: i - non-negative integer representing an edge
              e_i - vertex joined by the edge
              e_j - vertex joined by the edge, always strictly smaller than e_i
  RETURN: none
*/

void
i2e(int i, int* e_i, int* e_j) {
  *e_i = 1 + (unsigned int) (-0.5 + sqrt(0.25 + 2.0 * ((double) i)));
  *e_j = i - (unsigned int) ((double) ((*e_i)*((*e_i)-1)) / 2.0);
}



/*
  FUNCTION: e2i
  PURPOSE: transform two non-negative integers representing the vertices of an edge
           into a non-negative integer representing this same edge
  PARAMETERS: e_i - vertex joined by the edge
              e_j - vertex joined by the edge, always strictly smaller than e_i
              i - non-negative integer representing an edge
  RETURN: none
*/

int
e2i(int e_i, int e_j, int* i) {
  if (e_i < e_j) { /* e_j should always be smaller than e_i */
    e_i = e_i ^ e_j;
    e_j = e_i ^ e_j;
    e_i = e_i ^ e_j;
  }

  return(((int) (((double) (e_i * (e_i - 1))) / 2.0)) + e_j);
}



/*
  FUNCTION: calculate_means
  PURPOSE: calculate the means of the values at the columns of the input matrix
           provided as a column-major vector
  PARAMETERS: X - vector containing the column-major stored matrix of values
              n_var - number of variables
              N - number of values per variable
              meanv - output vector of mean values
  RETURN: none
*/

void
calculate_means(double* X, int n_var, int N, double* meanv) {
  long double sum, tmp;
  double*     xx;
  int         i, j;

  for (i=0;i < n_var;i++) {
    xx = &X[i * N];
    sum = 0.0;
    for (j=0;j < N;j++)
      sum += xx[j];
    tmp = sum / N;
    if (R_FINITE((double) tmp)) {
      sum = 0.0;
      for (j=0;j < N;j++)
        sum += (xx[j] - tmp);
      tmp = tmp + sum / N;
    }
      meanv[i] = tmp;
  }
}

/*
  FUNCTION: qp_cov_upper_triangular
  PURPOSE: calculate a covariance matrix returning only the upper triangle
           of the matrix in column-major order (for creating later a dspMatrix object)
  PARAMETERS: X - vector containing the column-major stored matrix of values
              n_var - number of variables
              N - number of values per variable
  RETURN: cov_val covariance values
*/

static SEXP
qp_cov_upper_triangular(SEXP XR) {
  SEXP          cov_valR;
  double*       X;
  double*       cov_val;
  double*       meanv;
  int           n_var,N,N1;
  int           i,j,k,l;
  PROTECT_INDEX Xpi;

  PROTECT_WITH_INDEX(XR,&Xpi);

  REPROTECT(XR  = coerceVector(XR,REALSXP),Xpi);
  X = REAL(XR);

  /* number of observations equals number of rows */
  N = INTEGER(getAttrib(XR,R_DimSymbol))[0];
  /* number of variables equals number of columns */
  n_var = INTEGER(getAttrib(XR,R_DimSymbol))[1];

  PROTECT(cov_valR = allocVector(REALSXP, n_var * (n_var+1) / 2));
  cov_val = REAL(cov_valR);

  meanv = Calloc(n_var, double);

  calculate_means(X, n_var, N, meanv);

  N1 = N - 1;

  l = 0;
  for (i=0; i < n_var; i++)
    for (j=0; j <= i; j++) {
      double*     xx;
      double*     yy;
      long double xxm, yym, sum;

      xx  = &X[i * N];
      xxm = meanv[i];
      yy  = &X[j * N];
      yym = meanv[j];
      sum = 0.0;
      for (k=0; k < N; k++)
        sum += (xx[k] - xxm) * (yy[k] - yym);

      cov_val[l++] = (double) (sum / ((long double) N1));
    }

  Free(meanv);

  UNPROTECT(2); /* XR cov_valR */

  return(cov_valR);
}
