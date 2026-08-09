/** \file */
//
// Copyright (c) 2026 INRIA
//
#pragma once

#include <proxsuite/fwd.hpp>
#include <proxsuite/linalg/slice.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace proxsuite {
namespace linalg {
namespace dynstack {

using proxsuite::isize;
using proxsuite::usize;

namespace detail {

constexpr auto
max2(isize a, isize b) noexcept -> isize
{
  return (a > b) ? a : b;
}

/// Rounds `n` up to the next multiple of `to`, which must be a power of two.
constexpr auto
round_up_pow2(isize n, isize to) noexcept -> isize
{
  return isize((usize(n) + usize(to) - 1) & ~(usize(to) - 1));
}

} // namespace detail

/*!
 * The memory requirements — a byte count together with an alignment — of one or
 * more temporary allocations.
 *
 * Requirements compose with two operators:
 *  - `a & b` describes a buffer large enough to hold `a` *and* `b` at the same
 *    time, accounting for the padding needed to align `b` after `a`;
 *  - `a | b` describes a buffer large enough to hold `a` *or* `b`, whichever is
 *    larger.
 *
 * A routine advertises what it needs through a `*_req` function, and the caller
 * allocates `req.alloc_req()` bytes once and reuses them.
 */
struct StackReq
{
  isize size_bytes = 0;
  isize align = 1;

  /// Requirement for `len` contiguous objects of type `T`.
  template<typename T>
  static constexpr auto with_len(isize len) noexcept -> StackReq
  {
    return { isize{ sizeof(T) } * len, isize{ alignof(T) } };
  }

  /*!
   * The number of bytes to hand to `DynStackMut` so that this requirement is
   * satisfiable whatever the alignment of the buffer itself, i.e. including the
   * worst-case padding at the front.
   */
  constexpr auto alloc_req() const noexcept -> isize
  {
    return size_bytes + align - 1;
  }

  /// Requirement for holding every element of `reqs` at the same time.
  static constexpr auto and_(std::initializer_list<StackReq> reqs) noexcept
    -> StackReq
  {
    StackReq total{ 0, 1 };
    for (StackReq req : reqs) {
      total = total & req;
    }
    return total;
  }

  /// Requirement for holding whichever element of `reqs` is the largest.
  static constexpr auto or_(std::initializer_list<StackReq> reqs) noexcept
    -> StackReq
  {
    StackReq total{ 0, 1 };
    for (StackReq req : reqs) {
      total = total | req;
    }
    return total;
  }

  friend constexpr auto operator==(StackReq a, StackReq b) noexcept -> bool
  {
    return a.size_bytes == b.size_bytes && a.align == b.align;
  }
  friend constexpr auto operator!=(StackReq a, StackReq b) noexcept -> bool
  {
    return !(a == b);
  }

  /// Both allocations are live at the same time: sizes add up, with padding.
  friend constexpr auto operator&(StackReq a, StackReq b) noexcept -> StackReq
  {
    return {
      detail::round_up_pow2(detail::round_up_pow2(a.size_bytes, b.align) +
                              b.size_bytes,
                            detail::max2(a.align, b.align)),
      detail::max2(a.align, b.align),
    };
  }

  /// Only one of the allocations is live at a time: sizes are maxed.
  friend constexpr auto operator|(StackReq a, StackReq b) noexcept -> StackReq
  {
    return {
      detail::max2(
        detail::round_up_pow2(a.size_bytes, detail::max2(a.align, b.align)),
        detail::round_up_pow2(b.size_bytes, detail::max2(a.align, b.align))),
      detail::max2(a.align, b.align),
    };
  }
};

template<typename T>
struct DynStackArray;

/*!
 * A bump allocator over a byte buffer owned by the caller.
 *
 * Allocations are carved out of the front of the remaining space and are
 * returned as `DynStackArray` objects. They are released, in reverse order of
 * acquisition, when those objects go out of scope — so a `DynStackMut` behaves
 * like a stack of scoped temporaries, and never touches the heap.
 *
 * ```cpp
 * std::vector<unsigned char> storage(usize(req.alloc_req()));
 * proxsuite::linalg::dynstack::DynStackMut stack{ storage.data(),
 *                                                 isize(storage.size()) };
 * {
 *   auto work = stack.make_new_for_overwrite<double>(n);
 *   double* p = work.ptr_mut();
 *   // ... use p[0..n) ...
 * } // `work` is popped here
 * ```
 */
struct DynStackMut
{
  DynStackMut() = default;

  DynStackMut(unsigned char* data, isize nbytes) noexcept
    : m_data(data)
    , m_remaining(nbytes)
  {
    assert(nbytes >= 0);
  }

  [[nodiscard]] auto remaining_bytes() const noexcept -> isize
  {
    return m_remaining;
  }
  [[nodiscard]] auto ptr() const noexcept -> void const* { return m_data; }
  [[nodiscard]] auto ptr_mut() const noexcept -> void* { return m_data; }

  /*!
   * Allocates `len` value-initialized objects (zeroed, for scalar types).
   *
   * @param len number of objects
   * @param align alignment of the allocation, defaults to `alignof(T)`
   */
  template<typename T>
  [[nodiscard]] auto make_new(isize len, isize align = alignof(T))
    -> DynStackArray<T>
  {
    return DynStackArray<T>{ *this, len, align, /* value_init = */ true };
  }

  /*!
   * Allocates `len` default-initialized objects. For scalar types this leaves
   * the memory uninitialized; the caller must write every element before
   * reading it.
   *
   * @param len number of objects
   * @param align alignment of the allocation, defaults to `alignof(T)`
   */
  template<typename T>
  [[nodiscard]] auto make_new_for_overwrite(isize len, isize align = alignof(T))
    -> DynStackArray<T>
  {
    return DynStackArray<T>{ *this, len, align, /* value_init = */ false };
  }

private:
  /*!
   * Aligns the head of the free space up to `align`, then reserves `nbytes`
   * from it. Returns `nullptr` and leaves the stack untouched when the
   * remaining space is too small.
   */
  auto bump(isize align, isize nbytes) noexcept -> unsigned char*
  {
    assert(align > 0 && (usize(align) & (usize(align) - 1)) == 0);

    if (m_remaining < nbytes) {
      return nullptr;
    }

    auto const addr = reinterpret_cast<std::uintptr_t>(m_data);
    auto const mask = std::uintptr_t(align) - 1;
    auto const padding = isize(((addr + mask) & ~mask) - addr);

    if (m_remaining - nbytes < padding) {
      return nullptr;
    }

    unsigned char* const out = m_data + padding;
    m_data = out + nbytes;
    m_remaining -= padding + nbytes;
    return out;
  }

  unsigned char* m_data = nullptr;
  isize m_remaining = 0;

  template<typename T>
  friend struct DynStackArray;
};

/*!
 * An array of `T` borrowed from a `DynStackMut`, released when it goes out of
 * scope.
 *
 * Instances are obtained from `DynStackMut::make_new` and
 * `DynStackMut::make_new_for_overwrite`. They are move-only, and must be
 * destroyed in reverse order of creation — which is what scoping gives you for
 * free.
 */
template<typename T>
struct DynStackArray
{
  DynStackArray() = default;

  ~DynStackArray() { release(); }

  DynStackArray(DynStackArray const&) = delete;
  auto operator=(DynStackArray const&) -> DynStackArray& = delete;

  DynStackArray(DynStackArray&& other) noexcept { steal_from(other); }

  auto operator=(DynStackArray&& other) noexcept -> DynStackArray&
  {
    if (this != &other) {
      release();
      steal_from(other);
    }
    return *this;
  }

  [[nodiscard]] auto ptr() const noexcept -> T const* { return m_data; }
  [[nodiscard]] auto ptr_mut() noexcept -> T* { return m_data; }
  [[nodiscard]] auto len() const noexcept -> isize { return m_len; }

  [[nodiscard]] auto as_ref() const noexcept -> Slice<T>
  {
    return { m_data, m_len };
  }
  [[nodiscard]] auto as_mut() noexcept -> SliceMut<T>
  {
    return { m_data, m_len };
  }

private:
  friend struct DynStackMut;

  DynStackArray(DynStackMut& parent, isize len, isize align, bool value_init)
    : m_parent(&parent)
    , m_old_data(parent.m_data)
    , m_old_remaining(parent.m_remaining)
  {
    assert(len >= 0);

    unsigned char* const raw = parent.bump(align, len * isize(sizeof(T)));
    assert(raw != nullptr &&
           "dynamic stack: out of memory. The buffer handed to DynStackMut is "
           "smaller than the StackReq of the routine being called.");
    if (raw == nullptr) {
      return;
    }

    T* const first = reinterpret_cast<T*>(raw);
    if (value_init) {
      for (isize i = 0; i < len; ++i) {
        ::new (static_cast<void*>(first + i)) T();
      }
    } else {
      for (isize i = 0; i < len; ++i) {
        ::new (static_cast<void*>(first + i)) T;
      }
    }
    m_data = first;
    m_len = len;
  }

  void steal_from(DynStackArray& other) noexcept
  {
    m_parent = other.m_parent;
    m_old_data = other.m_old_data;
    m_old_remaining = other.m_old_remaining;
    m_data = other.m_data;
    m_len = other.m_len;

    other.m_parent = nullptr;
    other.m_data = nullptr;
    other.m_len = 0;
  }

  void release() noexcept
  {
    if (m_parent == nullptr) {
      return;
    }

    if (m_data != nullptr) {
      if constexpr (!std::is_trivially_destructible<T>::value) {
        for (isize i = m_len; i > 0; --i) {
          m_data[i - 1].~T();
        }
      }

      // Allocations must be released in reverse order: nothing may have been
      // taken from the parent stack after this one without being released
      // first.
      assert(reinterpret_cast<unsigned char*>(m_data) +
               m_len * isize(sizeof(T)) ==
             m_parent->m_data);
    }

    m_parent->m_data = m_old_data;
    m_parent->m_remaining = m_old_remaining;
    m_parent = nullptr;
    m_data = nullptr;
    m_len = 0;
  }

  DynStackMut* m_parent = nullptr;
  unsigned char* m_old_data = nullptr;
  isize m_old_remaining = 0;
  T* m_data = nullptr;
  isize m_len = 0;
};

} // namespace dynstack
} // namespace linalg
} // namespace proxsuite
