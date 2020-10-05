#ifndef DECORATOR_ALPHA_NETWORK_STREAM_H
#define DECORATOR_ALPHA_NETWORK_STREAM_H

#include "stream.h"

namespace alpha {
class NetworkStream : public Stream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace alpha

#endif
