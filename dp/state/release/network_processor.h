#ifndef STATE_RELEASE_NETOWRK_PROCESSOR_H
#define STATE_RELEASE_NETOWRK_PROCESSOR_H
#include <iostream>
#include "network_state.h"
namespace release {
class NetworkProcessor {
 public:
  explicit NetworkProcessor(NetworkState* state) : state_(state) {}
  void Operation1();
  void Operation2();

 private:
  NetworkState* state_;
};
}  // namespace release

#endif