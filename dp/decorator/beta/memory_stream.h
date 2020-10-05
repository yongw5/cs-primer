#ifndef DECORATOR_BETA_MEMORY_STREAM_H
#define DECORATOR_BETA_MEMORY_STREAM_H

#include "stream.h"
namespace beta {
class MemoryStream : public Stream {
 public:
  char Read(int number)  override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace beta

#endif
