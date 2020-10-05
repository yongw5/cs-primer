#ifndef OBSERVER_RELEASE_CONSOLE_H
#define OBSERVER_RELEASE_CONSOLE_H
#include <iostream>
#include <string>
#include "iprogress.h"

namespace release {
class ProgressBar;
class Console : public IProgress {
 public:
  Console(ProgressBar* bar) : bar_(bar) {}
  void ButtonClick();
  void DoProgress(float value) override;
  ~Console() {}

 private:
  std::string get_text(const char* ptr = NULL);
  ProgressBar* bar_;
};
}  // namespace release

#endif
