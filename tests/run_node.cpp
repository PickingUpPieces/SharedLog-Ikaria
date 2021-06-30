#include "rpc.h"
#include "ReplicationManager.h"

#define HOSTNAME_HEAD "131.159.102.1"
#define PORT_HEAD 31850
#define HOSTNAME_TAIL "131.159.102.2" 
#define PORT_TAIL 31850


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
        case HEAD: localNode = new ReplicationManager(node, HOSTNAME_HEAD, PORT_HEAD, HOSTNAME_TAIL, PORT_TAIL, &receive_locally); break;
        case TAIL: localNode = new ReplicationManager(node, HOSTNAME_TAIL, PORT_TAIL, std::string(), -1, &receive_locally ); break;
        case MIDDLE: break;
    }

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
	    req = localNode->NetworkManager_->rpc_->alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	    message.respBuffer = localNode->NetworkManager_->rpc_->alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
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
        } else {
            LogEntryInFlight logEntryInFlight{counter, { 5, "Test"}};
            memcpy(message.reqBuffer->buf, &logEntryInFlight, sizeof(logEntryInFlight));
            message.messageType = APPEND;

            DEBUG_MSG("run_node.main(Message: Type: " << std::to_string(message.messageType) << "; logOffset: " << std::to_string(message.logOffset) << " ; sentByThisNode: " << message.sentByThisNode << " ; reqBufferSize: " << std::to_string(message.reqBufferSize) << " ; respBufferSize: " << std::to_string(message.respBufferSize) <<")");
            DEBUG_MSG("run_node.main(LogEntryInFlight: logOffset: " << std::to_string(logEntryInFlight.logOffset) << " ; dataLength: " << std::to_string(logEntryInFlight.logEntry.dataLength) << " ; data: " << logEntryInFlight.logEntry.data << ")");
            localNode->append(&message);

            ++changer;
        }
    
    localNode->NetworkManager_->sync(100);
    ++counter;
    sleep(1);
    DEBUG_MSG("-------------------------------------");
    }
}