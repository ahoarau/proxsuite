FetchContent_Declare(
  eigen
  URL https://gitlab.com/libeigen/eigen/-/archive/5.0.0/eigen-5.0.0.zip
  URL_HASH MD5=3804ac4802bb18a75874d7a165c5c6f3
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(eigen)
