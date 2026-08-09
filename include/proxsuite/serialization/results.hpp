//
// Copyright (c) 2022 INRIA
//
/**
 * @file results.hpp
 */

#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <proxsuite/proxqp/results.hpp>

namespace cereal {

template<class Archive, typename T>
void
serialize(Archive& archive, proxsuite::proxqp::Info<T>& info)
{
  archive(CEREAL_NVP(info.mu_eq),
          CEREAL_NVP(info.mu_eq_inv),
          CEREAL_NVP(info.mu_in),
          CEREAL_NVP(info.mu_in_inv),
          CEREAL_NVP(info.rho),
          CEREAL_NVP(info.nu),
          CEREAL_NVP(info.iter),
          CEREAL_NVP(info.iter_ext),
          CEREAL_NVP(info.mu_updates),
          CEREAL_NVP(info.rho_updates),
          CEREAL_NVP(info.status),
          CEREAL_NVP(info.setup_time),
          CEREAL_NVP(info.solve_time),
          CEREAL_NVP(info.run_time),
          CEREAL_NVP(info.objValue),
          CEREAL_NVP(info.pri_res),
          CEREAL_NVP(info.dua_res),
          CEREAL_NVP(info.duality_gap),
          CEREAL_NVP(info.iterative_residual),
          CEREAL_NVP(info.sparse_backend));
}

template<class Archive, typename T>
void
serialize(Archive& archive, proxsuite::proxqp::Results<T>& results)
{
  archive(CEREAL_NVP(results.x),
          CEREAL_NVP(results.y),
          CEREAL_NVP(results.z),
          CEREAL_NVP(results.se),
          CEREAL_NVP(results.si),
          CEREAL_NVP(results.active_constraints),
          CEREAL_NVP(results.info));
}

} // namespace cereal
