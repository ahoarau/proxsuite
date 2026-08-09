#include "util_f32.hpp"

namespace proxsuite {
namespace proxqp {
namespace utils {

namespace eigen {
template auto
llt_compute<Mat<f32, colmajor>>(Eigen::LLT<Mat<f32, colmajor>>&,
                                Mat<f32, colmajor> const&) -> void;
template auto
ldlt_compute<Mat<f32, colmajor>>(Eigen::LDLT<Mat<f32, colmajor>>&,
                                 Mat<f32, colmajor> const&) -> void;
template auto
llt_compute<Mat<f32, rowmajor>>(Eigen::LLT<Mat<f32, rowmajor>>&,
                                Mat<f32, rowmajor> const&) -> void;
template auto
ldlt_compute<Mat<f32, rowmajor>>(Eigen::LDLT<Mat<f32, rowmajor>>&,
                                 Mat<f32, rowmajor> const&) -> void;
} // namespace eigen

namespace rand {
template auto matrix_rand<f32>(isize, isize) -> Mat<f32, colmajor>;
template auto vector_rand<f32>(isize) -> Vec<f32>;
template auto positive_definite_rand<f32>(isize, f32) -> Mat<f32, colmajor>;
template auto orthonormal_rand<f32>(isize) -> Mat<f32, colmajor> const&;
template auto sparse_matrix_rand<f32>(isize, isize, f32) -> SparseMat<f32>;
template auto sparse_positive_definite_rand<f32>(isize, f32, f32)
  -> SparseMat<f32>;
} // namespace rand

template auto
matmul_impl<long double>(Mat<long double, proxqp::colmajor> const&,
                         Mat<long double, proxqp::colmajor> const&)
  -> Mat<long double, proxqp::colmajor>;
template auto
mat_cast<proxqp::f32, long double>(Mat<long double, proxqp::colmajor> const&)
  -> Mat<proxqp::f32, proxqp::colmajor>;

} // namespace utils
} // namespace proxqp
} // namespace proxsuite
