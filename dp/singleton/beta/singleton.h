#ifndef SINGLETON_BETA_SINGLETON_H
#define SINGLETON_BETA_SINGLETON_H

#include <mutex>

namespace beta {
class Singleton {
 public:
  static Singleton* GetInstance();

 private:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  static Singleton* instance_;
  static std::mutex mutex_;
};
}  // namespace beta
#endif