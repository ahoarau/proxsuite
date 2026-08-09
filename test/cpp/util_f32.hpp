#pragma once

#include <proxsuite/proxqp/utils/random_qp_problems.hpp>

namespace proxsuite {
namespace proxqp {
namespace utils {

// Instantiated once in util_f32.cpp, so that every test translation unit does
// not have to instantiate these (Eigen-heavy) templates again.
namespace eigen {
extern template auto
llt_compute<Mat<f32, colmajor>>(Eigen::LLT<Mat<f32, colmajor>>&,
                                Mat<f32, colmajor> const&) -> void;
extern template auto
ldlt_compute<Mat<f32, colmajor>>(Eigen::LDLT<Mat<f32, colmajor>>&,
                                 Mat<f32, colmajor> const&) -> void;
extern template auto
llt_compute<Mat<f32, rowmajor>>(Eigen::LLT<Mat<f32, rowmajor>>&,
                                Mat<f32, rowmajor> const&) -> void;
extern template auto
ldlt_compute<Mat<f32, rowmajor>>(Eigen::LDLT<Mat<f32, rowmajor>>&,
                                 Mat<f32, rowmajor> const&) -> void;
} // namespace eigen

namespace rand {
extern template auto matrix_rand<f32>(isize, isize) -> Mat<f32, colmajor>;
extern template auto vector_rand<f32>(isize) -> Vec<f32>;
extern template auto positive_definite_rand<f32>(isize, f32)
  -> Mat<f32, colmajor>;
extern template auto orthonormal_rand<f32>(isize) -> Mat<f32, colmajor> const&;
extern template auto sparse_matrix_rand<f32>(isize, isize, f32)
  -> SparseMat<f32>;
extern template auto sparse_positive_definite_rand<f32>(isize, f32, f32)
  -> SparseMat<f32>;
} // namespace rand

extern template auto
matmul_impl<long double>(Mat<long double, proxqp::colmajor> const&,
                         Mat<long double, proxqp::colmajor> const&)
  -> Mat<long double, proxqp::colmajor>;
extern template auto
mat_cast<proxqp::f32, long double>(Mat<long double, proxqp::colmajor> const&)
  -> Mat<proxqp::f32, proxqp::colmajor>;

} // namespace utils
} // namespace proxqp
} // namespace proxsuite
