#include "open_state.h"
#include <iostream>
#include "close_state.h"
#include "connect_state.h"

namespace release {
NetworkState* OpenState::instance_ = nullptr;

NetworkState* OpenState::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new OpenState();
  }
  return instance_;
}

void OpenState::Operation1() {
  std::cout << "OpenState::Operation1\n";
  set_next(CloseState::GetInstance());
}

void OpenState::Operation2() {
  std::cout << "OpenState::Operation2\n";
  set_next(ConnectState::GetInstance());
}

}  // namespace release