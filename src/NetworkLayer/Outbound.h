#ifndef REPLICATIONNODE_OUTBOUND_H
#define REPLICATIONNODE_OUTBOUND_H

#include "rpc.h"
#include "common_info.h"
#include <string>
using namespace std;

class NetworkManager;

class Outbound {
    friend void cont_func(void *context, void *tag);

    private:
        int8_t sessionNum_;
        NetworkManager *NetworkManager_;
        erpc::Rpc<erpc::CTransport> *rpc_;

    public:
        Outbound(string connectURI, NetworkManager *NetworkManager, erpc::Rpc<erpc::CTransport> *rpc);
        void terminate();
        void send_message(Message *message);
        void connect(string connectURI);

};

#endif //REPLICATIONNODE_OUTBOUND_H