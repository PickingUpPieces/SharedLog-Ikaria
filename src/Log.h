#ifndef REPLICATIONNODE_LOG_H
#define REPLICATIONNODE_LOG_H
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <libpmemlog.h>
#include "common_info.h"
using namespace std;

struct LogEntry
{
    uint64_t dataLength;
    char data[LOG_BLOCK_SIZE];
};


class Log {
    private:
        const uint64_t logTotalSize_;
        const uint64_t logBlockSize_;
        const char *pathToLog_;
        PMEMlogpool *plp_;

        void init();

    public:
        Log(uint64_t logTotalSize, uint64_t logBlockSize, const char *pathToLog);
        void append(uint64_t logOffset, void *data);
        void* read(uint64_t logOffset, int *logEntryLength);
        void terminate();
};

#endif // REPLICATIONNODE_LOG_H