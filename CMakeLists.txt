cmake_minimum_required(VERSION 2.8.12)

project(codevs5 CXX)

ENABLE_LANGUAGE(C)

set(TARGET ${PROJECT_NAME})

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

if (WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall /W2 /EHsc")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /MDd /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /MD")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
  CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
  
  if(COMPILER_SUPPORTS_CXX14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  elseif(COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall -Wno-long-long -pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
  CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
  
  if(COMPILER_SUPPORTS_CXX114)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
  elseif(COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -stdlib=libc++")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

endif()

file(GLOB CXX_SOURCE_FILES ${CMAKE_SOURCE_DIR}/*.cpp)

add_executable(${TARGET} ${CXX_SOURCE_FILES})
