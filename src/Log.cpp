#include "Log.h"
#include "common_info.h"
#include <iostream>

Log::Log(uint64_t logTotalSize, uint64_t logBlockSize, char *pathToLog):
    logTotalSize_{logTotalSize},
    logBlockSize_{logBlockSize},
    pathToLog_{pathToLog},
    plp_{nullptr}
{
    init();
}

void Log::init() {
	/* create the pmemlog pool or open it if it already exists */
	plp_ = pmemlog_create(pathToLog_, logTotalSize_, 0666);

	if (plp_ == NULL &&
	    (plp_ = pmemlog_open(pathToLog_)) == NULL) {
		perror(pathToLog_);
		exit(EXIT_FAILURE);
	}
}

void Log::append(uint64_t logOffset, void *log) {
    DEBUG_MSG("Log.append(Offset: " << std::to_string(logOffset) << ")");
    LogEntry *logEntry = (LogEntry *) log;
    
	uint64_t totalLogEntrySize = logEntry->dataLength + sizeof(logEntry->dataLength);
    DEBUG_MSG("LogEntry.dataLength: " << std::to_string(logEntry->dataLength) << " ; LogEntry.data: " << logEntry->data);

	if (pmemlog_write(plp_, logEntry, totalLogEntrySize, logOffset) < 0) {
		perror("pmemlog_write");
		exit(EXIT_FAILURE);
	}
}


void* Log::read(uint64_t logOffset, uint64_t *logEntryLength) {
    DEBUG_MSG("Log.read(Offset: " << std::to_string(logOffset) << ")");

    void *returnRead = pmemlog_read(plp_, logOffset);
    // TODO: Check if first byte (LogEntry.dataLength) is 0 -> read failed

    LogEntry *logEntry = (LogEntry *) returnRead;
	uint64_t totalLogEntrySize = logEntry->dataLength + sizeof(logEntry->dataLength);
    DEBUG_MSG("LogEntry.dataLength: " << std::to_string(logEntry->dataLength) << " ; LogEntry.data: " << logEntry->data);

    *logEntryLength = totalLogEntrySize;
    return returnRead;
}

void Log::terminate() {
    DEBUG_MSG("Log.terminate()");

	if (plp_ == NULL) {
		perror("No log is open!");
		exit(EXIT_FAILURE);
	} else 
	    pmemlog_close(plp_);

    exit(EXIT_SUCCESS);
}


