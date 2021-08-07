cmake_minimum_required(VERSION 3.13)
project(replication_node)

set(CMAKE_CXX_STANDARD 17)
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

# eRPC-dpdk
set(ERPC-DPDK "${CMAKE_CURRENT_SOURCE_DIR}/eRPC-dpdk")
message(STATUS "eRPC-dpdk path: ${ERPC-DPDK}")

# PMDK
set(PMDK "${CMAKE_CURRENT_SOURCE_DIR}/pmdk")
message(STATUS "PMDK-path: ${PMDK}")

# DPDK
set(DPDK-PATH ${CMAKE_CURRENT_SOURCE_DIR}/dpdk)
message(STATUS "DPDK-path: ${DPDK-PATH}")


# This project source/test directory:
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TESTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# User options:
set(DEBUG "OFF" CACHE STRING "Enable debugging options (ON/OFF)") 
set(TRANSPORT "dpdk" CACHE STRING "Which transport type to use(infiniband/dpdk)") 
set(DPDK_CLUSTER "TRUE" CACHE STRING "Use DPDK-Cluster IPs (TRUE/FALSE)") 
set(CHAIN_NODES "3" CACHE STRING "How many nodes are in the chain (3/4/5)") 

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

string(TOUPPER ${DPDK_CLUSTER} DPDK_CLUSTER)
if (${DPDK_CLUSTER} STREQUAL "TRUE")
  add_compile_definitions(DPDK_CLUSTER)
  message(STATUS "Using DPDK Cluster")
endif()

if (${CHAIN_NODES} STREQUAL "3")
  add_compile_definitions(THREE_NODES)
  message(STATUS "Using a 3-Node Chain")
elseif (${CHAIN_NODES} STREQUAL "4")
  add_compile_definitions(FOUR_NODES)
  message(STATUS "Using a 4-Node Chain")
elseif (${CHAIN_NODES} STREQUAL "5")
  add_compile_definitions(FIVE_NODES)
  message(STATUS "Using a 5-Node Chain")
endif()


if(TRANSPORT STREQUAL "infiniband")
  set(ERPC "${CMAKE_CURRENT_SOURCE_DIR}/eRPC")
  message(STATUS "Compiling with Transport=infiniband")
  add_definitions(-DERPC_INFINIBAND=true)

elseif(TRANSPORT STREQUAL "dpdk") 
  set(ERPC "${CMAKE_CURRENT_SOURCE_DIR}/eRPC-dpdk")
  message(STATUS "Compiling with Transport=DPDK")
  add_definitions(-DERPC_DPDK=true)

  set(DPDK_INCLUDE_DIR ${DPDK-PATH}/include/dpdk)
  message(STATUS "DPDK include directory = ${DPDK_INCLUDE_DIR}")
  include_directories(SYSTEM ${DPDK_INCLUDE_DIR})
  set(DPDK_LIBS 
            -L${DPDK-PATH}/lib -Wl,--whole-archive dpdk -Wl,--no-whole-archive dl)

else()
    message(FATAL_ERROR "Invalid transport: ${TRANSPORT}")
endif()



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
include_directories(${TESTS})

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
  ${SRC}/helperFunctions.cpp
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

target_link_libraries(replication_node
    ${DPDK_LIBS}
)


# Compile benchmark
add_executable(benchmark
  ${BENCHMARK}
)

target_link_libraries(benchmark
  replication_node
)
