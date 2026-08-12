#pragma once

#include <proxsuite/proxqp/utils/random_qp_problems.hpp>

namespace proxsuite {
namespace proxqp {
namespace utils {

// Instantiated once in util_f64.cpp, so that every test translation unit does
// not have to instantiate these (Eigen-heavy) templates again.
namespace eigen {
PROXSUITE_EXPLICIT_TPL_DECL(2, llt_compute<Mat<f64, colmajor>>);
PROXSUITE_EXPLICIT_TPL_DECL(2, ldlt_compute<Mat<f64, colmajor>>);
PROXSUITE_EXPLICIT_TPL_DECL(2, llt_compute<Mat<f64, rowmajor>>);
PROXSUITE_EXPLICIT_TPL_DECL(2, ldlt_compute<Mat<f64, rowmajor>>);
} // namespace eigen

namespace rand {
PROXSUITE_EXPLICIT_TPL_DECL(2, matrix_rand<f64>);
PROXSUITE_EXPLICIT_TPL_DECL(1, vector_rand<f64>);
PROXSUITE_EXPLICIT_TPL_DECL(2, positive_definite_rand<f64>);
PROXSUITE_EXPLICIT_TPL_DECL(1, orthonormal_rand<f64>);
PROXSUITE_EXPLICIT_TPL_DECL(3, sparse_matrix_rand<f64>);
PROXSUITE_EXPLICIT_TPL_DECL(3, sparse_positive_definite_rand<f64>);
} // namespace rand

PROXSUITE_EXPLICIT_TPL_DECL(1, mat_cast<proxqp::f64, long double>);

} // namespace utils
} // namespace proxqp
} // namespace proxsuite
