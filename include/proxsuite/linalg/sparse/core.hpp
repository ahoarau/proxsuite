/** \file */
//
// Copyright (c) 2022 INRIA
//
#ifndef PROXSUITE_LINALG_SPARSE_LDLT_CORE_HPP
#define PROXSUITE_LINALG_SPARSE_LDLT_CORE_HPP

#include <proxsuite/fwd.hpp>
#include <proxsuite/linalg/dynstack.hpp>
#include <proxsuite/linalg/slice.hpp>

#include <cassert>
#include <type_traits>
#include <Eigen/SparseCore>

namespace proxsuite {
namespace linalg {
namespace sparse {

using proxsuite::isize;
using proxsuite::usize;

using proxsuite::linalg::Slice;
using proxsuite::linalg::SliceMut;
using proxsuite::linalg::dynstack::DynStackMut;

inline namespace tags {
/// Tag disambiguating constructors that take the raw components of a view.
struct FromRawParts
{};
inline constexpr FromRawParts from_raw_parts{};

/// Tag disambiguating constructors that borrow from an Eigen object.
struct FromEigen
{};
inline constexpr FromEigen from_eigen{};
} // namespace tags

namespace util {

/*!
 * Unsigned type in which wrapping arithmetic on `I` is performed. Types
 * narrower than `int` would otherwise be promoted to `int`, where overflow is
 * undefined behavior.
 */
template<typename I>
using WrappingType =
  typename std::conditional<(sizeof(I) < sizeof(int)),
                            unsigned,
                            typename std::make_unsigned<I>::type>::type;

/// `a + b`, wrapping around on overflow instead of being undefined.
template<typename I>
auto
wrapping_plus(I a, I b) noexcept -> I
{
  using U = WrappingType<I>;
  return I(U(a) + U(b));
}

/// `a + b`, asserting that the sum does not wrap around.
template<typename I>
auto
checked_non_negative_plus(I a, I b) noexcept -> I
{
  I const sum = util::wrapping_plus(a, b);
  assert(sum >= a);
  return sum;
}

/// `++a`, wrapping around on overflow. Returns the new value.
template<typename I>
auto
wrapping_inc(I& a) noexcept -> I
{
  return a = util::wrapping_plus(a, I(1));
}

/// `--a`, wrapping around on underflow. Returns the new value.
template<typename I>
auto
wrapping_dec(I& a) noexcept -> I
{
  return a = util::wrapping_plus(a, I(-1));
}

/// Reinterprets `a` as signed, then widens it to a `usize` bit pattern.
template<typename I>
auto
sign_extend(I a) noexcept -> usize
{
  return usize(isize(typename std::make_signed<I>::type(a)));
}

/// Narrowing cast, asserting that the value survives the conversion.
template<typename To, typename From>
auto
narrow(From from) noexcept -> To
{
  To const to = static_cast<To>(from);
  assert(static_cast<From>(to) == from);
  return to;
}

/// Reinterprets `a` as unsigned, then widens it to `usize`.
template<typename I>
auto
zero_extend(I a) noexcept -> usize
{
  return usize(typename std::make_unsigned<I>::type(a));
}

} // namespace util

/// Read-only view over a dense vector.
template<typename T>
struct DenseVecRef
{
  DenseVecRef() = default;
  DenseVecRef(FromRawParts /*tag*/, T const* data, isize len) noexcept
    : m_ptr(data)
    , m_len(len)
  {
  }
  template<typename V>
  DenseVecRef(FromEigen /*tag*/, V const& v) noexcept
    : m_ptr(v.data())
    , m_len(v.rows())
  {
    static_assert(V::InnerStrideAtCompileTime == 1, ".");
    static_assert(V::ColsAtCompileTime == 1, ".");
  }

  auto as_slice() const noexcept -> Slice<T> { return { m_ptr, m_len }; }
  auto nrows() const noexcept -> isize { return m_len; }
  auto ncols() const noexcept -> isize { return 1; }

  auto to_eigen() const noexcept -> Eigen::Map<Eigen::Matrix<T, -1, 1> const>
  {
    return { m_ptr, m_len };
  }

private:
  T const* m_ptr = nullptr;
  isize m_len = 0;
};

/// Mutable view over a dense vector.
template<typename T>
struct DenseVecMut
{
  DenseVecMut() = default;
  DenseVecMut(FromRawParts /*tag*/, T* data, isize len) noexcept
    : m_ptr(data)
    , m_len(len)
  {
  }
  template<typename V>
  DenseVecMut(FromEigen /*tag*/, V&& v) noexcept
    : m_ptr(v.data())
    , m_len(v.rows())
  {
    using Vec =
      typename std::remove_cv<typename std::remove_reference<V>::type>::type;
    static_assert(Vec::InnerStrideAtCompileTime == 1, ".");
    static_assert(Vec::ColsAtCompileTime == 1, ".");
  }

  auto as_slice() const noexcept -> Slice<T> { return { m_ptr, m_len }; }
  auto as_slice_mut() const noexcept -> SliceMut<T> { return { m_ptr, m_len }; }

  auto as_const() const noexcept -> DenseVecRef<T>
  {
    return { from_raw_parts, m_ptr, m_len };
  }
  auto nrows() const noexcept -> isize { return m_len; }
  auto ncols() const noexcept -> isize { return 1; }

  auto to_eigen() const noexcept -> Eigen::Map<Eigen::Matrix<T, -1, 1>>
  {
    return { m_ptr, m_len };
  }

private:
  T* m_ptr = nullptr;
  isize m_len = 0;
};

/// Read-only view over a sparse vector, in (indices, values) form.
template<typename T, typename I = isize>
struct VecRef
{
  VecRef(FromRawParts /*tag*/,
         isize nrows,
         isize nnz,
         I const* row_indices,
         T const* values) noexcept
    : m_nrows(nrows)
    , m_nnz(nnz)
    , m_row(row_indices)
    , m_val(values)
  {
  }

  auto nrows() const noexcept -> isize { return m_nrows; }
  auto ncols() const noexcept -> isize { return 1; }
  auto nnz() const noexcept -> isize { return m_nnz; }

  auto row_indices() const noexcept -> I const* { return m_row; }
  auto values() const noexcept -> T const* { return m_val; }

private:
  isize m_nrows;
  isize m_nnz;
  I const* m_row;
  T const* m_val;
};

namespace _detail {

/*!
 * The structure of a compressed-column sparse matrix, and the read-only
 * accessors shared by every view of one.
 *
 * `m_col[j]` is the start of column `j` in `m_row`. The matrix is *compressed*
 * when `m_nnz_per_col` is null, meaning column `j` ends where column `j + 1`
 * starts; otherwise `m_nnz_per_col[j]` gives its length.
 *
 * Pointers are stored as `I const*`. The mutable views below hand out non-const
 * pointers to the same memory: that is well defined because they are only ever
 * constructed from storage that is itself mutable.
 */
template<typename I>
struct SymbolicMatBase
{
  isize m_nrows = 0;
  isize m_ncols = 0;
  isize m_nnz = 0;
  I const* m_col = nullptr;
  I const* m_nnz_per_col = nullptr;
  I const* m_row = nullptr;

  auto nrows() const noexcept -> isize { return m_nrows; }
  auto ncols() const noexcept -> isize { return m_ncols; }
  auto nnz() const noexcept -> isize { return m_nnz; }

  auto col_ptrs() const noexcept -> I const* { return m_col; }
  auto nnz_per_col() const noexcept -> I const* { return m_nnz_per_col; }
  auto row_indices() const noexcept -> I const* { return m_row; }

  auto is_compressed() const noexcept -> bool
  {
    return m_nnz_per_col == nullptr;
  }

  auto col_start(usize j) const noexcept -> usize
  {
    assert(j < usize(m_ncols));
    return util::zero_extend(m_col[j]);
  }
  auto col_start_unchecked(usize j) const noexcept -> usize
  {
    return util::zero_extend(m_col[j]);
  }
  auto col_end(usize j) const noexcept -> usize
  {
    assert(j < usize(m_ncols));
    return col_end_unchecked(j);
  }
  auto col_end_unchecked(usize j) const noexcept -> usize
  {
    return util::zero_extend(is_compressed() ? m_col[j + 1]
                                             : I(m_col[j] + m_nnz_per_col[j]));
  }
};

} // namespace _detail

/// Read-only view over the structure of a sparse matrix, without its values.
template<typename I = isize>
struct SymbolicMatRef : _detail::SymbolicMatBase<I>
{
  SymbolicMatRef(FromRawParts /*tag*/,
                 isize nrows,
                 isize ncols,
                 isize nnz,
                 I const* col_ptrs,
                 I const* nnz_per_col,
                 I const* row_indices) noexcept
    : _detail::SymbolicMatBase<I>{ nrows,    ncols,       nnz,
                                   col_ptrs, nnz_per_col, row_indices }
  {
  }
};

/// Mutable view over the structure of a sparse matrix, without its values.
template<typename I = isize>
struct SymbolicMatMut : _detail::SymbolicMatBase<I>
{
  SymbolicMatMut(FromRawParts /*tag*/,
                 isize nrows,
                 isize ncols,
                 isize nnz,
                 I* col_ptrs,
                 I* nnz_per_col,
                 I* row_indices) noexcept
    : _detail::SymbolicMatBase<I>{ nrows,    ncols,       nnz,
                                   col_ptrs, nnz_per_col, row_indices }
  {
  }

  auto col_ptrs_mut() const noexcept -> I*
  {
    return const_cast<I*>(this->m_col);
  }
  auto nnz_per_col_mut() const noexcept -> I*
  {
    return const_cast<I*>(this->m_nnz_per_col);
  }
  auto row_indices_mut() const noexcept -> I*
  {
    return const_cast<I*>(this->m_row);
  }

  auto as_const() const noexcept -> SymbolicMatRef<I>
  {
    return {
      from_raw_parts,   this->nrows(),       this->ncols(),       this->nnz(),
      this->col_ptrs(), this->nnz_per_col(), this->row_indices(),
    };
  }
};

/// Read-only view over a compressed-column sparse matrix.
template<typename T, typename I = isize>
struct MatRef : _detail::SymbolicMatBase<I>
{
  MatRef(FromRawParts /*tag*/,
         isize nrows,
         isize ncols,
         isize nnz,
         I const* col_ptrs,
         I const* nnz_per_col,
         I const* row_indices,
         T const* values) noexcept
    : _detail::SymbolicMatBase<I>{ nrows,    ncols,       nnz,
                                   col_ptrs, nnz_per_col, row_indices }
    , m_val(values)
  {
  }

  template<typename M>
  MatRef(FromEigen /*tag*/, M const& m) noexcept
    : _detail::SymbolicMatBase<I>{ m.rows(),
                                   m.cols(),
                                   m.nonZeros(),
                                   m.outerIndexPtr(),
                                   m.innerNonZeroPtr(),
                                   m.innerIndexPtr() }
    , m_val(m.valuePtr())
  {
    static_assert(!bool(M::IsRowMajor), ".");
  }

  auto values() const noexcept -> T const* { return m_val; }

  auto symbolic() const noexcept -> SymbolicMatRef<I>
  {
    return {
      from_raw_parts,   this->nrows(),       this->ncols(),       this->nnz(),
      this->col_ptrs(), this->nnz_per_col(), this->row_indices(),
    };
  }

  auto to_eigen() const noexcept
    -> Eigen::Map<Eigen::SparseMatrix<T, Eigen::ColMajor, I> const>
  {
    return { this->m_nrows, this->m_ncols, this->m_nnz,        this->m_col,
             this->m_row,   m_val,         this->m_nnz_per_col };
  }

private:
  T const* m_val;
};

/// Mutable view over a compressed-column sparse matrix.
template<typename T, typename I = isize>
struct MatMut : _detail::SymbolicMatBase<I>
{
  MatMut(FromRawParts /*tag*/,
         isize nrows,
         isize ncols,
         isize nnz,
         I* col_ptrs,
         I* nnz_per_col,
         I* row_indices,
         T* values) noexcept
    : _detail::SymbolicMatBase<I>{ nrows,    ncols,       nnz,
                                   col_ptrs, nnz_per_col, row_indices }
    , m_val(values)
  {
  }

  template<typename M>
  MatMut(FromEigen /*tag*/, M&& m) noexcept
    : _detail::SymbolicMatBase<I>{ m.rows(),
                                   m.cols(),
                                   m.nonZeros(),
                                   m.outerIndexPtr(),
                                   m.innerNonZeroPtr(),
                                   m.innerIndexPtr() }
    , m_val(m.valuePtr())
  {
    using Mat =
      typename std::remove_cv<typename std::remove_reference<M>::type>::type;
    static_assert(!bool(Mat::IsRowMajor), ".");
  }

  auto col_ptrs_mut() const noexcept -> I*
  {
    return const_cast<I*>(this->m_col);
  }
  auto nnz_per_col_mut() const noexcept -> I*
  {
    return const_cast<I*>(this->m_nnz_per_col);
  }
  auto row_indices_mut() const noexcept -> I*
  {
    return const_cast<I*>(this->m_row);
  }

  auto values() const noexcept -> T const* { return m_val; }
  auto values_mut() const noexcept -> T* { return const_cast<T*>(m_val); }

  auto as_const() const noexcept -> MatRef<T, I>
  {
    return {
      from_raw_parts,      this->nrows(),    this->ncols(),
      this->nnz(),         this->col_ptrs(), this->nnz_per_col(),
      this->row_indices(), this->values(),
    };
  }
  auto symbolic() const noexcept -> SymbolicMatRef<I>
  {
    return {
      from_raw_parts,   this->nrows(),       this->ncols(),       this->nnz(),
      this->col_ptrs(), this->nnz_per_col(), this->row_indices(),
    };
  }
  auto to_eigen() const noexcept
    -> Eigen::Map<Eigen::SparseMatrix<T, Eigen::ColMajor, I>>
  {
    return { this->m_nrows,     this->m_ncols, this->m_nnz,      col_ptrs_mut(),
             row_indices_mut(), values_mut(),  nnz_per_col_mut() };
  }

  void _set_nnz(isize new_nnz) noexcept { this->m_nnz = new_nnz; }

private:
  T const* m_val;
};

} // namespace sparse
} // namespace linalg
} // namespace proxsuite

#endif /* end of include guard PROXSUITE_LINALG_SPARSE_LDLT_CORE_HPP */
