#include "NetworkManager.h"
#include "common_networkLayer.h"
#include "ReplicationManager.h"
#include <iostream>

void empty_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

NetworkManager::NetworkManager(string inboundHostname, int inboundPort, string outboundHostname, int outboundPort, ReplicationManager *ReplicationManager) {

    string inboundURI = inboundHostname + ":" + std::to_string(inboundPort);
    nexus_ = new erpc::Nexus(inboundURI, 0, 0);
    Inbound_ = new Inbound(nexus_, 0, this);
    ReplicationManager_ = ReplicationManager;

    if (!outboundHostname.empty()) {
        string outboundURI = outboundHostname + ":" + std::to_string(outboundPort);
        DEBUG_MSG("NetworkManager.outboundURI: " << outboundURI);
        Outbound_ = new Outbound(nexus_, 1, outboundURI, this);
    }

    DEBUG_MSG("NetworkManager.inboundURI: " << inboundURI);
}

void NetworkManager::send_message(Message *message) {
    Outbound_->send_message(message);
}

void NetworkManager::sync_inbound(int numberOfRuns) {
    Inbound_->run_event_loop(numberOfRuns);
}

void NetworkManager::receive_message(Message *message) {
    switch (message->messageType)
    {
    case READ:
        ReplicationManager_->read(message);
        break;
    case APPEND:
        ReplicationManager_->append(message);
        break;
    }
}

void NetworkManager::receive_response(Message *message) {
    Inbound_->send_response(message);
}

void NetworkManager::terminate() {}
