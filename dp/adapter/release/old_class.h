#ifndef ADAPTER_RELEASE_OLD_CLASS_H
#define ADAPTER_RELEASE_OLD_CLASS_H

#include <iostream>
#include "adaptee_base.h"

namespace release {
class OldClass : public AdapteeBase {
 public:
  void foo(const int data) override { std::cout << "OldClass::foo\n"; }
  int bar() override {
    std::cout << "OldClass::bar\n";
    return 2;
  }
};
}  // namespace release
#endif