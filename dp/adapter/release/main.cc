#include "adaptee_base.h"
#include "adapter.h"
#include "old_class.h"
#include "target_base.h"

using namespace release;

int main() {
  AdapteeBase* adaptee = new OldClass();
  TargetBase* target = new Adapter(adaptee);
  target->Process();
  delete adaptee;
  delete target;
}