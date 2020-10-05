#ifndef ADAPTER_RELEASE_ADAPTER_H
#define ADAPTER_RELEASE_ADAPTER_H

#include "adaptee_base.h"
#include "target_base.h"
namespace release {
class Adapter : public TargetBase {
 public:
  Adapter(AdapteeBase* adaptee) : adaptee_(adaptee) {}
  void Process() override {
    int data = adaptee_->bar();
    adaptee_->foo(data);
  }

 protected:
  AdapteeBase* adaptee_;
};
}  // namespace release
#endif