## Migration guide

### 0.7.x to the next release: removing `veg`

ProxSuite used to bundle `proxsuite::linalg::veg`, a metaprogramming library
that emulated concepts, tuples, tags, function objects and allocators with
C++11 preprocessor machinery. ProxSuite has required C++17 for some time, so
all of it now has a standard spelling and the library has been removed:
41 headers and around 10 000 lines.

One piece was worth keeping and has been reimplemented from scratch: the bump
allocator the solvers use for their scratch space. It now lives in
`proxsuite/linalg/dynstack.hpp`.

#### Do I need to do anything?

| You are | Impact |
| --- | --- |
| A Python user | **None.** The bindings expose exactly the same names and types. |
| A C++ user of `proxqp::dense::QP` / `proxqp::sparse::QP` only | **None,** unless you read the workspace or model buffers directly. |
| A C++ user who names `proxsuite::linalg::veg::…` | See below; the namespace is gone. |
| A C++ user of the `linalg` backends (`Ldlt`, sparse factorization) | See below; the `*_req` signatures changed. |
| A Python user unpickling `Results` / `QP` saved by an older ProxSuite | **The pickle will not load.** See "Serialized archives" below. |
| Anyone reading JSON or XML archives written by an older ProxSuite | **Not compatible.** Binary archives are unaffected on 64-bit. See below. |

---

#### Headers

Every `proxsuite/linalg/veg/**` header is gone. Two replace them:

```cpp
#include <proxsuite/linalg/dynstack.hpp>   // StackReq, DynStackMut, DynStackArray
#include <proxsuite/linalg/slice.hpp>      // Slice, SliceMut
```

Everything else maps onto the standard library: `<vector>`, `<tuple>`,
`<type_traits>`, `<optional>`.

`PROXSUITE_THROW_PRETTY`, `PROXSUITE_CHECK_ARGUMENT_SIZE` and
`PROXSUITE_PRETTY_FUNCTION` used to live in a `veg` header and were often
picked up transitively. They are ProxSuite's own macros and now live where
they belong:

```cpp
#include <proxsuite/helpers/common.hpp>
```

Headers now use `#pragma once`, so the `PROXSUITE_*_HPP` include-guard macros
are no longer defined. If you tested for one, stop.

---

#### Namespaces and types

`proxsuite::linalg::veg` no longer exists.

| Was | Is now |
| --- | --- |
| `veg::isize`, `veg::usize` | `proxsuite::isize`, `proxsuite::usize` |
| `veg::i32`, `veg::u64`, … | `std::int32_t`, `std::uint64_t`, … |
| `veg::uncvref_t<T>` | `proxsuite::remove_cvref_t<T>` |
| `veg::DoNotDeduce<T>` | `proxsuite::DoNotDeduce<T>` |
| `veg::Slice<T>`, `veg::SliceMut<T>` | `proxsuite::linalg::Slice<T>`, `SliceMut<T>` |
| `veg::dynstack::StackReq` etc. | `proxsuite::linalg::dynstack::StackReq` etc. |
| `veg::Vec<T>` | `std::vector<T>` |
| `veg::Tuple<…>`, `veg::tuplify(…)` | `std::tuple<…>`, `std::make_tuple(…)` |
| `veg::meta::if_t` | `std::conditional_t` |
| `veg::meta::bool_constant` | `std::bool_constant` |
| `veg::meta::constant<T, V>` | `std::integral_constant<T, V>` |
| `veg::meta::unptr_t` | `std::remove_pointer_t` |

---

#### Owning buffers are now `std::vector`

These public members changed type:

- `proxqp::Results<T>::active_constraints` → `std::vector<bool>`
- `proxqp::dense::Workspace<T>::ldl_stack`, `::alphas`
- `proxqp::sparse::Model<T, I>`: the six `kkt_*` arrays
- `proxqp::sparse::Workspace<T, I>`: `storage`, `kkt_nnz_counts`, and
  `ldl.{etree, perm, perm_inv, col_ptrs, nnz_counts, row_indices, values}`
- `linalg::dense::Ldlt<T>`: its internal storage (private, but the ABI changed)

So the accessors change with them:

| `veg::Vec` | `std::vector` |
| --- | --- |
| `v.len()` | `isize(v.size())` — note the signedness |
| `v.ptr()`, `v.ptr_mut()` | `v.data()` |
| `v.push(x)` | `v.push_back(x)` |
| `v.resize_for_overwrite(n)` | `v.resize(usize(n))` |
| `v.reserve_exact(n)` | `v.reserve(usize(n))` |
| `v.pop_mid(i)` | `v.erase(v.begin() + i)` |
| `v.push_mid(x, i)` | `v.insert(v.begin() + i, x)` |
| `v.as_mut()` | `SliceMut<T>{ v.data(), isize(v.size()) }` |
| `v.as_ref()` | `Slice<T>{ v.data(), isize(v.size()) }` |

`resize_for_overwrite` left new elements uninitialized; `resize` value-initializes
them. The extra zero-fill is `O(n)` on buffers whose users are `O(n²)` or worse,
and only happens on setup paths.

One member did **not** become a `std::vector`:
`proxqp::sparse::Workspace<T, I>::active_inequalities` is now a `VecBool`, the
Eigen boolean vector the dense workspace already used. The solver takes a
contiguous `SliceMut<bool>` of it, which the bit-packed `std::vector<bool>`
cannot provide.

---

#### `Tag<T>` parameters became template arguments

Every `*_req` function used to take a `veg::Tag<T>` parameter purely to carry a
type. The type is now an explicit template argument:

```cpp
// before
auto req = proxsuite::linalg::dense::factorize_req(veg::Tag<T>{}, n);
auto r2  = proxsuite::linalg::sparse::factorize_numeric_req(
             veg::Tag<T>{}, veg::Tag<I>{}, n, nnz, ordering);
auto r3  = veg::dynstack::StackReq::with_len(veg::Tag<T>{}, n);

// after
auto req = proxsuite::linalg::dense::factorize_req<T>(n);
auto r2  = proxsuite::linalg::sparse::factorize_numeric_req<T, I>(n, nnz, ordering);
auto r3  = proxsuite::linalg::dynstack::StackReq::with_len<T>(n);
```

This applies to all of them: `transpose_req`, `transpose_symbolic_req`,
`etree_req`, `postorder_req`, `column_counts_req`, `amd_req`,
`symmetric_permute_req`, `symmetric_permute_symbolic_req`,
`factorize_symbolic_req`, `factorize_numeric_req`, `merge_second_col_into_first_req`,
`rank1_update_req`, `factorize_unblocked_req`, `factorize_blocked_req`,
`factorize_recursive_req`, `factorize_req`, `ldlt_insert_rows_and_cols_req`,
`ldlt_delete_rows_and_cols_req`, `temp_vec_req`, `temp_mat_req`, and the
row-modification requests.

`Preconditioner::scale_qp_in_place_req` drops the tag entirely, since its scalar
type already comes from the class:

```cpp
P::scale_qp_in_place_req(n, n_eq, n_in);   // was P::scale_qp_in_place_req(veg::Tag<T>{}, …)
```

---

#### The dynamic stack

`StackReq` is unchanged in meaning: `a & b` is "both allocations live at the
same time", `a | b` is "either one, whichever is larger", and `alloc_req()` is
the number of bytes to allocate.

```cpp
// before
VEG_MAKE_STACK(stack, req);

// after
std::vector<unsigned char> stack_storage(proxsuite::usize(req.alloc_req()));
proxsuite::linalg::dynstack::DynStackMut stack{
  stack_storage.data(), proxsuite::isize(stack_storage.size())
};
```

Borrowing from the stack:

```cpp
// before
auto work = stack.make_new_for_overwrite(veg::Tag<T>{}, n);
auto zero = stack.make_new(veg::Tag<T>{}, n);

// after
auto work = stack.make_new_for_overwrite<T>(n);   // uninitialized, for scalars
auto zero = stack.make_new<T>(n);                 // value-initialized
```

`make_new` and `make_new_for_overwrite` still take an optional alignment as
their last argument. `make_alloc` is gone; nothing used it.

The returned `DynStackArray<T>` is move-only and releases its memory when it
goes out of scope. Allocations must be released in reverse order of
acquisition, which scoping gives you for free — and unlike before, that
invariant is now asserted in debug builds.

`PROX_QP_ALL_OF` / `PROX_QP_ANY_OF` are gone; call `StackReq::and_({…})` and
`StackReq::or_({…})` directly.

---

#### Slices

`Slice<T>` and `SliceMut<T>` are still pointer/length views, but they are
constructed plainly:

```cpp
// before
Slice<T>{ veg::unsafe, veg::from_raw_parts, ptr, len }

// after
proxsuite::linalg::Slice<T>{ ptr, len }
```

The interface is `ptr()`, `ptr_mut()`, `len()`, `is_empty()`, `operator[]`,
`begin()`, `end()`, and `as_const()` on the mutable one. `SliceMut<T>` converts
implicitly to `Slice<T>`. The bounds are asserted in debug builds.

`split_at`, `get_unchecked`, `as_bytes` and `veg::Array` were unused and have
not been carried over.

---

#### Sparse matrix views

`MatRef`, `MatMut`, `SymbolicMatRef` and `SymbolicMatMut` no longer use a CRTP
interface; they derive from a small shared base. Their public interface is
unchanged apart from:

- the `Unsafe` tag is dropped from `col_start_unchecked(j)` and
  `col_end_unchecked(j)` — the name already says it;
- the same for `proxqp::sparse::detail::top_rows_unchecked(mat, n)` and
  `top_rows_mut_unchecked(mat, n)`;
- `MatMut::symbolic_mut()` is removed; nothing used it;
- `col_ptrs_mut()`, `nnz_per_col_mut()`, `row_indices_mut()` and `values_mut()`
  are `const`-qualified, so they can be called on a const view. The view is
  const, the data it points at is not.

Their layout changed, so this is an ABI break.

---

#### Function objects became function templates

`veg`'s "niebloids" — objects with a templated `operator()` — are now ordinary
function templates. **Call syntax is unchanged**, so most code needs no edit:

```cpp
util::zero_extend(i);        // still fine
infty_norm(v);               // still fine
```

What no longer works is treating them as values:

```cpp
// before
auto zx = util::zero_extend;

// after
auto zx = [](auto i) { return util::zero_extend(i); };
```

This affects `sparse::util::{wrapping_plus, checked_non_negative_plus,
wrapping_inc, wrapping_dec, sign_extend, zero_extend}`,
`proxqp::dense::{infty_norm, sqrt, fabs, pow}` and the `min2` / `max2` helpers.

`wrapping_inc` and `wrapping_dec` take a plain reference now, not a `RefMut`:

```cpp
util::wrapping_inc(x);       // was util::wrapping_inc(veg::mut(x))
```

---

#### Macros

**Removed**

| Macro | Replacement |
| --- | --- |
| all `VEG_*` | the standard C++17 construct it emulated |
| `VEG_BIND(auto, (a, b), expr)` | `auto [a, b] = expr;` |
| `LDLT_CONCEPT(x)`, `LDLT_CHECK_CONCEPT(x)` | `concepts::x`, `static_assert(concepts::x)` |
| `PROXSUITE_DEDUCE_RET` | C++14 return type deduction |
| `PROXSUITE_WITH_CPP_14`, `PROXSUITE_WITH_CPP_17` | nothing; C++17 is required |
| `PROXSUITE_MAYBE_UNUSED` | `[[maybe_unused]]` |
| `PROX_QP_ALL_OF`, `PROX_QP_ANY_OF` | `StackReq::and_`, `StackReq::or_` |
| `PROXSUITE_CHECK_SIZE` | an explicit `if (…) return false;` |
| `DENSE_LDLT_FP_PRAGMA`, `LDLT_FN_IMPL3`, `LDLT_LOAD_STORE`, `LDLT_ARITHMETIC_IMPL` | internal; now `#undef`'d after use |

**Renamed.** These were unprefixed macros defined in installed headers with no
`#undef`, so they leaked into every translation unit that included ProxSuite:

| Was | Is now |
| --- | --- |
| `LDLT_TEMP_VEC`, `LDLT_TEMP_VEC_UNINIT` | `PROXSUITE_LDLT_TEMP_VEC`, `PROXSUITE_LDLT_TEMP_VEC_UNINIT` |
| `LDLT_TEMP_MAT`, `LDLT_TEMP_MAT_UNINIT` | `PROXSUITE_LDLT_TEMP_MAT`, `PROXSUITE_LDLT_TEMP_MAT_UNINIT` |
| `LDLT_ID`, `LDLT_CONCAT`, `LDLT_CONCAT_IMPL` | `PROXSUITE_LDLT_ID`, `PROXSUITE_LDLT_CONCAT`, `PROXSUITE_LDLT_CONCAT_IMPL` |
| `__LDLT_TEMP_VEC_IMPL`, `__LDLT_TEMP_MAT_IMPL` | `PROXSUITE_LDLT_TEMP_VEC_IMPL`, `PROXSUITE_LDLT_TEMP_MAT_IMPL` |
| `LDLT_EXPLICIT_TPL_DECL`, `LDLT_EXPLICIT_TPL_DEF` | `PROXSUITE_EXPLICIT_TPL_DECL`, `PROXSUITE_EXPLICIT_TPL_DEF` |

The `__`-prefixed ones, and the old `__proxsuite_fwd_hpp__` include guard, had a
leading double underscore, which is reserved to the implementation.

`PROXSUITE_EXPLICIT_TPL_DECL` / `_DEF` keep the old call syntax — the number of
parameters, then the function name:

```cpp
PROXSUITE_EXPLICIT_TPL_DECL(2, llt_compute<Mat<f32, colmajor>>);
```

They are no longer built on the veg preprocessor library, so they now support 1
to 3 parameters rather than an arbitrary number.

---

#### Serialized archives

Two serialized fields changed representation: `Results<T>::active_constraints`
and `dense::Workspace<T>::alphas`. They used to be written by hand-rolled
`save`/`load` overloads on `veg::Vec`; they now go through cereal's own
`std::vector` support.

**Binary archives are unaffected on 64-bit platforms.** The old code wrote the
element count as an `isize` (`std::ptrdiff_t`), cereal writes it as a
`cereal::size_type` (`std::uint64_t`); both are eight little-endian bytes for a
non-negative count, and the elements follow unchanged. The two encodings are
byte-for-byte identical:

```text
old: 02 00 00 00 00 00 00 00 01 00     // std::vector<bool>{ true, false }
new: 02 00 00 00 00 00 00 00 01 00
```

On a 32-bit platform `std::ptrdiff_t` is four bytes, so binary archives written
there would not be compatible either.

**JSON and XML archives are not compatible.** The old code emitted a named
`len` field followed by elements that cereal auto-named `value0`, `value1`, …,
producing a JSON *object*. Cereal's vector support emits a JSON *array*, with
the size implicit in the array length:

```json
// before
"active_constraints": { "len": 2, "value0": true, "value1": false }

// after
"active_constraints": [ true, false ]
```

These are different node types, so an old archive will not round-trip. There is
no version tag in the format, so nothing detects the mismatch and reports it
cleanly.

This reaches Python: `Results.__getstate__` and `__setstate__` go through
`saveToString` / `loadFromString`, which use the JSON archive. **Pickles of
`Results` — and of `QP`, which holds one — produced by an older ProxSuite will
not load.** Re-generate them, or read them with the older version and convert.

Incidentally, the old `load` was broken independently of any of this: it called
`reserve(len)` and then wrote through `operator[]`, past `size()`. Round-tripping
a non-empty vector was undefined behaviour.

---

#### What did not change

- The solver API: `QP::init`, `QP::solve`, `QP::update`, `Settings`, `Results`,
  `Model` and the free `solve` functions are untouched.
- Numerical behaviour. The full test suite passes unchanged, including with
  assertions enabled.
- Alignment of the dense factorization storage. The removed SIMD-aligned
  allocator never asked for more alignment than `operator new` already
  guarantees, so `std::vector`'s default allocator is equivalent.
- The Python bindings, in both name and behaviour — but see the note on
  pickles above, which are data rather than API.
