#ifndef REPLICATIONNODE_LOG_H
#define REPLICATIONNODE_LOG_H
#include <stdint.h>
#include <unistd.h>
#include <string>
#include "libpmemlog.h"
#include "common_info.h"
using namespace std;

static void init(const char *pathToLog, uint64_t logTotalSize);

class Log {
    private:
        const uint64_t logTotalSize_;
        const uint64_t logBlockSize_;
        const char *pathToLog_;


    public:
        Log(uint64_t logTotalSize, uint64_t logBlockSize, const char *pathToLog);
        void append(uint64_t logOffset, void *data);
        void* read(uint64_t logOffset, size_t *logEntryLength);
        void terminate();
        uint64_t validate_log(string *data, bool logsSavedWithLogOffset);
};

#endif // REPLICATIONNODE_LOG_H
