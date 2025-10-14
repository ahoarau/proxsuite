project(proxsuite-tests)

# gersemi: off

add_library(proxsuite-test-util STATIC
    src/util_f64.cpp
    src/util_f32.cpp
)
target_include_directories(proxsuite-test-util PRIVATE include)
target_link_libraries(proxsuite-test-util PUBLIC proxsuite)

add_library(proxsuite-test-util-vectorized STATIC
    src/util_f64.cpp
    src/util_f32.cpp
)
target_include_directories(proxsuite-test-util-vectorized PRIVATE include)
target_link_libraries(proxsuite-test-util-vectorized PUBLIC proxsuite-vectorized)

function(proxsuite_add_test)
  cmake_parse_arguments(PARSE_ARGV 0 arg "" "NAME" "INCLUDE_DIRECTORIES;SOURCES;LINK_LIBRARIES")
  add_executable(${arg_NAME} ${arg_SOURCES})
  target_include_directories(${arg_NAME} PRIVATE ${arg_INCLUDE_DIRECTORIES})
  target_link_libraries(${arg_NAME} PRIVATE ${arg_LINK_LIBRARIES} Catch2::Catch2WithMain)
  catch_discover_tests(${arg_NAME} DISCOVERY_MODE "PRE_TEST")
endfunction()

# Standard tests
proxsuite_add_test(NAME proxsuite-test-cpp-dense_backward SOURCES src/dense_backward.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-eq SOURCES src/dense_qp_eq.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-solve SOURCES src/dense_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-with-eq-and-in SOURCES src/dense_qp_with_eq_and_in.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-wrapper SOURCES src/dense_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-ruiz-equilibration SOURCES src/dense_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-unconstrained-qb SOURCES src/dense_unconstrained_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-factorization SOURCES src/sparse_factorization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-solve SOURCES src/sparse_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-wrapper SOURCES src/sparse_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb SOURCES src/sparse_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-ruiz-equilibration SOURCES src/sparse_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util)

# Vectorized tests
proxsuite_add_test(NAME proxsuite-test-cpp-dense_backward-vectorized SOURCES src/dense_backward.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-eq-vectorized SOURCES src/dense_qp_eq.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-solve-vectorized SOURCES src/dense_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-with-eq-and-in-vectorized SOURCES src/dense_qp_with_eq_and_in.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-wrapper-vectorized SOURCES src/dense_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-ruiz-equilibration-vectorized SOURCES src/dense_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-unconstrained-qb-vectorized SOURCES src/dense_unconstrained_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-factorization-vectorized SOURCES src/sparse_factorization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-solve-vectorized SOURCES src/sparse_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-wrapper-vectorized SOURCES src/sparse_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-vectorized SOURCES src/sparse_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-ruiz-equilibration-vectorized SOURCES src/sparse_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-test-util-vectorized)


if(BUILD_WITH_OPENMP_SUPPORT)
  proxsuite_add_test(NAME proxsuite-test-cpp-parallel SOURCES src/parallel_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
endif()

if(BUILD_WITH_SERIALIZATION)
  proxsuite_add_test(NAME proxsuite-test-cpp-serialization SOURCES src/serialization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
  make_directory(${CMAKE_BINARY_DIR}/serialization-data) # We have to create the folder for the output data, tests don't do that.
  target_compile_definitions(proxsuite-test-cpp-serialization PRIVATE TEST_SERIALIZATION_FOLDER="${CMAKE_BINARY_DIR}/serialization-data")
endif()

if(BUILD_MAROS_MESZAROS_TESTS)
  proxsuite_add_test(NAME proxsuite-test-cpp-dense-maros_meszaros SOURCES src/dense_maros_meszaros.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite matio::matio)
  target_compile_definitions(proxsuite-test-cpp-dense-maros_meszaros PRIVATE MAROS_MESZAROS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/maros_meszaros_data/")
endif()

# gersemi: on
