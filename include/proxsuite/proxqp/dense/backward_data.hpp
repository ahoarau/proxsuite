//
// Copyright (c) 2022 INRIA
//
/**
 * @file results.hpp
 */
#pragma once

#include <proxsuite/helpers/optional.hpp>
#include <type_traits>
#include "proxsuite/proxqp/dense/fwd.hpp"

namespace proxsuite {
namespace proxqp {
namespace dense {

///
/// @brief This class stores the jacobians of PROXQP solvers with
/// dense backends at a solutions wrt model parameters.
///
/*!
 * Jacobian class of dense and sparse solver.
 */
template<typename T>
struct BackwardData
{
  ///// jacobians of solutions wrt model parameters
  using Mat = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

  // dL_dH
  Mat dL_dH;
  // dL_dg
  Vec<T> dL_dg;
  // dL_dA
  Mat dL_dA;
  // dL_db
  Vec<T> dL_db;
  // dL_dC
  Mat dL_dC;
  // dL_du
  Vec<T> dL_du;
  // dL_dl
  Vec<T> dL_dl;

  bool is_valid(isize dim, isize n_eq, isize n_in)
  {
    // check that all matrices and vectors of qpmodel have the correct size
    // and that H and C have expected properties.
    // A dimension of zero means "not set", and is accepted; any other value
    // has to match exactly.
    auto matches = [](isize size, isize expected) {
      return size == 0 || size == expected;
    };

    // dL_dH
    if (!dL_dH.size() || !matches(dL_dH.rows(), dim) ||
        !matches(dL_dH.cols(), dim)) {
      return false;
    }
    // dL_dg
    if (!dL_dg.size() || !matches(dL_dg.rows(), dim)) {
      return false;
    }
    // dL_dA
    if (!dL_dA.size() || !matches(dL_dA.rows(), n_eq) ||
        !matches(dL_dA.cols(), dim)) {
      return false;
    }
    // dL_db
    if (!dL_db.size() || !matches(dL_db.rows(), n_eq)) {
      return false;
    }
    // dL_dC
    if (!dL_dC.size() || !matches(dL_dC.rows(), n_in) ||
        !matches(dL_dC.cols(), dim)) {
      return false;
    }
    // dL_du
    if (!dL_du.size() || !matches(dL_du.rows(), n_in)) {
      return false;
    }
    // dL_dl
    if (!dL_dl.size() || !matches(dL_dl.rows(), n_in)) {
      return false;
    }
    return true;
  }

  void initialize(isize dim, isize n_eq, isize n_in)
  {
    bool valid_dimensions = is_valid(dim, n_eq, n_in);
    if (valid_dimensions == false) {

      dL_dH.resize(dim, dim);
      dL_dg.resize(dim);
      dL_dA.resize(n_eq, dim);
      dL_db.resize(n_eq);
      dL_dC.resize(n_in, dim);
      dL_du.resize(n_in);
      dL_dl.resize(n_in);
    }
    dL_dH.setZero();
    dL_dg.setZero();
    dL_dA.setZero();
    dL_db.setZero();
    dL_dC.setZero();
    dL_du.setZero();
    dL_dl.setZero();
  }
};

} // namespace dense
} // namespace proxqp
} // namespace proxsuite
