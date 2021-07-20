cmake_minimum_required(VERSION 3.13)
project(replication_node)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-Wall -Wextra -Wpedantic -Wsign-conversion -Wunused)
add_definitions(-Wno-unused-function -Wno-deprecated-declarations)
add_definitions(-Wno-format -Wno-format-security)
add_definitions(-msse4.1)

# Libraries that are required:
set(LIBRARIES pthread numa dl ibverbs)

# These paths might need to be changed:
# eRPC
set(ERPC "${CMAKE_CURRENT_SOURCE_DIR}/eRPC")
message(STATUS "eRPC-path: ${ERPC}")

# PMDK
set(PMDK "${CMAKE_CURRENT_SOURCE_DIR}/pmdk")
message(STATUS "PMDK-path: ${PMDK}")

# This project source/test directory:
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TESTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# User options:
set(DEBUG "OFF" CACHE STRING "Enable debugging options (ON/OFF)") 

string(TOUPPER ${DEBUG} DEBUG)
if (${DEBUG} STREQUAL "ON")
  message(STATUS "Debugging enabled. Compiling with address sanitizer and debug messages")
  add_compile_definitions(DEBUG)
  add_definitions(-g -fsanitize=address)
  link_libraries(-fsanitize=address)
else()
  message(STATUS "Debugging disabled")
  add_definitions(-O2)
endif()

# Adds IDs to log entries
#add_compile_definitions(TESTING)

message(STATUS "Compiling with infiniband")
add_definitions(-DERPC_INFINIBAND=true)

# PMDK
include_directories(SYSTEM ${PMDK}/lib/include)
include_directories(SYSTEM ${PMDK}/src)
link_directories(${PMDK}/lib/lib64)
link_libraries(pmem pmemlog)

# eRPC
include_directories(SYSTEM ${ERPC}/src)
link_directories(${ERPC}/build)
link_libraries(erpc)

include_directories(${SRC})
include_directories(${SRC}/NetworkLayer)

link_libraries(${LIBRARIES})

set(NETWORK_SOURCE
  ${SRC}/NetworkLayer/NetworkManager.h
  ${SRC}/NetworkLayer/NetworkManager.cpp
  ${SRC}/NetworkLayer/Inbound.h
  ${SRC}/NetworkLayer/Inbound.cpp
  ${SRC}/NetworkLayer/Outbound.h
  ${SRC}/NetworkLayer/Outbound.cpp
)

set(REPLICATION_SOURCE
  ${SRC}/ReplicationManager.h
  ${SRC}/ReplicationManager.cpp
)

set(SHAREDLOGNODE_SOURCE
  ${SRC}/SharedLogNode.h
  ${SRC}/SharedLogNode.cpp
)

set(LOG_SOURCE
  ${SRC}/Log.h
  ${SRC}/Log.cpp
)  

set(TEST_UTILS 
  ${TESTS}/run_node.cpp 
)

set(BENCHMARK
  ${TESTS}/benchmark.cpp
)

# Compile as static library
add_library(replication_node STATIC
  ${NETWORK_SOURCE}
  ${REPLICATION_SOURCE}
  ${SHAREDLOGNODE_SOURCE}
  ${LOG_SOURCE}
)

# Compile benchmark
add_executable(benchmark
  ${BENCHMARK}
)

target_link_libraries(benchmark
  replication_node
)


# Compile run_node test
add_executable(run_node
  ${TEST_UTILS}
)


target_link_libraries(run_node
  replication_node
)
