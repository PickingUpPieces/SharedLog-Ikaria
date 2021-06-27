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


#endif // REPLICATIONNODE_COMMON_INFO_H