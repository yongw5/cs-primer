#include "singleton.h"

namespace beta {
Singleton* Singleton::instance_ = nullptr;
std::mutex Singleton::mutex_;

/// 加锁，线程安全，但锁的代价过高
Singleton* Singleton::GetInstance() {
  std::lock_guard<std::mutex> guard(mutex_);
  if (instance_ == nullptr) {
    instance_ = new Singleton();
  }
  return instance_;
}
}  // namespace beta