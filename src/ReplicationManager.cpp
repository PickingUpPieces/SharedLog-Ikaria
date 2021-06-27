#include "ReplicationManager.h"
#include "rpc.h"
#include "NetworkManager.h"
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
            cout << "append()" << endl;
            // TODO: Log on this node 
            softCounter_++;
            NetworkManager_->send_message(APPEND, reqBuffer, reqBufferLength);
        }; break;
        case TAIL: {
            cout << "append()" << endl;
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
            cout << "read()" << endl;
            // TODO: Call send_message with reqBuffer on successor
            NetworkManager_->send_message(READ, reqBuffer, 8);
        }; break;
        case TAIL:
        {
            cout << "read()" << endl;
            // TODO: Cast reqBuffer to uint64_t
            // TODO: Do local read
            // TODO: memcpy ret_logData to respBuffer
            memcpy(respBuffer, "Fake", 5);
            respBufferLength = 5;
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
    std::cout << "This node is: " << node << endl;
    ReplicationManager *localNode{nullptr};

    switch(node) {
        case HEAD: localNode = new ReplicationManager(node, hostname_head, port_head, hostname_tail, port_tail); break;
        case TAIL: localNode = new ReplicationManager(node, hostname_tail, port_tail, std::string(), -1 ); break;
        case MIDDLE: break;
    }

    int counter = 0;
    string message = "Test";
    char buffer[128]{0};

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
