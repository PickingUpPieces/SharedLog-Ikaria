#ifndef REPLICATIONNODE_COMMON_INFO_H
#define REPLICATIONNODE_COMMON_INFO_H

#include <string>
#include <iostream>
#include "rpc.h"

#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

/* Max message size */
#define MAX_MESSAGE_SIZE 4112

// Log variables
/* size of the pmemlog pool -- 1 GB = 2^30 */
#define POOL_SIZE ((off_t)(1UL << 33))
/* log data size in B */
#define LOG_BLOCK_DATA_SIZE 4096
/* log block size in B */
#define LOG_BLOCK_TOTAL_SIZE sizeof(LogEntry)
/* Path to the Pool file */
#define POOL_PATH "/home/vincent/pmem/log-test-0.log"

struct LogEntry {
    uint64_t dataLength;
    char data[LOG_BLOCK_DATA_SIZE];
};

enum MessageType {
    READ = 2,
    APPEND = 3,
    SETUP = 4
};

struct LogEntryInFlight {
    uint64_t logOffset;
    MessageType messageType;
    LogEntry logEntry;
};

enum NodeType {
    HEAD,
    MIDDLE,
    SUCCESSOR = 1, // Refers to the next node in the chain
    TAIL
};

struct Message {
    MessageType messageType;
    bool sentByThisNode{false};
    uint64_t logOffset{0};
    erpc::ReqHandle *reqHandle{nullptr};
    erpc::MsgBuffer reqBuffer;
    size_t reqBufferSize{0};
    erpc::MsgBuffer respBuffer;
    size_t respBufferSize{0};
};

#endif // REPLICATIONNODE_COMMON_INFO_H
