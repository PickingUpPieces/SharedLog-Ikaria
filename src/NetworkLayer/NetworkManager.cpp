#include <iostream>
#include "NetworkManager.h"

void empty_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

NetworkManager::NetworkManager(string inboundHostname, int inboundPort, string outboundHostname, int outboundPort, ReplicationManager *ReplicationManager) {
    string inboundURI = inboundHostname + ":" + std::to_string(inboundPort);
    nexus_ = new erpc::Nexus(inboundURI, 0, 0);
    Inbound_ = new Inbound(nexus_, this);
    rpc_ = new erpc::Rpc<erpc::CTransport>(nexus_, this, 0, empty_sm_handler, 0);
    Inbound_->set_rpc(rpc_);
    ReplicationManager_ = ReplicationManager;

    if (!outboundHostname.empty()) {
        string outboundURI = outboundHostname + ":" + std::to_string(outboundPort);
        DEBUG_MSG("NetworkManager.outboundURI: " << outboundURI);
        Outbound_ = new Outbound(outboundURI, this, rpc_);
    }

    DEBUG_MSG("NetworkManager.inboundURI: " << inboundURI);
}

void NetworkManager::send_message(Message *message) {
    Outbound_->send_message(message);
}

void NetworkManager::receive_message(Message *message) {
    switch (message->messageType) {
    case READ:
        ReplicationManager_->read(message);
        break;
    case APPEND:
        ReplicationManager_->append(message);
        break;
    }
}

void NetworkManager::receive_response(Message *message) {
    if( message->sentByThisNode)
        ReplicationManager_->rec(message);
    else
        Inbound_->send_response(message);
}

void NetworkManager::sync(int numberOfRuns) {
    for (int i = 0; i < numberOfRuns; i++)
        rpc_->run_event_loop_once();
}

void NetworkManager::terminate() {}