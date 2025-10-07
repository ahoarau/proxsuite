FetchContent_Declare(
  catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.10.0.zip
  URL_HASH MD5=3da0c8767d69b07a2682bdef71edc658
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(catch2)
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
