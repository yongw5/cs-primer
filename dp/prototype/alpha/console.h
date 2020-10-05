#ifndef PROTOTYPE_ALPHA_CONSOLE_H
#define PROTOTYPE_ALPHA_CONSOLE_H

#include <string>
#include "splitter_base.h"

namespace alpha {
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
}  // namespace alpha
#endif