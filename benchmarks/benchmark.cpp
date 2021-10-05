#include <iostream>
#include <random>
#include <chrono>
#include <shared_mutex>
#include "common_info.h"
#include "common_benchmark.h"
#include "SharedLogNode.h"

/* FIXME: In case IPs change */
#define BILL_URI "131.159.102.1:31850"
#define NARDOLE_URI "131.159.102.2:31850"

#define CLARA_URI "129.215.165.58:31851"
#define AMY_URI "129.215.165.57:31851"
#define DONNA_URI "129.215.165.54:31851"
#define ROSE_URI "129.215.165.52:31851"
#define MARTHA_URI "129.215.165.53:31851"

#ifdef CR
#define REPLICATION CRReplication
#elif CRAQ
#define REPLICATION CRAQReplication
#endif
SharedLogNode<REPLICATION> *localNode;

BenchmarkData benchmarkData;
std::mutex startBenchmark;
bool benchmarkTime{false};

/* Benchmarking function for operations */
void start_benchmark_operations() {
    localNode->get_thread_ready();
    startBenchmark.unlock();

    #ifndef BENCHMARK
    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking on operations..." << endl;
    #endif
    
    /* Take start time */
    auto start = std::chrono::high_resolution_clock::now();

    localNode->terminate(false);

    /* Take end time */
    auto end = std::chrono::high_resolution_clock::now();
    benchmarkData.totalExecutionTime = end - start;

    localNode->get_results(&benchmarkData);
}

/* Benchmarking function for time */
void start_benchmark_time() {
    localNode->get_thread_ready();
    startBenchmark.unlock();

    #ifndef BENCHMARK
    std::cout << "-------------------------------------" << endl;
    std::cout << "Start benchmarking on time..." << endl;
    #endif
    
    /* Take start time */
    auto start = std::chrono::high_resolution_clock::now();

    std::this_thread::sleep_for(benchmarkData.progArgs.time);
    localNode->terminate(true);

    /* Take end time */
    auto end = std::chrono::high_resolution_clock::now();
    benchmarkData.totalExecutionTime = end - start;

    localNode->get_results(&benchmarkData);
}


/* Print out the benchmarkData struct and calculate additional information */
void printbenchmarkData() {
    std::cout << "-------------------------------------" << endl;
    std::cout << "Benchmark Summary" << endl;
    std::cout << "-------------------------------------" << endl;
#ifdef CRAQ 
#ifdef UCR
    std::cout << "Replication Type: U-CR" << endl;
#else
    std::cout << "Replication Type: CRAQ" << endl;
#endif
#elif CR
    std::cout << "Replication Type: CR" << endl;
#endif
    if (benchmarkTime)
        std::cout << "Total time: " << benchmarkData.progArgs.time.count() << "s" << endl;
    else {
        std::cout << "Total Requests to process: " << benchmarkData.progArgs.totalNumberOfRequests << endl;
        std::cout << "Total Requests to process by each thread: " << benchmarkData.remainderNumberOfRequests << endl;
    }
    std::cout << "Total Requests processed on this node: " << benchmarkData.totalMessagesProcessed << endl;
    std::cout << "Processed READ/APPEND: " << benchmarkData.amountReadsSent << "/" << benchmarkData.amountAppendsSent << endl;
    std::cout << "Sent READ ratio: " << to_string((static_cast<double>(benchmarkData.amountReadsSent) / static_cast<double>(benchmarkData.totalMessagesProcessed)) * 100) << "% (shoud " << benchmarkData.progArgs.probabilityOfRead << "%)" << endl;
    std::cout << "Total State Requests: " << benchmarkData.amountStateRequests << endl;
    std::cout << "Total failed READ Requests: " << benchmarkData.amountReadsErrors << endl;
    std::cout << "Sequencer Number: " << benchmarkData.lastSequencerNumber << endl;
    std::cout << "Total time taken: " << benchmarkData.totalExecutionTime.count() << "s" << endl;
    if (benchmarkTime)
        std::cout << "Operations per Second: " << (static_cast<double>(benchmarkData.totalMessagesProcessed) / benchmarkData.totalExecutionTime.count()) << " Op/s" << endl;
    else
        std::cout << "Operations per Second: " << (static_cast<double>(benchmarkData.progArgs.totalNumberOfRequests) / benchmarkData.totalExecutionTime.count()) << " Op/s" << endl;
    std::cout << "Append Latency total count: " << to_string(benchmarkData.appendlatency.count()) << " min: " << ( benchmarkData.appendlatency.min() / benchmarkData.latencyFactor ) << "us ; max: " << ( benchmarkData.appendlatency.max() / benchmarkData.latencyFactor ) << "us ; avg: " << ( benchmarkData.appendlatency.avg() / benchmarkData.latencyFactor ) << "us" << endl;
        cout << "Append Latency: .50: " << ( benchmarkData.appendlatency.perc(0.50) / benchmarkData.latencyFactor ) << "us ; .99: " << ( benchmarkData.appendlatency.perc(0.99) / benchmarkData.latencyFactor ) << "us ; .999: " << ( benchmarkData.appendlatency.perc(0.999) / benchmarkData.latencyFactor ) << "us ; .9999: " << ( benchmarkData.appendlatency.perc(0.9999) / benchmarkData.latencyFactor ) << "us" << endl;
    std::cout << "-------------------------------------" << endl;
}

void printToCSV() {
    // Check if file already exists for printing header
    ifstream f(benchmarkData.progArgs.csvName);
    bool newFile = f.good();
    string replicationType{};
    #ifdef CRAQ
        #ifdef UCR
        replicationType = "UCR";
        #else
        replicationType = "CRAQ";
        #endif
    #elif CR
    replicationType = "CR";
    #endif

    std::array<size_t, 5> opsbyNode{};
    opsbyNode[benchmarkData.progArgs.nodeID] = benchmarkData.totalMessagesProcessed;

    // Open CSV file in append mode
    std::ofstream file( benchmarkData.progArgs.csvName, std::ios::app );

    if (!newFile)
        file << "reads,appends,rops,aops,ops,probRead,time,valueSize,threads,chainNodes,maxInFlight,alat_50,alat_95,alat_99,rlat_50,rlat_95,rlat_99,ops_node1,ops_node2,ops_node3,ops_node4,ops_node5,replType" << endl;

    file << benchmarkData.amountReadsSent << "," << benchmarkData.amountAppendsSent << "," <<  (static_cast<double>(benchmarkData.amountReadsSent) / benchmarkData.totalExecutionTime.count()) << "," << (static_cast<double>(benchmarkData.amountAppendsSent) / benchmarkData.totalExecutionTime.count()) << "," << (static_cast<double>(benchmarkData.totalMessagesProcessed) / benchmarkData.totalExecutionTime.count()) << "," << benchmarkData.progArgs.probabilityOfRead << "," << benchmarkData.totalExecutionTime.count() << "," << benchmarkData.progArgs.valueSize << "," << benchmarkData.progArgs.amountThreads << "," << benchmarkData.progArgs.chainNodes << "," << benchmarkData.progArgs.messageInFlightCap << "," << ( benchmarkData.appendlatency.perc(0.50) / benchmarkData.latencyFactor ) << "," <<  ( benchmarkData.appendlatency.perc(0.95) / benchmarkData.latencyFactor ) << "," << ( benchmarkData.appendlatency.perc(0.99) / benchmarkData.latencyFactor ) <<  "," << ( benchmarkData.readlatency.perc(0.50) / benchmarkData.latencyFactor ) << "," <<  ( benchmarkData.readlatency.perc(0.95) / benchmarkData.latencyFactor ) << "," << ( benchmarkData.readlatency.perc(0.99) / benchmarkData.latencyFactor ) <<  "," << opsbyNode[0] << "," << opsbyNode[1] << "," << opsbyNode[2] << "," << opsbyNode[3] << "," << opsbyNode[4] << "," << replicationType << endl;
    
    // Close the file
    file.close();
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
                benchmarkData.progArgs.probabilityOfRead = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 's': // Size value
                benchmarkData.progArgs.valueSize = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'c': // Chain Nodes
                benchmarkData.progArgs.chainNodes = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'j': // Cap
                benchmarkData.progArgs.messageInFlightCap = std::stoul(&(argv[i][3]), nullptr, 0);
                break;
            case 'h': // Time to run program in seconds
                benchmarkData.progArgs.time = std::chrono::seconds(std::stoul(&(argv[i][3]), nullptr, 0));
                benchmarkData.progArgs.totalNumberOfRequests = 1000000000;
                benchmarkTime = true;
                break;
            case 'f': // csv file name
                if (strlen(&(argv[i][3])) >= sizeof(benchmarkData.progArgs.csvName)) {
                    std::cout << "filename too long: " << argv[i][3] << endl;
                    exit(1);
                }
                strcpy(benchmarkData.progArgs.csvName, &(argv[i][3]));
                break;
        }
    }

    benchmarkData.remainderNumberOfRequests = benchmarkData.progArgs.totalNumberOfRequests / benchmarkData.progArgs.amountThreads;
    benchmarkData.startBenchmark = &startBenchmark;
    #ifndef BENCHMARK
    std::cout << "Input Parameters: nodeID: " << to_string(benchmarkData.progArgs.nodeID) << " nodeType: " << benchmarkData.progArgs.nodeType << " activeMode: " << benchmarkData.progArgs.activeMode << " amountThreads: " << benchmarkData.progArgs.amountThreads << " totalNumOfRequests: " << benchmarkData.progArgs.totalNumberOfRequests << " RequestsPerThread: " << benchmarkData.remainderNumberOfRequests  << " Probability of Reads: " << benchmarkData.progArgs.probabilityOfRead << " Time: " << std::to_string(benchmarkData.progArgs.time.count()) << " valueSize: " << benchmarkData.progArgs.valueSize << endl;
    #endif
}


int main(int argc, char** argv) {
    #ifndef BENCHMARK
    std::cout << "-------------------------------------" << endl;
    std::cout << "Init everything..." << endl;
    #endif

    parser(argc, argv);
    startBenchmark.lock();

    // Set Log file name to nodeID
    char *poolPath = static_cast<char *>(malloc(strlen(POOL_PATH) + 1)); 
    strncpy(poolPath, POOL_PATH, strlen(POOL_PATH) + 1);
    poolPath[strlen(POOL_PATH) - 5] = '0' + static_cast<char>(benchmarkData.progArgs.nodeID);

    #ifndef DPDK_CLUSTER
        switch(benchmarkData.progArgs.nodeType) {
            case HEAD: localNode = new SharedLogNode<REPLICATION>(benchmarkData.progArgs.nodeType, benchmarkData.progArgs.nodeID, poolPath, BILL_URI, std::string(), NARDOLE_URI, NARDOLE_URI, &benchmarkData); break;
            case MIDDLE: break;
            case TAIL: localNode = new SharedLogNode<REPLICATION>(benchmarkData.progArgs.nodeType, benchmarkData.progArgs.nodeID, poolPath, NARDOLE_URI, BILL_URI, std::string(), std::string(), &benchmarkData); break;
        }
    #else
        string headNode = ROSE_URI;
        string middleFirstNode = CLARA_URI;
        string middleSecondNode = DONNA_URI;
        string middleThirdNode = AMY_URI;
        //string tailNode = MARTHA_URI;
        string tailNode = DONNA_URI;

        if (benchmarkData.progArgs.nodeID == 0)
            localNode = new SharedLogNode<REPLICATION>(HEAD, 0, poolPath, headNode, std::string(), middleFirstNode, tailNode, &benchmarkData); 

        switch(benchmarkData.progArgs.chainNodes) {
            case 3: {
                switch(benchmarkData.progArgs.nodeID) {
                    case 1: localNode = new SharedLogNode<REPLICATION>(MIDDLE, 1, poolPath, middleFirstNode, headNode, tailNode, tailNode, &benchmarkData); break;
                    case 2: localNode = new SharedLogNode<REPLICATION>(TAIL, 2, poolPath, tailNode, headNode, std::string(), std::string(), &benchmarkData ); break;
                }
            };
            break;
            case 4: {
                switch(benchmarkData.progArgs.nodeID) {
                    case 1: localNode = new SharedLogNode<REPLICATION>(MIDDLE, 1, poolPath, middleFirstNode, headNode, middleSecondNode, tailNode, &benchmarkData); break;
                    case 2: localNode = new SharedLogNode<REPLICATION>(MIDDLE, 2, poolPath, middleSecondNode, headNode, tailNode, tailNode, &benchmarkData); break;
                    case 3: localNode = new SharedLogNode<REPLICATION>(TAIL, 3, poolPath, tailNode, headNode, std::string(), std::string(), &benchmarkData ); break;
                }
            };
            break;
            case 5: {
                switch(benchmarkData.progArgs.nodeID) {
                    case 1: localNode = new SharedLogNode<REPLICATION>(MIDDLE, 1, poolPath, middleFirstNode, headNode, middleSecondNode, tailNode, &benchmarkData); break;
                    case 2: localNode = new SharedLogNode<REPLICATION>(MIDDLE, 2, poolPath, middleSecondNode, headNode, middleThirdNode, tailNode, &benchmarkData); break;
                    case 3: localNode = new SharedLogNode<REPLICATION>(MIDDLE, 3, poolPath, middleThirdNode, headNode, tailNode, tailNode, &benchmarkData); break;
                    case 4: localNode = new SharedLogNode<REPLICATION>(TAIL, 4, poolPath, tailNode, headNode, std::string(), std::string(), &benchmarkData ); break;
                }
            };
            break;
        }
    #endif

    if (benchmarkTime)
        start_benchmark_time();
    else
        start_benchmark_operations();

    #ifndef BENCHMARK
    std::cout << "...Finished benchmarking" << endl;
    std::cout << "-------------------------------------" << endl;
    #endif

    #ifdef BENCHMARK
    printToCSV();
    #else
    printbenchmarkData();
    #endif
    printbenchmarkData();
}
