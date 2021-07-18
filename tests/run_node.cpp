#include "rpc.h"
#include "ReplicationManager.h"
#include <iostream>
#include <random>

enum Modus {
    SLOW,
    FAST
};

#define BILL_URI "131.159.102.1:31850"
#define NARDOLE_URI "131.159.102.2:31850" 
#define ACTIVE_MODE false
//#define MODUS SLOW
#define MODUS FAST

int messagesInFlight_{0};
int messagesSent_{0};
int messagesFinished_{0};
int messagesValidated_{0};
ReplicationManager *localNode;
Message *message;
erpc::MsgBuffer *reqRead; 
erpc::MsgBuffer *reqAppend; 
// Check which type this node should be
NodeType node{HEAD};
string randomString = "";


void receive_locally(Message *message) {
    messagesInFlight_--;
    messagesFinished_++;
    DEBUG_MSG("run_node.receive_locally(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");
    DEBUG_MSG("run_node.receive_locally(reqLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->reqBuffer->buf)->logEntry.data << ")");
    DEBUG_MSG("run_node.receive_locally(respLogEntryInFlight: logOffset: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logOffset) << " ; dataLength: " << std::to_string(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.dataLength) << " ; data: " << ((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data << ")");


    /* Verify the entry */
    if (message->messageType == READ) {
	#ifdef TESTING
        string uniqueString = randomString + "-ID-" + std::to_string(message->logOffset);
	string tempString((char *) &(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data));
	#else
        string uniqueString = randomString; 
	string tempString((char *) &(((LogEntryInFlight *) message->respBuffer.buf)->logEntry.data));
	#endif
        DEBUG_MSG("rec generatedString vs returnedString: '" << uniqueString << "' vs '" << tempString << "'");

	if (uniqueString.compare(tempString) == 0)
            messagesValidated_++;
    }

    DEBUG_MSG("run_node.receive_locally(messagesInFlight_: " << std::to_string(messagesInFlight_) << " ; messagesSent_: " << std::to_string(messagesSent_) << " ; messagesFinished_: " << std::to_string(messagesFinished_) << " ; messagesValidated_: " << std::to_string(messagesValidated_) << ")");
    localNode->NetworkManager_->rpc_.free_msg_buffer(*(message->reqBuffer));
    localNode->NetworkManager_->rpc_.free_msg_buffer(message->respBuffer);
}

void send_read_message(uint64_t logOffset) {
    message = (Message *) malloc(sizeof(Message));
    reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
    *reqRead = localNode->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);

    /* Fill message struct */
    message->reqBuffer = reqRead;
	message->respBuffer = localNode->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->logOffset = logOffset;
    message->messageType = READ;

    /* Fill request data */
    uint64_t *reqPointer = (uint64_t *) message->reqBuffer->buf;
    *reqPointer = message->logOffset;
    message->reqBufferSize = sizeof(message->logOffset);
    localNode->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);

    DEBUG_MSG("run_node.send_read_message(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << std::to_string(message->logOffset) << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

    if (node == HEAD)
        localNode->read(message);
    else
        localNode->NetworkManager_->send_message(HEAD, message);

    messagesInFlight_++;
    messagesSent_++;
    DEBUG_MSG("run_node.send_read_message(messagesInFlight_: " << std::to_string(messagesInFlight_) << " ; messagesSent_: " << std::to_string(messagesSent_) << ")");
}

void send_append_message(void *data, size_t dataLength) {
    message = (Message *) malloc(sizeof(Message));
    reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
    *reqRead = localNode->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);

    /* Fill message struct */
    message->reqBuffer = reqRead;
    message->reqBufferSize = MAX_MESSAGE_SIZE;
	message->respBuffer = localNode->NetworkManager_->rpc_.alloc_msg_buffer_or_die(MAX_MESSAGE_SIZE);
	message->respBufferSize = MAX_MESSAGE_SIZE;
    message->sentByThisNode = true;
    message->messageType = APPEND;

    /* Fill request data */
    memcpy(message->reqBuffer->buf, data, dataLength);
    message->reqBufferSize = dataLength;
    #ifndef TESTING
    localNode->NetworkManager_->rpc_.resize_msg_buffer(message->reqBuffer, message->reqBufferSize);
    #endif



    DEBUG_MSG("run_node.send_append_message(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

	if ( node == HEAD )
        localNode->append(message);
    else 
        localNode->NetworkManager_->send_message(HEAD, message);

    messagesInFlight_++;
    messagesSent_++;
    DEBUG_MSG("run_node.send_append_message(messagesInFlight_: " << std::to_string(messagesInFlight_) << " ; messagesSent_: " << std::to_string(messagesSent_) << ")");
}

void testing(Modus modus) {
    DEBUG_MSG("-------------------------------------");
    uint64_t counter{0};

    /* Generate random string */
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);
    for(int i = 0; i < 16; i++){
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }


    while (true) {
        /* Create string and LogEntryInFlight */
        string uniqueString = randomString;
        LogEntryInFlight logEntryInFlight{counter, { 0, ""}};
        uniqueString.copy(logEntryInFlight.logEntry.data, uniqueString.length());
        logEntryInFlight.logEntry.dataLength = uniqueString.length();

        /* APPEND */
        send_append_message(&logEntryInFlight, sizeof(logEntryInFlight));

        /* READ and verify */
        send_read_message(counter);
        ++counter;

        if(modus == SLOW)
            sleep(1);
        else {
            if ((messagesSent_ % 10000) == 0) {
                std::cout << "tests: messagesInFlight_: " << std::to_string(messagesInFlight_) << "; messagesSent_: " << std::to_string(messagesSent_) << " ; messagesFinished_: " << std::to_string(messagesFinished_) << " ; messagesValidated_: " << std::to_string(messagesValidated_) << endl;
                std::cout << "localNode: messagesInFlight_: " << std::to_string(localNode->NetworkManager_->messagesInFlight_) << " ; totalMessagesCompleted_: " << std::to_string(localNode->NetworkManager_->totalMessagesCompleted_) << " ; totalMessagesProcessed_: " << std::to_string(localNode->NetworkManager_->totalMessagesProcessed_) << endl;
                std::cout << "HugePage allocated in MB: " << std::to_string(localNode->NetworkManager_->rpc_.get_stat_user_alloc_tot() / 1024 / 1024) << "; Average RX batch: " << std::to_string(localNode->NetworkManager_->rpc_.get_avg_rx_batch()) << "; Average TX batch: " << std::to_string(localNode->NetworkManager_->rpc_.get_avg_tx_batch()) << endl;
                std::cout << "Active Sessions: " << std::to_string(localNode->NetworkManager_->rpc_.num_active_sessions()) << endl;

                std::cout << "Validating Log..." << endl;
		#ifdef TESTING
                uint64_t untilThisEntryValid = localNode->Log_.validate_log(&randomString, true);
		#else
                uint64_t untilThisEntryValid = localNode->Log_.validate_log(&randomString, false);
		#endif
                std::cout << "Until this Entry is Log Valid: " << std::to_string(untilThisEntryValid) << endl;
		        std::cout << "-------------------------" << endl;

            }
        }

        while (messagesInFlight_ > 10000)
            localNode->NetworkManager_->sync(1000);

        DEBUG_MSG("------------------------------------");
    }
}

int main(int argc, char** argv) {
    DEBUG_MSG("-------------------------------------");
    DEBUG_MSG("Init everything...");

    if ( argc == 2 ) { 
        std::string cmd_arg(argv[1]);

        if ( cmd_arg.compare("head") == 0 )
            node = HEAD;
        else if ( cmd_arg.compare("tail") == 0 )
            node = TAIL;
    }
    DEBUG_MSG("This node is: " << node << "(HEAD=0, MIDDLE=1, TAIL=2)");

    switch(node) {
        case HEAD: localNode = new ReplicationManager(node, 0, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, false, &receive_locally); break;
        case TAIL: localNode = new ReplicationManager(node, 0, NARDOLE_URI, BILL_URI, std::string(), std::string(), false, &receive_locally ); break;
        case MIDDLE: break;
    }
    localNode->init();

    if (ACTIVE_MODE)
        testing(MODUS);
    else {
	if (node == HEAD)
	    send_read_message(0);

        while (true)
           localNode->NetworkManager_->sync(1000); 
    }
}
