cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(lit VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)

add_subdirectory(cli)
enable_testing()
