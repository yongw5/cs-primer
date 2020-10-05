#ifndef DECORATOR_RELEASE_DECORATOR_STREAM_H
#define DECORATOR_RELEASE_DECORATOR_STREAM_H

#include "stream.h"
namespace release {
class DerocatorStream : public Stream {
 public:
  DerocatorStream(Stream* stream) : stream_(stream) {}

 protected:
  Stream* stream_;
};
}  // namespace release
#endif