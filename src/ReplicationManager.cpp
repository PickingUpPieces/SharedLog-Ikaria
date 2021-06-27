#include "ReplicationManager.h"
#include "rpc.h"
#include "NetworkManager.h"
#include "common_info.h"
#include <iostream>
#include <unistd.h>


ReplicationManager::ReplicationManager(NodeType NodeType, std::string hostname, int port, std::string hostnameSuccessor, int portSuccessor): 
        NodeType_{NodeType}, 
        Log_{POOL_SIZE, LOG_BLOCK_SIZE, POOL_PATH} 
{
    this->NetworkManager_ = new NetworkManager(hostname, port, hostnameSuccessor, portSuccessor, this);
}


void ReplicationManager::append(Message *message) {
    switch(NodeType_) {
        case HEAD: 
        {
            DEBUG_MSG("ReplicationManager.append()");
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            /* Set logOffset */
            ++softCounter_;
            logEntryInFlight->logOffset = softCounter_;
            message->logOffset = softCounter_;
            /* Append the log entry to the local log */
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
            /* Send append to next node in chain */
            NetworkManager_->send_message(message);
        }; break;
        case MIDDLE: break;
        case TAIL: {
            DEBUG_MSG("ReplicationManager.append()");
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            Log_.append(logEntryInFlight->logOffset, &logEntryInFlight->logEntry);
        };
    }
}

void ReplicationManager::read(Message *message) {
    size_t respBufferSize = 0;

    switch(NodeType_) {
        case MIDDLE: ;
        case HEAD: 
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            DEBUG_MSG("ReplicationManager.read()");
            /* Send READ request to next node in chain, to get the answer from the tail */
            // FIXME: Maybe send request directly to the tail
            NetworkManager_->send_message(message);
        }; break;
        case TAIL:
        {
            LogEntryInFlight *logEntryInFlight = (LogEntryInFlight *) message->reqBuffer->buf;
            DEBUG_MSG("ReplicationManager.read(" << std::to_string(logEntryInFlight->logOffset) << ")");
            void *logEntry = Log_.read(logEntryInFlight->logOffset, &respBufferSize);
            // TODO: Check respBufferSize == 0, if read was successful
            message->respBufferSize = respBufferSize;
            memcpy(message->respBuffer->buf, logEntry, respBufferSize);
            /* Send READ response */
            NetworkManager_->receive_response(message);
        }; 
    }
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

    // TODO: Fix message buffer; In need of real MsgBuffer so buffer.buf works
    Message message;
    message.reqBuffer = (erpc::MsgBuffer *) malloc(4096);
    message.reqBufferSize = 4096;
    message.respBuffer = (erpc::MsgBuffer *) malloc(4096);
    message.respBufferSize = 4096;

    while (true) {

        if(counter) {
            localNode->read(&message);
        } else {
            LogEntryInFlight logEntryInFlight{counter, { 5, "Test"}};
            DEBUG_MSG("main.LogEntryInFlight.logOffset: " << std::to_string(counter) << " ; LogEntryInFlight.dataLength: " << std::to_string(logEntryInFlight.logEntry.dataLength) << " ; main.LogEntryInFlight.data: " << logEntryInFlight.logEntry.data);
            localNode->append(&message);
        }
    
    for(int i = 0; i < 10; i++)
        localNode->NetworkManager_->sync_inbound(20);

    ++counter;
    counter %= 2;
    sleep(1);
    }
}
