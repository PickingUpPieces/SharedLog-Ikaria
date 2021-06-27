#include "Outbound.h"
#include "ReplicationManager.h"
#include "NetworkManager.h"
#include "rpc.h"
#include <iostream>

void empty_cont_func(void *, void *) {}

void outbound_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

// FIXME: Could be the same cont_func in the end!
void cont_func_read(void *context, void *tag) {
    DEBUG_MSG("Outbound.cont_func_append(messsageType: " << to_string(((Message *) tag)->messageType) << " ; logOffset: " << to_string(((Message *) tag)->logOffset));
    auto outbound = static_cast<Outbound *>(context);
    outbound->NetworkManager_->receive_response((Message *) tag);
}

void cont_func_append(void *context, void *tag) {
    DEBUG_MSG("Outbound.cont_func_append(messsageType: " << to_string(((Message *) tag)->messageType) << " ; logOffset: " << to_string(((Message *) tag)->logOffset));
    auto outbound = static_cast<Outbound *>(context);
    outbound->NetworkManager_->receive_response((Message *) tag);
}

Outbound::Outbound(erpc::Nexus *nexus, uint8_t erpcID, string connectURI, NetworkManager *NetworkManager):
    erpcID_{erpcID}, 
    sessionNum_{-1}, 
    rpc_{nexus, this, erpcID_, outbound_sm_handler, 0}, 
    NetworkManager_{NetworkManager} 
{
    DEBUG_MSG("Outbound(): sessionNum " << std::to_string(this->sessionNum_) << "; erpcID: " << std::to_string(this->erpcID_) << "; connectURI: " << connectURI);
    Outbound::connect(connectURI);
}

void Outbound::send_message(Message *message) {
    DEBUG_MSG("Outbound.send_message(Type: " << std::to_string(message->messageType) << "; logOffset: " << to_string(message->logOffset) << ")");

    // FIXME: Do the buffers need to be from the same rpc?
    //reqBuffer_ = rpc_.alloc_msg_buffer_or_die(dataLength);
    //respBuffer_ = rpc_.alloc_msg_buffer_or_die(maxMessageSize);

    // FIXME: Send message without memcpy'ing it
    // Write message into buffer
    //memcpy(reqBuffer_.buf, data, dataLength);

    erpc::erpc_cont_func_t cont_func{nullptr};

    switch(message->messageType) {
      case READ: cont_func = cont_func_read; break;
      case APPEND: cont_func = cont_func_append; break;
    }

    // Enqueue the request
    rpc_.enqueue_request(sessionNum_, message->messageType, message->reqBuffer, message->respBuffer, cont_func, (void *) message);

    for (size_t i = 0; i < DEFAULT_RUN_EVENT_LOOP; i++)
      rpc_.run_event_loop_once();
}


void Outbound::connect(string connectURI) {
    DEBUG_MSG("Outbound.connect(" << connectURI << "): erpcID: " << std::to_string(this->erpcID_)); 
    sessionNum_ = rpc_.create_session(connectURI, 0);

    /* Try until Client is connected */
    while (!rpc_.is_connected( sessionNum_ )) 
        rpc_.run_event_loop_once();

    DEBUG_MSG("Outbound.connect(): Connection is ready");
    DEBUG_MSG("Outbound.connect(): Connection Bandwith is " << std::to_string( rpc_.get_bandwidth() / (1024 * 1024)) << "MiB/s");
}

void Outbound::terminate() {}