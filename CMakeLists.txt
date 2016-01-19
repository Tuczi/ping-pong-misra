cmake_minimum_required(VERSION 3.3)
project(ping-pong-misra)

set(CMAKE_C_COMPILER mpicc)
set(CMAKE_CXX_COMPILER mpicxx)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#target_link_libraries(main mpi)

set(SOURCE_FILES
        main.cpp ping-pong.cpp ping-pong.hpp)

add_executable(ping-pong ${SOURCE_FILES})
