cmake_minimum_required(VERSION 3.14.5)
project(abcd LANGUAGES CXX)

if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif()

## Enforce c++17 usage when building catch2
set(CMAKE_CXX_STANDARD 17 CACHE INTERNAL "CMAKE_CXX_STANDARD")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE INTERNAL "CMAKE_CXX_STANDARD_REQUIRED") # c++17 is required
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "CMAKE_CXX_EXTENSIONS") # use -std=c++* instead of -std=gnu++*

## Superbuild placeholder

## Enforce out-of-tree build
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

## Sanitizers on debug mode
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address")

## Force colored output in c compilers
## https://medium.com/@alasher/colored-c-compiler-output-with-ninja-clang-gcc-10bfe7f2b949
if(NOT ALREADY_FORCED_COLORED_OUTPUT)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always" CACHE INTERNAL "CMAKE_CXX_FLAGS")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" CACHE INTERNAL "CMAKE_CXX_FLAGS")
  endif()
endif()
set(ALREADY_FORCED_COLORED_OUTPUT TRUE CACHE INTERNAL "ALREADY_FORCED_COLORED_OUTPUT")

file(GLOB srcs LIST_DIRECTORIES true CONFIGURE_DEPENDS *.cc)
add_executable(run ${srcs})

include(FetchContent)
FetchContent_Declare(libarchive URL https://github.com/libarchive/libarchive/releases/download/v3.6.1/libarchive-3.6.1.tar.xz)
FetchContent_GetProperties(libarchive)
if(NOT libarchive_POPULATED)
  FetchContent_Populate(libarchive)

  SET(ENABLE_MBEDTLS false)
  SET(ENABLE_NETTLE false)
  SET(ENABLE_OPENSSL false)
  SET(ENABLE_LIBB2 false)
  SET(ENABLE_LZ4 false)
  SET(ENABLE_LZO false)
  SET(ENABLE_LZMA false)

  SET(ENABLE_ZSTD false)
  SET(ENABLE_ZLIB false)
  SET(ENABLE_BZip2 false)
  SET(ENABLE_LIBXML2 false)
  SET(ENABLE_EXPAT false)
  SET(ENABLE_CNG false)

  SET(ENABLE_TAR false)
  SET(ENABLE_TAR_SHARED false)
  SET(ENABLE_CPIO false)
  SET(ENABLE_CPIO_SHARED false)
  SET(ENABLE_CAT false)
  SET(ENABLE_CAT_SHARED false)
  SET(ENABLE_XATTR false)
  SET(ENABLE_ACL false)
  SET(ENABLE_TEST false)
  SET(ENABLE_COVERAGE false)
  SET(ENABLE_INSTALL false)

  add_subdirectory(${libarchive_SOURCE_DIR} ${minizip_BINARY_DIR})
  #target_include_directories(run PRIVATE ${libarchive_SOURCE_DIR}/lib)
endif()
target_link_libraries(run PRIVATE archive_static)


FetchContent_Declare(zstd URL https://github.com/facebook/zstd/releases/download/v1.5.2/zstd-1.5.2.tar.gz)
FetchContent_GetProperties(zstd)
if(NOT zstd_POPULATED)
  FetchContent_Populate(zstd)

  set(ZSTD_BUILD_PROGRAMS false)
  set(ZSTD_BUILD_SHARED false)

  add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
  target_include_directories(run PRIVATE ${zstd_SOURCE_DIR}/lib)
endif()
target_link_libraries(run PRIVATE libzstd_static)

