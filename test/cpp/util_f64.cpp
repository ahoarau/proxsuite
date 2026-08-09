#include "util_f64.hpp"

namespace proxsuite {
namespace proxqp {
namespace utils {

namespace eigen {
template auto
llt_compute<Mat<f64, colmajor>>(Eigen::LLT<Mat<f64, colmajor>>&,
                                Mat<f64, colmajor> const&) -> void;
template auto
ldlt_compute<Mat<f64, colmajor>>(Eigen::LDLT<Mat<f64, colmajor>>&,
                                 Mat<f64, colmajor> const&) -> void;
template auto
llt_compute<Mat<f64, rowmajor>>(Eigen::LLT<Mat<f64, rowmajor>>&,
                                Mat<f64, rowmajor> const&) -> void;
template auto
ldlt_compute<Mat<f64, rowmajor>>(Eigen::LDLT<Mat<f64, rowmajor>>&,
                                 Mat<f64, rowmajor> const&) -> void;
} // namespace eigen

namespace rand {
template auto matrix_rand<f64>(isize, isize) -> Mat<f64, colmajor>;
template auto vector_rand<f64>(isize) -> Vec<f64>;
template auto positive_definite_rand<f64>(isize, f64) -> Mat<f64, colmajor>;
template auto orthonormal_rand<f64>(isize) -> Mat<f64, colmajor> const&;
template auto sparse_matrix_rand<f64>(isize, isize, f64) -> SparseMat<f64>;
template auto sparse_positive_definite_rand<f64>(isize, f64, f64)
  -> SparseMat<f64>;
} // namespace rand

template auto
mat_cast<proxqp::f64, long double>(Mat<long double, proxqp::colmajor> const&)
  -> Mat<proxqp::f64, proxqp::colmajor>;

} // namespace utils
} // namespace proxqp
} // namespace proxsuite
