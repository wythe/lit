cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
include_directories(../include)
add_executable(lit lit.cpp rpc.cpp)
target_link_libraries(lit ${CURL_LIBRARIES})
