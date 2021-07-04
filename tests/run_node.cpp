#include "rpc.h"
#include "ReplicationManager.h"

#define BILL_URI "131.159.102.1:31850"
#define NARDOLE_URI "131.159.102.2:31850" 


void receive_locally(Message *message) {
    DEBUG_MSG("run_node.receive_locally(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    DEBUG_MSG("run_node.receive_locally(LogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
}

int main(int argc, char** argv) {

    DEBUG_MSG("-------------------------------------");
    DEBUG_MSG("Init everything...");
    
    struct LogEntry
    {
        uint64_t dataLength;
        char data[LOG_BLOCK_DATA_SIZE];
    };

    // Check which type this node should be
    NodeType node = HEAD;
    if ( argc == 2 ) { 
        std::string cmd_arg(argv[1]);

        if ( cmd_arg.compare("head") == 0 ) {
            node = HEAD;
        } else if ( cmd_arg.compare("tail") == 0 ) {
            node = TAIL;
        }
    }
    DEBUG_MSG("This node is: " << node << "(HEAD= 0, MIDDLE= 1, TAIL= 2)");
    ReplicationManager *localNode{nullptr};

    switch(node) {
        case HEAD: localNode = new ReplicationManager(node, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, &receive_locally); break;
        case TAIL: localNode = new ReplicationManager(node, NARDOLE_URI, BILL_URI, std::string(), std::string(), &receive_locally ); break;
        case MIDDLE: break;
    }

    localNode->init();

    DEBUG_MSG("-------------------------------------");
    DEBUG_MSG("Start testing...");

    uint64_t counter{0};
    uint64_t changer{0};
    erpc::MsgBuffer req;

    Message message;
    message.sentByThisNode = true;
    message.reqBuffer = &req;

    while (true) {
        /* Fill message struct */
	    req = localNode->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	    message.respBuffer = localNode->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
        message.reqBufferSize = MAX_MESSAGE_SIZE;
        message.reqBufferSize = sizeof(LogEntryInFlight);
	    message.respBufferSize = MAX_MESSAGE_SIZE;
        message.logOffset = counter;

        if(changer) {
            LogEntryInFlight logEntryInFlight{counter, { 1, ""}};
            memcpy(message.reqBuffer->buf, &logEntryInFlight, sizeof(logEntryInFlight));
            message.messageType = READ;

            DEBUG_MSG("run_node.main(Message: Type: " << std::to_string(message.messageType) << "; logOffset: " << std::to_string(message.logOffset) << " ; sentByThisNode: " << message.sentByThisNode << " ; reqBufferSize: " << std::to_string(message.reqBufferSize) << " ; respBufferSize: " << std::to_string(message.respBufferSize) <<")");
            DEBUG_MSG("run_node.main(LogEntryInFlight: logOffset: " << std::to_string(logEntryInFlight.logOffset) << " ; dataLength: " << std::to_string(logEntryInFlight.logEntry.dataLength) << " ; data: " << logEntryInFlight.logEntry.data << " )");
            localNode->read(&message);

            ++changer;
            changer %= 2;
            ++counter;
        } else {
            LogEntryInFlight logEntryInFlight{counter, { 5, "Test"}};
            memcpy(message.reqBuffer->buf, &logEntryInFlight, sizeof(logEntryInFlight));
            message.messageType = APPEND;

            DEBUG_MSG("run_node.main(Message: Type: " << std::to_string(message.messageType) << "; logOffset: " << std::to_string(message.logOffset) << " ; sentByThisNode: " << message.sentByThisNode << " ; reqBufferSize: " << std::to_string(message.reqBufferSize) << " ; respBufferSize: " << std::to_string(message.respBufferSize) <<")");
            DEBUG_MSG("run_node.main(LogEntryInFlight: logOffset: " << std::to_string(logEntryInFlight.logOffset) << " ; dataLength: " << std::to_string(logEntryInFlight.logEntry.dataLength) << " ; data: " << logEntryInFlight.logEntry.data << ")");
	    if ( node == HEAD )
            	localNode->append(&message);
        else if ( node == TAIL )
            localNode->NetworkManager_->send_message(HEAD, &message);
	    else
	        localNode->NetworkManager_->send_message(MIDDLE, &message);

            ++changer;
        }
    
    localNode->NetworkManager_->sync(100);
    sleep(1);
    DEBUG_MSG("-------------------------------------");
    }
}
