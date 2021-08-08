#include <iostream>
#include "CRAQNetworkManager.h"

void empty_sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *) {}

/**
 * Constructs the NetworkManager. Creates and configures the RPC Object. 
 * Creates the Inbound and the needed Outbounds.
 * @param nexus Nexus needed for the eRPC connection
 * @param hostURI String "hostname:port" where this node can be reached 
 * @param headURI String "hostname:port" of the HEAD node of the chain. If this node is the HEAD, leave it empty.
 * @param successorURI String "hostname:port" of the SUCCESSOR node of this node in the chain.
 * @param tailURI String "hostname:port" of the TAIL node of the chain. If this node is the TAIL, leave it empty.
 * @param CRAQReplication Reference needed for the message flow e.g. handing of messages for further process 
 */
NetworkManager::NetworkManager(NodeType nodeType, erpc::Nexus *nexus, uint8_t erpcID, string headURI, string successorURI, string tailURI, CRAQReplication *CRAQReplication):
        nodeType_{nodeType},
        erpcID_{erpcID},
        CRAQReplication_{CRAQReplication},
        Nexus_{nexus},
        Inbound_(new Inbound(nodeType, Nexus_, this)),
        rpc_{Nexus_, this, erpcID, empty_sm_handler, 0}
{
    rpc_.retry_connect_on_invalid_rpc_id = true;
    rpc_.set_pre_resp_msgbuf_size(MAX_MESSAGE_SIZE);

    if (nodeType_ != HEAD)
        Head_ = make_unique<Outbound>(headURI, erpcID, this, &rpc_);

    if (nodeType_ != TAIL) {
        Tail_ = make_shared<Outbound>(tailURI, erpcID, this, &rpc_);
    
        /* SUCCESSOR is the TAIL node */
        (successorURI.compare(tailURI) == 0) ? Successor_ = Tail_ : Successor_ = make_shared<Outbound>(successorURI, erpcID, this, &rpc_);
    }
}

/**
 * Establishes the connection for all Outbounds
 */
void NetworkManager::init() {
    if (Head_) Head_->connect();
    if (Successor_) Successor_->connect();
    if (Tail_ && (Tail_ != Successor_)) Tail_->connect();
}

/**
 * Further delegates a message which should be send out to the responsible Outbound
 * @param targetNode Specifying the node in the chain the message should be send to
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void NetworkManager::send_message(NodeType targetNode, Message *message) {
    messagesInFlight_++;
    DEBUG_MSG("NetworkManager.send_message(messagesInFlight: " << std::to_string(messagesInFlight_) << " ; erpcID: " << std::to_string(erpcID_) << ")");

    switch (targetNode)
    {
    case HEAD:
        Head_->send_message(message); 
        break;
    case SUCCESSOR:
        Successor_->send_message(message); 
        break;
    case TAIL:
        Tail_->send_message(message); 
        break;
    }
}

/**
 * Further delegates a response message for a previous received message to the Inbound
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void NetworkManager::send_response(Message *message) {
    DEBUG_MSG("NetworkManager.send_response(messagesInFlight: " << std::to_string(messagesInFlight_) << " ; totalMessagesCompleted: " << std::to_string(totalMessagesCompleted_) << " ; erpcID: " << std::to_string(erpcID_) << ")");
    Inbound_->send_response(message);
}

/**
 * Further delegates a received message from the Inbound to the CRAQReplication 
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void NetworkManager::receive_message(Message *message) {
    totalMessagesProcessed_++;
    // FIXME: Can be deleted
    if (!(totalMessagesProcessed_ % 1000000))
        std::cout << "localNode: messagesInFlight_: " << std::to_string(messagesInFlight_) << " ; totalMessagesCompleted_: " << std::to_string(totalMessagesCompleted_) << " ; totalMessagesProcessed_: " << std::to_string(totalMessagesProcessed_) << " ; erpcID: " << std::to_string(erpcID_) << endl;

    switch (message->messageType) 
    {
        case SETUP: 
            CRAQReplication_->setup(message); 
            break;
        case READ: 
            CRAQReplication_->read(message); 
            break;
        case APPEND: 
            CRAQReplication_->append(message); 
            break;
    }
}

/**
 * Further delegates a received response from an Outbound
 * SentByThisNode is needed when traffic for the chain on this node is created
 * @param message Message contains important meta information/pointer e.g. Request Handle, resp/req Buffers
 */
void NetworkManager::receive_response(Message *message) {
    messagesInFlight_--;
    totalMessagesCompleted_++;
    DEBUG_MSG("NetworkManager.receive_message(messagesInFlight: " << std::to_string(messagesInFlight_) << " ; totalMessagesCompleted: " << std::to_string(totalMessagesCompleted_) << " ; erpcID: " << std::to_string(erpcID_) << ")");

    if (message->sentByThisNode) {
        switch (message->messageType)
        {
            case APPEND:
                totalAppendsProcessed_++;
                break;
            case READ:
                totalReadsProcessed_++;
                break;
            default:
                break;
        }
        CRAQReplication_->receive_locally(message);
        rpc_.free_msg_buffer(message->reqBuffer);
        rpc_.free_msg_buffer(message->respBuffer);
        delete message;
        return;
    }

    switch (message->messageType)
    {
    case SETUP:
        CRAQReplication_->setup_response();
        if (nodeType_ == MIDDLE)
            send_response(message);
        break;
    case APPEND:
        totalAppendsProcessed_++;
        Inbound_->send_response(message);
        break;
    case READ:
        totalReadsProcessed_++;
        Inbound_->send_response(message);
        break;
    default:
        Inbound_->send_response(message);
        break;
    }
}

/**
 * Runs the event loop
 * @param numberOfRuns How often the event loop should run
 */
void NetworkManager::sync(int numberOfRuns) {
    for (int i = 0; i < numberOfRuns; i++)
        rpc_.run_event_loop_once();
}
