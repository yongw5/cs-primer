#ifndef DECORATOR_ALPHA_MEMORY_STREAM_H
#define DECORATOR_ALPHA_MEMORY_STREAM_H

#include "stream.h"
namespace alpha {
class MemoryStream : public Stream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace alpha

#endif
