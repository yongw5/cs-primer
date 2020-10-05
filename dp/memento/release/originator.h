#ifndef MEMENTO_RELEASE_ORIGINATOR_H
#define MEMENTO_RELEASE_ORIGINATOR_H

#include <string>
#include "memento.h"
namespace release {
class Originator {
 public:
  Originator() = default;
  Memento CreateMemento() {
    Memento m(state_);
    return m;
  }
  void SetMemento(const Memento& m) { state_ = m.state(); }

 private:
  std::string state_;
};
}  // namespace release
#endif