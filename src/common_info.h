#ifndef REPLICATIONNODE_COMMON_INFO_H
#define REPLICATIONNODE_COMMON_INFO_H

#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

#include <string>

static const std::string hostname_head = "131.159.102.1";
static const int port_head = 31850;
static const std::string hostname_tail = "131.159.102.2"; 
static const int port_tail = 31850;

/* size of the pmemlog pool -- 1 GB = 2^30 */
#define POOL_SIZE ((off_t)(1 << 30))
/* log block size in KB */
#define LOG_BLOCK_SIZE 4096
/* Path to the Pool file */
#define POOL_PATH "/home/vincent/pmem/log-test-0.log"


#endif // REPLICATIONNODE_COMMON_INFO_H
