#ifndef COMPOSITE_RELEASE_COMPOSITE_H
#define COMPOSITE_RELEASE_COMPOSITE_H

#include <iostream>
#include <list>
#include <string>
#include "component.h"

namespace release {
class Composite : public Component {
 public:
  explicit Composite(std::string& name) : name_(name) {}
  explicit Composite(const char* name) : name_(name) {}
  void Add(Component* element) { elements_.push_back(element); }
  void Remove(Component* element) {
    auto target = elements_.end();
    for (auto it = elements_.begin(); it != elements_.end(); ++it) {
      if (*it == element) {
        target = it;
        break;
      }
      elements_.erase(target);
    }
  }
  void Process() override {
    std::cout << "Composite(" << name_ << ")::Process\n";
    for (auto& e : elements_) {
      e->Process();
    }
  }

 private:
  std::string name_;
  std::list<Component*> elements_;
};
}  // namespace release

#endif