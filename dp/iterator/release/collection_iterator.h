#ifndef ITERATOR_RELEASE_COLLECTION_ITERATOR_H
#define ITERATOR_RELEASE_COLLECTION_ITERATOR_H

#include "collection.h"
#include "iterator.h"

namespace release {
template <typename T>
class CollectionIterator : public Iterator<T> {
 public:
  CollectionIterator(const Collection<T>& collection)
      : collection_(collection) {}
  void First() override {}
  void Next() override {}
  void IsDone() override {}
  T& Current() override {}

 private:
  Collection<T> collection_;
};
}  // namespace release

#endif