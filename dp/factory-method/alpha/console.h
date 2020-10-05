#ifndef FACTORY_METHOD_ALPHA_CONSOLE_H
#define FACTORY_METHOD_ALPHA_CONSOLE_H

#include <iostream>
#include <string>
#include "splitter_base.h"

namespace alpha {
class Console {
 public:
  Console() {}
  void ButtonClick();
  ~Console() {}

 private:
  static std::string get_text(const char *ptr = NULL);
};
}  // namespace alpha
#endif