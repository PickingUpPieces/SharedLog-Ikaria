#include <iostream>
#include <random>
#include <chrono>
#include <shared_mutex>
#include "common_info.h"
#include "common_tests.h"
#include "SharedLogNode.h"

/* FIXME: In case IPs change */
#define BILL_URI "131.159.102.1:31850"
#define NARDOLE_URI "131.159.102.2:31850"

#define CLARA_URI "129.215.165.58:31850"
#define AMY_URI "129.215.165.57:31850"
#define DONNA_URI "129.215.165.54:31850"
#define ROSE_URI "129.215.165.52:31850"
#define MARTHA_URI "129.215.165.53:31850"


SharedLogNode *localNode;
BenchmarkData benchmarkData;
std::mutex startBenchmark;


/* Callback function when a response is received */
void receive_locally(Message *message) {
	benchmarkData.messagesInFlight--;
    
    if (message->messageType == APPEND) {
        uint64_t *returnedLogOffset = (uint64_t *) message->respBuffer.buf;
        if (benchmarkData.highestKnownLogOffset < *returnedLogOffset)
            benchmarkData.highestKnownLogOffset = *returnedLogOffset;
    }  
}


/* Send a READ message */
void send_read_message(uint64_t logOffset) {
    localNode->read(logOffset);
    benchmarkData.amountReadsSent++;
}

/* Create an APPEND message, which is always sent */
void send_append_message(LogEntryInFlight *logEntryInFlight, size_t dataLength) {
    localNode->append(logEntryInFlight, dataLength);
    benchmarkData.amountAppendsSent++;
}


/* Benchmarking function for multiple threads */
void start_benchmarking_threads() {
    localNode->get_thread_ready();
    startBenchmark.unlock();

    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking..." << endl;
    
    /* Take start time */
    auto start = std::chrono::high_resolution_clock::now();

    localNode->terminate(false);

    /* Take end time */
    auto end = std::chrono::high_resolution_clock::now();
    benchmarkData.totalExecutionTime = end - start;

    localNode->get_results(&benchmarkData);
}


/* Benchmarking function for single thread */
void start_benchmarking_single() {
    /* Create data struct for APPEND */
    LogEntryInFlight logEntryInFlight = generate_random_logEntryInFlight(benchmarkData.progArgs.valueSize);
    localNode->get_thread_ready();

    // Append few messages so something can be read
    for(int i = 0; i < 100; i++) 
    	send_append_message(&logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));

    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking..." << endl;

    /* Take start time */
    auto start = std::chrono::high_resolution_clock::now();

    while(likely(benchmarkData.remainderNumberOfRequests)) {
        if (( rand() % 100 ) < benchmarkData.progArgs.probabilityOfRead) {
	        if ( benchmarkData.highestKnownLogOffset < 1)
		        continue;

	        uint64_t randuint = static_cast<uint64_t>(rand());
            uint64_t randReadOffset = randuint % benchmarkData.highestKnownLogOffset; 
            logEntryInFlight.messageType = READ;
            send_read_message(randReadOffset); 
        } else {
            logEntryInFlight.messageType = APPEND;
    	    send_append_message(&logEntryInFlight, logEntryInFlight.logEntry.dataLength + (2 * 8) + sizeof(MessageType));
        }

	    while(benchmarkData.messagesInFlight > 20000)
	        localNode->sync(10); 

	    benchmarkData.messagesInFlight++;
        benchmarkData.remainderNumberOfRequests--;
    }

    while((benchmarkData.progArgs.totalNumberOfRequests - benchmarkData.messagesInFlight) < (benchmarkData.progArgs.totalNumberOfRequests - benchmarkData.progArgs.percentileNumberOfRequests))
		localNode->sync(1);

    /* Take end time */
    auto end = std::chrono::high_resolution_clock::now();
    benchmarkData.totalExecutionTime = end - start;
}


/* Print out the benchmarkData struct and calculate additional information */
void printbenchmarkData() {
    std::cout << "-------------------------------------" << endl;
    std::cout << "Benchmark Summary" << endl;
    std::cout << "-------------------------------------" << endl;
    std::cout << "Total Requests to process: " << benchmarkData.progArgs.totalNumberOfRequests << endl;
    std::cout << "Total Requests to process by each thread: " << benchmarkData.remainderNumberOfRequests << endl;
    std::cout << "Total Requests processed on this node: " << benchmarkData.totalMessagesProcessed << endl;
    std::cout << "Processed READ/APPEND: " << benchmarkData.amountReadsSent << "/" << benchmarkData.amountAppendsSent << endl;
    std::cout << "Total time: " << benchmarkData.totalExecutionTime.count() << "s" << endl;
    std::cout << "Operations per Second: " << (static_cast<double>(benchmarkData.progArgs.totalNumberOfRequests) / benchmarkData.totalExecutionTime.count()) << " Op/s" << endl;
    std::cout << "-------------------------------------" << endl;
}


/* Parse the input arguments */
void parser(int amountArgs, char **argv) {
    for (int i = 1; i < amountArgs; i++) {
        switch (argv[i][1]) {
            case 'n': // NodeType
                if(!std::strtol(&(argv[i][3]), nullptr, 0))
                    benchmarkData.progArgs.nodeType = HEAD;
                else if (std::strtol(&(argv[i][3]), nullptr, 0) == 1)
                    benchmarkData.progArgs.nodeType = MIDDLE;
                else
                    benchmarkData.progArgs.nodeType = TAIL;
                break;
            case 'i': // NodeID
                benchmarkData.progArgs.nodeID = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 't': // Threads amount
                benchmarkData.progArgs.amountThreads = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'm': // Request amount
                benchmarkData.progArgs.totalNumberOfRequests = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'a': // Active mode
                benchmarkData.progArgs.activeMode = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'r': // Percentage reads
                benchmarkData.progArgs.probabilityOfRead = std::strtol(&(argv[i][3]), nullptr, 0);
                break;
            case 's': // Size value
                benchmarkData.progArgs.valueSize = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'p': // Percentile of messages to wait for 
                benchmarkData.progArgs.percentile = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
        }
    }

    benchmarkData.progArgs.percentileNumberOfRequests = benchmarkData.progArgs.totalNumberOfRequests - ((benchmarkData.progArgs.percentile * benchmarkData.progArgs.totalNumberOfRequests) / 100);
    benchmarkData.remainderNumberOfRequests = benchmarkData.progArgs.totalNumberOfRequests / benchmarkData.progArgs.amountThreads;
    benchmarkData.startBenchmark = &startBenchmark;
    std::cout << "Input Parameters: nodeType: " << benchmarkData.progArgs.nodeType << " activeMode: " << benchmarkData.progArgs.activeMode << " amountThreads: " << benchmarkData.progArgs.amountThreads << " totalNumOfRequests: " << benchmarkData.progArgs.totalNumberOfRequests << " RequestsPerThread: " << benchmarkData.remainderNumberOfRequests  << " Probability of Reads: " << benchmarkData.progArgs.probabilityOfRead << " percentileMessages: " << benchmarkData.progArgs.percentileNumberOfRequests  << " valueSize: " << benchmarkData.progArgs.valueSize << endl;
}


int main(int argc, char** argv) {
    std::cout << "-------------------------------------" << endl;
    std::cout << "Init everything..." << endl;

    parser(argc, argv);
    startBenchmark.lock();

    // Set Log file name to nodeID
    char *poolPath = static_cast<char *>(malloc(strlen(POOL_PATH) + 1)); 
    strncpy(poolPath, POOL_PATH, strlen(POOL_PATH) + 1);
    poolPath[strlen(POOL_PATH) - 5] = '0' + static_cast<char>(benchmarkData.progArgs.nodeID);

    #ifndef DPDK_CLUSTER
        switch(benchmarkData.progArgs.nodeType) {
            case HEAD: localNode = new SharedLogNode(benchmarkData.progArgs.nodeType, benchmarkData.progArgs.nodeID, poolPath, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, &benchmarkData, &receive_locally); break;
            case MIDDLE: break;
            case TAIL: localNode = new SharedLogNode(benchmarkData.progArgs.nodeType, benchmarkData.progArgs.nodeID, poolPath, NARDOLE_URI, BILL_URI, std::string(), std::string(), &benchmarkData, &receive_locally ); break;
        }
    #else
        #ifdef THREE_NODES
            switch(benchmarkData.progArgs.nodeID) {
                case 0: localNode = new SharedLogNode(HEAD, 0, poolPath, AMY_URI, std::string(), CLARA_URI, MARTHA_URI, &benchmarkData, &receive_locally); break;
                case 1: localNode = new SharedLogNode(MIDDLE, 1, poolPath, CLARA_URI, AMY_URI, MARTHA_URI, MARTHA_URI, &benchmarkData, &receive_locally ); break;
                case 2: localNode = new SharedLogNode(TAIL, 2, poolPath, MARTHA_URI, AMY_URI, std::string(), std::string(), &benchmarkData, &receive_locally ); break;
            }
        #endif
        #ifdef FOUR_NODES
            switch(benchmarkData.progArgs.nodeID) {
                case 0: localNode = new SharedLogNode(HEAD, 0, poolPath, AMY_URI, std::string(), CLARA_URI, ROSE_URI, &benchmarkData, &receive_locally); break;
                case 1: localNode = new SharedLogNode(MIDDLE, 1, poolPath, CLARA_URI, AMY_URI, MARTHA_URI, ROSE_URI, &benchmarkData, &receive_locally ); break;
                case 2: localNode = new SharedLogNode(MIDDLE, 2, poolPath, MARTHA_URI, AMY_URI, ROSE_URI, ROSE_URI, &benchmarkData, &receive_locally ); break;
                case 3: localNode = new SharedLogNode(TAIL, 3, poolPath, ROSE_URI, AMY_URI, std::string(), std::string(), &benchmarkData, &receive_locally ); break;
            }
        #endif
        #ifdef FIVE_NODES
            switch(benchmarkData.progArgs.nodeID) {
                case 0: localNode = new SharedLogNode(HEAD, 0, poolPath, AMY_URI, std::string(), CLARA_URI, DONNA_URI, &benchmarkData, &receive_locally); break;
                case 1: localNode = new SharedLogNode(MIDDLE, 1, poolPath, CLARA_URI, AMY_URI, MARTHA_URI, DONNA_URI, &benchmarkData, &receive_locally ); break;
                case 2: localNode = new SharedLogNode(MIDDLE, 2, poolPath, MARTHA_URI, AMY_URI, ROSE_URI, DONNA_URI, &benchmarkData, &receive_locally ); break;
                case 3: localNode = new SharedLogNode(MIDDLE, 3, poolPath, ROSE_URI, AMY_URI, DONNA_URI, DONNA_URI, &benchmarkData, &receive_locally ); break;
                case 4: localNode = new SharedLogNode(TAIL, 4, poolPath, DONNA_URI, AMY_URI, std::string(), std::string(), &benchmarkData, &receive_locally ); break;
            }
        #endif
    #endif


    if (benchmarkData.progArgs.amountThreads < 2) {
        if (benchmarkData.progArgs.activeMode)
            start_benchmarking_single();
        else {
            send_read_message(0);
            while(true) 
                localNode->sync(1);
        }
    } else
        start_benchmarking_threads();


    std::cout << "...Finished benchmarking" << endl;
    std::cout << "-------------------------------------" << endl;

    printbenchmarkData();
}
