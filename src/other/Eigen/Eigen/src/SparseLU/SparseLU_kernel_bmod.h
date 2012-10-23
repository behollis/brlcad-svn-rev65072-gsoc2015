// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2012 Désiré Nuentsa-Wakam <desire.nuentsa_wakam@inria.fr>
// Copyright (C) 2012 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SPARSELU_KERNEL_BMOD_H
#define SPARSELU_KERNEL_BMOD_H

/**
 * \brief Performs numeric block updates from a given supernode to a single column
 * 
 * \param segsize Size of the segment (and blocks ) to use for updates
 * \param [in,out]dense Packed values of the original matrix
 * \param tempv temporary vector to use for updates
 * \param lusup array containing the supernodes
 * \param nsupr Number of rows in the supernode
 * \param nrow Number of rows in the rectangular part of the supernode
 * \param lsub compressed row subscripts of supernodes
 * \param lptr pointer to the first column of the current supernode in lsub
 * \param no_zeros Number of nonzeros elements before the diagonal part of the supernode
 * \return 0 on success
 */
template <int SegSizeAtCompileTime> struct LU_kernel_bmod
{
  template <typename BlockScalarVector, typename ScalarVector, typename IndexVector>
  EIGEN_DONT_INLINE static void run(const int segsize, BlockScalarVector& dense, ScalarVector& tempv, ScalarVector& lusup, int& luptr, const int nsupr, const int nrow, IndexVector& lsub, const int lptr, const int no_zeros)
  {
    typedef typename ScalarVector::Scalar Scalar;
    // First, copy U[*,j] segment from dense(*) to tempv(*)
    // The result of triangular solve is in tempv[*]; 
      // The result of matric-vector update is in dense[*]
    int isub = lptr + no_zeros; 
    int i, irow;
    for (i = 0; i < ((SegSizeAtCompileTime==Dynamic)?segsize:SegSizeAtCompileTime); i++)
    {
      irow = lsub(isub); 
      tempv(i) = dense(irow); 
      ++isub; 
    }
    // Dense triangular solve -- start effective triangle
    luptr += nsupr * no_zeros + no_zeros; 
    // Form Eigen matrix and vector 
    Map<Matrix<Scalar,SegSizeAtCompileTime,SegSizeAtCompileTime>, 0, OuterStride<> > A( &(lusup.data()[luptr]), segsize, segsize, OuterStride<>(nsupr) );
    Map<Matrix<Scalar,SegSizeAtCompileTime,1> > u(tempv.data(), segsize);
    
    u = A.template triangularView<UnitLower>().solve(u); 
    
    // Dense matrix-vector product y <-- B*x 
    luptr += segsize;
    Map<Matrix<Scalar,Dynamic,SegSizeAtCompileTime>, 0, OuterStride<> > B( &(lusup.data()[luptr]), nrow, segsize, OuterStride<>(nsupr) );
    Map<Matrix<Scalar,Dynamic,1> > l(tempv.data()+segsize, nrow);
    if(SegSizeAtCompileTime==2)
      l = u(0) * B.col(0) + u(1) * B.col(1);
    else if(SegSizeAtCompileTime==3)
      l = u(0) * B.col(0) + u(1) * B.col(1) + u(2) * B.col(2);
    else
      l.noalias() = B * u;
    
    // Scatter tempv[] into SPA dense[] as a temporary storage 
    isub = lptr + no_zeros;
    for (i = 0; i < ((SegSizeAtCompileTime==Dynamic)?segsize:SegSizeAtCompileTime); i++)
    {
      irow = lsub(isub++); 
      dense(irow) = tempv(i);
    }
    
    // Scatter l into SPA dense[]
    for (i = 0; i < nrow; i++)
    {
      irow = lsub(isub++); 
      dense(irow) -= l(i);
    } 
  }
};

template <> struct LU_kernel_bmod<1>
{
  template <typename BlockScalarVector, typename ScalarVector, typename IndexVector>
  EIGEN_DONT_INLINE static void run(const int /*segsize*/, BlockScalarVector& dense, ScalarVector& /*tempv*/, ScalarVector& lusup, int& luptr, const int nsupr, const int nrow, IndexVector& lsub, const int lptr, const int no_zeros)
  {
    typedef typename ScalarVector::Scalar Scalar;
    Scalar f = dense(lsub(lptr + no_zeros));
    luptr += nsupr * no_zeros + no_zeros + 1;
    const Scalar* a(lusup.data() + luptr);
    const typename IndexVector::Scalar*  irow(lsub.data()+lptr + no_zeros + 1);
    int i = 0;
    for (; i+1 < nrow; i+=2)
    {
      int i0 = *(irow++);
      int i1 = *(irow++);
      Scalar a0 = *(a++);
      Scalar a1 = *(a++);
      Scalar d0 = dense.coeff(i0);
      Scalar d1 = dense.coeff(i1);
      d0 -= f*a0;
      d1 -= f*a1;
      dense.coeffRef(i0) = d0;
      dense.coeffRef(i1) = d1;
    }
    if(i<nrow)
      dense.coeffRef(*(irow++)) -= f * *(a++);
  }
};
#endif
