cmake_minimum_required(VERSION 3.19)
project(pthreadPool)

set(CMAKE_CXX_STANDARD 14)
include_directories(./pthreadPool.h)
link_directories(pthread)

add_executable(pthreadPool main.cpp pthreadPool.h pthreadPool.cpp)