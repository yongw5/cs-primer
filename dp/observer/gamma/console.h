#ifndef OBSERVER_GAMMA_CONSOLE_H
#define OBSERVER_GAMMA_CONSOLE_H
#include <iostream>
#include <string>
#include "iprogress.h"

namespace gamma {
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
}  // namespace gamma

#endif
