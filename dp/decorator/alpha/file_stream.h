#ifndef DECORATOR_ALPHA_FILE_STREAM_HPP_
#define DECORATOR_ALPHA_FILE_STREAM_HPP_

#include "stream.h"

namespace alpha {
class FileStream : public Stream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace alpha
#endif
