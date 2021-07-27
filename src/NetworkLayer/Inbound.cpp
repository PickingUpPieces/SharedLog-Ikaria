#include <iostream>
#include "Inbound.h"

static once_flag req_handler_once;
static void register_req_handler(erpc::Nexus *nexus);

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
    std::call_once(req_handler_once, register_req_handler, nexus);
}


/**
 * The generic request handler for all requests types
 * @param req_handle Request Handle used as reference for the incoming message
 * @param context Pointer to the NetworkManager for handing of the message
 */
void req_handler(erpc::ReqHandle *req_handle, void *context) {
    auto networkManager = static_cast<NetworkManager *>(context);
    auto *logEntryInFlight = reinterpret_cast<LogEntryInFlight *>(req_handle->get_req_msgbuf()->buf);
    Message *message = (Message *) malloc(sizeof(Message));

    switch (logEntryInFlight->messageType) {
        case SETUP: 
            message->respBufferSize = 1; 
            message->messageType = SETUP;
            break;
        case READ: 
            message->respBufferSize = MAX_MESSAGE_SIZE; 
            message->messageType = READ;
            break;
        case APPEND: 
            message->respBufferSize = sizeof(uint64_t); 
            message->messageType = APPEND;
            break;
    }

    /* Alloc new request Buffer */
    if (networkManager->nodeType_ != TAIL) {
        size_t oldReqBufferSize = req_handle->get_req_msgbuf()->get_data_size();
        message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        while(!message->reqBuffer.buf) {
            networkManager->rpc_.run_event_loop_once();
            message->reqBuffer = networkManager->rpc_.alloc_msg_buffer(oldReqBufferSize);
        }
        memcpy(message->reqBuffer.buf, req_handle->get_req_msgbuf()->buf, oldReqBufferSize);
    } else
        message->reqBuffer = *req_handle->get_req_msgbuf();

    message->reqBufferSize = message->reqBuffer.get_data_size();

    /* Alloc new response Buffer */
    if (networkManager->nodeType_ != TAIL) {
        message->respBuffer = networkManager->rpc_.alloc_msg_buffer(message->respBufferSize);
        while(!message->respBuffer.buf) {
            networkManager->rpc_.run_event_loop_once();
            message->respBuffer = networkManager->rpc_.alloc_msg_buffer(message->respBufferSize);
        }
    } else 
        message->respBuffer = req_handle->pre_resp_msgbuf;
    
    /* Fill the rest of the message meta information */
    message->logOffset = logEntryInFlight->logOffset;
    message->sentByThisNode = false;
    message->reqHandle = req_handle;
    DEBUG_MSG("Inbound.req_handler(LogEntryInFlight: logOffset: " << std::to_string(logEntryInFlight->logOffset) << " ; MessageType: " << std::to_string(logEntryInFlight->messageType) << ")"); 

    networkManager->receive_message(message);
}


/**
 * Sends response for a previous received message
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void Inbound::send_response(Message *message) {
    DEBUG_MSG("Inbound.send_response(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    
    /* Copy data in pre_resp MsgBuffer */
    if (nodeType_ != TAIL)
        memcpy(message->reqHandle->pre_resp_msgbuf.buf, message->respBuffer.buf, message->respBufferSize);

    /* Resize respBuffer if necessary */
	if (message->respBufferSize < message->reqHandle->pre_resp_msgbuf.get_data_size())
        NetworkManager_->rpc_.resize_msg_buffer(&message->reqHandle->pre_resp_msgbuf, message->respBufferSize);

    NetworkManager_->rpc_.enqueue_response(message->reqHandle, &message->reqHandle->pre_resp_msgbuf);
    NetworkManager_->rpc_.run_event_loop_once();

    if (nodeType_ != TAIL) {
        NetworkManager_->rpc_.free_msg_buffer(message->reqBuffer);
        NetworkManager_->rpc_.free_msg_buffer(message->respBuffer);
    }
    free(message);
}

/**
 * Registers the request handler at the Nexus Object
 * @param nexus Nexus needed for registering the request handlers
 */
static void register_req_handler(erpc::Nexus *nexus) {
    if (nexus->register_req_func(1, req_handler)) {
        cerr << "Failed to initialize req hanlder" << endl;
        std::terminate();
    } 
}
