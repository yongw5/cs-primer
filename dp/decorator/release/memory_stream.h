#ifndef DECORATOR_RELEASE_MEMORY_STREAM_H
#define DECORATOR_RELEASE_MEMORY_STREAM_H

#include "stream.h"
namespace release {
class MemoryStream : public Stream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace release

#endif
