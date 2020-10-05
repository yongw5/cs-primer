#include "console.h"
#include <cstdlib>
#include <iostream>
#include "splitter_base.h"
using std::string;

namespace release {
void Console::ButtonClick() {
  SplitterBase* splitter = prototype_->Clone();
  splitter->Split();
  delete splitter;
}
}  // namespace release