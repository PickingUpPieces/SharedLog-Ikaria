#include "NetworkManager.h"
#include "common_networkLayer.h"
#include "ReplicationManager.h"


NetworkManager::NetworkManager(string inbound_url, int inbound_port, 
    string outbound_url, int outbound_port, 
    ReplicationManager *ReplicationManager) {

    std::string inbound_uri = inbound_url + ":" + std::to_string(inbound_port);
    this->nexus_ = new erpc::Nexus(inbound_uri, 0, 0);
    this->Inbound_ = new Inbound(nexus_, 0, ReplicationManager);

    if (!outbound_url.empty()) {
        std::string outbound_uri = outbound_url + ":" + std::to_string(outbound_port);
        this->Outbound_ = new Outbound(nexus_, 1, outbound_uri, ReplicationManager);
    }
}

void NetworkManager::send_message(messageType messageType, void *data, uint64_t dataLength) {
   this->Outbound_->send_message(messageType, data, dataLength); 
}

void NetworkManager::sync_inbound(int numberOfRuns) {
    this->Inbound_->run_event_loop(numberOfRuns);
}

void NetworkManager::terminate() {}
