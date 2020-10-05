#ifndef SINGLETON_GAMMA_SINGLETON_H
#define SINGLETON_GAMMA_SINGLETON_H

#include <mutex>
namespace gamma {
class Singleton {
 public:
  static Singleton* GetInstance();

 private:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  static Singleton* instance_;
  static std::mutex mutex_;
};
}  // namespace gamma
#endif