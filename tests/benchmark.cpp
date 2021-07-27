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
void send_append_message(void *data, size_t dataLength) {
    localNode->append(data, dataLength);
    benchmarkData.amountAppendsSent++;
}


/* Benchmarking function for multiple threads */
void start_benchmarking_threads() {
    startBenchmark.unlock();
    
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
    std::cout << "Total Requests: " << benchmarkData.progArgs.totalNumberOfRequests << endl;
    std::cout << "Total Requests Processed on this node: " << benchmarkData.totalMessagesProcessed << endl;
    std::cout << "Sent READ/APPEND: " << benchmarkData.amountReadsSent << "/" << benchmarkData.amountAppendsSent << endl;
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

    switch(benchmarkData.progArgs.nodeType) {
        case HEAD: localNode = new SharedLogNode(benchmarkData.progArgs.nodeType, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, &benchmarkData, &receive_locally); break;
        case TAIL: localNode = new SharedLogNode(benchmarkData.progArgs.nodeType, NARDOLE_URI, BILL_URI, std::string(), std::string(), &benchmarkData, &receive_locally ); break;
        case MIDDLE: break;
    }

    localNode->get_benchmark_ready();

    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking..." << endl;


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
