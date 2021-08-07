#ifndef HELPERFUNCTIONS_H 
#define HELPERFUNCTIONS_H 
#include <random>
#include "rpc.h"
#include "common_info.h"

class ReplicationManager;

void readLog(ReplicationManager *rp, uint64_t logOffset);
void appendLog(ReplicationManager *rp, LogEntryInFlight *logEntryInFlight, size_t dataLength);
LogEntryInFlight generate_random_logEntryInFlight(uint64_t totalSize);

#endif 