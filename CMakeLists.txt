#
# Copyright 2017 Segment Inc. <friends@segment.com>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.
#

cmake_minimum_required (VERSION 3.1)
project(analytics-cpp CXX)

# We require C++ 11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIED on)
set(CMAKE_CXX_EXTENSIONS off)

enable_testing()

set(SOURCES analytics.cpp analytics.h
    http-curl.cpp http-curl.hpp
    http-none.hpp)

include(FindCURL)

# Dynamic library
add_library(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})

# Static library
add_library(${PROJECT_NAME}_static STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME}_static ${CURL_LIBRARIES})

if(NOT CURL_FOUND)
    message(FATAL_ERROR "Cannot find libcurl")
endif()

include_directories(AFTER SYSTEM ${CURL_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})

add_subdirectory(tests)
