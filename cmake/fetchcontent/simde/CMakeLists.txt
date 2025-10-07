FetchContent_Declare(
  simde
  URL
    https://github.com/simd-everywhere/simde/releases/download/v0.8.2/simde-amalgamated-0.8.2.tar.xz
  URL_HASH MD5=57239a45e0cc7ceccb20b3b69cae6db5
  SOURCE_DIR
  ${FETCHCONTENT_BASE_DIR}/simde-src/simde
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(simde)
add_library(simde INTERFACE)
target_include_directories(
  simde
  INTERFACE $<BUILD_INTERFACE:${FETCHCONTENT_BASE_DIR}/simde-src>
)
#NOTE: we change the base dir with SOURCE_DIR to match the simde installed with apt-get (simde prefix)

############################# TO BE REMOVED FROM HERE #############################
if(UNIX)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-march=native COMPILER_SUPPORTS_MARCH_NATIVE)
  if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "[proxsuite] Compiler supports -march=native")
    target_compile_options(simde INTERFACE -march=native) # Add -march=native if supported by the compiler
  else()
    message(STATUS "[proxsuite] Compiler does not support -march=native")
  endif()
endif()
