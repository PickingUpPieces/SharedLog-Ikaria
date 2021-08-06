#include <iostream>
#include "Log.h"

static once_flag plp_once;
static uint64_t logEntryTotalSize = sizeof(LogEntry);
static PMEMlogpool *plp_;

/**
 * Constructs the Log
 * @param logTotalSize Total Size of the Log in Bytes
 * @param logBlockSize Size of one log block in the log
 * @param pathToLog Path where the log file should reside
*/
Log::Log(uint64_t logTotalSize, uint64_t logBlockSize, const char *pathToLog):
    logTotalSize_{logTotalSize},
    logBlockSize_{logBlockSize},
    pathToLog_{pathToLog}
{
    std::call_once(plp_once, init, pathToLog, logTotalSize);
}


Log::~Log() {
	if (plp_ != NULL)
	    pmemlog_close(plp_);
}


/**
 * Creates a new / Opens an existing log and creates the Log Handler plp_
*/
static void init(const char *pathToLog, uint64_t logTotalSize) {
	/* create the pmemlog pool or open it if it already exists */
	plp_ = pmemlog_create(pathToLog, logTotalSize, 0666);

	if (plp_ == NULL &&
	    (plp_ = pmemlog_open(pathToLog)) == NULL) {
		perror(pathToLog);
		exit(EXIT_FAILURE);
	}
}

/**
 * Appends a new LogEntry to the log
 * @param logOffset The log entry number of the new LogEntry
 * @param logEntry Pointer to the LogEntry which should be logged
*/
void Log::append(uint64_t logOffset, LogEntry *logEntry) {
    DEBUG_MSG("Log.append(Offset: " << std::to_string(logOffset) << " ; LogEntry: dataLength: " << std::to_string(logEntry->dataLength) << " ; data: " << logEntry->data << ")");
    
	/* Only copy the real entry size */
	// FIXME: When totalSize % 8 != 0 then the data is gonna be aligned. Is this a problem?
	uint64_t totalLogEntrySize = logEntry->dataLength + sizeof(logEntry->dataLength);

	if (pmemlog_write(plp_, logEntry, totalLogEntrySize, logOffset * logEntryTotalSize) < 0) {
		perror("pmemlog_write");
		exit(EXIT_FAILURE);
	}
}

/**
 * Returns the pointer to the LogEntry for the requested logOffset
 * @param logOffset The log entry number 
 * @param logEntryLength Pointer for the logSize of the requested log, which is returned back to the ReplicationManager
*/
pair<LogEntry *, uint64_t> Log::read(uint64_t logOffset) {
    LogEntry *logEntry = static_cast<LogEntry *>(pmemlog_read(plp_, logOffset * logEntryTotalSize));
    DEBUG_MSG("Log.read(Offset: " << std::to_string(logOffset) << " ; LogEntry: dataLength: " << std::to_string(logEntry->dataLength) << " ; data: " << logEntry->data << ")");

	return make_pair(logEntry, 	logEntry->dataLength + sizeof(logEntry->dataLength));
}

/**
 * Terminates the Log
*/
void Log::terminate() {
}