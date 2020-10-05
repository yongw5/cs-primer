#ifndef CHAIN_RELEASE_REQUEST_H
#define CHAIN_RELEASE_REQUEST_H

#include <string>
#include "request_type.h"

namespace release {
class Request {
 public:
  Request(const std::string& desc, RequestType type)
      : description_(desc), type_(type) {}
  RequestType type() const { return type_; }
  const std::string& description() const { return description_; }

 private:
  std::string description_;
  RequestType type_;
};
}  // namespace release

#endif