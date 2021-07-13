#include "ReplicationManager.h"
#include "common_tests.h"
#include <iostream>
#include <random>
#include <chrono>

struct MeasureData {
    size_t totalNumberOfRequests{0};
    size_t remainderNumberOfRequests{0};
    size_t amountReadsSent{0};
    size_t amountAppendsSent{0};
    size_t amountReadsReceived{0};
    size_t amountAppendsReceived{0};
    size_t totalMessagesProcessed{0};
    std::chrono::duration<double> totalExecutionTime;
    double operationsPerSecond{0.0}; // Op/s
    double dataOut{0.0}; // MB/s
    double dataIn{0.0}; // MB/s
    size_t highestKnownLogOffset{0}; // So reads are performed on offset smaller than this
};

struct ProgArgs {
    NodeType nodeType; // -n
    size_t amountThreads; // -t
    size_t totalNumberOfRequests; // -r 
    size_t percentageOfReads; // -p ; Between 0 - 100
    size_t valueSize; // -s ; Bytes
};

ReplicationManager *localNode;
ProgArgs progArgs{HEAD, 1, 1000000, 50, 64};
MeasureData measureData;
string randomString = "";


void receive_locally(Message *message) {
    measureData.messagesReceived++;

    if (message->messageType == READ)
        measureData.amountReadsReceived++;
    else if (message->messageType == APPEND) 
        measureData.amountAppendsReceived++;

    localNode->NetworkManager_->rpc_.free_msg_buffer(*(message->reqBuffer));
    localNode->NetworkManager_->rpc_.free_msg_buffer(message->respBuffer);
}

// FIXME: Use the same message object for all reads. Just exchange the logOffset
void send_read_message(uint64_t logOffset) {
    Message message = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
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

    if (nodeType == HEAD)
        localNode->read(message);
    else
        localNode->NetworkManager_->send_message(HEAD, message);

    measureData.amountReadsSent++;
}

// FIXME: Use the same message object for all appends
void send_append_message(void *data, size_t dataLength) {
    Message message = (Message *) malloc(sizeof(Message));
    erpc::MsgBuffer reqRead = (erpc::MsgBuffer *) malloc(sizeof(erpc::MsgBuffer));
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

    DEBUG_MSG("run_node.send_append_message(Message: Type: " << std::to_string(message->messageType) << "; logOffset: " << " ; sentByThisNode: " << message->sentByThisNode << " ; reqBufferSize: " << std::to_string(message->reqBufferSize) << " ; respBufferSize: " << std::to_string(message->respBufferSize) <<")");

	if ( nodeType == HEAD )
        localNode->append(message);
    else 
        localNode->NetworkManager_->send_message(HEAD, message);

    measureData.amountAppendsSent++;
}

void start_benchmarking() {
    LogEntryInFlight logEntryInFlight{counter, { 0, ""}};
    randomString.copy(logEntryInFlight.logEntry.data, uniqueString.length());
    logEntryInFlight.logEntry.dataLength = randomString.length();

    auto start = std::chrono::high_resolution_clock::now();

    while(measureData.remainderNumberOfRequests) {
        int randomInt = rand() % 100
        if (randomInt < progArgs.percentageOfReads)
            send_read_message(1);
        else 
            send_append_message(&logEntryInFlight, logEntryInFlight.dataLength + sizeof(logEntryInFlight.dataLength));

        measureData.remainderNumberOfRequests--;
    }

    auto end = std::chrono::high_resolution_clock::now();
    measureData.totalExecutionTime = end - start;
    std::cout << "Time measured: seconds. " << std::to_string(measureData.totalExecutionTime.count())  << endl;
}

void parser(int amountArgs, const char **argv) {
    for (int i = 1; i < amountArgs; i++) {
        switch (argv[i][1]) {
            case 'n': // NodeType
                progArgs.nodeType = std::strtol(*(argv[i][3]), 0);
                break;
            case 't': // Threads amount
                progArgs.amountThreads = std::strtol(*(argv[i][3]), 0);
                break;
            case 'r': // Request amount
                progArgs.totalNumberOfRequests = std::strtol(*(argv[i][3]), 0) * 1000000;
                measureData.totalNumberOfRequests = progArgs.totalNumberOfRequests;
                measureData.remainderNumberOfRequests = progArgs.totalNumberOfRequests;
                break;
            case 'p': // Percentage reads
                progArgs.percentageOfReads = std::strtol(*(argv[i][3]), 0);
                break;
            case 's': // Size value
                progArgs.valueSize = std::strtol(*(argv[i][3]), 0);
                break;
        }
    }
    std::cout << "Parameter: nodeType: " << progArgs.nodeType << " amountThreads: " << progArgs.amountThreads << " totalNum: " << progArgs.totalNumberOfRequests << " percentage: " << progArgs.percentageOfReads << " valueSize: " << progArgs.valueSize << endl;
}

void printMeasureData() {
    measureData.totalMessagesProcessed = localNode->NetworkManager_->totalMessagesProcessed_ + measureData.totalNumberOfRequests;
    size_t totalMBSent = (measureData.amountAppendsSent * (sizeof(LogEntryInFlight.logOffset) + sizeof(LogEntry.dataLength) + progArgs.valueSize)) + measureData.amountReadsSent * 8;
    size_t totalMBReceived = (measureData.amountAppendsReceived * 8) + (measureData.amountReadsReceived * (sizeof(LogEntryInFlight.logOffset) + sizeof(LogEntry.dataLength) + progArgs.valueSize)); 

    std::cout << "-------------------------------------" << endl;
    std::cout << "Benchmark Summary" << endl;
    std::cout << "-------------------------------------" << endl;
    std::cout << "Total Requests: " << measureData.totalNumberOfRequests << endl;
    std::cout << "Total Requests Processed: " << measureData.totalMessagesProcessed << endl;
    std::cout << "Total Requests Received: " << (measureData.amountReadsReceived + measureData.amountAppendsReceived) << endl;
    std::cout << "Read Sent/Received: " << measureData.amountReadsSent << "/" << measureData.amountReadsReceived << endl;
    std::cout << "Append Sent/Received: " << measureData.amountAppendsSent << "/" << measureData.amountAppendsReceived << endl;
    std::cout << "Operations per Second: " << (measureData.totalMessagesProcessed / measureData.totalExecutionTime) << "Op/s" << endl;
    std::cout << "Total MB Sent/Received: " << totalMBSent << "/" << totalMBReceived << endl;
    std::cout << "Total MB/s Sent/Received: " << (totalMBSent / measureData.totalExecutionTime ) << "MB/s / " << (totalMBReceived / measureData.totalExecutionTime) << "MB/s" << endl;
    std::cout << "-------------------------------------" << endl;
}

void generateValueSize(int valueSize){
    /* Generate random string */
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);

    for(int i = 0; i < valueSize; i++){
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }
}

int main(int argc, char** argv) {
    std::cout << "-------------------------------------" << endl;
    std::cout << "Init everything..." << endl;

    parser(argc, argv);
    generateValueSize(progArgs.valueSize);

    switch(node) {
        case HEAD: localNode = new ReplicationManager(nodeType, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, &receive_locally); break;
        case TAIL: localNode = new ReplicationManager(nodeType, NARDOLE_URI, BILL_URI, std::string(), std::string(), &receive_locally ); break;
        case MIDDLE: break;
    }
    localNode->init();

    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking..." << endl;

    start_benchmarking();

    std::cout << "...Finished benchmarking" << endl;
    std::cout << "-------------------------------------" << endl;

    std::cout << "Validating Log..." << endl;
    uint64_t untilThisEntryValid = localNode->Log_.validate_log(&randomString, true);
    std::cout << "Until this Entry is Log Valid: " << std::to_string(untilThisEntryValid) << endl;

    printMeasureData();
}