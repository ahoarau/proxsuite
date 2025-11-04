if(PYTHON_BINDING_CUSTOM_INSTALL_DIR)
  set(PYTHON_BINDING_INSTALL_DIR ${PYTHON_BINDING_CUSTOM_INSTALL_DIR})
  message(
    STATUS
    "[proxsuite] Using custom python binding install dir: ${PYTHON_BINDING_INSTALL_DIR}"
  )
else()
  set(PYTHON_BINDING_INSTALL_DIR "${Python_SITELIB}")
  message(
    STATUS
    "[proxsuite] Using default python binding install dir: ${PYTHON_BINDING_INSTALL_DIR}"
  )
endif()

add_custom_target(proxsuite-python)

set(
  PYWRAP_HEADERS
  src/algorithms.hpp
  src/expose-backward.hpp
  src/expose-helpers.hpp
  src/expose-model.hpp
  src/expose-qpobject.hpp
  src/expose-qpvector.hpp
  src/expose-results.hpp
  src/expose-settings.hpp
  src/expose-solve.hpp
  src/expose-workspace.hpp
  src/optional-eigen-fix.hpp
)
if(BUILD_WITH_OPENMP_SUPPORT)
  list(APPEND PYWRAP_HEADERS src/expose-parallel.hpp)
endif()

set(PYWRAP_SOURCES src/expose-all.cpp)

function(create_python_target target_name)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs COMPILE_OPTIONS LINK_LIBRARIES)
  cmake_parse_arguments(
    PARSE_ARGV
    0
    arg
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
  )

  nanobind_add_module(${target_name} NB_STATIC LTO ${PYWRAP_SOURCES} ${PYWRAP_HEADERS})
  add_dependencies(proxsuite-python ${target_name})

  target_link_libraries(${target_name} PRIVATE ${arg_LINK_LIBRARIES})
  target_compile_options(${target_name} PRIVATE ${arg_COMPILE_OPTIONS})
  target_compile_definitions(
    ${target_name}
    PRIVATE PYTHON_MODULE_NAME=${target_name}
  )
  set_target_properties(
    ${target_name}
    PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/lib/site-packages/${PROJECT_NAME}
  )

  if(UNIX AND NOT APPLE)
    set_target_properties(
      ${target_name}
      PROPERTIES INSTALL_RPATH "\$ORIGIN/../../.."
    )
  endif()
endfunction()

# The main python binding target
create_python_target(proxsuite_pywrap LINK_LIBRARIES proxsuite)

# Vectorized python binding targets
if(
  BUILD_WITH_VECTORIZATION_SUPPORT
  AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "(x86)|(X86)|(amd64)|(AMD64)"
)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(AVX_COMPILE_OPTION "/arch:AVX")
    set(AVX2_COMPILE_OPTION "/arch:AVX2")
    set(FMA_COMPILE_OPTION "/fp:fast")
    set(AVX512_COMPILE_OPTION "/arch:AVX512")
  else()
    set(AVX_COMPILE_OPTION "-mavx")
    set(AVX2_COMPILE_OPTION "-mavx2")
    set(FMA_COMPILE_OPTION "-mfma")
    set(AVX512_COMPILE_OPTION "-mavx512f")
  endif()

  if(BUILD_BINDINGS_WITH_AVX2_SUPPORT)
    message(STATUS "[proxsuite] Building python bindings with AVX2 support")
    create_python_target(
      proxsuite_pywrap_avx2
      COMPILE_OPTIONS "${AVX2_COMPILE_OPTION};${FMA_COMPILE_OPTION}"
      LINK_LIBRARIES proxsuite-vectorized
    )
  endif()

  if(BUILD_BINDINGS_WITH_AVX512_SUPPORT)
    message(STATUS "[proxsuite] Building python bindings with AVX512 support")
    create_python_target(
      proxsuite_pywrap_avx512
      COMPILE_OPTIONS "${AVX512_COMPILE_OPTION};${FMA_COMPILE_OPTION}"
      LINK_LIBRARIES proxsuite-vectorized
    )
  endif()
endif()

# Add simd feature detectors for current intel CPU
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
  nanobind_add_module(instructionset NB_STATIC LTO helpers/instruction-set.cpp)
  add_dependencies(proxsuite-python instructionset)
  target_link_libraries(instructionset PRIVATE proxsuite)
  set_target_properties(
    instructionset
    PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/lib/site-packages/${PROJECT_NAME}
  )

  if(UNIX AND NOT APPLE)
    set_target_properties(
      instructionset
      PROPERTIES INSTALL_RPATH "\$ORIGIN/../../.."
    )
  endif()

  if(GENERATE_PYTHON_STUBS)
    nanobind_add_stub(
      instructionset_stub
      MODULE instructionset
      OUTPUT ${CMAKE_BINARY_DIR}/lib/site-packages/${PROJECT_NAME}/instructionset.pyi
      PYTHON_PATH $<TARGET_FILE_DIR:instructionset>
      DEPENDS instructionset
    )
  endif()
endif()

# Still relevant ? source_group ?
#add_header_group(PYWRAP_HEADERS)
#add_source_group(PYWRAP_SOURCES)

if(GENERATE_PYTHON_STUBS)
  nanobind_add_stub(
    proxsuite_pywrap_stub
    MODULE proxsuite
    VERBOSE
    OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib/site-packages/${PROJECT_NAME}
    OUTPUT
      __init__.pyi
      helpers.pyi
      proxqp/__init__.pyi
      proxqp/dense.pyi
      proxqp/sparse.pyi
    RECURSIVE
    PYTHON_PATH ${CMAKE_BINARY_DIR}/lib/site-packages
    DEPENDS proxsuite_pywrap
  )
  add_dependencies(proxsuite-python proxsuite_pywrap_stub)
endif()

xxx_python_compile_files(
  FILES
    proxsuite/__init__.py
    proxsuite/torch/__init__.py
    proxsuite/torch/qplayer.py
    proxsuite/torch/utils.py
  GEN_DIR ${CMAKE_BINARY_DIR}/lib/site-packages
)

# Install the whole python package directory
install(
  DIRECTORY ${CMAKE_BINARY_DIR}/lib/site-packages/${PROJECT_NAME}
  DESTINATION ${PYTHON_BINDING_INSTALL_DIR}
)
