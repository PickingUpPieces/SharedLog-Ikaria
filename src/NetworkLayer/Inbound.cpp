#include <iostream>
#include "rpc.h"
#include "Inbound.h"
#include "common_info.h"
#include "NetworkManager.h"

void inbound_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

Inbound::Inbound(erpc::Nexus *nexus, uint8_t erpcID, NetworkManager *NetworkManager) {
    erpcID_ = erpcID;
    NetworkManager_ = NetworkManager;

    Inbound::init(nexus);
    // FIXME: Two RPC Objects: One Inbound + One outbound; Each own hw_port
    rpc_ = new erpc::Rpc<erpc::CTransport>(nexus, this, erpcID_, inbound_sm_handler, 0);

    DEBUG_MSG("Inbound(): erpcID " << std::to_string(erpcID_));
}


// Request handler for read requests
void req_handler_read(erpc::ReqHandle *req_handle, void *context) {
    DEBUG_MSG("Inbound.req_handler_read()");
    auto inbound = static_cast<Inbound *>(context);

    // FIXME: Not sure if pre_resp_msgbuf is okay
    //erpc::MsgBuffer resp = req_handle->pre_resp_msgbuf;
    erpc::MsgBuffer resp = inbound->rpc_->alloc_msg_buffer_or_die(maxMessageSize);
    const erpc::MsgBuffer *req = req_handle->get_req_msgbuf();

    /* Alloc space for the message meta information and fill it */
    Message *message = (Message *) malloc(sizeof(Message));
    message->messageType = READ;
    message->reqHandle = req_handle;
    // FIXME: Is this const_cast a good idea?
    message->reqBuffer = const_cast<erpc::MsgBuffer *>(req);
    message->reqBufferSize = req->get_data_size();
    message->respBuffer = &resp;
    message->respBufferSize = maxMessageSize;

    inbound->NetworkManager_->receive_message(message);
}


// Request handler for append requests
void req_handler_append(erpc::ReqHandle *req_handle, void *context) {
    DEBUG_MSG("Inbound.req_handler_append()");
    auto inbound = static_cast<Inbound *>(context);
    const erpc::MsgBuffer *req = req_handle->get_req_msgbuf();
    // FIXME: Not sure if pre_resp_msgbuf is okay
    erpc::MsgBuffer resp = req_handle->pre_resp_msgbuf;
     
    /* Alloc space for the message meta information and fill it */
    Message *message = (Message *) malloc(sizeof(Message));
    message->messageType = APPEND;
    message->reqHandle = req_handle;
    // FIXME: Is this const_cast a good idea?
    message->reqBuffer = const_cast<erpc::MsgBuffer *>(req);
    message->reqBufferSize = req->get_data_size();
    message->respBuffer = &resp;
    message->respBufferSize = resp.get_data_size();

    DEBUG_MSG("DELETE THIS: respBuffer pre_resp_msgbuf size: " << resp.get_data_size());

    inbound->NetworkManager_->receive_message(message);
}

void Inbound::send_response(Message *message) {
    DEBUG_MSG("Inbound.send_response(messageType: " << message->messageType << " ; logOffset: " << message->logOffset << ")");

    switch (message->messageType) {
        case READ: {
            rpc_->resize_msg_buffer((erpc::MsgBuffer *) message->respBuffer, message->respBufferSize);
            break;
        }
        case APPEND: { 
            // FIXME: Find out minimal message size required for the buffer
            rpc_->resize_msg_buffer(message->respBuffer, 8);
            break;
        }
    }
    
    rpc_->enqueue_response(message->reqHandle, message->respBuffer);

    // FIXME: Is there any finally thing in c++?
    // FIXME: Check when to free_msg_buffers and if it's necessary
    delete message;
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

void Inbound::terminate() {}