#ifndef SINGLETON_ALPHA_SINGLETON_H
#define SINGLETON_ALPHA_SINGLETON_H

namespace alpha {
class Singleton {
 public:
  static Singleton* GetInstance();

 private:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  static Singleton* instance_;
};
}  // namespace alpha
#endif