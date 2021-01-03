// This file is used during transition from double to Real type.
// TODO: all these functions should be replaced to use Eigen MatrixXr and VectorXr. Then this file should be removed.
#if (defined(YADE_POTENTIAL_PARTICLES) or defined(YADE_POTENTIAL_BLOCKS))
#pragma once

#ifdef __cplusplus
// note: extern "C" strips out any namespaces.
extern "C" {
#endif

/* LAPACK LU */
//int dgesv(int varNo, int varNo2, double *H, int varNo3, int *pivot, double* g, int varNo4, int info){
extern void dgesv_(const int* N, const int* nrhs, double* Hessian, const int* lda, int* ipiv, double* gradient, const int* ldb, int* info);
// int ans;
// dgesv_(&varNo, &varNo2, H, &varNo3, pivot,g, &varNo4, &ans);
// return ans;
//}

/* LAPACK Cholesky */
extern void dpbsv_(const char* uplo, const int* n, const int* kd, const int* nrhs, double* AB, const int* ldab, double* B, const int* ldb, int* info);

/* LAPACK QR */
// not used.
//	extern void dgels_(const char *Trans, const int *m, const int *n, const int *nrhs, double *A, const int *lda, double *B, const int *ldb, const double *work, const int *lwork, int *info);


/*BLAS */
extern void
dgemm_(const char*   transA,
       const char*   transB,
       const int*    m,
       const int*    n,
       const int*    k,
       const double* alpha,
       double*       A,
       const int*    lda,
       double*       B,
       const int*    ldb,
       const double* beta,
       double*       C,
       const int*    ldc);

extern void
dgemv_(const char*   trans,
       const int*    m,
       const int*    n,
       const double* alpha,
       double*       A,
       const int*    lda,
       double*       x,
       const int*    incx,
       const double* beta,
       double*       y,
       const int*    incy);

extern void dcopy_(const int* N, double* x, const int* incx, double* y, const int* incy);

extern double ddot_(const int* N, double* x, const int* incx, double* y, const int* incy);

extern void daxpy_(const int* N, const double* da, double* dx, const int* incx, double* dy, const int* incy);

extern void dscal_(const int* N, const double* alpha, double* x, const int* incx);

void dsyev_(const char* jobz, const char* uplo, const int* N, double* A, const int* lda, double* W, double* work, int* lwork, int* info);

#ifdef __cplusplus
};
#endif

#if defined(YADE_REAL_BIT) and (YADE_REAL_BIT != 64)
#include <lib/high-precision/Real.hpp>

std::vector<double> toDoubleVec(const ::yade::Real* from, int size);

void toRealArrPtr(const std::vector<double>& from, ::yade::Real* to, int size);

void dgesv_(const int* N, const int* nrhs, ::yade::Real* Hessian_Real, const int* lda, int* ipiv, ::yade::Real* gradient_Real, const int* ldb, int* info);

void dpbsv_(
        const char*   uplo,
        const int*    n,
        const int*    kd,
        const int*    nrhs,
        ::yade::Real* AB_Real,
        const int*    ldab,
        ::yade::Real* B_Real,
        const int*    ldb,
        int*          info);

void dgemm_(
        const char*         transA,
        const char*         transB,
        const int*          m,
        const int*          n,
        const int*          k,
        const ::yade::Real* alpha_Real,
        ::yade::Real*       A_Real,
        const int*          lda,
        ::yade::Real*       B_Real,
        const int*          ldb,
        const ::yade::Real* beta_Real,
        ::yade::Real*       C_Real,
        const int*          ldc);

void dgemv_(
        const char*         trans,
        const int*          m,
        const int*          n,
        const ::yade::Real* alpha_Real,
        ::yade::Real*       A_Real,
        const int*          lda,
        ::yade::Real*       x_Real,
        const int*          incx,
        const ::yade::Real* beta_Real,
        ::yade::Real*       y_Real,
        const int*          incy);

void dcopy_(const int* N, ::yade::Real* from_x, const int* incx, ::yade::Real* to_y, const int* incy);

::yade::VectorXr toVectorXr(const ::yade::Real* from, int size);

::yade::Real ddot_(const int* N, const ::yade::Real* x, const int* incx, const ::yade::Real* y, const int* incy);

void daxpy_(const int* N, const ::yade::Real* da_Real, ::yade::Real* dx_Real, const int* incx, ::yade::Real* dy_Real, const int* incy);

void dscal_(const int* N, const ::yade::Real* alpha_Real, ::yade::Real* x_Real, const int* incx);

void dsyev_(
        const char*   jobz,
        const char*   uplo,
        const int*    N,
        ::yade::Real* A_Real,
        const int*    lda,
        ::yade::Real* W_Real,
        ::yade::Real* work_Real,
        int*          lwork,
        int*          info);

#endif
#endif
