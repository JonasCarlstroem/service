cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(service INTERFACE)

set(LIB_TARGETS service PARENT_SCOPE)