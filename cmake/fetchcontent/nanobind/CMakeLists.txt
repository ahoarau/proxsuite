FetchContent_Declare(
  nanobind
  GIT_REPOSITORY https://github.com/wjakob/nanobind/
  GIT_TAG v2.9.2
  SYSTEM
  EXCLUDE_FROM_ALL
)
# NOTE: Cannot use the zip tar ball here because nanobind uses submodules...
# This unfortunately adds a git dependency.

FetchContent_MakeAvailable(nanobind)
