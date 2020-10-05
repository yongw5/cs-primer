#ifndef OBSERVER_ALPHA_CONSOLE_H
#define OBSERVER_ALPHA_CONSOLE_H
#include <iostream>
#include <string>
#include "file_splitter.h"

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
