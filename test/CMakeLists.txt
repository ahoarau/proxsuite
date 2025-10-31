project(proxsuite-tests)

add_library(proxsuite-test-util STATIC src/util_f64.cpp src/util_f32.cpp)
target_include_directories(proxsuite-test-util PRIVATE include)
target_link_libraries(proxsuite-test-util PUBLIC proxsuite)

add_library(
  proxsuite-test-util-vectorized
  STATIC
  src/util_f64.cpp
  src/util_f32.cpp
)
target_include_directories(proxsuite-test-util-vectorized PRIVATE include)
target_link_libraries(
  proxsuite-test-util-vectorized
  PUBLIC proxsuite-vectorized
)

function(proxsuite_add_test)
  cmake_parse_arguments(
    PARSE_ARGV
    0
    arg
    ""
    "NAME"
    "INCLUDE_DIRECTORIES;SOURCES;LINK_LIBRARIES"
  )
  add_executable(${arg_NAME} ${arg_SOURCES})
  target_include_directories(${arg_NAME} PRIVATE ${arg_INCLUDE_DIRECTORIES})
  target_link_libraries(
    ${arg_NAME}
    PRIVATE ${arg_LINK_LIBRARIES} Catch2::Catch2WithMain
  )
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

if(BUILD_WITH_VECTORIZATION_SUPPORT)
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
endif()

if(BUILD_WITH_OPENMP_SUPPORT)
  proxsuite_add_test(NAME proxsuite-test-cpp-parallel SOURCES src/parallel_qp_solve.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite)
endif()

if(BUILD_WITH_SERIALIZATION)
  proxsuite_add_test(NAME proxsuite-test-cpp-serialization SOURCES src/serialization.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite-serialization)
  make_directory(${CMAKE_BINARY_DIR}/serialization-data) # We have to create the folder for the output data, tests don't do that.
  target_compile_definitions(
    proxsuite-test-cpp-serialization
    PRIVATE TEST_SERIALIZATION_FOLDER="${CMAKE_BINARY_DIR}/serialization-data"
  )
endif()

if(BUILD_MAROS_MESZAROS_TESTS)
  proxsuite_add_test(NAME proxsuite-test-cpp-dense-maros_meszaros SOURCES src/dense_maros_meszaros.cpp INCLUDE_DIRECTORIES include LINK_LIBRARIES proxsuite matio::matio)
  target_compile_definitions(
    proxsuite-test-cpp-dense-maros_meszaros
    PRIVATE
      MAROS_MESZAROS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/maros_meszaros_data/"
  )
endif()

# if(BUILD_PYTHON_INTERFACE)
#   add_test(NAME proxsuite-python-tests
#     COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/lib/site-packages pytest ${CMAKE_CURRENT_SOURCE_DIR}
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#   )
# endif()


# if(BUILD_PYTHON_INTERFACE)
# include(FetchContent)
#   FetchContent_Declare(
#     pytest-cmake
#     URL https://github.com/python-cmake/pytest-cmake/archive/refs/tags/1.1.0.zip
#     URL_HASH MD5=d3332d7fe077402a83f10d3ab8beaada
#   )
#   FetchContent_MakeAvailable(pytest-cmake)
#   list(APPEND CMAKE_MODULE_PATH ${pytest-cmake_SOURCE_DIR}/cmake)

#   xxx_find_package(Pytest REQUIRED)
# endif()

# if(BUILD_PYTHON_INTERFACE)
#   pytest_discover_tests(
#       proxsuite-python-tests
#       PYTHON_PATH_PREPEND
#           $<TARGET_FILE_DIR:proxsuite_pywrap>/../
#       TRIM_FROM_NAME "^(Test|test_)" 
#       DEPENDS proxsuite_pywrap
#       INCLUDE_FILE_PATH
#       TEST_PATHS
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_cvxpy.py
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_dense_qp_solve.py
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_dense_qp_wrapper.py
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_parallel_qp_solve.py
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_serialization.py
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_sparse_qp_solve.py
#           ${CMAKE_CURRENT_SOURCE_DIR}/src/test_sparse_qp_wrapper.py
#   )
# endif()
if(BUILD_PYTHON_INTERFACE)

  find_program(pytest pytest REQUIRED)
  execute_process(
    COMMAND ${pytest} --version
    OUTPUT_VARIABLE pytest_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message(STATUS "Found pytest: ${pytest} (${pytest_version})")

  function(python_add_test test_name)
    add_test(NAME proxsuite-python.${test_name}
      COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/lib/site-packages pytest ${CMAKE_CURRENT_SOURCE_DIR}/src/${test_name}.py
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endfunction()

  python_add_test(test_cvxpy)
  python_add_test(test_dense_qp_solve)
  python_add_test(test_dense_qp_wrapper)

  if(BUILD_WITH_OPENMP_SUPPORT)
    python_add_test(test_parallel_qp_solve)
  endif()

  if(BUILD_WITH_SERIALIZATION)
    python_add_test(test_serialization)
  endif()

  python_add_test(test_sparse_qp_solve)
  python_add_test(test_sparse_qp_wrapper)
endif()