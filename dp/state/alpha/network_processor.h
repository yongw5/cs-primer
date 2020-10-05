#ifndef STATE_ALPHA_NETOWRK_PROCESSOR_H
#define STATE_ALPHA_NETOWRK_PROCESSOR_H

#include "network_state.h"
namespace alpha {
class NetworkProcessor {
 public:
  explicit NetworkProcessor(NetworkState state) : state_(state) {}
  void Operation1();
  void Operation2();

 private:
  NetworkState state_;
};
}  // namespace alpha

#endif