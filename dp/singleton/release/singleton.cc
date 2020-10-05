#include "singleton.h"

namespace release {
std::atomic<Singleton*> Singleton::instance_;
std::mutex Singleton::mutex_;

/// C++11
Singleton* Singleton::GetInstance() {
  Singleton* tmp = instance_.load(std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_acquire);  // 获取内存屏障
  if (tmp == nullptr) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (tmp == nullptr) {
      tmp = new Singleton();
      std::atomic_thread_fence(std::memory_order_release);  // 释放内存屏障
      instance_.store(tmp, std::memory_order_relaxed);
    }
  }
  return tmp;
}
}  // namespace release