#include "progress_bar.h"
#include <sstream>

namespace beta {
void ProgressBar::SetValue(float value) const {
  std::ostringstream ostr;
  const int kTotal = 50;
  int num = int(value * kTotal);
  for (int i = 0; i < num; ++i) {
    ostr << '>';
  }
  for (int i = num; i < kTotal; ++i) {
    ostr << '_';
  }
  std::cout << '\r' << ostr.str();
  std::cout.flush();
}
}  // namespace beta
