#ifndef COMMONTESTS_H 
#define COMMONTESTS_H 
#include "rpc.h"
#include "helperFunctions.h"

/* Holds the program input arguments */
struct ProgArgs {
    NodeType nodeType{HEAD}; // -n
    bool activeMode{true}; // -a
    int amountThreads{1}; // -t
    size_t totalNumberOfRequests{10000000}; // -m
    size_t percentile{100}; // -p ; Only works for very high percentiles > 90. Depends on the totalNumberOfRequests.
    size_t percentileNumberOfRequests{10000000};
    int probabilityOfRead{50}; // -r ; Between 0 - 100
    size_t valueSize{64}; // -v ; Bytes
};

/* Collects the measured data */
struct BenchmarkData {
    ProgArgs progArgs;
    size_t remainderNumberOfRequests{1000000};
    size_t messagesInFlight{0}; 
    size_t amountReadsSent{0};
    size_t amountAppendsSent{0};
    size_t totalMessagesProcessed{0};
    std::chrono::duration<double> totalExecutionTime{};
    double operationsPerSecond{0.0}; // Op/s
    uint64_t highestKnownLogOffset{1}; // So reads are performed on offset smaller than this
};

#endif
