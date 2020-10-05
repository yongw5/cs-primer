#ifndef ADAPTER_RELEASE_TARGET_BASE_H
#define ADAPTER_RELEASE_TARGET_BASE_H

namespace release {
class TargetBase {
 public:
  virtual void Process() = 0;
};
}  // namespace release
#endif