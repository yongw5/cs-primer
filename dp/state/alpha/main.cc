#include "network_processor.h"
#include "network_state.h"
using namespace alpha;

int main() {
  {
    NetworkProcessor processor(NetworkClose);
    processor.Operation1();
  }

  {
    NetworkProcessor processor(NetworkClose);
    processor.Operation2();
  }
}