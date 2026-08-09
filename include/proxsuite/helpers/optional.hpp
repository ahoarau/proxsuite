//
// Copyright (c) 2022 INRIA
//
/**
 * @file optional.hpp
 */

#pragma once

#include <optional>

namespace proxsuite {
template<class T>
using optional = std::optional<T>;
using nullopt_t = std::nullopt_t;
inline constexpr nullopt_t nullopt = std::nullopt;
} // namespace proxsuite
