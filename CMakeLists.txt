#
# Copyright (c) 2022-2026 INRIA
#
cmake_minimum_required(VERSION 3.22...4.1)

project(
  proxsuite
  VERSION 0.7.2
  DESCRIPTION "The Advanced Proximal Optimization Toolbox"
  HOMEPAGE_URL "https://github.com/Simple-Robotics/proxsuite"
)

# Import CMake-provided helper functions
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)
include(CMakeDependentOption)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake-helpers.cmake)
xxx_configure_default_build_type(RelWithDebInfo)
xxx_configure_default_install_prefix("${CMAKE_BINARY_DIR}/install")
xxx_configure_default_binary_dirs()
xxx_configure_default_install_dirs()

xxx_option(BUILD_TESTING "Build the tests" ON)
xxx_option(BUILD_EXAMPLES "Build the examples" ON)
xxx_option(BUILD_BENCHMARK "Build the benchmarks" ON)
xxx_option(BUILD_DOCUMENTATION "Build the documentation." OFF)
xxx_option(INSTALL_DOCUMENTATION "Install the documentation" OFF)
xxx_option(INSTALL_HEADERS "Generate the install target" ON)

xxx_option(BUILD_PYTHON_INTERFACE "Build the Python bindings" ON)
xxx_option(INITIALIZE_EIGEN_WITH_NAN "Initialize Eigen objects with NAN values" ON)
xxx_option(
  CHECK_RUNTIME_MALLOC
  "Check if some memory allocations are performed at runtime"
  OFF
)
xxx_option(SUFFIX_SO_VERSION "Suffix library name with its version" ON)
xxx_option(
  BUILD_WITH_VECTORIZATION_SUPPORT
  "Build the library with the support of modern SIMD instructions."
  ON
)
xxx_option(
  BUILD_BINDINGS_WITH_AVX2_SUPPORT
  "Build the bindings with AVX2 support."
  OFF
)
xxx_option(
  BUILD_BINDINGS_WITH_AVX512_SUPPORT
  "Build the bindings with AVX512 support."
  OFF
)
xxx_option(TEST_JULIA_INTERFACE "Run the julia examples as unittest" OFF)
xxx_option(BUILD_WITH_OPENMP_SUPPORT "Build the library with the OpenMP support" ON)
cmake_dependent_option(
  LINK_PYTHON_INTERFACE_TO_OPENMP
  "Link OpenMP to the Python interface"
  ON
  BUILD_WITH_OPENMP_SUPPORT
  OFF
)
cmake_dependent_option(
  BUILD_WITH_SERIALIZATION
  "Build with serialization support"
  ON
  BUILD_PYTHON_INTERFACE
  ON
)
xxx_option(GENERATE_PYTHON_STUBS "Generate Python stubs" OFF)
xxx_option(
  PYTHON_BINDING_CUSTOM_INSTALL_DIR
  "Override the default <current-python-install-sitelib>/<project-name>"
  ""
)

############################################################
############################################################

# Eigen3
xxx_find_package(Eigen3 3.4.0 CONFIG REQUIRED EXPECTED_TARGETS Eigen3::Eigen )

# Catch2
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    xxx_find_package(Catch2 CONFIG REQUIRED EXPECTED_TARGETS Catch2::Catch2 Catch2::Catch2WithMain)
    include(${Catch2_DIR}/Catch.cmake)
endif()

# cereal (Serialization)
if(BUILD_WITH_SERIALIZATION)
  xxx_find_package(cereal CONFIG REQUIRED EXPECTED_TARGETS cereal::cereal)
endif()

# nanobind (Python bindings)
if(BUILD_PYTHON_INTERFACE)
    xxx_find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module EXPECTED_TARGETS Python::Interpreter Python::Module)
    get_target_property(Python_EXECUTABLE Python::Interpreter LOCATION)
    # Python_SITELIB with sysconfig.get_path('purelib')
    execute_process(
      COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('purelib'))"
      OUTPUT_STRIP_TRAILING_WHITESPACE
      OUTPUT_VARIABLE Python_SITELIB
    )
    message(STATUS "[proxsuite] Python site-packages directory: ${Python_SITELIB}")
    # Python_INCLUDE_DIRS with target include dirs
    get_target_property(Python_INCLUDE_DIRS Python::Module INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "[proxsuite] Python include directories: ${Python_INCLUDE_DIRS}")
    # Detect the installed nanobind package and import it into CMake
    # ref: https://nanobind.readthedocs.io/en/latest/building.html#finding-nanobind
    execute_process(
      COMMAND ${Python_EXECUTABLE} -m nanobind --cmake_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE
      OUTPUT_VARIABLE nanobind_ROOT
    )
    message(STATUS "[proxsuite] nanobind cmake directory: ${nanobind_ROOT}")
    xxx_find_package(nanobind CONFIG REQUIRED)
endif()


# simde
if(BUILD_WITH_VECTORIZATION_SUPPORT)
  xxx_find_package(simde REQUIRED
      MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/simde
      EXPECTED_TARGETS simde::simde 
  )
endif()

# matio
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
  xxx_find_package(matio REQUIRED
    MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/matio
    EXPECTED_TARGETS matio::matio
  )
endif()

# OpenMP
if(BUILD_WITH_OPENMP_SUPPORT)
  xxx_find_package(OpenMP REQUIRED EXPECTED_TARGETS OpenMP::OpenMP_CXX)
endif()

# Doxygen + Graphviz (dot)
if(BUILD_DOCUMENTATION AND PROJECT_IS_TOP_LEVEL)
  xxx_find_package(Doxygen REQUIRED COMPONENTS dot EXPECTED_TARGETS Doxygen::Doxygen Doxygen::dot)
endif()


get_property(packages_found GLOBAL PROPERTY _xxx_${PROJECT_NAME}_packages_found)
message(STATUS "[proxsuite] Packages found: ${packages_found}")

############################################################
############################################################

add_library(proxsuite INTERFACE)
target_compile_features(proxsuite INTERFACE cxx_std_17)
xxx_target_enforce_msvc_conformance(proxsuite)

target_include_directories(
  proxsuite
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

xxx_target_generate_config_header(proxsuite)

# xxx_target_set_standard_compile_options(proxsuite INTERFACE)
# xxx_target_treat_all_warnings_as_errors(proxsuite INTERFACE)

target_compile_definitions(
  proxsuite
  INTERFACE
    $<$<BOOL:${INITIALIZE_EIGEN_WITH_NAN}>:EIGEN_INITIALIZE_MATRICES_BY_NAN>
    $<$<BOOL:${CHECK_RUNTIME_MALLOC}>:EIGEN_RUNTIME_NO_MALLOC>
    $<$<BOOL:${CHECK_RUNTIME_MALLOC}>:PROXSUITE_EIGEN_CHECK_MALLOC>
    $<$<BOOL:${BUILD_WITH_OPENMP_SUPPORT}>:PROXSUITE_PYTHON_INTERFACE_WITH_OPENMP>
)

target_link_libraries(proxsuite INTERFACE Eigen3::Eigen)
add_library(proxsuite::proxsuite ALIAS proxsuite)

if(BUILD_WITH_OPENMP_SUPPORT)
  target_link_libraries(proxsuite INTERFACE OpenMP::OpenMP_CXX)
endif()

if(BUILD_WITH_SERIALIZATION)
  target_link_libraries(proxsuite INTERFACE cereal::cereal)
endif()

if(BUILD_PYTHON_INTERFACE)
  add_subdirectory(bindings/python)
endif()

if(BUILD_WITH_VECTORIZATION_SUPPORT)
  add_library(proxsuite-vectorized INTERFACE)
  add_library(proxsuite::proxsuite-vectorized ALIAS proxsuite-vectorized)
  target_link_libraries(proxsuite-vectorized INTERFACE proxsuite simde::simde)
  target_compile_definitions(proxsuite-vectorized INTERFACE PROXSUITE_VECTORIZE)
endif()

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
  message(STATUS "[proxsuite] Building tests")
  add_subdirectory(test)
endif()

if(BUILD_EXAMPLES AND PROJECT_IS_TOP_LEVEL)
  message(STATUS "[proxsuite] Building examples")
  add_subdirectory(examples)
endif()

if(BUILD_BENCHMARK AND PROJECT_IS_TOP_LEVEL)
  message(STATUS "[proxsuite] Building benchmarks")
  add_subdirectory(benchmark)
endif()

if(BUILD_DOCUMENTATION AND PROJECT_IS_TOP_LEVEL)
  message(STATUS "[proxsuite] Building documentation")
  add_subdirectory(doc)
endif()

xxx_declare_component(TARGETS proxsuite COMPONENT standard)

if(BUILD_WITH_VECTORIZATION_SUPPORT)
  xxx_declare_component(TARGETS proxsuite-vectorized COMPONENT vectorized)
endif()

if(INSTALL_HEADERS)
  install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
  )
endif()

xxx_cmake_module_files()

return()

# xxx_export_dependencies(
#   TARGETS proxsuite
#   EXPORT proxsuite-standard
#   FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-standard-dependencies.cmake
# )

# xxx_export_dependencies(
#   TARGETS proxsuite-vectorized
#   EXPORT proxsuite-vectorized
#   FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-vectorized-dependencies.cmake
# )


# if(INSTALL_HEADERS)
#   # Install Headers
#   install(
#     DIRECTORY ${PROJECT_SOURCE_DIR}/include/
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#     FILES_MATCHING
#     PATTERN "*.hpp"
#   )

#   # xxx_setup_install(
#   #   TARGETS proxsuite
#   #   EXPORT proxsuite-standard
#   #   HEADER_FILES
#   #     include/proxsuite/fwd.hpp
#   #   HEADER_DIRS
#   #     include/proxsuite/helpers
#   #     include/proxsuite/linalg
#   #     include/proxsuite/proxqp
      
#   # )

#   xxx_cmake_module_targets(
#     TARGETS proxsuite
#     EXPORT ${PROJECT_NAME}-standard-targets
#     NAMESPACE ${PROJECT_NAME}::
#     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-standard-targets.cmake
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#   )
#   if(BUILD_WITH_VECTORIZATION_SUPPORT)
#     xxx_cmake_module_targets(
#       TARGETS proxsuite-vectorized
#       EXPORT ${PROJECT_NAME}-vectorized
#       NAMESPACE ${PROJECT_NAME}::
#       OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-vectorized-targets.cmake
#       DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#     )
#   endif()

#   xxx_cmake_module_version(
#     VERSION ${PROJECT_VERSION}
#     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-version.cmake
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#   )

#   # Declare the components for the project
#   xxx_declare_component(TARGETS proxsuite COMPONENT standard)
#   xxx_declare_component(TARGETS proxsuite-vectorized COMPONENT vectorized)

#   # generate the CMake module:
#   #   <package>-config.cmake
#   # And for each component:
#   #     <component>-targets.cmake
#   #     <component>-dependencies.cmake
#   xxx_cmake_module_config(
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#   )


#   # xxx_generate_component_dependencies(TARGETS proxsuite
#   #   EXPORT ${PROJECT_NAME}-standard-targets 
#   #   FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-standard-dependencies.cmake
#   # )

#   # Install libraries
#   install(TARGETS proxsuite
#         EXPORT ${PROJECT_NAME}-standard-targets
#         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   )
#   install(EXPORT ${PROJECT_NAME}-standard-targets
#         FILE ${PROJECT_NAME}-standard-targets.cmake
#         NAMESPACE ${PROJECT_NAME}::
#         DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#   )
#   string(MD5 tmp_dest_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
#   set(tmp_target_file ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/Export/${tmp_dest_dir}/${PROJECT_NAME}-standard-targets.cmake)
#   file(COPY ${tmp_target_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME})
#   unset(tmp_dest_dir)
#   unset(tmp_target_file)
  
#   # export(EXPORT standard-targets 
#   #       FILE ${CMAKE_CURRENT_BINARY_DIR}/standard-targets.cmake 
#   #       NAMESPACE proxsuite::
#   # )

#   if(BUILD_WITH_VECTORIZATION_SUPPORT)
#     install(TARGETS ${PROJECT_NAME}-vectorized
#           EXPORT ${PROJECT_NAME}-vectorized
#           ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#           LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     )
#     install(EXPORT ${PROJECT_NAME}-vectorized
#           FILE ${PROJECT_NAME}-vectorized-targets.cmake
#           NAMESPACE ${PROJECT_NAME}::
#           DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#     )
#   endif()

#   include(CMakePackageConfigHelpers)

#   set(components "")
#   list(APPEND components standard)
#   list(APPEND components vectorized)
#   string(REPLACE ";" " " xxx_project_components "${components}")

#   # <package>-config.cmake
#   configure_package_config_file(
#       ${PROJECT_SOURCE_DIR}/cmake/config.cmake.in
#       ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake
#       INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
#       NO_SET_AND_CHECK_MACRO
#       NO_CHECK_REQUIRED_COMPONENTS_MACRO
#   )

#   # <package>-version.cmake
#   write_basic_package_version_file(
#       ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}/${PROJECT_NAME}-version.cmake
#       COMPATIBILITY AnyNewerVersion
#   )

#   # Install all generated cmake files
#   install(
#       DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/cmake/${PROJECT_NAME}
#       DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
#       FILES_MATCHING PATTERN "*.cmake"
#   )
#   # Generate and install <package>-config.cmake, <package>-version.cmake and <package>-targets.cmake
#   # xxx_generate_cmake_module_files()
# endif()

#xxx_print_dependency_summary()
#xxx_print_option_summary()