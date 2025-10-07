# gersemi: off

FetchContent_Declare(
    zlib

    URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib131.zip
    URL_HASH MD5=ef76f7ebfd97778a6653b1d8413541c0
    SOURCE_SUBDIR extract_only
    SYSTEM
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(zlib)

FetchContent_Declare(
  hdf5-raw
  URL https://github.com/HDFGroup/hdf5/releases/download/hdf5_1.14.6/hdf5-1.14.6-win-vs2022_cl.zip
  URL_HASH MD5=e6fc0506732606e922e7f3f58015ae5a
)
FetchContent_MakeAvailable(hdf5-raw)

FetchContent_Declare(
  hdf5
  URL ${hdf5-raw_SOURCE_DIR}/HDF5-1.14.6-win64.zip
  URL_HASH MD5=a4229a651ff6245addf825cf95d0c734
)
FetchContent_MakeAvailable(hdf5)

set(CMAKE_PREFIX_PATH ${hdf5_SOURCE_DIR})

set(MATIO_WITH_ZLIB       ON  CACHE BOOL "" FORCE)
set(MATIO_WITH_HDF5       ON  CACHE BOOL "" FORCE)
set(HDF5_USE_STATIC_LIBRARIES       ON  CACHE BOOL "" FORCE)
set(MATIO_MAT73           OFF CACHE BOOL "" FORCE)
set(MATIO_BUILD_TESTING   OFF CACHE BOOL "" FORCE)
set(MATIO_SHARED          OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  matio
  URL https://github.com/tbeu/matio/archive/f75ab919972cfa39813354368bd0c7eb7a01f871.zip
  URL_HASH MD5=93efce73fa3327a8e3c3a848abd32d38
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(matio)
target_include_directories(matio PRIVATE ${zlib_SOURCE_DIR})

# gersemi: on
