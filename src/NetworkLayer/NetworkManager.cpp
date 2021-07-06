#include <iostream>
#include "NetworkManager.h"

void empty_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

NetworkManager::NetworkManager(string hostURI, string headURI, string successorURI, string tailURI, ReplicationManager *ReplicationManager):
        ReplicationManager_{ReplicationManager},
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


void NetworkManager::init() {
    if (Head_) Head_->connect();
    if (Successor_) Successor_->connect();
    if (Tail_ && (Tail_ != Successor_)) Tail_->connect();
}

void NetworkManager::send_message(NodeType targetNode, Message *message) {
    messagesInFlight_++;
    DEBUG_MSG("NetworkManager.send_message(messagesInFlight: " << std::to_string(messagesInFlight_) << ")");

    switch (targetNode)
    {
    case HEAD: 
        Head_->send_message(message); 
        break;
    case SUCCESSOR: 
        Successor_->send_message(message); 
        break;
    case TAIL: 
        Tail_->send_message(message); 
        break;
    }
}

void NetworkManager::receive_message(Message *message) {
    messagesInFlight_--;
    totalMessagesCompleted_++;
    DEBUG_MSG("NetworkManager.receive_message(messagesInFlight: " << std::to_string(messagesInFlight_) << " ; totalMessagesCompleted: " << std::to_string(totalMessagesCompleted_) << ")");

    switch (message->messageType) 
    {
        case SETUP: 
            ReplicationManager_->setup(message); 
            break;
        case READ: 
            ReplicationManager_->read(message); 
            break;
        case APPEND: 
            ReplicationManager_->append(message); 
            break;
    }
}

void NetworkManager::receive_response(Message *message) {
    if (message->sentByThisNode) {
        ReplicationManager_->rec(message);
        return;
    }

    switch (message->messageType)
    {
    case SETUP:
        ReplicationManager_->setup_response();
	break;
    default:
        Inbound_->send_response(message);
        break;
    }
}

void NetworkManager::sync(int numberOfRuns) {
    for (int i = 0; i < numberOfRuns; i++)
        rpc_.run_event_loop_once();
}
