/** \file */
//
// Copyright (c) 2026 INRIA
//
#pragma once

#include <proxsuite/fwd.hpp>

#include <cassert>
#include <cstddef>

namespace proxsuite {
namespace linalg {

using proxsuite::isize;
using proxsuite::usize;

template<typename T>
struct SliceMut;

/*!
 * Non-owning view over a contiguous, read-only sequence of `T`.
 *
 * A `Slice` is a pointer/length pair. It does not own the pointed-to memory,
 * and it is the caller's responsibility to keep that memory alive for as long
 * as the slice is used.
 */
template<typename T>
struct Slice
{
  Slice() = default;

  Slice(T const* data, isize len) noexcept
    : m_data(data)
    , m_len(len)
  {
    assert(len >= 0);
  }

  /* implicit */ Slice(SliceMut<T> const& other) noexcept;

  auto ptr() const noexcept -> T const* { return m_data; }
  auto len() const noexcept -> isize { return m_len; }
  auto is_empty() const noexcept -> bool { return m_len == 0; }

  auto operator[](isize i) const noexcept -> T const&
  {
    assert(i >= 0 && i < m_len);
    return m_data[i];
  }

  auto begin() const noexcept -> T const* { return m_data; }
  auto end() const noexcept -> T const* { return m_data + m_len; }

private:
  T const* m_data = nullptr;
  isize m_len = 0;
};

/*!
 * Non-owning view over a contiguous, mutable sequence of `T`.
 *
 * Constness of the view is independent from constness of the viewed elements:
 * a `SliceMut const&` still hands out mutable references, exactly like a
 * `T* const` does.
 */
template<typename T>
struct SliceMut
{
  SliceMut() = default;

  SliceMut(T* data, isize len) noexcept
    : m_data(data)
    , m_len(len)
  {
    assert(len >= 0);
  }

  auto ptr() const noexcept -> T const* { return m_data; }
  auto ptr_mut() const noexcept -> T* { return m_data; }
  auto len() const noexcept -> isize { return m_len; }
  auto is_empty() const noexcept -> bool { return m_len == 0; }

  auto as_const() const noexcept -> Slice<T> { return { m_data, m_len }; }

  auto operator[](isize i) const noexcept -> T&
  {
    assert(i >= 0 && i < m_len);
    return m_data[i];
  }

  auto begin() const noexcept -> T* { return m_data; }
  auto end() const noexcept -> T* { return m_data + m_len; }

private:
  T* m_data = nullptr;
  isize m_len = 0;
};

template<typename T>
Slice<T>::Slice(SliceMut<T> const& other) noexcept
  : m_data(other.ptr())
  , m_len(other.len())
{
}

} // namespace linalg
} // namespace proxsuite
