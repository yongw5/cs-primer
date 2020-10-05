#ifndef ITERATOR_RELEASE_COLLECTION_H
#define ITERATOR_RELEASE_COLLECTION_H

#include "iterator.h"
namespace release {
template <typename T>
class Collection {
 public:
  Iterator<T> CreateIterator() {}
};
}  // namespace release

#endif