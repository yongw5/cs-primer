#ifndef OBSERVER_BETA_PROGRESS_BAR_H
#define OBSERVER_BETA_PROGRESS_BAR_H

#include <iostream>

namespace beta {
class ProgressBar {
 public:
  ProgressBar() {}
  void SetValue(float value) const;
  ~ProgressBar() {}
};
}  // namespace beta
#endif
