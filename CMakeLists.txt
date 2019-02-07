cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(lit VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)
add_library(boost INTERFACE IMPORTED)

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)
#include_directories(${CURL_INCLUDE_DIR})

add_subdirectory(cli)
enable_testing()
