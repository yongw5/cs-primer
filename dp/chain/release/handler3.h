#ifndef CHAIN_RELEASE_HANDLER3_H
#define CHAIN_RELEASE_HANDLER3_H

#include <iostream>
#include "chain_handler.h"
#include "request.h"
#include "request_type.h"

namespace release {
class Handler3 : public ChainHandler {
 protected:
  bool CanHandleRequest(const Request& req) override {
    return req.type() == RequestType::REQ_HANDLER3;
  }
  void ProcessRequest(const Request& req) override {
    std::cout << "Handler3::ProcessRequest: " << req.description() << std::endl;
  }
};
}  // namespace release

#endif