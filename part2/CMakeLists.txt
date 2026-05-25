cmake_minimum_required(VERSION 3.10)
project(ClientServerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(program task2.cpp)

target_link_libraries(program Threads::Threads)
