//
// Copyright (c) 2022 INRIA
//
/**
 * @file views.hpp
 */
#pragma once

#include <proxsuite/fwd.hpp>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <Eigen/Core>

namespace proxsuite {
namespace proxqp {

using proxsuite::isize;
using proxsuite::usize;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
namespace detail {

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

template<typename T, bool = std::is_floating_point<T>::value>
struct SetZeroImpl
{
  static void fn(T* dest, usize n)
  {
    for (usize i = 0; i < n; ++i) {
      *dest = 0;
    }
  }
};

template<typename T>
struct SetZeroImpl<T, true>
{
  static void fn(T* dest, usize n)
  {
    // TODO: assert bit representation is zero
    std::memset(dest, 0, n * sizeof(T));
  }
};

template<typename T>
void
set_zero(T* dest, usize n)
{
  SetZeroImpl<T>::fn(dest, n);
}

constexpr auto
round_up(isize n, isize k) noexcept -> isize
{
  return (n + k - 1) / k * k;
}
constexpr auto
uround_up(usize n, usize k) noexcept -> usize
{
  return (n + k - 1) / k * k;
}

inline auto
bytes_to_prev_aligned(void* ptr, usize align) noexcept -> isize
{
  using UPtr = std::uintptr_t;

  UPtr mask = align - 1;
  UPtr iptr = UPtr(ptr);
  UPtr aligned_ptr = iptr & ~mask;
  return isize(aligned_ptr - iptr);
}
inline auto
bytes_to_next_aligned(void* ptr, usize align) noexcept -> isize
{
  using UPtr = std::uintptr_t;

  UPtr mask = align - 1;
  UPtr iptr = UPtr(ptr);
  UPtr aligned_ptr = (iptr + mask) & ~mask;
  return isize(aligned_ptr - iptr);
}

inline auto
next_aligned(void* ptr, usize align) noexcept -> void*
{
  using BytePtr = unsigned char*;
  using VoidPtr = void*;
  return VoidPtr(BytePtr(ptr) + detail::bytes_to_next_aligned(ptr, align));
}
inline auto
prev_aligned(void* ptr, usize align) noexcept -> void*
{
  using BytePtr = unsigned char*;
  using VoidPtr = void*;
  return VoidPtr(BytePtr(ptr) + detail::bytes_to_prev_aligned(ptr, align));
}

} // namespace detail

enum struct Layout : unsigned char
{
  colmajor = 0,
  rowmajor = 1,
};

constexpr Layout colmajor = Layout::colmajor;
constexpr Layout rowmajor = Layout::rowmajor;

constexpr auto
flip_layout(Layout l) noexcept -> Layout
{
  return Layout(1 - u32(l));
}
constexpr auto
to_eigen_layout(Layout l) -> int
{
  return l == colmajor ? Eigen::ColMajor : Eigen::RowMajor;
}
constexpr auto
from_eigen_layout(int l) -> Layout
{
  return (unsigned(l) & Eigen::RowMajorBit) == Eigen::RowMajor ? rowmajor
                                                               : colmajor;
}

static_assert(to_eigen_layout(from_eigen_layout(Eigen::ColMajor)) ==
                Eigen::ColMajor,
              ".");
static_assert(to_eigen_layout(from_eigen_layout(Eigen::RowMajor)) ==
                Eigen::RowMajor,
              ".");

namespace detail {
template<Layout L>
struct ElementAccess;

template<>
struct ElementAccess<Layout::colmajor>
{
  template<typename T>
  PROXSUITE_INLINE static constexpr auto offset(T* ptr,
                                                isize row,
                                                isize col,
                                                isize outer_stride) noexcept
    -> T*
  {
    return ptr + (usize(row) + usize(col) * usize(outer_stride));
  }

  using NextRowStride = Eigen::Stride<0, 0>;
  using NextColStride = Eigen::InnerStride<Eigen::Dynamic>;
  PROXSUITE_INLINE static auto next_row_stride(isize outer_stride) noexcept
    -> NextRowStride
  {
    (void)outer_stride;
    return NextRowStride{};
  }
  PROXSUITE_INLINE static auto next_col_stride(isize outer_stride) noexcept
    -> NextColStride
  {
    return NextColStride /* NOLINT(modernize-return-braced-init-list) */ (
      outer_stride);
  }

  template<typename T>
  PROXSUITE_INLINE static void transpose_if_rowmajor(T* ptr,
                                                     isize dim,
                                                     isize outer_stride)
  {
    (void)ptr, (void)dim, (void)outer_stride;
  }
};

template<>
struct ElementAccess<Layout::rowmajor>
{
  template<typename T>
  PROXSUITE_INLINE static constexpr auto offset(T* ptr,
                                                isize row,
                                                isize col,
                                                isize outer_stride) noexcept
    -> T*
  {
    return ptr + (usize(col) + usize(row) * usize(outer_stride));
  }

  using NextColStride = Eigen::Stride<0, 0>;
  using NextRowStride = Eigen::InnerStride<Eigen::Dynamic>;
  PROXSUITE_INLINE static auto next_col_stride(isize outer_stride) noexcept
    -> NextColStride
  {
    (void)outer_stride;
    return NextColStride{};
  }
  PROXSUITE_INLINE static auto next_row_stride(isize outer_stride) noexcept
    -> NextRowStride
  {
    return NextRowStride /* NOLINT(modernize-return-braced-init-list) */ (
      outer_stride);
  }

  template<typename T>
  PROXSUITE_INLINE static void transpose_if_rowmajor(T* ptr,
                                                     isize dim,
                                                     isize outer_stride)
  {
    Eigen::Map<                          //
      Eigen::Matrix<                     //
        T,                               //
        Eigen::Dynamic,                  //
        Eigen::Dynamic                   //
        >,                               //
      Eigen::Unaligned,                  //
      Eigen::OuterStride<Eigen::Dynamic> //
      >{
      ptr,
      dim,
      dim,
      Eigen::OuterStride<Eigen::Dynamic>(outer_stride),
    }
      .transposeInPlace();
  }
};
} // namespace detail

namespace detail {
template<typename T>
struct unlref
{
  using Type = T;
};
template<typename T>
struct unlref<T&>
{
  using Type = T;
};

template<typename T>
auto
is_eigen_matrix_base_impl(Eigen::MatrixBase<T> const volatile*)
  -> std::true_type;
auto
is_eigen_matrix_base_impl(void const volatile*) -> std::false_type;

template<typename T>
auto
is_eigen_owning_matrix_base_impl(Eigen::PlainObjectBase<T> const volatile*)
  -> std::true_type;
auto
is_eigen_owning_matrix_base_impl(void const volatile*) -> std::false_type;

template<typename... Ts>
using Void = void;

template<typename Mat, typename T>
using DataExpr = decltype(static_cast<T*>(std::declval<Mat&>().data()));

template<typename Dummy,
         typename Fallback,
         template<typename...> class F,
         typename... Ts>
struct DetectedImpl : std::false_type
{
  using Type = Fallback;
};

template<typename Fallback, template<typename...> class F, typename... Ts>
struct DetectedImpl<Void<F<Ts...>>, Fallback, F, Ts...> : std::true_type
{
  using Type = F<Ts...>;
};

template<typename Fallback, template<typename...> class F, typename... Ts>
using Detected = typename DetectedImpl<void, Fallback, F, Ts...>::Type;

template<typename T>
using CompTimeColsImpl =
  std::integral_constant<isize, isize(T::ColsAtCompileTime)>;
template<typename T>
using CompTimeRowsImpl =
  std::integral_constant<isize, isize(T::RowsAtCompileTime)>;
template<typename T>
using CompTimeInnerStrideImpl =
  std::integral_constant<isize, isize(T::InnerStrideAtCompileTime)>;
template<typename T>
using LayoutImpl =
  std::integral_constant<Layout, (bool(T::IsRowMajor) ? rowmajor : colmajor)>;

template<typename T, Layout L>
using EigenMatMap = Eigen::Map<      //
  Eigen::Matrix<                     //
    T,                               //
    Eigen::Dynamic,                  //
    Eigen::Dynamic,                  //
    (L == colmajor)                  //
      ? Eigen::ColMajor              //
      : Eigen::RowMajor              //
    > const,                         //
  Eigen::Unaligned,                  //
  Eigen::OuterStride<Eigen::Dynamic> //
  >;
template<typename T, Layout L>
using EigenMatMapMut = Eigen::Map<   //
  Eigen::Matrix<                     //
    T,                               //
    Eigen::Dynamic,                  //
    Eigen::Dynamic,                  //
    (L == colmajor)                  //
      ? Eigen::ColMajor              //
      : Eigen::RowMajor              //
    >,                               //
  Eigen::Unaligned,                  //
  Eigen::OuterStride<Eigen::Dynamic> //
  >;

template<typename T, typename Stride>
using EigenVecMap = Eigen::Map< //
  Eigen::Matrix<                //
    T,                          //
    Eigen::Dynamic,             //
    1                           //
    > const,                    //
  Eigen::Unaligned,             //
  Stride                        //
  >;
template<typename T, typename Stride>
using EigenVecMapMut = Eigen::Map< //
  Eigen::Matrix<                   //
    T,                             //
    Eigen::Dynamic,                //
    1                              //
    >,                             //
  Eigen::Unaligned,                //
  Stride                           //
  >;

template<typename T, Layout L>
using ColToVec = EigenVecMap<T, typename ElementAccess<L>::NextRowStride>;
template<typename T, Layout L>
using RowToVec = EigenVecMap<T, typename ElementAccess<L>::NextColStride>;
template<typename T, Layout L>
using ColToVecMut = EigenVecMapMut<T, typename ElementAccess<L>::NextRowStride>;
template<typename T, Layout L>
using RowToVecMut = EigenVecMapMut<T, typename ElementAccess<L>::NextColStride>;

template<typename T>
using VecMap = EigenVecMap<T, Eigen::Stride<0, 0>>;
template<typename T>
using VecMapMut = EigenVecMapMut<T, Eigen::Stride<0, 0>>;

} // namespace detail

template<typename T>
using unref = typename detail::unlref<T&>::Type;

namespace eigen {
template<typename T>
using CompTimeCols = detail::
  Detected<std::integral_constant<isize, 0>, detail::CompTimeColsImpl, T>;
template<typename T>
using CompTimeRows = detail::
  Detected<std::integral_constant<isize, 0>, detail::CompTimeRowsImpl, T>;
template<typename T>
using CompTimeInnerStride = detail::Detected<std::integral_constant<isize, 0>,
                                             detail::CompTimeInnerStrideImpl,
                                             T>;
template<typename T>
using GetLayout = detail::Detected<
  std::integral_constant<Layout, Layout(static_cast<unsigned char>(-1))>,
  detail::LayoutImpl,
  T>;
} // namespace eigen

/*!
 * Compile-time predicates on the Eigen types the views below can be built
 * from. They are plain `constexpr bool` variable templates, meant to be used
 * in `std::enable_if_t`.
 */
namespace concepts {
template<typename T>
inline constexpr bool rvalue_ref = std::is_rvalue_reference<T>::value;
template<typename T>
inline constexpr bool lvalue_ref = std::is_lvalue_reference<T>::value;
template<template<typename...> class F, typename... Ts>
inline constexpr bool detected =
  detail::DetectedImpl<void, void, F, Ts...>::value;

namespace aux {
/// `Mat::data()` exists and yields a `T*`.
template<typename Mat, typename T>
inline constexpr bool has_data_expr =
  concepts::detected<detail::DataExpr, Mat, T>;

template<typename Mat>
inline constexpr bool matrix_base = decltype(detail::is_eigen_matrix_base_impl(
  static_cast<Mat*>(nullptr)))::value;

template<typename Mat>
inline constexpr bool is_plain_object_base =
  decltype(detail::is_eigen_owning_matrix_base_impl(
    static_cast<Mat*>(nullptr)))::value;

/// An owning Eigen object bound to an rvalue: viewing it would dangle.
template<typename Mat>
inline constexpr bool tmp_matrix =
  aux::is_plain_object_base<unref<Mat>> && !concepts::lvalue_ref<Mat>;
} // namespace aux

template<typename Mat, typename T>
inline constexpr bool eigen_view =
  aux::matrix_base<unref<Mat>> && aux::has_data_expr<Mat, T const>;

template<typename Mat, typename T>
inline constexpr bool eigen_view_mut =
  aux::matrix_base<unref<Mat>> && aux::has_data_expr<Mat, T> &&
  !aux::tmp_matrix<Mat>;

template<typename Mat, typename T>
inline constexpr bool eigen_strided_vector_view =
  eigen_view<Mat, T> && (eigen::CompTimeCols<unref<Mat>>::value == 1);

template<typename Mat, typename T>
inline constexpr bool eigen_strided_vector_view_mut =
  eigen_view_mut<Mat, T> && (eigen::CompTimeCols<unref<Mat>>::value == 1);

template<typename Mat, typename T>
inline constexpr bool eigen_vector_view =
  eigen_strided_vector_view<Mat, T> &&
  (eigen::CompTimeInnerStride<unref<Mat>>::value == 1);

template<typename Mat, typename T>
inline constexpr bool eigen_vector_view_mut =
  eigen_strided_vector_view_mut<Mat, T> &&
  (eigen::CompTimeInnerStride<unref<Mat>>::value == 1);
} // namespace concepts

/// Tags disambiguating the view constructors below.
inline namespace tags {
struct FromPtrSize
{};
inline constexpr FromPtrSize from_ptr_size{};
struct FromPtrSizeStride
{};
inline constexpr FromPtrSizeStride from_ptr_size_stride{};
struct FromPtrRowsColsStride
{};
inline constexpr FromPtrRowsColsStride from_ptr_rows_cols_stride{};
struct FromEigen
{};
inline constexpr FromEigen from_eigen{};
} // namespace tags

template<typename T>
struct VectorView
{
  T const* data;
  isize dim;

  PROXSUITE_INLINE
  VectorView(FromPtrSize /*tag*/, T const* _data, isize _dim) noexcept
    : data(_data)
    , dim(_dim)
  {
  }

  template<typename Vec,
           typename = std::enable_if_t<concepts::eigen_vector_view<Vec, T>>>
  PROXSUITE_INLINE VectorView(FromEigen /*tag*/, Vec const& vec) noexcept
    : data(vec.data())
    , dim(vec.rows())
  {
  }

  PROXSUITE_INLINE auto ptr(isize index) const noexcept -> T const*
  {
    return data + index;
  }
  PROXSUITE_INLINE auto operator()(isize index) const noexcept -> T const&
  {
    return *ptr(index);
  }
  PROXSUITE_INLINE auto segment(isize i, isize size) const noexcept
    -> VectorView
  {
    return {
      from_ptr_size,
      data + i,
      size,
    };
  }
  PROXSUITE_INLINE auto to_eigen() const -> detail::VecMap<T>
  {
    return detail::VecMap<T>(data, Eigen::Index(dim));
  }
};

template<typename T>
struct VectorViewMut
{
  T* data;
  isize dim;

  PROXSUITE_INLINE
  VectorViewMut(FromPtrSize /*tag*/, T* _data, isize _dim) noexcept
    : data(_data)
    , dim(_dim)
  {
  }

  template<typename Vec,
           typename = std::enable_if_t<concepts::eigen_vector_view_mut<Vec, T>>>
  PROXSUITE_INLINE VectorViewMut(FromEigen /*tag*/, Vec&& vec) noexcept
    : data(vec.data())
    , dim(vec.rows())
  {
  }

  PROXSUITE_INLINE auto as_const() const noexcept -> VectorView<T>
  {
    return {
      from_ptr_size,
      data,
      dim,
    };
  }
  PROXSUITE_INLINE auto ptr(isize index) const noexcept -> T*
  {
    return data + index;
  }
  PROXSUITE_INLINE auto operator()(isize index) const noexcept -> T&
  {
    return *ptr(index);
  }
  PROXSUITE_INLINE auto segment(isize i, isize size) const noexcept
    -> VectorViewMut
  {
    return {
      from_ptr_size,
      data + i,
      size,
    };
  }
  PROXSUITE_INLINE auto to_eigen() const -> detail::VecMapMut<T>
  {
    return detail::VecMapMut<T>(data, Eigen::Index(dim));
  }
};

template<typename T>
struct StridedVectorView
{
  T const* data;
  isize dim;
  isize stride;

  PROXSUITE_INLINE
  StridedVectorView(FromPtrSizeStride /*tag*/,
                    T const* _data,
                    isize _dim,
                    isize _stride) noexcept
    : data(_data)
    , dim(_dim)
    , stride(_stride)
  {
  }

  template<
    typename Vec,
    typename = std::enable_if_t<concepts::eigen_strided_vector_view<Vec, T>>>
  PROXSUITE_INLINE StridedVectorView(FromEigen /*tag*/, Vec const& vec) noexcept
    : data(vec.data())
    , dim(vec.rows())
    , stride(vec.innerStride())
  {
  }

  PROXSUITE_INLINE auto ptr(isize index) const noexcept -> T const*
  {
    return data + stride * index;
  }
  PROXSUITE_INLINE auto operator()(isize index) const noexcept -> T const&
  {
    return *ptr(index);
  }
  PROXSUITE_INLINE auto segment(isize i, isize size) const noexcept
    -> StridedVectorView
  {
    return {
      from_ptr_size_stride,
      data + stride * i,
      size,
      stride,
    };
  }
  PROXSUITE_INLINE auto to_eigen() const
    -> detail::EigenVecMap<T, Eigen::InnerStride<Eigen::Dynamic>>
  {
    return detail::EigenVecMap<T, Eigen::InnerStride<Eigen::Dynamic>>(
      data,
      Eigen::Index(dim),
      Eigen::Index(1),
      Eigen::InnerStride<Eigen::Dynamic>(Eigen::Index(stride)));
  }
};

template<typename T>
struct StridedVectorViewMut
{
  T* data;
  isize dim;
  isize stride;

  PROXSUITE_INLINE
  StridedVectorViewMut(FromPtrSizeStride /*tag*/,
                       T* _data,
                       isize _dim,
                       isize _stride) noexcept
    : data(_data)
    , dim(_dim)
    , stride(_stride)
  {
  }

  template<typename Vec,
           typename =
             std::enable_if_t<concepts::eigen_strided_vector_view_mut<Vec, T>>>
  PROXSUITE_INLINE StridedVectorViewMut(FromEigen /*tag*/, Vec&& vec) noexcept
    : data(vec.data())
    , dim(vec.rows())
    , stride(vec.innerStride())
  {
  }

  PROXSUITE_INLINE auto as_const() const noexcept -> StridedVectorView<T>
  {
    return {
      from_ptr_size_stride,
      data,
      dim,
      stride,
    };
  }
  PROXSUITE_INLINE auto ptr(isize index) const noexcept -> T*
  {
    return data + stride * index;
  }
  PROXSUITE_INLINE auto operator()(isize index) const noexcept -> T&
  {
    return *ptr(index);
  }
  PROXSUITE_INLINE auto segment(isize i, isize size) const noexcept
    -> StridedVectorViewMut
  {
    return {
      from_ptr_size_stride,
      data + stride * i,
      size,
      stride,
    };
  }
  PROXSUITE_INLINE auto to_eigen() const
    -> detail::EigenVecMapMut<T, Eigen::InnerStride<Eigen::Dynamic>>
  {
    return detail::EigenVecMapMut<T, Eigen::InnerStride<Eigen::Dynamic>>(
      data,
      Eigen::Index(dim),
      Eigen::Index(1),
      Eigen::InnerStride<Eigen::Dynamic>(Eigen::Index(stride)));
  }
};

template<typename T, Layout L>
struct MatrixView
{
  T const* data;
  isize rows;
  isize cols;
  isize outer_stride;

  PROXSUITE_INLINE MatrixView(FromPtrRowsColsStride /*tag*/,
                              T const* _data,
                              isize _rows,
                              isize _cols,
                              isize _outer_stride) noexcept
    : data(_data)
    , rows(_rows)
    , cols(_cols)
    , outer_stride(_outer_stride)
  {
  }

  template<
    typename Mat,
    typename = std::enable_if_t<concepts::eigen_view<Mat, T> &&
                                eigen::GetLayout<unref<Mat>>::value == L>>
  PROXSUITE_INLINE MatrixView(FromEigen /*tag*/, Mat const& mat) noexcept
    : data(mat.data())
    , rows(mat.rows())
    , cols(mat.cols())
    , outer_stride(mat.outerStride())
  {
  }

  PROXSUITE_INLINE auto ptr(isize row, isize col) const noexcept -> T const*
  {
    return detail::ElementAccess<L>::offset(data, row, col, outer_stride);
  }
  PROXSUITE_INLINE auto operator()(isize row, isize col) const noexcept
    -> T const&
  {
    return *ptr(row, col);
  }
  PROXSUITE_INLINE auto block(isize row,
                              isize col,
                              isize nrows,
                              isize ncols) const noexcept -> MatrixView
  {
    return {
      from_ptr_rows_cols_stride,
      detail::ElementAccess<L>::offset(data, row, col, outer_stride),
      nrows,
      ncols,
      outer_stride,
    };
  }

  PROXSUITE_INLINE auto col(isize c) const noexcept
    -> std::conditional_t<(L == colmajor), VectorView<T>, StridedVectorView<T>>
  {
    if constexpr (L == colmajor) {
      return { from_ptr_size, data + c * outer_stride, rows };
    } else {
      return { from_ptr_size_stride, data + c, rows, outer_stride };
    }
  }
  PROXSUITE_INLINE auto row(isize r) const noexcept
    -> std::conditional_t<(L == rowmajor), VectorView<T>, StridedVectorView<T>>
  {
    return trans().col(r);
  }
  PROXSUITE_INLINE auto trans() const noexcept
    -> MatrixView<T, proxqp::flip_layout(L)>
  {
    return {
      from_ptr_rows_cols_stride, data, cols, rows, outer_stride,
    };
  }
  PROXSUITE_INLINE auto to_eigen() const noexcept -> detail::EigenMatMap<T, L>
  {
    return detail::EigenMatMap<T, L>(
      data,
      Eigen::Index(rows),
      Eigen::Index(cols),
      Eigen::OuterStride<Eigen::Dynamic>(Eigen::Index(outer_stride)));
  }
};

template<typename T, Layout L>
struct MatrixViewMut
{
  T* data;
  isize rows;
  isize cols;
  isize outer_stride;

  PROXSUITE_INLINE MatrixViewMut(FromPtrRowsColsStride /*tag*/,
                                 T* _data,
                                 isize _rows,
                                 isize _cols,
                                 isize _outer_stride) noexcept
    : data(_data)
    , rows(_rows)
    , cols(_cols)
    , outer_stride(_outer_stride)
  {
  }

  template<
    typename Mat,
    typename = std::enable_if_t<concepts::eigen_view<Mat, T> &&
                                eigen::GetLayout<unref<Mat>>::value == L>>
  PROXSUITE_INLINE MatrixViewMut(FromEigen /*tag*/, Mat&& mat) noexcept
    : data(mat.data())
    , rows(mat.rows())
    , cols(mat.cols())
    , outer_stride(mat.outerStride())
  {
  }

  PROXSUITE_INLINE auto ptr(isize row, isize col) const noexcept -> T*
  {
    return detail::ElementAccess<L>::offset(data, row, col, outer_stride);
  }
  PROXSUITE_INLINE auto operator()(isize row, isize col) const noexcept -> T&
  {
    return *ptr(row, col);
  }
  PROXSUITE_INLINE auto block(isize row,
                              isize col,
                              isize nrows,
                              isize ncols) const noexcept -> MatrixViewMut
  {
    return {
      from_ptr_rows_cols_stride,
      detail::ElementAccess<L>::offset(data, row, col, outer_stride),
      nrows,
      ncols,
      outer_stride,
    };
  }

  PROXSUITE_INLINE auto col(isize c) const noexcept -> std::
    conditional_t<(L == colmajor), VectorViewMut<T>, StridedVectorViewMut<T>>
  {
    if constexpr (L == colmajor) {
      return { from_ptr_size, data + c * outer_stride, rows };
    } else {
      return { from_ptr_size_stride, data + c, rows, outer_stride };
    }
  }
  PROXSUITE_INLINE auto row(isize r) const noexcept -> std::
    conditional_t<(L == rowmajor), VectorViewMut<T>, StridedVectorViewMut<T>>
  {
    return trans().col(r);
  }
  PROXSUITE_INLINE auto trans() const noexcept
    -> MatrixViewMut<T, proxqp::flip_layout(L)>
  {
    return {
      from_ptr_rows_cols_stride, data, cols, rows, outer_stride,
    };
  }
  PROXSUITE_INLINE auto to_eigen() const noexcept
    -> detail::EigenMatMapMut<T, L>
  {
    return detail::EigenMatMapMut<T, L>(
      data,
      Eigen::Index(rows),
      Eigen::Index(cols),
      Eigen::OuterStride<Eigen::Dynamic>(Eigen::Index(outer_stride)));
  }
  PROXSUITE_INLINE auto as_const() const noexcept -> MatrixView<T, L>
  {
    return {
      from_ptr_rows_cols_stride, data, rows, cols, outer_stride,
    };
  }
};

template<typename T>
struct LdltView
{
private:
  MatrixView<T, colmajor> ld;

public:
  explicit LdltView(MatrixView<T, colmajor> ld) noexcept
    : ld(ld)
  {
    assert(ld.rows == ld.cols);
  }

  PROXSUITE_INLINE auto l() const noexcept -> MatrixView<T, colmajor>
  {
    return ld;
  }
  PROXSUITE_INLINE auto d() const noexcept -> StridedVectorView<T>
  {
    return { from_ptr_size_stride, ld.data, ld.rows, ld.outer_stride + 1 };
  }

  PROXSUITE_INLINE auto head(isize k) const -> LdltView
  {
    return LdltView{ ld.block(0, 0, k, k) };
  }
  PROXSUITE_INLINE auto tail(isize k) const -> LdltView
  {
    isize n = ld.rows;
    return LdltView{ ld.block(n - k, n - k, k, k) };
  }
};
template<typename T>
struct LdltViewMut
{
private:
  MatrixViewMut<T, colmajor> ld;

public:
  explicit LdltViewMut(MatrixViewMut<T, colmajor> ld) noexcept
    : ld(ld)
  {
    assert(ld.rows == ld.cols);
  }

  PROXSUITE_INLINE auto l() const noexcept -> MatrixView<T, colmajor>
  {
    return ld.as_const();
  }
  PROXSUITE_INLINE auto l_mut() const noexcept -> MatrixViewMut<T, colmajor>
  {
    return ld;
  }
  PROXSUITE_INLINE auto d() const noexcept -> StridedVectorView<T>
  {
    return { from_ptr_size_stride, ld.data, ld.rows, ld.outer_stride + 1 };
  }
  PROXSUITE_INLINE auto d_mut() const noexcept -> StridedVectorViewMut<T>
  {
    return { from_ptr_size_stride, ld.data, ld.rows, ld.outer_stride + 1 };
  }

  PROXSUITE_INLINE auto as_const() const noexcept -> LdltView<T>
  {
    return LdltView<T>{ ld.as_const() };
  }

  PROXSUITE_INLINE auto head(isize k) const -> LdltViewMut
  {
    return LdltViewMut{ ld.block(0, 0, k, k) };
  }
  PROXSUITE_INLINE auto tail(isize k) const -> LdltViewMut
  {
    isize n = ld.rows;
    return LdltViewMut{ ld.block(n - k, n - k, k, k) };
  }
};

namespace detail {
template<typename T>
void
noalias_mul_add(MatrixViewMut<T, colmajor> dst,
                MatrixView<T, colmajor> lhs,
                MatrixView<T, colmajor> rhs,
                T factor)
{

  if ((dst.cols == 0) || (dst.rows == 0) || (lhs.cols == 0)) {
    return;
  }

#if !EIGEN_VERSION_AT_LEAST(3, 3, 8)
#define LAZY_PRODUCT(a, b) a.lazyProduct(b)
#else
#define LAZY_PRODUCT(a, b) a.operator*(b)
#endif

  if (dst.cols == 1 && dst.rows == 1) {
    // dot
    auto rhs_col = rhs.col(0);
    auto lhs_row = lhs.row(0);
    auto lhs_as_col = lhs.col(0);
    lhs_as_col.dim = lhs_row.dim;
    if (lhs_row.stride == 1) {
      dst(0, 0) += factor * lhs_as_col.to_eigen().dot(rhs_col.to_eigen());
    } else {
      dst(0, 0) += factor * lhs_row.to_eigen().dot(rhs_col.to_eigen());
    }
  } else if (dst.cols == 1) {
    // gemv
    auto rhs_col = rhs.col(0);
    auto dst_col = dst.col(0);
    dst_col.to_eigen().noalias().operator+=(
      factor * LAZY_PRODUCT(lhs.to_eigen(), rhs_col.to_eigen()));
  }

#if !EIGEN_VERSION_AT_LEAST(3, 3, 8)
  else if ((dst.rows < 20) && (dst.cols < 20) && (rhs.rows < 20)) {
    // gemm
    // workaround for eigen 3.3.7 bug:
    // https://gitlab.com/libeigen/eigen/-/issues/1562
    using Stride = Eigen::OuterStride<Eigen::Dynamic>;
    using Mat =
      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor, 20, 20>;
    using MapMut = Eigen::Map<Mat, Eigen::Unaligned, Stride>;
    using Map = Eigen::Map<Mat const, Eigen::Unaligned, Stride>;

    MapMut(dst.data, dst.rows, dst.cols, Stride(dst.outer_stride))
      .noalias()
      .operator+=(
        factor *
        LAZY_PRODUCT(
          Map(lhs.data, lhs.rows, lhs.cols, Stride(lhs.outer_stride)),
          Map(rhs.data, rhs.rows, rhs.cols, Stride(rhs.outer_stride))));
  }
#endif

  else {
    // gemm
    dst.to_eigen().noalias().operator+=(
      factor * LAZY_PRODUCT(lhs.to_eigen(), rhs.to_eigen()));
  }

#undef LAZY_PRODUCT
}

template<typename T>
void
noalias_mul_add_vec(VectorViewMut<T> dst,
                    MatrixView<T, colmajor> lhs,
                    VectorView<T> rhs,
                    T factor)
{
  detail::noalias_mul_add<T>(
    {
      from_ptr_rows_cols_stride,
      dst.data,
      dst.dim,
      1,
      0,
    },
    lhs,
    {
      from_ptr_rows_cols_stride,
      rhs.data,
      rhs.dim,
      1,
      0,
    },
    factor);
}

template<typename T>
auto
dot(StridedVectorView<T> lhs, VectorView<T> rhs) -> T
{
  auto out = T(0);
  detail::noalias_mul_add<T>(
    {
      from_ptr_rows_cols_stride,
      std::addressof(out),
      1,
      1,
      0,
    },
    {
      from_ptr_rows_cols_stride,
      lhs.data,
      1,
      lhs.dim,
      lhs.stride,
    },
    {
      from_ptr_rows_cols_stride,
      rhs.data,
      rhs.dim,
      1,
      0,
    },
    1);
  return out;
}
template<typename T>
void
assign_cwise_prod(VectorViewMut<T> out,
                  StridedVectorView<T> lhs,
                  StridedVectorView<T> rhs)
{
  out.to_eigen() = lhs.to_eigen().cwiseProduct(rhs.to_eigen());
}
template<typename T>
void
assign_scalar_prod(VectorViewMut<T> out, T factor, VectorView<T> in)
{
  out.to_eigen() = in.to_eigen().operator*(factor);
}

template<typename T>
void
trans_tr_unit_up_solve_in_place_on_right(MatrixView<T, colmajor> tr,
                                         MatrixViewMut<T, colmajor> rhs)
{
  if (rhs.cols == 1) {
    tr.to_eigen()
      .transpose()
      .template triangularView<Eigen::UnitUpper>()
      .template solveInPlace<Eigen::OnTheRight>(rhs.col(0).to_eigen());
  } else {
    tr.to_eigen()
      .transpose()
      .template triangularView<Eigen::UnitUpper>()
      .template solveInPlace<Eigen::OnTheRight>(rhs.to_eigen());
  }
}

template<typename T>
void
apply_diag_inv_on_right(MatrixViewMut<T, colmajor> out,
                        StridedVectorView<T> d,
                        MatrixView<T, colmajor> in)
{
  if (out.cols == 1) {
    out.col(0).to_eigen() =
      in.col(0).to_eigen().operator*(d.to_eigen().asDiagonal().inverse());
  } else {
    out.to_eigen() =
      in.to_eigen().operator*(d.to_eigen().asDiagonal().inverse());
  }
}
template<typename T>
void
apply_diag_on_right(MatrixViewMut<T, colmajor> out,
                    StridedVectorView<T> d,
                    MatrixView<T, colmajor> in)
{
  if (out.cols == 1) {
    out.col(0).to_eigen() =
      in.col(0).to_eigen().operator*(d.to_eigen().asDiagonal());
  } else {
    out.to_eigen() = in.to_eigen().operator*(d.to_eigen().asDiagonal());
  }
}

template<typename T>
void
noalias_mul_sub_tr_lo(MatrixViewMut<T, colmajor> out,
                      MatrixView<T, colmajor> lhs,
                      MatrixView<T, rowmajor> rhs)
{
  if (lhs.cols == 1) {
    out.to_eigen().template triangularView<Eigen::Lower>().operator-=(
      lhs.col(0).to_eigen().operator*(
        Eigen::Map<Eigen::Matrix<T, 1, Eigen::Dynamic> const>(
          rhs.data, 1, rhs.cols)));
  } else {
    out.to_eigen().template triangularView<Eigen::Lower>().operator-=(
      lhs.to_eigen().operator*(rhs.to_eigen()));
  }
}

} // namespace detail
} // namespace proxqp
} // namespace proxsuite

namespace proxsuite {
namespace proxqp {

namespace dense {

struct EigenAllowAlloc
{
  bool alloc_was_allowed;
  EigenAllowAlloc(EigenAllowAlloc&&) = delete;
  EigenAllowAlloc(EigenAllowAlloc const&) = delete;
  auto operator=(EigenAllowAlloc&&) -> EigenAllowAlloc& = delete;
  auto operator=(EigenAllowAlloc const&) -> EigenAllowAlloc& = delete;

#if defined(EIGEN_RUNTIME_NO_MALLOC)
  EigenAllowAlloc() noexcept
    : alloc_was_allowed(Eigen::internal::is_malloc_allowed())
  {
    Eigen::internal::set_is_malloc_allowed(true);
  }
  ~EigenAllowAlloc() noexcept
  {
    Eigen::internal::set_is_malloc_allowed(alloc_was_allowed);
  }
#else
  EigenAllowAlloc() = default;
#endif
};

template<typename T>
struct QpView
{

  static constexpr Layout layout = rowmajor;

  MatrixView<T, layout> H;
  VectorView<T> g;

  MatrixView<T, layout> A;
  VectorView<T> b;
  MatrixView<T, layout> C;
  VectorView<T> d;
};

template<typename Scalar>
struct QpViewBox
{
  static constexpr Layout layout = rowmajor;

  MatrixView<Scalar, layout> H;
  VectorView<Scalar> g;

  MatrixView<Scalar, layout> A;
  VectorView<Scalar> b;
  MatrixView<Scalar, layout> C;
  VectorView<Scalar> u;
  VectorView<Scalar> l;
  VectorView<Scalar> I;
  VectorView<Scalar> u_box;
  VectorView<Scalar> l_box;
};

template<typename T>
struct QpViewMut
{
  static constexpr Layout layout = rowmajor;

  MatrixViewMut<T, layout> H;
  VectorViewMut<T> g;

  MatrixViewMut<T, layout> A;
  VectorViewMut<T> b;
  MatrixViewMut<T, layout> C;
  VectorViewMut<T> d;

  PROXSUITE_INLINE constexpr auto as_const() const noexcept -> QpView<T>
  {
    return {
      H.as_const(), g.as_const(), A.as_const(),
      b.as_const(), C.as_const(), d.as_const(),
    };
  }
};

template<typename Scalar>
struct QpViewBoxMut
{
  static constexpr Layout layout = rowmajor;

  MatrixViewMut<Scalar, layout> H;
  VectorViewMut<Scalar> g;

  MatrixViewMut<Scalar, layout> A;
  VectorViewMut<Scalar> b;
  MatrixViewMut<Scalar, layout> C;
  VectorViewMut<Scalar> u;
  VectorViewMut<Scalar> l;
  VectorViewMut<Scalar> I;
  VectorViewMut<Scalar> l_box;
  VectorViewMut<Scalar> u_box;

  PROXSUITE_INLINE constexpr auto as_const() const noexcept -> QpViewBox<Scalar>
  {
    return { H.as_const(),     g.as_const(),    A.as_const(), b.as_const(),
             C.as_const(),     u.as_const(),    l.as_const(), I.as_const(),
             u_box.as_const(), l_box.as_const() };
  }
};

/*!
 * `std::pow`, reached through a using-declaration so that a scalar type with
 * its own `pow` overload is picked up by ADL.
 */
template<typename T>
auto
pow(T x, T y) -> T
{
  using std::pow;
  return pow(x, y);
}

/// `std::sqrt`, see `pow` above.
template<typename T>
auto
sqrt(T x) -> T
{
  using std::sqrt;
  return sqrt(x);
}

/// `std::fabs`, see `pow` above.
template<typename T>
auto
fabs(T x) -> T
{
  using std::fabs;
  return fabs(x);
}

/// Largest absolute coefficient of `mat`, or zero if it is empty.
template<typename D>
auto
infty_norm(Eigen::MatrixBase<D> const& mat) -> typename D::Scalar
{
  if (mat.rows() == 0 || mat.cols() == 0) {
    return typename D::Scalar(0);
  }
  return mat.template lpNorm<Eigen::Infinity>();
}
} // namespace dense
} // namespace proxqp
} // namespace proxsuite
