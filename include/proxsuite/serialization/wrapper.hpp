//
// Copyright (c) 2022 INRIA
//
/**
 * @file wrapper.hpp
 */

#pragma once

#include <cereal/cereal.hpp>
#include <proxsuite/proxqp/dense/wrapper.hpp>

namespace cereal {

template<class Archive, typename T>
void
serialize(Archive& archive, proxsuite::proxqp::dense::QP<T>& qp)
{
  archive(
    CEREAL_NVP(qp.model), CEREAL_NVP(qp.results), CEREAL_NVP(qp.settings));
} // CEREAL_NVP(qp.ruiz), ,CEREAL_NVP(qp.ruiz)
} // namespace cereal
