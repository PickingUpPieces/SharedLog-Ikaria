#ifndef REPLICATIONNODE_OUTBOUND_H
#define REPLICATIONNODE_OUTBOUND_H

#include <string>
#include "rpc.h"
#include "common_info.h"
#ifdef CR
#include "CRNetworkManager.h"
#else
#include "CRAQNetworkManager.h"
#endif
using namespace std;

class NetworkManager;

class Outbound {
    friend void cont_func(void *context, void *tag);
    friend NetworkManager;

    private:
        int8_t sessionNum_;
        NetworkManager *networkManager_;
        erpc::Rpc<erpc::CTransport> *rpc_;
        void send_message(Message *message);
        void connect();

    public:
        Outbound(string connectURI, uint8_t erpcID, NetworkManager *networkManager, erpc::Rpc<erpc::CTransport> *rpc);
};

#endif //REPLICATIONNODE_OUTBOUND_H
