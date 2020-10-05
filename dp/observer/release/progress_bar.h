#ifndef OBSERVER_RELEASE_PROGRESS_BAR_H
#define OBSERVER_RELEASE_PROGRESS_BAR_H

#include <iostream>

namespace release {
class ProgressBar {
 public:
  ProgressBar() {}
  void SetValue(float value) const;
  ~ProgressBar() {}
};
}  // namespace release
#endif
