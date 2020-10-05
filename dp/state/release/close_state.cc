#include "close_state.h"
#include <iostream>
#include "connect_state.h"
#include "open_state.h"

namespace release {
NetworkState* CloseState::instance_ = nullptr;

NetworkState* CloseState::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new CloseState();
  }
  return instance_;
}

void CloseState::Operation1() {
  std::cout << "CloseState::Operation1\n";
  set_next(ConnectState::GetInstance());
}

void CloseState::Operation2() {
  std::cout << "CloseState::Operation2\n";
  set_next(OpenState::GetInstance());
}

}  // namespace release