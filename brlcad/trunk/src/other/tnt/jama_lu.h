#ifndef JAMA_LU_H
#define JAMA_LU_H

#include "tnt.h"
#include <algorithm>
#include <float.h>
//for min(), max() below

#define JAMA_NEAR_ZERO(val, epsilon)(((val) > -epsilon) && ((val) < epsilon))


namespace JAMA
{

   /** LU Decomposition.
   <P>
   For an m-by-n matrix A with m >= n, the LU decomposition is an m-by-n
   unit lower triangular matrix L, an n-by-n upper triangular matrix U,
   and a permutation vector piv of length m so that A(piv,:) = L*U.
   If m < n, then L is m-by-m and U is m-by-n.
   <P>
   The LU decompostion with pivoting always exists, even if the matrix is
   singular, so the constructor will never fail.  The primary use of the
   LU decomposition is in the solution of square systems of simultaneous
   linear equations.  This will fail if isNonsingular() returns false.
   */
template <class Real>
class LU
{



   /* Array for internal storage of decomposition.  */
   TNT::Array2D<Real>  LU_;
   int m, n, pivsign;
   TNT::Array1D<int> piv;


   TNT::Array2D<Real> permute_copy(const TNT::Array2D<Real> &A,
   			const TNT::Array1D<int> &pivot, int j0, int j1)
	{
		int pivotlength = pivot.dim();

		TNT::Array2D<Real> X(pivotlength, j1-j0+1);


         for (int i = 0; i < pivotlength; i++)
            for (int j = j0; j <= j1; j++)
               X[i][j-j0] = A[pivot[i]][j];

		return X;
	}

   TNT::Array1D<Real> permute_copy(const TNT::Array1D<Real> &A,
   		const TNT::Array1D<int> &pivot)
	{
		int pivotlength = pivot.dim();
		if (pivotlength != A.dim())
			return TNT::Array1D<Real>();

		TNT::Array1D<Real> x(pivotlength);


         for (int i = 0; i < pivotlength; i++)
               x[i] = A[pivot[i]];

		return x;
	}


	public :

   /** LU Decomposition
   @param  A   Rectangular matrix
   @return     LU Decomposition object to access L, U and piv.
   */

    LU (const TNT::Array2D<Real> &A) : LU_(A.copy()), m(A.dim1()), n(A.dim2()),
		piv(A.dim1())

	{

   // Use a "left-looking", dot-product, Crout/Doolittle algorithm.


      for (int i = 0; i < m; i++) {
         piv[i] = i;
      }
      pivsign = 1;
      Real *LUrowi = 0;;
      TNT::Array1D<Real> LUcolj(m);

      // Outer loop.

      for (int j = 0; j < n; j++) {

         // Make a copy of the j-th column to localize references.

         for (int i = 0; i < m; i++) {
            LUcolj[i] = LU_[i][j];
         }

         // Apply previous transformations.

         for (int i = 0; i < m; i++) {
            LUrowi = LU_[i];

            // Most of the time is spent in the following dot product.

            int kmax = std::min<int>(i,j);
            double s = 0.0;
            for (int k = 0; k < kmax; k++) {
               s += LUrowi[k]*LUcolj[k];
            }

            LUrowi[j] = LUcolj[i] -= s;
         }

         // Find pivot and exchange if necessary.

         int p = j;
         for (int i = j+1; i < m; i++) {
            if (std::abs(LUcolj[i]) > std::abs(LUcolj[p])) {
               p = i;
            }
         }
         if (p != j) {
		    int k=0;
            for (k = 0; k < n; k++) {
               double t = LU_[p][k];
			   LU_[p][k] = LU_[j][k];
			   LU_[j][k] = t;
            }
            k = piv[p];
			piv[p] = piv[j];
			piv[j] = k;
            pivsign = -pivsign;
         }

         // Compute multipliers.

         if ((j < m) && (JAMA_NEAR_ZERO(LU_[j][j], DBL_MIN))) {
            for (int i = j+1; i < m; i++) {
               LU_[i][j] /= LU_[j][j];
            }
         }
      }
   }


   /** Is the matrix nonsingular?
   @return     1 (true)  if upper triangular factor U (and hence A)
   				is nonsingular, 0 otherwise.
   */

   int isNonsingular () {
      for (int j = 0; j < n; j++) {
        if (JAMA_NEAR_ZERO(LU_[j][j], DBL_MIN))
            return 0;
      }
      return 1;
   }

   /** Return lower triangular factor
   @return     L
   */

   TNT::Array2D<Real> getL () {
      TNT::Array2D<Real> L_(m,n);
      for (int i = 0; i < m; i++) {
         for (int j = 0; j < n; j++) {
            if (i > j) {
               L_[i][j] = LU_[i][j];
            } else if (i == j) {
               L_[i][j] = 1.0;
            } else {
               L_[i][j] = 0.0;
            }
         }
      }
      return L_;
   }

   /** Return upper triangular factor
   @return     U portion of LU factorization.
   */

   TNT::Array2D<Real> getU () {
   	  TNT::Array2D<Real> U_(n,n);
      for (int i = 0; i < n; i++) {
         for (int j = 0; j < n; j++) {
            if (i <= j) {
               U_[i][j] = LU_[i][j];
            } else {
               U_[i][j] = 0.0;
            }
         }
      }
      return U_;
   }

   /** Return pivot permutation vector
   @return     piv
   */

   TNT::Array1D<int> getPivot () {
      return piv;
   }


   /** Compute determinant using LU factors.
   @return     determinant of A, or 0 if A is not square.
   */

   Real det () {
      if (m != n) {
         return Real(0);
      }
      Real d = Real(pivsign);
      for (int j = 0; j < n; j++) {
         d *= LU_[j][j];
      }
      return d;
   }

   /** Solve A*X = B
   @param  B   A Matrix with as many rows as A and any number of columns.
   @return     X so that L*U*X = B(piv,:), if B is nonconformant, returns
   					0x0 (null) array.
   */

   TNT::Array2D<Real> solve (const TNT::Array2D<Real> &B)
   {

	  /* Dimensions: A is mxn, X is nxk, B is mxk */

      if (B.dim1() != m) {
	  	return TNT::Array2D<Real>(0,0);
      }
      if (!isNonsingular()) {
        return TNT::Array2D<Real>(0,0);
      }

      // Copy right hand side with pivoting
      int nx = B.dim2();


	  TNT::Array2D<Real> X = permute_copy(B, piv, 0, nx-1);

      // Solve L*Y = B(piv,:)
      for (int k = 0; k < n; k++) {
         for (int i = k+1; i < n; i++) {
            for (int j = 0; j < nx; j++) {
               X[i][j] -= X[k][j]*LU_[i][k];
            }
         }
      }
      // Solve U*X = Y;
      for (int k = n-1; k >= 0; k--) {
         for (int j = 0; j < nx; j++) {
            X[k][j] /= LU_[k][k];
         }
         for (int i = 0; i < k; i++) {
            for (int j = 0; j < nx; j++) {
               X[i][j] -= X[k][j]*LU_[i][k];
            }
         }
      }
      return X;
   }


   /** Solve A*x = b, where x and b are vectors of length equal
   		to the number of rows in A.

   @param  b   a vector (TNT::Array1D> of length equal to the first dimension
   						of A.
   @return x a vector (TNT::Array1D> so that L*U*x = b(piv), if B is nonconformant,
   					returns 0x0 (null) array.
   */

   TNT::Array1D<Real> solve (const TNT::Array1D<Real> &b)
   {

	  /* Dimensions: A is mxn, X is nxk, B is mxk */

      if (b.dim1() != m) {
	  	return TNT::Array1D<Real>();
      }
      if (!isNonsingular()) {
        return TNT::Array1D<Real>();
      }


	  TNT::Array1D<Real> x = permute_copy(b, piv);

      // Solve L*Y = B(piv)
      for (int k = 0; k < n; k++) {
         for (int i = k+1; i < n; i++) {
               x[i] -= x[k]*LU_[i][k];
            }
         }

	  // Solve U*X = Y;
      for (int k = n-1; k >= 0; k--) {
            x[k] /= LU_[k][k];
      		for (int i = 0; i < k; i++)
            	x[i] -= x[k]*LU_[i][k];
      }


      return x;
   }

}; /* class LU */

} /* namespace JAMA */

#endif
/* JAMA_LU_H */
