#include "network_processor.h"

namespace release {
void NetworkProcessor::Operation1() {
  state_->Operation1();
  state_ = state_->next();
}

void NetworkProcessor::Operation2() {
  state_->Operation2();
  state_ = state_->next();
}
}  // namespace release