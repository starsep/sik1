cmake_minimum_required(VERSION 2.8.4)
project(sik1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(sik1 COMMAND make -C ${sik1_SOURCE_DIR})
