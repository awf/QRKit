// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2017 Jan Svoboda <jan.svoboda@usi.ch>
// Copyright (C) 2016 Andrew Fitzgibbon <awf@microsoft.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.


// import basic and product tests for deprectaed DynamicSparseMatrix
#define EIGEN_NO_DEPRECATED_WARNING

#include <iostream>
#include <iomanip>
#include <ctime>
#include <future>
#include <random>

#include "main.h"
//#include "sparse_basic.cpp"
//#include "sparse_product.cpp"

#include <Eigen/Eigen>
#include <Eigen/SparseCore>
#include <QRKit/QRKit>

using namespace Eigen;
using namespace QRKit;

typedef double Scalar;

typedef SparseMatrix<Scalar, ColMajor, int> JacobianType;
typedef SparseMatrix<Scalar, RowMajor, int> JacobianTypeRowMajor;
typedef Matrix<Scalar, Dynamic, Dynamic> DenseMatrixType;
typedef HouseholderQR<Matrix<Scalar, Dynamic, Dynamic>> BandBlockQRSolver;
typedef BandedBlockedSparseQR<JacobianType, BandBlockQRSolver, 2, 8, false> BandedBlockedQRSolver;

typedef BandedBlockedQRSolver LeftSuperBlockSolver;
typedef ColPivHouseholderQR<DenseMatrixType> RightSuperBlockSolver;
typedef BlockAngularSparseQR<LeftSuperBlockSolver, RightSuperBlockSolver> BlockAngularQRSolver;
typedef Matrix<Scalar, 7, 2> DenseMatrix7x2;
typedef ColPivHouseholderQR<DenseMatrix7x2 > DenseQRSolver;
typedef BlockDiagonalSparseQR<DenseQRSolver> BlockDiagonalQRSolver;

typedef BlockedThinDenseQR<DenseMatrixType, 2, false> BlockedThinDenseSolver;
typedef BlockedThinSparseQR<JacobianType, 2, false> BlockedThinSparseSolver;

typedef BlockAngularSparseQR<LeftSuperBlockSolver, BlockedThinDenseSolver> BlockAngularQRSolverDenseBlocked;
typedef BlockAngularSparseQR<LeftSuperBlockSolver, BlockedThinSparseSolver> BlockAngularQRSolverDenseBlockedSparse;


/*
* Generate block diagonal sparse matrix with overlapping diagonal blocks.
*/
void generate_overlapping_block_diagonal_matrix(const Eigen::Index numParams, const Eigen::Index numResiduals, JacobianType &spJ, bool permuteRows = true) {
  std::default_random_engine gen;
  std::uniform_real_distribution<double> dist(0.5, 5.0);

  int stride = 7;
  QRKit::TripletArray<Scalar, typename JacobianType::Index> jvals(stride * numParams);
  for (int i = 0; i < numParams; i++) {
    for (int j = i * 2; j < (i * 2) + 2 && j < numParams; j++) {
      jvals.add(i * stride, j, dist(gen));
      jvals.add(i * stride + 1, j, dist(gen));
      jvals.add(i * stride + 2, j, dist(gen));
      jvals.add(i * stride + 3, j, dist(gen));
      jvals.add(i * stride + 4, j, dist(gen));
      jvals.add(i * stride + 5, j, dist(gen));
      jvals.add(i * stride + 6, j, dist(gen));
      if (j < numParams - 2) {
        jvals.add(i * stride + 6, j + 2, dist(gen));
      }
    }
  }

  spJ.resize(numResiduals, numParams);
  spJ.setZero();
  spJ.setFromTriplets(jvals.begin(), jvals.end());
  spJ.makeCompressed();

  // Permute Jacobian rows (we want to see how our QR handles a general matrix)	
  if (permuteRows) {
    PermutationMatrix<Dynamic, Dynamic, JacobianType::StorageIndex> perm(spJ.rows());
    perm.setIdentity();
    std::random_shuffle(perm.indices().data(), perm.indices().data() + perm.indices().size());
    spJ = perm * spJ;
  }
}

/*
* Generate block diagonal sparse matrix.
*/
void generate_block_diagonal_matrix(const Eigen::Index numParams, const Eigen::Index numResiduals, JacobianType &spJ, bool permuteRows = true) {
  std::default_random_engine gen;
  std::uniform_real_distribution<double> dist(0.5, 5.0);

  int stride = 7;
  QRKit::TripletArray<Scalar, typename JacobianType::Index> jvals(stride * numParams);
  for (int i = 0; i < numParams; i++) {
    for (int j = i * 2; j < (i * 2) + 2 && j < numParams; j++) {
      jvals.add(i * stride, j, dist(gen));
      jvals.add(i * stride + 1, j, dist(gen));
      jvals.add(i * stride + 2, j, dist(gen));
      jvals.add(i * stride + 3, j, dist(gen));
      jvals.add(i * stride + 4, j, dist(gen));
      jvals.add(i * stride + 5, j, dist(gen));
      jvals.add(i * stride + 6, j, dist(gen));
    }
  }

  spJ.resize(numResiduals, numParams);
  spJ.setZero();
  spJ.setFromTriplets(jvals.begin(), jvals.end());
  spJ.makeCompressed();

  // Permute Jacobian rows (we want to see how our QR handles a general matrix)	
  if (permuteRows) {
    PermutationMatrix<Dynamic, Dynamic, JacobianType::StorageIndex> perm(spJ.rows());
    perm.setIdentity();
    std::random_shuffle(perm.indices().data(), perm.indices().data() + perm.indices().size());
    spJ = perm * spJ;
  }
}
/*
* Generate block angular sparse matrix with overlapping diagonal blocks.
*/
void generate_block_angular_matrix(const Eigen::Index numParams, const Eigen::Index numAngularParams, const Eigen::Index numResiduals, JacobianType &spJ) {
  std::default_random_engine gen;
  std::uniform_real_distribution<double> dist(0.5, 5.0);

  int stride = 7;
  QRKit::TripletArray<Scalar, typename JacobianType::Index> jvals(stride * numParams + numResiduals * numAngularParams);
  for (int i = 0; i < numParams; i++) {
    for (int j = i * 2; j < (i * 2) + 2 && j < numParams; j++) {
      jvals.add(i * stride, j, dist(gen));
      jvals.add(i * stride + 1, j, dist(gen));
      jvals.add(i * stride + 2, j, dist(gen));
      jvals.add(i * stride + 3, j, dist(gen));
      jvals.add(i * stride + 4, j, dist(gen));
      jvals.add(i * stride + 5, j, dist(gen));
      jvals.add(i * stride + 6, j, dist(gen));
      if (j < numParams - 2) {
        jvals.add(i * stride + 6, j + 2, dist(gen));
      }
    }
  }
  for (int i = 0; i < numResiduals; i++) {
    for (int j = 0; j < numAngularParams; j++) {
      jvals.add(i, numParams + j, dist(gen));
    }
  }

  spJ.resize(numResiduals, numParams + numAngularParams);
  spJ.setZero();
  spJ.setFromTriplets(jvals.begin(), jvals.end());
  spJ.makeCompressed();
}

void test_block_diagonal(const JacobianType &spJ,  const int nVecEvals = 10) {
  /*
  * Solve the problem using the block diagonal QR solver.
  */
  BlockDiagonalQRSolver bdqr;
  // Convert sparse matrix into block diagonal
  SparseBlockDiagonal<DenseMatrix7x2> blkDiag;
  blkDiag.fromBlockDiagonalPattern<JacobianType>(spJ, 7, 2);
  // 1) Factorization
  bdqr.compute(blkDiag);

  // 3) Test simple LS solving
  // Prepare the data
  Eigen::VectorXd bdqrXDense = Eigen::VectorXd::Random(spJ.cols());
  Eigen::VectorXd bdqrVecDense = spJ * bdqrXDense;
  // Solve LS
  Eigen::VectorXd bdqrResDense;
  for (int i = 0; i < nVecEvals; i++) {
    bdqrResDense = bdqr.matrixQ().transpose() * bdqrVecDense;//slvrVec;
  }
  VectorXd bdqrSolved;
  for (int i = 0; i < nVecEvals; i++) {
    bdqrSolved = bdqr.matrixR().topLeftCorner(spJ.cols(), spJ.cols()).template triangularView<Upper>().solve(bdqrResDense.head(spJ.cols()));
  }
  VectorXd bdqrSolvedBackperm = VectorXd::Zero(spJ.cols());
  for (int i = 0; i < spJ.cols(); i++) {
    bdqrSolvedBackperm(bdqr.colsPermutation().indices().coeff(i)) = bdqrSolved(i);
  }
  // 4) Apply computed column reordering
  JacobianType spJPerm = (spJ * bdqr.colsPermutation());

  // 5) Test results
  VERIFY_IS_APPROX(bdqr.matrixQ() * bdqr.matrixR(), spJPerm);
  VERIFY_IS_APPROX(bdqr.matrixQ().transpose() * spJPerm, bdqr.matrixR());
  VERIFY_IS_APPROX(bdqrXDense, bdqrSolvedBackperm);
}

void test_banded_blocked(const JacobianType &spJ, const int nVecEvals = 10) {

  // Auxiliary identity matrix (for later use)
  JacobianType I(spJ.rows(), spJ.rows());
  I.setIdentity();

  /*
  * Solve the problem using the banded blocked QR solver.
  */
  BandedBlockedQRSolver slvr;

  // 1) Factorization
  slvr.compute(spJ);
  // Q * I
  JacobianType slvrQ(spJ.rows(), spJ.rows());
  slvrQ = slvr.matrixQ() * I;
  // Q.T * I
  JacobianType slvrQt(spJ.rows(), spJ.rows());
  slvrQt = slvr.matrixQ().transpose() * I;

  // 3) Test simple LS solving
  // Prepare the data
  Eigen::VectorXd slvrXDense = Eigen::VectorXd::Random(spJ.cols());
  Eigen::VectorXd slvrVecDense = spJ * slvrXDense;
  // Solve LS
  Eigen::VectorXd slvrResDense;	
  slvrVecDense = (slvr.rowsPermutation() * slvrVecDense);
  for (int i = 0; i < nVecEvals; i++) {
    slvrResDense = slvr.matrixQ().transpose() * slvrVecDense;//slvrVec;
  }
  VectorXd solved;
  for (int i = 0; i < nVecEvals; i++) {
    solved = slvr.matrixR().topLeftCorner(spJ.cols(), spJ.cols()).template triangularView<Upper>().solve(slvrResDense.head(spJ.cols()));
  }
  VectorXd solvedBackperm = VectorXd::Zero(spJ.cols());
  for (int i = 0; i < spJ.cols(); i++) {
    solvedBackperm(slvr.colsPermutation().indices().coeff(i)) = solved(i);
  }
  // 4) Apply computed row reordering
  JacobianType spJRowPerm = (slvr.rowsPermutation() * spJ);

  // 5) Test results
  VERIFY_IS_APPROX(slvrQ * slvr.matrixR(), spJRowPerm);
  VERIFY_IS_APPROX(slvrQ.transpose() * spJRowPerm, slvr.matrixR());
  VERIFY_IS_APPROX(slvrQt.transpose() * slvr.matrixR(), spJRowPerm);
  VERIFY_IS_APPROX(slvrQt * spJRowPerm, slvr.matrixR());
  VERIFY_IS_APPROX(slvrXDense, solvedBackperm);
}

void test_block_angular(const JacobianType &spJ, const int numAngularParams, const int nVecEvals = 10) {
  // 6) Solve sparse block angular matrix
  // Factorize
  BlockAngularQRSolver baqr;
  JacobianType leftBlock = spJ.block(0, 0, spJ.rows(), spJ.cols() - numAngularParams);
  DenseMatrixType rightBlock = spJ.block(0, spJ.cols() - numAngularParams, spJ.rows(), numAngularParams);
  BlockMatrix1x2<JacobianType, DenseMatrixType> blkAngular(leftBlock, rightBlock);
  baqr.compute(blkAngular);
  // Prepare the data
  Eigen::VectorXd baqrXDense = Eigen::VectorXd::Random(spJ.cols());
  Eigen::VectorXd baqrVecDense = spJ * baqrXDense;
  // Apply row permutation before solving
  baqrVecDense = baqr.rowsPermutation() * baqrVecDense;
  // Solve LS
  Eigen::VectorXd baqrResDense;
  for (int i = 0; i < nVecEvals; i++) {
    baqrResDense = baqr.matrixQ().transpose() * baqrVecDense;//slvrVec;
  }
  VectorXd baqrSolved;
  for (int i = 0; i < nVecEvals; i++) {
    baqrSolved = baqr.matrixR().topLeftCorner(spJ.cols(), spJ.cols()).template triangularView<Upper>().solve(baqrResDense.head(spJ.cols()));
  }
  VectorXd baqrSolvedBackperm = VectorXd::Zero(spJ.cols());
  for (int i = 0; i < spJ.cols(); i++) {
    baqrSolvedBackperm(baqr.colsPermutation().indices().coeff(i)) = baqrSolved(i);
  }

  VERIFY_IS_APPROX(baqrXDense, baqrSolvedBackperm);
}

void test_block_angular_denseblocked(const JacobianType &spJ, const int numAngularParams, const int nVecEvals = 10) {
  // 6) Solve sparse block angular matrix
  // Factorize
  BlockAngularQRSolverDenseBlocked baqr;
  int rightBlockCols = 384;
  JacobianType leftBlock = spJ.block(0, 0, spJ.rows(), spJ.cols() - numAngularParams);
  DenseMatrixType rightBlock = spJ.block(0, spJ.cols() - numAngularParams, spJ.rows(), numAngularParams);
  BlockMatrix1x2<JacobianType, DenseMatrixType> blkAngular(leftBlock, rightBlock);
  baqr.compute(blkAngular);
  // Prepare the data
  Eigen::VectorXd baqrXDense = Eigen::VectorXd::Random(spJ.cols());
  Eigen::VectorXd baqrVecDense = spJ * baqrXDense;
  // Apply row permutation before solving
  baqrVecDense = baqr.rowsPermutation() * baqrVecDense;
  // Solve LS
  Eigen::VectorXd baqrResDense;
  for (int i = 0; i < nVecEvals; i++) {
    baqrResDense = baqr.matrixQ().transpose() * baqrVecDense;//slvrVec;
  }
  VectorXd baqrSolved;
  for (int i = 0; i < nVecEvals; i++) {
    baqrSolved = baqr.matrixR().topLeftCorner(spJ.cols(), spJ.cols()).template triangularView<Upper>().solve(baqrResDense.head(spJ.cols()));
  }
  VectorXd baqrSolvedBackperm = VectorXd::Zero(spJ.cols());
  for (int i = 0; i < spJ.cols(); i++) {
    baqrSolvedBackperm(baqr.colsPermutation().indices().coeff(i)) = baqrSolved(i);
  }

  VERIFY_IS_APPROX(baqrXDense, baqrSolvedBackperm);
}

void test_block_angular_denseblocked_sparse(const JacobianType &spJ, const int numAngularParams, const int nVecEvals = 10) {
  // 6) Solve sparse block angular matrix
  // Factorize
  BlockAngularQRSolverDenseBlockedSparse baqr;
  int rightBlockCols = 384;
  JacobianType leftBlock = spJ.block(0, 0, spJ.rows(), spJ.cols() - numAngularParams);
  JacobianType rightBlock = spJ.block(0, spJ.cols() - numAngularParams, spJ.rows(), numAngularParams);
  BlockMatrix1x2<JacobianType, JacobianType> blkAngular(leftBlock, rightBlock);
  baqr.compute(blkAngular);
  // Prepare the data
  Eigen::VectorXd baqrXDense = Eigen::VectorXd::Random(spJ.cols());
  Eigen::VectorXd baqrVecDense = spJ * baqrXDense;
  // Apply row permutation before solving
  baqrVecDense = baqr.rowsPermutation() * baqrVecDense;
  // Solve LS
  Eigen::VectorXd baqrResDense;
  for (int i = 0; i < nVecEvals; i++) {
    baqrResDense = baqr.matrixQ().transpose() * baqrVecDense;//slvrVec;
  }
  VectorXd baqrSolved;
  for (int i = 0; i < nVecEvals; i++) {
    baqrSolved = baqr.matrixR().topLeftCorner(spJ.cols(), spJ.cols()).template triangularView<Upper>().solve(baqrResDense.head(spJ.cols()));
  }
  VectorXd baqrSolvedBackperm = VectorXd::Zero(spJ.cols());
  for (int i = 0; i < spJ.cols(); i++) {
    baqrSolvedBackperm(baqr.colsPermutation().indices().coeff(i)) = baqrSolved(i);
  }

  VERIFY_IS_APPROX(baqrXDense, baqrSolvedBackperm);
}

void test_sparse_qr_extra() {
  /*
  * Set-up the problem to be solved
  */
  // Problem size
  Eigen::Index numVars = 256;
  Eigen::Index numParams = numVars * 2;
  Eigen::Index numResiduals = numVars * 3 + numVars + numVars * 3;
  int nVecEvals = 10;

  // Generate the sparse matrix
  JacobianType spJ;
  generate_block_diagonal_matrix(numParams, numResiduals, spJ, false);
  CALL_SUBTEST_1(test_block_diagonal(spJ));


  // Generate the sparse matrix
  generate_block_diagonal_matrix(numParams, numResiduals, spJ, false);
  CALL_SUBTEST_2(test_banded_blocked(spJ));
  generate_overlapping_block_diagonal_matrix(numParams, numResiduals, spJ, false);
  CALL_SUBTEST_3(test_banded_blocked(spJ));
  generate_overlapping_block_diagonal_matrix(numParams, numResiduals, spJ, true);
  CALL_SUBTEST_4(test_banded_blocked(spJ));
  
  // Generate new input 
  numVars = 1024;
  numParams = numVars * 2;
  numResiduals = numVars * 3 + numVars + numVars * 3;
  Eigen::Index numAngularParams = 384; // 128 control points

  // Generate the sparse matrix
  generate_block_angular_matrix(numParams, numAngularParams, numResiduals, spJ);
  CALL_SUBTEST_5(test_block_angular(spJ, numAngularParams));

  // Generate the sparse matrix
  generate_block_angular_matrix(numParams, numAngularParams, numResiduals, spJ);
  CALL_SUBTEST_6(test_block_angular_denseblocked(spJ, numAngularParams));

  // Generate the sparse matrix
  generate_block_angular_matrix(numParams, numAngularParams, numResiduals, spJ);
  CALL_SUBTEST_7(test_block_angular_denseblocked_sparse(spJ, numAngularParams));
}
