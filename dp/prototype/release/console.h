#ifndef PROTOTYPE_RELEASE_CONSOLE_H
#define PROTOTYPE_RELEASE_CONSOLE_H

#include <string>

namespace release {
class SplitterBase;
class Console {
 public:
  Console(SplitterBase* prototype) : prototype_(prototype) {}
  void ButtonClick();
  ~Console() {}

 private:
  SplitterBase* prototype_;
};
}  // namespace release
#endif