#include "ReplicationManager.h"
#include "rpc.h"
#include "NetworkManager.h"
#include "common_info.h"
#include <iostream>
#include <unistd.h>


ReplicationManager::ReplicationManager(NodeType NodeType, std::string hostname, int port, std::string hostnameSuccessor, int portSuccessor): 
        NodeType_{NodeType}, 
        softCounter_{0},
        Log_{POOL_SIZE, LOG_BLOCK_SIZE, POOL_PATH} 
{
    this->NetworkManager_ = new NetworkManager(hostname, port, hostnameSuccessor, portSuccessor, this);
}


void ReplicationManager::append(void *reqBuffer, uint64_t reqBufferLength) {
    switch(NodeType_) {
        case HEAD: 
        {
            DEBUG_MSG("ReplicationManager.append()");
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) reqBuffer;
            /* Set logOffset */
            ++softCounter_;
            logEntryInFlight->logOffset = softCounter_;
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Send append to next node in chain */
            NetworkManager_->send_message(APPEND, reqBuffer, reqBufferLength);
        }; break;
        case TAIL: {
            DEBUG_MSG("ReplicationManager.append()");
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) reqBuffer;
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
        }; break;
        case MIDDLE: break;
    }
}

int ReplicationManager::read(void *reqBuffer, void *respBuffer) {
    int respBufferLength = -1;

    switch(NodeType_) {
        case HEAD: 
        {
            DEBUG_MSG("ReplicationManager.read()");
            NetworkManager_->send_message(READ, reqBuffer, sizeof(uint64_t));
        }; break;
        case TAIL:
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) reqBuffer;
            DEBUG_MSG("ReplicationManager.read(" << std::to_string(logEntryInFlight->logOffset) << ")");
            void *data = Log_.read(logEntryInFlight->logOffset, &respBufferLength);
            // TODO: Check respBufferLength
            memcpy(respBuffer, data, respBufferLength);
        }; break;
        case MIDDLE: break;
    }

    return respBufferLength;
}

int main(int argc, char** argv) {

    struct LogEntry
    {
        uint64_t dataLength;
        char data[LOG_BLOCK_SIZE];
    };

    // Check which type this node should be
    NodeType node = HEAD;
    if ( argc == 2 ) { 
        std::string cmd_arg(argv[1]);

        if ( cmd_arg.compare("head") == 0 ) {
            node = HEAD;
        } else if ( cmd_arg.compare("tail") == 0 ) {
            node = TAIL;
        }
    }
    DEBUG_MSG("This node is: " << node);
    ReplicationManager *localNode{nullptr};

    switch(node) {
        case HEAD: localNode = new ReplicationManager(node, hostname_head, port_head, hostname_tail, port_tail); break;
        case TAIL: localNode = new ReplicationManager(node, hostname_tail, port_tail, std::string(), -1 ); break;
        case MIDDLE: break;
    }

    DEBUG_MSG("Start testing...");

    uint64_t counter{0};
    uint64_t readCounter{0};
    char message[5]{"Test"};
    char *buffer = (char *) malloc(4096);

    while (true) {

        LogEntryInFlight logEntryInFlight{counter, { 5, *message}};
        DEBUG_MSG("main.LogEntryInFlight.logOffset: " << std::to_string(counter) " ; LogEntryInFlight.dataLength: " << std::to_string(logEntryInFlight.logEntry.dataLength) << " ; main.LogEntryInFlight.data: " << logEntryInFlight.logEntry.data);
        if(counter) {
            localNode->append(&logEntryInFlight, sizeof(logEntryInFlight));
        } else {
            localNode->read(&readCounter, buffer);
            ++readCounter;
        }
    
    for(int i = 0; i < 10; i++)
        localNode->NetworkManager_->sync_inbound(20);

    ++counter;
    counter %= 2;
    sleep(1);
    }
}
