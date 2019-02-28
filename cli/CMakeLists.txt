cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
include_directories(../include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
find_package(Boost 1.54.0 REQUIRED COMPONENTS log)
add_library(boost INTERFACE IMPORTED)

add_executable(lit lit.cpp rpc.cpp web_rpc.cpp ln_rpc.cpp bc_rpc.cpp channel.cpp)
target_link_libraries(lit ${CURL_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)
