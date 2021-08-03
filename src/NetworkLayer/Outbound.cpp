#include <iostream>
#include "Outbound.h"

/**
 * Constructs an Outbound Object and creates the session
 * @param connectURI hostname and port to which the Outbound should connect to
 * @param NetworkManager Reference needed for the message flow e.g. handing of messages for further process 
 * @param rpc RPC Object for creating the session and sending messages / receiving responses
 */
Outbound::Outbound(string connectURI, uint8_t erpcID, NetworkManager *networkManager, erpc::Rpc<erpc::CTransport> *rpc):
        sessionNum_{-1}, 
        networkManager_{networkManager},
        rpc_{rpc}
{
    sessionNum_ = rpc_->create_session(connectURI, erpcID);
    DEBUG_MSG("Outbound(): sessionNum " << std::to_string(this->sessionNum_) << " ; connectURI: " << connectURI);
}


/**
 * (Callback) Continuation Function called when a response for a previous sent message is received
 * @param context Pointer to the NetworkManager for handing of the message
 * @param tag Pointer to the Message struct which holds all the important information for identifing the message
 */
void cont_func(void *context, void *tag) {
    ((Message *) tag)->respBufferSize = ((Message *) tag)->respBuffer.get_data_size();
    DEBUG_MSG("Outbound.cont_func(Message: Type: " << std::to_string(((Message *) tag)->messageType) << "; logOffset: " << std::to_string(((Message *) tag)->logOffset) << " ; sentByThisNode: " << ((Message *) tag)->sentByThisNode << " ; reqBufferSize: " << std::to_string(((Message *) tag)->reqBufferSize) << " ; respBufferSize: " << std::to_string(((Message *) tag)->respBufferSize) << ")");
    auto networkManager = static_cast<NetworkManager *>(context);
    networkManager->receive_response((Message *) tag);
}

/**
 * Sends a message to the connected client
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void Outbound::send_message(Message *message) {
    DEBUG_MSG("Outbound.send_message(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    DEBUG_MSG("Outbound.send_message(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer.buf)->logEntry.data << ")");
    DEBUG_MSG("Outbound.send_message(): reqSend: " << to_string(message->reqBuffer.get_data_size()) << " ; respSend: " << to_string(message->respBuffer.get_data_size()));
    
    /* Enqueue the request and send it */
    rpc_->enqueue_request(sessionNum_, 1, &message->reqBuffer, &(message->respBuffer), cont_func, (void *) message);
    rpc_->run_event_loop_once();

    DEBUG_MSG("Outbound.send_message(): Message sent");
}


/**
 * Establishes the connection to the client
 */
void Outbound::connect() {
    DEBUG_MSG("Outbound.connect(): Establishing Connection...");
    /* Try until Client is connected */
    while (!rpc_->is_connected(sessionNum_)) 
    	rpc_->run_event_loop_once();

    DEBUG_MSG("Outbound.connect(): Connection is ready");
    DEBUG_MSG("Outbound.connect(): Connection Bandwith is " << std::to_string( rpc_->get_bandwidth() / (1024 * 1024)) << "MiB/s");
}

/**
 * Disconnects the client and terminates this session
 */
void Outbound::terminate() {}
