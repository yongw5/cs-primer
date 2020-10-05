#ifndef OBSERVER_GAMMA_PROGRESS_BAR_H
#define OBSERVER_GAMMA_PROGRESS_BAR_H

#include <iostream>

namespace gamma {
class ProgressBar {
 public:
  ProgressBar() {}
  void SetValue(float value) const;
  ~ProgressBar() {}
};
}  // namespace gamma
#endif
