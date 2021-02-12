cmake_minimum_required (VERSION 3.13.0)
Project(prpc_test
        VERSION 0.1
        DESCRIPTION "Tests for PicoRPC"
        LANGUAGES CXX)

set (STD_CXX "c++17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(prpc_test prpc.hpp test/prpc.cpp test/catch.cpp test/catch.hpp)
target_include_directories(prpc_test
  PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test>
)
