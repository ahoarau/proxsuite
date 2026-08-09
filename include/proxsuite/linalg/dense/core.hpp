/** \file */
//
// Copyright (c) 2022 INRIA
//
#pragma once

#include <proxsuite/fwd.hpp>
#include <proxsuite/linalg/dynstack.hpp>

#include <cassert>
#include <cstdint>
#include <type_traits>

#if !(defined(__aarch64__) || defined(__PPC64__) || defined(__ppc64__) ||      \
      defined(_ARCH_PPC64))
#include <immintrin.h>
#endif

#ifdef PROXSUITE_VECTORIZE
#include <cmath> // to avoid error of the type no member named 'isnan' in namespace 'std';
#include <simde/x86/avx2.h>
#include <simde/x86/fma.h>
#endif

#include <Eigen/Core>

// Pastes the current line number onto `id`, so that a macro can declare a
// uniquely named variable in the caller's scope.
#define PROXSUITE_LDLT_CONCAT_IMPL(a, b) a##b
#define PROXSUITE_LDLT_CONCAT(a, b) PROXSUITE_LDLT_CONCAT_IMPL(a, b)
#define PROXSUITE_LDLT_ID(id) PROXSUITE_LDLT_CONCAT(id, __LINE__)

#define PROXSUITE_LDLT_TEMP_VEC_IMPL(Type, Name, Rows, Stack, Make)            \
  auto PROXSUITE_LDLT_ID(vec_storage) = (Stack).template Make<Type>(           \
    (Rows), ::proxsuite::linalg::dense::_detail::align<Type>());               \
  auto Name /* NOLINT */ =                                                     \
    ::Eigen::Map<::Eigen::Matrix<Type, ::Eigen::Dynamic, 1>,                   \
                 ::Eigen::Unaligned,                                           \
                 ::Eigen::Stride<::Eigen::Dynamic, 1>>{                        \
      PROXSUITE_LDLT_ID(vec_storage).ptr_mut(),                                \
      PROXSUITE_LDLT_ID(vec_storage).len(),                                    \
      ::Eigen::Stride<::Eigen::Dynamic, 1>{                                    \
        PROXSUITE_LDLT_ID(vec_storage).len(),                                  \
        1,                                                                     \
      },                                                                       \
    };                                                                         \
  static_assert(true, ".")

#define PROXSUITE_LDLT_TEMP_MAT_IMPL(Type, Name, Rows, Cols, Stack, Make)      \
  ::proxsuite::isize PROXSUITE_LDLT_ID(rows) = (Rows);                         \
  ::proxsuite::isize PROXSUITE_LDLT_ID(cols) = (Cols);                         \
  ::proxsuite::isize PROXSUITE_LDLT_ID(stride) =                               \
    ::proxsuite::linalg::dense::_detail::adjusted_stride<Type>(                \
      PROXSUITE_LDLT_ID(rows));                                                \
  auto PROXSUITE_LDLT_ID(vec_storage) = (Stack).template Make<Type>(           \
    PROXSUITE_LDLT_ID(stride) * PROXSUITE_LDLT_ID(cols),                       \
    ::proxsuite::linalg::dense::_detail::align<Type>());                       \
  auto Name /* NOLINT */ = ::Eigen::Map<                                       \
    ::Eigen::                                                                  \
      Matrix<Type, ::Eigen::Dynamic, ::Eigen::Dynamic, ::Eigen::ColMajor>,     \
    ::Eigen::Unaligned,                                                        \
    ::Eigen::Stride<::Eigen::Dynamic, 1>>{                                     \
    PROXSUITE_LDLT_ID(vec_storage).ptr_mut(),                                  \
    PROXSUITE_LDLT_ID(rows),                                                   \
    PROXSUITE_LDLT_ID(cols),                                                   \
    ::Eigen::Stride<::Eigen::Dynamic, 1>{                                      \
      PROXSUITE_LDLT_ID(stride),                                               \
      1,                                                                       \
    },                                                                         \
  };                                                                           \
  static_assert(true, ".")

#define PROXSUITE_LDLT_TEMP_VEC(Type, Name, Rows, Stack)                       \
  PROXSUITE_LDLT_TEMP_VEC_IMPL(Type, Name, Rows, Stack, make_new)
#define PROXSUITE_LDLT_TEMP_VEC_UNINIT(Type, Name, Rows, Stack)                \
  PROXSUITE_LDLT_TEMP_VEC_IMPL(Type, Name, Rows, Stack, make_new_for_overwrite)

#define PROXSUITE_LDLT_TEMP_MAT(Type, Name, Rows, Cols, Stack)                 \
  PROXSUITE_LDLT_TEMP_MAT_IMPL(Type, Name, Rows, Cols, Stack, make_new)
#define PROXSUITE_LDLT_TEMP_MAT_UNINIT(Type, Name, Rows, Cols, Stack)          \
  PROXSUITE_LDLT_TEMP_MAT_IMPL(                                                \
    Type, Name, Rows, Cols, Stack, make_new_for_overwrite)

namespace proxsuite {
namespace linalg {
namespace dense {
using proxsuite::isize;
using proxsuite::usize;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using f32 = float;
using f64 = double;

namespace _detail {
namespace _simd {

#ifdef __clang__
#define DENSE_LDLT_FP_PRAGMA _Pragma("STDC FP_CONTRACT ON")
#else
#define DENSE_LDLT_FP_PRAGMA
#endif

static_assert(static_cast<unsigned char>(-1) == 255, "char should have 8 bits");
static_assert(sizeof(f32) == 4, "f32 should be 32 bits");
static_assert(sizeof(f64) == 8, "f64 should be 64 bits");

#define LDLT_FN_IMPL3(Fn, Prefix, Suffix)                                      \
  PROXSUITE_INLINE static auto Fn(Pack a, Pack b, Pack c) noexcept -> Pack     \
  {                                                                            \
    return Pack{ simde_mm##Prefix##_##Fn##_##Suffix(                           \
      a.inner, b.inner, c.inner) };                                            \
  }                                                                            \
  static_assert(true, ".")

#define LDLT_ARITHMETIC_IMPL(Prefix, Suffix)                                   \
  LDLT_FN_IMPL3(fmadd, Prefix, Suffix);  /* (a * b + c) */                     \
  LDLT_FN_IMPL3(fnmadd, Prefix, Suffix); /* (-a * b + c) */

#define LDLT_LOAD_STORE(Prefix, Suffix)                                        \
  PROXSUITE_INLINE static auto load_unaligned(ScalarType const* ptr) noexcept  \
    -> Pack                                                                    \
  {                                                                            \
    return Pack{ simde_mm##Prefix##_loadu_##Suffix(ptr) };                     \
  }                                                                            \
  PROXSUITE_INLINE static auto broadcast(ScalarType value) noexcept -> Pack    \
  {                                                                            \
    return Pack{ simde_mm##Prefix##_set1_##Suffix(value) };                    \
  }                                                                            \
  PROXSUITE_INLINE void store_unaligned(ScalarType* ptr) const noexcept        \
  {                                                                            \
    simde_mm##Prefix##_storeu_##Suffix(ptr, inner);                            \
  }                                                                            \
  static_assert(true, ".")

template<typename T, usize N>
struct Pack;

template<typename T>
struct Pack<T, 1>
{
  using ScalarType = T;

  T inner;

  PROXSUITE_INLINE static auto fmadd(Pack a, Pack b, Pack c) noexcept -> Pack
  {
    DENSE_LDLT_FP_PRAGMA
    return { a.inner * b.inner + c.inner };
  }
  PROXSUITE_INLINE static auto fnmadd(Pack a, Pack b, Pack c) noexcept -> Pack
  {
    return fmadd({ -a.inner }, b, c);
  }
  PROXSUITE_INLINE static auto load_unaligned(ScalarType const* ptr) noexcept
    -> Pack
  {
    return { *ptr };
  }
  PROXSUITE_INLINE static auto broadcast(ScalarType value) noexcept -> Pack
  {
    return { value };
  }
  PROXSUITE_INLINE void store_unaligned(ScalarType* ptr) const noexcept
  {
    *ptr = inner;
  }
};

#ifdef PROXSUITE_VECTORIZE
template<>
struct Pack<f32, 4>
{
  using ScalarType = f32;

  simde__m128 inner;
  LDLT_ARITHMETIC_IMPL(, ps)
  LDLT_LOAD_STORE(, ps);
};

template<>
struct Pack<f32, 8>
{
  using ScalarType = f32;

  simde__m256 inner;
  LDLT_ARITHMETIC_IMPL(256, ps)
  LDLT_LOAD_STORE(256, ps);
};

#ifdef __AVX512F__
template<>
struct Pack<f32, 16>
{
  using ScalarType = f32;

  __m512 inner;
  PROXSUITE_INLINE static auto fmadd(Pack a, Pack b, Pack c) noexcept -> Pack
  {
    DENSE_LDLT_FP_PRAGMA
    return { _mm512_fmadd_ps(a.inner, b.inner, c.inner) };
  }
  PROXSUITE_INLINE static auto fnmadd(Pack a, Pack b, Pack c) noexcept -> Pack
  {
    return { _mm512_fnmadd_ps(a.inner, b.inner, c.inner) };
  }
  PROXSUITE_INLINE static auto load_unaligned(ScalarType const* ptr) noexcept
    -> Pack
  {
    return { _mm512_loadu_ps(ptr) };
  }
  PROXSUITE_INLINE static auto broadcast(ScalarType value) noexcept -> Pack
  {
    return { _mm512_set1_ps(value) };
  }
  PROXSUITE_INLINE void store_unaligned(ScalarType* ptr) const noexcept
  {
    _mm512_storeu_ps(ptr, inner);
  }
};
#endif

template<>
struct Pack<f64, 2>
{
  using ScalarType = f64;

  simde__m128d inner;
  LDLT_ARITHMETIC_IMPL(, pd)
  LDLT_LOAD_STORE(, pd);
};
template<>
struct Pack<f64, 4>
{
  using ScalarType = f64;

  simde__m256d inner;
  LDLT_ARITHMETIC_IMPL(256, pd)
  LDLT_LOAD_STORE(256, pd);
};

#ifdef __AVX512F__
template<>
struct Pack<f64, 8>
{
  using ScalarType = f64;

  __m512d inner;
  PROXSUITE_INLINE static auto fmadd(Pack a, Pack b, Pack c) noexcept -> Pack
  {
    DENSE_LDLT_FP_PRAGMA
    return { _mm512_fmadd_pd(a.inner, b.inner, c.inner) };
  }
  PROXSUITE_INLINE static auto fnmadd(Pack a, Pack b, Pack c) noexcept -> Pack
  {
    return { _mm512_fnmadd_pd(a.inner, b.inner, c.inner) };
  }
  PROXSUITE_INLINE static auto load_unaligned(ScalarType const* ptr) noexcept
    -> Pack
  {
    return { _mm512_loadu_pd(ptr) };
  }
  PROXSUITE_INLINE static auto broadcast(ScalarType value) noexcept -> Pack
  {
    return { _mm512_set1_pd(value) };
  }
  PROXSUITE_INLINE void store_unaligned(ScalarType* ptr) const noexcept
  {
    _mm512_storeu_pd(ptr, inner);
  }
};
#endif

#endif

template<typename T>
struct NativePackInfo
{
  static constexpr usize N = 1;
  using Type = Pack<f32, N>;
};

#ifdef PROXSUITE_VECTORIZE
template<>
struct NativePackInfo<f32>
{
  static constexpr usize N = SIMDE_NATURAL_VECTOR_SIZE / 32;
  using Type = Pack<f32, N>;
};
template<>
struct NativePackInfo<f64>
{
  static constexpr usize N = SIMDE_NATURAL_VECTOR_SIZE / 64;
  using Type = Pack<f64, N>;
};
#endif

template<typename T>
using NativePack = typename NativePackInfo<T>::Type;
#undef LDLT_ARITHMETIC_IMPL
#undef LDLT_LOAD_STORE
#undef LDLT_FN_IMPL3
#undef DENSE_LDLT_FP_PRAGMA

} // namespace _simd
} // namespace _detail

namespace _detail {
using proxsuite::remove_cvref_t;
template<bool COND, typename T>
using const_if = std::conditional_t<COND, T const, T>;

template<typename T>
using ptr_is_const =
  std::bool_constant<std::is_const<std::remove_pointer_t<T>>::value>;

template<typename T>
constexpr auto
round_up(T a, T b) noexcept -> T
{
  return a + (b - 1) / b * b;
}

#ifdef PROXSUITE_VECTORIZE
template<typename T>
using should_vectorize = std::bool_constant<std::is_same<T, f32>::value ||
                                            std::is_same<T, f64>::value>;
#else
template<typename T>
using should_vectorize = std::bool_constant<false>;
#endif

template<typename T>
auto
adjusted_stride(isize n) noexcept -> isize
{
#ifndef SIMDE_NATURAL_FLOAT_VECTOR_SIZE
  isize simd_stride = 1;
#else
  isize simd_stride =
    (SIMDE_NATURAL_VECTOR_SIZE / CHAR_BIT) / isize{ sizeof(T) };
#endif
  return _detail::should_vectorize<T>::value ? _detail::round_up(n, simd_stride)
                                             : n;
}
template<typename T>
auto
align() noexcept -> isize
{
  return isize{ alignof(T) } * _detail::adjusted_stride<T>(1);
}

struct NoCopy
{
  NoCopy() = default;
  ~NoCopy() = default;

  NoCopy(NoCopy const&) = delete;
  NoCopy(NoCopy&&) = delete;
  auto operator=(NoCopy const&) -> NoCopy& = delete;
  auto operator=(NoCopy&&) -> NoCopy& = delete;
};

template<typename T>
PROXSUITE_INLINE constexpr auto
max2(T const& a, T const& b) -> T const&
{
  return a > b ? a : b;
}
template<typename T>
PROXSUITE_INLINE constexpr auto
min2(T a, T b) -> T
{
  return (a < b) ? a : b;
}

template<typename T>
void
set_zero(T* dest, usize n)
{
  for (usize i = 0; i < n; ++i) {
    *dest = 0;
  }
}

template<typename T>
using OwnedMatrix =
  Eigen::Matrix<typename T::Scalar,
                Eigen::Dynamic,
                Eigen::Dynamic,
                bool(T::IsRowMajor) ? Eigen::RowMajor : Eigen::ColMajor>;

template<typename T>
using OwnedAll =
  Eigen::Matrix<typename T::Scalar,
                T::RowsAtCompileTime,
                T::ColsAtCompileTime,
                bool(T::IsRowMajor) ? Eigen::RowMajor : Eigen::ColMajor>;

template<typename T>
using OwnedRows =
  Eigen::Matrix<typename T::Scalar,
                Eigen::Dynamic,
                T::ColsAtCompileTime,
                bool(T::IsRowMajor) ? Eigen::RowMajor : Eigen::ColMajor>;

template<typename T>
using OwnedCols =
  Eigen::Matrix<typename T::Scalar,
                T::RowsAtCompileTime,
                Eigen::Dynamic,
                bool(T::IsRowMajor) ? Eigen::RowMajor : Eigen::ColMajor>;

template<typename T>
using OwnedColVector = Eigen::Matrix< //
  typename T::Scalar,
  Eigen::Dynamic,
  1,
  Eigen::ColMajor>;
template<typename T>
using OwnedRowVector = Eigen::Matrix< //
  typename T::Scalar,
  1,
  Eigen::Dynamic,
  Eigen::RowMajor>;

template<bool ROWMAJOR>
struct ElemAddrImpl;

template<>
struct ElemAddrImpl<false>
{
  template<typename T>
  static auto fn(T* ptr,
                 isize row,
                 isize col,
                 isize outer_stride,
                 isize inner_stride) noexcept -> T*
  {
    return ptr + (inner_stride * row + outer_stride * col);
  }
};
template<>
struct ElemAddrImpl<true>
{
  template<typename T>
  static auto fn(T* ptr,
                 isize row,
                 isize col,
                 isize outer_stride,
                 isize inner_stride) noexcept -> T*
  {
    return ptr + (outer_stride * row + inner_stride * col);
  }
};

template<bool COLMAJOR>
struct RowColAccessImpl;

template<>
struct RowColAccessImpl<true>
{
  template<typename T>
  using Col = Eigen::Map<
    const_if<ptr_is_const<decltype(std::declval<T&&>().data())>::value,
             OwnedColVector<remove_cvref_t<T>>>,
    Eigen::Unaligned,
    Eigen::InnerStride<remove_cvref_t<T>::InnerStrideAtCompileTime>>;
  template<typename T>
  using Row = Eigen::Map<
    const_if<ptr_is_const<decltype(std::declval<T&&>().data())>::value,
             OwnedRowVector<remove_cvref_t<T>>>,
    Eigen::Unaligned,
    Eigen::InnerStride<remove_cvref_t<T>::OuterStrideAtCompileTime>>;

  template<typename T>
  static auto col(T&& mat, isize col_idx) noexcept -> Col<T>
  {
    return {
      mat.data() + col_idx * mat.outerStride(),
      mat.rows(),
      1,
      Eigen::InnerStride<remove_cvref_t<T>::InnerStrideAtCompileTime>{
        mat.innerStride(),
      },
    };
  }
  template<typename T>
  static auto row(T&& mat, isize row_idx) noexcept -> Row<T>
  {
    return {
      mat.data() + row_idx * mat.innerStride(),
      1,
      mat.cols(),
      Eigen::InnerStride<remove_cvref_t<T>::OuterStrideAtCompileTime>{
        mat.outerStride(),
      },
    };
  }
};
template<>
struct RowColAccessImpl<false>
{
  template<typename T>
  using Col = Eigen::Map<
    const_if<ptr_is_const<decltype(std::declval<T&&>().data())>::value,
             OwnedColVector<remove_cvref_t<T>>>,
    Eigen::Unaligned,
    Eigen::InnerStride<remove_cvref_t<T>::OuterStrideAtCompileTime>>;
  template<typename T>
  using Row = Eigen::Map<
    const_if<ptr_is_const<decltype(std::declval<T&&>().data())>::value,
             OwnedRowVector<remove_cvref_t<T>>>,
    Eigen::Unaligned,
    Eigen::InnerStride<remove_cvref_t<T>::InnerStrideAtCompileTime>>;

  template<typename T>
  static auto col(T&& mat, isize col_idx) noexcept -> Col<T>
  {
    return {
      mat.data() + col_idx * mat.innerStride(),
      mat.rows(),
      1,
      Eigen::InnerStride<remove_cvref_t<T>::OuterStrideAtCompileTime>{
        mat.outerStride(),
      },
    };
  }
  template<typename T>
  static auto row(T&& mat, isize row_idx) noexcept -> Row<T>
  {
    return {
      mat.data() + row_idx * mat.outerStride(),
      1,
      mat.cols(),
      Eigen::InnerStride<remove_cvref_t<T>::InnerStrideAtCompileTime>{
        mat.innerStride(),
      },
    };
  }
};
template<typename T>
using StrideOf = Eigen::Stride< //
  T::OuterStrideAtCompileTime,
  T::InnerStrideAtCompileTime>;
} // namespace _detail

namespace util {
template<bool COLMAJOR, typename T>
auto
elem_addr(T* ptr,
          isize row,
          isize col,
          isize outer_stride,
          isize inner_stride) noexcept -> T*
{
  return _detail::ElemAddrImpl<!COLMAJOR>::fn(
    ptr, row, col, outer_stride, inner_stride);
}

template<typename Mat>
auto
matrix_elem_addr(Mat&& mat, isize row, isize col) noexcept
  -> decltype(mat.data())
{
  return util::elem_addr<!bool(proxsuite::remove_cvref_t<Mat>::IsRowMajor)>( //
    mat.data(),
    row,
    col,
    mat.outerStride(),
    mat.innerStride());
}

template<typename T>
auto
col(T&& mat, isize col_idx) noexcept -> typename _detail::RowColAccessImpl<
  !bool(proxsuite::remove_cvref_t<T>::IsRowMajor)>::template Col<T>
{
  return _detail::RowColAccessImpl<!bool(
    proxsuite::remove_cvref_t<T>::IsRowMajor)>::col(mat, col_idx);
}
template<typename T>
auto
row(T&& mat, isize row_idx) noexcept -> typename _detail::RowColAccessImpl<
  !bool(proxsuite::remove_cvref_t<T>::IsRowMajor)>::template Row<T>
{
  return _detail::RowColAccessImpl<!bool(
    proxsuite::remove_cvref_t<T>::IsRowMajor)>::row(mat, row_idx);
}

template<typename Mat>
auto
trans(Mat&& mat) noexcept -> Eigen::Map< //
  _detail::const_if<
    _detail::ptr_is_const<decltype(mat.data())>::value,
    Eigen::Matrix< //
      typename proxsuite::remove_cvref_t<Mat>::Scalar,
      proxsuite::remove_cvref_t<Mat>::ColsAtCompileTime,
      proxsuite::remove_cvref_t<Mat>::RowsAtCompileTime,
      bool(proxsuite::remove_cvref_t<Mat>::IsRowMajor) ? Eigen::ColMajor
                                                       : Eigen::RowMajor>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    mat.data(),
    mat.cols(),
    mat.rows(),
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
diagonal(Mat&& mat) noexcept -> Eigen::Map< //
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    Eigen::Matrix< //
                      typename proxsuite::remove_cvref_t<Mat>::Scalar,
                      Eigen::Dynamic,
                      1,
                      Eigen::ColMajor>>,
  Eigen::Unaligned,
  Eigen::InnerStride<Eigen::Dynamic>>
{
  assert(mat.rows() == mat.cols());
  return { mat.data(),
           mat.rows(),
           1,
           Eigen::InnerStride<Eigen::Dynamic>{ mat.outerStride() + 1 } };
}

template<typename Mat>
auto
submatrix(Mat&& mat,
          isize row_start,
          isize col_start,
          isize nrows,
          isize ncols) noexcept
  -> Eigen::Map<
    _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                      _detail::OwnedMatrix<proxsuite::remove_cvref_t<Mat>>>,
    Eigen::Unaligned,
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    util::elem_addr<!bool(proxsuite::remove_cvref_t<Mat>::IsRowMajor)>(
      mat.data(), row_start, col_start, mat.outerStride(), mat.innerStride()),
    nrows,
    ncols,
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
to_view(Mat&& mat) noexcept -> Eigen::Map<
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    _detail::OwnedAll<proxsuite::remove_cvref_t<Mat>>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    mat.data(),
    mat.rows(),
    mat.cols(),
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
to_view_dyn_rows(Mat&& mat) noexcept -> Eigen::Map<
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    _detail::OwnedRows<proxsuite::remove_cvref_t<Mat>>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    mat.data(),
    mat.rows(),
    mat.cols(),
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
to_view_dyn_cols(Mat&& mat) noexcept -> Eigen::Map<
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    _detail::OwnedCols<proxsuite::remove_cvref_t<Mat>>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    mat.data(),
    mat.rows(),
    mat.cols(),
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
to_view_dyn(Mat&& mat) noexcept -> Eigen::Map<
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    _detail::OwnedMatrix<proxsuite::remove_cvref_t<Mat>>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    mat.data(),
    mat.rows(),
    mat.cols(),
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
subrows(Mat&& mat, isize row_start, isize nrows) noexcept -> Eigen::Map<
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    _detail::OwnedRows<proxsuite::remove_cvref_t<Mat>>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    util::elem_addr<!bool(proxsuite::remove_cvref_t<Mat>::IsRowMajor)>(
      mat.data(), row_start, 0, mat.outerStride(), mat.innerStride()),
    nrows,
    mat.cols(),
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

template<typename Mat>
auto
subcols(Mat&& mat, isize col_start, isize ncols) noexcept -> Eigen::Map<
  _detail::const_if<_detail::ptr_is_const<decltype(mat.data())>::value,
                    _detail::OwnedCols<proxsuite::remove_cvref_t<Mat>>>,
  Eigen::Unaligned,
  _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>>
{
  return {
    util::elem_addr<!bool(proxsuite::remove_cvref_t<Mat>::IsRowMajor)>(
      mat.data(), 0, col_start, mat.outerStride(), mat.innerStride()),
    mat.rows(),
    ncols,
    _detail::StrideOf<proxsuite::remove_cvref_t<Mat>>{
      mat.outerStride(),
      mat.innerStride(),
    },
  };
}

} // namespace util
namespace _detail {
template<typename Dst, typename Lhs, typename Rhs, typename T>
void
noalias_mul_add_impl(Dst dst, Lhs lhs, Rhs rhs, T factor)
{
  assert(dst.rows() == lhs.rows());
  assert(dst.cols() == rhs.cols());
  assert(lhs.cols() == rhs.rows());

  isize nrows = dst.rows();
  isize ncols = dst.cols();
  isize depth = lhs.cols();

  if (nrows == 0 || ncols == 0 || depth == 0) {
    return;
  }

#if !EIGEN_VERSION_AT_LEAST(3, 3, 8)
#define LAZY_PRODUCT(a, b) a.lazyProduct(b)
#else
#define LAZY_PRODUCT(a, b) a.operator*(b)
#endif

#if !EIGEN_VERSION_AT_LEAST(3, 3, 8)
  if ((dst.rows() < 20) && (dst.cols() < 20) && (rhs.rows() < 20)) {
    // gemm
    // workaround for eigen 3.3.7 bug:
    // https://gitlab.com/libeigen/eigen/-/issues/1562
    using Mat =
      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor, 20, 20>;
    using MapMut =
      Eigen::Map<Mat, Eigen::Unaligned, Eigen::OuterStride<Eigen::Dynamic>>;

    auto dst_ =
      MapMut(dst.data(), dst.rows(), dst.cols(), { dst.outerStride() });
    dst_.noalias().operator+=(factor * LAZY_PRODUCT(lhs, rhs));
  } else
#endif
  {
    dst.noalias().operator+=(factor * LAZY_PRODUCT(lhs, rhs));
  }

#undef LAZY_PRODUCT
}
} // namespace _detail
namespace util {
template<typename Dst, typename Lhs, typename Rhs, typename T>
void
noalias_mul_add(Dst&& dst, Lhs const& lhs, Rhs const& rhs, T factor)
{
  _detail::noalias_mul_add_impl(
    util::submatrix(dst, 0, 0, dst.rows(), dst.cols()),
    util::submatrix(lhs, 0, 0, lhs.rows(), lhs.cols()),
    util::submatrix(rhs, 0, 0, rhs.rows(), rhs.cols()),
    factor);
}
} // namespace util
template<typename T>
auto
temp_mat_req(isize rows, isize cols) noexcept
  -> proxsuite::linalg::dynstack::StackReq
{
  return {
    _detail::adjusted_stride<T>(rows) * cols * isize{ sizeof(T) },
    _detail::align<T>(),
  };
}

template<typename T>
auto
temp_vec_req(isize rows) noexcept -> proxsuite::linalg::dynstack::StackReq
{
  return {
    rows * isize{ sizeof(T) },
    _detail::align<T>(),
  };
}
} // namespace dense
} // namespace linalg
} // namespace proxsuite
