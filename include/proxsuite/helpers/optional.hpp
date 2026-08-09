//
// Copyright (c) 2022 INRIA
//
/**
 * @file optional.hpp
 */

#ifndef PROXSUITE_HELPERS_OPTIONAL_HPP
#define PROXSUITE_HELPERS_OPTIONAL_HPP

#include <optional>

namespace proxsuite {
template<class T>
using optional = std::optional<T>;
using nullopt_t = std::nullopt_t;
inline constexpr nullopt_t nullopt = std::nullopt;
} // namespace proxsuite

#endif /* end of include guard PROXSUITE_HELPERS_OPTIONAL_HPP */
