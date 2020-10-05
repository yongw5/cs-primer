#ifndef DECORATOR_RELEASE_NETWORK_STREAM_H
#define DECORATOR_RELEASE_NETWORK_STREAM_H

#include "stream.h"
namespace release {
class NetworkStream : public Stream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace release

#endif
