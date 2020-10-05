#ifndef OBSERVER_RELEASE_IPROGRESS_H
#define OBSERVER_RELEASE_IPROGRESS_H

namespace release {
class IProgress {
 public:
  IProgress() {}
  virtual void DoProgress(float value) = 0;
  virtual ~IProgress() {}
};
}  // namespace release
#endif
