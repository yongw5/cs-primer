#ifndef ADAPTER_RELEASE_ADAPTEE_BASE_H
#define ADAPTER_RELEASE_ADAPTEE_BASE_H

namespace release {
class AdapteeBase {
 public:
  virtual void foo(const int data) = 0;
  virtual int bar() = 0;
};
}  // namespace release

#endif