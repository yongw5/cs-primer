#ifndef DECORATOR_BETA_BUFFER_STREAM_H
#define DECORATOR_BETA_BUFFER_STREAM_H

#include "stream.h"

namespace beta {
class BufferStream : public Stream {
 public:
  BufferStream(Stream *stream) : stream_(stream) {}
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;

 private:
  Stream *stream_;
};
}  // namespace beta

#endif
