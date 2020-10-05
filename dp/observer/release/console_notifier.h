#ifndef OBSERVER_RELEASE_NOTIFIER_H
#define OBSERVER_RELEASE_NOTIFIER_H
#include "iprogress.h"

namespace release {
class ConsoleNotifier : public IProgress {
 public:
  ConsoleNotifier() = default;
  void DoProgress(float value);
  ~ConsoleNotifier(){};
};
}  // namespace release

#endif
