#include <iostream>
#include "Outbound.h"

Outbound::Outbound(string connectURI, NetworkManager *NetworkManager, erpc::Rpc<erpc::CTransport> *rpc):
        sessionNum_{-1}, 
        NetworkManager_{NetworkManager},
        rpc_{rpc}
{
    sessionNum_ = rpc_->create_session(connectURI, 0);
    DEBUG_MSG("Outbound(): sessionNum " << std::to_string(this->sessionNum_) << " ; connectURI: " << connectURI);
}


void cont_func(void *context, void *tag) {
    DEBUG_MSG("Outbound.cont_func(Message: Type: " << std::to_string(((Message *) tag)->messageType) << "; logOffset: " << std::to_string(((Message *) tag)->logOffset) << " ; sentByThisNode: " << ((Message *) tag)->sentByThisNode << " ; reqBufferSize: " << std::to_string(((Message *) tag)->reqBufferSize) << " ; respBufferSize: " << std::to_string(((Message *) tag)->respBufferSize) << ")");
    auto networkManager = static_cast<NetworkManager *>(context);
    networkManager->receive_response((Message *) tag);

    free((Message *) tag);
}


void Outbound::send_message(Message *message) {
    DEBUG_MSG("Outbound.send_message(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    DEBUG_MSG("Outbound.send_message(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
    
    /* Enqueue the request and send it */
    rpc_->enqueue_request(sessionNum_, message->messageType, message->reqBuffer, &message->respBuffer, cont_func, (void *) message);
    rpc_->run_event_loop_once();
}


void Outbound::connect() {
    /* Try until Client is connected */
    while (!rpc_->is_connected(sessionNum_)) 
    	rpc_->run_event_loop_once();

    DEBUG_MSG("Outbound.connect(): Connection is ready");
    DEBUG_MSG("Outbound.connect(): Connection Bandwith is " << std::to_string( rpc_->get_bandwidth() / (1024 * 1024)) << "MiB/s");
}

void Outbound::terminate() {}
