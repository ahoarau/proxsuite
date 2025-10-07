project(proxsuite-tests)

# add_library(proxsuite-test-util STATIC
#     src/util_f64.cpp
#     src/util_f32.cpp
# )
# target_include_directories(proxsuite-test-util PRIVATE include)
# target_link_libraries(proxsuite-test-util PRIVATE proxsuite)
# set(proxsuite-tests-sources
#   src/dense_backward.cpp
#   src/dense_qp_eq.cpp
#   src/dense_qp_solve.cpp
#   src/dense_qp_with_eq_and_in.cpp
#   src/dense_qp_wrapper.cpp
#   src/dense_ruiz_equilibration.cpp
#   src/dense_unconstrained_qp.cpp
#   src/sparse_factorization.cpp
#   src/sparse_qp_solve.cpp
#   src/sparse_qp_wrapper.cpp
#   src/sparse_qp.cpp
#   src/sparse_ruiz_equilibration.cpp
# )

function(proxsuite_add_test)
  cmake_parse_arguments(
    ARG
    ""
    "NAME"
    "INCLUDE_DIRECTORIES;SOURCES;LINK_LIBRARIES"
    ${ARGN}
  )
  add_executable(${ARG_NAME} ${ARG_SOURCES})
  target_include_directories(${ARG_NAME} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
  target_link_libraries(
    ${ARG_NAME}
    PRIVATE ${ARG_LINK_LIBRARIES} Catch2::Catch2WithMain
  )
  catch_discover_tests(${ARG_NAME} DISCOVERY_MODE "PRE_TEST")
endfunction()

# gersemi: off
proxsuite_add_test(NAME proxsuite-test-cpp-dense_backward SOURCES src/dense_backward.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-eq SOURCES src/dense_qp_eq.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-solve SOURCES src/dense_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-with-eq-and-in SOURCES src/dense_qp_with_eq_and_in.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-wrapper SOURCES src/dense_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-ruiz-equilibration SOURCES src/dense_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-unconstrained-qb SOURCES src/dense_unconstrained_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-factorization SOURCES src/sparse_factorization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-solve SOURCES src/sparse_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-wrapper SOURCES src/sparse_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb SOURCES src/sparse_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-ruiz-equilibration SOURCES src/sparse_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)

# Vectorized tests
proxsuite_add_test(NAME proxsuite-test-cpp-dense_backward-vectorized SOURCES src/dense_backward.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-eq-vectorized SOURCES src/dense_qp_eq.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-solve-vectorized SOURCES src/dense_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-with-eq-and-in-vectorized SOURCES src/dense_qp_with_eq_and_in.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-qb-wrapper-vectorized SOURCES src/dense_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-ruiz-equilibration-vectorized SOURCES src/dense_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-dense-unconstrained-qb-vectorized SOURCES src/dense_unconstrained_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-factorization-vectorized SOURCES src/sparse_factorization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-solve-vectorized SOURCES src/sparse_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-wrapper-vectorized SOURCES src/sparse_qp_wrapper.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-qb-vectorized SOURCES src/sparse_qp.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
proxsuite_add_test(NAME proxsuite-test-cpp-sparse-ruiz-equilibration-vectorized SOURCES src/sparse_ruiz_equilibration.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-vectorized)
# gersemi: on

# proxsuite_add_test(proxsuite-test-cpp-dense_backward src/dense_backward.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-dense-qb-eq src/dense_qp_eq.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-dense-qb-solve src/dense_qp_solve.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-dense-qb-with-eq-and-in src/dense_qp_with_eq_and_in.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-dense-qb-wrapper src/dense_qp_wrapper.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-dense-ruiz-equilibration src/dense_ruiz_equilibration.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-dense-unconstrained-qb src/dense_unconstrained_qp.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-sparse-factorization src/sparse_factorization.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-sparse-qb-solve src/sparse_qp_solve.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-sparse-qb-wrapper src/sparse_qp_wrapper.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-sparse-qb src/sparse_qp.cpp proxsuite)
# proxsuite_add_test(proxsuite-test-cpp-sparse-ruiz-equilibration src/sparse_ruiz_equilibration.cpp proxsuite)

# add_executable(proxsuite-tests ${proxsuite-tests-sources} )
# target_link_libraries(proxsuite-tests PRIVATE proxsuite util Catch2::Catch2WithMain)

# add_executable(proxsuite-tests-vectorized ${proxsuite-tests-sources})
# target_include_directories(proxsuite-tests-vectorized PRIVATE include)
# target_link_libraries(proxsuite-tests-vectorized PRIVATE proxsuite-vectorized Catch2::Catch2WithMain)

## THOSE REQUIRE MATIO --> To be replaced by json files + python script to generate them
## add_executable(dense_maros_meszaros src/dense_maros_meszaros.cpp)
## target_link_libraries(dense_maros_meszaros PRIVATE utils)

## add_executable(sparse_maros_meszaros src/sparse_maros_meszaros.cpp)
## target_link_libraries(sparse_maros_meszaros PRIVATE utils)

# add_executable(dense_qp_eq src/dense_qp_eq.cpp)

# target_link_libraries(dense_qp_eq PRIVATE Catch2::Catch2WithMain proxsuite)
# doctest_discover_tests(dense_qp_eq)

# add_executable(dense_qp_solve src/dense_qp_solve.cpp)
# target_include_directories(dense_qp_solve PRIVATE include)
# target_link_libraries(dense_qp_solve PRIVATE utils)
# doctest_discover_tests(dense_qp_solve)

# add_executable(dense_qp_with_eq_and_in src/dense_qp_with_eq_and_in.cpp)
# target_sources(proxsuite-tests PRIVATE src/dense_qp_with_eq_and_in.cpp)
# target_include_directories(dense_qp_with_eq_and_in PRIVATE include)
# target_link_libraries(dense_qp_with_eq_and_in PRIVATE utils)

# add_executable(dense_qp_wrapper src/dense_qp_wrapper.cpp)
# target_sources(proxsuite-tests PRIVATE src/dense_qp_wrapper.cpp)
# target_include_directories(dense_qp_wrapper PRIVATE include)
# target_link_libraries(dense_qp_wrapper PRIVATE utils)

# add_executable(dense_ruiz_equilibration src/dense_ruiz_equilibration.cpp)
# target_sources(proxsuite-tests PRIVATE src/dense_ruiz_equilibration.cpp)
# target_include_directories(dense_ruiz_equilibration PRIVATE include)
# target_link_libraries(dense_ruiz_equilibration PRIVATE utils)

# add_executable(dense_unconstrained_qp src/dense_unconstrained_qp.cpp)
# target_include_directories(dense_unconstrained_qp PRIVATE include)
# target_link_libraries(dense_unconstrained_qp PRIVATE utils)

# add_executable(parallel_qp_solve src/parallel_qp_solve.cpp)
# target_include_directories(parallel_qp_solve PRIVATE include)
# target_link_libraries(parallel_qp_solve PRIVATE utils)

# ## add_executable(serialization src/serialization.cpp)
# ## target_include_directories(serialization PRIVATE include)
# ## target_link_libraries(serialization PRIVATE utils)

# add_executable(sparse_factorization src/sparse_factorization.cpp)
# target_include_directories(sparse_factorization PRIVATE include)
# target_link_libraries(sparse_factorization PRIVATE utils)

# add_executable(sparse_qp_solve src/sparse_qp_solve.cpp)
# target_include_directories(sparse_qp_solve PRIVATE include)
# target_link_libraries(sparse_qp_solve PRIVATE utils)

# add_executable(sparse_qp_wrapper src/sparse_qp_wrapper.cpp)
# target_include_directories(sparse_qp_wrapper PRIVATE include)
# target_link_libraries(sparse_qp_wrapper PRIVATE utils)

# add_executable(sparse_qp src/sparse_qp.cpp)
# target_include_directories(sparse_qp PRIVATE include)
# target_link_libraries(sparse_qp PRIVATE utils)

# add_executable(sparse_ruiz_equilibration src/sparse_ruiz_equilibration.cpp)
# target_include_directories(sparse_ruiz_equilibration PRIVATE include)
# target_link_libraries(sparse_ruiz_equilibration PRIVATE utils)

# add_library(${PROJECT_NAME}-doctest STATIC doctest/doctest.cpp)
# target_include_directories(${PROJECT_NAME}-doctest PUBLIC ./doctest)
# add_library(cnpy OBJECT src/cnpy.cpp)
# target_link_libraries(cnpy Eigen3::Eigen)
# target_include_directories(cnpy PUBLIC ./include)

# file(GLOB_RECURSE TEST_HEADERS ./include/*.hpp)
# ADD_HEADER_GROUP(TEST_HEADERS)

# add_library(
#   proxsuite-test-util
#   STATIC
#   ./src/util_f64.cpp
#   ./src/util_f32.cpp
#   ${TEST_HEADERS}
# )
# target_include_directories(proxsuite-test-util PUBLIC ./include)
# if(BUILD_WITH_VECTORIZATION_SUPPORT)
#   target_link_libraries(proxsuite-test-util proxsuite-vectorized matio)
# else()
#   target_link_libraries(proxsuite-test-util proxsuite matio)
# endif()

# macro(proxsuite_test name path)
#   set(target_name ${PROJECT_NAME}-test-cpp-${name})
#   add_executable(${target_name} ${path})
#   doctest_discover_tests(${target_name})
#   target_link_libraries(
#     ${target_name}
#     PUBLIC proxsuite ${PROJECT_NAME}-doctest proxsuite-test-util
#   )
#   target_compile_definitions(
#     ${target_name}
#     PRIVATE PROBLEM_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
#   )
#   add_dependencies(build_tests ${target_name})
# endmacro()

# proxsuite_test(dense_ruiz_equilibration src/dense_ruiz_equilibration.cpp)
# proxsuite_test(dense_qp_eq src/dense_qp_eq.cpp)
# proxsuite_test(dense_qp_with_eq_and_in src/dense_qp_with_eq_and_in.cpp)
# proxsuite_test(dense_qp_unconstrained src/dense_unconstrained_qp.cpp)
# proxsuite_test(dense_backward src/dense_backward.cpp)
# proxsuite_test(dense_qp_wrapper src/dense_qp_wrapper.cpp)
# proxsuite_test(dense_qp_solve src/dense_qp_solve.cpp)
# proxsuite_test(sparse_ruiz_equilibration src/sparse_ruiz_equilibration.cpp)
# proxsuite_test(sparse_qp src/sparse_qp.cpp)
# proxsuite_test(sparse_qp_wrapper src/sparse_qp_wrapper.cpp)
# proxsuite_test(sparse_qp_solve src/sparse_qp_solve.cpp)
# proxsuite_test(sparse_factorization src/sparse_factorization.cpp)
# proxsuite_test(cvxpy src/cvxpy.cpp)

if(BUILD_WITH_OPENMP_SUPPORT)
  proxsuite_add_test(NAME proxsuite-test-cpp-parallel SOURCES src/parallel_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
endif()

if(BUILD_WITH_SERIALIZATION)
  proxsuite_add_test(NAME proxsuite-test-cpp-serialization SOURCES src/serialization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
  # We have to create the folder for the output data, tests don't do that.
  make_directory(${CMAKE_BINARY_DIR}/serialization-data)
  target_compile_definitions(
    proxsuite-test-cpp-serialization
    PRIVATE TEST_SERIALIZATION_FOLDER="${CMAKE_BINARY_DIR}/serialization-data"
  )
endif()

proxsuite_add_test(NAME proxsuite-test-cpp-dense-maros_meszaros SOURCES src/dense_maros_meszaros.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite matio::matio)
target_compile_definitions(
  proxsuite-test-cpp-dense-maros_meszaros
  PRIVATE
    MAROS_MESZAROS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/maros_meszaros_data/"
)

# # Test serialization
# macro(ADD_TEST_CFLAGS target flag)
#   set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${flag}")
# endmacro()

# make_directory("${CMAKE_CURRENT_BINARY_DIR}/serialization-data")
# proxsuite_test(serialization src/serialization.cpp)
# add_test_cflags(
#   ${PROJECT_NAME}-test-cpp-serialization
#   "-DTEST_SERIALIZATION_FOLDER=\\\\\"${CMAKE_CURRENT_BINARY_DIR}/serialization-data\\\\\""
# )
# if(cereal_FOUND)
#   target_link_libraries(
#     ${PROJECT_NAME}-test-cpp-serialization
#     PRIVATE cereal::cereal
#   )
# else()
#   target_include_directories(
#     ${PROJECT_NAME}-test-cpp-serialization
#     SYSTEM
#     PRIVATE ${PROJECT_SOURCE_DIR}/external/cereal/include
#   )
# endif()

# if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
#   proxsuite_test(dense_maros_meszaros src/dense_maros_meszaros.cpp)
#   proxsuite_test(sparse_maros_meszaros src/sparse_maros_meszaros.cpp)
# endif()

# if(BUILD_PYTHON_INTERFACE)
#   file(GLOB_RECURSE ${PROJECT_NAME}_PYTHON_UNITTEST *.py)

#   if(
#     (CMAKE_BUILD_TYPE STREQUAL "Debug" AND MSVC)
#     OR NOT BUILD_WITH_OPENMP_SUPPORT
#   )
#     list_filter(
#       "${${PROJECT_NAME}_PYTHON_UNITTEST}"
#       "parallel_qp_solve"
#       ${PROJECT_NAME}_PYTHON_UNITTEST
#     )
#   endif()

#   foreach(TEST_FILE ${${PROJECT_NAME}_PYTHON_UNITTEST})
#     get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
#     string(REGEX REPLACE "${PROJECT_SOURCE_DIR}/" "" TEST_FILE ${TEST_FILE})
#     ADD_PYTHON_UNIT_TEST(
#       "${PROJECT_NAME}-test-py-${TEST_NAME}"
#       "${TEST_FILE}"
#       "bindings/python"
#     )
#   endforeach()
# endif()
