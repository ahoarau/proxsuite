# gersemi: off
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)
set(BUILD_DOC                   OFF CACHE BOOL "" FORCE)
set(BUILD_SANDBOX               OFF CACHE BOOL "" FORCE)
set(BUILD_TEST                  OFF CACHE BOOL "" FORCE)
set(CEREAL_INSTALL              OFF CACHE BOOL "" FORCE)
# gersemi: on

FetchContent_Declare(
  cereal
  URL https://github.com/USCiLab/cereal/archive/refs/tags/v1.3.2.zip
  URL_HASH MD5=953a872cd3d78abf2e29212987a0ba71
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cereal)
