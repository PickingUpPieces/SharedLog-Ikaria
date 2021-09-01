cmake_minimum_required(VERSION 3.13)
project(replication_node)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-Wall -Wextra -Wpedantic -Wsign-conversion -Wunused -Wimplicit-fallthrough )
add_definitions(-Wno-unused-function -Wno-deprecated-declarations)
add_definitions(-Wno-format -Wno-format-security)
add_definitions(-msse4.1)

# Libraries that are required:
set(LIBRARIES pthread numa dl ibverbs ndctl daxctl)

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
set(BENCHMARKS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)

# User options:
set(DEBUG "OFF" CACHE STRING "Enable debugging options (ON/OFF)") 
set(TRANSPORT "dpdk" CACHE STRING "Which transport type to use(infiniband/dpdk)") 
set(DPDK_CLUSTER "TRUE" CACHE STRING "Use DPDK-Cluster IPs (TRUE/FALSE)") 
set(REPLICATION_PROTOCOL "CR" CACHE STRING "Choose Replication Protocoll (CR/UCR/CRAQ)")
set(BENCHMARK "TRUE" CACHE STRING "Enable benchmark (TRUE/FALSE)")
set(BENCHMARK_LATENCY "FALSE" CACHE STRING "Enable latency benchmarks (TRUE/FALSE)")
set(BENCHMARK_MAX "FALSE" CACHE STRING "Keep max appends inFlight (TRUE/FALSE)")
set(RESET_LOG "TRUE" CACHE STRING "Enable resetting of the log (TRUE/FALSE)")

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

string(TOUPPER ${REPLICATION_PROTOCOL} REPLICATION_PROTOCOL)
if(${REPLICATION_PROTOCOL} STREQUAL "CR")
  add_compile_definitions(CR)
  message(STATUS "Using Chain Replication")
elseif(${REPLICATION_PROTOCOL} STREQUAL "UCR")
  add_compile_definitions(CRAQ)
  add_compile_definitions(UCR)
  message(STATUS "Using Uncommited Chain Replication")
elseif(${REPLICATION_PROTOCOL} STREQUAL "CRAQ")
  add_compile_definitions(CRAQ)
  message(STATUS "Using CRAQ Replication")
else()
  message(FATAL_ERROR "Invalid Replication Protocol")
endif()


string(TOUPPER ${BENCHMARK} BENCHMARK)
if (${BENCHMARK} STREQUAL "TRUE")
  add_compile_definitions(BENCHMARK)
  message(STATUS "Benchmarking enabled")
endif()

string(TOUPPER ${BENCHMARK_LATENCY} BENCHMARK_LATENCY)
if (${BENCHMARK_LATENCY} STREQUAL "TRUE")
  add_compile_definitions(LATENCY)
  message(STATUS "Latency benchmarking enabled")
endif()

string(TOUPPER ${BENCHMARK_MAX} BENCHMARK_MAX)
if (${BENCHMARK_MAX} STREQUAL "TRUE")
  add_compile_definitions(BENCHMARK_MAX)
  message(STATUS "Maximum benchmark enabled")
endif()

string(TOUPPER ${RESET_LOG} RESET_LOG)
if (${RESET_LOG} STREQUAL "TRUE")
  add_compile_definitions(RESET_LOG)
  message(STATUS "Resetting of the log sequencer enabled")
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
include_directories(${BENCHMARKS})

link_libraries(${LIBRARIES})

set(NETWORK_SOURCE
  ${SRC}/NetworkLayer/Inbound.h
  ${SRC}/NetworkLayer/Inbound.cpp
  ${SRC}/NetworkLayer/Outbound.h
  ${SRC}/NetworkLayer/Outbound.cpp
  ${SRC}/NetworkLayer/NetworkManager.h
  ${SRC}/NetworkLayer/NetworkManager.cpp
)


if(${REPLICATION_PROTOCOL} STREQUAL "CR")
set(REPLICATION_SOURCE
  ${SRC}/CRReplication.h
  ${SRC}/CRReplication.cpp
  ${SRC}/helperFunctions.cpp
)
elseif(${REPLICATION_PROTOCOL} STREQUAL "CRAQ" OR ${REPLICATION_PROTOCOL} STREQUAL "UCR")
set(REPLICATION_SOURCE
  ${SRC}/CRAQReplication.h
  ${SRC}/CRAQReplication.cpp
  ${SRC}/helperFunctions.cpp
)
endif()

set(SHAREDLOGNODE_SOURCE
  ${SRC}/SharedLogNode.h
)

set(LOG_SOURCE
  ${SRC}/Log.h
  ${SRC}/Log.cpp
)  

set(BENCHMARK_PROGRAM
  ${BENCHMARKS}/benchmark.cpp
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
  ${BENCHMARK_PROGRAM}
)

target_link_libraries(benchmark
  replication_node
)
