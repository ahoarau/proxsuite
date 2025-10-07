if(PYTHON_BINDING_CUSTOM_INSTALL_DIR)
  set(PYTHON_BINDING_INSTALL_DIR ${PYTHON_BINDING_CUSTOM_INSTALL_DIR})
  message(
    STATUS
    "[proxsuite] Using custom python binding install dir: ${PYTHON_BINDING_CUSTOM_INSTALL_DIR}"
  )
else()
  set(PYTHON_BINDING_INSTALL_DIR "${Python_SITELIB}/${PROJECT_NAME}")
  message(
    STATUS
    "[proxsuite] Using default python binding install dir: ${PYTHON_BINDING_INSTALL_DIR}"
  )
endif()

add_custom_target(${PROJECT_NAME}-python)

set(
  PYWRAP_HEADERS
  src/algorithms.hpp
  src/expose-backward.hpp
  src/expose-helpers.hpp
  src/expose-model.hpp
  src/expose-parallel.hpp
  src/expose-qpobject.hpp
  src/expose-qpvector.hpp
  src/expose-results.hpp
  src/expose-settings.hpp
  src/expose-solve.hpp
  src/expose-workspace.hpp
  src/optional-eigen-fix.hpp
)
set(PYWRAP_SOURCES src/expose-all.cpp)

# Add simd feature detectors for current intel CPU
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
  nanobind_add_module(instructionset helpers/instruction-set.cpp)
  add_dependencies(${PROJECT_NAME}-python instructionset)
  target_link_libraries(instructionset PRIVATE proxsuite)
  set_target_properties(
    instructionset
    PROPERTIES
      OUTPUT_NAME instructionset
      LIBRARY_OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      # On Windows, shared library are treat as binary
      RUNTIME_OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
  )
  if(UNIX AND NOT APPLE)
    set_target_properties(
      instructionset
      PROPERTIES INSTALL_RPATH "\$ORIGIN/../../.."
    )
  endif()
  install(
    TARGETS instructionset
    EXPORT ${PROJECT_NAME}Targets
    DESTINATION ${PYTHON_BINDING_INSTALL_DIR}
  )
  if(GENERATE_PYTHON_STUBS)
    nanobind_add_stub(
      instructionset_stub
      MODULE instructionset
      OUTPUT instructionset.pyi
      PYTHON_PATH $<TARGET_FILE_DIR:instructionset>
      DEPENDS instructionset
    )
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/instructionset.pyi
      DESTINATION ${PYTHON_BINDING_INSTALL_DIR}
    )
  endif()
endif()

function(list_filter list regular_expression dest_list)
  foreach(elt ${list})
    if(${elt} MATCHES ${regular_expression})
      list(REMOVE_ITEM list ${elt})
    endif()
  endforeach(elt ${list})
  set(${dest_list} ${list} PARENT_SCOPE)
endfunction(list_filter)

function(
  create_python_target
  target_name
  compile_options
  dependencies
  generate_stubs
)
  nanobind_add_module(${target_name} ${PYWRAP_SOURCES} ${PYWRAP_HEADERS}) # TODO: add LTO ?
  add_dependencies(${PROJECT_NAME}-python ${target_name})

  target_link_libraries(${target_name} PUBLIC ${dependencies})
  target_compile_options(${target_name} PRIVATE ${compile_options})
  target_link_libraries(${target_name} PRIVATE proxsuite)
  target_compile_definitions(
    ${target_name}
    PRIVATE PYTHON_MODULE_NAME=${target_name}
  )

  if(BUILD_WITH_OPENMP_SUPPORT)
    target_compile_options(${target_name} PRIVATE ${OpenMP_CXX_FLAGS})
    target_compile_definitions(
      ${target_name}
      PRIVATE PROXSUITE_PYTHON_INTERFACE_WITH_OPENMP
    )
    target_include_directories(
      ${target_name}
      SYSTEM
      PRIVATE ${OpenMP_CXX_INCLUDE_DIR}
    )
    if(LINK_PYTHON_INTERFACE_TO_OPENMP)
      target_link_libraries(${target_name} PRIVATE ${OpenMP_CXX_LIBRARIES})
    endif()
  else()
    list_filter("${PYWRAP_HEADERS}" "expose-parallel" PYWRAP_HEADERS)
  endif()

  target_link_libraries(${target_name} PRIVATE $<$<BOOL:${BUILD_WITH_SERIALIZATION}>:cereal::cereal>)

  set_target_properties(
    ${target_name}
    PROPERTIES
      OUTPUT_NAME ${target_name}
      LIBRARY_OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      # On Windows, shared library are treat as binary
      RUNTIME_OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${PROJECT_BINARY_DIR}/bindings/python/${PROJECT_NAME}"
  )

  if(UNIX AND NOT APPLE)
    set_target_properties(
      ${target_name}
      PROPERTIES INSTALL_RPATH "\$ORIGIN/../../.."
    )
  endif()

  install(TARGETS ${target_name} DESTINATION ${PYTHON_BINDING_INSTALL_DIR})
  if(${generate_stubs})
    set(
      stub_outputs
      ${target_name}/__init__.pyi
      ${target_name}/helpers.pyi
      ${target_name}/proxqp/__init__.pyi
      ${target_name}/proxqp/dense.pyi
      ${target_name}/proxqp/sparse.pyi
    )
    nanobind_add_stub(
      ${target_name}_stub
      MODULE ${target_name}
      OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}
      OUTPUT ${stub_outputs}
      RECURSIVE
      PYTHON_PATH $<TARGET_FILE_DIR:${target_name}>
      DEPENDS ${target_name}
    )
    install(
      DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target_name}
      DESTINATION ${PYTHON_BINDING_INSTALL_DIR}
    )
  endif()
endfunction()

if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  set(AVX_COMPILE_OPTION "/arch:AVX")
  set(AVX2_COMPILE_OPTION "/arch:AVX2")
  set(FMA_COMPILE_OPTION "/fp:fast,")
  set(AVX512_COMPILE_OPTION "/arch:AVX512")
else()
  set(AVX_COMPILE_OPTION "-mavx")
  set(AVX2_COMPILE_OPTION "-mavx2")
  set(FMA_COMPILE_OPTION "-mfma")
  set(AVX512_COMPILE_OPTION "-mavx512f")
endif()

create_python_target(proxsuite_pywrap "" proxsuite ${GENERATE_PYTHON_STUBS})
if(
  BUILD_WITH_VECTORIZATION_SUPPORT
  AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "(x86)|(X86)|(amd64)|(AMD64)"
)
  if(BUILD_BINDINGS_WITH_AVX2_SUPPORT)
    create_python_target(
      proxsuite_pywrap_avx2
      "${AVX2_COMPILE_OPTION};${FMA_COMPILE_OPTION}"
      proxsuite-vectorized
      FALSE
    )
  endif()
  if(BUILD_BINDINGS_WITH_AVX512_SUPPORT)
    create_python_target(
      proxsuite_pywrap_avx512
      "${AVX512_COMPILE_OPTION};${FMA_COMPILE_OPTION}"
      proxsuite-vectorized
      FALSE
    )
  endif()
endif()

# Still relevant ? source_group ?
#add_header_group(PYWRAP_HEADERS)
#add_source_group(PYWRAP_SOURCES)

# --- INSTALL SCRIPTS

# On Windows, we need to enforce the environment variable KMP_DUPLICATE_LIB_OK
# to True to to allow the program to continue to execute with OpenMP support
# if(
#   WIN32
#   AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
#   AND BUILD_WITH_OPENMP_SUPPORT
# )
#   set(
#     OPENMP_KMP_DUPLICATE_LIB_OK_SCRIPT
#     "import os\n"
#     "os.environ[\"KMP_DUPLICATE_LIB_OK\"] = \"1\"\n\n"
#   )

#   if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
#     set(PYTHON_MODULE_DIR "${CMAKE_CURRENT_BINARY_DIR}/proxsuite/$<CONFIG>")
#   else()
#     set(PYTHON_MODULE_DIR "${CMAKE_CURRENT_BINARY_DIR}/proxsuite")
#   endif()

#   set(original_init_dot_py_file ${CMAKE_CURRENT_LIST_DIR}/proxsuite/__init__.py)
#   set(
#     generated_init_dot_py_file
#     ${CMAKE_CURRENT_BINARY_DIR}/proxsuite/__init__.py
#   )
#   set(generated_init_dot_pyc_file ${PYTHON_MODULE_DIR}/__init__.pyc)

#   # Copy content of the __init__.py file
#   file(READ ${original_init_dot_py_file} INIT_CONTENT)
#   # Create a new __init__.py file containing both the content of __init__.py
#   # prepended with the OPENMP_KMP_DUPLICATE_LIB_OK_SCRIPT content
#   file(
#     WRITE
#     ${generated_init_dot_py_file}
#     ${OPENMP_KMP_DUPLICATE_LIB_OK_SCRIPT}
#   )
#   file(APPEND ${generated_init_dot_py_file} ${INIT_CONTENT})

#   python_build_file(
#     ${generated_init_dot_py_file}
#     ${generated_init_dot_pyc_file}
#   )
#   install(
#     FILES "${generated_init_dot_py_file}"
#     DESTINATION ${PYTHON_BINDING_INSTALL_DIR}
#   )
# else()
#   python_build(proxsuite __init__.py)
#   install(
#     FILES "${CMAKE_CURRENT_SOURCE_DIR}/proxsuite/__init__.py"
#     DESTINATION ${PYTHON_BINDING_INSTALL_DIR}
#   )
# endif()

# python_build_get_target(compile_pyc)
# add_dependencies(${PROJECT_NAME}-python ${compile_pyc})

set(PYTHON_FILES torch/__init__.py torch/qplayer.py torch/utils.py)

file(MAKE_DIRECTORY ${PYTHON_BINDING_INSTALL_DIR}/torch)

# foreach(python ${PYTHON_FILES})
#   python_build(proxsuite ${python})
#   get_filename_component(pysubmodule ${python} PATH)
#   get_filename_component(pyname ${python} NAME)
#   set(MODULE_NAME proxsuite/${pysubmodule})
#   python_install_on_site(${MODULE_NAME} ${pyname})
# endforeach(python)
