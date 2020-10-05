#ifndef SINGLETON_RELEASE_SINGLETON_H
#define SINGLETON_RELEASE_SINGLETON_H

#include <atomic>
#include <mutex>

namespace release {
class Singleton {
 public:
  static Singleton* GetInstance();

 private:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  static std::atomic<Singleton*> instance_;
  static std::mutex mutex_;
};
}  // namespace release
#endif