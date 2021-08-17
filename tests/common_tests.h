#ifndef COMMONTESTS_H 
#define COMMONTESTS_H 
#include "rpc.h"
#include <shared_mutex>

/* Holds the program input arguments */
struct ProgArgs {
    NodeType nodeType{HEAD}; // -n
    uint8_t nodeID{0}; // -i
    bool activeMode{true}; // -a
    size_t amountThreads{1}; // -t
    size_t totalNumberOfRequests{10000000}; // -m
    std::chrono::duration<long> time{};
    int probabilityOfRead{50}; // -r ; Between 0 - 100
    size_t valueSize{64}; // -v ; Bytes
    char csvName[32];
};

/* Collects the measured data */
struct BenchmarkData {
    ProgArgs progArgs;
    std::mutex *startBenchmark{nullptr};
    size_t remainderNumberOfRequests{1000000};
    size_t lastSequencerNumber{0}; 
    size_t amountReadsSent{0};
    size_t amountAppendsSent{0};
    size_t totalMessagesProcessed{0};
    std::chrono::duration<double> totalExecutionTime{};
    double operationsPerSecond{0.0}; // Op/s
    uint64_t highestKnownLogOffset{1}; // So reads are performed on offset smaller than this
};

#endif
