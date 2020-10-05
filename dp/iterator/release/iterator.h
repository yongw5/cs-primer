#ifndef ITERATOR_RELEASE_ITERATOR_H
#define ITERATOR_RELEASE_ITERATOR_H

namespace release {
template <typename T>
class Iterator {
 public:
  virtual void First() = 0;
  virtual void Next() = 0;
  virtual void IsDone() const = 0;
  virtual T& Current() = 0;
};
}  // namespace release

#endif