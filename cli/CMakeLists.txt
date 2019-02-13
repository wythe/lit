cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
include_directories(../include)
add_executable(lit lit.cpp rpc.cpp web_rpc.cpp ln_rpc.cpp bc_rpc.cpp node.cpp)
target_link_libraries(lit ${CURL_LIBRARIES})
