//
// Copyright (c) 2022 INRIA
//

#ifndef __proxsuite_fwd_hpp__
#define __proxsuite_fwd_hpp__

#define PROXSUITE_MAYBE_UNUSED [[maybe_unused]]

// Same logic as in Pinocchio to check eigen malloc
#ifdef PROXSUITE_EIGEN_CHECK_MALLOC
#ifndef EIGEN_RUNTIME_NO_MALLOC
#define EIGEN_RUNTIME_NO_MALLOC_WAS_NOT_DEFINED
#define EIGEN_RUNTIME_NO_MALLOC
#endif
#endif

#include <Eigen/Core>
#include <cassert>

#ifdef PROXSUITE_EIGEN_CHECK_MALLOC
#ifdef EIGEN_RUNTIME_NO_MALLOC_WAS_NOT_DEFINED
#undef EIGEN_RUNTIME_NO_MALLOC
#undef EIGEN_RUNTIME_NO_MALLOC_WAS_NOT_DEFINED
#endif
#endif

// Check memory allocation for Eigen
#ifdef PROXSUITE_EIGEN_CHECK_MALLOC
#define PROXSUITE_EIGEN_MALLOC(allowed)                                        \
  ::Eigen::internal::set_is_malloc_allowed(allowed)
#define PROXSUITE_EIGEN_MALLOC_ALLOWED() PROXSUITE_EIGEN_MALLOC(true)
#define PROXSUITE_EIGEN_MALLOC_NOT_ALLOWED() PROXSUITE_EIGEN_MALLOC(false)
#else
#define PROXSUITE_EIGEN_MALLOC(allowed)
#define PROXSUITE_EIGEN_MALLOC_ALLOWED()
#define PROXSUITE_EIGEN_MALLOC_NOT_ALLOWED()
#endif

#endif // #ifndef __proxsuite_fwd_hpp__
