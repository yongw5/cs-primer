#include "network_processor.h"
#include <iostream>
namespace alpha {
void NetworkProcessor::Operation1() {
  switch (state_) {
    case NetworkOpen:
      std::cout << "NetworkProcessor::Operation1() on NetworkOpen\n";
      state_ = NetworkClose;
      break;
    case NetworkClose:
      std::cout << "NetworkProcessor::Operation1() on NetworkClose\n";
      state_ = NetworkConnect;
      break;
    case NetworkConnect:
      std::cout << "NetworkProcessor::Operation1() on NetworkConnect\n";
      state_ = NetworkOpen;
      break;
    default:
      std::cout << "Invalid State\n";
      break;
  }
}

void NetworkProcessor::Operation2() {
  switch (state_) {
    case NetworkOpen:
      std::cout << "NetworkProcessor::Operation2() on NetworkOpen\n";
      state_ = NetworkConnect;
      break;
    case NetworkClose:
      std::cout << "NetworkProcessor::Operation2() on NetworkClose\n";
      state_ = NetworkOpen;
      break;
    case NetworkConnect:
      std::cout << "NetworkProcessor::Operation2() on NetworkConnect\n";
      state_ = NetworkClose;
      break;
    default:
      std::cout << "Invalid State\n";
      break;
  }
}
}  // namespace alpha