#include <iostream>
#include "Log.h"

std::once_flag flag;
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
    std::call_once(flag, init, pathToLog, logTotalSize);
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
 * @param log Pointer to the LogEntry which should be logged
*/
void Log::append(uint64_t logOffset, void *log) {
    LogEntry *logEntry = (LogEntry *) log;
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
void* Log::read(uint64_t logOffset, size_t *logEntryLength) {
    void *returnRead = pmemlog_read(plp_, logOffset * logEntryTotalSize);
    // TODO: Check if first byte (LogEntry.dataLength) is 0 -> read failed

    LogEntry *logEntry = (LogEntry *) returnRead;
	uint64_t totalLogEntrySize = logEntry->dataLength + sizeof(logEntry->dataLength);
    DEBUG_MSG("Log.read(Offset: " << std::to_string(logOffset) << " ; LogEntry: dataLength: " << std::to_string(logEntry->dataLength) << " ; data: " << logEntry->data << ")");

    *logEntryLength = totalLogEntrySize;
    return returnRead;
}

/**
 * Terminates the Log
*/
void Log::terminate() {
    DEBUG_MSG("Log.terminate()");

	if (plp_ == NULL) {
		perror("No log is open!");
		exit(EXIT_FAILURE);
	} else 
	    pmemlog_close(plp_);

    exit(EXIT_SUCCESS);
}

/* DEBUG functions */

/* Struct for passing informations between the callback function calls */
struct PmemlogWalkArg {
	uint64_t currentLogOffset;
	string *data;
	bool logsSavedWithLogOffset;
};

/**
 * Callback function for the pmemlog_walk function. Gets called for every LogEntry saved in the current log.
 * The Log is blocked when this callback function is running.
 * @param buf Pointer to logEntry
 * @param len Length of buf, in this case LOG_BLOCK_TOTAL_SIZE
 * @param arg context argument passed by pmemlog_walk. Here: Struct PmemlogWalkArg
 */
static int callbackWalkLog(const void *buf, size_t len, void *arg) {
	PmemlogWalkArg *pmemlogWalkArg = (PmemlogWalkArg *) arg;
	(void) len;

	while (true) {
		LogEntry *logEntry = (LogEntry *) (((uint8_t *) buf) + (pmemlogWalkArg->currentLogOffset * LOG_BLOCK_TOTAL_SIZE));
		string compString = "";

		if (pmemlogWalkArg->currentLogOffset == 0) {
			pmemlogWalkArg->currentLogOffset++; 
			continue;
		}

		if (pmemlogWalkArg->logsSavedWithLogOffset)
			compString = *(pmemlogWalkArg->data) + "-ID-" + std::to_string(pmemlogWalkArg->currentLogOffset);
		else
			compString = *(pmemlogWalkArg->data); 

		string dataString(logEntry->data);
		DEBUG_MSG("generatedString vs returnedString: '" << compString << "' vs '" << dataString << "'");

		if (compString.compare(dataString) != 0) {
			pmemlogWalkArg->currentLogOffset--;
			/* Stop walking through the log */
			return 1;
		}
		pmemlogWalkArg->currentLogOffset++;
	}

	return 0;
} 

/**
 * Handles an incoming response for a previous send out SETUP message
 * @param data Data string which is written in every LogEntry
 * @param logsSavedWithLogOffset True, if the IDs are added at the end of the entry
 */
uint64_t Log::validate_log(string *data, bool logsSavedWithLogOffset) {
	
	PmemlogWalkArg pmemlogWalkArg{0, data, logsSavedWithLogOffset};
	pmemlog_walk(plp_, 0, callbackWalkLog, &pmemlogWalkArg);
	return pmemlogWalkArg.currentLogOffset;
}
