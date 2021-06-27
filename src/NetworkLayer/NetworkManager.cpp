#include "NetworkManager.h"
#include "common_networkLayer.h"
#include "ReplicationManager.h"
#include <iostream>

void empty_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

NetworkManager::NetworkManager(string inboundHostname, int inboundPort, string outboundHostname, int outboundPort, ReplicationManager *ReplicationManager) {

    std::string inboundURI = inboundHostname + ":" + std::to_string(inboundPort);
    this->nexus_ = new erpc::Nexus(inboundURI, 0, 0);
    this->Inbound_ = new Inbound(nexus_, 0, ReplicationManager);

    if (!outboundHostname.empty()) {
        std::string outboundURI = outboundHostname + ":" + std::to_string(outboundPort);
        DEBUG_MSG("NetworkManager(): outboundURI " << outboundURI);
        this->Outbound_ = new Outbound(nexus_, 1, outboundURI, ReplicationManager);
    }

    DEBUG_MSG("NetworkManager(): inboundURI " << inboundURI);
}

void NetworkManager::send_message(messageType messageType, void *data, uint64_t dataLength) {
   this->Outbound_->send_message(messageType, data, dataLength); 
}

void NetworkManager::sync_inbound(int numberOfRuns) {
    this->Inbound_->run_event_loop(numberOfRuns);
}

void NetworkManager::terminate() {}
