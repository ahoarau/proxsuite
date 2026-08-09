//
// Copyright (c) 2022 INRIA
//

#ifndef PROXSUITE_FWD_HPP
#define PROXSUITE_FWD_HPP

// `inline`, upgraded to a hard always-inline request in optimized builds. Used
// on the small accessors of the linear algebra backends, where the call
// overhead would otherwise dominate.
#if defined(NDEBUG) || defined(__OPTIMIZE__)
#if defined(__GNUC__) || defined(__clang__)
#define PROXSUITE_INLINE __attribute__((__always_inline__)) inline
#elif defined(_MSC_VER)
#define PROXSUITE_INLINE __forceinline
#else
#define PROXSUITE_INLINE inline
#endif
#else
#define PROXSUITE_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#define PROXSUITE_NO_INLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
#define PROXSUITE_NO_INLINE __declspec(noinline)
#else
#define PROXSUITE_NO_INLINE
#endif

// Same logic as in Pinocchio to check eigen malloc
#ifdef PROXSUITE_EIGEN_CHECK_MALLOC
#ifndef EIGEN_RUNTIME_NO_MALLOC
#define EIGEN_RUNTIME_NO_MALLOC_WAS_NOT_DEFINED
#define EIGEN_RUNTIME_NO_MALLOC
#endif
#endif

#include <Eigen/Core>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace proxsuite {
/// Signed size/index type, used throughout the linear algebra backends.
using isize = std::ptrdiff_t;
/// Unsigned size type.
using usize = std::size_t;

namespace detail {
template<typename T>
struct TypeIdentity
{
  using type = T;
};
} // namespace detail

/*!
 * Blocks template argument deduction on a function parameter, so that its type
 * is fixed by the other arguments instead. `std::type_identity_t` is C++20.
 */
template<typename T>
using DoNotDeduce = typename detail::TypeIdentity<T>::type;

/// `std::remove_cvref_t` is C++20; this is its C++17 spelling.
template<typename T>
using remove_cvref_t =
  typename std::remove_cv<typename std::remove_reference<T>::type>::type;
} // namespace proxsuite

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

#endif // #ifndef PROXSUITE_FWD_HPP
