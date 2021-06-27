#include "Outbound.h"
#include "ReplicationManager.h"
#include "rpc.h"
#include <iostream>

void empty_cont_func(void *, void *) {}

void cont_func_read(void *context, void *) {
    DEBUG_MSG("Outbound.cont_func_read()");
    auto outbound = static_cast<Outbound *>(context);

    // TODO: Implement cont function
}

void cont_func_append(void *context, void *) {
    DEBUG_MSG("Outbound.cont_func_append()");
    auto outbound = static_cast<Outbound *>(context);

    // TODO: Implement cont function
}

Outbound::Outbound(erpc::Nexus *nexus, uint8_t erpcID, string connectURI, ReplicationManager *ReplicationManager):
    erpcID_{erpcID} {
    this->rpc_ = new erpc::Rpc<erpc::CTransport>(nexus, this, this->erpcID_, nullptr);
    this->ReplicationManager_ = ReplicationManager;

    DEBUG_MSG("Outbound(): sessionNum " << this->sessionNum_ << "; erpcID: " << this->erpcID_ << "; connectURI: " << connectURI);
    
    Outbound::connect(connectURI);
}

void Outbound::send_message(messageType messageType, void *data, uint64_t dataLength) {
    DEBUG_MSG("send_message(" << messageType << ")");

    // Get buffer for request and response
    reqBuffer_ = this->rpc_->alloc_msg_buffer_or_die(dataLength);
    respBuffer_ = this->rpc_->alloc_msg_buffer_or_die(max_message_size);

    erpc::erpc_cont_func_t cont_func{nullptr};

    // Write message into buffer
    // FIXME: Send message without memcpy'ing it
    memcpy(respBuffer_.buf, data, dataLength);

    switch(messageType) {
      case READ: cont_func = cont_func_read; break;
      case APPEND: cont_func = cont_func_append; break;
    }

    // Enqueue the request
    this->rpc_->enqueue_request(sessionNum_, messageType, &reqBuffer_, &respBuffer_, cont_func, 0);

    for (size_t i = 0; i < DEFAULT_RUN_EVENT_LOOP; i++)
      this->rpc_->run_event_loop_once();
}


void Outbound::connect(string connectURI) {
    DEBUG_MSG("Outbound.connect(" << connectURI << ");"); 
    this->sessionNum_ = rpc_->create_session(connectURI, 0);

    /* Try until Client is connected */
    while (!rpc_->is_connected(this->sessionNum_)) 
      this->rpc_->run_event_loop_once();

    DEBUG_MSG("Connection is ready");
    DEBUG_MSG("Connection Bandwith: " << ( rpc_->get_bandwidth() / (1024 * 1024)) << "MiB/s");
}

void Outbound::terminate() {}