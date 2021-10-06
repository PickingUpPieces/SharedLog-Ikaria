#ifndef REPLICATIONNODE_OUTBOUND_H
#define REPLICATIONNODE_OUTBOUND_H

#include <string>
#include "rpc.h"
#include "common_info.h"
#include "NetworkManager.h"
using namespace std;

template<class Replication>
class NetworkManager;

template<class Replication>
class Outbound {
    friend void cont_func(void *context, void *tag);
    friend NetworkManager<Replication>;

    private:
        int8_t sessionNum_;
        NetworkManager<Replication> *networkManager_;
        erpc::Rpc<erpc::CTransport> *rpc_;
        void send_message(Message *message);
        void connect();

    public:
        Outbound(string connectURI, uint8_t erpcID, NetworkManager<Replication> *networkManager, erpc::Rpc<erpc::CTransport> *rpc);
};

#endif //REPLICATIONNODE_OUTBOUND_H
