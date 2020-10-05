#ifndef COMPOSITE_RELEASE_LEAF_H
#define COMPOSITE_RELEASE_LEAF_H

#include <iostream>
#include <string>
#include "component.h"

namespace release {
class Leaf : public Component {
 public:
  Leaf(std::string& name) : name_(name) {}
  Leaf(const char* name) : name_(name) {}
  void Process() override { std::cout << "Leaf(" << name_ << ")::Process\n"; }

 private:
  std::string name_;
};
}  // namespace release

#endif