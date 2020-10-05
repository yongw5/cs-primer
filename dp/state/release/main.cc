#include "close_state.h"
#include "network_processor.h"
using namespace release;

int main() {
  {
    NetworkState* state = CloseState::GetInstance();
    NetworkProcessor processor(state);
    processor.Operation1();
  }

  {
    NetworkState* state = CloseState::GetInstance();
    NetworkProcessor processor(state);
    processor.Operation2();
  }
}