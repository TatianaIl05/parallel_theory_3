cmake_minimum_required(VERSION 3.10)
project(MatrixVector)

set(CMAKE_CXX_STANDARD 17)
add_executable(matrix_vector main.cpp)
target_link_libraries(matrix_vector pthread)
