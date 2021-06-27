#include "Inbound.h"
#include "ReplicationManager.h"
#include "common_networkLayer.h"
#include "rpc.h"
#include <iostream>

Inbound::Inbound(erpc::Nexus *nexus, uint8_t erpc_id, ReplicationManager *ReplicationManager) {
  this->erpcID_ = erpc_id; 
  this->ReplicationManager_ = ReplicationManager;
  Inbound::init(nexus);
  this->rpc_ = new erpc::Rpc<erpc::CTransport>(nexus, this, this->erpcID_, nullptr);

  cout << "Inbound init done" << endl;
}


// Request handler for read requests
void req_handler_read(erpc::ReqHandle *req_handle, void *context) {

  auto inbound = static_cast<Inbound *>(context);

  // Pre allocated MsgBuf -> Single packet response
  erpc::MsgBuffer resp = req_handle->pre_resp_msgbuf;
  const erpc::MsgBuffer *req = req_handle->get_req_msgbuf();

  // Call read method
  size_t readLength = inbound->ReplicationManager_->read(req->buf, resp.buf);

  erpc::Rpc<erpc::CTransport>::resize_msg_buffer(&resp, readLength);

// Enqueue the request --> Will be send when returning to the while(true)
  inbound->rpc_->enqueue_response(req_handle, &resp);
}


// Request handler for append requests
void req_handler_append(erpc::ReqHandle *req_handle, void *context) {

  auto inbound = static_cast<Inbound *>(context);
  const erpc::MsgBuffer *req = req_handle->get_req_msgbuf();
  inbound->ReplicationManager_->append(req->buf, req->get_data_size());

  // Prepare ACK message
  erpc::MsgBuffer resp = req_handle->pre_resp_msgbuf;
  inbound->rpc_->resize_msg_buffer(&resp, 16);
  sprintf(reinterpret_cast<char *>(resp.buf), "ACK");
  // Enqueue the request --> Will be send when returned
  inbound->rpc_->enqueue_response(req_handle, &resp);
}

void Inbound::run_event_loop(int numberOfRuns) {
  for (int i = 0; i < numberOfRuns; i++)
    rpc_->run_event_loop_once();
}


void Inbound::init(erpc::Nexus *nexus) {

  // Register request handler for Request Type ReqTypeRead
  if (nexus->register_req_func(ReqTypeRead, req_handler_read)) {
    cerr << "Failed to initialize ReqTypeRead" << endl;
    std::terminate();
  } 

    // Register request handler for Request Type ReqTypeAppend
  if (nexus->register_req_func(ReqTypeAppend, req_handler_read)) {
    cerr << "Failed to initialize ReqTypeAppend" << endl;
    std::terminate();
  }
}

void Inbound::terminate() {}