#include "singleton.h"

namespace gamma {
Singleton* Singleton::instance_ = nullptr;
std::mutex Singleton::mutex_;

/// 双检查锁，但由于内存读写 reorder 不安全
Singleton* Singleton::GetInstance() {
  if (instance_ == nullptr) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (instance_ == nullptr) {
      instance_ = new Singleton();
    }
  }
  return instance_;
}
}  // namespace gamma

/**
 * Line 12
 *   instance_ = new Singleton();
 * can divide into three steps by default:
 * 1. Singleton* mem =
 *      static_cast<Singleton*>(operator new(sizeof(Singleton)));
 * 2. mem->Singleton::Singleton();
 * 3. instance_ = mem;
 * 
 * But, if it is reordered, it can be:
 * 1. Singleton* mem =
 *      static_cast<Singleton*>(operator new(sizeof(Singleton)));
 * 2. instance_ = mem;
 * 3. mem->Singleton::Singleton();
 * 
 * So, instance_ is not null but is a raw pointer between 2 and 3.
 */