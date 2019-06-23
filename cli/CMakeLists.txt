cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
include_directories(../include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(lit logger.cpp lit.cpp rpc.cpp web_rpc.cpp ln_rpc.cpp bc_rpc.cpp channel.cpp)
target_link_libraries(lit ${CURL_LIBRARIES} Threads::Threads)
