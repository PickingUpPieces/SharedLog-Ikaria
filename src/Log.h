#ifndef REPLICATIONNODE_LOG_H
#define REPLICATIONNODE_LOG_H
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <libpmemlog.h>
using namespace std;

struct LogEntry
{
    uint64_t dataLength;
    char * data;
};

/* size of the pmemlog pool -- 1 GB = 2^30 */
#define POOL_SIZE ((off_t)(1 << 30))
/* log block size in KB */
#define BLOCK_SIZE 20
/* Path to the Pool file */
#define POOL_PATH "/pmem/log-test-0.log"


class Log {
    private:
        const uint64_t logTotalSize_;
        const uint64_t logBlockSize_;
        const char *pathToLog_;
        PMEMlogpool *plp_;

        void init();

    public:
        Log(uint64_t logTotalSize, uint64_t logBlockSize, char *pathToLog);
        void append(uint64_t logOffset, void *data);
        void* read(uint64_t logOffset, uint64_t *logEntryLength);
        void terminate();
};

#endif // REPLICATIONNODE_LOG_H