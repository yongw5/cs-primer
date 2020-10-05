#include "singleton.h"

namespace alpha {
Singleton* Singleton::instance_ = nullptr;

/// 非线程安全版本
Singleton* Singleton::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new Singleton();
  }
  return instance_;
}
}  // namespace alpha