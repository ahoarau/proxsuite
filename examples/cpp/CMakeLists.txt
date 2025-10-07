project(proxsuite-examples-cpp)

#
# Copyright (c) 2022 INRIA
#
add_custom_target(proxsuite-examples-cpp)

# function(ADD_PROXSUITE_CPP_EXAMPLE EXAMPLE)
#   get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
#   set(EXAMPLE_TARGET "${PROJECT_NAME}-example-cpp-${EXAMPLE_NAME}")
#   ADD_UNIT_TEST(${EXAMPLE_TARGET} "${EXAMPLE}")
#   target_link_libraries(${EXAMPLE_TARGET} PRIVATE proxsuite-test-util)

#   add_dependencies(${PROJECT_NAME}-example-cpp ${EXAMPLE_TARGET})
# endfunction()

# file(GLOB_RECURSE ${PROJECT_NAME}_CPP_EXAMPLES *.cpp)

# foreach(EXAMPLE ${${PROJECT_NAME}_CPP_EXAMPLES})
#   add_proxsuite_cpp_example(${EXAMPLE})
# endforeach()

add_executable(estimate_nonconvex_eigenvalue estimate_nonconvex_eigenvalue.cpp)
target_link_libraries(estimate_nonconvex_eigenvalue PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp estimate_nonconvex_eigenvalue)

add_executable(first_example_dense first_example_dense.cpp)
target_link_libraries(first_example_dense PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp first_example_dense)

add_executable(first_example_sparse first_example_sparse.cpp)
target_link_libraries(first_example_sparse PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp first_example_sparse)

add_executable(init_dense_qp_with_box init_dense_qp_with_box.cpp)
target_link_libraries(init_dense_qp_with_box PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp init_dense_qp_with_box)

add_executable(
  init_dense_qp_with_other_options
  init_dense_qp_with_other_options.cpp
)
target_link_libraries(init_dense_qp_with_other_options PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp init_dense_qp_with_other_options)

add_executable(init_dense_qp_with_timings init_dense_qp_with_timings.cpp)
target_link_libraries(init_dense_qp_with_timings PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp init_dense_qp_with_timings)

add_executable(init_dense_qp init_dense_qp.cpp)
target_link_libraries(init_dense_qp PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp init_dense_qp)

add_executable(init_with_default_options init_with_default_options.cpp)
target_link_libraries(init_with_default_options PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp init_with_default_options)

add_executable(
  initializing_with_none_without_api
  initializing_with_none_without_api.cpp
)
target_link_libraries(initializing_with_none_without_api PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp initializing_with_none_without_api)

add_executable(initializing_with_none initializing_with_none.cpp)
target_link_libraries(initializing_with_none PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp initializing_with_none)

add_executable(
  loading_dense_qp_with_box_ineq
  loading_dense_qp_with_box_ineq.cpp
)
target_link_libraries(loading_dense_qp_with_box_ineq PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp loading_dense_qp_with_box_ineq)

add_executable(
  loading_dense_qp_with_different_backend_choice
  loading_dense_qp_with_different_backend_choice.cpp
)
target_link_libraries(
  loading_dense_qp_with_different_backend_choice
  PRIVATE proxsuite
)
add_dependencies(
  proxsuite-examples-cpp
  loading_dense_qp_with_different_backend_choice
)

add_executable(loading_dense_qp loading_dense_qp.cpp)
target_link_libraries(loading_dense_qp PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp loading_dense_qp)

add_executable(loading_sparse_qp loading_sparse_qp.cpp)
target_link_libraries(loading_sparse_qp PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp loading_sparse_qp)

add_executable(overview-simple overview-simple.cpp)
target_link_libraries(overview-simple PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp overview-simple)

add_executable(solve_dense_qp_with_setting solve_dense_qp_with_setting.cpp)
target_link_libraries(solve_dense_qp_with_setting PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp solve_dense_qp_with_setting)

add_executable(solve_dense_qp solve_dense_qp.cpp)
target_link_libraries(solve_dense_qp PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp solve_dense_qp)

add_executable(solve_without_api_and_option solve_without_api_and_option.cpp)
target_link_libraries(solve_without_api_and_option PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp solve_without_api_and_option)

add_executable(solve_without_api solve_without_api.cpp)
target_link_libraries(solve_without_api PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp solve_without_api)

add_executable(
  update_dense_qp_ws_previous_result
  update_dense_qp_ws_previous_result.cpp
)
target_link_libraries(update_dense_qp_ws_previous_result PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp update_dense_qp_ws_previous_result)

add_executable(update_dense_qp update_dense_qp.cpp)
target_link_libraries(update_dense_qp PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp update_dense_qp)

add_executable(update_sparse_qp update_sparse_qp.cpp)
target_link_libraries(update_sparse_qp PRIVATE proxsuite)
add_dependencies(proxsuite-examples-cpp update_sparse_qp)
