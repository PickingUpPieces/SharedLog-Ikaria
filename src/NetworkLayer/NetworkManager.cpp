#include <iostream>
#include "NetworkManager.h"

void empty_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

NetworkManager::NetworkManager(string hostURI, string headURI, string successorURI, string tailURI, ReplicationManager *ReplicationManager):
        ReplicationManager_{ReplicationManager},
        nodeReady_{false},
        chainReady_{false},
        initMessage_{nullptr},
        Nexus_{hostURI, 0, 0},
        Inbound_{new Inbound(&Nexus_, this)},
        Head_{nullptr},
        Successor_{nullptr},
        Tail_{nullptr},
        rpc_{&Nexus_, this, 0, empty_sm_handler, 0}
{
    rpc_.retry_connect_on_invalid_rpc_id = true;

    if (!headURI.empty())
        Head_ = new Outbound(headURI, this, &rpc_);

    if (!tailURI.empty())
        Tail_ = new Outbound(tailURI, this, &rpc_);
    
    if (successorURI.empty())
        /* This node is the tail node */
        Successor_ = nullptr;
    else if (successorURI.compare(tailURI) == 0)
        Successor_ = Tail_;
    else
        Successor_ = new Outbound(successorURI, this, &rpc_);
}

void NetworkManager::send_message(NodeType targetNode, Message *message) {
    switch (targetNode)
    {
    case HEAD: Head_->send_message(message); break;
    case SUCCESSOR: Successor_->send_message(message); break;
    case TAIL: Tail_->send_message(message); break;
    }
}

void NetworkManager::receive_message(Message *message) {
    switch (message->messageType) {
        case INIT: 
        {
            if (nodeReady_) {
                if (Successor_)
                    Successor_->send_message(initMessage_);
                else
                    Inbound_->send_response(initMessage_);
            } else
                initMessage_ = message;
            break;
        }
        case READ: 
        {
            chainReady_ = true;
            ReplicationManager_->read(message); 
            break;
        }
        case APPEND: 
        {
            chainReady_ = true; 
            ReplicationManager_->append(message); 
            break;
        }
    }
}

void NetworkManager::receive_response(Message *message) {
    if (message->sentByThisNode) {
        ReplicationManager_->rec(message);
        return;
    }
    
    if (Head_)
        Inbound_->send_response(message);
    else
        chainReady_ = true;
}

void NetworkManager::sync(int numberOfRuns) {
    for (int i = 0; i < numberOfRuns; i++)
        rpc_.run_event_loop_once();
}

void NetworkManager::connect() {
    if (Head_) Head_->connect();
    if (Successor_) Successor_->connect();
    if (Tail_) Tail_->connect();
    nodeReady_ = true;

    /* Check if INIT has already been received */
    if (initMessage_) {
        if (Successor_)
            Successor_->send_message(initMessage_);
        else
            Inbound_->send_response(initMessage_);
    } 
    else if (!Head_)
    /* Check if node is head and send INIT message */
        send_init_message();
}

void NetworkManager::send_init_message() {
    initMessage_ = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer reqBuffer = rpc_.alloc_msg_buffer_or_die(1);

    initMessage_->messageType = INIT;
    initMessage_->reqBuffer = &reqBuffer;
    initMessage_->reqBufferSize = 1;
    initMessage_->respBuffer = rpc_.alloc_msg_buffer_or_die(1);
    initMessage_->respBufferSize = 1;

    Successor_->send_message(initMessage_);
}

void NetworkManager::wait_for_init() {
    while(!chainReady_)
        rpc_.run_event_loop_once();
}