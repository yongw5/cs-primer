#ifndef OBSERVER_BETA_CONSOLE_H
#define OBSERVER_BETA_CONSOLE_H
#include <iostream>
#include <string>
#include "file_splitter.h"

namespace beta {
class ProgressBar;
class Console {
 public:
  Console(ProgressBar* bar) : bar_(bar) {}
  void ButtonClick();
  ~Console() {}

 private:
  std::string get_text(const char* ptr = NULL);
  ProgressBar* bar_;
};
}  // namespace beta

#endif
