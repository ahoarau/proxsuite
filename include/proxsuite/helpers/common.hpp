//
// Copyright (c) 2022 INRIA
//
/**
 * @file common.hpp
 */

#ifndef PROXSUITE_HELPERS_COMMON_HPP
#define PROXSUITE_HELPERS_COMMON_HPP

#include "proxsuite/config.hpp"
#include <limits>
#include <sstream>
#include <stdexcept>

#if defined(_MSC_VER)
#define PROXSUITE_PRETTY_FUNCTION __FUNCSIG__
#else
#define PROXSUITE_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

/// Throws `exception`, built from `message` plus the source location, when
/// `condition` holds.
#define PROXSUITE_THROW_PRETTY(condition, exception, message)                  \
  if (condition) {                                                             \
    std::ostringstream ss;                                                     \
    ss << "From file: " << __FILE__ << "\n";                                   \
    ss << "in function: " << PROXSUITE_PRETTY_FUNCTION << "\n";                \
    ss << "at line: " << __LINE__ << "\n";                                     \
    ss << message << "\n";                                                     \
    throw exception(ss.str());                                                 \
  }

/// Throws `std::invalid_argument` when `size` differs from `expected_size`.
#define PROXSUITE_CHECK_ARGUMENT_SIZE(size, expected_size, message)            \
  if (size != expected_size) {                                                 \
    std::ostringstream oss;                                                    \
    oss << "wrong argument size: expected " << expected_size << ", got "       \
        << size << "\n";                                                       \
    oss << "hint: " << message << std::endl;                                   \
    PROXSUITE_THROW_PRETTY(true, std::invalid_argument, oss.str());            \
  }

namespace proxsuite {
namespace helpers {

template<typename Scalar>
struct infinite_bound
{
  static Scalar value()
  {
    using namespace std;
    return sqrt(std::numeric_limits<Scalar>::max());
  }
};

/// @brief \brief Returns the part of the expression which is lower than value
template<typename T, typename Scalar>
auto
at_most(T const& expr, const Scalar value)
{
  return (expr.array() < value).select(expr, T::Constant(expr.rows(), value));
}

/// @brief \brief Returns the part of the expression which is greater than value
template<typename T, typename Scalar>
auto
at_least(T const& expr, const Scalar value)
{
  return (expr.array() > value).select(expr, T::Constant(expr.rows(), value));
}

/// @brief \brief Returns the positive part of an expression
template<typename T>
auto
positive_part(T const& expr)
{
  return (expr.array() > 0).select(expr, T::Zero(expr.rows()));
}

/// @brief \brief Returns the negative part of an expression
template<typename T>
auto
negative_part(T const& expr)
{
  return (expr.array() < 0).select(expr, T::Zero(expr.rows()));
}

/// @brief \brief Select the components of the expression if the condition is
/// fullfiled. Otherwise, set the component to value
template<typename Condition, typename T, typename Scalar>
auto
select(Condition const& condition, T const& expr, const Scalar value)
{
  return (condition).select(expr, T::Constant(expr.rows(), value));
}

} // helpers
} // proxsuite

#endif // ifndef PROXSUITE_HELPERS_COMMON_HPP
