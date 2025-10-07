project(proxsuite-benchmark)

add_executable(timings-box-constraints timings-box-constraints.cpp)
target_link_libraries(timings-box-constraints PRIVATE proxsuite)

add_executable(timings-dense-backend timings-dense-backend.cpp)
target_link_libraries(timings-dense-backend PRIVATE proxsuite)

add_executable(timings-diagonal-hessian timings-diagonal-hessian.cpp)
target_link_libraries(timings-diagonal-hessian PRIVATE proxsuite)

add_executable(timings-lp timings-lp.cpp)
target_link_libraries(timings-lp PRIVATE proxsuite)

add_executable(timings-box-constraints-vectorized timings-box-constraints.cpp)
target_link_libraries(
  timings-box-constraints-vectorized
  PRIVATE proxsuite-vectorized
)

add_executable(timings-dense-backend-vectorized timings-dense-backend.cpp)
target_link_libraries(
  timings-dense-backend-vectorized
  PRIVATE proxsuite-vectorized
)

add_executable(timings-diagonal-hessian-vectorized timings-diagonal-hessian.cpp)
target_link_libraries(
  timings-diagonal-hessian-vectorized
  PRIVATE proxsuite-vectorized
)

add_executable(timings-lp-vectorized timings-lp.cpp)
target_link_libraries(timings-lp-vectorized PRIVATE proxsuite-vectorized)

if(BUILD_WITH_OPENMP_SUPPORT)
  add_executable(timings-parallel timings-parallel.cpp)
  target_link_libraries(timings-parallel PRIVATE proxsuite)

  add_executable(timings-parallel-vectorized timings-parallel.cpp)
  target_link_libraries(
    timings-parallel-vectorized
    PRIVATE proxsuite-vectorized
  )
endif()
