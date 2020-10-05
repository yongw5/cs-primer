#ifndef FACTORY_METHOD_RELEASE_CONSOLE_H
#define FACTORY_METHOD_RELEASE_CONSOLE_H

#include <string>
#include "splitter_base.h"

namespace release {
class SplitterFactory;
class Console {
 public:
  Console(SplitterFactory* factory) : factory_(factory) {}
  void ButtonClick();
  ~Console() {}

 private:
  static std::string get_text(const char* ptr = NULL);
  SplitterFactory* factory_;
};
}  // namespace release
#endif