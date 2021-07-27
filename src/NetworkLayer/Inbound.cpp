#include <iostream>
#include "Inbound.h"

static once_flag req_handler_once;
static void register_req_handlers(erpc::Nexus *nexus);

/**
 * Constructs an Inbound Object and registers the request handlers
 * @param nexus Nexus needed for the eRPC connection
 * @param NetworkManager Reference needed for the message flow e.g. handing of messages for further process 
 */
Inbound::Inbound(NodeType nodeType, erpc::Nexus *nexus, NetworkManager *NetworkManager):
        nodeType_{nodeType},
        NetworkManager_{NetworkManager}
{
    DEBUG_MSG("Inbound()");
    //std::call_once(req_handler_once, register_req_handlers, nexus);
    std::call_once(req_handler_once, register_req_handlers, nexus);
}


/**
 * The request handler for SETUP requests
 * @param req_handle Request Handle used as reference for the incoming message
 * @param context Pointer to the NetworkManager for handing of the message
 */
void req_handler_setup(erpc::ReqHandle *req_handle, void *context) {
    auto networkManager = static_cast<NetworkManager *>(context);
    Message *message = (Message *) malloc(sizeof(Message));

    // Alloc new request Buffer
    if (networkManager->nodeType_ != TAIL) {
        size_t oldReqBufferSize = req_handle->get_req_msgbuf()->get_data_size();
        message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        while(!message->reqBuffer.buf) {
            networkManager->sync(1);
            message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        }
        memcpy(message->reqBuffer.buf, req_handle->get_req_msgbuf()->buf, oldReqBufferSize);
    } else
        message->reqBuffer = *req_handle->get_req_msgbuf();


    /* Alloc space for the message meta information and fill it */
    message->messageType = SETUP;
    message->sentByThisNode = false;
    message->logOffset = 0;
    message->reqHandle = req_handle;
    message->reqBufferSize = message->reqBuffer.get_data_size();
    message->respBuffer = req_handle->pre_resp_msgbuf;
    message->respBufferSize = message->respBuffer.get_data_size();

    DEBUG_MSG("Inbound.req_handler_init(LogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");

    networkManager->receive_message(message);
}


/**
 * The request handler for READ requests
 * @param req_handle Request Handle used as reference for the incoming message
 * @param context Pointer to the NetworkManager for handing of the message
 */
void req_handler_read(erpc::ReqHandle *req_handle, void *context) {
    auto networkManager = static_cast<NetworkManager *>(context);
    Message *message = (Message *) malloc(sizeof(Message));

    // Alloc new request Buffer
    if (networkManager->nodeType_ != TAIL) {
        size_t oldReqBufferSize = req_handle->get_req_msgbuf()->get_data_size();
        message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        while(!message->reqBuffer.buf) {
            networkManager->sync(1);
            message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        }
        memcpy(message->reqBuffer.buf, req_handle->get_req_msgbuf()->buf, oldReqBufferSize);
    } else
        message->reqBuffer = *req_handle->get_req_msgbuf();


    /* Alloc space for the message meta information and fill it */
    message->messageType = READ;
    message->sentByThisNode = false;
    message->logOffset = 0;
    message->reqHandle = req_handle;
    message->reqBufferSize = message->reqBuffer.get_data_size();
    message->respBuffer = req_handle->pre_resp_msgbuf;
    message->respBufferSize = message->respBuffer.get_data_size();

    DEBUG_MSG("Inbound.req_handler_read(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << ")"); 

    networkManager->receive_message(message);
}


/**
 * The request handler for APPEND requests
 * @param req_handle Request Handle used as reference for the incoming message
 * @param context Pointer to the NetworkManager for handing of the message
 */
void req_handler_append(erpc::ReqHandle *req_handle, void *context) {
    auto networkManager = static_cast<NetworkManager *>(context);
    Message *message = (Message *) malloc(sizeof(Message));

    // Alloc new request Buffer
    if (networkManager->nodeType_ != TAIL) {
        size_t oldReqBufferSize = req_handle->get_req_msgbuf()->get_data_size();
        message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        while(!message->reqBuffer.buf) {
            networkManager->sync(1);
            message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        }
        memcpy(message->reqBuffer.buf, req_handle->get_req_msgbuf()->buf, oldReqBufferSize);
    } else
        message->reqBuffer = *req_handle->get_req_msgbuf();

    /* Alloc space for the message meta information and fill it */
    message->messageType = APPEND;
    message->sentByThisNode = false;
    message->logOffset = 0;
    message->reqHandle = req_handle;
    message->reqBufferSize = message->reqBuffer.get_data_size();
    message->respBuffer = req_handle->pre_resp_msgbuf;
    message->respBufferSize = message->respBuffer.get_data_size();

    DEBUG_MSG("Inbound.req_handler_append(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");

    networkManager->receive_message(message);
}


/**
 * Sends response for a previous received message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void Inbound::send_response(Message *message) {
    DEBUG_MSG("Inbound.send_response(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    DEBUG_MSG("Inbound.send_response(LogEntryInFlight: dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");

    /* Resize respBuffer if necessary */
	if (message->respBufferSize < message->respBuffer.get_data_size())
        NetworkManager_->rpc_.resize_msg_buffer((erpc::MsgBuffer *) &message->respBuffer, message->respBufferSize);
    
    NetworkManager_->rpc_.enqueue_response(message->reqHandle, &message->respBuffer);
    NetworkManager_->rpc_.run_event_loop_once();

    if (nodeType_ != TAIL)
        NetworkManager_->rpc_.free_msg_buffer(message->reqBuffer);

    free(message);
}

/**
 * Registers the request handlers at the Nexus Object
 * @param nexus Nexus needed for registering the request handlers
 */
static void register_req_handlers(erpc::Nexus *nexus) {
    DEBUG_MSG("Inbound.register_req_handlers()");
    // Register request handler for Request Type INIT
    if (nexus->register_req_func(SETUP, req_handler_setup)) {
        cerr << "Failed to initialize req INIT" << endl;
        std::terminate();
    } 

    // Register request handler for Request Type READ
    if (nexus->register_req_func(READ, req_handler_read)) {
        cerr << "Failed to initialize req READ" << endl;
        std::terminate();
    } 

    // Register request handler for Request Type APPEND
    if (nexus->register_req_func(APPEND, req_handler_append)) {
        cerr << "Failed to initialize req APPEND" << endl;
        std::terminate();
    }
}
