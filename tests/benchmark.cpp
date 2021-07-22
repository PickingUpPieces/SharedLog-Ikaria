#include "rpc.h"
#include "SharedLogNode.h"
#include "common_info.h"
#include <iostream>
#include <random>
#include <chrono>

/* FIXME: In case IPs change */
#define NODETYPE TAIL
#define BILL_URI "131.159.102.1:31850"
#define NARDOLE_URI "131.159.102.2:31850"

/* Collects the measured data */
struct MeasureData {
    size_t totalNumberOfRequests{0};
    size_t remainderNumberOfRequests{0};
    size_t amountReadsSent{0};
    size_t amountAppendsSent{0};
    size_t amountReadsReceived{0};
    size_t amountAppendsReceived{0};
    size_t totalMessagesProcessed{0};
    std::chrono::duration<double> totalExecutionTime{};
    double operationsPerSecond{0.0}; // Op/s
    double dataOut{0.0}; // MB/s
    double dataIn{0.0}; // MB/s
    uint64_t highestKnownLogOffset{0}; // So reads are performed on offset smaller than this
};

/* Holds the input arguments */
struct ProgArgs {
    NodeType nodeType; // -n
    size_t amountThreads; // -t
    size_t totalNumberOfRequests; // -r 
    int percentageOfReads; // -p ; Between 0 - 100
    size_t valueSize; // -s ; Bytes
};

SharedLogNode *localNode;
ProgArgs progArgs{NODETYPE, 1, 500000, 50, 64};
MeasureData measureData{500000, 500000};
string randomString = "";
static int messagesInFlight;


/* Callback function when a response is received */
void receive_locally(Message *message) {
    messagesInFlight--;
    
    if (message->messageType == READ) {
        measureData.amountReadsReceived++;
    } else if (message->messageType == APPEND) {
        measureData.amountAppendsReceived++;
        uint64_t *returnedLogOffset = (uint64_t *) message->respBuffer.buf;

        if (measureData.highestKnownLogOffset < *returnedLogOffset)
            measureData.highestKnownLogOffset = *returnedLogOffset;
    }  
}


/* Send a READ message */
void send_read_message(uint64_t logOffset) {
    localNode->read(logOffset);
    measureData.amountReadsSent++;
}

/* Create an APPEND message, which is always sent */
void send_append_message(void *data, size_t dataLength) {
    localNode->append(data, dataLength);
    measureData.amountAppendsSent++;
}


/* Benchmarking function for multiple threads */
void start_benchmarking_threads() {

    /* Take start time */
    auto start = std::chrono::high_resolution_clock::now();

    localNode->join_threads();

    /* Take end time */
    auto end = std::chrono::high_resolution_clock::now();
    measureData.totalExecutionTime = end - start;
}


/* Benchmarking function for single thread */
void start_benchmarking_single() {
    /* Create data struct for APPEND */
    LogEntryInFlight logEntryInFlight{1, { 0, ""}};
    randomString.copy(logEntryInFlight.logEntry.data, randomString.length());
    logEntryInFlight.logEntry.dataLength = randomString.length();
    measureData.highestKnownLogOffset = 1;

    /* Take start time */
    auto start = std::chrono::high_resolution_clock::now();

    while(measureData.remainderNumberOfRequests) {
        if (( rand() % 100 ) < progArgs.percentageOfReads) {
	        if ( measureData.highestKnownLogOffset < 1)
		        continue;

	    uint64_t randuint = static_cast<uint64_t>(rand());
            uint64_t randReadOffset = randuint % measureData.highestKnownLogOffset; 
            send_read_message(randReadOffset); 
        } else {
    	    send_append_message(&logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8));
        }

	    while(messagesInFlight > 20000)
	        localNode->sync(10); 

	    messagesInFlight++;
        measureData.remainderNumberOfRequests--;
    }

    /* Take end time */
    auto end = std::chrono::high_resolution_clock::now();
    measureData.totalExecutionTime = end - start;
}


/* Print out the MeasureData struct and calculate additional information */
void printMeasureData() {
    size_t totalMBSent = ((measureData.amountAppendsSent * ( 8 + 8 + progArgs.valueSize)) + measureData.amountReadsSent * 8 ) / 1024 / 1024;
    size_t totalMBReceived = ((measureData.amountAppendsReceived * 8) + (measureData.amountReadsReceived * ((8 + 8) + progArgs.valueSize))) / 1024 / 1024; 

    std::cout << "-------------------------------------" << endl;
    std::cout << "Benchmark Summary" << endl;
    std::cout << "-------------------------------------" << endl;
    std::cout << "Total Requests: " << measureData.totalNumberOfRequests << endl;
    //std::cout << "Total Requests Processed: " << measureData.totalMessagesProcessed << endl;
    std::cout << "Total Requests Received: " << (measureData.amountReadsReceived + measureData.amountAppendsReceived) << endl;
    std::cout << "Read Sent/Received: " << measureData.amountReadsSent << "/" << measureData.amountReadsReceived << endl;
    std::cout << "Append Sent/Received: " << measureData.amountAppendsSent << "/" << measureData.amountAppendsReceived << endl;
    std::cout << "Total time: " << measureData.totalExecutionTime.count() << "s" << endl;
    std::cout << "Operations per Second: " << (static_cast<double>(measureData.totalNumberOfRequests) / measureData.totalExecutionTime.count()) << " Op/s" << endl;
    std::cout << "Total MB Sent/Received: " << totalMBSent << " MB / " << totalMBReceived << " MB" << endl;
    std::cout << "Total MB/s Sent/Received: " << (static_cast<double>(totalMBSent) / measureData.totalExecutionTime.count() ) << " MB/s / " << (static_cast<double>(totalMBReceived) / measureData.totalExecutionTime.count()) << " MB/s" << endl;
    std::cout << "-------------------------------------" << endl;
}


/* Parse the input arguments */
void parser(int amountArgs, char **argv) {
    for (int i = 1; i < amountArgs; i++) {
        switch (argv[i][1]) {
            case 'n': // NodeType
                //progArgs.nodeType = std::strtol(&(argv[i][3]), nullptr, 0);
                break;
            case 't': // Threads amount
                progArgs.amountThreads = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'r': // Request amount
                progArgs.totalNumberOfRequests = std::stoul(&(argv[i][3]), nullptr, 0) * 1000000;
                measureData.totalNumberOfRequests = progArgs.totalNumberOfRequests;
                measureData.remainderNumberOfRequests = progArgs.totalNumberOfRequests;
                break;
            case 'p': // Percentage reads
                progArgs.percentageOfReads = std::strtol(&(argv[i][3]), nullptr, 0);
                break;
            case 's': // Size value
                progArgs.valueSize = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
        }
    }
    std::cout << "Input Parameters: nodeType: " << progArgs.nodeType << " amountThreads: " << progArgs.amountThreads << " totalNumOfRequests: " << progArgs.totalNumberOfRequests << " Percentage of Reads: " << progArgs.percentageOfReads << " valueSize: " << progArgs.valueSize << endl;
}


/* Generate a random string for sending in append requests */
void generateValueSize(int valueSize){
    string possibleCharacters = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    mt19937 generator{random_device{}()};
    uniform_int_distribution<> dist(0, possibleCharacters.size()-1);

    for(int i = 0; i < valueSize; i++) {
        size_t random_index = static_cast<size_t>(dist(generator)); //get index between 0 and possible_characters.size()-1
        randomString += possibleCharacters[random_index];
    }
}



int main(int argc, char** argv) {
    std::cout << "-------------------------------------" << endl;
    std::cout << "Init everything..." << endl;

    parser(argc, argv);
    generateValueSize(progArgs.valueSize);

    switch(progArgs.nodeType) {
        case HEAD: localNode = new SharedLogNode(progArgs.nodeType, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, progArgs.amountThreads, &receive_locally); break;
        case TAIL: localNode = new SharedLogNode(progArgs.nodeType, NARDOLE_URI, BILL_URI, std::string(), std::string(), progArgs.amountThreads, &receive_locally ); break;
        case MIDDLE: break;
    }

    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking..." << endl;


    if (progArgs.amountThreads < 2)
        start_benchmarking_single();
    else
        start_benchmarking_threads();


    std::cout << "...Finished benchmarking" << endl;
    std::cout << "-------------------------------------" << endl;

    printMeasureData();
}
