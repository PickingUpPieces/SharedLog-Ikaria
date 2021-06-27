#include "ReplicationManager.h"
#include "rpc.h"
#include "NetworkManager.h"
#include "common_info.h"
#include <iostream>
#include <unistd.h>


ReplicationManager::ReplicationManager(nodeType node, std::string hostname, int port, std::string hostname_successor, int port_successor): 
        node_{node}, softCounter_{0} {
    this->NetworkManager_ = new NetworkManager(hostname, port, hostname_successor, port_successor, this);
}


// TODO: Define header of append()
void ReplicationManager::append(void *reqBuffer, uint64_t reqBufferLength) {
    switch(this->node_) {
        case HEAD: 
        {
            DEBUG_MSG("append()");
            // TODO: Log on this node 
            softCounter_++;
            NetworkManager_->send_message(APPEND, reqBuffer, reqBufferLength);
        }; break;
        case TAIL: {
            DEBUG_MSG("append()");
            softCounter_++;
            // TODO: Log on this node
        }; break;
        case MIDDLE: break;
    }
}

uint64_t ReplicationManager::read(void *reqBuffer, void *respBuffer) {
    // TODO: Find way to return an error over int
    uint64_t respBufferLength = 0;

    switch(this->node_) {
        case HEAD: 
        {
            DEBUG_MSG("read()");
            // TODO: Call send_message with reqBuffer on successor
            NetworkManager_->send_message(READ, reqBuffer, sizeof(uint64_t));
        }; break;
        case TAIL:
        {
            DEBUG_MSG("read()");
            uint64_t logOffset = (uint64_t) &reqBuffer;
            DEBUG_MSG("ReplicationManager.read(" << logOffset << ")");
            // TODO: Do local read
            // TODO: memcpy ret_logData to respBuffer
            sprintf(reinterpret_cast<char *>(respBuffer), "ACK");
            respBufferLength = 4;
        }; break;
        case MIDDLE: break;
    }

    return respBufferLength;
}

int main(int argc, char** argv) {

    // Check which type this node should be
    nodeType node = HEAD;
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
    string message = "Test";
    char *buffer = (char *) malloc(4096);

    while (true) {
        if(counter) {
            localNode->append(&message, 6);
        } else {
            localNode->read(&counter, buffer);
        }
    
    for(int i = 0; i < 10; i++)
        localNode->NetworkManager_->sync_inbound(20);

    ++counter;
    counter %= 2;
    sleep(1);
    }
}
