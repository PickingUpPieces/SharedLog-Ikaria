#include <iostream>
#include "rpc.h"
#include "Inbound.h"
#include "common_info.h"
#include "NetworkManager.h"

Inbound::Inbound(erpc::Nexus *nexus, NetworkManager *NetworkManager) {
    DEBUG_MSG("Inbound()");
    NetworkManager_ = NetworkManager;
    Inbound::init(nexus);
}


// Request handler for read requests
void req_handler_read(erpc::ReqHandle *req_handle, void *context) {
    auto networkManager = static_cast<NetworkManager *>(context);

    // FIXME: Not sure if pre_resp_msgbuf is okay
    //erpc::MsgBuffer resp = req_handle->pre_resp_msgbuf;
    const erpc::MsgBuffer *req = req_handle->get_req_msgbuf();

    /* Alloc space for the message meta information and fill it */
    Message *message = (Message *) malloc(sizeof(Message));
    message->messageType = READ;
    message->sentByThisNode = false;
    message->logOffset = 0;
    message->reqHandle = req_handle;
    // FIXME: Is this const_cast a good idea?
    message->reqBuffer = const_cast<erpc::MsgBuffer *>(req);
    message->reqBufferSize = req->get_data_size();
    message->respBuffer = networkManager->rpc_->alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
    message->respBufferSize = MAX_MESSAGE_SIZE;

    DEBUG_MSG("Inbound.req_handler_read(LogEntryInFlight.logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; LogEntryInFlight.dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; main.LogEntryInFlight.data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");

    networkManager->receive_message(message);
}


// Request handler for append requests
void req_handler_append(erpc::ReqHandle *req_handle, void *context) {
    auto networkManager = static_cast<NetworkManager *>(context);
    const erpc::MsgBuffer *req = req_handle->get_req_msgbuf();
     
    /* Alloc space for the message meta information and fill it */
    Message *message = (Message *) malloc(sizeof(Message));
    message->messageType = APPEND;
    message->sentByThisNode = false;
    message->logOffset = 0;
    message->reqHandle = req_handle;
    // FIXME: Is this const_cast a good idea?
    message->reqBuffer = const_cast<erpc::MsgBuffer *>(req);
    message->reqBufferSize = req->get_data_size();
    // FIXME: Find out minimal message size required for the buffer
    message->respBuffer = networkManager->rpc_->alloc_msg_buffer_or_die(8);
    message->respBufferSize = 8;

    DEBUG_MSG("Inbound.req_handler_append(LogEntryInFlight.logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; LogEntryInFlight.dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; main.LogEntryInFlight.data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");

    networkManager->receive_message(message);
}

void Inbound::send_response(Message *message) {
    DEBUG_MSG("Inbound.send_response(messageType: " << message->messageType << " ; logOffset: " << message->logOffset << ")");

    switch (message->messageType) {
        case READ: {
	    if ( message->respBufferSize < MAX_MESSAGE_SIZE)
            	rpc_->resize_msg_buffer((erpc::MsgBuffer *) &message->respBuffer, message->respBufferSize);
            break;
        }
        case APPEND: 
            break;
    }
    
    rpc_->enqueue_response(message->reqHandle, &message->respBuffer);

    for (size_t i = 0; i < DEFAULT_RUN_EVENT_LOOP; i++)
      rpc_->run_event_loop_once();

    // FIXME: Is there any finally thing in c++?
    // FIXME: Check when to free_msg_buffers and if it's necessary
    free(message);
}

void Inbound::run_event_loop(int numberOfRuns) {
    for (int i = 0; i < numberOfRuns; i++)
        rpc_->run_event_loop_once();
}


void Inbound::init(erpc::Nexus *nexus) {
    // Register request handler for Request Type ReqTypeRead
    if (nexus->register_req_func(READ, req_handler_read)) {
        cerr << "Failed to initialize ReqTypeRead" << endl;
        std::terminate();
    } 

    // Register request handler for Request Type ReqTypeAppend
    if (nexus->register_req_func(APPEND, req_handler_append)) {
        cerr << "Failed to initialize ReqTypeAppend" << endl;
        std::terminate();
    }
}

void Inbound::set_rpc(erpc::Rpc<erpc::CTransport> *rpc) {
    rpc_ = rpc;
}

void Inbound::terminate() {}
