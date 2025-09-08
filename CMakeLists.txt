cmake_minimum_required(VERSION 3.22)

# Options
option(BUILD_EXECUTABLE "Build an executable binary " ON)

set(CMAKE_CXX_STANDARD 20)

set(VCPKG_ROOT "vcpkg")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/modules")

project(latchy VERSION 0.0.1)
add_executable(${CMAKE_PROJECT_NAME})

include (jose.cmake)

find_package(absl CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(Botan CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL CONFIG REQUIRED)

set(CMAKE_SIZEOF_VOID_P "")   # Required to avoid a failure with jansson when check for 32/64 bitness. Hopefully this is not impactful
find_package(jansson REQUIRED)

set (LAT_GLOBAL_INCL)
add_subdirectory(models)
include_directories(${LAT_GLOBAL_INCL})

add_subdirectory(src)

set(BUILD_EXECUTABLE OFF)
add_subdirectory(clevisLib/src)


target_link_libraries(${CMAKE_PROJECT_NAME}
  models
  Botan::Botan-static
  CURL::libcurl
  ${JOSE_INCLUDE_OBJECT}
  libjose_static
  protobuf::libprotobuf
  ${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libjansson.a
  ZLIB::ZLIB
  OpenSSL::Crypto
)


