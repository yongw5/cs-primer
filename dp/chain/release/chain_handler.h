#ifndef CHAIN_RELEASE_CHAIN_HANDLER_H
#define CHAIN_RELEASE_CHAIN_HANDLER_H

namespace release {
class Request;
class ChainHandler {
 public:
  ChainHandler() : next_chain_(nullptr) {}
  void set_next(ChainHandler* next) { next_chain_ = next; }
  void Handle(const Request& req) {
    if (CanHandleRequest(req)) {
      ProcessRequest(req);
    } else {
      SendRequestToNextHandler(req);
    }
  }

 protected:
  virtual bool CanHandleRequest(const Request& req) = 0;
  virtual void ProcessRequest(const Request& req) = 0;

 private:
  void SendRequestToNextHandler(const Request& req) {
    if (nullptr != next_chain_) {
      next_chain_->Handle(req);
    }
  }
  ChainHandler* next_chain_;
};

}  // namespace release

#endif