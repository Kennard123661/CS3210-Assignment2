cmake_minimum_required(VERSION 3.12)
project(cs3210_mpi_implementation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER mpicxx)

add_executable(mpi_impl main.cpp)

add_executable(simulation_process simulation.cpp)