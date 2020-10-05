#ifndef OBSERVER_GAMMA_IPROGRESS_H
#define OBSERVER_GAMMA_IPROGRESS_H

namespace gamma {
class IProgress {
 public:
  IProgress() {}
  virtual void DoProgress(float value) = 0;
  virtual ~IProgress() {}
};
}
#endif 
