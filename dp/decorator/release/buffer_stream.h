#ifndef DECORATOR_RELEASE_BUFFER_STREAM_H
#define DECORATOR_RELEASE_BUFFER_STREAM_H

#include "decorator_stream.h"

namespace release {
class BufferStream : public DerocatorStream {
 public:
  BufferStream(Stream *stream) : DerocatorStream(stream) {}
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace release

#endif
