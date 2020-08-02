cmake_minimum_required (VERSION 3.13.0)
Project(urpc
        VERSION 0.1
        DESCRIPTION "transport-agnostic micro-RPC for C++"
        LANGUAGES CXX)

set (STD_CXX "c++17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(prpc_test prpc.hpp prpc_test.cpp catch/catch.cpp catch/catch.hpp)
target_include_directories(prpc_test
  PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/catch/)