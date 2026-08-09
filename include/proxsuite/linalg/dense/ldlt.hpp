/** \file */
//
// Copyright (c) 2022 INRIA
//
#pragma once

#include "proxsuite/linalg/dense/factorize.hpp"
#include "proxsuite/linalg/dense/update.hpp"
#include "proxsuite/linalg/dense/modify.hpp"
#include "proxsuite/linalg/dense/solve.hpp"
#include <vector>

namespace proxsuite {
namespace linalg {
namespace dense {
/*!
 * Wrapper class that handles an allocated LDLT decomposition,
 * with an applied permutation.
 * When provided with a matrix `A`, this internally stores a lower triangular
 * matrix with unit diagonal `L`, a vector `D`, and a permutation `P` such that
 * `A = P.T L diag(D) L.T P`.
 *
 * Example usage:
 * ```cpp
#include <proxsuite/linalg/dense/ldlt.hpp>
#include <vector>

auto main() -> int {
        constexpr auto DYN = Eigen::Dynamic;
        using Matrix = Eigen::Matrix<double, DYN, DYN>;
        using Vector = Eigen::Matrix<double, DYN, 1>;
        using Ldlt = proxsuite::linalg::dense::Ldlt<double>;
        using proxsuite::linalg::dynstack::StackReq;

        // allocate a matrix `a`
        auto a0 = Matrix{
                        2,
                        2,
        };

        // workspace memory requirements
        auto req =
                        Ldlt::factorize_req(2) |          // initial
factorization of dim 2 Ldlt::insert_block_at_req(2, 1) | // or 1 insertion to
matrix of dim 2 Ldlt::delete_at_req(3, 2) |       // or 2 deletions from matrix
of dim 3 Ldlt::solve_in_place_req(1);      // or solve in place with dim 1

        std::vector<unsigned char> stack_storage(size_t(req.alloc_req()));
        proxsuite::linalg::dynstack::DynStackMut stack{
                stack_storage.data(), proxsuite::isize(stack_storage.size())
        };

        Ldlt ldl;

        // fill up the lower triangular part
        // matrix is
        // 1.0 2.0
        // 2.0 3.0
        a0(0, 0) = 1.0;
        a0(1, 0) = 2.0;
        a0(1, 1) = 3.0;

        ldl.factorize(a0, stack);

        // add one column at the index 1
        // matrix is
        // 1.0 4.0 2.0
        // 4.0 5.0 6.0
        // 2.0 6.0 3.0
        auto c = Matrix{3, 1};
        c(0, 0) = 4.0;
        c(1, 0) = 5.0;
        c(2, 0) = 6.0;
        ldl.insert_block_at(1, c, stack);

        // then delete two rows and columns at indices 0 and 2
        // matrix is
        // 5.0
        proxsuite::isize const indices[] = {0, 2};
        ldl.delete_at(indices, 2, stack);

        auto rhs = Vector{1};
        rhs[0] = 5.0;

        ldl.solve_in_place(rhs, stack);
        assert(rhs[0] == 1.0);
}
 * ```
 */
template<typename T>
struct Ldlt
{
private:
  static constexpr auto DYN = Eigen::Dynamic;
  using ColMat = Eigen::Matrix<T, DYN, DYN, Eigen::ColMajor>;
  using RowMat = Eigen::Matrix<T, DYN, DYN, Eigen::RowMajor>;
  using Vec = Eigen::Matrix<T, DYN, 1>;

  using LView = Eigen::TriangularView<Eigen::Map< //
                                        ColMat const,
                                        Eigen::Unaligned,
                                        Eigen::OuterStride<DYN>>,
                                      Eigen::UnitLower>;
  using LViewMut = Eigen::TriangularView<Eigen::Map< //
                                           ColMat,
                                           Eigen::Unaligned,
                                           Eigen::OuterStride<DYN>>,
                                         Eigen::UnitLower>;

  using LTView = Eigen::TriangularView<Eigen::Map< //
                                         RowMat const,
                                         Eigen::Unaligned,
                                         Eigen::OuterStride<DYN>>,
                                       Eigen::UnitUpper>;
  using LTViewMut = Eigen::TriangularView<Eigen::Map< //
                                            RowMat,
                                            Eigen::Unaligned,
                                            Eigen::OuterStride<DYN>>,
                                          Eigen::UnitUpper>;

  using DView =
    Eigen::Map<Vec const, Eigen::Unaligned, Eigen::InnerStride<DYN>>;
  using DViewMut = Eigen::Map<Vec, Eigen::Unaligned, Eigen::InnerStride<DYN>>;

  using VecMapISize = Eigen::Map<Eigen::Matrix<isize, DYN, 1> const>;
  using Perm = Eigen::PermutationWrapper<VecMapISize>;

  std::vector<T> ld_storage;
  isize stride{};
  std::vector<isize> perm;
  std::vector<isize> perm_inv;

  // sorted on a best effort basis
  std::vector<T> maybe_sorted_diag;

  static auto adjusted_stride(isize n) noexcept -> isize
  {
    return _detail::adjusted_stride<T>(n);
  }

  // soft invariants:
  // - isize(perm.size()) == isize(perm_inv.size()) == dim
  // - dim < stride
  // - isize(ld_storage.size()) >= dim * stride
public:
  /*!
   * Default constructor, initialized with a `0×0` empty matrix.
   */
  Ldlt() = default;

  /*!
   * Reserves enough internal storage for a matrix `A` of size at least
   * `cap×cap`.
   * This operation invalidates the existing decomposition.
   *
   * @param cap new capacity
   */
  void reserve_uninit(isize cap) noexcept
  {
    static_assert(std::is_nothrow_default_constructible<T>::value, ".");

    auto new_stride = adjusted_stride(cap);
    if (cap <= stride && cap * new_stride <= isize(ld_storage.size())) {
      return;
    }

    ld_storage.reserve(usize(cap * new_stride));
    perm.reserve(usize(cap));
    perm_inv.reserve(usize(cap));
    maybe_sorted_diag.reserve(usize(cap));

    ld_storage.resize(usize(cap * new_stride));
    stride = new_stride;
  }

  /*!
   * Reserves enough internal storage for a matrix `A` of size at least
   * `cap×cap`.
   * This operation preserves the existing decomposition.
   *
   * @param cap new capacity
   */
  void reserve(isize cap) noexcept
  {
    auto new_stride = adjusted_stride(cap);
    if (cap <= stride && cap * new_stride <= isize(ld_storage.size())) {
      return;
    }
    auto n = dim();

    ld_storage.reserve(usize(cap * new_stride));
    perm.reserve(usize(cap));
    perm_inv.reserve(usize(cap));
    maybe_sorted_diag.reserve(usize(cap));

    ld_storage.resize(usize(cap * new_stride));

    for (isize i = 0; i < n; ++i) {
      auto col = n - i - 1;
      T* ptr = ld_col_mut().data();
      std::move_backward( //
        ptr + col * stride,
        ptr + col * stride + n,
        ptr + col * new_stride + n);
    }
    stride = new_stride;
  }

  /*!
   * Returns the memory storage requirements for performing a rank `k` update
   * on a matrix with size at most `n×n`, with `k ≤ r`.
   *
   * @param n maximum dimension of the matrix
   * @param r maximum number of simultaneous rank updates
   */
  static auto rank_r_update_req(isize n, isize r) noexcept
    -> proxsuite::linalg::dynstack::StackReq
  {
    auto w_req = proxsuite::linalg::dynstack::StackReq{
      _detail::adjusted_stride<T>(n) * r * isize{ sizeof(T) },
      _detail::align<T>(),
    };
    auto alpha_req = proxsuite::linalg::dynstack::StackReq{
      r * isize{ sizeof(T) },
      alignof(T),
    };
    return w_req & alpha_req;
  }

  /*!
   * Returns the memory storage requirements for deleting at most `r` rows and
   * columns from a matrix with size at most `n×n`.
   *
   * @param n maximum dimension of the matrix
   * @param r maximum number of rows to be deleted
   */
  static auto delete_at_req(isize n, isize r) noexcept
    -> proxsuite::linalg::dynstack::StackReq
  {
    return proxsuite::linalg::dynstack::StackReq{
      r * isize{ sizeof(isize) },
      alignof(isize),
    } & proxsuite::linalg::dense::ldlt_delete_rows_and_cols_req<T>(n, r);
  }

  /*!
   * Given an LDLT decomposition for a matrix `A`, this computes the
   * decomposition for the matrix `A` with `r` columns and rows removed, as
   * indicated by the indices `indices[0], ..., indices[r-1]`.
   *
   * @param indices pointer to the array of indices to be deleted
   * @param r number of the indices to be deleted
   * @param stack workspace memory stack
   */
  void delete_at(isize const* indices,
                 isize r,
                 proxsuite::linalg::dynstack::DynStackMut stack)
  {
    if (r == 0) {
      return;
    }

    assert(std::is_sorted(indices, indices + r));

    isize n = dim();

    auto _indices_actual = stack.make_new_for_overwrite<isize>(r);
    auto* indices_actual = _indices_actual.ptr_mut();

    for (isize k = 0; k < r; ++k) {
      indices_actual[k] = perm_inv[usize(indices[k])];
    }

    proxsuite::linalg::dense::ldlt_delete_rows_and_cols_sort_indices( //
      ld_col_mut(),
      indices_actual,
      r,
      stack);

    // PERF: do this in one pass
    for (isize k = 0; k < r; ++k) {
      auto i_actual = indices_actual[r - 1 - k];
      auto i = indices[r - 1 - k];

      perm.erase(perm.begin() + i_actual);
      perm_inv.erase(perm_inv.begin() + i);
      maybe_sorted_diag.erase(maybe_sorted_diag.begin() + i_actual);

      for (isize j = 0; j < n - 1 - k; ++j) {
        auto& p_j = perm[usize(j)];
        auto& pinv_j = perm_inv[usize(j)];

        if (p_j > i) {
          --p_j;
        }
        if (pinv_j > i_actual) {
          --pinv_j;
        }
      }
    }
  }

  auto choose_insertion_position(isize i, Eigen::Ref<Vec const> a) -> isize
  {
    isize n = dim();
    auto diag_elem = a[i];

    isize pos = 0;
    for (; pos < n; ++pos) {
      if (diag_elem >= maybe_sorted_diag[usize(pos)]) {
        break;
      }
    }
    return pos;
  }

  /*!
   * Returns the memory storage requirements for inserting at most `r` rows and
   * columns from a matrix with size at most `n×n`.
   *
   * @param n maximum dimension of the matrix
   * @param r maximum number of rows to be inserted
   */
  static auto insert_block_at_req(isize n, isize r) noexcept
    -> proxsuite::linalg::dynstack::StackReq
  {
    using proxsuite::linalg::dynstack::StackReq;
    return StackReq{
      isize{ sizeof(T) } * (adjusted_stride(n + r) * r),
      _detail::align<T>(),
    } & proxsuite::linalg::dense::ldlt_insert_rows_and_cols_req<T>(n, r);
  }

  /*!
   * Given an LDLT decomposition for a matrix `A`, this computes the
   * decomposition for the matrix `A` with extra `r` columns and rows from `a`
   * added at the index `i`.
   *
   * @param i index where the block should be inserted
   * @param a matrix of the new columns that are inserted
   * @param stack workspace memory stack
   */
  void insert_block_at(isize i,
                       Eigen::Ref<ColMat const> a,
                       proxsuite::linalg::dynstack::DynStackMut stack)
  {

    isize n = dim();
    isize r = a.cols();

    if (r == 0) {
      return;
    }

    reserve(n + r);

    isize i_actual = choose_insertion_position(i, a.col(0));

    for (isize j = 0; j < n; ++j) {
      auto& p_j = perm[usize(j)];
      auto& pinv_j = perm_inv[usize(j)];

      if (p_j >= i) {
        p_j += r;
      }
      if (pinv_j >= i_actual) {
        pinv_j += r;
      }
    }

    for (isize k = 0; k < r; ++k) {
      perm.insert(perm.begin() + i_actual + k, i + k);
      perm_inv.insert(perm_inv.begin() + i + k, i_actual + k);
      maybe_sorted_diag.insert(maybe_sorted_diag.begin() + (i_actual + k),
                               a(i + k, k));
    }

    PROXSUITE_LDLT_TEMP_MAT_UNINIT(T, permuted_a, n + r, r, stack);

    for (isize k = 0; k < r; ++k) {
      for (isize j = 0; j < n + r; ++j) {
        permuted_a(j, k) = a(perm[usize(j)], k);
      }
    }

    proxsuite::linalg::dense::ldlt_insert_rows_and_cols(
      ld_col_mut(), i_actual, permuted_a, stack);
  }

  /*!
   * Returns the memory storage requirements for a diagonal subsection update
   * with size at most `r`, in a matrix with size at most `n×n`.
   *
   * @param n maximum dimension of the matrix
   * @param r maximum size of diagonal subsection that gets updated
   */
  static auto diagonal_update_req(isize n, isize r) noexcept
    -> proxsuite::linalg::dynstack::StackReq
  {
    using proxsuite::linalg::dynstack::StackReq;
    auto algo_req = StackReq{
      2 * r * isize{ sizeof(isize) },
      alignof(isize),
    };
    auto w_req = StackReq{
      _detail::adjusted_stride<T>(n) * r * isize{ sizeof(T) },
      _detail::align<T>(),
    };
    auto alpha_req = StackReq{
      r * isize{ sizeof(T) },
      alignof(T),
    };
    return algo_req & w_req & alpha_req;
  }

  /*!
   * Given an LDLT decomposition for a matrix `A`, this computes the
   * decomposition for the matrix `A` with the vector `alpha` added to a
   * diagonal subset, as specified by the provided indices.
   *
   * The values pointed at by `indices` are unspecified after a call to this
   * function.
   *
   * @param indices pointer to the array of indices of diagonal elements that
   * are updated
   * @param r number of the indices to be updated
   * @param stack workspace memory stack
   */
  void diagonal_update_clobber_indices( //
    isize* indices,
    isize r,
    Eigen::Ref<Vec const> alpha,
    proxsuite::linalg::dynstack::DynStackMut stack)
  {

    if (r == 0) {
      return;
    }

    auto _positions = stack.make_new_for_overwrite<isize>(r);
    auto _sorted_indices = stack.make_new_for_overwrite<isize>(r);
    auto* positions = _positions.ptr_mut();
    auto* sorted_indices = _sorted_indices.ptr_mut();

    for (isize k = 0; k < r; ++k) {
      indices[k] = perm_inv[usize(indices[k])];
      positions[k] = k;
    }

    std::sort(
      positions, positions + r, [indices](isize i, isize j) noexcept -> bool {
        return indices[i] < indices[j];
      });

    for (isize k = 0; k < r; ++k) {
      sorted_indices[k] = indices[positions[k]];
    }

    auto first = sorted_indices[0];
    auto n = dim() - first;

    PROXSUITE_LDLT_TEMP_MAT(T, _w, n, r, stack);
    PROXSUITE_LDLT_TEMP_VEC_UNINIT(T, _alpha, r, stack);

    for (isize k = 0; k < r; ++k) {
      _alpha(k) = alpha(positions[k]);
      _w(sorted_indices[k] - first, k) = 1;
    }

    proxsuite::linalg::dense::_detail::rank_r_update_clobber_w_impl(
      util::submatrix(ld_col_mut(), first, first, n, n),
      _w.data(),
      _w.outerStride(),
      _alpha.data(),
      _detail::IndicesR{
        first,
        0,
        r,
        sorted_indices,
      });
  }

  /*!
   * Given an LDLT decomposition for a matrix `A`, this computes the
   * decomposition for the rank-updated matrix `A + w×diag(alpha)×w.T`
   *
   * @param w rank update matrix
   * @param alpha rank update diagonal vector
   * @param stack workspace memory stack
   */
  void rank_r_update( //
    Eigen::Ref<ColMat const> w,
    Eigen::Ref<Vec const> alpha,
    proxsuite::linalg::dynstack::DynStackMut stack)
  {

    auto n = dim();
    auto r = w.cols();
    if (r == 0) {
      return;
    }

    assert(w.rows() == n);

    PROXSUITE_LDLT_TEMP_MAT_UNINIT(T, _w, n, r, stack);
    PROXSUITE_LDLT_TEMP_VEC_UNINIT(T, _alpha, r, stack);

    for (isize k = 0; k < r; ++k) {
      auto alpha_tmp = alpha(k);
      _alpha(k) = alpha_tmp;
      for (isize i = 0; i < n; ++i) {
        auto w_tmp = w(perm[usize(i)], k);
        _w(i, k) = w_tmp;
        maybe_sorted_diag[usize(i)] += alpha_tmp * (w_tmp * w_tmp);
      }
    }

    proxsuite::linalg::dense::rank_r_update_clobber_inputs(
      ld_col_mut(), _w, _alpha);
  }

  /*!
   * Returns the dimension of the stored decomposition.
   */
  auto dim() const noexcept -> isize { return isize(perm.size()); }

  auto ld_col() const noexcept -> Eigen::Map< //
    ColMat const,
    Eigen::Unaligned,
    Eigen::OuterStride<DYN>>
  {
    return { ld_storage.data(), dim(), dim(), stride };
  }
  auto ld_col_mut() noexcept -> Eigen::Map< //
    ColMat,
    Eigen::Unaligned,
    Eigen::OuterStride<DYN>>
  {
    return { ld_storage.data(), dim(), dim(), stride };
  }
  auto ld_row() const noexcept -> Eigen::Map< //
    RowMat const,
    Eigen::Unaligned,
    Eigen::OuterStride<DYN>>
  {
    return {
      ld_storage.data(),
      dim(),
      dim(),
      Eigen::OuterStride<DYN>{ stride },
    };
  }
  auto ld_row_mut() noexcept -> Eigen::Map< //
    RowMat,
    Eigen::Unaligned,
    Eigen::OuterStride<DYN>>
  {
    return {
      ld_storage.data(),
      dim(),
      dim(),
      Eigen::OuterStride<DYN>{ stride },
    };
  }

  auto l() const noexcept -> LView
  {
    return ld_col().template triangularView<Eigen::UnitLower>();
  }
  auto l_mut() noexcept -> LViewMut
  {
    return ld_col_mut().template triangularView<Eigen::UnitLower>();
  }
  auto lt() const noexcept -> LTView
  {
    return ld_row().template triangularView<Eigen::UnitUpper>();
  }
  auto lt_mut() noexcept -> LTViewMut
  {
    return ld_row_mut().template triangularView<Eigen::UnitUpper>();
  }

  auto d() const noexcept -> DView
  {
    return {
      ld_storage.data(),
      dim(),
      1,
      Eigen::InnerStride<DYN>{ stride + 1 },
    };
  }
  auto d_mut() noexcept -> DViewMut
  {
    return {
      ld_storage.data(),
      dim(),
      1,
      Eigen::InnerStride<DYN>{ stride + 1 },
    };
  }
  auto p() const -> Perm { return { VecMapISize(perm.data(), dim()) }; }
  auto pt() const -> Perm { return { VecMapISize(perm_inv.data(), dim()) }; }

  /*!
   * Returns the memory storage requirements for a factorization of a matrix
   * of size at most `n×n`
   *
   * @param n maximum dimension of the matrix
   */
  static auto factorize_req(isize n) -> proxsuite::linalg::dynstack::StackReq
  {
    return proxsuite::linalg::dynstack::StackReq{
      n * adjusted_stride(n) * isize{ sizeof(T) },
      _detail::align<T>(),
    } | proxsuite::linalg::dense::factorize_req<T>(n);
  }

  /*!
   * Computes the decomposition of a given matrix `A`.
   * The matrix is interpreted as a symmetric matrix and only
   * the lower triangular part of `A` is accessed.
   *
   * @param mat matrix whose decomposition should be computed
   * @param stack workspace memory stack
   */
  void factorize(Eigen::Ref<ColMat const> mat /* NOLINT */,
                 proxsuite::linalg::dynstack::DynStackMut stack)
  {
    assert(mat.rows() == mat.cols());
    isize n = mat.rows();
    reserve_uninit(n);

    perm.resize(usize(n));
    perm_inv.resize(usize(n));
    maybe_sorted_diag.resize(usize(n));

    proxsuite::linalg::dense::_detail::compute_permutation( //
      perm.data(),
      perm_inv.data(),
      util::diagonal(mat));

    {
      PROXSUITE_LDLT_TEMP_MAT_UNINIT(T, work, n, n, stack);
      ld_col_mut() = mat;
      proxsuite::linalg::dense::_detail::apply_permutation_tri_lower(
        ld_col_mut(), work, perm.data());
    }
    for (isize i = 0; i < n; ++i) {
      maybe_sorted_diag[usize(i)] = ld_col()(i, i);
    }
    proxsuite::linalg::dense::factorize(ld_col_mut(), stack);
  }

  /*!
   * Returns the memory storage requirements for solving a linear system
   * with a decomposition of dimension at most `n`
   *
   * @param n maximum dimension of the matrix
   */
  static auto solve_in_place_req(isize n)
    -> proxsuite::linalg::dynstack::StackReq
  {
    return {
      n * isize{ sizeof(T) },
      _detail::align<T>(),
    };
  }

  /*!
   * Solves the system `A×x = rhs`, and stores the result in `rhs`.
   *
   * @param rhs right hand side of the linear system
   * @param stack workspace memory stack
   */
  void solve_in_place(Eigen::Ref<Vec> rhs,
                      proxsuite::linalg::dynstack::DynStackMut stack) const
  {
    isize n = rhs.rows();
    PROXSUITE_LDLT_TEMP_VEC_UNINIT(T, work, n, stack);

    for (isize i = 0; i < n; ++i) {
      work[i] = rhs[perm[usize(i)]];
    }

    proxsuite::linalg::dense::solve(ld_col(), work);

    for (isize i = 0; i < n; ++i) {
      rhs[i] = work[perm_inv[usize(i)]];
    }
  }

  void dual_solve_in_place(Eigen::Ref<Vec> rhs,
                           isize n,
                           proxsuite::linalg::dynstack::DynStackMut stack) const
  {
    isize m = rhs.rows();
    PROXSUITE_LDLT_TEMP_VEC_UNINIT(T, work, m, stack);

    for (isize i = 0; i < m; ++i) {
      work[i] = rhs[perm[usize(n + i)] -
                    n]; // n are the first n entries that are not considered
    }
    proxsuite::linalg::dense::solve(ld_col().bottomRightCorner(m, m), work);
    for (isize i = 0; i < m; ++i) {
      rhs[i] = work[perm_inv[usize(n + i)] - n];
    }
  }

  auto dbg_reconstructed_matrix_internal() const -> ColMat
  {
    isize n = dim();
    auto tmp = ColMat(n, n);
    tmp = l();
    tmp = tmp * d().asDiagonal();
    auto A = ColMat(tmp * lt());
    return A;
  }

  auto dbg_reconstructed_matrix() const -> ColMat
  {
    isize n = dim();
    auto tmp = ColMat(n, n);
    tmp = l();
    tmp = tmp * d().asDiagonal();
    auto A = ColMat(tmp * lt());

    for (isize i = 0; i < n; i++) {
      tmp.row(i) = A.row(perm_inv[usize(i)]);
    }
    for (isize i = 0; i < n; i++) {
      A.col(i) = tmp.col(perm_inv[usize(i)]);
    }
    return A;
  }
};
} // namespace dense
} // namespace linalg
} // namespace proxsuite
