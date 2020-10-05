#include "connect_state.h"
#include <iostream>
#include "close_state.h"
#include "open_state.h"

namespace release {
NetworkState* ConnectState::instance_ = nullptr;

NetworkState* ConnectState::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new ConnectState();
  }
  return instance_;
}

void ConnectState::Operation1() {
  std::cout << "ConnectState::Operation1\n";
  set_next(OpenState::GetInstance());
}

void ConnectState::Operation2() {
  std::cout << "ConnectState::Operation2\n";
  set_next(CloseState::GetInstance());
}

}  // namespace release