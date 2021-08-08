#ifndef REPLICATIONNODE_LOG_H
#define REPLICATIONNODE_LOG_H
#include <stdint.h>
#include <unistd.h>
#include <string>
#include "libpmemlog.h"
#include "common_info.h"
using namespace std;

static void init(const char *pathToLog, uint64_t logTotalSize);
struct LogEntry {
    uint64_t dataLength;
    char data[LOG_BLOCK_DATA_SIZE];
};

class Log {
    private:
        const uint64_t logTotalSize_;
        const uint64_t logBlockSize_;
        const char *pathToLog_;

    public:
        Log(uint64_t logTotalSize, uint64_t logBlockSize, const char *pathToLog);
        ~Log();
        void append(uint64_t logOffset, LogEntry *logEntry);
        pair<LogEntry *, uint64_t> read(uint64_t logOffset);
};

#endif // REPLICATIONNODE_LOG_H
