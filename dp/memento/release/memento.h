#ifndef MEMENTO_RELEASE_MEMENTO_H
#define MEMENTO_RELEASE_MEMENTO_H

#include <string>
namespace release {
class Memento {
 public:
  Memento(const std::string& state) : state_(state) {}
  std::string state() const { return state_; }
  void set_state(const std::string& state) { state_ = state; }

 private:
  std::string state_;
};
}  // namespace release
#endif