#ifndef DECORATOR_RELEASE_CRYPTO_STREAM_H
#define DECORATOR_RELEASE_CRYPTO_STREAM_H

#include "decorator_stream.h"

namespace release {
class CryptoStream : public DerocatorStream {
 public:
  CryptoStream(Stream *stream) : DerocatorStream(stream) {}
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace release

#endif
