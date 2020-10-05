#ifndef STATE_RELEASE_NETOWRK_STATE_H
#define STATE_RELEASE_NETOWRK_STATE_H

namespace release {
class NetworkState {
 public:
  virtual void Operation1() = 0;
  virtual void Operation2() = 0;
  NetworkState* next() const { return next_; }
  void set_next(NetworkState* next) { next_ = next; }
  ~NetworkState() = default;

 private:
  NetworkState* next_;
};
}  // namespace release
#endif