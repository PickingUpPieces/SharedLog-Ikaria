cmake_minimum_required(VERSION 3.13)

project(Single-Node-Setup)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-Wall -Wextra -Wpedantic -Wsign-conversion -Wunused )
add_definitions(-Wno-unused-function -Wno-deprecated-declarations)
add_definitions(-msse4.1)

# Libraries that are required:
set(LIBRARIES pthread numa dl ibverbs)

# Some paths:
# These two paths might need to be changed:
# eRPC
set(ERPC "${CMAKE_CURRENT_SOURCE_DIR}/eRPC")
message(STATUS "eRPC-path: ${ERPC}")

# PMDK
set(PMDK "${CMAKE_CURRENT_SOURCE_DIR}/pmdk")
message(STATUS "PMDK-path: ${PMDK}")

# This project source directory:
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# User options:
set(DEBUG "OFF" CACHE STRING "Enable debugging options (ON/OFF)") 

string(TOUPPER ${DEBUG} DEBUG)
if (${DEBUG} STREQUAL "ON")
  message(STATUS "Debugging enabled. Compiling with address sanitizer and debug messages")
  add_definitions(-g -fsanitize=address)
  add_compile_definitions(DEBUG)
  link_libraries(-fsanitize=address)
else()
  message(STATUS "Debugging disabled")
  add_definitions(-O2)
endif()

message(STATUS "Compiling with infiniband")
add_definitions(-DERPC_INFINIBAND=true)
link_libraries(erpc)


include_directories(SYSTEM ${PMDK}/lib/include)
link_directories(${PMDK}/lib/lib64)
link_libraries(pmem pmemlog)

include_directories(${PMDK}/src)
include_directories(SYSTEM ${ERPC}/src)
include_directories(${SRC})
include_directories(${SRC}/NetworkLayer)

link_directories(${ERPC}/build)
link_libraries(${LIBRARIES})

set(NETWORK_SOURCE
  ${SRC}/NetworkLayer/Inbound.h
  ${SRC}/NetworkLayer/Inbound.cpp
  ${SRC}/NetworkLayer/Outbound.h
  ${SRC}/NetworkLayer/Outbound.cpp
  ${SRC}/NetworkLayer/NetworkManager.h
  ${SRC}/NetworkLayer/NetworkManager.cpp
  ${SRC}/NetworkLayer/common_networkLayer.h
)

set(REPLICATION_SOURCE
  ${SRC}/ReplicationManager.h
  ${SRC}/ReplicationManager.cpp
  ${SRC}/common_info.h
)

set(LOG_SOURCE
  ${SRC}/Log.h
  ${SRC}/Log.cpp
)  


add_executable(replicationNode
  ${NETWORK_SOURCE}
  ${REPLICATION_SOURCE}
  ${LOG_SOURCE}
)
